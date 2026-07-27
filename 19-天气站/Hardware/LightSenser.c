#include "LightSenser.h"
#include "Delay.h"

/*================================================================
 *  光敏电阻 ADC 读数（PA0 -> ADC1_Channel_0）
 *  注意：本驱动不初始化 ADC1，由 Thermistor_Init() 统一初始化
 *================================================================*/
void LightSenser_Init(void)
{
    /* 只需要把 PA0 配成模拟输入 + 开 GPIOA 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

uint16_t LightSenser_ReadADC(void)
{
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(ADC1);
}

/*================================================================
 *  光照强度百分比：实测光照越强，光敏电阻越小，ADC 越大
 *  实际使用中需要根据当地光源强弱做修正，这里给一个常用近似
 *================================================================*/
uint8_t LightSenser_ReadPercent(void)
{
    uint16_t adc = LightSenser_ReadADC();
    /* 经验映射：把 0-4095 映射到 0-100，最大约 3000~3500 可视作"很亮" */
    uint32_t pct = adc * 100 / 3500;
    if (pct > 100) pct = 100;
    return (uint8_t)pct;
}
