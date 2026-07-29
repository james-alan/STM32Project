#include "stm32f10x.h"
#include "Delay.h" // Device header
#include "OLED.h"
#include "Serial.h"
#include "Key.h"

uint8_t KeyNum;

int main(void)
{

	OLED_Init();
	Serial_Init();
	Key_init();

	OLED_ShowString(1, 1, "TxData");
	OLED_ShowString(3, 1, "RxData");

	// 本程序主要内容是 自己给自己发数据包测试
	Serial_TxPacket[0] = 0x01;
	Serial_TxPacket[1] = 0x02;
	Serial_TxPacket[2] = 0x03;
	Serial_TxPacket[3] = 0x04;

	while (1)
	{
		// 获取按键码
		KeyNum = Key_GetNum();
		// 按键为1的时候是按下
		if (KeyNum == 1)
		{
			// 按键数据自增
			Serial_TxPacket[0]++;
			Serial_TxPacket[1]++;
			Serial_TxPacket[2]++;
			Serial_TxPacket[3]++;

			Serial_SendPacket();

			// 显示发送的数据包
			OLED_ShowNum(2, 1, Serial_TxPacket[0], 2);
			OLED_ShowNum(2, 4, Serial_TxPacket[1], 2);
			OLED_ShowNum(2, 7, Serial_TxPacket[2], 2);
			OLED_ShowNum(2, 10, Serial_TxPacket[3], 2);
		}

		// 如果接收到数据包
		if (Serial_GetRxFlag() == 1)
		{
			// 显示接收的数据包
			OLED_ShowNum(4, 1, Serial_RxPacket[0], 2);
			OLED_ShowNum(4, 4, Serial_RxPacket[1], 2);
			OLED_ShowNum(4, 7, Serial_RxPacket[2], 2);
			OLED_ShowNum(4, 10, Serial_RxPacket[3], 2);
		}
	}
}
