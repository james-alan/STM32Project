#include "W25Q64.h"
#include "Delay.h"

/*================================================================
 *  SPI 引脚初始化
 *================================================================*/
static void W25Q64_GPIO_Init(void)
{
    /* 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);

    /* PA4 (CS) 推挽输出 */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    W25Q64_CS_HIGH();    /* 默认拉高（未选中） */

    /* PA5(SCK) / PA7(MOSI) 复用推挽输出 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* PA6(MISO) 浮空输入 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/*================================================================
 *  SPI1 硬件初始化
 *  工作模式：全双工主机、模式 0 (CPOL=0, CPHA=0)、8 位、MSB
 *  W25Q64 支持的最大时钟为 80MHz，这里给 9MHz
 *================================================================*/
static void W25Q64_SPI_Init(void)
{
    SPI_InitTypeDef SPI_InitStructure;

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;          /* 模式 0 */
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;        /* 模式 0 */
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;  /* 72MHz/8=9MHz */
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;

    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_Cmd(SPI1, ENABLE);
}

void W25Q64_Init(void)
{
    W25Q64_GPIO_Init();
    W25Q64_SPI_Init();

    /* 调试用：读 JEDEC ID，正常应返回 0xEF4017 */
    uint16_t id = W25Q64_ReadID();
    /* 在此可以加 断言：id==W25Q64_JEDEC_ID */
    (void)id;
}

/*================================================================
 *  SPI 收发一字节
 *================================================================*/
uint8_t W25Q64_ReadWriteByte(uint8_t TxData)
{
    uint32_t timeout = W25Q64_TIMEOUT;
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) {
        if (--timeout == 0) return 0xFF;
    }
    SPI_I2S_SendData(SPI1, TxData);

    timeout = W25Q64_TIMEOUT;
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) {
        if (--timeout == 0) return 0xFF;
    }
    return SPI_I2S_ReceiveData(SPI1);
}

/*================================================================
 *  读 JEDEC ID
 *================================================================*/
uint16_t W25Q64_ReadID(void)
{
    uint16_t id = 0;
    W25Q64_CS_LOW();
    W25Q64_ReadWriteByte(W25Q64_CMD_READ_JEDEC_ID);
    id |= (uint16_t)W25Q64_ReadWriteByte(0xFF) << 8;   /* Manufacturer ID */
    id |= W25Q64_ReadWriteByte(0xFF);                  /* Memory Type + Capacity */
    id = (id << 8) | W25Q64_ReadWriteByte(0xFF);       /* 实际只用到前两个字节 */
    /* 这里返回 Manufacturer ID + Memory Type，简化用 */
    W25Q64_CS_HIGH();
    return id;
}

/*================================================================
 *  读状态寄存器1
 *================================================================*/
uint8_t W25Q64_ReadSR(void)
{
    uint8_t sr;
    W25Q64_CS_LOW();
    W25Q64_ReadWriteByte(W25Q64_CMD_READ_STATUS_REG1);
    sr = W25Q64_ReadWriteByte(0xFF);
    W25Q64_CS_HIGH();
    return sr;
}

/*================================================================
 *  写使能
 *================================================================*/
void W25Q64_WriteEnable(void)
{
    W25Q64_CS_LOW();
    W25Q64_ReadWriteByte(W25Q64_CMD_WRITE_ENABLE);
    W25Q64_CS_HIGH();
}

/*================================================================
 *  等待 BUSY 位清零（写/擦除完成后返回）
 *================================================================*/
void W25Q64_WaitForWriteEnd(void)
{
    uint32_t timeout = W25Q64_TIMEOUT;
    do {
        if (--timeout == 0) break;
    } while ((W25Q64_ReadSR() & 0x01) == 0x01);    /* 0=空闲，1=忙 */
}

/*================================================================
 *  扇区擦除（4KB）
 *  注意：W25Q64 的扇区写入只能从 1 变为 0，要重写必须先擦除
 *================================================================*/
void W25Q64_SectorErase(uint32_t addr)
{
    /* 地址合法性检查 */
    if (addr % W25Q64_SECTOR_SIZE != 0) return;

    W25Q64_WriteEnable();
    W25Q64_WaitForWriteEnd();

    W25Q64_CS_LOW();
    W25Q64_ReadWriteByte(W25Q64_CMD_SECTOR_ERASE);
    W25Q64_ReadWriteByte((uint8_t)(addr >> 16));
    W25Q64_ReadWriteByte((uint8_t)(addr >> 8));
    W25Q64_ReadWriteByte((uint8_t)addr);
    W25Q64_CS_HIGH();

    W25Q64_WaitForWriteEnd();   /* 扇区擦除典型 30~400ms */
}

