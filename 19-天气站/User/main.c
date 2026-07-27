/**
 * @file    main.c
 * @brief   多功能桌面气象站 - Task 1 测试版
 * @note    这一版只做模块初始化 + 简单显示验证 + 各模块返回原始值
 *          用于检查每个模块的接线是否正确。
 *
 * 显示内容：
 *      Line 1: 标题
 *      Line 2: 温度 (原始 ADC + 换算值)
 *      Line 3: 光照百分比
 *      Line 4: W25Q64 JEDEC ID
 *
 * 如果一切正常，你会看到：
 *      T:25.3C  L:68  ID:EF4017
 */

#include "stm32f10x.h"
#include "Delay.h"

#include "OLED.h"
#include "Key.h"
#include "Buzzer.h"
#include "LED.h"
#include "Serial.h"

#include "LightSenser.h"
#include "Thermistor.h"
#include "W25Q64.h"

int main(void)
{
    /* 系统初始化 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* 模块初始化 */
    OLED_Init();
    Key_init();
    Buzzer_init();
    LED_init();

    Serial_Init();      /* 调试用 */
    LightSenser_Init();
    Thermistor_Init();  /* ADC1 主初始化（LightSenser 共享 ADC1） */
    W25Q64_Init();

    /* 验证用：读 W25Q64 ID，应当是 0xEF4017 */
    uint16_t w25_id = W25Q64_ReadID();

    /* 清屏并写标题 */
    OLED_Clear();
    OLED_ShowString(1, 1, "Weather v1.0");

    /* 使用 SysTick 反推毫秒计数 */
    uint32_t tick = 0;

    while (1) {
        /* 每 500ms 刷新一次（粗略用循环 + Delay 实现） */
        if (++tick > 50) {
            tick = 0;

            /* 温度 */
            float   tempC = Thermistor_ReadTemp();
            uint16_t tempRaw = Thermistor_ReadADC();

            /* 光照 */
            uint8_t lightPct = LightSenser_ReadPercent();

            /* OLED 显示（行 2, 3） */
            OLED_ShowString(2, 1, "T:");
            OLED_ShowSignedNum(2, 3, (int)(tempC * 10), 4);   /* 显示 1 位小数 */
            OLED_ShowChar(2, 8, 'C');

            OLED_ShowString(3, 1, "L:");
            OLED_ShowNum(3, 3, lightPct, 3);
            OLED_ShowChar(3, 7, '%');

            OLED_ShowString(4, 1, "ID:");
            OLED_ShowHexNum(4, 4, w25_id, 4);

            /* 串口上报（调试用，正式发布时可注释掉 Serial_Init 和这段） */
            Serial_printf("T=%.1f C  L=%d%%  raw=%d\r\n",
                          tempC, lightPct, tempRaw);
        }

        /* 简单按键示例：按 PB1 后蜂鸣器短叫一下 */
        if (Key_GetNum()) {
            Buzzer_OFF();   /* 注意：当前 Buzzer 是低电平触发，OFF = 响 */
            Delay_ms(100);
            Buzzer_ON();
        }

        /* LED 心跳 */
        LED1_turn();
        Delay_ms(10);
    }
}
