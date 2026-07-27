#include "Thermistor.h"
#include "Delay.h"
#include <math.h>

/*================================================================
 *  ADC1 + PA1 初始化
 *  - 采集 NTC 热敏电阻分压输出
 *  - 不开 DMA、不开扫描，单通道单次转换
 *================================================================*/
void Thermistor_Init(void)
{
    /* 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);     /* ADC 时钟 = 72/6 = 12MHz (<=14MHz) */

    /* PA1 模拟输入 */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* ADC 复位 + 配置 */
    ADC_DeInit(ADC1);
    ADC_InitTypeDef ADC_InitStructure;
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE);

    /* 上电后第一次转换会有问题，校准 */
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1) == SET);
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1) == SET);
}

/*================================================================
 *  软件触发一次 ADC 转换，返回 0~4095
 *================================================================*/
uint16_t Thermistor_ReadADC(void)
{
    /* 配置通道 1 (PA1 -> ADC1_Channel_1) */
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_55Cycles5);

    /* 触发 */
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    /* 等待转换完成 */
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);

    /* 读结果 + 清标志 */
    return ADC_GetConversionValue(ADC1);
}

/*================================================================
 *  NTC 温度换算（Steinhart-Hart 简化形式）
 *
 *  硬件是 3.3V → R_FIXED → NTC → GND，AO 取 NTC 上端电压
 *      Vout = 3.3 * Rntc / (R_FIXED + Rntc)
 *      Rntc  = R_FIXED * Vout / (3.3 - Vout)
 *  求温度：
 *      1/T = 1/T_ref + (1/B)*ln(Rntc/R_ref)
 *================================================================*/
float Thermistor_ToTemp(uint16_t adc)
{
    if (adc == 0 || adc >= (uint16_t)NTC_ADC_MAX) return -99.0f;   /* 异常值 */

    float v     = adc * (NTC_ADC_VREF / NTC_ADC_MAX);
    float rntc  = NTC_R_FIXED * v / (NTC_ADC_VREF - v);

    float invT  = (1.0f / NTC_T_REF) + (1.0f / NTC_B) * logf(rntc / NTC_R_REF);
    float tempK = 1.0f / invT;
    return tempK - 273.15f;
}

/*================================================================
 *  一次平均值滤波：连采 8 次，去最大最小取平均
 *================================================================*/
float Thermistor_ReadTemp(void)
{
    uint16_t min = 0xFFFF, max = 0, sum = 0;
    for (uint8_t i = 0; i < 8; i++) {
        uint16_t v = Thermistor_ReadADC();
        if (v < min) min = v;
        if (v > max) max = v;
        sum += v;
    }
    uint16_t avg = (sum - min - max) / 6;
    return Thermistor_ToTemp(avg);
}
