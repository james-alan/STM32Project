#include "stm32f10x.h"
#include "Delay.h"
#include "Serial.h"
#include "OLED.h"
#include "RC522.h"
#include "W25Q64.h"

/* ------------------------------------------------------------------
 * STM32F103C8T6 + RC522 门禁卡读写器
 *
 * 接线：
 *   RC522 SDA(NSS) -> PA2      RC522 RST  -> PB0
 *   RC522 SCK      -> PA5      RC522 MOSI -> PA7
 *   RC522 MISO     -> PA6      RC522 IRQ  -> 不接
 *   RC522 VCC/GND  -> 3.3V/GND
 *
 *   W25Q64 和 RC522 共用 SPI1（PA5/PA7/PA6），片选分开：
 *   W25Q64 CS -> PA4，RC522 CS -> PA2
 *
 * 串口：USART1，PA9=TX / PA10=RX，115200-8-N-1
 * OLED：软件 I2C，SCL=PB8 / SDA=PB9
 *
 * 串口命令（收到一个字符就执行）：
 *   R  读当前天线上的卡，并把它记成「源卡」
 *   W  把源卡的卡号写到当前天线上的卡（第 0 块，给 CUID 白卡用）
 *   D  用常用默认密钥把整张卡 16 个扇区都读出来，存进内存
 *   F  把内存里的整卡数据写回当前天线上的卡
 *   ?  打印这份帮助
 * ------------------------------------------------------------------ */

#define BLOCK_COUNT   64
#define SECTOR_COUNT  16
#define KEY_COUNT     48

/* 常见出厂密钥和门禁系统常用密钥。
 * 前几个是 NXP 出厂默认值，后面是各家读卡器/梯控厂商常见的写法。
 * 穷举时按这个顺序试，命中越早越快。 */
static const uint8_t DefaultKeys[KEY_COUNT][6] =
{
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	{0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5},
	{0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5},
	{0x4D, 0x3A, 0x99, 0xC3, 0x51, 0xDD},
	{0x1A, 0x98, 0x2C, 0x7E, 0x45, 0x9A},
	{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
	{0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0},
	{0xA1, 0xB1, 0xC1, 0xD1, 0xE1, 0xF1},
	{0x11, 0x11, 0x11, 0x11, 0x11, 0x11},
	{0x22, 0x22, 0x22, 0x22, 0x22, 0x22},
	{0x33, 0x33, 0x33, 0x33, 0x33, 0x33},
	{0x44, 0x44, 0x44, 0x44, 0x44, 0x44},
	{0x55, 0x55, 0x55, 0x55, 0x55, 0x55},
	{0x66, 0x66, 0x66, 0x66, 0x66, 0x66},
	{0x77, 0x77, 0x77, 0x77, 0x77, 0x77},
	{0x88, 0x88, 0x88, 0x88, 0x88, 0x88},
	{0x99, 0x99, 0x99, 0x99, 0x99, 0x99},
	{0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA},
	{0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB},
	{0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC},
	{0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD},
	{0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE},
	{0x12, 0x34, 0x56, 0x78, 0x90, 0xAB},
	{0x01, 0x23, 0x45, 0x67, 0x89, 0xAB},
	{0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56},
	{0x11, 0x22, 0x33, 0x44, 0x55, 0x66},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0xFF},
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE},
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00},
	{0x48, 0x45, 0x58, 0x41, 0x43, 0x54},   /* "HEXACT" */
	{0x73, 0x64, 0x75, 0x74, 0x69, 0x6C},   /* "sdutil" */
	{0x8F, 0xD0, 0xA4, 0xF2, 0x56, 0xE9},
	{0xD0, 0x1A, 0xFE, 0xEB, 0x89, 0x0A},
	{0x75, 0xCC, 0xB5, 0x9C, 0x9B, 0xED},
	{0x5C, 0x8F, 0xF9, 0x99, 0x0D, 0xA2},
	{0x71, 0x4C, 0x5C, 0x88, 0x6E, 0x97},
	{0x58, 0x7E, 0xE5, 0xF9, 0x35, 0x0A},
	{0xA0, 0x47, 0x8C, 0xC3, 0x90, 0x91},
	{0x53, 0x3C, 0xB6, 0xC7, 0x23, 0xF6},
	{0x00, 0x00, 0x01, 0x4B, 0x5C, 0x31},
	{0x60, 0x94, 0x29, 0xAC, 0x9C, 0x47},
	{0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6},
	{0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6},
	{0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA6},
	{0x00, 0xFF, 0xA5, 0x71, 0x5A, 0x1B},
};

static uint8_t SourceUid[5];
static uint8_t SourceAtqa[2];
static uint8_t SourceSak;
static uint8_t SourceValid;
static uint8_t SourceBlock0[16];   /* 原卡第 0 块完整 16 字节，连厂商数据一起抄 */
static uint8_t SourceBlock0Valid;

static uint8_t Dump[BLOCK_COUNT][16];
static uint8_t DumpKey[SECTOR_COUNT][6];
static uint8_t DumpKeyType[SECTOR_COUNT];
static uint8_t DumpDone[SECTOR_COUNT];

/* 寻卡 -> 防冲突 -> 选卡。
 * 用 WUPA 而不是 REQA：卡片被 Halt 过之后，只有 WUPA 能把它重新叫醒。 */
static uint8_t CardPresent(uint8_t *uid, uint8_t *sak, uint8_t *atqa)
{
	RC522_StopCrypto();

	if (RC522_Request(PICC_WUPA, atqa) != RC522_OK)
	{
		return 0;
	}
	if (RC522_Anticoll(uid) != RC522_OK)
	{
		return 0;
	}
	if (RC522_Select(uid, sak) != RC522_OK)
	{
		return 0;
	}

	return 1;
}

