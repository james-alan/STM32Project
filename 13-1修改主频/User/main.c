#include "stm32f10x.h"
#include "Delay.h" // Device header
#include "OLED.h"

// 光敏传感器控制蜂鸣器
int main(void)
{
	OLED_Init();
	OLED_ShowString(1, 1, "SYSCLK:"); // 显示静态字符串
	OLED_ShowNum(1, 8, SystemCoreClock, 8);

	while (1)
	{

		OLED_ShowString(2, 1, "Running");
		Delay_ms(50);
		OLED_ShowString(2, 1, "       ");
		Delay_ms(500);
	}
}
