# STM32 DMA 数据转运 & DMA+AD 多通道
> 来源：B站 STM32入门教程-2023版 P24  
> 芯片：STM32F103C8（中密度）  
> 日期：2026-05-25

---

## 目录
1. [存储器地址与变量存储位置](#1-存储器地址与变量存储位置)
2. [外设寄存器地址查找方法](#2-外设寄存器地址查找方法)
3. [结构体访问寄存器的原理](#3-结构体访问寄存器的原理)
4. [DMA 存储器到存储器数据转运](#4-dma-存储器到存储器数据转运)
5. [DMA + ADC 多通道采集](#5-dma--adc-多通道采集)
6. [关键陷阱与最佳实践](#6-关键陷阱与最佳实践)
7. [寄存器速查表](#7-寄存器速查表)

---

## 1. 存储器地址与变量存储位置

### 1.1 地址空间划分（STM32F103）

| 地址范围 | 区域 | 说明 |
|----------|------|------|
| `0x2000 0000` 开始 | SRAM | 变量存储区，运行时可读写 |
| `0x0800 0000` 开始 | Flash | 程序代码 + `const` 常量，只读 |
| `0x4000 0000` 开始 | 外设 | 外设寄存器区域 |

### 1.2 验证变量地址的实验

```c
// 普通变量 → 存在 SRAM（地址 0x2000 开头）
uint8_t AA = 0x66;
OLED_ShowHexNum(1, 1, AA, 2);
OLED_ShowHexNum(2, 1, (uint32_t)&AA, 8);  // 显示 0x20000000

// const 常量 → 存在 Flash（地址 0x0800 开头）
const uint8_t AA = 0x66;
OLED_ShowHexNum(2, 1, (uint32_t)&AA, 8);  // 显示 0x0800xxxx
```

### 1.3 `const` 关键字的作用

```c
// 不加 const → 存储在 SRAM，占用宝贵的内存空间
uint8_t OLED_Font[][16] = {...};  // 大字库会消耗大量 SRAM！

// 加 const → 存储在 Flash，节省 SRAM
const uint8_t OLED_Font[][16] = {...};  // 只读，不占 SRAM
```

> **最佳实践**：查找表、字库、固定数据 → 一律加 `const`，存 Flash。

---

## 2. 外设寄存器地址查找方法

### 2.1 手动查手册计算地址

**步骤**：
1. 查参考手册**存储器映像图** → 得到外设基地址
2. 查外设章节**寄存器总表** → 得到寄存器偏移量
3. 基地址 + 偏移量 = 寄存器实际地址

**示例：ADC1_DR 寄存器**
```
APB2 外设基地址  = 外设基地址 + 0x10000 = 0x40010000
ADC1 基地址        = APB2 基地址 + 0x2400  = 0x40012400
ADC_DR 偏移量     = 0x4C
ADC1_DR 实际地址  = 0x40012400 + 0x4C     = 0x4001244C
```

### 2.2 用库函数直接获取（推荐）

```c
// 无需手动计算，库函数已帮你算好
#define ADC1_DR_ADDR  ((uint32_t)&(ADC1->DR))

// 或直接用结构体指针访问
uint16_t adcValue = ADC1->DR;  // 读取 ADC1 数据寄存器
```

---

## 3. 结构体访问寄存器的原理

### 3.1 原理说明

STM32 标准外设驱动使用**结构体指针**来映射寄存器：

```c
// 在 stm32f10x.h 中的定义
#define ADC1  ((ADC_TypeDef *)ADC1_BASE)

// ADC_TypeDef 结构体成员顺序与寄存器偏移一一对应
typedef struct {
    __IO uint32_t SR;    // 偏移 0x00
    __IO uint32_t CR1;   // 偏移 0x04
    __IO uint32_t CR2;   // 偏移 0x08
    // ...
    __IO uint32_t DR;    // 偏移 0x4C ← 我们要找的！
} ADC_TypeDef;
```

**内存映射图**：
```
地址 →   0x40012400  |  0x40012404  |  ...  |  0x4001244C
内容 →   SR 寄存器  |  CR1 寄存器  |  ...  |  DR 寄存器
结构体 →  ADC1->SR   |  ADC1->CR1   |  ...  |  ADC1->DR
```

### 3.2 两种访问寄存器的方式对比

```c
// 方式一：结构体访问（库函数风格，推荐）
uint16_t val = ADC1->DR;

// 方式二：直接指针访问（等价于方式一）
#define ADC1_DR  (*(uint32_t *)0x4001244C)
uint16_t val = ADC1_DR;
```

---

## 4. DMA 存储器到存储器数据转运

### 4.1 DMA 框图参数说明

```
        外设站点                  存储器站点
    ┌──────────┐          ┌──────────┐
    │  基地址    │          │  基地址    │
    │  数据宽度  │          │  数据宽度  │
    │  地址自增  │          │  地址自增  │
    └─────┬────┘          └─────┬────┘
          │    传输方向     │
          └──────┬──────┘
               │
         ┌────▼────┐
         │ 传输计数器 │  (BufferSize)
         │ 传输模式   │  (是否自动重装)
         └─────────┘
```

### 4.2 DMA 初始化步骤

```c
#include "stm32f10x.h"

uint8_t DataA[] = {0x01, 0x02, 0x03, 0x04};  // 源数组
uint8_t DataB[] = {0x00, 0x00, 0x00, 0x00};  // 目的数组
uint16_t MyDMA_Size;  // 全局变量，记录传输次数

void MyDMA_Init(uint32_t AddrA, uint32_t AddrB, uint16_t Size) {
    // 第一步：开启 DMA 时钟（DMA 挂在 AHB 总线上）
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    // 第二步：配置 DMA 初始化结构体
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_PeripheralBaseAddr = AddrA;          // 外设站点基地址（源）
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;   // 字节宽度
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Enable;  // 地址自增

    DMA_InitStructure.DMA_MemoryBaseAddr = AddrB;                 // 存储器站点基地址（目的）
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;      // 字节宽度
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;           // 地址自增

    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;   // 外设作为数据源（→ 存储器）
    DMA_InitStructure.DMA_BufferSize = Size;                // 传输次数（传输计数器）
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;          // 正常模式（不自动重装）
    DMA_InitStructure.DMA_M2M = DMA_M2M_Enable;           // 存储器到存储器（软件触发）
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;   // 中等优先级

    DMA_Init(DMA1_Channel1, &DMA_InitStructure);

    // 记录 size，供后续重新启动使用
    MyDMA_Size = Size;

    // 注意：这里不立刻使能，等调用 Transfer 时再启动
}

void MyDMA_Transfer(void) {
    // 重新启动 DMA 前，必须先失能
    DMA_Cmd(DMA1_Channel1, DISABLE);

    // 重新给传输计数器赋值
    DMA_SetCurrDataCounter(DMA1_Channel1, MyDMA_Size);

    // 使能 DMA，软件触发立即开始转运
    DMA_Cmd(DMA1_Channel1, ENABLE);

    // 等待转运完成（查询 TC 标志位）
    while (DMA_GetFlagStatus(DMA1_FLAG_TC1) == RESET);

    // 清除转运完成标志位（必须手动清除！）
    DMA_ClearFlag(DMA1_FLAG_TC1);
}
```

### 4.3 DMA 转运的三个条件（缺一不可）

| 条件 | 说明 |
|------|------|
| ① 传输计数器 > 0 | `DMA_BufferSize` 不为零 |
| ② 触发信号到来 | 软件触发：一直有信号；硬件触发：外设信号 |
| ③ DMA 使能 | `DMA_Cmd(ENABLE)` |

### 4.4 主函数调用示例

```c
int main(void) {
    OLED_Init();
    MyDMA_Init((uint32_t)DataA, (uint32_t)DataB, 4);
    // 注意：初始化时不使能，等下面手动触发

    while (1) {
        // 变换源数据
        DataA[0]++; DataA[1]++; DataA[2]++; DataA[3]++;

        // 显示转运前数据
        OLED_ShowHexNum(1, 1, DataA[0], 2);
        // ...

        // 启动 DMA 转运
        MyDMA_Transfer();

        // 显示转运后数据（DataB 内容与 DataA 相同）
        OLED_ShowHexNum(2, 1, DataB[0], 2);
        // ...

        Delay_ms(1000);
    }
}
```

### 4.5 重要限制

> ⚠️ **软件触发 + 自动重装 不能同时使用！**
> 若同时使用，DMA 会连续不断触发，永远停不下来。
> 存储器到存储器模式只能用**正常模式（不自动重装）**。

---

## 5. DMA + ADC 多通道采集

### 5.1 硬件接线

| 外设 | STM32 引脚 | 功能 |
|------|------------|------|
| 电位器 | PA0 | ADC 通道 0（电位器） |
| 光敏传感器 | PA1 | ADC 通道 1 |
| 热敏传感器 | PA2 | ADC 通道 2 |
| 红外反射传感器 | PA3 | ADC 通道 3 |

### 5.2 ADC 多通道配置（扫描模式）

```c
void AD_Init(void) {
    // 开启时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // 配置 PA0~PA3 为模拟输入
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // ADC 初始化
    ADC_InitTypeDef ADC_InitStructure;
    ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode      = ENABLE;   // 扫描模式（多通道）
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; // 单次扫描（也可开连续）
    ADC_InitStructure.ADC_ExternalTrigConv  = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign         = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel     = 4;       // 4 个通道
    ADC_Init(ADC1, &ADC_InitStructure);

    // 配置规则组序列（菜单）
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_55Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 3, ADC_SampleTime_55Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 4, ADC_SampleTime_55Cycles5);

    // 开启 ADC 到 DMA 的触发信号（关键！）
    ADC_DMACmd(ADC1, ENABLE);

    // 使能 ADC
    ADC_Cmd(ADC1, ENABLE);
    // ...（校准步骤省略，参考之前代码）
}
```

### 5.3 DMA 配置（配合 ADC）

```c
uint16_t ADValue[4];  // 存储 4 个通道的 ADC 结果

void MyDMA_Init(void) {
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_InitTypeDef DMA_InitStructure;
    // 外设站点 = ADC1 的 DR 寄存器
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(ADC1->DR);
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;  // 16位
    DMA_InitStructure.DMA_PeripheralInc     = DMA_PeripheralInc_Disable;  // 始终读同一个 DR

    // 存储器站点 = ADValue 数组
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)ADValue;
    DMA_InitStructure.DMA_MemoryDataSize  = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryInc      = DMA_MemoryInc_Enable;  // 数组地址自增

    DMA_InitStructure.DMA_DIR        = DMA_DIR_PeripheralSRC;  // 外设作为数据源
    DMA_InitStructure.DMA_BufferSize = 4;                      // 4 个 ADC 通道
    DMA_InitStructure.DMA_Mode       = DMA_Mode_Normal;        // 或 Circular
    DMA_InitStructure.DMA_M2M        = DMA_M2M_Disable;       // 硬件触发（非存储器到存储器）
    DMA_InitStructure.DMA_Priority   = DMA_Priority_Medium;

    // ⚠️ 关键：ADC1 硬件触发只接在 DMA1 的通道 1 上！
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);

    // 使能 DMA（此时硬件触发还没来，不会立刻转运）
    DMA_Cmd(DMA1_Channel1, ENABLE);
}
```

### 5.4 两种工作模式对比

#### 模式一：单次扫描 + 单次转运

```c
// ADC 配置：ADC_ContinuousConvMode = DISABLE（单次）
// DMA 配置：DMA_Mode = DMA_Mode_Normal（不自动重装）

void AD_GetValue(void) {
    // 重新设置传输计数器（单次模式每次都要设）
    DMA_Cmd(DMA1_Channel1, DISABLE);
    DMA_SetCurrDataCounter(DMA1_Channel1, 4);
    DMA_Cmd(DMA1_Channel1, ENABLE);

    // 软件触发 ADC 开始转换
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    // 等待 DMA 转运完成
    while (DMA_GetFlagStatus(DMA1_FLAG_TC1) == RESET);
    DMA_ClearFlag(DMA1_FLAG_TC1);
}

// 主循环调用
while (1) {
    AD_GetValue();
    OLED_ShowNum(1, 1, ADValue[0], 4);  // 通道 0 结果
    OLED_ShowNum(1, 9, ADValue[1], 4);  // 通道 1 结果
    // ...
    Delay_ms(1000);
}
```

#### 模式二：连续扫描 + 循环转运（推荐）

```c
// ADC 配置：ADC_ContinuousConvMode = ENABLE（连续扫描）
// DMA 配置：DMA_Mode = DMA_Mode_Circular（自动重装）

// 初始化完成后，直接启动 ADC：
ADC_SoftwareStartConvCmd(ADC1, ENABLE);
// DMA 设置为 Circular 模式后，ADC 每转换完一个通道，
// DMA 自动把结果搬到 ADValue[]，不需要软件干预！

// 主循环直接读数组即可
while (1) {
    OLED_ShowNum(1, 1, ADValue[0], 4);
    OLED_ShowNum(1, 9, ADValue[1], 4);
    // ...
    // 不需要调用任何转换函数，数据始终是最新的！
}
```

### 5.5 硬件协同工作框图

```
    ADC1 转换完成
         │
         ▼
    ADC_DMACmd(ENABLE)  →  触发信号  →  DMA1_Channel1
                                       │
                     ┌─────────────────┤
                     │  外设站点 = ADC1->DR       │
                     │  存储器站点 = ADValue[]     │
                     │  传输计数器 = 4             │
                     └────────────────────────────┘
                                │
                                ▼
                     ADValue[0] = 通道0结果
                     ADValue[1] = 通道1结果
                     ADValue[2] = 通道2结果
                     ADValue[3] = 通道3结果
```

> **硬件自动化特色**：ADC 转换完 → 自动触发 DMA → DMA 自动转运数据到数组，
> CPU 完全不需要干预，也不需要中断！

---

## 6. 关键陷阱与最佳实践

### 6.1 常见陷阱

| 陷阱 | 说明 | 正确做法 |
|------|------|----------|
| 软件触发 + 自动重装同时使用 | DMA 会无限循环 | 存储器到存储器时，只用 Normal 模式 |
| ADC 没开启 DMA 触发 | DMA 永远等不到触发信号 | 务必调用 `ADC_DMACmd(ADC1, ENABLE)` |
| DMA 通道选错 | ADC1 硬件触发只接 DMA1\_Channel1 | 必须用 `DMA1_Channel1` |
| 传输计数器不清零就想重启 | DMA 不工作 | 先 `DMA_Cmd(DISABLE)`，再设新值 |
| 标志位不清除 | 下次判断会出错 | 传输完成后手动 `DMA_ClearFlag()` |
| `const` 变量试图修改 | 编译报错 | `const` 只在定义时赋值，之后只读 |

### 6.2 最佳实践

- **大数据表加 `const`**：字库、查找表等 → 省 SRAM
- **DMA  Circular 模式 + 连续扫描**：最省 CPU，数据始终最新
- **地址计算用库函数**：`&(ADC1->DR)` 比手算地址安全
- **等待 DMA 完成要查询标志位**：`DMA_FLAG_TCx`

---

## 7. 寄存器速查表

### 7.1 DMA 库函数速查

| 函数 | 功能 |
|------|------|
| `DMA_Init()` | 初始化 DMA 通道 |
| `DMA_Cmd()` | 使能/失能 DMA 通道 |
| `DMA_SetCurrDataCounter()` | 设置传输计数器（重新启动传输） |
| `DMA_GetCurrDataCounter()` | 获取当前剩余传输次数 |
| `DMA_GetFlagStatus()` | 查询标志位（完成/过半/错误） |
| `DMA_ClearFlag()` | 清除标志位 |
| `DMA_ITConfig()` | 开启 DMA 中断 |

### 7.2 ADC 多通道关键函数

| 函数 | 功能 |
|------|------|
| `ADC_RegularChannelConfig()` | 配置规则组序列（点菜单） |
| `ADC_DMACmd()` | 开启 ADC → DMA 硬件触发信号 |
| `ADC_SoftwareStartConvCmd()` | 软件触发 ADC 开始转换 |

---

## 附录：DMA 触发源对应表（STM32F103）

| 外设 | 对应的 DMA 通道 |
|------|----------------|
| ADC1 | DMA1\_Channel1（固定！） |
| TIM2\_UP | DMA1\_Channel2 |
| TIM3\_UP | DMA1\_Channel3 |
| UART1\_TX | DMA1\_Channel4 |
| UART1\_RX | DMA1\_Channel5 |

> 查参考手册 **DMA 请求映射表** 可得完整对应关系。

---

*笔记生成时间：2026-05-25*  
*工具：VideoCaptioner bijian ASR + WorkBuddy AI 整理*
