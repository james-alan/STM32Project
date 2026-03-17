#ifndef __COUNTSENSOR_H
#define __COUNTSENSOR_H

void CountSensor_Init();
uint16_t CountSensor_Get();
void EXTI15_10_IRQHandler();




#endif