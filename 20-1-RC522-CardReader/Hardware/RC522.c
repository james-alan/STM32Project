#include "stm32f10x.h"
#include "MySPI.h"
#include "RC522.h"
#include "Delay.h"

/* ------------------------------------------------------------------
 * 片选和复位引脚
 *   PA2 -> RC522 SDA(NSS)，低电平选中
 *   PB0 -> RC522 RST，低电平复位
 * SPI 的 SCK/MOSI/MISO(PA5/PA7/PA6) 由 MySPI 初始化，和 W25Q64 共用一条总线
 * ------------------------------------------------------------------ */
#define RC522_CS_PORT       GPIOA
#define RC522_CS_PIN        GPIO_Pin_2
#define RC522_RST_PORT      GPIOB
#define RC522_RST_PIN       GPIO_Pin_0

static void RC522_CS(uint8_t value)
{
	GPIO_WriteBit(RC522_CS_PORT, RC522_CS_PIN, (BitAction)value);
}

static void RC522_RST(uint8_t value)
{
	GPIO_WriteBit(RC522_RST_PORT, RC522_RST_PIN, (BitAction)value);
}

/* ------------------------------------------------------------------
 * 底层寄存器读写
 * MFRC522 的 SPI 地址字节是：bit7 = 读写标志，bit6..1 = 寄存器地址
 * ------------------------------------------------------------------ */

static void RC522_WriteReg(uint8_t addr, uint8_t value)
{
	RC522_CS(0);
	MySPI_SwapByte((addr << 1) & 0x7E);
	MySPI_SwapByte(value);
	RC522_CS(1);
}

static uint8_t RC522_ReadReg(uint8_t addr)
{
	uint8_t value;

	RC522_CS(0);
	MySPI_SwapByte(((addr << 1) & 0x7E) | 0x80);
	value = MySPI_SwapByte(0x00);
	RC522_CS(1);

	return value;
}

static void RC522_SetBitMask(uint8_t addr, uint8_t mask)
{
	RC522_WriteReg(addr, RC522_ReadReg(addr) | mask);
}

static void RC522_ClearBitMask(uint8_t addr, uint8_t mask)
{
	RC522_WriteReg(addr, RC522_ReadReg(addr) & (uint8_t)(~mask));
}

void RC522_AntennaOn(void)
{
	uint8_t value = RC522_ReadReg(RC522_REG_TX_CONTROL);

	if ((value & 0x03) != 0x03)
	{
		RC522_SetBitMask(RC522_REG_TX_CONTROL, 0x03);
	}
}

void RC522_AntennaOff(void)
{
	RC522_ClearBitMask(RC522_REG_TX_CONTROL, 0x03);
}

void RC522_StopCrypto(void)
{
	RC522_ClearBitMask(RC522_REG_STATUS2, 0x08);
}

/* MfRxReg：bit4 是 ParityDisable。置 1 之后 RC522 就不再自动产生/校验 parity，
 * 整帧（数据位 + parity 位）由主机拼好塞进 FIFO 原样发出去。 */
#define RC522_REG_MF_RX 0x1D

void RC522_ParityDisable(void)
{
	RC522_WriteReg(RC522_REG_MF_RX, 0x10);
}

void RC522_ParityEnable(void)
{
	RC522_WriteReg(RC522_REG_MF_RX, 0x00);
}

/* 关一下天线再开，卡片回到 IDLE，可以重新开始一次认证 */
void RC522_ResetPICC(void)
{
	RC522_AntennaOff();
	Delay_ms(5);
	RC522_AntennaOn();
	Delay_ms(5);
}

uint8_t RC522_RawTransceive(uint8_t *data, uint8_t len, uint8_t lastBits,
                            uint8_t *out, uint8_t *outLen)
{
	uint8_t i;
	uint8_t irq;
	uint32_t timeout;

	RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_IDLE);
	/* MFRC522 的 ComIrqReg 标志只有在 ComIEnReg 对应位使能时才会置起来，
	 * 这行漏掉的话接收完成标志永远不出现，响应会被当成超时丢掉。 */
	RC522_WriteReg(RC522_REG_COM_IEN, 0x77);
	RC522_WriteReg(RC522_REG_COM_IRQ, 0x7F);
	RC522_SetBitMask(RC522_REG_FIFO_LEVEL, 0x80);

	for (i = 0; i < len; i++)
	{
		RC522_WriteReg(RC522_REG_FIFO_DATA, data[i]);
	}

	RC522_WriteReg(RC522_REG_BIT_FRAMING, lastBits);
	RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_TRANSCEIVE);
	RC522_SetBitMask(RC522_REG_BIT_FRAMING, 0x80);

	timeout = 20000;
	do
	{
		irq = RC522_ReadReg(RC522_REG_COM_IRQ);
		if ((irq & 0x30) != 0)
		{
			break;
		}
		if ((irq & 0x01) != 0)
		{
			*outLen = 0;
			return 0;
		}
	} while (--timeout != 0);

	RC522_ClearBitMask(RC522_REG_BIT_FRAMING, 0x80);

	*outLen = RC522_ReadReg(RC522_REG_FIFO_LEVEL);
	if (*outLen > 32)
	{
		*outLen = 32;
	}
	for (i = 0; i < *outLen; i++)
	{
		out[i] = RC522_ReadReg(RC522_REG_FIFO_DATA);
	}

	return (uint8_t)(*outLen > 0);
}

