#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"
#include <stdio.h>

void Serial_Init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(char *String);
void Serial_printf(char *format, ...);

/* 从接收缓冲里取一个字节，取到返回 1，没有返回 0 */
uint8_t Serial_GetChar(char *Char);

#endif
