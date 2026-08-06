#include "stm32f10x.h"
#include "MyI2C.h"
#include "MPU6050_Reg.h"

#define MPU6050_ADDRESS 0xD0

// 写寄存器
void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data)
{
    MyI2C_Start();
    MyI2C_SendByte(MPU6050_ADDRESS);
    MyI2C_ReceiveAck();
    MyI2C_SendByte(RegAddress);
    MyI2C_ReceiveAck();
    MyI2C_SendByte(Data);
    MyI2C_ReceiveAck();
 	MyI2C_Stop();
 }

// 读寄存器
uint8_t MPU6050_ReadReg(uint8_t RegAddress)
{
    uint8_t Data;

    MyI2C_Start();
    MyI2C_SendByte(MPU6050_ADDRESS);
    MyI2C_ReceiveAck();
    MyI2C_SendByte(RegAddress);
    MyI2C_ReceiveAck();
    // 重复起始
    MyI2C_Start();
    MyI2C_SendByte(MPU6050_ADDRESS | 0x01); // 发送从机地址 最后一位改为读取
    MyI2C_ReceiveAck();
    Data = MyI2C_ReceiveByte();
 	MyI2C_SendAck(1);
 	MyI2C_Stop();

 	return Data;
 }

// 初始化

void MPU6050_Init()
{
     MyI2C_Init();

    // mpu6050寄存器初始化
    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01); // 取消睡眠模式 选择时钟为x轴的
    MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);
    MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x09);   // 分频器 也就是选择精度
    MPU6050_WriteReg(MPU6050_CONFIG, 0x06);       // 配置寄存器 配置DLPF
    MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18);  // 陀螺仪配置寄存器
    MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18); // 加速度计配置寄存器
}

// 读取ID

uint8_t MPU6050_GetID()
{
    return MPU6050_ReadReg(MPU6050_WHO_AM_I);
}

// 读取数据

// mpu6050主要是一个6轴的加速度计 使用i2c试试读取他的数据
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ,
                     int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
{
    uint8_t DataH, DataL;
    DataH = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_L);
    *AccX = (DataH << 8) | DataL;

    DataH = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_L);
    *AccY = (DataH << 8) | DataL;

    DataH = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_L);
    *AccZ = (DataH << 8) | DataL;
 	DataH = MPU6050_ReadReg(MPU6050_GYRO_XOUT_H);
 	DataL = MPU6050_ReadReg(MPU6050_GYRO_XOUT_L);
 	*GyroX = (DataH << 8) | DataL;
 	DataH = MPU6050_ReadReg(MPU6050_GYRO_YOUT_H);
 	DataL = MPU6050_ReadReg(MPU6050_GYRO_YOUT_L);
 	*GyroY = (DataH << 8) | DataL;
 	DataH = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_H);
 	DataL = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_L);
 	*GyroZ = (DataH << 8) | DataL;
}
