#include "stm32f10x.h"
// 定时器初始化

// // 练习定时器中断 rcc内部时钟
// void Timer_Init()
// {
//     // 初始化流程 开启rtc->配置时钟源->时基单元初始化->中断设置->中断输出控制->nvic优先级设置
//     RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

//     // 选择使用哪个定时器 本设备有高级、通用、基本定时器
//     TIM_InternalClockConfig(TIM2);

//     // 时基单元初始化
//     TIM_TimeBaseInitTypeDef TIM_TimBaseInitStructure;
//     TIM_TimBaseInitStructure.TIM_Prescaler = 7200 - 1;             // 预分频器 即PSC的值 将时钟频率分频  STM32默认内部时钟72MHz
//     TIM_TimBaseInitStructure.TIM_Period = 10000 - 1;               // 计数周期 即ARR的值 自动重装器
//     TIM_TimBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     // 时钟分频 用于配置滤波器参数 选择1分频
//     TIM_TimBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; // 分频模式 选择向上计数
//     TIM_TimBaseInitStructure.TIM_RepetitionCounter = 0;            // 重复计数器 高级定时器才会用到
//     TIM_TimeBaseInit(TIM2, &TIM_TimBaseInitStructure);

//     // 中断设置
//     TIM_ClearFlag(TIM2, TIM_FLAG_Update); // 在TIM_TimeBaseInit函数的末尾产生了一个更新标志位，若不清楚，会默认进入一次中断

//     // 开始配置中断
//     TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

//     // nvic中断配置
//     NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

//     // nvic配置
//     NVIC_InitTypeDef NVIC_InitStructure;
//     NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
//     NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//     NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
//     NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
//     NVIC_Init(&NVIC_InitStructure);

//     // 使能tim
//     TIM_Cmd(TIM2, ENABLE);
// }

// 练习外部时钟
void Timer_Init()
{
    // 初始化流程 开启GPIO->ETR外部时钟->配置时钟模式->配置时基单元->中断输出控制->中断配置
    // 开启GPIO
    // 第一步 开启时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    // gpio初始化
    GPIO_InitTypeDef GPIO_InitSturcture;
    GPIO_InitSturcture.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitSturcture.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitSturcture.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitSturcture);

    // 配置时钟
    // 外部时钟配置
    TIM_ETRClockMode2Config(TIM2, TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_Inverted, 0x0F);
    // 外部时钟2模式 时钟从TIM_ETR引脚输入
    // 最后一个滤波器参数加到最大0x0F，可滤除时钟信号抖动

    // 时基单元初始化
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseInitStructure.TIM_Prescaler = 2 - 1; // 预分频
    TIM_TimeBaseInitStructure.TIM_Period = 10 - 1;   // 计数周期
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0; // 重复计数器
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

    // 中断输出控制
    TIM_ClearFlag(TIM2, TIM_FLAG_Update); // 配置时基单元后

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); // 开启中断更新

    NVIC_InitTypeDef NVIC_InitSturcture;
    NVIC_InitSturcture.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitSturcture.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitSturcture.NVIC_IRQChannelPreemptionPriority = 2; // 抢占优先级
    NVIC_InitSturcture.NVIC_IRQChannelSubPriority = 1;        // 优先级
    NVIC_Init(&NVIC_InitSturcture);

    TIM_Cmd(TIM2, ENABLE);
}

uint16_t Timer_GetCounter()
{
    return TIM_GetCounter(TIM2);
}
// void TIM2_IRQHandler()
// {
// 	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
// 	{
// 		Num++;
// 		TIM_ClearITPendingBit(TIM2, TIM_IT_Update); // 清楚中断标志位  可以理解为推出中断
// 	}
// }
