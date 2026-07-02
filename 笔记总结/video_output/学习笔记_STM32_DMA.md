# STM32 DMA 直接存储器存取 — 学习笔记

> 来源：B站 STM32入门教程-2023版 P23 [8-1] DMA
> 芯片：STM32F103C8T6（仅有 DMA1 的 7 个通道，无 DMA2）

---

## 一、DMA 是什么？

**DMA（Direct Memory Access）**——直接存储器存取，是协助 CPU 完成数据转运的硬件模块。

| 对比 | CPU 转运 | DMA 转运 |
|------|----------|----------|
| 方式 | for 循环逐字节复制 | 硬件自动搬运 |
| CPU 占用 | 全程占用 | **释放 CPU** |
| 触发 | 软件代码 | 软件触发 / 硬件触发 |

> 核心价值：让 CPU 去做更重要的事，数据搬运交给 DMA。

---

## 二、STM32 存储器映射

### 2.1 为什么需要了解存储器？

DMA 的本质是**存储器之间的数据搬运**。所有外设寄存器本质上也是存储器（SRAM）。只有了解各自的地址范围，才能正确配置 DMA。

### 2.2 存储器地址总表

| 区域 | 起始地址 | 类型 | 用途 |
|------|----------|------|------|
| Flash（主闪存） | `0x0800 0000` | ROM | 存储程序代码 |
| 系统存储器 | `0x1FFF xxxx` | ROM | Bootloader（串口下载） |
| 选项字节 | `0x1FFF xxxx` | ROM | Flash 读写保护、看门狗等 |
| SRAM（运行内存） | `0x2000 0000` | RAM | 变量、数组、结构体 |
| 外设寄存器 | `0x4000 0000` | RAM | 各外设配置寄存器 |
| 内核外设 | `0xE000 0000` | RAM | NVIC、SysTick 等 |

**速记口诀：**
- `0x0800` 开头 → Flash（程序代码）
- `0x2000` 开头 → SRAM（运行变量）
- `0x4000` 开头 → 外设寄存器
- CPU 32 位寻址范围 4GB，STM32 实际使用不到 1%

---

## 三、DMA 基本结构

```
┌─────────────┐     ┌──────────────┐     ┌─────────────┐
│  源端站点    │ ──► │   DMA 控制器  │ ──► │  目标站点    │
│ (外设/存储器) │     │  • 传输计数器  │     │ (外设/存储器) │
└─────────────┘     │  • 自动重装器  │     └─────────────┘
                    │  • 触发控制    │
                    └──────────────┘
```

### 3.1 三个核心站点参数

| 参数 | 作用 | 选项 |
|------|------|------|
| **起始地址** | 源/目的数据的首地址 | 32 位地址 |
| **数据宽度** | 每次转运的数据大小 | Byte(8位) / HalfWord(16位) / Word(32位) |
| **地址自增** | 转运后地址是否 +1 | ENABLE / DISABLE |

### 3.2 传输计数器 & 自动重装

```
传输计数器 = N  →  转运 N 次 → 计数器归零 → 停止

自动重装器：
  DISABLE → 单次模式（转运 N 次后停止）
  ENABLE  → 循环模式（归零后自动恢复到 N，继续转运）
```

> 类比：循环模式 ≈ ADC 连续转换模式

### 3.3 触发控制

| M2M 位 | 触发方式 | 适用场景 |
|--------|----------|----------|
| `M2M = 1` | **软件触发** | 存储器→存储器（尽快完成，连续触发） |
| `M2M = 0` | **硬件触发** | 外设相关转运（需要等待时机） |

> 软件触发 ≠ 调用一次触发一次，而是以最快速度连续触发直到计数器归零。
> 软件触发 + 循环模式 = DMA 停不下来，**不能同时用！**

---

## 四、DMA 通道与硬件触发源

### 4.1 核心规则

**每个通道的硬件触发源不同，使用某个外设的硬件触发就必须用对应的通道。**

| 通道 | 硬件触发源（可选） | 常用场景 |
|------|-------------------|----------|
| 通道 1 | **ADC1** / TIM2_CH3 / TIM4_CH1 | ADC 扫描模式 |
| 通道 2 | SPI1_RX / TIM2_UP 等 | SPI 接收 |
| 通道 3 | USART1_TX / TIM3_CH1 等 | 串口发送 |
| 通道 4 | USART1_RX / TIM3_TRIG 等 | 串口接收 |
| 通道 5 | USART2_TX 等 | 串口 2 发送 |
| 通道 6 | USART2_RX 等 | 串口 2 接收 |
| 通道 7 | TIM2_UP / TIM3_CH4 等 | 定时器 |

> 软件触发不挑通道，任意均可。

---