uint8_t RC522_ReadVersion(void)
{
	return RC522_ReadReg(RC522_REG_VERSION);
}

uint8_t RC522_GetReg(uint8_t addr)
{
	return RC522_ReadReg(addr);
}

/* ------------------------------------------------------------------
 * 用 MFRC522 内部的 CRC 协处理器算 CRC_A
 * result[0] 是先发出去的那一个字节，所以顺序是 L 在前
 * ------------------------------------------------------------------ */
static uint8_t RC522_CalcCRC(uint8_t *data, uint8_t length, uint8_t *result)
{
	uint16_t timeout = 0xFFFF;
	uint8_t i;

	RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_IDLE);
	RC522_WriteReg(RC522_REG_DIV_IRQ, 0x04);
	RC522_SetBitMask(RC522_REG_FIFO_LEVEL, 0x80);

	for (i = 0; i < length; i++)
	{
		RC522_WriteReg(RC522_REG_FIFO_DATA, data[i]);
	}

	RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_CALC_CRC);

	do
	{
		if (timeout-- == 0)
		{
			return RC522_ERR;
		}
	} while ((RC522_ReadReg(RC522_REG_DIV_IRQ) & 0x04) == 0);

	result[0] = RC522_ReadReg(RC522_REG_CRC_RESULT_L);
	result[1] = RC522_ReadReg(RC522_REG_CRC_RESULT_H);

	return RC522_OK;
}

/* ------------------------------------------------------------------
 * 软复位 + 上电初始化
 * 寄存器初值取自 MFRC522 数据手册的推荐配置
 * ------------------------------------------------------------------ */
void RC522_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* SPI1 的引脚和时钟由 MySPI_Init 负责 */
	MySPI_Init();

	/* 片选和复位脚：都是推挽输出，空闲时为高 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	RC522_CS(1);
	RC522_RST(1);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_InitStructure.GPIO_Pin = RC522_CS_PIN;
	GPIO_Init(RC522_CS_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = RC522_RST_PIN;
	GPIO_Init(RC522_RST_PORT, &GPIO_InitStructure);

	/* 硬件复位：RST 拉低一下再放开 */
	RC522_RST(0);
	Delay_ms(10);
	RC522_RST(1);
	Delay_ms(50);

	/* 软复位 */
	RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_SOFT_RESET);
	Delay_ms(50);

	/* 定时器：TAuto=1，预分频 0x0A9=169 → f_timer=40kHz（25us 一拍），
	 * 重装 1000 → 超时 25ms。
	 * 这个值必须给够：原始帧交互（认证来回）要几百微秒到几毫秒，
	 * 设成几十微秒的话卡片还没答就超时了。 */
	RC522_WriteReg(RC522_REG_T_MODE, 0x80);
	RC522_WriteReg(RC522_REG_T_PRESCALER, 0xA9);
	RC522_WriteReg(RC522_REG_T_RELOAD_H, 0x03);
	RC522_WriteReg(RC522_REG_T_RELOAD_L, 0xE8);
	RC522_WriteReg(RC522_REG_TX_ASK, 0x40);       /* 强制 100% ASK 调制 */
	RC522_WriteReg(RC522_REG_MODE, 0x3D);         /* CRC 初值 0x6363 */

	RC522_AntennaOn();
}

/* ------------------------------------------------------------------
 * 与卡片通信的公共函数
 * appendCrc 为 1 时先算 CRC_A 再补到数据末尾
 * backData / backBits 返回卡片应答，没有应答时传 0 即可
 * ------------------------------------------------------------------ */
