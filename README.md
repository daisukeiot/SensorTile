# SensorTile

## Tools

1. SensorTile.Box
    - Read sensor data over USB

1. PC
    - Run an app to read sensor data from SensorTile.Box over USB
    - Tested with Windows 10 and Ubuntu 18.04

1. STM32CubeProgrammer
    - Download and install STM32CubeProgrammer from <https://www.st.com/ja/development-tools/stm32cubeprog.html>
    - Tool is used to flash the firmware

1. STM32CubeIDE  
    - Optional : To compile the firmware
    - Download and install STM32CubeID from <https://www.st.com/ja/development-tools/stm32cubeide.html>
    - Tested with version 1.3.1

1. FP-SNS-STBOX1 firmware package
    - Optional : To write your own firmware code
    - Download and extract the firmware package from <https://www.st.com/en/embedded-software/fp-sns-stbox1.html>
    - Tested with version 1.3.0

1. Visual Studio 2019
    - Optional : To compile sample app

1. USB Type-A to Micro-USB cable
    - To connect your PC and SensorTile.Box

## Pre-compiled Firmware

The pre-compiled firmware is based on DataLogExtended project in the firmware package.

Clone this repo or download from <https://github.com/daisukeiot/SensorTile/raw/master/Firmware/pnpdemo_v1.1.bin>

## Flashing Firmware

1. Prepare the firmware  

    - Download the pre-compiled firmware from <https://github.com/daisukeiot/SensorTile/raw/master/Firmware/pnpdemo_v1.1.bin>  
    or
    - Compile your own firmware with STM32CubeIDE + FP-SNS-STBOX1

1. Launch STM32CubeProgrammer and Open **Erasing & programming** tab

    ![Flash_00](media/Programmer_00.png)

1. Boot Sensor into DFU mode

    Hold `Boot` button then connect sensor to the PC with USB cable

    ![Sensor_00](media/Sensor_00.png)

1. Make sure STM3CubeProgrammer detects the sensor

    If not, click the circle icon

    ![Flash_02](media/Programmer_02.png)

    Once it is recognized, you should see `Port` = `USBx` and the serial number of SensorTile.Box

1. Press `Connect` button to connect to SensorTile.Box

    ![Flash_03](media/Programmer_03.png)

1. Browse the firmware file  
    If you clone this repo, use **pnpdemo_v1.1.bin** in **Firmware** folder

    ![Flash_01](media/Programmer_01.png)



1. Press "Start Programming` to flash the firmware

    ![Flash_04](media/Programmer_04.png)

1. Confirm file is successfully downloadd (Flashed)

    ![Flash_05](media/Programmer_05.png)

1. Disconnect SensorTile.Box

## Reading Sensor Data over USB

The sample firmware requires following steps :

1. Open Serial Port
1. Initialize sensors
1. Optional : Synchronize RTC with your PC
1. Read sensor data

or 

Run [Sample Console App](SensorTile-Console/README.md) in <https://github.com/daisukeiot/SensorTile/blob/master/SensorTile-Console/executable/SensorTile.Console.exe>

```cmd

SensorTile.Console.exe -c <Serial Port Number>
```

### Example

```
C:\Repo\Git\SensorTile\SensorTile-Console\executable>SensorTile.Console.exe -c 3
Serial Port Opened
System Time 14:43:59
Presentation String : SensorTile.Box for IoT Plug and Play Demo v1.0
Sensor initialization : Success
Flag 3f 14:43:59.02 : Pre 1008.02 Pa / Temp 28.77 C / Hum 41.98 % / Acc -262    4 -959 / Gyro    0  350 -560 / Mag -525 -174  675
Flag 39 14:43:59.51 : Pre 1007.94 Pa / Temp 28.77 C / Hum 41.98 % / Acc -262    4 -953 / Gyro    0  280 -560 / Mag -532 -171  670
Flag 3f 14:44:00.01 : Pre 1007.95 Pa / Temp 28.75 C / Hum 41.83 % / Acc -261    2 -952 / Gyro    0  210 -630 / Mag -526 -174  672
Flag 39 14:44:00.50 : Pre 1007.93 Pa / Temp 28.75 C / Hum 41.83 % / Acc -260    2 -954 / Gyro  -70  280 -560 / Mag -531 -171  675
Flag 3f 14:44:01.00 : Pre 1007.92 Pa / Temp 28.78 C / Hum 41.70 % / Acc -260    2 -953 / Gyro  -70    0 -560 / Mag -531 -180  675
Flag 39 14:44:01.50 : Pre 1007.92 Pa / Temp 28.78 C / Hum 41.70 % / Acc -260    3 -953 / Gyro  -70  140 -560 / Mag -531 -177  672
Flag 3f 14:44:01.99 : Pre 1007.91 Pa / Temp 28.75 C / Hum 41.52 % / Acc -259    3 -954 / Gyro    0  140 -630 / Mag -529 -175  661
Flag 3b 14:44:02.48 : Pre 1008.01 Pa / Temp 28.80 C / Hum 41.52 % / Acc -259    3 -954 / Gyro  -70  140 -630 / Mag -534 -174  670
```