## 五、数据宽度与对齐

| 源端宽度 | 目标宽度 | 行为 |
|----------|----------|------|
| 8 bit → 8 bit | 一致 | 正常转运 |
| 8 bit → 16 bit | 目标更宽 | 高位补 0 |
| 16 bit → 8 bit | 目标更窄 | 高位舍弃 |
| 16 bit → 32 bit | 目标更宽 | 高位补 0 |

> 规则：等同于 `uint8_t` ↔ `uint16_t` ↔ `uint32_t` 之间赋值的行为。

---

## 六、编程实战

### 6.1 场景一：存储器到存储器（数组复制）

**任务**：将 `dataA[4] = {1,2,3,4}` 复制到 `dataB[4]`

```c
// 配置 DMA
DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)dataA;    // 源地址
DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)dataB;    // 目标地址
DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC; // 外设→存储器
DMA_InitStructure.DMA_BufferSize         = 4;                   // 传输 4 次
DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Enable;  // 源地址自增
DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;      // 目标地址自增
DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; // 8位宽度
DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;    // 单次模式
DMA_InitStructure.DMA_Priority           = DMA_Priority_Medium;
DMA_InitStructure.DMA_M2M                = DMA_M2M_Enable;     // 软件触发！
DMA_Init(DMA1_Channel1, &DMA_InitStructure);

DMA_Cmd(DMA1_Channel1, ENABLE);  // 开启后自动开始转运
```

**地址自增效果对照：**

| 源自增 | 目自增 | 结果 |
|--------|--------|------|
| ✅ | ✅ | `B[0]=A[0], B[1]=A[1], ...` 正确复制 |
| ❌ | ✅ | 所有 B 元素都 = `A[0]` |
| ✅ | ❌ | `B[0]` = `A[最后一个]`，其他不变 |
| ❌ | ❌ | `B[0]=A[0]`，其他不变 |

### 6.2 场景二：ADC 扫描模式 + DMA

**问题**：ADC 扫描模式数据覆盖——7 个通道结果都写入同一个 DR 寄存器，前面的被覆盖。

**DMA 解决方案**：

```c
uint16_t AD_Value[7];  // 定义数组存放多通道结果

DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR; // ADC 数据寄存器
DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)AD_Value;  // 目标数组
DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC; //外设到存储器
DMA_InitStructure.DMA_BufferSize         = 7;                    // 7 个通道
DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable; // 外设地址不变
DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;      // 目标地址自增
DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; // 16位
DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
DMA_InitStructure.DMA_Mode               = DMA_Mode_Circular;  // ADC连续则用循环
DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;    // 硬件触发！
DMA_Init(DMA1_Channel1, &DMA_InitStructure);  // ADC1 必须用通道 1

DMA_Cmd(DMA1_Channel1, ENABLE);
ADC_DMACmd(ADC1, ENABLE);  // 开启 ADC 的 DMA 输出
```

**ADC 单通道转换完成 → 触发 DMA → DR 数据搬运到数组 → 地址自增**，周而复始。

---

## 七、运转条件 & 注意事项

### DMA 正常工作的三个条件
1. `DMA_Cmd(ENABLE)` — 开关使能
2. **传输计数器 > 0**
3. **触发信号到达**

### 关键注意
- 写传输计数器前**必须先关 DMA**，写完再开
- 软件触发 + 循环模式 = 死循环，**严禁同时使用**
- ADC 扫描模式 + DMA 是**经典组合**——ADC 数据覆盖是固有缺陷，DMA 是唯一优雅的解决方案

---

## 八、寄存器速查

| 寄存器 | 作用 |
|--------|------|
| `DMA_ISR` / `DMA_IFCR` | 中断状态 / 清除标志 |
| `DMA_CCRx` | 通道配置（数据宽度、自增、循环、方向等） |
| `DMA_CNDTRx` | 传输计数器（0~65535） |
| `DMA_CPARx` | 外设地址 |
| `DMA_CMARx` | 存储器地址 |

---

## 九、知识小结

```
DMA 数据转运
├── 触发方式
│   ├── 软件触发 (M2M=1) ──► 存储器→存储器（数组复制）
│   └── 硬件触发 (M2M=0) ──► ADC/串口/定时器触发（需选正确通道）
├── 转运模式
│   ├── 单次 (Normal)  ──► 计数归零停止
│   └── 循环 (Circular) ──► 自动重装，配合 ADC 连续模式
└── 经典组合
    ├── DMA + ADC 扫描模式（解决数据覆盖问题）
    ├── DMA + 串口（高效收发）
    └── DMA + 存储器（释放 CPU）
```

> DMA 对于 ADC 扫描模式是**刚需**，对其他外设是**锦上添花**。
