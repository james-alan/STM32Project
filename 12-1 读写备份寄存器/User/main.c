#include "stm32f10x.h"
#include "Delay.h" // Device header
#include "OLED.h"


// 光敏传感器控制蜂鸣器
int main(void)
{
OLED_Init();
OLED_ShowChar(1,1,'A');
OLED_ShowString(2,1,"Hello world");
OLED_ShowNum(3,1,12345,5);


	while (1)
	{

		
	}
}
