#include "SensorTile_Box.h"

static const int G_FrameBuffer_Len = 2048;
unsigned char* G_FrameBuffer;
unsigned int G_FrameBuffer_End;

/********************************************************************************************
* Add checksum to the end of message
* Return : None
********************************************************************************************/
void Compute_Checksum(TMsg* Msg)
{
	unsigned char checksum = 0;
	unsigned int i;

	for (i = 0; i < Msg->Len; i++) {
		checksum -= Msg->Data[i];
	}
	Msg->Data[i] = checksum;
	Msg->Len++;
}

/********************************************************************************************
* Compute checksum to check
********************************************************************************************/
BOOL Check_Checksum(TMsg* Msg)
{
	unsigned char CHK = 0;
	unsigned int i;

	for (i = 0; i < Msg->Len; i++) {
		CHK += Msg->Data[i];
	}
	Msg->Len--;
	return (CHK == 0);
}


/********************************************************************************************
* Convert message into byte stream
* Add MGsg_EOF tag to the end of message
* Return : # of bytes
********************************************************************************************/
int Message_to_ByteStream(unsigned char* MsgDest, TMsg* MsgSrc)
{
	unsigned int i, Count;

	Count = 0;
	for (i = 0; i < MsgSrc->Len; i++) {
		switch (MsgSrc->Data[i]) {
		case TMsg_EOF:
			MsgDest[Count] = TMsg_BS;
			Count++;
			MsgDest[Count] = TMsg_BS_EOF;
			Count++;
			break;
		case TMsg_BS:
			MsgDest[Count] = TMsg_BS;
			Count++;
			MsgDest[Count] = TMsg_BS;
			Count++;
			break;
		default:
			MsgDest[Count] = MsgSrc->Data[i];
			Count++;
		}
	}
	MsgDest[Count] = TMsg_EOF;
	Count++;
	return Count;
}

/********************************************************************************************
* Convert byte stream into TMsg
* Look for TMsg_EOF (Enf of message marker)
* Return : 1 = Success, 0 = Error
********************************************************************************************/
int ByteStream_to_Message(TMsg* Msg, unsigned char* ByteStream)
{
	int byteCount = 0;
	int state = 0;

	while ((*ByteStream) != TMsg_EOF) {
		if (state == 0) {
			if ((*ByteStream) == TMsg_BS) {
				state = 1;
			}
			else {
				Msg->Data[byteCount] = *ByteStream;
				byteCount++;
			}
		}
		else {
			if ((*ByteStream) == TMsg_BS) {
				Msg->Data[byteCount] = TMsg_BS;
				byteCount++;
			}
			else {
				if ((*ByteStream) == TMsg_BS_EOF) {
					Msg->Data[byteCount] = TMsg_EOF;
					byteCount++;
				}
				else {
					return 0; // invalid sequence
				}
			}
			state = 0;
		}
		ByteStream++;
	}
	if (state != 0)
		return 0;
	Msg->Len = byteCount;
	return 1;
}

/********************************************************************************************
* Wrapper function to call OS depending Read Function
* Return :
********************************************************************************************/
#ifdef WIN32
// Windows
BOOL Read(HANDLE hComm, unsigned char* Buffer, int Buffer_Len, int* Read_Len)
{
	DWORD l;
	if (!ReadFile(hComm, Buffer, Buffer_Len, &l, 0))
	{
		printf("ERR : ReadFile() failed.  Status %d\r\n", GetLastError());
		return FALSE;
	}
	*Read_Len = l;
	return TRUE;
}
#else
// Linux
#endif