/* 带重试的版本，命令里用 */
static uint8_t ActivateCard(uint8_t *uid, uint8_t *sak, uint8_t *atqa)
{
	uint8_t attempt;

	for (attempt = 0; attempt < 5; attempt++)
	{
		if (CardPresent(uid, sak, atqa))
		{
			return 1;
		}
		Delay_ms(30);
	}

	return 0;
}

static void PrintHex(uint8_t *data, uint8_t length)
{
	uint8_t i;

	for (i = 0; i < length; i++)
	{
		Serial_printf("%02X ", data[i]);
	}
	Serial_printf("\r\n");
}

/* ------------------------------------------------------------------
 * OLED 状态显示
 * 屏幕 4 行、每行 16 个字符，所以每行都补齐到 16 个字符再写，
 * 不然上一次的残留字符会留在后面。
 * ------------------------------------------------------------------ */
static void UiText(uint8_t line, char *text)
{
	char buf[17];
	uint8_t i;
	uint8_t ended = 0;

	for (i = 0; i < 16; i++)
	{
		if (!ended && text[i] == '\0')
		{
			ended = 1;
		}
		buf[i] = ended ? ' ' : text[i];
	}
	buf[16] = '\0';

	OLED_ShowString(line, 1, buf);
}

static void UiUidLine(char *prefix, uint8_t *uid)
{
	char buf[17];

	sprintf(buf, "%s%02X%02X%02X%02X", prefix, uid[0], uid[1], uid[2], uid[3]);
	UiText(2, buf);
}

/* 第 4 行画进度条：12 格，满格 12 个 # */
static void UiBar(uint8_t done, uint8_t total)
{
	char buf[17];
	uint8_t filled;
	uint8_t i;

	if (total == 0)
	{
		total = 1;
	}
	filled = (uint8_t)(((uint16_t)done * 12) / total);
	if (filled > 12)
	{
		filled = 12;
	}

	buf[0] = '[';
	for (i = 0; i < 12; i++)
	{
		buf[1 + i] = (i < filled) ? '#' : '-';
	}
	buf[13] = ']';
	buf[14] = '\0';

	UiText(4, buf);
}

/* 没有卡：等待放卡 */
static void UiWaiting(void)
{
	UiText(1, "RC522 Card Tool");
	UiText(2, "Waiting card...");
	UiText(3, " ");
	UiText(4, " ");
}

/* 卡放到天线上了 */
static void UiCardOnAntenna(uint8_t *uid)
{
	UiText(1, "Card on antenna");
	UiUidLine("UID:", uid);
	UiText(3, "ready");
	UiText(4, " ");
}

/* 读 / 写进行中，带进度条 */
static void UiBusy(char *action, uint8_t *uid, uint8_t done, uint8_t total)
{
	char buf[17];

	UiText(1, action);
	UiUidLine("UID:", uid);
	sprintf(buf, "Sector %d/%d", done, total);
	UiText(3, buf);
	UiBar(done, total);
}

static void UiResult(char *line1, char *line2)
{
	UiText(1, line1);
	UiText(2, line2);
	UiText(3, " ");
	UiText(4, " ");
}

static const char *CardTypeName(uint8_t *atqa, uint8_t sak)
{
	uint16_t value = (uint16_t)(atqa[0] | ((uint16_t)atqa[1] << 8));

	if (value == 0x0004 && sak == 0x08)
	{
		return "MIFARE Classic 1K (S50)";
	}
	if (value == 0x0002 && sak == 0x18)
	{
		return "MIFARE Classic 4K (S70)";
	}
	if (value == 0x0004 && sak == 0x09)
	{
		return "MIFARE Mini";
	}
	if (value == 0x0044 && sak == 0x00)
	{
		return "MIFARE Ultralight / NTAG (UID only)";
	}
	if (sak == 0x20)
	{
		return "ISO14443-4 / DESFire / Plus";
	}
	return "Unknown";
}

/* ------------------------------------------------------------------
 * W25Q64 Flash 存储：把源卡信息和整卡数据存起来，断电不丢
 *
 * 地址布局（一个 4KB 扇区就够）：
 *   0x000000  256 字节文件头
 *   0x000100  1024 字节卡片数据
 * 驱动一次最多写 255 字节、且不能跨 256 字节页，
 * 所以统一按 128 字节对齐分块，正好不会跨页。
 * ------------------------------------------------------------------ */
#define FLASH_MAGIC_0 'R'
#define FLASH_MAGIC_1 'C'
#define FLASH_MAGIC_2 '5'
#define FLASH_MAGIC_3 '2'

static uint8_t FlashBuf[256];
static uint8_t FlashReady;

static void FlashHeaderPack(void)
{
	uint8_t i;
	uint8_t j;
	uint16_t n;

	/* 注意：循环变量必须用 uint16_t。用 uint8_t 的话 i 永远到不了 256，会死循环 */
	for (n = 0; n < 256; n++)
	{
		FlashBuf[n] = 0;
	}

	FlashBuf[0] = FLASH_MAGIC_0;
	FlashBuf[1] = FLASH_MAGIC_1;
	FlashBuf[2] = FLASH_MAGIC_2;
	FlashBuf[3] = FLASH_MAGIC_3;
	FlashBuf[4] = SourceValid;
	for (i = 0; i < 5; i++)
	{
		FlashBuf[5 + i] = SourceUid[i];
	}
	FlashBuf[10] = SourceAtqa[0];
	FlashBuf[11] = SourceAtqa[1];
	FlashBuf[12] = SourceSak;
	for (i = 0; i < 16; i++)
	{
		FlashBuf[13 + i] = SourceBlock0[i];
	}
	FlashBuf[29] = SourceBlock0Valid;
	for (i = 0; i < SECTOR_COUNT; i++)
	{
		FlashBuf[30 + i] = DumpDone[i];
	}
	for (i = 0; i < SECTOR_COUNT; i++)
	{
		for (j = 0; j < 6; j++)
		{
			FlashBuf[46 + i * 6 + j] = DumpKey[i][j];
		}
	}
	for (i = 0; i < SECTOR_COUNT; i++)
	{
		FlashBuf[142 + i] = DumpKeyType[i];
	}
}

