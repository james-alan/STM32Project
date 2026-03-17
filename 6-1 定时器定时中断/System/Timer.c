#include "stm32f10x.h"
// 定时器初始化

// 练习定时器中断 rcc内部时钟
void Timer_Init()
{
    // 初始化流程 开启rtc->配置时钟源->时基单元初始化->中断设置->中断输出控制->nvic优先级设置
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    // 选择使用哪个定时器 本设备有高级、通用、基本定时器
    TIM_InternalClockConfig(TIM2);

    // 时基单元初始化
    TIM_TimeBaseInitTypeDef TIM_TimBaseInitStructure;
    TIM_TimBaseInitStructure.TIM_Prescaler = 7200 - 1;             // 预分频器 即PSC的值 将时钟频率分频  STM32默认内部时钟72MHz
    TIM_TimBaseInitStructure.TIM_Period = 10000 - 1;               // 计数周期 即ARR的值 自动重装器
    TIM_TimBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     // 时钟分频 用于配置滤波器参数 选择1分频
    TIM_TimBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; // 分频模式 选择向上计数
    TIM_TimBaseInitStructure.TIM_RepetitionCounter = 0;            // 重复计数器 高级定时器才会用到
    TIM_TimeBaseInit(TIM2, &TIM_TimBaseInitStructure);

    // 中断设置
    TIM_ClearFlag(TIM2, TIM_FLAG_Update); // 在TIM_TimeBaseInit函数的末尾产生了一个更新标志位，若不清楚，会默认进入一次中断

    // 开始配置中断
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    // nvic中断配置
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    // nvic配置
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

    // 使能tim
    TIM_Cmd(TIM2, ENABLE);
}
