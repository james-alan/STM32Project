#include "stm32f10x.h"
#include "Delay.h"

// 添加I2C的引脚配置

void MyI2C_W_SCl(uint8_t BitValue)
{
    GPIO_WriteBit(GPIOB, GPIO_Pin_10, (BitAction)BitValue);
    Delay_us(10);
}

void MyI2C_W_SDA(uint8_t BitValue)
{
    GPIO_WriteBit(GPIOB, GPIO_Pin_11, (BitAction)BitValue);
    Delay_us(10);
}

uint8_t MyI2C_R_SDA(void)
{
    uint8_t BitValue;
    BitValue = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11);
    Delay_us(10);
    return BitValue;
}
// 初始化 软件读写 实际上就是gpio的初始化
 void MyI2C_Init(void)
{
    // 开启时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    // GPIO初始化
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; // I2C使用开漏输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    // 默认电平
    GPIO_SetBits(GPIOB, GPIO_Pin_10 | GPIO_Pin_11);
}

// 协议层
// scl在高的时候 sda从高边低
void MyI2C_Start(void)
{
    // MyI2C_W_SCl(1);
    // MyI2C_W_SDA(0);

    // 正确写法
    MyI2C_W_SDA(1);
    MyI2C_W_SCl(1);
    MyI2C_W_SDA(0);
    MyI2C_W_SCl(0);
}
// 停止 scl高的时候 sda从低到高
void MyI2C_Stop(void)
{
    // MyI2C_W_SCl(1);
    // MyI2C_W_SDA(0);

    // 正确写法
    MyI2C_W_SDA(0);
    MyI2C_W_SCl(1);
    MyI2C_W_SDA(1);
}

// 发送  主机在scl低的时候 放数据 sda高的时候保持数据稳定性
//      并且是高位先行
void MyI2C_SendByte(uint8_t Byte)
{

    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        MyI2C_W_SDA(Byte & (0x80 >> i));
        MyI2C_W_SCl(1);
        MyI2C_W_SCl(0);
    }
}
// 读取  和发送数据相反 读取数据需要在scl高的时候 且sda需要被释放
uint8_t MyI2C_ReceiveByte(void)
{
    uint8_t i, Byte = 0x00;
    MyI2C_W_SDA(1); // 主机确保释放sda
    for (i = 0; i < 8; i++)
    {
        MyI2C_W_SCl(1); // 释放scl
        if (MyI2C_R_SDA() == 1)
        {
            Byte |= (0x80 >> i);
        }
        MyI2C_W_SCl(0);
    }
    return Byte;
}
// 发送应答位 等同于发送一位数据 发数据的时候 scl为低
// 范围：0~1，0表示应答，1表示非应答

void MyI2C_SendAck(uint8_t AckBit)
{
    MyI2C_W_SDA(AckBit);
    MyI2C_W_SCl(1); // 从机在1的时候读应答
    MyI2C_W_SCl(0); // 拉低，开始下一个时许
}

// 接受应答
// 范围：0~1，0表示应答，1表示非应答
// 本质上还是收 那就是先释放sda 再在scl高的时候 读取数据
uint8_t MyI2C_ReceiveAck(void)
{
    uint8_t AckBit;
    MyI2C_W_SDA(1);
    MyI2C_W_SCl(1);
    AckBit = MyI2C_R_SDA();
    MyI2C_W_SCl(0);
    return AckBit;
}
