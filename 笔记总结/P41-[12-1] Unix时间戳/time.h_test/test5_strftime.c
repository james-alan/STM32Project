#include <stdio.h>
#include <time.h>

int main(void)
{
    time_t timeStamp = 1672588795;    // 手动指定秒数
    struct tm timeStruct;

    timeStruct = *localtime(&timeStamp);   // 先转成当地时间

    // strftime()：按自定义格式把日期时间转换为字符串
    char str[50];                          // 定义字符数组接收结果，长度给 50
    strftime(str, 50, "%H:%M:%S", &timeStruct);
    //           ↑    ↑      ↑        ↑
    //       字符数组 数组长度  格式串  tm 地址

    printf("%s\n", str);                   // 打印字符串

    return 0;
}
