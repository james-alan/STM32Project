#include "stm32f10x.h"

int16_t count;

void Encoder_init()
{
    // 开启gpio
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    // 开启afio
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    // 配置gpio
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // afio选择中断引脚
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);

    // 配置exti
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line0 | EXTI_Line1;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_Init(&EXTI_InitStructure);

    // nvic中断分组
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    // 配置nvic
    // 要对两个通道分别设置优先级
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);
}

// 旋转编码器的增量值
int16_t Encoder_Get()
{
    int16_t temp;
    temp = count;
    count = 0;
    return temp;
}

// 正转 A下降沿 B高电平
void EXTI1_IRQHandler()
{
    if (EXTI_GetITStatus(EXTI_Line1) == SET)
    {
        // B0下降沿触发后进入中断

        if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
        {
            if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 0) //B1下降沿触发中断，检测B0的电平
            {
                count++;
            }
        }
    }
    // 清楚中断标志位
    EXTI_ClearITPendingBit(EXTI_Line1);
}

// 反转 (逆时针)
void EXTI0_IRQHandler()
{
    if (EXTI_GetITStatus(EXTI_Line0) == SET)
    {
        // B0下降沿触发后进入中断

        if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 0)
        {
            if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
            {
                // B0下降触发中断，检测B1的电平
                count--;
            }
        }
    }
    // 清楚中断标志位
    EXTI_ClearITPendingBit(EXTI_Line0);
}
