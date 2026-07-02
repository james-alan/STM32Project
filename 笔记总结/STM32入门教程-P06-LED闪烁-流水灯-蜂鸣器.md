# STM32入门教程 P06：[3-2] LED闪烁 & LED流水灯 & 蜂鸣器

> **视频来源**：B站 BV1th411z7sn | 江协科技 | STM32入门教程-2023版
> **时长**：39:10 | **芯片**：STM32F103C8T6

---

## 一、程序总览

| 程序 | 功能 | IO口 | 关键知识点 |
|------|------|------|-----------|
| LED闪烁 | 单个LED交替亮灭 | PA0 | GPIO三步走、延时函数 |
| LED流水灯 | 8个LED依次点亮 | PA0~PA7 | 按位或选多引脚、GPIO_Write |
| 蜂鸣器 | 有源蜂鸣器滴滴响 | PB12 | 换IO口配置、推挽/开漏对比 |

---

## 二、GPIO操作三步走

```
Step 1: RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOx, ENABLE);  // 使能时钟
Step 2: GPIO_Init(GPIOx, &GPIO_InitStructure);                  // 初始化
Step 3: GPIO_SetBits/ResetBits/WriteBit/Write(...);            // 输出电平
```

### 2.1 RCC时钟函数（只需3个）

| 函数 | 控制的总线 |
|------|-----------|
| `RCC_AHBPeriphClockCmd()` | AHB（DMA等） |
| `RCC_APB2PeriphClockCmd()` | APB2（GPIO、TIM1、ADC、USART1等） |
| `RCC_APB1PeriphClockCmd()` | APB1（TIM2~4、USART2~3、I2C、SPI2等） |

> GPIO全部在APB2上 → 统一用 `RCC_APB2PeriphClockCmd`

### 2.2 GPIO常用函数

| 函数 | 功能 | 参数特点 |
|------|------|---------|
| `GPIO_Init(GPIOx, &结构体)` | **初始化**端口模式 | 结构体传地址 |
| `GPIO_SetBits(GPIOx, Pin)` | 端口置**高电平** | Pin支持按位或多选 |
| `GPIO_ResetBits(GPIOx, Pin)` | 端口置**低电平** | Pin支持按位或多选 |
| `GPIO_WriteBit(GPIOx, Pin, BitVal)` | 写单个端口 | BitVal: Bit_SET/Bit_RESET |
| `GPIO_Write(GPIOx, PortVal)` | 写**全部16端口** | 直接写ODR寄存器 |

### 2.3 GPIO_Init结构体

```c
GPIO_InitTypeDef GPIO_InitStructure;
GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;   // 推挽输出
GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0;         // 引脚
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   // 速度
GPIO_Init(GPIOA, &GPIO_InitStructure);
```

---

## 三、程序1：LED闪烁

### 3.1 电路连接

```
PA0 ← LED负极（短脚）
3.3V ← LED正极（长脚）
方式：低电平驱动
```

### 3.2 代码实现

```c
#include "stm32f10x.h"
#include "Delay.h"

int main(void)
{
    // Step 1: 使能GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    // Step 2: 初始化PA0为推挽输出
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // Step 3: 主循环闪烁
    while (1)
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_0);  // 低电平→亮
        Delay_ms(500);
        GPIO_SetBits(GPIOA, GPIO_Pin_0);    // 高电平→灭
        Delay_ms(500);
    }
}
```

### 3.3 延时函数模块（Delay）

```c
// delay.h 提供三个延时函数
void Delay_us(uint32_t us);   // 微秒延时
void Delay_ms(uint32_t ms);   // 毫秒延时
void Delay_s(uint32_t s);     // 秒延时

// 底层用SysTick定时器实现，直接拿来用即可
```

> 添加方法：复制delay.c/h到工程 → 建system组 → 添加文件 → 声明头文件路径 → `#include "Delay.h"`

---

## 四、程序2：LED流水灯

### 4.1 电路

```
PA0~PA7 ← 8个LED负极
3.3V ← 8个LED正极（共阳接法，低电平驱动）
```

### 4.2 多引脚配置（按位或）

```c
// 一次性配置8个引脚
GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3
                            | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
// 或者直接用 GPIO_Pin_All（16个全配）
```

**原理**：
```
GPIO_Pin_0 = 0x0001 = 0000 0000 0000 0001
GPIO_Pin_1 = 0x0002 = 0000 0000 0000 0010
GPIO_Pin_2 = 0x0004 = 0000 0000 0000 0100
OR result   = 0x0007 = 0000 0000 0000 0111  ← 同时选中3个
```

