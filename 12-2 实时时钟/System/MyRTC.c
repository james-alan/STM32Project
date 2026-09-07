#include "stm32f10x.h"
#include <time.h>

uint16_t MyRTC_Time[]={2023,1,1,23,59,55};


void MtRTC_SetTime(void);

void MyRTC_Init(void){
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR,ENABLE); //使能rtc 先pwr 再bkp
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP,ENABLE); 
    
    PWR_BackupAccessCmd(ENABLE);



}

void MtRTC_SetTime(void){
    time_t time_cnt;
    


}