static uint8_t FlashHeaderUnpack(void)
{
	uint8_t i;
	uint8_t j;

	if (FlashBuf[0] != FLASH_MAGIC_0 || FlashBuf[1] != FLASH_MAGIC_1 ||
	    FlashBuf[2] != FLASH_MAGIC_2 || FlashBuf[3] != FLASH_MAGIC_3)
	{
		return 0;
	}

	SourceValid = FlashBuf[4];
	for (i = 0; i < 5; i++)
	{
		SourceUid[i] = FlashBuf[5 + i];
	}
	SourceAtqa[0] = FlashBuf[10];
	SourceAtqa[1] = FlashBuf[11];
	SourceSak = FlashBuf[12];
	for (i = 0; i < 16; i++)
	{
		SourceBlock0[i] = FlashBuf[13 + i];
	}
	SourceBlock0Valid = FlashBuf[29];
	for (i = 0; i < SECTOR_COUNT; i++)
	{
		DumpDone[i] = FlashBuf[30 + i];
	}
	for (i = 0; i < SECTOR_COUNT; i++)
	{
		for (j = 0; j < 6; j++)
		{
			DumpKey[i][j] = FlashBuf[46 + i * 6 + j];
		}
	}
	for (i = 0; i < SECTOR_COUNT; i++)
	{
		DumpKeyType[i] = FlashBuf[142 + i];
	}

	return 1;
}

static uint8_t FlashSave(void)
{
	uint8_t i;
	uint32_t addr;

	if (!FlashReady)
	{
		return 0;
	}

	FlashHeaderPack();
	W25Q64_SectorErase(0x000000);

	addr = 0x000000;
	for (i = 0; i < 2; i++)
	{
		W25Q64_PageProgram(addr, &FlashBuf[i * 128], 128);
		addr += 128;
	}

	addr = 0x000100;
	for (i = 0; i < 8; i++)
	{
		W25Q64_PageProgram(addr, ((uint8_t *)Dump) + i * 128, 128);
		addr += 128;
	}

	return 1;
}

static uint8_t FlashLoad(void)
{
	uint8_t i;
	uint32_t addr;

	if (!FlashReady)
	{
		return 0;
	}

	addr = 0x000000;
	for (i = 0; i < 2; i++)
	{
		W25Q64_ReadData(addr, &FlashBuf[i * 128], 128);
		addr += 128;
	}

	if (!FlashHeaderUnpack())
	{
		return 0;
	}

	addr = 0x000100;
	for (i = 0; i < 8; i++)
	{
		W25Q64_ReadData(addr, ((uint8_t *)Dump) + i * 128, 128);
		addr += 128;
	}

	return 1;
}

/* ---------------- 命令 R：读卡 ---------------- */
static void CmdRead(void)
{
	uint8_t uid[5];
	uint8_t sak;
	uint8_t atqa[2];
	uint8_t i;

	if (!ActivateCard(uid, &sak, atqa))
	{
		Serial_printf("R: no card on the antenna\r\n");
		return;
	}

	for (i = 0; i < 5; i++)
	{
		SourceUid[i] = uid[i];
	}
	SourceAtqa[0] = atqa[0];
	SourceAtqa[1] = atqa[1];
	SourceSak = sak;
	SourceValid = 1;

	Serial_printf("R: card found\r\n");
	Serial_printf("   UID : ");
	PrintHex(uid, 4);
	Serial_printf("   BCC : %02X\r\n", uid[4]);
	Serial_printf("   ATQA: 0x%02X%02X  SAK: 0x%02X\r\n", atqa[1], atqa[0], sak);
	Serial_printf("   Type: %s\r\n", CardTypeName(atqa, sak));
	Serial_printf("   saved as source card\r\n");

	/* 顺手把第 0 块整块读出来：写白卡时要把厂商数据也一起复制，
	 * 有些门禁会核对这几个字节，清零了就露馅。 */
	SourceBlock0Valid = 0;
	for (i = 0; i < KEY_COUNT; i++)
	{
		if (!ActivateCard(uid, &sak, atqa))
		{
			break;
		}
		if (RC522_AuthKey(PICC_AUTH_KEY_A, 0, (uint8_t *)DefaultKeys[i], uid) == RC522_OK)
		{
			if (RC522_ReadBlock(0, SourceBlock0) == RC522_OK)
			{
				SourceBlock0Valid = 1;
			}
			RC522_Halt();
			break;
		}
		RC522_Halt();
	}

	if (SourceBlock0Valid)
	{
		Serial_printf("   block0: ");
		PrintHex(SourceBlock0, 16);
	}
	else
	{
		Serial_printf("   block0: not readable with default keys\r\n");
	}

	UiCardOnAntenna(uid);
	UiText(3, "saved as source");

	RC522_Halt();
}