static uint8_t RC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLength,
                            uint8_t *backData, uint16_t *backBits, uint8_t appendCrc)
{
	uint8_t irqEn = 0x77;
	uint8_t waitIRq = 0x30;
	uint8_t lastBits;
	uint8_t n;
	uint16_t i;
	uint8_t status = RC522_ERR;
	uint8_t crc[2];

	if (appendCrc)
	{
		if (RC522_CalcCRC(sendData, sendLength, crc) != RC522_OK)
		{
			return RC522_ERR;
		}
		sendData[sendLength] = crc[0];
		sendData[sendLength + 1] = crc[1];
		sendLength += 2;
	}

	RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_IDLE);
	RC522_WriteReg(RC522_REG_COM_IEN, irqEn | 0x80);
	RC522_ClearBitMask(RC522_REG_COM_IRQ, 0x80);
	RC522_SetBitMask(RC522_REG_FIFO_LEVEL, 0x80);   /* 清空 FIFO */

	for (i = 0; i < sendLength; i++)
	{
		RC522_WriteReg(RC522_REG_FIFO_DATA, sendData[i]);
	}

	RC522_WriteReg(RC522_REG_COMMAND, command);
	if (command == RC522_CMD_TRANSCEIVE)
	{
		RC522_SetBitMask(RC522_REG_BIT_FRAMING, 0x80);  /* StartSend，开始发送 */
	}

	i = 2000;
	do
	{
		n = RC522_ReadReg(RC522_REG_COM_IRQ);
		i--;
	} while ((i != 0) && ((n & 0x01) == 0) && ((n & waitIRq) == 0));

	RC522_ClearBitMask(RC522_REG_BIT_FRAMING, 0x80);

	if (i != 0)
	{
		if ((RC522_ReadReg(RC522_REG_ERROR) & 0x1B) == 0x00)
		{
			status = RC522_OK;

			/* 定时器超时说明没收到卡片的应答 */
			if ((n & irqEn & 0x01) != 0)
			{
				status = RC522_NOTAG;
			}

			if (backData != 0 && backBits != 0)
			{
				n = RC522_ReadReg(RC522_REG_FIFO_LEVEL);
				lastBits = RC522_ReadReg(RC522_REG_CONTROL) & 0x07;

				if (lastBits != 0)
				{
					*backBits = (uint16_t)((n - 1) * 8 + lastBits);
				}
				else
				{
					*backBits = (uint16_t)(n * 8);
				}

				if (n == 0)
				{
					n = 1;
				}
				if (n > 16)
				{
					n = 16;
				}

				for (i = 0; i < n; i++)
				{
					backData[i] = RC522_ReadReg(RC522_REG_FIFO_DATA);
				}
			}
		}
		else
		{
			status = RC522_ERR;
		}
	}

	return status;
}

/* 寻卡：发 REQA / WUPA，正常应答是 2 字节（16 位） */
uint8_t RC522_Request(uint8_t reqMode, uint8_t *atqa)
{
	uint8_t status;
	uint16_t backBits;
	uint8_t sendData[1];

	RC522_WriteReg(RC522_REG_BIT_FRAMING, 0x07);   /* 短帧，只有 7 位 */

	sendData[0] = reqMode;
	status = RC522_ToCard(RC522_CMD_TRANSCEIVE, sendData, 1, atqa, &backBits, 0);

	if ((status != RC522_OK) || (backBits != 0x10))
	{
		status = RC522_ERR;
	}

	return status;
}

/* 防冲突：拿 4 字节卡号 + 1 字节 BCC */
uint8_t RC522_Anticoll(uint8_t *uid)
{
	uint8_t status;
	uint16_t backBits;
	uint8_t sendData[2];

	RC522_WriteReg(RC522_REG_BIT_FRAMING, 0x00);   /* 关掉短帧，回到正常帧 */

	sendData[0] = PICC_ANTICOLL_CL1;
	sendData[1] = 0x20;

	status = RC522_ToCard(RC522_CMD_TRANSCEIVE, sendData, 2, uid, &backBits, 0);

	if ((status != RC522_OK) || (backBits != 0x28))
	{
		return RC522_ERR;
	}

	/* BCC = 卡号 4 个字节异或，对不上说明这帧是脏的 */
	if ((uint8_t)(uid[0] ^ uid[1] ^ uid[2] ^ uid[3]) != uid[4])
	{
		return RC522_ERR;
	}

	return RC522_OK;
}

