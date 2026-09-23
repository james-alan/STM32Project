#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

void Key_init(void);

/* 阻塞式读取（老接口，按住不放会卡住主循环，不推荐再用） */
uint8_t Key_GetNum(void);

/* ---------- 非阻塞按键接口 ---------- */
/* 每 ~10ms 调用一次 Key_Tick()，它只做扫描和状态机，绝不阻塞 */
void    Key_Tick(void);
/* 短按事件：发生过就返回 1 并清掉标志；没发生返回 0 */
uint8_t Key_GetShortPress(uint8_t KeyNum);   /* KeyNum: 1 或 2 */
/* 长按事件：按满 2 秒就置位（按住期间就触发，不用等松手） */
uint8_t Key_GetLongPress(uint8_t KeyNum);

#endif