/* ---------------- 命令 W：写卡号到第 0 块 ---------------- */
static void CmdWriteUid(void)
{
	uint8_t uid[5];
	uint8_t sak;
	uint8_t atqa[2];
	uint8_t block0[16];
	uint8_t key;
	uint8_t ok = 0;

	if (!SourceValid)
	{
		Serial_printf("W: no source card yet, press R on the original card first\r\n");
		UiResult("No source card", "press R first");
		return;
	}

	UiText(1, "Writing UID...");
	UiUidLine("UID:", SourceUid);
	UiText(3, "block 0");
	UiBar(0, 1);

	if (SourceBlock0Valid)
	{
		/* 原卡第 0 块整块照抄，厂商数据也一起过来 */
		for (key = 0; key < 16; key++)
		{
			block0[key] = SourceBlock0[key];
		}
	}
	else
	{
		/* 没读到原块，只能自己拼：UID(4) + BCC(1) + SAK(1) + ATQA(2) + 厂商数据(8) */
		block0[0] = SourceUid[0];
		block0[1] = SourceUid[1];
		block0[2] = SourceUid[2];
		block0[3] = SourceUid[3];
		block0[4] = SourceUid[4];
		block0[5] = SourceSak;
		block0[6] = SourceAtqa[0];
		block0[7] = SourceAtqa[1];
		for (key = 8; key < 16; key++)
		{
			block0[key] = 0x00;
		}
	}

	for (key = 0; key < KEY_COUNT; key++)
	{
		if (!ActivateCard(uid, &sak, atqa))
		{
			Serial_printf("W: no card on the antenna\r\n");
			UiResult("No card", "put blank tag on");
			return;
		}

		UiBar((uint8_t)(key + 1), KEY_COUNT);

		if (RC522_AuthKey(PICC_AUTH_KEY_A, 0, (uint8_t *)DefaultKeys[key], uid) == RC522_OK)
		{
			if (RC522_WriteBlock(0, block0) == RC522_OK)
			{
				ok = 1;
				Serial_printf("W: block 0 written with key ");
				PrintHex((uint8_t *)DefaultKeys[key], 6);
				break;
			}
			Serial_printf("W: auth ok but write failed\r\n");
		}

		RC522_Halt();
		Delay_ms(20);
	}

	if (!ok)
	{
		Serial_printf("W: failed, no default key worked on block 0\r\n");
		UiResult("Write FAILED", "no default key");
		return;
	}

	/* 写完卡号，卡片要重新激活才能看到新 UID */
	RC522_Halt();
	Delay_ms(80);

	if (!ActivateCard(uid, &sak, atqa))
	{
		Serial_printf("W: written, but card did not come back for verify\r\n");
		UiResult("Written", "verify failed");
		return;
	}

	Serial_printf("W: verify -> UID ");
	PrintHex(uid, 4);
	Serial_printf("   target UID was ");
	PrintHex(SourceUid, 4);

	if (uid[0] == SourceUid[0] && uid[1] == SourceUid[1] &&
	    uid[2] == SourceUid[2] && uid[3] == SourceUid[3])
	{
		Serial_printf("   RESULT: OK, UID matches\r\n");
		UiResult("Write OK", "UID matches");
	}
	else
	{
		Serial_printf("   RESULT: MISMATCH\r\n");
		UiResult("Write BAD", "UID mismatch");
	}

	RC522_Halt();
}

/* ---------------- 命令 D：穷举密钥整卡读出 ---------------- */
static void CmdDump(void)
{
	uint8_t uid[5];
	uint8_t sak;
	uint8_t atqa[2];
	uint8_t sector;
	uint8_t block;
	uint8_t keyIndex;
	uint8_t typeIndex;
	uint8_t found;
	uint8_t authMode;
	uint8_t dispUid[5];
	uint8_t readOk = 0;
	uint8_t missCount = 0;

	UiText(1, "Reading card...");
	UiText(2, " ");
	UiText(3, "looking for key");
	UiBar(0, SECTOR_COUNT);

	if (!ActivateCard(uid, &sak, atqa))
	{
		Serial_printf("D: no card on the antenna\r\n");
		UiResult("No card", "put card on");
		return;
	}
	for (block = 0; block < 5; block++)
	{
		dispUid[block] = uid[block];
	}
	RC522_Halt();

	for (sector = 0; sector < SECTOR_COUNT; sector++)
	{
		DumpDone[sector] = 0;
	}

	Serial_printf("D: dumping, keep the card still...\r\n");

	for (sector = 0; sector < SECTOR_COUNT; sector++)
	{
		found = 0;

		for (keyIndex = 0; keyIndex < KEY_COUNT && !found; keyIndex++)
		{
			for (typeIndex = 0; typeIndex < 2 && !found; typeIndex++)
			{
				authMode = (typeIndex == 0) ? PICC_AUTH_KEY_A : PICC_AUTH_KEY_B;

				/* 穷举时用轻量激活：失败就重试一次，再不行跳过这个密钥。
				 * 这里绝不能调 ActivateCard——它失败一次要等 150ms，
				 * 48 个密钥 × 2 种认证会让每个扇区慢到几分钟。 */
				if (!CardPresent(uid, &sak, atqa))
				{
					Delay_ms(5);
					if (!CardPresent(uid, &sak, atqa))
					{
						missCount++;
						if (missCount > 40)
						{
							Serial_printf("D: card left the field at sector %d\r\n", sector);
							UiResult("Card removed", "read stopped");
							return;
						}
						continue;
					}
				}
				missCount = 0;

				if (RC522_AuthKey(authMode, (uint8_t)(sector * 4),
				                  (uint8_t *)DefaultKeys[keyIndex], uid) != RC522_OK)
				{
					RC522_StopCrypto();
					continue;
				}

				found = 1;
				DumpKeyType[sector] = authMode;
				for (block = 0; block < 6; block++)
				{
					DumpKey[sector][block] = DefaultKeys[keyIndex][block];
				}

				for (block = 0; block < 4; block++)
				{
					if (RC522_ReadBlock((uint8_t)(sector * 4 + block),
					                    Dump[sector * 4 + block]) != RC522_OK)
					{
						found = 0;
						break;
					}
				}

				if (found)
				{
					DumpDone[sector] = 1;
					Serial_printf("sector %2d ok  key%c = ", sector,
					              (authMode == PICC_AUTH_KEY_A) ? 'A' : 'B');
					PrintHex(DumpKey[sector], 6);
				}
				RC522_Halt();
			}
		}

		if (!DumpDone[sector])
		{
			Serial_printf("sector %2d FAILED (no default key)\r\n", sector);
		}

		if (DumpDone[sector])
		{
			readOk++;
		}
		UiBusy("Reading card...", dispUid, (uint8_t)(sector + 1), SECTOR_COUNT);
	}

	Serial_printf("D: ---- dump ----\r\n");
	for (block = 0; block < BLOCK_COUNT; block++)
	{
		uint8_t owner = (uint8_t)(block / 4);

		Serial_printf("%02d %02d: ", owner, block);
		if (DumpDone[owner])
		{
			PrintHex(Dump[block], 16);
		}
		else
		{
			Serial_printf("-- not read --\r\n");
		}
	}
	Serial_printf("D: done\r\n");
	RC522_Halt();

	if (readOk == SECTOR_COUNT)
	{
		UiResult("Read OK", "all 16 sectors");
	}
	else
	{
		char msg[17];

		sprintf(msg, "%d of 16 sectors", readOk);
		UiResult("Read partial", msg);
	}
}

