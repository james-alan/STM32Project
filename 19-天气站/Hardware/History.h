#ifndef __HISTORY_H
#define __HISTORY_H

#include "stm32f10x.h"

/* 一条历史记录，固定 8 字节（8 能整除页大小 256，所以不会跨页） */
typedef struct {
    uint32_t timestamp;   /* RTC 计数器值（秒） */
    int16_t  temp_x10;    /* 温度 ×10，例如 253 = 25.3℃ */
    uint8_t  light;       /* 光照 0~100 */
    uint8_t  reserved;    /* 填充，保持 8 字节 */
} History_Record;

/* 编译期检查结构体确实是 8 字节，防止以后改字段把布局搞乱 */
typedef char History_Record_SizeCheck[(sizeof(History_Record) == 8) ? 1 : -1];

void     History_Init(void);                                  /* 开机扫描，算出已存条数 */
uint32_t History_Count(void);                                 /* 当前条数 */
void     History_Add(uint32_t timestamp, int16_t temp_x10, uint8_t light);
uint8_t  History_Get(uint32_t index, History_Record *rec);    /* 返回 1 表示有效 */
void     History_Clear(void);                                 /* 擦除整个历史区 */

/* ---------- 系统时间存档（扇区 0，追加式写入） ----------
 * 没有 RTC 电池时，掉电后用来恢复最后一次已知时间 */
void     History_SaveTime(uint32_t sec);
uint32_t History_LoadTime(void);        /* 返回 0 表示没有有效存档 */

#endif