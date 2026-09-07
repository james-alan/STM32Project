#include "stm32f10x.h"


//蜂鸣器低电平启动 高电平停止工作
void Buzzer_init(){
    //通过GPIO输出
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP; //推挽
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(GPIOB,&GPIO_InitStructure);

    //默认低电平
    GPIO_SetBits(GPIOB,GPIO_Pin_12); //默认设置高电平 
}

void Buzzer_ON(){
    GPIO_SetBits(GPIOB,GPIO_Pin_12);
}

void Buzzer_OFF(){
    GPIO_ResetBits(GPIOB,GPIO_Pin_12); 
}

void Buzzer_turn(){
    if(GPIO_ReadOutputDataBit(GPIOB,GPIO_Pin_12)==0){
            GPIO_SetBits(GPIOB,GPIO_Pin_12);
    }
    else
    {
        GPIO_ResetBits(GPIOB,GPIO_Pin_12); 

    }

}


