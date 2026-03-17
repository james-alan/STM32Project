#include "stm32f10x.h"
#include "PWM.h"

// Servoc初始化
void Servo_Init()
{
    PWM_Init();
}

// 设置舵机角度
void Servo_SetAngle(float Angle)
{
    PWM_SetCompare2(Angle / 180 * 2000 + 500);
}
