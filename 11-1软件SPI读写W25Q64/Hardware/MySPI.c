#include "stm32f10x.h"
#include "MySPI.h"

// 驱动层

void MySPI_W_SS(uint8_t BitValue)
{
    GPIO_WriteBit(GPIOA,GPIO_Pin_4,(BitAction)BitValue);
}

void MySPI_W_SCK(uint8_t BitValue)
{
    GPIO_WriteBit(GPIOA,GPIO_Pin_5,(BitAction)BitValue);

}

void MySPI_W_MOSI(uint8_t BitValue)
{
    GPIO_WriteBit(GPIOA,GPIO_Pin_7,(BitAction)BitValue);
}

uint8_t MySPI_R_MISO()
{
    return GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_6);
}

// 引脚 PinA4、5、6、7
void MySPI_Init()
{

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitSturcture;
    GPIO_InitSturcture.GPIO_Mode = GPIO_Mode_Out_PP; // SS、SCK、MOSI 输出 使用推挽输出
    GPIO_InitSturcture.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_7;
    GPIO_InitSturcture.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitSturcture);

    GPIO_InitSturcture.GPIO_Mode = GPIO_Mode_IPU; // MISO 输入 使用上拉输入
    GPIO_InitSturcture.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitSturcture.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitSturcture);

    //最后设置默认电平
    MySPI_W_SS(1);
    MySPI_W_SCK(0);

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
    uint8_t i,ByteReceive=0x00;

    for(i=0;i<8;i++){
        //发送一个字节
        MySPI_W_MOSI(ByteSend&0x80>>i);
        MySPI_W_SCK(1);
        //收一个字节
        if(MySPI_R_MISO()==1){
            ByteReceive |= (0x80 >> i);
        }

        MySPI_W_SCK(0);
    }

    return ByteReceive;

}
