# STM32入门教程 P07：[3-3] GPIO输入 + C语言基础

> **视频来源**：B站 BV1th411z7sn | 江协科技 | STM32入门教程-2023版
> **时长**：43:59 | **芯片**：STM32F103C8T6

---

## 一、本节程序现象

| 程序 | 功能 | 硬件 |
|------|------|------|
| 按键控制LED | 按一下亮，再按一下灭，两个按键各控一个LED | 2×按键 + 2×LED |
| 光敏传感器控制蜂鸣器 | 手遮住→光线暗→蜂鸣器响；手离开→蜂鸣器停 | 光敏模块 + 蜂鸣器 |

---

## 二、按键抖动与消抖

### 2.1 抖动现象

```
        ┌──┐┌──┐
高电平──┘  └┘  └────────── 松手：也有抖动（5~10ms）
              ↑
        按下瞬间抖动（5~10ms）
```

> 人眼分辨不出，但单片机速度快，会误判为多次按下。

### 2.2 消抖方案

```c
if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0)  // 检测按下
{
    Delay_ms(20);                                      // 消抖延时20ms
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0) // 再次确认
    {
        // 执行按键操作
        while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0); // 等待释放
    }
}
```

---

## 三、传感器模块电路原理

### 3.1 基本框架

```
VCC ──┬── R1(定值电阻) ──┬── N1(传感器电阻) ── GND
      │                  │
      │                  ├── AO(模拟电压输出) → 排针
      │                  │
      │                  └── LM393比较器 ──→ DO(数字输出) → 排针
      └── C2(滤波电容) ── GND
```

### 3.2 分压原理（弹簧模型）

把上下拉电阻想象成弹簧：

| 元件 | 弹簧类比 | 电学含义 |
|------|---------|---------|
| R1(上拉) | 屋顶弹簧，向上拉 | 接VCC |
| N1(下拉) | 地面弹簧，向下拉 | 接GND |
| 电阻值↓ | 弹簧拉力↑ | 阻值小=拉力强 |
| 电阻值=0 | 拉力无穷大 | 短路 |
| AO端电压 | 弹簧拉扯后液面的高度 | 分压结果 |

> N1阻值变小 → 下拉变强 → AO电压降低
> N1阻值变大 → 下拉变弱 → AO电压升高

### 3.3 LM393 电压比较器

```
同相端(+) ← AO(模拟电压)
反相端(-) ← 电位器(可调阈值)
         ↓
    AO > 阈值 → 输出高(VCC)
    AO < 阈值 → 输出低(GND)
         ↓
      DO(数字输出)
```

> 传感器模块通过比较器实现模拟量→数字量的二值化。

### 3.4 四种传感器对比

| 传感器 | N1元件 | 检测量 | 特点 |
|--------|--------|--------|------|
| 光敏电阻 | 光敏电阻 | 光照强度 | 光强↑→阻值↓ |
| 热敏电阻 | 热敏电阻 | 温度 | 温度↑→阻值↓ |
| 对射式红外 | 红外接收管 | 遮挡/通断 | 有发射管+接收管 |
| 反射式红外 | 红外接收管 | 反射光强（循迹） | 向下发射+接收 |

---

## 四、按键的四种接法

### 4.1 下接按键（推荐，按下=低电平）

| 方案 | 电路 | 要求 | 按下 | 松手 |
|------|------|------|------|------|
| ① 内部上拉 | PA0 ─[按键]─ GND | 必须上拉输入 | 0 | 1(上拉) |
| ② 外部上拉 | PA0 ─[按键]─ GND, PA0 ─[10K]─ 3.3V | 浮空/上拉均可 | 0 | 1(外部上拉) |

### 4.2 上接按键（按下=高电平，少用）

| 方案 | 电路 | 要求 | 按下 | 松手 |
|------|------|------|------|------|
| ③ 内部下拉 | PA0 ─[按键]─ 3.3V | 必须下拉输入 | 1 | 0(下拉) |
| ④ 外部下拉 | PA0 ─[按键]─ 3.3V, PA0 ─[10K]─ GND | 浮空/下拉均可 | 1 | 0(外部下拉) |

