#include "stm32f10x.h"
#include "Delay.h" // Device header
#include "Key.h"
#include "LED.h"

uint8_t KeyNum;
// 按键控制小灯
int main(void)
{
	LED_init();
	Key_init();

	while (1)
	{
		KeyNum = Key_GetNum();
		if (KeyNum == 1)
		{
			LED1_turn();
		}
		if (KeyNum == 2)
		{
			LED2_turn();
		}
	}
}
