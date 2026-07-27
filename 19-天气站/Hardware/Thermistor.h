#ifndef __THERMISTOR_H
#define __THERMISTOR_H

#include "stm32f10x.h"
#include <stdint.h>

/*============ 引脚分配 ============
 * 热敏电阻分压输出 AO -> PA1 (ADC1_IN1)
 * 模块接线：VCC->3.3V  GND->GND  AO->PA1  其余不用
 * =================================*/

/* NTC 热敏电阻参数（10KΩ @25℃, B = 3950） */
#define NTC_R_REF       10000.0f
#define NTC_T_REF       298.15f      /* 25℃ + 273.15 */
#define NTC_B           3950.0f
#define NTC_R_FIXED     10000.0f     /* 板上串联的固定电阻 R1 */
#define NTC_ADC_VREF    3.3f
#define NTC_ADC_MAX     4095.0f

void    Thermistor_Init(void);
uint16_t Thermistor_ReadADC(void);                /* 读原始 ADC 值 (0-4095) */
float   Thermistor_ToTemp(uint16_t adc);          /* ADC → 摄氏度 */
float   Thermistor_ReadTemp(void);                /* 直接读一次温度（℃） */

#endif
