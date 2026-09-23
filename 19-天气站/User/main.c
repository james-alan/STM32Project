/**
 * @file    main.c
 * @brief   多功能桌面气象站 - 实时数据 + 历史记录
 *
 * 第 1 页（实时）：温度 / 光照 / 时间
 * 第 2 页（历史）：历史记录条数 + 最近 3 条
 *
 * 按键 1（PB1）  短按 -> 切换页面
 * 按键 2（PB11） 长按 2 秒 -> 清空历史
 *
 * 蓝灯 PA2 = TIM2_CH3 呼吸灯；红灯 PA3 = 报警指示
 */

#include "stm32f10x.h"
#include "Delay.h"

#include "OLED.h"
#include "Key.h"
#include "Buzzer.h"
#include "LED.h"
#include "PWM.h"
#include "Serial.h"

#include "LightSenser.h"
#include "Thermistor.h"
#include "W25Q64.h"
#include "MyRTC.h"
#include "History.h"

/* 报警阈值 */
#define ALARM_TEMP_HIGH   40.0f
#define ALARM_LIGHT_HIGH  95
#define ALARM_LIGHT_LOW   5

/* 历史记录间隔（秒）。调试时可以改小，正式使用建议 600（10 分钟） */
#define HIST_INTERVAL_SEC 60

/* 报警状态：1 = 报警中 */
uint8_t g_alarm;

/* 当前页面：1 = 实时数据，2 = 历史记录 */
uint8_t g_page;

/* 画那些不会变的内容（清屏后要重画一次） */
static void DrawStatic(void)
{
    OLED_ShowString(1, 1, "Weather v1.0");
    OLED_ShowChar(4, 3, ':');
    OLED_ShowChar(4, 6, ':');
}

/* 第 1 页：实时数据 */
static void ShowPageLive(float tempC, uint8_t lightPct)
{
    OLED_ShowString(2, 1, "T:");
    OLED_ShowSignedNum(2, 3, (int)(tempC * 10), 4);
    OLED_ShowChar(2, 8, 'C');

    OLED_ShowString(3, 1, "L:");
    OLED_ShowNum(3, 3, lightPct, 3);
    OLED_ShowChar(3, 7, '%');

    OLED_ShowNum(4, 1, MyRTC_Time[3], 2);   /* 时 */
    OLED_ShowNum(4, 4, MyRTC_Time[4], 2);   /* 分 */
    OLED_ShowNum(4, 7, MyRTC_Time[5], 2);   /* 秒 */
}

/* 第 2 页：历史记录（总数 + 最近三条） */
static void ShowPageHistory(void)
{
    uint32_t total = History_Count();
    uint32_t i, start;
    uint8_t  line;
    History_Record r;

    OLED_ShowString(1, 1, "History");
    OLED_ShowNum(1, 9, total, 5);

    start = (total > 3) ? (total - 3) : 0;

    for (i = start, line = 2; line <= 4; i++, line++)
    {
        if (i >= total)
        {
            OLED_ShowString(line, 1, "                ");   /* 清空整行 */
            continue;
        }

        History_Get(i, &r);

        OLED_ShowChar(line, 1, '[');
        OLED_ShowNum(line, 2, i, 2);
        OLED_ShowChar(line, 4, ']');

        OLED_ShowNum(line, 6, (uint16_t)(r.temp_x10 / 10), 2);
        OLED_ShowChar(line, 8, '.');
        OLED_ShowNum(line, 9, (uint16_t)(r.temp_x10 % 10), 1);
        OLED_ShowChar(line, 10, 'C');

        OLED_ShowNum(line, 12, r.light, 3);
        OLED_ShowChar(line, 15, '%');
    }
}

int main(void)
{
    uint32_t tick = 0;
    uint32_t lastRecordSec = 0;

    /* 呼吸灯状态：亮度 0~100 往返扫描 */
    int16_t breath = 0;
    int8_t  breathDir = 1;

    g_page = 1;          /* 开机默认显示实时数据页（显式赋值，不依赖 RW 初值） */

    /* 系统初始化 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* 模块初始化 */
    OLED_Init();
    Key_init();
    Buzzer_init();
    LED_init();
    PWM_Init();          /* 呼吸灯：TIM2_CH3 -> PA2 */

    Serial_Init();       /* 调试用 */
    LightSenser_Init();
    Thermistor_Init();   /* ADC1 主初始化（LightSenser 共享 ADC1） */
    W25Q64_Init();
    MyRTC_Init();        /* 实时时钟：首次上电用编译时间做初值 */
    History_Init();      /* 扫描历史区，算出已存条数 */

    /* 如果刚才是首次配置 RTC（说明备份域掉电了），就用 Flash 里存的
     * 最后一次时间恢复。没有存档就保持编译时间。 */
    if (MyRTC_IsFirstBoot)
    {
        uint32_t saved = History_LoadTime();
        if (saved != 0) MyRTC_SetFromCounter(saved);
    }

    /* 开机自检：读一次 W25Q64 的 JEDEC ID（正常应为 0xEF4017），结果不再显示 */
    (void)W25Q64_ReadID();

    OLED_Clear();
    DrawStatic();

    while (1)
    {
        /* ---- 按键：非阻塞扫描，每 ~10ms 一次 ---- */
        Key_Tick();

        if (Key_GetShortPress(1))            /* 按键1 短按：翻页 */
        {
            g_page = (g_page == 1) ? 2 : 1;
            OLED_Clear();
            DrawStatic();
        }
        if (Key_GetLongPress(2))             /* 按键2 长按2秒：清空历史 */
        {
            History_Clear();
            OLED_Clear();
            DrawStatic();
        }

        /* ---- 每 ~500ms 采集并刷新一次 ---- */
        if (++tick > 50)
        {
            tick = 0;

            float    tempC    = Thermistor_ReadTemp();
            uint16_t tempRaw  = Thermistor_ReadADC();
            uint8_t  lightPct = LightSenser_ReadPercent();

            MyRTC_ReadTime();

            /* 报警判断：温度过高 / 光照过亮或过暗 -> 红灯(LED2 / PA3)亮 */
            g_alarm = (tempC > ALARM_TEMP_HIGH ||
                       lightPct > ALARM_LIGHT_HIGH ||
                       lightPct < ALARM_LIGHT_LOW) ? 1 : 0;
            if (g_alarm) { LED2_ON(); } else { LED2_OFF(); }

            /* 按间隔存一条历史 */
            {
                uint32_t nowSec = RTC_GetCounter();
                if (nowSec - lastRecordSec >= HIST_INTERVAL_SEC)
                {
                    lastRecordSec = nowSec;
                    History_Add(nowSec, (int16_t)(tempC * 10.0f), lightPct);
                    History_SaveTime(nowSec);   /* 同时把最后已知时间写进 Flash */
                }
            }

            /* 显示当前页 */
            if (g_page == 1) ShowPageLive(tempC, lightPct);
            else             ShowPageHistory();

            /* 串口上报（调试用） */
            Serial_printf("T=%.1f C  L=%d%%  raw=%d  n=%d\r\n",
                          tempC, lightPct, tempRaw, (int)History_Count());
        }

        /* ---- 呼吸灯：PA2(TIM2_CH3) 亮度 0~100 往返扫描，约 2 秒一个周期 ---- */
        breath += breathDir;
        if (breath >= 100) { breath = 100; breathDir = -1; }
        else if (breath <= 0) { breath = 0; breathDir = 1; }
        PWM_SetCompare3((uint16_t)breath);

        Delay_ms(10);
    }
}