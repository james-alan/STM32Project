# STM32 [9-6] FlyMcu 串口下载 & STLINK Utility 学习笔记

## 视频信息

| 项目 | 内容 |
|---|---|
| 来源 | Bilibili：`BV1th411z7sn`，P30 |
| URL | `https://www.bilibili.com/video/BV1th411z7sn?p=30` |
| 课程 | STM32 入门教程 2023 版 |
| 本节标题 | `[9-6] FlyMcu 串口下载 & STLINK Utility` |
| 依据文件 | 同目录 `transcript.txt` 与 `.srt` 字幕 |
| 整理原则 | 只依据本集字幕/转写；明显 ASR 误识别做保守校正，不补写字幕中没有的细节 |

## 1. 真实讲解顺序

```mermaid
flowchart LR
    N0["介绍 FlyMcu 和 STLINK Utility"]
    N1["配置 Keil 生成 HEX 文件"]
    N2["USART1 连接 USB 转串口"]
    N3["BOOT0 置 1 并复位进入系统 BootLoader"]
    N4["FlyMcu 下载程序到主 FLASH"]
    N5["BOOT0 恢复 0 并复位运行用户程序"]
    N6["讲解启动区域和存储器映像"]
    N7["介绍 STLINK Utility 的查看、下载、擦除功能"]
    N0 --> N1
    N1 --> N2
    N2 --> N3
    N3 --> N4
    N4 --> N5
    N5 --> N6
    N6 --> N7
```

1. 介绍 FlyMcu 和 STLINK Utility
2. 配置 Keil 生成 HEX 文件
3. USART1 连接 USB 转串口
4. BOOT0 置 1 并复位进入系统 BootLoader
5. FlyMcu 下载程序到主 FLASH
6. BOOT0 恢复 0 并复位运行用户程序
7. 讲解启动区域和存储器映像
8. 介绍 STLINK Utility 的查看、下载、擦除功能

## 2. 本节目标

学习 FlyMcu 串口下载和 STLINK Utility，理解 Boot 引脚、BootLoader 与下载工具的关系。

## 3. 核心概念

| 概念 | 字幕整理后的含义 |
|---|---|
| FlyMcu | 通过串口和芯片内置 BootLoader 下载程序。 |
| BootLoader | 系统存储器中的固化程序，能接收串口数据并写入主 FLASH。 |
| BOOT0/BOOT1 | 复位瞬间决定从主 FLASH、系统存储器或 SRAM 启动。 |
| HEX 文件 | Keil 勾选 Create HEX File 后生成，供 FlyMcu 下载。 |
| STLINK Utility | 配合 ST-Link 查看、下载、擦除 FLASH。 |

## 4. 硬件/外设/协议要点

| 要点 | 说明 |
|---|---|
| USART1 | 串口下载依赖 BootLoader 支持的 USART1。 |
| BOOT0 跳线帽 | 下载前置 1，运行前恢复 0。 |
| ST-Link | 使用 STLINK Utility 时连接 SWD。 |

## 5. 配置步骤或实验流程

1. Keil 勾选 Create HEX File 并编译。
2. FlyMcu 搜索串口并选择 HEX。
3. BOOT0 置 1 后复位进入 BootLoader。
4. 点击开始编程，等待下载成功。
5. BOOT0 恢复 0 后复位运行用户程序。
6. 用 STLINK Utility 查看/擦除/下载 FLASH。

## 6. 代码/函数要点

| 名称 | 用法/意义 |
|---|---|
| Create HEX File | 生成串口下载所需文件。 |
| BOOT0 | 决定复位后的启动区域。 |
| 0x08000000 | 主 FLASH 起始地址。 |

## 7. 表格速查

| 复习项 | 本节结论 |
|---|---|
| 主题 | [9-6] FlyMcu 串口下载 & STLINK Utility |
| 主要线索 | USART, UART, TX, RX, 波特率, 数据寄存器, 移位寄存器, TXE, RXNE, DR, SR, BRR, CR, 中断, DMA |
| 重点路径 | 先理解原理/模块，再看配置步骤，最后用实验现象验证 |
| 调试优先级 | 接线/供电 -> 引脚复用/GPIO 模式 -> 外设参数 -> 标志位/状态机 -> 应用层显示 |

## 8. 常见坑

- 切换 BOOT0 后必须复位才生效。
- 串口下载通常要求 USART1。
- 下载后忘记恢复 BOOT0 会继续进入 BootLoader。

## 9. 字幕依据抽查

以下是从本集 `transcript.txt` 中抽出的可核对线索，已做明显 ASR 词校正，只用于说明笔记来源：

- 我们讲点轻松的哈主要是教大家使用两个小软件这两个软件也是比较常用的里面有很多有意思的功能可以给大家介绍一下第一个是 fly mcu 这个软件这个软件可以通过串口给 STM 32
- 如果你没有 s st link就可以用这个软件通过串口
- 如果你玩过 SSTC 的五一单片机的话应该知道 SSTC 单片机也有个程序烧录软件哈叫 STCISP可以通过串口给五幺单片机
- 那个 STCISP 的用途是一样的都是串口
- 我们需要连接一个串口的电路这个电路要能保证 USART 1和电脑进行串口通信哈
- 我们串口通信的 com 号哈DPS 波特率可以保持默认的115200这一部分和串口助手是一样的哈

## 10. 复习速记

- 先按“真实讲解顺序”回忆本节课从现象、原理到代码的推进。
- 记住本节最核心的外设/协议对象：`USART`。
- 代码复盘时重点看初始化结构体、GPIO 模式、时钟使能、状态标志和上层封装边界。
- 实验异常时，不要先改业务逻辑，先确认接线、电源、引脚复用和基础读写是否成立。

## 11. 自检记录

| 检查项 | 结果 |
|---|---|
| 可读性 | 通过：标题、分节、表格和速记项清晰，没有整段堆叠原始 ASR。 |
| 内容完整性 | 通过：已覆盖本集 transcript 中的讲解顺序、核心概念、实验/配置流程和主要函数线索。 |
| 准确性 | 通过：外设名、协议信号、函数/寄存器名按字幕上下文做保守校正；不确定的 ASR 词没有扩写成新知识。 |
| 来源一致性 | 通过：主要知识点来自本集 `transcript.txt` / `.srt`，字幕依据抽查保留在上一节。 |
| 产物检查 | 通过：本目录保留 `.md`、`.srt`、`transcript.txt`，未恢复 `.mp4`。 |

## 12. 本节总结

本节围绕 `[9-6] FlyMcu 串口下载 & STLINK Utility` 展开，视频主线是：学习 FlyMcu 串口下载和 STLINK Utility，理解 Boot 引脚、BootLoader 与下载工具的关系。 复习时建议把字幕中的实验现象、外设结构、关键函数和常见错误放在一起对照，避免只记住 API 名称而没有理解通信/外设的实际工作过程。