/* ---------------- 命令 F：把整卡数据写回去 ---------------- */
static void CmdWriteDump(void)
{
	uint8_t uid[5];
	uint8_t sak;
	uint8_t atqa[2];
	uint8_t sector;
	uint8_t block;
	uint8_t keyIndex;
	uint8_t done = 0;
	uint8_t first;
	uint8_t okAll;
	uint8_t trailer[16];
	uint8_t i;
	uint8_t targetBlock0[16];
	uint8_t targetBlock0Valid;
	uint8_t sameTarget;

	UiText(1, "Writing card...");
	UiUidLine("UID:", Dump[0]);
	UiText(3, "block 0 first");
	UiBar(0, SECTOR_COUNT);

	/* ---------------- 写入前的保险 ----------------
	 * 先看清天线上是哪张卡。如果它的第 0 块和源卡一模一样，那基本就是把原卡
	 * 又放回来了，这时候拒绝写入，避免把原卡的数据块再动一遍。 */
	if (!SourceBlock0Valid)
	{
		Serial_printf("F: source block0 not saved, run R on the original card first\r\n");
		UiResult("Need R first", "read source card");
		return;
	}

	targetBlock0Valid = 0;
	for (keyIndex = 0; keyIndex < KEY_COUNT; keyIndex++)
	{
		if (!ActivateCard(uid, &sak, atqa))
		{
			break;
		}
		if (RC522_AuthKey(PICC_AUTH_KEY_A, 0, (uint8_t *)DefaultKeys[keyIndex], uid) == RC522_OK)
		{
			if (RC522_ReadBlock(0, targetBlock0) == RC522_OK)
			{
				targetBlock0Valid = 1;
			}
			RC522_Halt();
			break;
		}
		RC522_Halt();
	}

	if (!targetBlock0Valid)
	{
		Serial_printf("F: cannot read target block0 (no card, or unknown key)\r\n");
		UiResult("No target card", "or unknown key");
		return;
	}

	Serial_printf("F: target block0 = ");
	PrintHex(targetBlock0, 16);

	sameTarget = 1;
	for (i = 0; i < 16; i++)
	{
		if (targetBlock0[i] != SourceBlock0[i])
		{
			sameTarget = 0;
		}
	}

	if (sameTarget)
	{
		Serial_printf("F: REFUSED - target block0 is identical to the source card\r\n");
		Serial_printf("   (this is probably the ORIGINAL card, not a blank tag)\r\n");
		UiResult("REFUSED", "looks like orig");
		return;
	}

	Serial_printf("F: target is a different card, writing...\r\n");

	for (sector = 0; sector < SECTOR_COUNT; sector++)
	{
		if (!DumpDone[sector])
		{
			UiBusy("Writing card...", Dump[0], (uint8_t)(sector + 1), SECTOR_COUNT);
			continue;
		}

		first = (uint8_t)(sector * 4);

		for (keyIndex = 0; keyIndex < KEY_COUNT; keyIndex++)
		{
			okAll = 1;

			if (!ActivateCard(uid, &sak, atqa))
			{
				Serial_printf("F: no card on the antenna\r\n");
				UiResult("No card", "put blank card on");
				return;
			}

			if (RC522_AuthKey(PICC_AUTH_KEY_A, first,
			                  (uint8_t *)DefaultKeys[keyIndex], uid) != RC522_OK)
			{
				RC522_Halt();
				continue;
			}

			for (block = 0; block < 4; block++)
			{
				uint8_t *payload = Dump[first + block];

				/* 扇区尾块（第 3 块）：密钥 A 读出来永远是 00，
				 * 原样写回去等于把卡锁死，所以要用验证过能用的密钥填回去。 */
				if (block == 3)
				{
					for (i = 0; i < 16; i++)
					{
						trailer[i] = Dump[first + 3][i];
					}

					if (DumpKeyType[sector] == PICC_AUTH_KEY_A)
					{
						for (i = 0; i < 6; i++)
						{
							trailer[i] = DumpKey[sector][i];
							trailer[10 + i] = 0xFF;
						}
					}
					else
					{
						for (i = 0; i < 6; i++)
						{
							trailer[i] = 0xFF;
							trailer[10 + i] = DumpKey[sector][i];
						}
					}
					payload = trailer;
				}

				if (RC522_WriteBlock((uint8_t)(first + block), payload) != RC522_OK)
				{
					okAll = 0;
					break;
				}
			}

			RC522_Halt();
			if (okAll)
			{
				done++;
				Serial_printf("F: sector %2d written\r\n", sector);
				break;
			}
		}

		UiBusy("Writing card...", Dump[0], (uint8_t)(sector + 1), SECTOR_COUNT);
	}

	Serial_printf("F: %d sectors written\r\n", done);

	if (done == SECTOR_COUNT)
	{
		UiResult("Write OK", "all 16 sectors");
	}
	else
	{
		char msg[17];

		sprintf(msg, "%d of 16 sectors", done);
		UiResult("Write partial", msg);
	}
}

