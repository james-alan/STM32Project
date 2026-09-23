#include "stm32f10x.h"
#include "Delay.h"
#include "Key.h"

/*================================================================
 *  按键驱动
 *
 *  Key_init()：PB1 / PB11 上拉输入，按下为低
 *
 *  为什么要有非阻塞版：
 *  原来的 Key_GetNum() 里是 while (读引脚==0); —— 按住不放就把整个
 *  主循环卡死（屏幕不刷新、历史不记录、呼吸灯以外的逻辑全停）。
 *  非阻塞版用状态机：每 10ms 调一次 Key_Tick()，只在事件发生的
 *  那一刻置一个标志，主循环随时查询，永远不会被按键卡住。
 *================================================================*/

#define KEY_LONG_MS     2000    /* 长按判定：2 秒 */
#define KEY_TICK_MS     10      /* Key_Tick() 的调用周期 */
#define KEY_DEBOUNCE_MS 20      /* 消抖时间 */

static uint16_t keyHoldMs[3];      /* 已按住时长 */
static uint8_t  keyPressed[3];     /* 上一次扫描是否按下 */
static uint8_t  keyShortEvt[3];    /* 短按事件标志 */
static uint8_t  keyLongEvt[3];     /* 长按事件标志 */
static uint8_t  keyLongFired[3];   /* 长按是否已经触发过 */

static uint16_t Key_PinOf(uint8_t n)
{
    return (n == 1) ? GPIO_Pin_1 : GPIO_Pin_11;
}

void Key_init(void)
{
    uint8_t i;

    /* 显式清零状态机变量：ARMCC 的 --bss_threshold 会把这些小数组
     * 放进 RW 段，初值存在 Flash 加载区，某些烧录方式写不到那里 */
    for (i = 0; i < 3; i++)
    {
        keyHoldMs[i]    = 0;
        keyPressed[i]   = 0;
        keyShortEvt[i]  = 0;
        keyLongEvt[i]   = 0;
        keyLongFired[i] = 0;
    }

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;      /* 上拉输入 */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/* 每 ~10ms 调一次 */
void Key_Tick(void)
{
    uint8_t n;

    for (n = 1; n <= 2; n++)
    {
        uint8_t down = (GPIO_ReadInputDataBit(GPIOB, Key_PinOf(n)) == 0) ? 1 : 0;

        if (down)
        {
            if (keyHoldMs[n] < 0xFFFF) keyHoldMs[n] += KEY_TICK_MS;

            /* 按满 2 秒 -> 长按事件（按住期间就触发一次） */
            if (!keyLongFired[n] && keyHoldMs[n] >= KEY_LONG_MS)
            {
                keyLongEvt[n]   = 1;
                keyLongFired[n] = 1;
            }
        }
        else
        {
            /* 松手时判断短按：按过、够消抖、且没触发过长按 */
            if (keyPressed[n] && !keyLongFired[n] &&
                keyHoldMs[n] >= KEY_DEBOUNCE_MS)
            {
                keyShortEvt[n] = 1;
            }
            keyHoldMs[n]    = 0;
            keyLongFired[n] = 0;
        }

        keyPressed[n] = down;
    }
}

uint8_t Key_GetShortPress(uint8_t KeyNum)
{
    if (KeyNum < 1 || KeyNum > 2) return 0;
    if (keyShortEvt[KeyNum]) { keyShortEvt[KeyNum] = 0; return 1; }
    return 0;
}

uint8_t Key_GetLongPress(uint8_t KeyNum)
{
    if (KeyNum < 1 || KeyNum > 2) return 0;
    if (keyLongEvt[KeyNum]) { keyLongEvt[KeyNum] = 0; return 1; }
    return 0;
}

/*================================================================
 *  老接口：阻塞式（保留仅供参考，主循环不要再用）
 *================================================================*/
uint8_t Key_GetNum(void)
{
    uint8_t KeyNum = 0;

    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
    {
        Delay_ms(20);
        while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0);
        Delay_ms(20);
        KeyNum = 1;
    }
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 0)
    {
        Delay_ms(20);
        while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 0);
        Delay_ms(20);
        KeyNum = 2;
    }
    return KeyNum;
}