#ifndef __MYRTC_H
#define __MYRTC_H

#include "stm32f10x.h"

/* 时间数组：年 月 日 时 分 秒 */
extern uint16_t MyRTC_Time[6];

/* RTC 时钟源：1 = HSE/128（8MHz 主晶振），0 = LSI */
extern uint8_t MyRTC_ClockSource;

/* 1 = 这次上电首次配置了 RTC（备份域掉电了），调用者可以据此恢复时间 */
extern uint8_t MyRTC_IsFirstBoot;

void     MyRTC_Init(void);
void     MyRTC_SetTime(void);
void     MyRTC_ReadTime(void);
void     MyRTC_SetFromCounter(uint32_t sec);   /* 用外部保存的时间覆盖 RTC */

#endif