/* ---------------- 命令 P：打印内存里已有的数据 ---------------- */
static void CmdPrintDump(void)
{
	uint8_t sector;
	uint8_t block;
	uint8_t have = 0;

	for (sector = 0; sector < SECTOR_COUNT; sector++)
	{
		if (DumpDone[sector])
		{
			have++;
		}
	}

	Serial_printf("P: source block0 = ");
	PrintHex(SourceBlock0, 16);
	Serial_printf("P: %d of %d sectors in memory\r\n", have, SECTOR_COUNT);

	for (block = 0; block < BLOCK_COUNT; block++)
	{
		uint8_t owner = (uint8_t)(block / 4);

		Serial_printf("%02d %02d: ", owner, block);
		if (DumpDone[owner])
		{
			PrintHex(Dump[block], 16);
		}
		else
		{
			Serial_printf("-- not read --\r\n");
		}
	}
}

/* ------------------------------------------------------------------
 * 原始帧命令：给 Mac 上的 nested 攻击逻辑用
 * 命令按行收（以 \r 或 \n 结束），这样带上参数也不会冲突
 * ------------------------------------------------------------------ */
static char CmdLine[240];
static uint8_t CmdLen;
/* 空闲探卡会和攻击流程抢卡片，一旦用了原始帧命令就自动关掉，
 * 需要恢复用 N1 */
static uint8_t PollEnabled = 1;
/* 原始帧命令停下多久之后自动把空闲探卡恢复回来，免得 OLED 一直卡着不动 */
static uint16_t RawIdle;

static uint8_t HexVal(char c)
{
	if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
	if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
	if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
	return 0xFF;
}

static uint8_t ParseHex(char *s, uint8_t *out, uint8_t maxLen)
{
	uint8_t n = 0;
	uint8_t hi;
	uint8_t lo;

	while (*s != '\0' && n < maxLen)
	{
		hi = HexVal(*s++);
		if (hi == 0xFF) break;
		lo = HexVal(*s++);
		if (lo == 0xFF) break;
		out[n++] = (uint8_t)((hi << 4) | lo);
	}
	return n;
}

static uint16_t ParseDec(char *s)
{
	uint16_t v = 0;

	while (*s >= '0' && *s <= '9')
	{
		v = (uint16_t)(v * 10 + (uint16_t)(*s - '0'));
		s++;
	}
	return v;
}

/* C：激活卡片并回报 UID / ATQA / SAK */
static void CmdActivateRaw(void)
{
	uint8_t uid[5];
	uint8_t sak = 0;
	uint8_t atqa[2] = {0, 0};

	if (!ActivateCard(uid, &sak, atqa))
	{
		Serial_printf("C:0\r\n");
		return;
	}
	Serial_printf("C:%02X%02X%02X%02X:%02X%02X:%02X\r\n",
	              uid[0], uid[1], uid[2], uid[3], atqa[1], atqa[0], sak);
}

/* X<lastbits>,<hex>：原样把字节（含 parity 位）发出去 */
static void CmdRaw(void)
{
	uint8_t payload[40];
	uint8_t out[40];
	uint8_t outLen = 0;
	uint8_t lastBits;
	uint8_t len;
	uint8_t i;
	char *p = &CmdLine[1];

	lastBits = (uint8_t)ParseDec(p);
	while (*p >= '0' && *p <= '9') p++;
	if (*p == ',') p++;

	len = ParseHex(p, payload, 40);

	if (!RC522_RawTransceive(payload, len, lastBits, out, &outLen))
	{
		/* 把中断和错误寄存器一起报出来，方便判断是没发出去还是没收到 */
		Serial_printf("X:0,irq=%02X,err=%02X\r\n",
		              RC522_GetReg(0x04), RC522_GetReg(0x06));
		return;
	}

	Serial_printf("X:%d,", outLen);
	for (i = 0; i < outLen; i++)
	{
		Serial_printf("%02X", out[i]);
	}
	Serial_printf("\r\n");
}

/* G[A|B],<block>,<hexkey>：用硬件 MFAuthent 认证（已知密钥时用） */
static void CmdAuthRaw(void)
{
	uint8_t key[8];
	uint8_t uid[5];
	uint8_t sak;
	uint8_t atqa[2];
	uint8_t mode;
	uint8_t block;
	uint8_t keyLen;
	char *p = &CmdLine[1];

	mode = (*p == 'B' || *p == 'b') ? PICC_AUTH_KEY_B : PICC_AUTH_KEY_A;
	while (*p != '\0' && *p != ',') p++;
	if (*p == ',') p++;

	block = (uint8_t)ParseDec(p);
	while (*p >= '0' && *p <= '9') p++;
	if (*p == ',') p++;

	keyLen = ParseHex(p, key, 8);

	if (keyLen != 6)
	{
		Serial_printf("G:0,badkey\r\n");
		return;
	}
	if (!ActivateCard(uid, &sak, atqa))
	{
		Serial_printf("G:0,nocard\r\n");
		return;
	}
	if (RC522_AuthKey(mode, block, key, uid) == RC522_OK)
	{
		Serial_printf("G:1\r\n");
	}
	else
	{
		Serial_printf("G:0\r\n");
	}
}

/* ---------------- 批量候选密钥验证 ----------------
 * 密钥恢复会产出十万级候选，一把一把过串口太慢，
 * 所以把验证放到板子上做：A 设定目标，T 灌一批密钥本地试。
 */
static uint8_t BatchBlock;
static uint8_t BatchType;