> **常用方案①**：内部上拉+按键接地，最简单，和LED低电平驱动习惯一致。

---

## 五、C语言基础补充（库函数必备）

### 5.1 数据类型重定义

| C标准类型 | stdint.h 名称 | 位数 | 范围 |
|-----------|-------------|------|------|
| `char` | `int8_t` | 8 | -128~127 |
| `unsigned char` | `uint8_t` | 8 | 0~255 |
| `short` | `int16_t` | 16 | -32768~32767 |
| `unsigned short` | `uint16_t` | 16 | 0~65535 |
| `int` | `int32_t` | **32** | ±21亿 |
| `unsigned int` | `uint32_t` | 32 | 0~42亿 |

> **注意**：STM32中 `int` 是 **32位**！51单片机中 `int` 是16位，不要混淆。

旧版本库函数用 `u8`/`u16`/`u32`，新版改用 `uint8_t`/`uint16_t`/`uint32_t`。

### 5.2 宏定义 #define

两种用途：
1. **语义化数字**：`#define GPIO_Pin_0 ((uint16_t)0x0001)` → 用名字代替难懂的十六进制
2. **统一修改参数**：10处 `GPIO_Pin_0` 改一处定义即可

```c
// 按键引脚可配置化
#define KEY1_PIN    GPIO_Pin_0
#define KEY1_PORT   GPIOA

if (GPIO_ReadInputDataBit(KEY1_PORT, KEY1_PIN) == 0) { ... }
```

### 5.3 Typedef（类型重命名）

| | #define | typedef |
|---|---------|---------|
| 新名字位置 | 左边 | 右边 |
| 分号 | 不需要 | **必须加分号** |
| 用途 | 任意字符串替换 | **只能给类型换名** |
| 安全性 | 无检查 | 会检查类型合法性 |

```c
typedef unsigned char uint8_t;   // 给 unsigned char 起新名字
uint8_t a;                        // 等价于 unsigned char a;
```

### 5.4 结构体 struct

> 结构体 = 不同数据类型的打包集合（数组只能存相同类型）

```c
// 定义结构体类型
struct Student {
    char name[20];
    uint8_t age;
    float score;
};

// 使用
struct Student s1;
s1.age = 20;

// typedef 简化
typedef struct {
    uint16_t GPIO_Pin;
    GPIOSpeed_TypeDef GPIO_Speed;
    GPIOMode_TypeDef GPIO_Mode;
} GPIO_InitTypeDef;

GPIO_InitTypeDef GPIO_InitStructure;  // 不用再加 struct 关键字
```

### 5.5 枚举 enum

```c
typedef enum {
    GPIO_Speed_10MHz = 1,
    GPIO_Speed_2MHz,
    GPIO_Speed_50MHz
} GPIOSpeed_TypeDef;
// GPIO_Speed_10MHz = 1, GPIO_Speed_2MHz = 2, GPIO_Speed_50MHz = 3
```

> 枚举 = 给一组相关常量起名字，限制取值范围，增加代码可读性。

---

## 六、GPIO输入操作代码

### 6.1 初始化（以按键为例，上拉输入）

```c
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

GPIO_InitTypeDef GPIO_InitStructure;
GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;   // 上拉输入
GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0;
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
GPIO_Init(GPIOA, &GPIO_InitStructure);
```

### 6.2 读取

```c
// 读单个引脚
if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == Bit_RESET)  // 0=低电平
{
    // 按键按下（低电平有效）
}

// 读整个端口
uint16_t port_val = GPIO_ReadInputData(GPIOA);
```

---

## 七、关键要点总结

1. **按键必须消抖**：延时20ms即可滤除机械抖动
2. **下接按键+上拉输入**最常用（按=0，松=1）
3. 传感器模块用**分压+比较器**将模拟量转为数字量
4. **弹簧模型**理解上下拉：阻值越小=拉力越强
5. **STM32中 int=32位**，16位用short
6. **typedef**给类型重命名（检查合法性），**#define**给任意字符串替换
7. **struct**打包不同类型数据，库函数Init全是结构体传参
8. **enum**给相关常量命名，限制取值防止乱传参数