/*================================================================
 *  全片擦除（约 40s，慎用！常用于首次格式化）
 *================================================================*/
void W25Q64_ChipErase(void)
{
    W25Q64_WriteEnable();
    W25Q64_WaitForWriteEnd();

    W25Q64_CS_LOW();
    W25Q64_ReadWriteByte(W25Q64_CMD_CHIP_ERASE);
    W25Q64_CS_HIGH();

    W25Q64_WaitForWriteEnd();
}

/*================================================================
 *  读数据：任意长度、任意地址
 *================================================================*/
void W25Q64_ReadData(uint32_t addr, uint8_t *buf, uint16_t len)
{
    W25Q64_CS_LOW();
    W25Q64_ReadWriteByte(W25Q64_CMD_READ_DATA);
    W25Q64_ReadWriteByte((uint8_t)(addr >> 16));
    W25Q64_ReadWriteByte((uint8_t)(addr >> 8));
    W25Q64_ReadWriteByte((uint8_t)addr);
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = W25Q64_ReadWriteByte(0xFF);
    }
    W25Q64_CS_HIGH();
}

/*================================================================
 *  页写（最多 256 字节，且必须在同一页内）
 *================================================================*/
void W25Q64_PageProgram(uint32_t addr, uint8_t *buf, uint16_t len)
{
    if (len == 0 || len > W25Q64_PAGE_SIZE) return;
    if ((addr % W25Q64_PAGE_SIZE) + len > W25Q64_PAGE_SIZE) return;   /* 跨页 */

    W25Q64_WriteEnable();

    W25Q64_CS_LOW();
    W25Q64_ReadWriteByte(W25Q64_CMD_PAGE_PROGRAM);
    W25Q64_ReadWriteByte((uint8_t)(addr >> 16));
    W25Q64_ReadWriteByte((uint8_t)(addr >> 8));
    W25Q64_ReadWriteByte((uint8_t)addr);
    for (uint16_t i = 0; i < len; i++) {
        W25Q64_ReadWriteByte(buf[i]);
    }
    W25Q64_CS_HIGH();

    W25Q64_WaitForWriteEnd();
}

/*================================================================
 *  跨页连续写入（已确保所有目标扇区已擦除，不会自动擦除）
 *================================================================*/
void W25Q64_WriteNoCheck(uint32_t addr, uint8_t *buf, uint16_t len)
{
    uint16_t pageRemain = W25Q64_PAGE_SIZE - (addr % W25Q64_PAGE_SIZE);
    if (len <= pageRemain) pageRemain = len;

    while (1) {
        W25Q64_PageProgram(addr, buf, pageRemain);
        if (len == pageRemain) break;
        else {
            buf   += pageRemain;
            addr  += pageRemain;
            len   -= pageRemain;
            pageRemain = (len > W25Q64_PAGE_SIZE) ? W25Q64_PAGE_SIZE : len;
        }
    }
}

/*================================================================
 *  安全写入：跨扇区时自动擦除目标扇区
 *  简化策略：如果起始/结束跨越扇区，就擦除涉及的扇区
 *================================================================*/
void W25Q64_Write(uint32_t addr, uint8_t *buf, uint16_t len)
{
    uint8_t tmp[W25Q64_SECTOR_SIZE];

    uint32_t secPos  = addr / W25Q64_SECTOR_SIZE;          /* 起始扇区 */
    uint32_t secOff  = addr % W25Q64_SECTOR_SIZE;          /* 扇区内偏移 */
    uint32_t secRem  = W25Q64_SECTOR_SIZE - secOff;        /* 起始扇区剩余空间 */
    uint16_t toWrite = len;

    if (toWrite <= secRem) secRem = toWrite;

    while (1) {
        /* 读出整扇区、修改、再写回 = 简单可靠（适合 4KB 范围内小数据） */
        W25Q64_ReadData(secPos * W25Q64_SECTOR_SIZE, tmp, W25Q64_SECTOR_SIZE);

        for (uint16_t i = 0; i < secRem; i++) {
            tmp[secOff + i] = buf[i];
        }

        W25Q64_SectorErase(secPos * W25Q64_SECTOR_SIZE);
        W25Q64_WriteNoCheck(secPos * W25Q64_SECTOR_SIZE, tmp, W25Q64_SECTOR_SIZE);

        if (toWrite == secRem) break;
        else {
            secPos++;
            secOff = 0;
            buf   += secRem;
            toWrite -= secRem;
            secRem  = (toWrite > W25Q64_SECTOR_SIZE) ? W25Q64_SECTOR_SIZE : toWrite;
        }
    }
}
