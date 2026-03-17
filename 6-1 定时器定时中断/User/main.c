#include "stm32f10x.h"
#include "Delay.h" // Device header
#include "OLED.h"
#include "Timer.h"
/*
现象：在oled显示器上显示计数器，显示CNT数字
*/
// 定时器定时中断

uint16_t Num;

int main(void)
{
	OLED_Init();
	Timer_Init();

	OLED_ShowString(1, 1, "Num:");

	while (1)
	{
		OLED_ShowNum(1, 5, Num, 4);
	}
}

void TIM2_IRQHandler()
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		Num++;
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update); // 清楚中断标志位  可以理解为推出中断
	}
}
