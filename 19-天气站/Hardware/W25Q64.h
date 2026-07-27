#ifndef __W25Q64_H
#define __W25Q64_H

#include "stm32f10x.h"
#include <stdint.h>

/* ============= 引脚分配 =============
 * W25Q64 (SPI1) 接线:
 *   CS/NSS  -> PA4  (软件控制)
 *   SCK     -> PA5  (SPI1 默认)
 *   MISO    -> PA6  (SPI1 默认)
 *   MOSI    -> PA7  (SPI1 默认)
 * VCC -> 3.3V   GND -> GND
 * ==================================== */

/* W25Q64 基本指令集（参考 W25Q64BV 数据手册） */
#define W25Q64_CMD_WRITE_ENABLE       0x06
#define W25Q64_CMD_WRITE_DISABLE      0x04
#define W25Q64_CMD_READ_STATUS_REG1   0x05
#define W25Q64_CMD_READ_STATUS_REG2   0x35
#define W25Q64_CMD_PAGE_PROGRAM       0x02
#define W25Q64_CMD_SECTOR_ERASE       0x20    /* 4KB 扇区擦除 */
#define W25Q64_CMD_BLOCK_ERASE_32K    0x52
#define W25Q64_CMD_BLOCK_ERASE_64K    0xD8
#define W25Q64_CMD_CHIP_ERASE         0xC7
#define W25Q64_CMD_READ_DATA          0x03
#define W25Q64_CMD_READ_JEDEC_ID      0x9F

#define W25Q64_PAGE_SIZE      256         /* 一页 256 字节 */
#define W25Q64_SECTOR_SIZE    4096        /* 一个扇区 4KB */
#define W25Q64_CAPACITY       8 * 1024 * 1024
#define W25Q64_SECTOR_COUNT   2048        /* 8MB / 4KB */
#define W25Q64_JEDEC_ID       0xEF4017    /* W25Q64 固定 JEDEC ID */
#define W25Q64_TIMEOUT        0xFFFFFF

/* CS 控制 */
#define W25Q64_CS_LOW()   GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define W25Q64_CS_HIGH()  GPIO_SetBits(GPIOA, GPIO_Pin_4)

void  W25Q64_Init(void);

/* 低层 SPI 收发 */
uint8_t  W25Q64_ReadWriteByte(uint8_t TxData);

/* 中层指令函数 */
uint16_t W25Q64_ReadID(void);                       /* 读取 JEDEC ID */
uint8_t  W25Q64_ReadSR(void);                       /* 读状态寄存器1 */
void     W25Q64_WriteEnable(void);                  /* 写使能 */
void     W25Q64_WaitForWriteEnd(void);              /* 等待写入/擦除完成 */

/* 擦除 */
void     W25Q64_SectorErase(uint32_t addr);         /* 擦除 4KB 扇区（addr 必须 4KB 对齐） */
void     W25Q64_ChipErase(void);                    /* 全片擦除（耗时长，约 40s） */

/* 读写（任意长度、任意地址） */
void     W25Q64_ReadData(uint32_t addr, uint8_t *buf, uint16_t len);
void     W25Q64_PageProgram(uint32_t addr, uint8_t *buf, uint16_t len);
void     W25Q64_WriteNoCheck(uint32_t addr, uint8_t *buf, uint16_t len);  /* 内部用：跨页写 */
void     W25Q64_Write(uint32_t addr, uint8_t *buf, uint16_t len);          /* 带擦除的安全写入 */

#endif
