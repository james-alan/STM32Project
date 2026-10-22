#include "stm32f10x.h"
#include <stdio.h>
#include <stdarg.h>
#include "Serial.h"

/* 串口配置：USART1，PA9=TX / PA10=RX，115200-8-N-1 */

#define SERIAL_RX_SIZE 64

static volatile uint8_t Serial_RxBuf[SERIAL_RX_SIZE];
static volatile uint8_t Serial_RxHead;
static volatile uint8_t Serial_RxTail;

void Serial_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	/* TX：复用推挽输出 */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* RX：上拉输入 */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART1, &USART_InitStructure);

	/* 打开接收中断，命令是一个字符一个字符收的，光靠主循环轮询会丢字节 */
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_Cmd(USART1, ENABLE);
}

void Serial_SendByte(uint8_t Byte)
{
	USART_SendData(USART1, Byte);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
	{
	}
}

void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
	uint16_t i;

	for (i = 0; i < Length; i++)
	{
		Serial_SendByte(Array[i]);
	}
}

void Serial_SendString(char *String)
{
	uint16_t i;

	for (i = 0; String[i] != '\0'; i++)
	{
		Serial_SendByte(String[i]);
	}
}

/* 把 printf 重定向到串口，Serial_printf 内部就能直接用 vsprintf 了 */
int fputc(int ch, FILE *f)
{
	Serial_SendByte((uint8_t)ch);
	return ch;
}

void Serial_printf(char *format, ...)
{
	char String[128];
	va_list arg;

	va_start(arg, format);
	vsprintf(String, format, arg);
	va_end(arg);

	Serial_SendString(String);
}

uint8_t Serial_GetChar(char *Char)
{
	if (Serial_RxHead == Serial_RxTail)
	{
		return 0;
	}

	*Char = (char)Serial_RxBuf[Serial_RxTail];
	Serial_RxTail = (uint8_t)((Serial_RxTail + 1) % SERIAL_RX_SIZE);
	return 1;
}

void USART1_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
	{
		uint8_t data = (uint8_t)USART_ReceiveData(USART1);
		uint8_t next = (uint8_t)((Serial_RxHead + 1) % SERIAL_RX_SIZE);

		if (next != Serial_RxTail)
		{
			Serial_RxBuf[Serial_RxHead] = data;
			Serial_RxHead = next;
		}
	}
}
