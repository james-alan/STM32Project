#include "stm32f10x.h"

uint16_t MyDMA_Size; //设置全局变量

void MyDMA_Init(uint32_t AddrA, uint32_t AddrB, uint16_t Size)
{

   MyDMA_Size = Size;

   RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
   DMA_InitTypeDef DMA_InitStructure;
   DMA_InitStructure.DMA_PeripheralBaseAddr = AddrA;                       // 外设的基地址
   DMA_InitStructure.DMA_MemoryBaseAddr = AddrB;                           // 存储器基地址
   DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;                      // 传输方向
   DMA_InitStructure.DMA_BufferSize = Size;                                // 转运数据大小
   DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Enable;         // 外设自增
   DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                 // 内存自增
   DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; // 外设数据大小
   DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;         // 内存数据大小
   DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                           // 模式
   DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;                   // 优先级
   DMA_InitStructure.DMA_M2M = DMA_M2M_Enable;                             // 触发方式 硬件触发还是软件触发  存储器到存储器是软件触发

   DMA_Init(DMA1_Channel1, &DMA_InitStructure);

   DMA_Cmd(DMA1_Channel1, DISABLE);
}

// 启动DMA转运

// 过成是 先关闭dma->写入数据到寄存器->打开dma ->等待DMA工作完成 ->清楚dma标志位
void My_DMA_Transfer(void)
{
   DMA_Cmd(DMA1_Channel1, DISABLE); //
   DMA_SetCurrDataCounter(DMA1_Channel1, MyDMA_Size);
   DMA_Cmd(DMA1_Channel1, ENABLE);

   while (DMA_GetFlagStatus(DMA1_FLAG_TC1) == RESET)
      ;
   DMA_ClearFlag(DMA1_FLAG_TC1);
}