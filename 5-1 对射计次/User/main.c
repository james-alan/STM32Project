#include "stm32f10x.h"
#include "Delay.h" // Device header
#include "OLED.h"
#include "CountSensor.h"


// 光敏传感器控制蜂鸣器
int main(void)
{
	//初始化	
	OLED_Init();
	CountSensor_Init();

	OLED_ShowString(1, 1, "Count:");
	// uint16_t count=CountSensor_Get();

	while (1)
	{	
		OLED_ShowNum(1,7,CountSensor_Get(),5);

	}
}