/********************************************************************************************
* Read from serial port into buffer
* Return :
********************************************************************************************/
int ReadFrame(HANDLE hComm, unsigned char* Buffer, size_t Buffer_Len, BOOL* BFound)
{
	int read_size;
	unsigned int i;
	ULONGLONG tickStart = GetTickCount64();

	if (Buffer_Len > G_FrameBuffer_Len)
		return 0;

	*BFound = FALSE;

	i = 0;

	while (TRUE) {
		if (!Read(hComm, &G_FrameBuffer[G_FrameBuffer_End], G_FrameBuffer_Len - G_FrameBuffer_End, &read_size))
		{
			printf("ERR : Read() failed.  Status %d\r\n", GetLastError());
			return 0;
		}

		//
		// Buffer should look like this
		// Msg->Data[0]   = Msg->Data[1]
		// Msg->Data[1]   = 50 (0x32)
		// Msg->Data[2]   = CMD + 0x80
		// Msg->Data[3~]  = Command Dependent
		// Msg->Data[N-1] = Checksum
		// Msg->Data[N]   = TMsg_EOF = 0xF0

		G_FrameBuffer_End += read_size;

		while (i < G_FrameBuffer_End) {

			if ((G_FrameBuffer[i] == TMsg_EOF) || (i >= (Buffer_Len - 1))) {
				// Found EOF marker or the end of buffer
				if (i == 0) {
					// EOF in the begenning of buffer
					// Copy the remaining contents to the beginning of buffer
					G_FrameBuffer_End--;
					memcpy((char*)G_FrameBuffer, (char*)&G_FrameBuffer[1], G_FrameBuffer_End);
				}
				else {
					i++;
					memcpy((char*)Buffer, (char*)G_FrameBuffer, i);
					Buffer[i] = TMsg_EOF;
					*BFound = TRUE;

					// Copy the remaining buffer contents to the beginning
					memcpy((char*)G_FrameBuffer, (char*)&G_FrameBuffer[i], G_FrameBuffer_End - i);
					G_FrameBuffer_End -= i;
					return i;
				}
			}
			else {
				i++;
			}
		}

		if ((GetTickCount64() - tickStart) > 1000)
		{
			return 1;
		}
	}
	return 1;
}

/********************************************************************************************
* Receive Message and validate message contents
* Return :
********************************************************************************************/
int32_t ReceivedMSG(HANDLE hComm, TMsg* Msg)
{
	uint8_t My_Buffer[2 * TMsg_MaxLen];// = { 0 };
	BOOL bAvailable;

	if (!ReadFrame(hComm, My_Buffer, sizeof(My_Buffer), &bAvailable))
		return 0;

	if (!bAvailable)
		return 0;

	if (!ByteStream_to_Message(Msg, My_Buffer))
		return 0;

	if (!Check_Checksum(Msg))
		return 0;

	return 1;
}

/********************************************************************************************
* Read buffer and look for response for the specified command
* Return : # of bytes sent
********************************************************************************************/
int32_t ReadCmdResponse(HANDLE hComm, uint8_t Addr, uint8_t Cmd, uint32_t Buffer_Len, uint8_t* Buffer)
{
	TMsg Msg;
	uint32_t offset = 3;
	do {
		if (!ReceivedMSG(hComm, &Msg))
		{
			return -1;
		}

		if (((int32_t)Msg.Len) < 3)
		{
			return -1;
		}

		if (Msg.Data[0] != FDevAddr)
		{
			return -1;
		}

		if (Msg.Data[1] != Addr)
		{
			return -1;
		}

	} while (Msg.Data[2] != (Cmd + CMD_Reply_Add)
		&& Msg.Data[2] != (Cmd));

	if (Buffer == 0 && Msg.Len != 3)
	{
		printf("ERR : No buffer for response data\r\n");
		return -1;
	}

	switch (Cmd)
	{
		case CMD_Start_Data_Streaming:
		case CMD_Stop_Data_Streaming:
			offset = 0;
			break;
		case CMD_Sensor:
			offset = 5;
			break;
		default:
			offset = 3;
			break;
	}

	if (Buffer)
	{
		if (Buffer_Len < Msg.Len - offset)
		{
			printf("ERR : Buffer too small. Msg.Len %d Buffer_Len %d\r\n", Msg.Len, Buffer_Len);
			return -1;
		}
		memcpy(Buffer, &Msg.Data[offset], (Msg.Len - offset));
	}
	return Msg.Len - offset;
}

