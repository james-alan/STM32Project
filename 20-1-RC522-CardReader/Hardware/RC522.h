#ifndef __RC522_H
#define __RC522_H

#include "stm32f10x.h"

/* ------------------------------------------------------------------
 * MFRC522 寄存器地址
 * 这里写的是数据手册里的编号（0x01 ~ 0x37），
 * 真正的 SPI 地址字节在 RC522.c 内部左移一位生成。
 * ------------------------------------------------------------------ */
#define RC522_REG_COMMAND       0x01
#define RC522_REG_COM_IEN       0x02
#define RC522_REG_DIV_IEN       0x03
#define RC522_REG_COM_IRQ       0x04
#define RC522_REG_DIV_IRQ       0x05
#define RC522_REG_ERROR         0x06
#define RC522_REG_STATUS1       0x07
#define RC522_REG_STATUS2       0x08
#define RC522_REG_FIFO_DATA     0x09
#define RC522_REG_FIFO_LEVEL    0x0A
#define RC522_REG_CONTROL       0x0C
#define RC522_REG_BIT_FRAMING   0x0D
#define RC522_REG_COLL          0x0E
#define RC522_REG_MODE          0x11
#define RC522_REG_TX_CONTROL    0x14
#define RC522_REG_TX_ASK        0x15
#define RC522_REG_CRC_RESULT_H  0x21
#define RC522_REG_CRC_RESULT_L  0x22
#define RC522_REG_T_MODE        0x2A
#define RC522_REG_T_PRESCALER   0x2B
#define RC522_REG_T_RELOAD_H    0x2C
#define RC522_REG_T_RELOAD_L    0x2D
#define RC522_REG_VERSION       0x37

/* ---------------- PCD 命令（写给 CommandReg） ---------------- */
#define RC522_CMD_IDLE          0x00
#define RC522_CMD_CALC_CRC      0x03
#define RC522_CMD_TRANSCEIVE    0x0C
#define RC522_CMD_MF_AUTHENT    0x0E
#define RC522_CMD_SOFT_RESET    0x0F

/* ---------------- PICC 命令（发给卡片） ---------------- */
#define PICC_REQA               0x26
#define PICC_WUPA               0x52
#define PICC_ANTICOLL_CL1       0x93
#define PICC_HALT               0x50
#define PICC_AUTH_KEY_A         0x60
#define PICC_AUTH_KEY_B         0x61
#define PICC_READ               0x30
#define PICC_WRITE              0xA0

/* ---------------- 返回状态 ---------------- */
#define RC522_OK                0
#define RC522_NOTAG             1
#define RC522_ERR               2

/* 常用出厂默认密钥，穷举时优先试这一批 */
#define RC522_KEY_LEN           6

void RC522_Init(void);
uint8_t RC522_ReadVersion(void);

/* 读任意寄存器，主要给开机自检用 */
uint8_t RC522_GetReg(uint8_t addr);

void RC522_AntennaOn(void);
void RC522_AntennaOff(void);

/* 寻卡：reqMode 填 PICC_REQA（空闲卡）或 PICC_WUPA（休眠卡）
 * atqa 返回 2 字节应答，判断卡类型要用 */
uint8_t RC522_Request(uint8_t reqMode, uint8_t *atqa);

/* 防冲突，uid 返回 5 字节：uid[0..3] 是卡号，uid[4] 是校验字节 BCC */
uint8_t RC522_Anticoll(uint8_t *uid);

/* 选卡，sak 返回 SAK 字节 */
uint8_t RC522_Select(uint8_t *uid, uint8_t *sak);

/* 让卡片进入休眠，避免同一张卡被反复读到 */
void RC522_Halt(void);

/* 退出 MIFARE Crypto1 认证状态。
 * 认证成功之后 MFRC522 会停在 Crypto1 模式，换扇区或换卡之前必须清掉这个位，
 * 否则下一次寻卡会直接失败。 */
void RC522_StopCrypto(void);

/* ---- 原始帧收发：给 mfoc 式 nested 攻击用的底层能力 ----
 * 关掉 MFRC522 的 parity 自动处理之后，数据流里每个字节后面要自己跟 1 位 parity，
 * 这些由主机侧拼好再交给 RC522 原样发出去。 */
void RC522_ParityDisable(void);   /* MfRxReg bit4 = 1 */
void RC522_ParityEnable(void);    /* MfRxReg bit4 = 0 */

/* 关天线再开，把卡片复位回 IDLE，方便重新认证 */
void RC522_ResetPICC(void);

/* 原样发一串字节（已经含 parity 位），lastBits 是最后一个字节的有效位数
 * 返回 1 表示有应答，outLen 是收到的字节数（同样含 parity 位） */
uint8_t RC522_RawTransceive(uint8_t *data, uint8_t len, uint8_t lastBits,
                            uint8_t *out, uint8_t *outLen);

/* ---- 下面几个是读写数据块用的，复制卡片阶段才会用到 ---- */

/* 用 key 认证 blockAddr 这个块，authMode 填 PICC_AUTH_KEY_A 或 _B */
uint8_t RC522_AuthKey(uint8_t authMode, uint8_t blockAddr, uint8_t *key, uint8_t *uid);

/* 读一个块，buf 需要 16 字节空间 */
uint8_t RC522_ReadBlock(uint8_t blockAddr, uint8_t *buf);

/* 写一个块，buf 是 16 字节数据 */
uint8_t RC522_WriteBlock(uint8_t blockAddr, uint8_t *buf);

#endif