/* A[A|B],<block>：设定批量验证的目标 */
static void CmdBatchStart(void)
{
	char *p = &CmdLine[1];

	BatchType = (*p == 'B' || *p == 'b') ? PICC_AUTH_KEY_B : PICC_AUTH_KEY_A;
	while (*p != '\0' && *p != ',')
	{
		p++;
	}
	if (*p == ',')
	{
		p++;
	}
	BatchBlock = (uint8_t)ParseDec(p);

	Serial_printf("A:ok type=%c block=%d\r\n",
	              (BatchType == PICC_AUTH_KEY_A) ? 'A' : 'B', BatchBlock);
}

/* T<12位hex密钥><12位hex密钥>...：一把一把试，命中就报出来 */
static void CmdBatchTest(void)
{
	uint8_t uid[5];
	uint8_t sak;
	uint8_t atqa[2];
	uint8_t key[6];
	uint8_t tested = 0;
	uint8_t i;
	uint8_t ok;
	char *p = &CmdLine[1];

	while (p[0] != '\0')
	{
		ok = 1;
		for (i = 0; i < 12; i++)
		{
			if (HexVal(p[i]) == 0xFF)
			{
				ok = 0;
				break;
			}
		}
		if (!ok)
		{
			break;
		}

		for (i = 0; i < 6; i++)
		{
			key[i] = (uint8_t)((HexVal(p[i * 2]) << 4) | HexVal(p[i * 2 + 1]));
		}
		p += 12;
		tested++;

		if (!CardPresent(uid, &sak, atqa))
		{
			Delay_ms(5);
			if (!CardPresent(uid, &sak, atqa))
			{
				Serial_printf("T:nocard after %d\r\n", tested);
				return;
			}
		}

		if (RC522_AuthKey(BatchType, BatchBlock, key, uid) == RC522_OK)
		{
			Serial_printf("T:FOUND %c %02X%02X%02X%02X%02X%02X\r\n",
			              (BatchType == PICC_AUTH_KEY_A) ? 'A' : 'B',
			              key[0], key[1], key[2], key[3], key[4], key[5]);
			return;
		}
		RC522_StopCrypto();
	}

	Serial_printf("T:%d\r\n", tested);
}

/* M<block>：读一个块 */
static void CmdReadBlockRaw(void)
{
	uint8_t uid[5];
	uint8_t sak;
	uint8_t atqa[2];
	uint8_t buf[18];
	uint8_t block;
	uint8_t i;

	block = (uint8_t)ParseDec(&CmdLine[1]);

	if (!ActivateCard(uid, &sak, atqa))
	{
		Serial_printf("M:0\r\n");
		return;
	}
	if (RC522_ReadBlock(block, buf) != RC522_OK)
	{
		Serial_printf("M:0\r\n");
		return;
	}

	Serial_printf("M:1,");
	for (i = 0; i < 16; i++)
	{
		Serial_printf("%02X", buf[i]);
	}
	Serial_printf("\r\n");
}

static void PrintHelp(void)
{
	Serial_printf("---------------- commands ----------------\r\n");
	Serial_printf(" R : read card on antenna, save it as source\r\n");
	Serial_printf(" W : write source UID to block 0 of the card on antenna\r\n");
	Serial_printf(" D : dump all 16 sectors using common default keys\r\n");
	Serial_printf(" F : write the dumped data back to the card on antenna\r\n");
	Serial_printf(" S : save the dump into the W25Q64 flash\r\n");
	Serial_printf(" L : load the dump back from flash\r\n");
	Serial_printf(" I : read the W25Q64 flash ID\r\n");
	Serial_printf(" P : print the card data currently in memory\r\n");
	Serial_printf(" --- raw frame layer (for the nested attack) ---\r\n");
	Serial_printf(" C : activate card, reply C:uid:atqa:sak\r\n");
	Serial_printf(" Z : reset PICC (antenna off/on)\r\n");
	Serial_printf(" Y0/Y1 : parity handling off / on\r\n");
	Serial_printf(" X<bits>,<hex> : raw transceive, reply X:<n>,<hex>\r\n");
	Serial_printf(" G[A|B],<block>,<hexkey> : hardware auth\r\n");
	Serial_printf(" M<block> : read one block\r\n");
	Serial_printf(" ? : this help\r\n");
	Serial_printf("------------------------------------------\r\n");
}

