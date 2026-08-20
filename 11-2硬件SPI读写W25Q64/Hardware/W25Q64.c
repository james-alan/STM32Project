#include "stm32f10x.h"
#include "MySPI.h"
#include "W25Q64_Ins.h"
#include "W25Q64.h"

void W25Q64_Init()
{
    MySPI_Init();
}

// 读id
void W25Q64_ReadID(uint8_t *MID, uint16_t *DID)
{

    MySPI_Start();
    MySPI_SwapByte(W25Q64_JEDEC_ID);          // 发送
    *MID = MySPI_SwapByte(W25Q64_DUMMY_BYTE); // 接收 同时发一个占位字节出去
    *DID = MySPI_SwapByte(W25Q64_DUMMY_BYTE); // 接收 同时发一个占位字节出去
    *DID <<= 8;                               //(左移8位)
    *DID |= MySPI_SwapByte(W25Q64_DUMMY_BYTE); // 接收 同时发一个占位字节出去
    MySPI_Stop();
}
// 写使能
void W25Q64_WriteEnable()
{
    MySPI_Start();
    MySPI_SwapByte(W25Q64_WRITE_ENABLE);
    MySPI_Stop();
}
// 写忙 判断收到的状态位是否为1
void W25Q64_WaitBusy()
{
    uint32_t Timeout;
    MySPI_Start();
    MySPI_SwapByte(W25Q64_READ_STATUS_REGISTER_1);
    Timeout = 100000;
    while ((MySPI_SwapByte(W25Q64_DUMMY_BYTE) & 0x01) == 0x01)
    {
        Timeout--;
        if (Timeout == 0)
        {
            break;
        }
    }
    MySPI_Stop();
}
// 页编程
void W25Q64_PageProgram(uint32_t Address, uint8_t *DataArry, uint8_t Count)
{
    uint8_t i;

    W25Q64_WriteEnable();
    MySPI_Start();
    MySPI_SwapByte(W25Q64_PAGE_PROGRAM);
    MySPI_SwapByte(Address >> 16);
    MySPI_SwapByte(Address >> 8);
    MySPI_SwapByte(Address);
    for (i = 0; i < Count; i++)
    {
        MySPI_SwapByte(DataArry[i]);
    }
    MySPI_Stop();
    W25Q64_WaitBusy();
}

// 擦除制定扇区
void W25Q64_SectorErase(uint32_t Address)
{
    W25Q64_WriteEnable();
    MySPI_Start();
    MySPI_SwapByte(W25Q64_SECTOR_ERASE_4KB);
    MySPI_SwapByte(Address >> 16);
    MySPI_SwapByte(Address >> 8);
    MySPI_SwapByte(Address);
    MySPI_Stop();
    W25Q64_WaitBusy();
}

void W25Q64_ReadData(uint32_t Address, uint8_t *DataArry, uint8_t Count)
{
    uint8_t i;

    MySPI_Start();
    MySPI_SwapByte(W25Q64_READ_DATA);
    MySPI_SwapByte(Address >> 16);
    MySPI_SwapByte(Address >> 8);
    MySPI_SwapByte(Address);
    for (i = 0; i < Count; i++)
    {
        DataArry[i] = MySPI_SwapByte(W25Q64_DUMMY_BYTE);
    }
    MySPI_Stop();
}
