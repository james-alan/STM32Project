#include "stm32f10x.h"
#include <time.h>

uint16_t MyRTC_Time[] = {2023, 1, 1, 23, 59, 55};

void MyRTC_SetTime(void);

void MyRTC_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE); // 使能rtc 先pwr 再bkp
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);

    PWR_BackupAccessCmd(ENABLE);

    if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
    { // 等待bkp数据寄存器完成
        RCC_LSEConfig(RCC_LSE_ON);
        while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) != SET)
            ;

        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
        RCC_RTCCLKCmd(ENABLE);

        RTC_WaitForSynchro();
        RTC_WaitForLastTask(); // 每完成一次 都要等待上一个任务完成

        RTC_SetPrescaler(32768 - 1); // 设置预分频起
        RTC_WaitForLastTask();

        MyRTC_SetTime();

        BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
    }
    else
    {
        RTC_WaitForSynchro();
        RTC_WaitForLastTask();
    }
}

// 设置时间 把数组的数据变成秒
void MyRTC_SetTime(void)
{
    time_t time_cnt;
    struct tm time_date;

    time_date.tm_year = MyRTC_Time[0] - 1900;
    time_date.tm_mon = MyRTC_Time[1] - 1;
    time_date.tm_mday = MyRTC_Time[2];
    time_date.tm_hour = MyRTC_Time[3];
    time_date.tm_min = MyRTC_Time[4];
    time_date.tm_sec = MyRTC_Time[5];

    time_cnt = mktime(&time_date) - 8 * 60 * 60; // 因为是东8区 所以要减8小时

    RTC_SetCounter(time_cnt);
    RTC_WaitForLastTask();
}

// 读取时间 把秒数据变成数组数据
void MyRTC_ReadTime()
{
    time_t time_cnt;
    struct tm time_date;

    time_cnt = RTC_GetCounter() + 8 * 60 * 60;

    time_date = *localtime(&time_cnt); // 把cnt的时间转成localtime

    MyRTC_Time[0] = time_date.tm_year + 1900;
    MyRTC_Time[1] = time_date.tm_mon + 1;
    MyRTC_Time[2] = time_date.tm_mday;
    MyRTC_Time[3] = time_date.tm_hour;
    MyRTC_Time[4] = time_date.tm_min;
    MyRTC_Time[5] = time_date.tm_sec;
}