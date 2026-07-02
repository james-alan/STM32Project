# STM32 ADC 模数转换 — 学习笔记

> 来源：B站 STM32入门教程-2023版 P22 [7-2] AD单通道 & AD多通道
> 芯片：STM32F103C8（中密度）

---

## 一、硬件接线

### 1.1 电位器（单通道）

```
    GND ─── 电位器左端（固定端）
    VCC ─── 电位器右端（固定端）
    PA0  ─── 电位器中端（滑动抽头，输出可调电压）
```

- 电位器三个引脚：左右为电阻两端，中间为滑动抽头
- 中间输出 0 ~ 3.3V 可调电压，接入 PA0

### 1.2 ADC 通道引脚

| 引脚范围 | 数量 | 用途 |
|----------|------|------|
| PA0 ~ PB7 | 10 个 | ADC 通道（可任意选择） |

> 只有这 10 个引脚能接模拟电压，其他 GPIO 不支持 ADC。

### 1.3 多通道接线

| 通道 | 引脚 | 传感器 |
|------|------|--------|
| 通道 0 | PA0 | 电位器 |
| 通道 1 | PA1 | 光敏传感器 |
| 通道 2 | PA2 | 热敏传感器 |
| 通道 3 | PA3 | 反射式红外传感器 |

> 所有传感器 VCC / GND 分别接面包板正负极，AO（模拟输出）接对应 ADC 引脚。

---

## 二、ADC 初始化流程

### 步骤总览

```
RCC时钟 ──► GPIO(模拟输入) ──► 通道选择(多路开关) ──► ADC结构体配置 ──► 校准 ──► 开启ADC
```

### 2.1 开启 RCC 时钟

```c
RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);  // ADC 时钟
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // GPIO 时钟
RCC_ADCCLKConfig(RCC_PCLK2_Div6);  // ADCCLK = 72MHz / 6 = 12MHz
```

> ADCCLK 最大 14MHz，通常选 6 分频得到 12MHz。

### 2.2 配置 GPIO 为模拟输入

```c
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0;
GPIO_Init(GPIOA, &GPIO_InitStructure);
```

### 2.3 选择 ADC 通道（规则组）

```c
ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);
//                        ADCx  通道号          排序  采样时间
```

- 规则组相当于"菜单"，把要转换的通道列入规则组

### 2.4 ADC 结构体初始化

```c
ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;  // 独立模式
ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;   // 右对齐
ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None; // 软件触发
ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;               // 单次转换
ADC_InitStructure.ADC_ScanConvMode       = DISABLE;               // 非扫描
ADC_InitStructure.ADC_NbrOfChannel       = 1;                     // 1个通道
ADC_Init(ADC1, &ADC_InitStructure);
```

**关键参数说明：**

| 参数 | 选项 | 说明 |
|------|------|------|
| `ADC_Mode` | Independent | 独立模式（单 ADC） |
| `ADC_DataAlign` | Right / Left | 数据对齐方式 |
| `ADC_ExternalTrigConv` | None | 软件触发（不用外部触发） |
| `ADC_ContinuousConvMode` | ENABLE / DISABLE | 连续转换 vs 单次转换 |
| `ADC_ScanConvMode` | ENABLE / DISABLE | 扫描模式 vs 非扫描 |
| `ADC_NbrOfChannel` | 1 ~ 16 | 规则组通道数量 |

### 2.5 开启 ADC + 校准

```c
ADC_Cmd(ADC1, ENABLE);  // 开启 ADC

// 校准（减小误差）
ADC_ResetCalibration(ADC1);
while (ADC_GetResetCalibrationStatus(ADC1) == SET);
ADC_StartCalibration(ADC1);
while (ADC_GetCalibrationStatus(ADC1) == SET);
```

---

## 三、关键库函数速查

| 函数 | 分类 | 作用 |
|------|------|------|
| `RCC_ADCCLKConfig()` | RCC | 配置 ADCCLK 分频器（2/4/6/8分频） |
| `ADC_Init()` | ADC | 结构体初始化 ADC |
| `ADC_Cmd()` | ADC | ADC 上电/断电 |
| `ADC_DMACmd()` | ADC | 开启 DMA 输出（下节讲） |
| `ADC_ITConfig()` | ADC | 中断输出控制 |
| `ADC_ResetCalibration()` | ADC | 复位校准 |
| `ADC_StartCalibration()` | ADC | 开始校准 |
| `ADC_SoftwareStartConvCmd()` | ADC | **软件触发转换** |
| `ADC_GetSoftwareStartConvStatus()` | ADC | 获取软件触发状态（≠转换完成标志） |
| `ADC_GetFlagStatus()` | ADC | 获取标志位（如 EOC） |
| `ADC_GetConversionValue()` | ADC | 读取转换结果（12位） |

### 关键注意

- `ADC_GetSoftwareStartConvStatus()` 返回的是"是否已触发"，不是"转换是否完成"
- **判断转换完成应使用 `ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)`**

