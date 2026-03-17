#include "stm32f10x.h"
#include "Delay.h" // Device header
#include "OLED.h"
#include "PWM.h"
/*
现象：在oled显示器上显示计数器，显示CNT数字
*/
// 定时器定时中断

// uint16_t Num;
// uint16_t CNT;
uint8_t i;
int main(void)
{
	OLED_Init();
	PWM_Init();
	// Timer_Init();

	OLED_ShowString(1, 1, "Num:");
	// OLED_ShowString(2, 1, "CNT:");

	while (1)
	{
		// OLED_ShowNum(1, 5, Num, 5);
		// OLED_ShowNum(2,5,Timer_GetCounter(),5);
		// PWM_SetCompare1(100);
		for (i = 0; i <= 100; i++)
		{
			PWM_SetCompare1(i);
			Delay_ms(10);
		}
		for (i = 0; i <= 100; i++)
		{
			PWM_SetCompare1(100 - i);
			Delay_ms(10);
		}
	}
}

// void TIM2_IRQHandler()
// {
// 	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
// 	{
// 		Num++;
// 		TIM_ClearITPendingBit(TIM2, TIM_IT_Update); // 清楚中断标志位  可以理解为推出中断
// 	}
// }
