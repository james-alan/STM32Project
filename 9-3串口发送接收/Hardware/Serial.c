#include "stm32f10x.h"
#include <stdio.h>
#include <stdarg.h>

uint8_t Serial_TxPacket[4];
uint8_t Serial_RxPacket[4];
uint8_t Serial_RxFlag;

void Serial_Init()
{
    // 开启时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // GPIO初始化 TX
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // GPIO初始化 RX
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // USART初始化
    USART_InitTypeDef USART_Initstructure;
    USART_Initstructure.USART_BaudRate = 9600;                                      // 波特率
    USART_Initstructure.USART_WordLength = USART_WordLength_8b;                     // 发送字长
    USART_Initstructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 硬件流控
    USART_Initstructure.USART_StopBits = USART_StopBits_1;                          // 停止位
    USART_Initstructure.USART_Parity = USART_Parity_No;                             // 奇偶校验
    USART_Initstructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;                 // 模式 输入和输出
    USART_Init(USART1, &USART_Initstructure);

    // 中断输出配置
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    // 中断分组
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    // 中断设置
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // USART使能
    USART_Cmd(USART1, ENABLE);
}

// 发送字节
void Serial_SendByte(uint8_t Byte)
{
    USART_SendData(USART1, Byte);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
        ; // 等待单个字节发送完成
    // 下次写入数据寄存器会自动清楚发送完成标志位
}

void Serial_SendArray(uint8_t *Array, uint8_t Length)
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

// 发送次方函数
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}
// 数字只能一位一位发
void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0'); //
    }
}

// 重定向printf
int fputc(int ch, FILE *f)
{
    Serial_SendByte(ch);
    return ch;
}

// 自己封装printf
void Serial_printf(char *format, ...)
{
    char String[100];
    va_list arg;
    va_start(arg, format);
    vsprintf(String, format, arg);
    va_end(arg);
    Serial_SendString(String);
}

// 发送HEX串
// 数据包的话 需要加上包头和包尾
void Serial_SendPacket()
{
    Serial_SendByte(0xFF);
    Serial_SendArray(Serial_TxPacket, 4);
    Serial_SendByte(0xFE);
}

// 获取串口结束数据包标识位
uint8_t Serial_GetRxFlag()
{
    if (Serial_RxFlag == 1)
    {
        Serial_RxFlag = 0;
        return 1;
    }
    return 0;
}

// 中断函数
// 中断函数用来区分不同状态下的函数执行
void USART1_IRQHandler(void)
{
    static uint8_t RxState = 0;   // 表示当前状态机的静态变量
    static uint8_t pRxPacket = 0; // 表示当前结束数据位置的静态变量
    // 首先要判断是否是USART1的接收事件触发了中断
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        uint8_t RxData = USART_ReceiveData(USART1);

        if (RxState == 0)
        {
            if (RxData == 0xFF)
            {
                RxState = 1;
                pRxPacket = 0;
            }
        }
        else if (RxState == 1)
        {
            Serial_RxPacket[pRxPacket] = RxData;
            pRxPacket++; // 每接收到一个数据 就会触发一次中断
            if (pRxPacket >= 4)
            {
                RxState = 2;
            }
        }
        else if (RxState == 2)
        {
            if (RxData == 0XFE)
            { // 确定是包尾

                RxState = 0;
                Serial_RxFlag = 1; // 确定成功接收一个数据包
            }
        }

        // 最后清楚中断标识位
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}