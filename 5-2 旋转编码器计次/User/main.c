#include "stm32f10x.h"
#include "OLED.h"
#include "Encode.h"

// 光敏传感器控制蜂鸣器
int main(void)
{
	// 初始化
	OLED_Init();
	Encoder_init();

	OLED_ShowString(1, 1, "Count:");
	int16_t count;

	while (1)
	{
		count += Encoder_Get();
		OLED_ShowNum(1, 7, count, 5);
	}
}
