#include "main.h"

volatile BOOL isRunning = TRUE;

void Read_Sensor_Data(HANDLE hComm)
{
    SensorData sensorData = { 0 };

    unsigned int offset;

    float pressure_value = 0;
    float temperature_value = 0;
    float humidity_value = 0;

    unsigned int acc_x = 0;
    unsigned int acc_y = 0;
    unsigned int acc_z = 0;

    unsigned int gyr_x = 0;
    unsigned int gyr_y = 0;
    unsigned int gyr_z = 0;

    unsigned int mag_x = 0;
    unsigned int mag_y = 0;
    unsigned int mag_z = 0;

    while (isRunning)
    {
         Get_Sensor_Data(hComm, sizeof(sensorData), &sensorData);
        offset = 0;

        if (sensorData.flag & SENSOR_FLAG_PRESSURE)
        {
            (void)memcpy((void*)&pressure_value, &sensorData.data[offset], sizeof(float));
            offset += 4;
        }

        if (sensorData.flag & SENSOR_FLAG_TEMPERATURE)
        {
            (void)memcpy((void*)&temperature_value, &sensorData.data[offset], sizeof(float));
            offset += 4;
        }

        if (sensorData.flag & SENSOR_FLAG_HUMIDITY)
        {
            (void)memcpy((void*)&humidity_value, &sensorData.data[offset], sizeof(float));
            offset += 4;
        }

        if (sensorData.flag & SENSOR_FLAG_ACCELEROMETER)
        {
            (void)memcpy((void*)&acc_x, &sensorData.data[offset], 4);
            offset += 4;

            (void)memcpy((void*)&acc_y, &sensorData.data[offset], 4);
            offset += 4;

            (void)memcpy((void*)&acc_z, &sensorData.data[offset], 4);
            offset += 4;
        }

        if (sensorData.flag & SENSOR_FLAG_GYRO)
        {
            (void)memcpy((void*)&gyr_x, &sensorData.data[offset], 4);
            offset += 4;

            (void)memcpy((void*)&gyr_y, &sensorData.data[offset], 4);
            offset += 4;

            (void)memcpy((void*)&gyr_z, &sensorData.data[offset], 4);
            offset += 4;
        }

        if (sensorData.flag & SENSOR_FLAG_MEGNETOMETER)
        {
            (void)memcpy((void*)&mag_x, &sensorData.data[offset], 4);
            offset += 4;

            (void)memcpy((void*)&mag_y, &sensorData.data[offset], 4);
            offset += 4;

            (void)memcpy((void*)&mag_z, &sensorData.data[offset], 4);
            offset += 4;
        }

        printf("Time : %02d:%02d:%02d.%02d : Flag %02x : Pre %04.1f Pa / Temp %02.1f C / Hum %02.1f %% / Acc x %5d y %5d z %5d / Gyro x %7d y %7d z %7d / Mag x %4d y %4d z %4d\r\n",
            sensorData.timeStamp.hours,
            sensorData.timeStamp.minutes,
            sensorData.timeStamp.seconds,
            sensorData.timeStamp.sub_seconds,
            sensorData.flag,
            pressure_value,
            temperature_value,
            humidity_value,
            (int)acc_x,
            (int)acc_y,
            (int)acc_z,
            (int)gyr_x,
            (int)gyr_y,
            (int)gyr_z,
            (int)mag_x,
            (int)mag_y,
            (int)mag_z);

        Sleep(500);
    }

}

BOOL WINAPI ConsoleHandler(DWORD signal) {

    if (signal == CTRL_C_EVENT)
        isRunning = FALSE;

    return TRUE;
}

int main(int argc, char* argv[])
{
    int exitCode = 0;
    int comPort = -1;
    HANDLE hComm = INVALID_HANDLE_VALUE;
    char* basename = strrchr(argv[0], '\\');

    //basename = basename ? ++basename : argv[0];

    for (int i = 1; i < argc; ++i)
    {
        if (0 == strcmp(argv[i], "-c"))
        {
            ++i;
            if (i < argc)
            {
                if (isdigit(*argv[i]) == 0)
                {
                    printf("Error : Please provide numeric number for COM port\r\n");
                    exitCode = -1;
                }
                comPort = atoi(argv[i]);
            }
        }
        else if (0 == strcmp(argv[i], "-h")) {
            goto Print_Help;
        }
    }

    if (comPort == -1)
    {
        goto Print_Help;
    }

    if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
        printf("ERR : Could not set control handler");
        goto Error_Return;
    }


    hComm = InitializeSensorTile(comPort);

    if (hComm == INVALID_HANDLE_VALUE)
    {
        exitCode = 1;
        goto Error_Return;
    }

    Read_Sensor_Data(hComm);

    CloseHandle(hComm);

    return exitCode;

Print_Help:
    printf("-----------------------------------\r\n");
    printf("%s -c <COM Port Number>\r\n", basename ? basename + 1 : argv[0]);
    printf("\r\n");
    return exitCode;

Error_Return:
    printf("Err : Exiting...\r\n");
    return exitCode;
}