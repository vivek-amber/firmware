
#include "ac_device.h"
#include "esp_log.h"

/* Private func declarations */
ac_uart_data_type_t xDataTypeFromCommand(ac_device_command_t cmd);
static bool bUartDataFromRawData(int32_t rawValue, uint16_t dataLength, uint8_t *result);
void vCalculateCheckSum(ac_packet_t *data, const uint16_t uartDataLen, const uint8_t totalLen);
ac_data_info xDataLenAndPacketInfo(const ac_device_command_t cmd);

uint32_t u32ReadBigEndianValue(const uint8_t *frame, uint8_t frameLen)
{

    if (frame == NULL || frameLen < 12)
    {
        return 0;
    }

    uint16_t dataLen = (frame[9] << 8) | frame[8];

    if (dataLen == AC_DEVICE_DATA_LEN_1)
    {
        return frame[10];
    }

    if (dataLen == AC_DEVICE_DATA_LEN_4)
    {
        return ((uint32_t)frame[10] << 24) |
               ((uint32_t)frame[11] << 16) |
               ((uint32_t)frame[12] << 8) |
               ((uint32_t)frame[13]);
    }

    return 0;
}

char *pcCommandToString(ac_device_command_t command)
{
    switch (command)
    {
    case CMD_PWR:
        return "powr";
    case CMD_TEMP:
        return "temp";
    case CMD_FAN_SPEED:
        return "fans";
    case CMD_MODE:
        return "mode";
    case CMD_TURBO:
        return "turb";
    case CMD_ECO:
        return "econ";
    case CMD_SLEEP:
        return "slep";
    case CMD_ROOM_TEMP:
        return "rtmp";
    case CMD_DISPLAY:
        return "disp";
    case CMD_IDU_AQI:
        return "iaqi";
    case CMD_ODU_AQI:
        return "oaqi";
    case CMD_RUNNING_PWR:
        return "rpwr";
    case CMD_FAULT_ALRM:
    case CMD_FAULT_BITS:
        return "eror";
    case CMD_V_SWING:
        return "vswg";
    case CMD_H_SWING:
        return "hswg";
    case CMD_AC_ON_TIMER:
        return "ontm";
    case CMD_AC_OFF_TIMER:
        return "oftm";

    default:
        return NULL;
    }
}

ac_device_command_t xStringToCommand(const char *cmd)
{
    if (cmd == NULL)
    {
        return CMD_INVALID;
    }

    if (strcmp(cmd, "powr") == 0)
        return CMD_PWR;
    if (strcmp(cmd, "temp") == 0)
        return CMD_TEMP;
    if (strcmp(cmd, "fans") == 0)
        return CMD_FAN_SPEED;
    if (strcmp(cmd, "mode") == 0)
        return CMD_MODE;
    if (strcmp(cmd, "turb") == 0)
        return CMD_TURBO;
    if (strcmp(cmd, "econ") == 0)
        return CMD_ECO;
    if (strcmp(cmd, "slep") == 0)
        return CMD_SLEEP;
    if (strcmp(cmd, "rtmp") == 0)
        return CMD_ROOM_TEMP;
    if (strcmp(cmd, "disp") == 0)
        return CMD_DISPLAY;
    if (strcmp(cmd, "iaqi") == 0)
        return CMD_IDU_AQI;
    if (strcmp(cmd, "oaqi") == 0)
        return CMD_ODU_AQI;
    if (strcmp(cmd, "rpwr") == 0)
        return CMD_RUNNING_PWR;
    if (strcmp(cmd, "eror") == 0)
        return CMD_FAULT_ALRM;
    if (strcmp(cmd, "vswg") == 0)
        return CMD_V_SWING;
    if (strcmp(cmd, "hswg") == 0)
        return CMD_H_SWING;
    if (strcmp(cmd, "ontm") == 0)
        return CMD_AC_ON_TIMER;
    if (strcmp(cmd, "oftm") == 0)
        return CMD_AC_OFF_TIMER;

    return CMD_INVALID;
}

/* Constructs Uart frame from cmd and value */
bool bUartFrame(const ac_device_command_t cmd, uint32_t val, ac_uart_packet_t *pUartData)
{

    ac_uart_data_type_t dataType = xDataTypeFromCommand(cmd);

    if (dataType == AC_DEVICE_DATA_TYPE_NONE)
    {
        return false;
    }

    ac_data_info dataInfo = xDataLenAndPacketInfo(cmd);

    if (dataInfo.dataLen == AC_DEVICE_DATA_LEN_0 || dataInfo.packetLen == AC_DEVICE_PACKET_LEN_0)
    {
        return false;
    }

    /* length total buffer to be sent */
    uint8_t totalBuffLen = (dataInfo.dataLen == AC_DEVICE_DATA_LEN_1) ? 12 : 15;
    uint8_t uartData[4] = {0};

    if (!bUartDataFromRawData((uint32_t)val, dataInfo.dataLen, uartData))
    {
        return false;
    }

    /* Constructing packet */
    ac_packet_t packet = {
        .header = AC_DEVICE_HEADER,
        .cmdId = (uint8_t)cmd,
        .dataLen = dataInfo.dataLen,
        .packetlen = dataInfo.packetLen,
        .direction = AC_DEVICE_DIRECTION_TX,
        .dataType = dataType};

    memcpy(packet.dataAndCheckSum,uartData,(dataInfo.dataLen >> 8));

    /* Calculate checksum and append */
    vCalculateCheckSum(&packet, dataInfo.dataLen, totalBuffLen);

    memcpy(pUartData->data, packet.buffer, totalBuffLen);
    pUartData->len = totalBuffLen;

    return true;
}