/* 选卡：拿走 SAK 字节，同时卡片状态变为 ACTIVE */
uint8_t RC522_Select(uint8_t *uid, uint8_t *sak)
{
	uint8_t status;
	uint16_t backBits;
	uint8_t sendData[9];

	sendData[0] = PICC_ANTICOLL_CL1;
	sendData[1] = 0x70;
	sendData[2] = uid[0];
	sendData[3] = uid[1];
	sendData[4] = uid[2];
	sendData[5] = uid[3];
	sendData[6] = uid[4];
	sendData[7] = 0x00;   /* 占位，CRC 在 RC522_ToCard 里补 */
	sendData[8] = 0x00;

	status = RC522_ToCard(RC522_CMD_TRANSCEIVE, sendData, 7, sak, &backBits, 1);

	/* 应答是 1 字节 SAK + 2 字节 CRC_A，共 24 位 */
	if ((status != RC522_OK) || (backBits != 24))
	{
		return RC522_ERR;
	}

	return RC522_OK;
}

/* 让卡片休眠 */
void RC522_Halt(void)
{
	uint16_t backBits;
	uint8_t sendData[4];

	sendData[0] = PICC_HALT;
	sendData[1] = 0x00;
	sendData[2] = 0x00;
	sendData[3] = 0x00;

	RC522_ToCard(RC522_CMD_TRANSCEIVE, sendData, 2, 0, &backBits, 1);
	RC522_StopCrypto();
}

/* ------------------------------------------------------------------
 * MIFARE Classic 认证 + 读写块
 * 认证用 MFRC522 自带的 MFAuthent 命令，成功后 Status2 的 MFCrypto1On 会置位
 * ------------------------------------------------------------------ */
uint8_t RC522_AuthKey(uint8_t authMode, uint8_t blockAddr, uint8_t *key, uint8_t *uid)
{
	/* 认证成功只要几毫秒。这个循环用来等 MFCrypto1On 置位，
	 * 失败时必须尽快退出——65535 次循环在 562kHz 的 SPI 上要等两秒，
	 * 密钥穷举会被拖到几十分钟。1500 次约 100ms，足够。 */
	uint16_t timeout = 1500;
	uint8_t i;

	RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_IDLE);
	RC522_SetBitMask(RC522_REG_FIFO_LEVEL, 0x80);

	RC522_WriteReg(RC522_REG_FIFO_DATA, authMode);
	RC522_WriteReg(RC522_REG_FIFO_DATA, blockAddr);
	for (i = 0; i < RC522_KEY_LEN; i++)
	{
		RC522_WriteReg(RC522_REG_FIFO_DATA, key[i]);
	}
	for (i = 0; i < 4; i++)
	{
		RC522_WriteReg(RC522_REG_FIFO_DATA, uid[i]);
	}

	RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_MF_AUTHENT);

	do
	{
		if (timeout-- == 0)
		{
			return RC522_ERR;
		}
	} while ((RC522_ReadReg(RC522_REG_STATUS2) & 0x08) == 0);

	return RC522_OK;
}

/* 读一个块：应答是 16 字节数据 + 2 字节 CRC，共 144 位 */
uint8_t RC522_ReadBlock(uint8_t blockAddr, uint8_t *buf)
{
	uint8_t status;
	uint16_t backBits;
	uint8_t sendData[4];

	sendData[0] = PICC_READ;
	sendData[1] = blockAddr;

	status = RC522_ToCard(RC522_CMD_TRANSCEIVE, sendData, 2, buf, &backBits, 1);

	if ((status != RC522_OK) || (backBits != 0x90))
	{
		status = RC522_ERR;
	}

	return status;
}

/* 写一个块：先发命令+块号等 4 位 ACK，再发 16 字节数据再等 4 位 ACK */
uint8_t RC522_WriteBlock(uint8_t blockAddr, uint8_t *buf)
{
	uint8_t status;
	uint16_t backBits;
	uint8_t sendData[18];
	uint8_t resp[18];
	uint8_t i;

	sendData[0] = PICC_WRITE;
	sendData[1] = blockAddr;

	status = RC522_ToCard(RC522_CMD_TRANSCEIVE, sendData, 2, resp, &backBits, 1);
	if ((status != RC522_OK) || (backBits != 4) || ((resp[0] & 0x0F) != 0x0A))
	{
		return RC522_ERR;
	}

	for (i = 0; i < 16; i++)
	{
		sendData[i] = buf[i];
	}

	status = RC522_ToCard(RC522_CMD_TRANSCEIVE, sendData, 16, resp, &backBits, 1);
	if ((status != RC522_OK) || (backBits != 4) || ((resp[0] & 0x0F) != 0x0A))
	{
		return RC522_ERR;
	}

	return RC522_OK;
}
