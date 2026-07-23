#include "stm32f10x.h"
#include "Delay.h" // Device header
#include "OLED.h"
#include "Serial.h"


int main(void)
{
	
	OLED_Init();

	Serial_Init();
	//串口发送一个字符
	Serial_SendByte(0x41);

	//发送数组
	uint8_t MyArray[]={0x42,0x43,0x44,0x45};
	Serial_SendArray(MyArray,4);

	//发送字符串
	Serial_SendString("\r\nNum1=");

	Serial_SendNumber(111,3);


	//以下方法实现printf

	//方法1 
	print("/r/nNum2=%d",222)；

	//方法2
	char String[100];
	sprintf(String,"\r\nNum3=%d",333);
	Serial_SendString(String);

	//方法3
	Serial_printf("\r\nNum4=%d",444);
	Serial_printf("\r\n");





	while (1)
	{

		
	}
}
