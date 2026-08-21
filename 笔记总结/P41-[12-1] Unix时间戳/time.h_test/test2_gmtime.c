#include <stdio.h>
#include <time.h>

int main(void)
{
    time_t timeStamp = 1672588795;    // 手动指定秒数：伦敦 2023-01-01 15:59:55
    struct tm timeStruct;             // 日期时间结构体变量

    // 把秒数转换为日期时间（返回的是结构体指针）
    // 方法一：右边返回的指针加 * 解引用，等号左右就都是结构体变量
    timeStruct = *gmtime(&timeStamp);       // 伦敦（UTC/GMT）时间

    // 打印日期时间：注意 tm_year 要 +1900、tm_mon 要 +1 才是真实年份月份
    printf("gmtime   : %d年%d月%d日 %d时%d分%d秒 星期%d\n",
           timeStruct.tm_year + 1900, timeStruct.tm_mon + 1, timeStruct.tm_mday,
           timeStruct.tm_hour, timeStruct.tm_min, timeStruct.tm_sec,
           timeStruct.tm_wday);

    // localtime()：根据时区自动 +8 小时，得到东八区（北京）时间
    timeStruct = *localtime(&timeStamp);
    printf("localtime: %d年%d月%d日 %d时%d分%d秒 星期%d\n",
           timeStruct.tm_year + 1900, timeStruct.tm_mon + 1, timeStruct.tm_mday,
           timeStruct.tm_hour, timeStruct.tm_min, timeStruct.tm_sec,
           timeStruct.tm_wday);

    return 0;
}
