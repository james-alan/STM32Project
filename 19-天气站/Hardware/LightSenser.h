#ifndef __LIGHTSENSER_H
#define __LIGHTSENSER_H

#include "stm32f10x.h"
#include <stdint.h>

/*============ 引脚分配 ============
 * 光敏电阻 AO -> PA0 (ADC1_IN0)
 * （原来用 PB13 的 DO 数字量，那只是阈值高低电平，
 *   现在改成 ADC 读模拟量，可以显示光照强度百分比）
 * =================================*/

void     LightSenser_Init(void);
uint16_t LightSenser_ReadADC(void);          /* 读原始 ADC 值 (0-4095) */
uint8_t  LightSenser_ReadPercent(void);      /* 光照百分比 0-100 */

#endif
