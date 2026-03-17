#include "stm32f10x.h"

//全局变量 用于计数
uint16_t CountSensor_Count;

void CountSensor_Init(){

    //主要配置AFIO-EXIT-NVIC
    //配置rcc  
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);


    //开启AFIO的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(GPIOB,&GPIO_InitStructure);


    //afio引脚选择
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource14);

    //exti初始化
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Mode=EXTI_Mode_Interrupt;
    // EXTI_InitStructure.EXTI_Trigger=EXTI_Trigger_Falling; //下降沿检测
    EXTI_InitStructure.EXTI_Trigger=EXTI_Trigger_Rising_Falling; //上升下降沿检测


    EXTI_InitStructure.EXTI_Line=EXTI_Line14;
    EXTI_InitStructure.EXTI_LineCmd=ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    //nvic中断分组
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel=EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=1;

    NVIC_Init(&NVIC_InitStructure);



}


//获取传感器的值
uint16_t CountSensor_Get(){
    return CountSensor_Count;
    
}

//外部中断函数
void EXTI15_10_IRQHandler(){
    //因为是15-10 所以要判断一下，是是否是pin14中断被触发
    if(EXTI_GetITStatus(EXTI_Line14)==SET){
        CountSensor_Count++;
       
    }
     //最后要清楚中断标志位
        EXTI_ClearITPendingBit(EXTI_Line14);
}