int main(void)
{
	uint8_t version;
	char command;
	char buf[17];
	uint8_t uid[5];
	uint8_t sak;
	uint8_t atqa[2];
	uint8_t present;
	uint8_t lastState = 9;
	uint16_t pollTick = 0;
	uint8_t flashMid;
	uint16_t flashDid;

	Serial_Init();
	OLED_Init();
	RC522_Init();
	W25Q64_Init();
	Delay_ms(200);

	version = RC522_ReadVersion();

	/* 先确认 Flash 在不在：不在的话后面绝不发起写入，免得在 WaitBusy 里干等 */
	W25Q64_ReadID(&flashMid, &flashDid);
	if (flashMid == 0xEF)
	{
		FlashReady = 1;
	}

	UiText(1, "RC522 Card Tool");
	sprintf(buf, "chip ver 0x%02X", version);
	UiText(2, buf);
	UiText(3, "ready");
	UiText(4, " ");

	Serial_printf("\r\n=== STM32 + RC522 card tool ===\r\n");
	Serial_printf("VersionReg  = 0x%02X\r\n", version);
	Serial_printf("readback    : TMode=0x%02X Mode=0x%02X TxCtrl=0x%02X\r\n",
	              RC522_GetReg(0x2A), RC522_GetReg(0x11), RC522_GetReg(0x14));

	if (FlashReady)
	{
		Serial_printf("flash       : W25Q64 MID=0x%02X DID=0x%04X\r\n", flashMid, flashDid);
		if (FlashLoad())
		{
			Serial_printf("flash       : previous dump loaded into RAM\r\n");
		}
		else
		{
			Serial_printf("flash       : empty, no saved dump yet\r\n");
		}
	}
	else
	{
		Serial_printf("flash       : NOT detected (MID=0x%02X DID=0x%04X)\r\n",
		              flashMid, flashDid);
	}

	PrintHelp();
	Serial_printf("> ");

	UiWaiting();

	while (1)
	{
		if (Serial_GetChar(&command))
		{
			/* 按行收命令：\r 或 \n 表示一条命令结束。
			 * 单字符命令（R/W/D...）后面跟一个换行即可。 */
			if (command != '\r' && command != '\n')
			{
				if (CmdLen < 90)
				{
					CmdLine[CmdLen++] = command;
				}
				goto cmd_done;
			}

			if (CmdLen == 0)
			{
				goto cmd_done;
			}

			CmdLine[CmdLen] = '\0';

			/* 原始帧命令会独占卡片，先把空闲探卡关掉 */
			if (CmdLine[0] == 'C' || CmdLine[0] == 'c' ||
			    CmdLine[0] == 'Z' || CmdLine[0] == 'z' ||
			    CmdLine[0] == 'Y' || CmdLine[0] == 'y' ||
			    CmdLine[0] == 'X' || CmdLine[0] == 'x' ||
			    CmdLine[0] == 'G' || CmdLine[0] == 'g' ||
			    CmdLine[0] == 'M' || CmdLine[0] == 'm' ||
			    CmdLine[0] == 'A' || CmdLine[0] == 'a' ||
			    CmdLine[0] == 'T' || CmdLine[0] == 't')
			{
				PollEnabled = 0;
				RawIdle = 0;
			}

			Serial_printf("\r\n");
			switch (CmdLine[0])
			{
				case 'R':
				case 'r':
					CmdRead();
					break;

				case 'W':
				case 'w':
					CmdWriteUid();
					break;

				case 'D':
				case 'd':
					CmdDump();
					break;

				case 'F':
				case 'f':
					CmdWriteDump();
					break;

				case 'S':
				case 's':
					if (FlashSave())
					{
						Serial_printf("S: dump saved to W25Q64 flash\r\n");
						UiResult("Saved to flash", "dump is safe");
					}
					else
					{
						Serial_printf("S: flash not detected\r\n");
						UiResult("No flash", "check W25Q64");
					}
					break;

				case 'L':
				case 'l':
					if (FlashLoad())
					{
						Serial_printf("L: dump loaded from flash\r\n");
						UiResult("Loaded", "from flash");
					}
					else
					{
						Serial_printf("L: nothing saved in flash yet\r\n");
						UiResult("Nothing saved", "in flash");
					}
					break;

				case 'I':
				case 'i':
				{
					uint8_t mid;
					uint16_t did;

					W25Q64_ReadID(&mid, &did);
					Serial_printf("I: flash MID=0x%02X DID=0x%04X ready=%d\r\n",
					              mid, did, FlashReady);
					break;
				}

				case 'V':
				case 'v':
					Serial_printf("V:ver=%02X tmode=%02X tpresc=%02X treload=%02X%02X mode=%02X txctrl=%02X\r\n",
					              RC522_ReadVersion(),
					              RC522_GetReg(0x2A), RC522_GetReg(0x2B),
					              RC522_GetReg(0x2C), RC522_GetReg(0x2D),
					              RC522_GetReg(0x11), RC522_GetReg(0x14));
					break;

				case 'P':
				case 'p':
					CmdPrintDump();
					break;

				case 'C':
				case 'c':
					CmdActivateRaw();
					break;

				case 'Z':
				case 'z':
					RC522_ResetPICC();
					Serial_printf("Z:ok\r\n");
					break;

				case 'Y':
				case 'y':
					if (CmdLine[1] == '1')
					{
						RC522_ParityEnable();
					}
					else
					{
						RC522_ParityDisable();
					}
					Serial_printf("Y:ok\r\n");
					break;

				case 'X':
				case 'x':
					CmdRaw();
					break;

				case 'G':
				case 'g':
					CmdAuthRaw();
					break;

				case 'M':
				case 'm':
					CmdReadBlockRaw();
					break;

				case 'A':
				case 'a':
					CmdBatchStart();
					break;

				case 'T':
				case 't':
					CmdBatchTest();
					break;

				case 'N':
				case 'n':
					PollEnabled = (CmdLine[1] == '1') ? 1 : 0;
					Serial_printf("N:ok\r\n");
					break;

				case '?':
				case 'h':
				case 'H':
					PrintHelp();
					break;

				default:
					Serial_printf("unknown command '%c'\r\n", command);
					break;
			}
			Serial_printf("> ");
			CmdLen = 0;

			/* 命令跑完，屏幕回到「等待」或「卡在上面」 */
			lastState = 9;
			pollTick = 0;
		}

cmd_done:
		/* 空闲时每 ~400ms 探一次卡，让屏幕能反映卡放上去/拿走 */
		Delay_ms(50);
		pollTick++;

		/* 原始帧命令停下来一会儿之后，自动把空闲探卡打开，
		 * 不然 OLED 会一直停在最后那个状态 */
		if (!PollEnabled)
		{
			RawIdle++;
			if (RawIdle > 60)
			{
				PollEnabled = 1;
				lastState = 9;
			}
		}

		if (PollEnabled && pollTick >= 8)
		{
			RawIdle = 0;
			pollTick = 0;
			present = CardPresent(uid, &sak, atqa);

			if (present)
			{
				if (lastState != 2)
				{
					UiCardOnAntenna(uid);
					lastState = 2;
				}
				RC522_Halt();
			}
			else if (lastState != 1)
			{
				UiWaiting();
				lastState = 1;
			}
		}
	}
}
