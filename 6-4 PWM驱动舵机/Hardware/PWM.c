#include "stm32f10x.h"

// pwm输出，内部时钟驱动 通过控制ARR、CNT、PSC、CRR来控制pwm输出波形
void PWM_Init()
{
    // 开始时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    // 配置gpio
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //复用
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; //
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置时钟源
    TIM_InternalClockConfig(TIM2);

    // 配置时基单元
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;   
    TIM_TimeBaseInitStructure.TIM_Period = 20000 - 1;                 // 周期 2ms 
    TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;              // 预分频 stm32晶振72Mhz 
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     // 滤波器时钟
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            // 重复计数器
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; // 计数模式

    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

    // 输出比较初始化
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCStructInit(&TIM_OCInitStructure); // 在未完全配置结构体的情况下，

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;  //输出比较模式
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //极性 
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;  //输出使能
    TIM_OCInitStructure.TIM_Pulse = 0;  //
    TIM_OC1Init(TIM2, &TIM_OCInitStructure);

    TIM_Cmd(TIM2, ENABLE);
}



//pwm设置ccr ccr和arr共同决定占空比 占空比Dety=CCR/（ARR+1）
void PWM_SetCompare2(uint16_t Compare)
{
    TIM_SetCompare2(TIM2, Compare);
}


