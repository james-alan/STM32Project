#include "stm32f10x.h"
#include "MySPI.h"

// 驱动层

void MySPI_W_SS(uint8_t BitValue)
{
    GPIO_WriteBit(GPIOA, GPIO_Pin_4, (BitAction)BitValue);
}

// 引脚 PinA4、5、6、7
void MySPI_Init()
{

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    GPIO_InitTypeDef GPIO_InitSturcture;
    GPIO_InitSturcture.GPIO_Mode = GPIO_Mode_Out_PP; // SS、SCK、MOSI 输出 使用推挽输出
    GPIO_InitSturcture.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitSturcture.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitSturcture);

    GPIO_InitSturcture.GPIO_Mode = GPIO_Mode_AF_PP; // SS、SCK、MOSI 输出 使用推挽输出
    GPIO_InitSturcture.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    GPIO_InitSturcture.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitSturcture);

    GPIO_InitSturcture.GPIO_Mode = GPIO_Mode_IPU; // MISO 输入 使用上拉输入
    GPIO_InitSturcture.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitSturcture.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitSturcture);

    SPI_InitTypeDef SPI_InitStructure;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128; // 128分频
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft; // NSS，选择由软件控制
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);

    SPI_Cmd(SPI1, ENABLE);
    // 最后设置默认电平
    MySPI_W_SS(1);
}

void MySPI_Start()
{
    MySPI_W_SS(0);
}

void MySPI_Stop()
{
    MySPI_W_SS(1);
}

// 交换函数 基于spi的协议 发一个位 必须收一个位
uint8_t MySPI_SwapByte(uint8_t ByteSend)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != SET); // 数据寄存器为空的时候

    SPI_I2S_SendData(SPI1, ByteSend);

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != SET);

    return SPI_I2S_ReceiveData(SPI1);
}
