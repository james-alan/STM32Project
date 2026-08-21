#include <stdio.h>
#include <time.h>          // 引入 time.h 头文件，时间戳相关函数都在里面

int main(void)
{
    time_t timeStamp;      // 秒计数器类型变量，用于存储时间戳秒数

    // time() 的两种用法：
    // 用法一：通过返回值获取当前秒数
    timeStamp = time(NULL);            // 参数不需要时传 NULL（空指针）即可
    printf("%lld\n", timeStamp);       // 打印当前时间戳秒数（64 位 MinGW 下 time_t 是 long long，用 %lld）

    // 用法二：通过输出参数获取当前秒数（效果一样，额外演示）
    time(&timeStamp);                  // 参数传入 timeStamp 的地址
    printf("%lld\n", timeStamp);       // 再次打印，两次应完全一致

    return 0;
}
