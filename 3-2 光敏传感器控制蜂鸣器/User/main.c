#include "stm32f10x.h"
#include "Delay.h" // Device header
#include "Buzzer.h"
#include "LightSenser.h"

// 光敏传感器控制蜂鸣器
int main(void)
{
	LightSenser_init();
	Buzzer_init();

	while (1)
	{
		if (LightSenser_Get() == 1)
		{
			Buzzer_ON();
		}
		else
		{
			Buzzer_OFF();
		}
	}
}