> RCC时钟控制和SetBits/ResetBits也支持按位或选多个

### 4.3 GPIO_Write控制全部端口

```c
// GPIO_Write 直接写ODR寄存器
GPIO_Write(GPIOA, ~0x0001);  // 仅PA0低电平（亮），其余高电平（灭）
Delay_ms(500);
GPIO_Write(GPIOA, ~0x0002);  // 仅PA1低电平
Delay_ms(500);
GPIO_Write(GPIOA, ~0x0004);  // 仅PA2低电平
Delay_ms(500);
// ...流水灯逐个点亮（低电平驱动需按位取反 ~）
```

### 4.4 输出函数对比

| 函数 | 控制范围 | 使用场景 |
|------|---------|---------|
| `GPIO_SetBits/ResetBits` | 单/多引脚 | 控制少量引脚 |
| `GPIO_WriteBit` | 单个引脚 | 带枚举参数的位操作 |
| `GPIO_Write` | 全部16引脚 | 流水灯、数码管等 |

---

## 五、程序3：蜂鸣器

### 5.1 电路

```
PB12 ← 蜂鸣器IO控制脚
VCC ← 蜂鸣器正极（接3.3V供电排）
GND ← 蜂鸣器负极（接GND供电排）
```

### 5.2 代码

```c
// 时钟改GPIOB
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

// 初始化PB12
GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
GPIO_Init(GPIOB, &GPIO_InitStructure);

// 主循环（滴滴声）
while (1)
{
    GPIO_ResetBits(GPIOB, GPIO_Pin_12);  // 低电平→蜂鸣器响
    Delay_ms(100);
    GPIO_SetBits(GPIOB, GPIO_Pin_12);    // 高电平→蜂鸣器停
    Delay_ms(100);
    GPIO_ResetBits(GPIOB, GPIO_Pin_12);  // 响
    Delay_ms(100);
    GPIO_SetBits(GPIOB, GPIO_Pin_12);    // 停
    Delay_ms(700);
}
```

### 5.3 推挽 vs 开漏实测

| 模式 | 低电平(接LED负极) | 高电平(接LED正极) |
|------|------------------|-------------------|
| **推挽输出** | ✅ 点亮 | ✅ 点亮 |
| **开漏输出** | ✅ 点亮 | ❌ 不亮 |

> 结论：推挽输出高低电平都有驱动能力；开漏输出仅低电平有驱动。一般用推挽即可。

### 5.4 避坑：调试端口不能随便用

| 端口 | 默认功能 | 用作普通IO |
|------|---------|-----------|
| PA13 | SWDIO（调试数据） | ❌ 不要占用 |
| PA14 | SWCLK（调试时钟） | ❌ 不要占用 |
| PA15 | JTDI | 需配置后用 |
| PB3 | JTDO | 需配置后用 |
| PB4 | JNTRST | 需配置后用 |

> 不小心全配成IO口会导致无法下载程序，需串口救急！

---

## 六、库函数查阅方法

| 方法 | 路径 | 优缺点 |
|------|------|--------|
| **右键跳转定义** | Keil中点函数 → Go to Definition | ✅ 直接看源码 ✅ 参数说明准确 |
| **库函数用户手册** | 资料包中的中文CHM | ✅ 有例子 ❌ 版本不一定对应 |
| **百度搜索** | 搜 "STM32 GPIO 初始化" | ✅ 丰富 ❌ 需要甄别 |

---

## 七、工程管理技巧

### 7.1 快速复制工程

直接复制整个工程文件夹，改名后双击 `.uvprojx` 即可。

### 7.2 清理中间文件

编译后的 `Listings/` 和 `Objects/` 文件夹很大（~20MB），用批处理脚本删除后可压缩到 ~2MB，方便分享。

### 7.3 强制类型转换技巧

```c
GPIO_WriteBit(GPIOA, GPIO_Pin_0, (BitAction)0);  // 强制转为枚举，避免警告
GPIO_WriteBit(GPIOA, GPIO_Pin_0, (BitAction)1);
```

---

## 八、关键要点总结

1. **GPIO三步走**：时钟 → Init → 输出/输入
2. **按位或 `|` 可同时选中多个引脚**（RCC、GPIO_Pin、SetBits等通用）
3. **GPIO_Write** 一次性控制全部16端口，适合流水灯
4. **默认用推挽输出**，开漏输出仅特殊场景（I2C、输出5V）
5. **调试口 PA13/PA14 不要占用**为普通IO
6. 延时函数用现成的 Delay 模块，底层是 SysTick
7. 复制粘贴工程来快速开始新程序
