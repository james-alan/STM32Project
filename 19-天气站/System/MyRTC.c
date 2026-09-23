#include "stm32f10x.h"
#include "MyRTC.h"
#include "Delay.h"
#include <time.h>

/* 时间数组：年 月 日 时 分 秒
 * 故意不在这里给初值 —— 带初值的全局变量会落进 RW 段，初值存在
 * Flash 加载区，某些烧录方式（如 gdb load）写不到那里。
 * 初值改在 MyRTC_Init() 里显式赋值。 */
uint16_t MyRTC_Time[6];

/* RTC 时钟源：1 = HSE/128，0 = LSI */
uint8_t MyRTC_ClockSource = 0;

/* 本次上电是否首次配置了 RTC（备份域掉电） */
uint8_t MyRTC_IsFirstBoot = 0;

void MyRTC_SetTime(void);

/*================================================================
 *  把编译时刻拿来当 RTC 的初始时间
 *
 *  用编译器内置的 __DATE__ / __TIME__：
 *      __DATE__ = "Sep 22 2026"   （日是一位数时前面补空格）
 *      __TIME__ = "17:48:46"
 *  编译完马上烧录，时间就是当前真实时间，不需要串口对时。
 *================================================================*/
static uint8_t MonthFromName(const char *m)
{
    static const char names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    uint8_t i;
    for (i = 0; i < 12; i++)
    {
        if (m[0] == names[i * 3] &&
            m[1] == names[i * 3 + 1] &&
            m[2] == names[i * 3 + 2]) return i + 1;
    }
    return 1;
}

static void ApplyBuildTime(void)
{
    char d0 = __DATE__[4];
    char d1 = __DATE__[5];

    MyRTC_Time[0] = (uint16_t)((__DATE__[7] - '0') * 1000 + (__DATE__[8] - '0') * 100
                             + (__DATE__[9] - '0') * 10   + (__DATE__[10] - '0'));  /* 年 */
    MyRTC_Time[1] = MonthFromName(__DATE__);                                          /* 月 */
    MyRTC_Time[2] = (d0 == ' ') ? (uint16_t)(d1 - '0')
                                : (uint16_t)((d0 - '0') * 10 + (d1 - '0'));           /* 日 */
    MyRTC_Time[3] = (uint16_t)((__TIME__[0] - '0') * 10 + (__TIME__[1] - '0'));       /* 时 */
    MyRTC_Time[4] = (uint16_t)((__TIME__[3] - '0') * 10 + (__TIME__[4] - '0'));       /* 分 */
    MyRTC_Time[5] = (uint16_t)((__TIME__[6] - '0') * 10 + (__TIME__[7] - '0'));       /* 秒 */
}

/*================================================================
 *  带超时的 RTC 寄存器同步等待
 *  库里的 RTC_WaitForSynchro() 是 while 死等，RTC 拿不到时钟就会卡死。
 *================================================================*/
static uint8_t RTC_WaitSyncBounded(uint32_t ms)
{
    RTC->CRL &= (uint16_t)~RTC_FLAG_RSF;
    while (ms--)
    {
        if (RTC->CRL & RTC_FLAG_RSF) return 1;
        Delay_ms(1);
    }
    return 0;
}

void MyRTC_Init(void)
{
    uint32_t i;

    MyRTC_ClockSource = 0;      /* 显式赋值，不依赖 RW 初值 */
    MyRTC_IsFirstBoot = 0;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);
    PWR_BackupAccessCmd(ENABLE);

    if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
    {
        uint8_t ok = 0;
        MyRTC_IsFirstBoot = 1;

        /* ---------- 1) 首选 HSE/128 ---------- */
        if (RCC_GetFlagStatus(RCC_FLAG_HSERDY) != RESET)
        {
            RCC_BackupResetCmd(ENABLE);      /* RTCSEL 只能在备份域复位后写一次 */
            RCC_BackupResetCmd(DISABLE);
            RCC_RTCCLKConfig(RCC_RTCCLKSource_HSE_Div128);
            RCC_RTCCLKCmd(ENABLE);
            if (RTC_WaitSyncBounded(200))
            {
                RTC_WaitForLastTask();
                RTC_SetPrescaler(62500 - 1);
                RTC_WaitForLastTask();
                MyRTC_ClockSource = 1;
                ok = 1;
            }
        }

        /* ---------- 2) 退回 LSI ---------- */
        if (!ok)
        {
            RCC_BackupResetCmd(ENABLE);
            RCC_BackupResetCmd(DISABLE);
            RCC_LSICmd(ENABLE);
            for (i = 0; i < 200; i++)
            {
                if (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) != RESET) break;
                Delay_ms(10);
            }
            RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
            RCC_RTCCLKCmd(ENABLE);
            if (RTC_WaitSyncBounded(200))
            {
                RTC_WaitForLastTask();
                RTC_SetPrescaler(40000 - 1);
                RTC_WaitForLastTask();
                MyRTC_ClockSource = 0;
                ok = 1;
            }
        }

        /* ---------- 3) 先用编译时间做初值 ---------- */
        ApplyBuildTime();
        if (ok) MyRTC_SetTime();

        BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
    }
    else
    {
        RTC_WaitForSynchro();
        RTC_WaitForLastTask();
    }
}

/* 用外部保存的时间（Flash 里的存档）覆盖 RTC */
void MyRTC_SetFromCounter(uint32_t sec)
{
    RTC_SetCounter(sec);
    RTC_WaitForLastTask();
    MyRTC_ReadTime();
}

void MyRTC_SetTime(void)
{
    time_t time_cnt;
    struct tm time_date;

    time_date.tm_year = MyRTC_Time[0] - 1900;
    time_date.tm_mon  = MyRTC_Time[1] - 1;
    time_date.tm_mday = MyRTC_Time[2];
    time_date.tm_hour = MyRTC_Time[3];
    time_date.tm_min  = MyRTC_Time[4];
    time_date.tm_sec  = MyRTC_Time[5];

    time_cnt = mktime(&time_date) - 8 * 60 * 60;   /* 东八区 */
    RTC_SetCounter(time_cnt);
    RTC_WaitForLastTask();
}

void MyRTC_ReadTime(void)
{
    time_t time_cnt;
    struct tm time_date;

    time_cnt = RTC_GetCounter() + 8 * 60 * 60;
    time_date = *localtime(&time_cnt);

    MyRTC_Time[0] = time_date.tm_year + 1900;
    MyRTC_Time[1] = time_date.tm_mon + 1;
    MyRTC_Time[2] = time_date.tm_mday;
    MyRTC_Time[3] = time_date.tm_hour;
    MyRTC_Time[4] = time_date.tm_min;
    MyRTC_Time[5] = time_date.tm_sec;
}