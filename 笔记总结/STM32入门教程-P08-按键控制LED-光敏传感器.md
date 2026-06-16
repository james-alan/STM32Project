# STM32入门教程 P08：[3-4] 按键控制LED & 光敏传感器控制蜂鸣器

> **视频来源**：B站 BV1th411z7sn | 江协科技 | STM32入门教程-2023版
> **时长**：33:05 | **芯片**：STM32F103C8T6

---

## 一、模块化编程

### 1.1 工程结构

```
工程文件夹
├── Hardware/          ← 硬件驱动模块（本节课新建）
│   ├── LED.c / LED.h           ← LED驱动
│   ├── Key.c / Key.h           ← 按键驱动
│   ├── Buzzer.c / Buzzer.h     ← 蜂鸣器驱动
│   └── LightSensor.c / .h      ← 光敏传感器驱动
├── System/            ← 系统资源（Delay）
├── Library/           ← 库函数
├── Start/             ← 启动文件
└── User/              ← 主函数
```

### 1.2 头文件固定模板

```c
// LED.h
#ifndef __LED_H
#define __LED_H

// 函数声明放这里
void LED_Init(void);
void LED1_ON(void);
void LED1_OFF(void);
void LED1_Turn(void);   // 翻转

#endif
```

> `.c` 第一行：`#include "stm32f10x.h"`
> 文件末尾必须留空行，否则Keil报警告

---

## 二、LED驱动模块

### 2.1 电路

| LED | 端口 | 驱动方式 |
|-----|------|---------|
| LED1 | PA1 | 低电平点亮 |
| LED2 | PA2 | 低电平点亮 |

### 2.2 LED.c 完整代码

```c
#include "stm32f10x.h"

void LED_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 初始化后默认关闭LED（高电平）
    GPIO_SetBits(GPIOA, GPIO_Pin_1 | GPIO_Pin_2);
}

void LED1_ON(void)  { GPIO_ResetBits(GPIOA, GPIO_Pin_1); }
void LED1_OFF(void) { GPIO_SetBits(GPIOA, GPIO_Pin_1); }
void LED2_ON(void)  { GPIO_ResetBits(GPIOA, GPIO_Pin_2); }
void LED2_OFF(void) { GPIO_SetBits(GPIOA, GPIO_Pin_2); }

// LED状态翻转（用到GPIO_ReadOutputDataBit读取当前输出）
void LED1_Turn(void)
{
    if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_1) == 0)
        GPIO_SetBits(GPIOA, GPIO_Pin_1);
    else
        GPIO_ResetBits(GPIOA, GPIO_Pin_1);
}
void LED2_Turn(void)
{
    if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_2) == 0)
        GPIO_SetBits(GPIOA, GPIO_Pin_2);
    else
        GPIO_ResetBits(GPIOA, GPIO_Pin_2);
}
```

---

## 三、按键驱动模块

### 3.1 电路

| 按键 | 端口 | 接法 |
|------|------|------|
| KEY1 | PB1 | 一端接IO，一端接GND（需要上拉输入） |
| KEY2 | PB11 | 同上 |

### 3.2 Key.c 完整代码

```c
#include "stm32f10x.h"
#include "Delay.h"

void Key_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;  // 上拉输入
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

uint8_t Key_GetNum(void)
{
    uint8_t KeyNum = 0;
    
    // 检测KEY1 (PB1)
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
    {
        Delay_ms(20);                                    // 按下消抖
        while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0); // 等待松开
        Delay_ms(20);                                    // 松开消抖
        KeyNum = 1;
    }
    
    // 检测KEY2 (PB11)
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 0)
    {
        Delay_ms(20);
        while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 0);
        Delay_ms(20);
        KeyNum = 2;
    }
    
    return KeyNum;
}
```

### 3.3 main.c 调用

```c
uint8_t KeyNum;  // 全局变量

int main(void)
{
    LED_Init();
    Key_Init();
    
    while (1)
    {
        KeyNum = Key_GetNum();
        
        if (KeyNum == 1) LED1_Turn();  // 按键1翻转LED1
        if (KeyNum == 2) LED2_Turn();  // 按键2翻转LED2
    }
}
```

---

## 四、GPIO读取函数辨析

| 函数 | 读取对象 | 用途 |
|------|---------|------|
| `GPIO_ReadInputDataBit(GPIOx, Pin)` | **输入数据寄存器**某一位 | 读外部引脚电平（按键/传感器） |
| `GPIO_ReadInputData(GPIOx)` | 整个输入数据寄存器 | 读全部引脚 |
| `GPIO_ReadOutputDataBit(GPIOx, Pin)` | **输出数据寄存器**某一位 | 看当前输出是什么（用于翻转等） |
| `GPIO_ReadOutputData(GPIOx)` | 整个输出数据寄存器 | 读全部输出状态 |

> 读外部输入用 InputData，读自己输出状态用 OutputData！

---

## 五、蜂鸣器驱动模块

```c
// Buzzer.c - 和LED驱动几乎一样（PB12，低电平有效）
void Buzzer_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB, GPIO_Pin_12);  // 默认关闭
}

void Buzzer_ON(void)  { GPIO_ResetBits(GPIOB, GPIO_Pin_12); }
void Buzzer_OFF(void) { GPIO_SetBits(GPIOB, GPIO_Pin_12); }
void Buzzer_Turn(void)
{
    if (GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_12) == 0)
        GPIO_SetBits(GPIOB, GPIO_Pin_12);
    else
        GPIO_ResetBits(GPIOB, GPIO_Pin_12);
}
```

---

## 六、光敏传感器模块

```c
// LightSensor.c (PB13，数字输出)
void LightSensor_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;  // 上拉输入
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

uint8_t LightSensor_Get(void)
{
    return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13);
}
```

### 主函数集成

```c
LightSensor_Init();
Buzzer_Init();

while (1)
{
    if (LightSensor_Get() == 1)  // 光线暗 → 高电平
        Buzzer_ON();
    else
        Buzzer_OFF();
}
```

> 光敏传感器模块：光强↑→阻值↓→AO电压↓；遮光时→DO输出高电平。电位器可调阈值。

---

## 七、GPIO操作总结

```
初始化三步走:
  1. RCC_APB2PeriphClockCmd(...)      开时钟
  2. GPIO_InitTypeDef + 赋值结构体     定参数
  3. GPIO_Init(...)                   写入寄存器

输出四个函数:
  GPIO_SetBits      → 置高电平（支持按位或多引脚）
  GPIO_ResetBits    → 置低电平
  GPIO_WriteBit     → 写单个引脚（Bit_SET/Bit_RESET）
  GPIO_Write        → 写全部16引脚（直接写ODR）

输入两个函数:
  GPIO_ReadInputDataBit   → 读单个引脚输入
  GPIO_ReadInputData      → 读全部16引脚输入

输出状态下读自己:
  GPIO_ReadOutputDataBit  → 读单个引脚当前输出（翻转用）
  GPIO_ReadOutputData     → 读全部输出
```

---

## 八、模块化编程要点

1. **每个硬件一个模块**（.c + .h），主函数只调用高层接口
2. **头文件固定模板**：`#ifndef __XXX_H` / `#define __XXX_H` / `#endif`
3. **写清楚注释**：函数用途、参数说明、返回值
4. **新建文件夹后别忘加头文件路径**（魔术棒→C/C++→Include Paths）
5. **局部变量 vs 全局变量**：同名变量在不同作用域互不影响
6. 模块化后主函数逻辑清晰：`Init()` → `while(1){ 逻辑 }`import
