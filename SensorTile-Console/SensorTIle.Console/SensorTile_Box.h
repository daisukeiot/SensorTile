#pragma once

#include <stdio.h>
#include <tchar.h>
#include <stdint.h>

#ifdef _WIN32

#include <windows.h>

#endif
#define FDevAddr                     1U
#define STEVAL_IDI001V1_ADDR	     50U

#define TMsg_EOF                     0xF0
#define TMsg_BS                      0xF1
#define TMsg_BS_EOF                  0xF2
#define TMsg_MaxLen                  1024

#define CMD_Ping                     0x01U
#define CMD_Read_PresString          0x02U
#define CMD_NACK                     0x03U
#define CMD_CheckModeSupport         0x04U
#define CMD_UploadAR                 0x05U
#define CMD_UploadCP                 0x06U
#define CMD_Start_Data_Streaming     0x08U
#define CMD_Stop_Data_Streaming      0x09U
#define CMD_StartDemo                0x0AU
#define CMD_Sleep_Sec                0x0BU
#define CMD_Set_DateTime             0x0CU
#define CMD_Get_DateTime             0x0DU
#define CMD_Enter_DFU_Mode           0x0EU
#define CMD_Reset                    0x0FU
#define CMD_Enable_Disable_Sensor    0x10U
#define CMD_Initialize_Sensor        0x11U
#define CMD_Read_Sensor_Data         0x12U
#define CMD_Sensor                   0x50U
#define CMD_Reply_Add                0x80U


#define SENSOR_FLAG_PRESSURE       0x1
#define SENSOR_FLAG_TEMPERATURE    0x2
#define SENSOR_FLAG_HUMIDITY       0x4
#define SENSOR_FLAG_ACCELEROMETER  0x8
#define SENSOR_FLAG_GYRO           0x10
#define SENSOR_FLAG_MEGNETOMETER   0x20

/* Enable sensor masks */
#define PRESSURE_SENSOR_ENABLED                 0x00000001U
#define TEMPERATURE_SENSOR_ENABLED              0x00000002U
#define HUMIDITY_SENSOR_ENABLED                 0x00000004U
#define UV_SENSOR_ENABLED                       0x00000008U /* for future use */
#define ACCELEROMETER_SENSOR_ENABLED            0x00000010U
#define GYROSCOPE_SENSOR_ENABLED                0x00000020U
#define MAGNETIC_SENSOR_ENABLED                 0x00000040U
#define INTERRUPTS_ENABLED                      0x00000100U
#define FSM_ENABLED                             0x00000200U
#define MLC_ENABLED                             0x00000400U

typedef struct {
	unsigned int Len;
	unsigned char Data[TMsg_MaxLen];
} TMsg;

// 3 : Header
// 4 : TimeStampe
// 1 : Flag
// 4 : Pressure
// 4 : Temperature
// 4 : Himidity
// 12 : Accelerometer
// 12 : Gyro
// 12 : Magneto
// 1  : Checksum

// = 57
typedef struct {
	unsigned char data[64];
}SensorData1;

typedef struct {
	unsigned char hours;
	unsigned char minutes;
	unsigned char seconds;
	unsigned char sub_seconds;
} TimeStamp_Data;

typedef struct {
	TimeStamp_Data timeStamp;
	unsigned char flag;
	unsigned char data[64];
}SensorData;



HANDLE InitializeSensorTile(int ComPortNumber);
int Get_Sensor_Data(HANDLE hComm, int BufferLength, SensorData* Buffer);