/********************************************************************************************
* Format Message and send to serial port
* 1. Adds checksum
* 2. Convert to byte stream
* Return : # of bytes sent
********************************************************************************************/
int32_t SendMsg(HANDLE hComm, TMsg* Msg)
{
	int32_t Count;
	uint8_t byteBuffer[2 * TMsg_MaxLen];
	int32_t ret_length = -1;
	DWORD dwWritten;

	Compute_Checksum(Msg);
	Count = Message_to_ByteStream(byteBuffer, Msg);

	if (WriteFile(hComm, &byteBuffer, Count, &dwWritten, NULL))
	{
		if (Count == dwWritten)
		{
			ret_length = dwWritten;
		}
	}
	return ret_length;
}

/********************************************************************************************
* Send Command
* Return : # of bytes sent
********************************************************************************************/
int32_t SendCmd(HANDLE hComm, uint8_t Addr, uint8_t Cmd, uint32_t DataLen, uint8_t* Data)
{
	TMsg Msg = { 0 };
	uint32_t i;

	Msg.Data[0] = Addr;
	Msg.Data[1] = FDevAddr;
	Msg.Data[2] = Cmd;
	Msg.Len = 3 + DataLen;

	//
	// Copy Data
	//
	for (i = 0; i < DataLen; i++) {
		Msg.Data[3 + i] = Data[i];
	}

	return SendMsg(hComm, &Msg);
}

/********************************************************************************************
* Send CMD_Start_Data_Streaming command to start data streaming
********************************************************************************************/
int Send_StartDataStreaming(HANDLE hComm, uint8_t* startingInfo)
{
	uint8_t Cmd = CMD_Start_Data_Streaming;
	int32_t ret_length = -1;

	if (SendCmd(hComm, STEVAL_IDI001V1_ADDR, Cmd, 0, NULL) != -1)
	{
		ret_length = ReadCmdResponse(hComm, STEVAL_IDI001V1_ADDR, Cmd, sizeof(TMsg), NULL);
	}

	return ret_length;
}

/********************************************************************************************
* Send CMD_Stop_Data_Streaming command to stop data streaming
********************************************************************************************/
int Send_StopDataStreaming(HANDLE hComm)
{
	uint8_t Cmd = CMD_Stop_Data_Streaming;
	int32_t ret_length = -1;

	if (SendCmd(hComm, STEVAL_IDI001V1_ADDR, Cmd, 0, NULL) != -1)
	{
		ret_length = ReadCmdResponse(hComm, STEVAL_IDI001V1_ADDR, Cmd, sizeof(TMsg), NULL);
	}

	return ret_length;
}

/********************************************************************************************
* Send CMD_Read_PresString command to retrieve Presentation String
********************************************************************************************/
int Get_PresentationString(HANDLE hComm)
{
	uint8_t Cmd = CMD_Read_PresString;
	int32_t ret_length = -1;
	unsigned char PresString[64] = { 0 };

	if (SendCmd(hComm, STEVAL_IDI001V1_ADDR, Cmd, 0, NULL) != -1)
	{
		ret_length = ReadCmdResponse(hComm, STEVAL_IDI001V1_ADDR, Cmd, sizeof(PresString), (uint8_t*)&PresString);

		if (ret_length > 0)
		{
			printf("Sensor String : %s\r\n", PresString);
		}
	}

	return ret_length;
}

/********************************************************************************************
* Send CMD_Set_DateTime command to set RTC
********************************************************************************************/
int Send_SetTimeStamp(HANDLE hComm)
{
	uint8_t Cmd = CMD_Set_DateTime;
	int32_t ret_length = -1;

	SYSTEMTIME sysTime;
	uint8_t timeData[3];
	GetLocalTime(&sysTime);

	timeData[0] = (uint8_t)sysTime.wHour;
	timeData[1] = (uint8_t)sysTime.wMinute;
	timeData[2] = (uint8_t)sysTime.wSecond;

	printf("System Time   : %02d:%02d:%02d\r\n", sysTime.wHour, sysTime.wMinute, sysTime.wSecond);

	if (SendCmd(hComm, STEVAL_IDI001V1_ADDR, Cmd, 3, (uint8_t*)&timeData) != -1)
	{
		ret_length = ReadCmdResponse(hComm, STEVAL_IDI001V1_ADDR, Cmd, sizeof(TMsg), NULL);
	}

	return ret_length;
}

