#include "stm32f10x.h"
#include "Delay.h" // Device header
#include "OLED.h"
#include "MyRTC.h"

// 光敏传感器控制蜂鸣器
int main(void)
{
	OLED_Init();
	MyRTC_Init();

	OLED_ShowString(1, 1, "Date:XXXX-XX-XX");
	OLED_ShowString(2, 1, "Time:XX:XX:XX");
	OLED_ShowString(3, 1, "CNT :");
	OLED_ShowString(4, 1, "DIV :");

	while (1)
	{

		MyRTC_ReadTime();
		
		OLED_ShowNum(1, 6, MyRTC_Time[0], 4);		//显示MyRTC_Time数组中的时间值，年
		OLED_ShowNum(1, 11, MyRTC_Time[1], 2);		//月
		OLED_ShowNum(1, 14, MyRTC_Time[2], 2);		//日
		OLED_ShowNum(2, 6, MyRTC_Time[3], 2);		//时
		OLED_ShowNum(2, 9, MyRTC_Time[4], 2);		//分
		OLED_ShowNum(2, 12, MyRTC_Time[5], 2);		//秒

		OLED_ShowNum(3,6,RTC_GetCounter(),10);	 //显示32位秒寄存器
		OLED_ShowNum(4,6,RTC_GetDivider(),10);   //显示余数寄存器 




		
	}
}
