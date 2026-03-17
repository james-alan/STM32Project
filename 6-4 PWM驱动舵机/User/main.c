#include "stm32f10x.h"
#include "Delay.h" // Device header
#include "OLED.h"
#include "Servo.h"
#include "Key.h"

/*
现象：控制舵机角度
*/
// 定时器定时中断
uint8_t KeyNum;
float Angle;

uint8_t i;
int main(void)
{
	OLED_Init();
	Servo_Init();
	Key_init();

	OLED_ShowString(1, 1, "Angle:");

	while (1)
	{

		KeyNum = Key_GetNum();
		if (KeyNum == 1)
		{
			Angle += 30;
			if (Angle > 180)
			{
				Angle = 0;
			}
		}
		Servo_SetAngle(Angle);
		OLED_ShowNum(1, 7, Angle, 3);
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