/********************************************************************************************
* Send CMD_Initialize_Sensor command to intialize sensors
********************************************************************************************/
int Send_Initialize_Sensor(HANDLE hComm, uint32_t* sensorFlag)
{
	uint8_t Cmd = CMD_Initialize_Sensor;
	int32_t ret_length = -1;
	int32_t resp_ren;
	uint32_t enabled_Sensors = 0;

	if (SendCmd(hComm, STEVAL_IDI001V1_ADDR, Cmd, 4, (uint8_t*)sensorFlag) != -1)
	{
		resp_ren = ReadCmdResponse(hComm, STEVAL_IDI001V1_ADDR, Cmd, sizeof(TMsg) + sizeof(enabled_Sensors), (uint8_t*)&enabled_Sensors);

		if (resp_ren == sizeof(uint32_t) && enabled_Sensors == *sensorFlag)
		{
			printf("Sensor init   : Success\r\n");
			ret_length = resp_ren;
		}
	}

	return ret_length;
}

/********************************************************************************************
* Send CMD_Read_Sensor_Data command to read sensor data
********************************************************************************************/
int Get_Sensor_Data(HANDLE hComm, int BufferLength, SensorData* Buffer)
{
	uint8_t Cmd = CMD_Read_Sensor_Data;
	int32_t ret_length = -1;

	if (SendCmd(hComm, STEVAL_IDI001V1_ADDR, Cmd, 0, NULL) != -1)
	{
		ret_length = ReadCmdResponse(hComm, STEVAL_IDI001V1_ADDR, Cmd, BufferLength, (uint8_t*)Buffer);
	}

	return ret_length;
}

/********************************************************************************************
* Send CMD_Ping command
********************************************************************************************/
int Send_Ping(HANDLE hComm)
{
	uint8_t Cmd = CMD_Ping;
	int32_t ret_length = -1;

	if (SendCmd(hComm, STEVAL_IDI001V1_ADDR, Cmd, 0, NULL) != -1)
	{
		ret_length = ReadCmdResponse(hComm, STEVAL_IDI001V1_ADDR, Cmd, 0, NULL);
	}

	return ret_length;
}


/********************************************************************************************
* Send CMD_Sensor + SC_GET_SENSOR_NAME command to read sensor name
********************************************************************************************/
int Get_Sensor_Name(HANDLE hComm, int iSensor)
{
	uint8_t Cmd = CMD_Sensor;
	int32_t ret_length = -1;
	SensorName sensorName = { 0 };

	//Send_Ping(hComm);

	ZeroMemory((void*)&sensorName, sizeof(sensorName));
	sensorName.sensorCmd = SC_GET_SENSOR_NAME; // SC_GET_SENSOR_LIST;
	sensorName.sensorid = iSensor;

	if (SendCmd(hComm, STEVAL_IDI001V1_ADDR, Cmd, sizeof(SensorName), &sensorName) != -1)
	{
		ret_length = ReadCmdResponse(hComm, STEVAL_IDI001V1_ADDR, Cmd, sizeof(SensorName), (uint8_t*)&sensorName);
	}

	if (ret_length > 0)
	{
		switch (iSensor)
		{
		case SC_ACCELEROMETER:
			printf("Accelerometer : ");
			break;
		case SC_GYROSCOPE:
			printf("Gyroscope     : ");
			break;
		case SC_MAGNETOMETER:
			printf("Magnetometer  : ");
			break;
		case SC_TEMPERATURE:
			printf("Temperature   : ");
			break;
		case SC_HUMIDITY:
			printf("Humidity      : ");
			break;
		case SC_PRESSURE:
			printf("Pressure      : ");
			break;
		default:
			printf("Unknonw       : ");
			break;
		}

		printf("%s\r\n", sensorName.name);

	}

	return ret_length;
}

