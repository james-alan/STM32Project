#include "stm32f10x.h"
#include "Delay.h" // Device header
#include "OLED.h"
#include "Key.h"

uint8_t KeyNum;

uint16_t ArrayWrite[]={0x1234,0x5678}; //发 
uint16_t ArrayRead[2]; //收



// 光敏传感器控制蜂鸣器
int main(void)
{
OLED_Init();
Key_init();

OLED_ShowString(1,1,"W:");
OLED_ShowString(2,1,"R:");

RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR,ENABLE);
RCC_APB1PeriphClockCmd(RCC_APB1ENR_BKPEN,ENABLE); //防止误开启 所以开启RTC的时候需要先开PWR 再开BKP


PWR_BackupAccessCmd(ENABLE);

	while (1)
	{

		KeyNum= Key_GetNum();

		if(KeyNum==1){
			ArrayWrite[0]++;
			ArrayWrite[1]++;

			BKP_WriteBackupRegister(BKP_DR1,ArrayWrite[0]);
			BKP_WriteBackupRegister(BKP_DR1,ArrayWrite[1]);

			OLED_ShowHexNum(1,3,ArrayWrite[0],4);
			OLED_ShowHexNum(2,3,ArrayWrite[1],4);


		}

		
	}
}