// CONVERT RAW DATA INTO CHUNKS OF BYTES
static bool bUartDataFromRawData(int32_t rawValue, uint16_t dataLength, uint8_t *result)
{
    if (result == NULL)
    {
        return false;
    }

    switch (dataLength)
    {
    case AC_DEVICE_DATA_LEN_1:
        result[0] = (uint8_t)rawValue;
        return true;

    case AC_DEVICE_DATA_LEN_4:
        result[0] = (uint8_t)((rawValue >> 24) & 0xFFU);
        result[1] = (uint8_t)((rawValue >> 16) & 0xFFU);
        result[2] = (uint8_t)((rawValue >> 8) & 0xFFU);
        result[3] = (uint8_t)(rawValue & 0xFFU);
        return true;

    default:
        return false;
    }
}

ac_data_info xDataLenAndPacketInfo(const ac_device_command_t cmd)
{

    switch (cmd)
    {
    case CMD_PWR:
    case CMD_H_SWING:
    case CMD_V_SWING:
    case CMD_DISPLAY:
    case CMD_ECO:
    case CMD_SLEEP:
    case CMD_TURBO:
    case CMD_HEALTH:
    case CMD_CLR_PWR_DATA:
    case CMD_MUTE:
    case CMD_MILDEW_PROOF:
    case CMD_AUTO_CLEANING:
    case CMD_CONVERTIBLE:
    case CMD_MODE:
    case CMD_FAN_SPEED:
    case CMD_MODEL_MARK:
    {
        ac_data_info data = {
            .dataLen = AC_DEVICE_DATA_LEN_1,
            .packetLen = AC_DEVICE_PACKET_LEN_5};
        return data;
    }
    case CMD_TEMP:
    case CMD_RUNNING_PWR:
    case CMD_ROOM_TEMP:
    case CMD_AC_ON_TIMER:
    case CMD_AC_OFF_TIMER:
    case CMD_FAULT_BITS:
    case CMD_FAULT_ALRM:
    {
        ac_data_info data = {
            .dataLen = AC_DEVICE_DATA_LEN_4,
            .packetLen = AC_DEVICE_PACKET_LEN_8};
        return data;
    }
    default:
    {
        ac_data_info data = {
            .dataLen = AC_DEVICE_DATA_LEN_0,
            .packetLen = AC_DEVICE_PACKET_LEN_0,
        };
        return data;
    }
    }
}

void vCalculateCheckSum(ac_packet_t *data, const uint16_t uartDataLen, const uint8_t totalLen)
{

    uint8_t checkSum = 0x00;

    for (uint8_t i = 0; i < totalLen - 1; i++)
    {
        checkSum += data->buffer[i];
    }

    if (uartDataLen == AC_DEVICE_DATA_LEN_1)
    {
        data->dataAndCheckSum[1] = checkSum;
    }
    else if (uartDataLen == AC_DEVICE_DATA_LEN_4)
    {
        data->dataAndCheckSum[4] = checkSum;
    }
}

/* Validate check sum of uart data */
bool bIsValidCheckSum(const uint8_t *frame, const uint8_t frame_len)
{

    /* Local variables */
    volatile uint32_t calcChecksum = 0;

    /* Calculated checksum */
    for (uint8_t i = 0; i < frame_len - 1; i++)
    {
        calcChecksum += frame[i];
    }

    uint8_t expectedChecksum = (uint8_t)(calcChecksum & 0xFF);
    uint8_t receivedChecksum = frame[frame_len - 1];

    return (expectedChecksum == receivedChecksum);
}

ac_uart_data_type_t xDataTypeFromCommand(ac_device_command_t cmd)
{

    switch (cmd)
    {
    case CMD_PWR:
    case CMD_H_SWING:
    case CMD_V_SWING:
    case CMD_DISPLAY:
    case CMD_ECO:
    case CMD_SLEEP:
    case CMD_TURBO:
    case CMD_HEALTH:
    case CMD_CLR_PWR_DATA:
    case CMD_MUTE:
    case CMD_MILDEW_PROOF:
    case CMD_AUTO_CLEANING:

        return AC_DEVICE_DATA_TYPE_BOOL;

    case CMD_TEMP:
    case CMD_RUNNING_PWR:
    case CMD_ROOM_TEMP:
    case CMD_AC_ON_TIMER:
    case CMD_AC_OFF_TIMER:

        return AC_DEVICE_DATA_TYPE_32BIT;

    case CMD_CONVERTIBLE:
    case CMD_MODE:
    case CMD_FAN_SPEED:
    case CMD_MODEL_MARK:

        return AC_DEVICE_DATA_TYPE_ENUM;

    case CMD_FAULT_BITS:
    case CMD_FAULT_ALRM:

        return AC_DEVICE_DATA_TYPE_FAULT;

    default:
        return AC_DEVICE_DATA_TYPE_NONE;
    }
}