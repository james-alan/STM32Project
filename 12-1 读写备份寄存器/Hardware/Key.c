#include "stm32f10x.h"
#include "Delay.h"


/**
  * 函    数：按键初始化
  * 参    数：无
  * 返 回 值：无
  */

//按键使用
void Key_init(){
    //先开时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    
    //创建结构体
    GPIO_InitTypeDef GPIO_InitStructure; 
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU; //上拉输入
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_1 | GPIO_Pin_11; 
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    //gpio初始化
    GPIO_Init(GPIOB,&GPIO_InitStructure);

}

/**
  * 函    数：按键获取键码
  * 参    数：无
  * 返 回 值：按下按键的键码值，范围：0~2，返回0代表没有按键按下
  * 注意事项：此函数是阻塞式操作，当按键按住不放时，函数会卡住，直到按键松手
  */

uint8_t Key_GetNum(){
     uint8_t KeyNum = 0; //0为按下了 1代表未按下
    //上拉输入，按键按下为1 不按为0
    //按键1
    if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_1)==0){
        //消除抖动
        Delay_ms(20);
        while (GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_1)==0); // while()中为真1 进入循环
        Delay_ms(20);
        KeyNum = 1;

    }
    //按键2
        if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11)==0){
        //消除抖动
        Delay_ms(20);
        while (GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11)==0); // while()中为真1 进入循环
        Delay_ms(20);
        KeyNum = 2;

    }
    return KeyNum;
}