---

## 四、单通道 ADC 代码实现

### 4.1 AD.c — 基础版（单次转换 + 非扫描）

```c
void AD_Init(void)
{
    // 1. RCC 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    // 2. GPIO 模拟输入
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 3. 选择通道
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);

    // 4. ADC 初始化
    ADC_InitTypeDef ADC_InitStructure;
    ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
    ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ScanConvMode       = DISABLE;
    ADC_InitStructure.ADC_NbrOfChannel       = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    // 5. 开启 ADC + 校准
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1) == SET);
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1) == SET);
}

uint16_t AD_GetValue(void)
{
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);               // 软件触发
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET); // 等待转换完成
    return ADC_GetConversionValue(ADC1);                   // 读取结果
}
```

### 4.2 连续转换模式

只需修改两个地方：

```c
// 初始化中
ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;  // 改为连续转换

// 触发只需在初始化最后执行一次
ADC_SoftwareStartConvCmd(ADC1, ENABLE);

// GetValue 中无需等待
uint16_t AD_GetValue(void)
{
    return ADC_GetConversionValue(ADC1);  // 直接读（数据寄存器持续刷新）
}
```

**两种模式对比：**

| 模式 | 触发次数 | 等待 EOC | 适用场景 |
|------|----------|----------|----------|
| 单次转换 | 每次读都要触发 | 需要 | 偶尔采集 |
| 连续转换 | 初始化时触发一次 | 不需要 | 持续监测 |

---

## 五、OLED 显示电压值

```c
// 电压换算公式：V = AD_Value * 3.3 / 4095
// OLED 显示浮点数技巧：乘 100 显示两位小数

uint16_t AD_Value  = AD_GetValue();
uint16_t Voltage_x100 = (uint16_t)((float)AD_Value / 4095 * 3.3 * 100);

OLED_ShowNum(1, 5, AD_Value, 4);                    // 显示原始AD值
OLED_ShowNum(2, 5, Voltage_x100 / 100, 2);          // 整数部分
OLED_ShowString(2, 7, ".");
OLED_ShowNum(2, 8, Voltage_x100 % 100, 2);          // 小数部分
```

---

## 六、多通道 ADC 实现（核心技巧）

### 6.1 为什么不用扫描模式？

| 问题 | 详细 |
|------|------|
| 无单通道 EOC | 扫描模式只在**全部通道转换完**才产生一次 EOC |
| 数据被覆盖 | 前一个通道的数据还没取出就被后一个覆盖 |
| 速度太快 | 单通道转换仅几微秒，手动搬运来不及 |
| DMA 是正解 | 扫描模式 + DMA 才是正确方案（下节讲） |

### 6.2 本节的巧妙方案

> **单次转换 + 非扫描模式 + 每次触发前切换通道**

```c
uint16_t AD_GetValue(uint8_t ADC_Channel)
{
    // 每次转换前动态指定通道
    ADC_RegularChannelConfig(ADC1, ADC_Channel, 1, ADC_SampleTime_55Cycles5);

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(ADC1);
}
```

### 6.3 main.c 调用示例

```c
uint16_t AD0 = AD_GetValue(ADC_Channel_0);  // 电位器
uint16_t AD1 = AD_GetValue(ADC_Channel_1);  // 光敏
uint16_t AD2 = AD_GetValue(ADC_Channel_2);  // 热敏
uint16_t AD3 = AD_GetValue(ADC_Channel_3);  // 红外

OLED_ShowNum(1, 5, AD0, 4);
OLED_ShowNum(2, 5, AD1, 4);
OLED_ShowNum(3, 5, AD2, 4);
OLED_ShowNum(4, 5, AD3, 4);
```

> 本质是**四次独立的单次转换**，每次转换前修改规则组第一个通道。

---

## 七、知识脉络图

```
ADC 工作模式
├── 单次转换 (Single)
│   ├── 非扫描 ──► 单通道基础版 | 多通道切换版（本讲方法）
│   └── 扫描   ──► 需要 DMA（下讲）
└── 连续转换 (Continuous)
    ├── 非扫描 ──► 持续采集单个通道
    └── 扫描   ──► 需要 DMA（下讲）
```

---

## 八、关键总结

1. **ADC 引脚**：仅 PA0~PB7 共 10 个通道可用
2. **初始化 5 步**：RCC → GPIO(AIN) → 通道选择 → ADC_Init → 校准
3. **ADCCLK**：72MHz / 6 = 12MHz ≤ 14MHz
4. **软件触发**：`ADC_SoftwareStartConvCmd()`，等待 `ADC_FLAG_EOC`
5. **多通道巧法**：单次非扫描 + 每次触发前 `ADC_RegularChannelConfig()` 切换通道
6. **扫描模式**：留待 DMA 章节解决数据覆盖问题
7. **电压换算**：`V = AD值 × 3.3 / 4095`
