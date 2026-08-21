#include <stdio.h>
#include <time.h>

int main(void)
{
    time_t timeStamp = 1672588795;    // 原始秒数：伦敦 2023-01-01 15:59:55
    struct tm timeStruct;             // 日期时间结构体变量

    // 秒数 → 当地时间（东八区 +8 小时，得到北京 2023-01-01 23:59:55）
    timeStruct = *localtime(&timeStamp);
    printf("%d年%d月%d日 %d时%d分%d秒 星期%d\n",
           timeStruct.tm_year + 1900, timeStruct.tm_mon + 1, timeStruct.tm_mday,
           timeStruct.tm_hour, timeStruct.tm_min, timeStruct.tm_sec,
           timeStruct.tm_wday);

    // 日期时间（当地时间） → 秒数
    timeStamp = mktime(&timeStruct);  // 传入日期时间结构体地址
    printf("%lld\n", timeStamp);      // 打印转换回来的秒数（64 位 MinGW 下 time_t 是 long long，用 %lld）

    return 0;
}