/********************************************************************************************
* Initialize SensorTile.Box connection
* 1. Open Serial Port
* 2. Get Presentation String
* 3. 
********************************************************************************************/
HANDLE InitializeSensorTile(int ComPortNumber)
{
	HANDLE hComm = INVALID_HANDLE_VALUE;
	TCHAR portName[50];
	DCB dcbSerialParams = { 0 };
	COMMTIMEOUTS commTimeOut = { 0 };

	uint32_t sensorFlag = PRESSURE_SENSOR_ENABLED | TEMPERATURE_SENSOR_ENABLED | HUMIDITY_SENSOR_ENABLED | ACCELEROMETER_SENSOR_ENABLED | GYROSCOPE_SENSOR_ENABLED | MAGNETIC_SENSOR_ENABLED;

	wsprintf(portName, TEXT("\\\\.\\COM%d"), ComPortNumber);
	
	hComm = CreateFile(portName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (hComm == INVALID_HANDLE_VALUE)
	{
		printf("Error in opening serial port\r\n");
		goto Error_Return;
	}

	printf("Serial Port   : %ws Opened\r\n", portName);

	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

	if (!GetCommState(hComm, &dcbSerialParams))
	{
		printf("ERR : GetCommState() Status %d\r\n", GetLastError());
		goto Error_Return;
	}

	dcbSerialParams.BaudRate = CBR_115200;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;

	SetCommState(hComm, &dcbSerialParams);

	commTimeOut.ReadTotalTimeoutConstant = 10;
	commTimeOut.ReadIntervalTimeout = 0;
	commTimeOut.ReadTotalTimeoutMultiplier = 0;

	if (!GetCommTimeouts(hComm, &commTimeOut)) {
		printf("ERR : GetCommTimeouts() Status %d\r\n", GetLastError());
		goto Error_Return;
	}

	commTimeOut.ReadTotalTimeoutConstant = 10;
	commTimeOut.ReadIntervalTimeout = 0;
	commTimeOut.ReadTotalTimeoutMultiplier = 0;

	if (!SetCommTimeouts(hComm, &commTimeOut)) {
		printf("ERR : SetCommTimeouts() Status %d\r\n", GetLastError());
		goto Error_Return;
	}

	if (!SetCommMask(hComm, EV_RXCHAR)) {
		printf("ERR : SetCommMask() Status %d\r\n", GetLastError());
		goto Error_Return;
	}

	//
	// Allocate buffer to read from serial
	//

	G_FrameBuffer = malloc(G_FrameBuffer_Len);

	if (G_FrameBuffer == NULL)
	{
		printf("ERR : malloc() Status %d\r\n", GetLastError());
		goto Error_Return;
	}

	ZeroMemory(G_FrameBuffer, G_FrameBuffer_Len);

	G_FrameBuffer_End = 0;

	//
	// Make sure the device is not streaming data
	//
	Send_StopDataStreaming(hComm);

	if (Send_SetTimeStamp(hComm) == -1)
	{
		printf("ERR : Failed to set timestamp\r\n");
		goto Error_Return;
	}

	if (Get_PresentationString(hComm) == -1)
	{
		printf("ERR : Failed to read Presentation String\r\n");
		goto Error_Return;
	}

	if (Send_Initialize_Sensor(hComm, &sensorFlag) == -1)
	{
		printf("ERR : Failed to initialize sensors\r\n");
		goto Error_Return;
	}

	Get_Sensor_Name(hComm, SC_ACCELEROMETER);
	Get_Sensor_Name(hComm, SC_GYROSCOPE);
	Get_Sensor_Name(hComm, SC_MAGNETOMETER);
	Get_Sensor_Name(hComm, SC_TEMPERATURE);
	Get_Sensor_Name(hComm, SC_HUMIDITY);
	Get_Sensor_Name(hComm, SC_PRESSURE);

	return hComm;

Error_Return:

	if (hComm != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hComm);
	}

	return INVALID_HANDLE_VALUE;
}