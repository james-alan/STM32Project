#include <stdio.h>
#include <time.h>

int main(void)
{
    time_t timeStamp = 1672588795;    // 手动指定秒数
    struct tm timeStruct;

    timeStruct = *localtime(&timeStamp);   // 先转成当地时间

    // ctime()：秒数 → 字符串（默认格式）
    char *str;
    str = ctime(&timeStamp);               // 参数是 time_t 指针
    printf("%s", str);

    // asctime()：日期时间 → 字符串（默认格式）
    str = asctime(&timeStruct);            // 参数是 struct tm 指针
    printf("%s", str);

    return 0;
}
