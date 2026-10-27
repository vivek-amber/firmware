
#ifndef __AC_DEVICE_H
#define __AC_DEVICE_H

#include "app_types.h"
#include "app_context.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Type defs */
#define AC_DEVICE_HEADER 0xAA55

typedef enum
{
    AC_DEVICE_DIRECTION_TX = 0x0600,
    AC_DEVICE_DIRECTION_RX = 0x0703,
} ac_uart_dir_t;

typedef enum
{
    AC_DEVICE_DATA_LEN_0 = 0x0000,
    AC_DEVICE_DATA_LEN_1 = 0x0100,
    AC_DEVICE_DATA_LEN_4 = 0x0400,
} ac_uart_data_len_t;

typedef enum
{
    AC_DEVICE_PACKET_LEN_0 = 0x0000,
    AC_DEVICE_PACKET_LEN_5 = 0x0500,
    AC_DEVICE_PACKET_LEN_8 = 0x0800,
} ac_uart_packet_len_t;

typedef enum
{
    AC_DEVICE_DATA_TYPE_NONE = 0x00,
    AC_DEVICE_DATA_TYPE_BOOL = 0x01,
    AC_DEVICE_DATA_TYPE_32BIT = 0x02,
    AC_DEVICE_DATA_TYPE_ENUM = 0x04,
    AC_DEVICE_DATA_TYPE_FAULT = 0x05,
} ac_uart_data_type_t;

typedef struct
{
    ac_uart_data_len_t dataLen;
    ac_uart_packet_len_t packetLen;
} ac_data_info;

typedef union
{

    struct
    {
        uint16_t header;
        uint16_t direction;
        uint16_t packetlen;

        uint8_t cmdId;
        uint8_t dataType;
        uint16_t dataLen;

        uint8_t dataAndCheckSum[5];
    };
    uint8_t buffer[15];
} ac_packet_t;

typedef enum
{

    CMD_INVALID = 0x00,
    CMD_PWR = 0x01,
    CMD_TEMP = 0x02,
    CMD_MODE = 0x04,
    CMD_FAN_SPEED = 0x05,
    CMD_ECO = 0x08,
    CMD_IDU_AQI = 0x0A,
    CMD_ODU_AQI = 0x0B,
    CMD_FAULT_BITS = 0x16,
    CMD_SLEEP = 0x19,
    CMD_HEALTH = 0x1A,
    CMD_RUNNING_PWR = 0x1C,
    CMD_CLR_PWR_DATA = 0x66,
    CMD_TURBO = 0x67,
    CMD_MUTE = 0x69,
    CMD_ROOM_TEMP = 0x6A,
    CMD_AC_ON_TIMER = 0x6B,
    CMD_MODEL_MARK = 0x6C,
    CMD_DISPLAY = 0x6D,
    CMD_V_SWING = 0x6E,
    CMD_H_SWING = 0x6F,
    CMD_MILDEW_PROOF = 0x70,
    CMD_AUTO_CLEANING = 0x72,
    CMD_AC_OFF_TIMER = 0x73,
    CMD_FAULT_ALRM = 0x74,
    CMD_CONVERTIBLE = 0x75,

} ac_device_command_t;

typedef struct
{

    uint8_t len;
    uint8_t data[15];
} ac_uart_packet_t;

typedef struct
{
    bool busy;

    uint64_t cmi;
    uint64_t vrs;

    char cmd[32];
    uint32_t val;

    TickType_t timestamp;
} pending_command_t;

/********************************** Module Functions ************************************/

void vAcDeviceProcessUartFrame(uart_msg_t *msg);
void vAcDeviceProcessCloudCommand(const mqtt_msg_t *msg);

/********************************** Helper Methods **************************************/

bool bIsValidCheckSum(const uint8_t *frame, const uint8_t frame_len);
char *pcCommandToString(ac_device_command_t command);
uint32_t u32ReadBigEndianValue(const uint8_t *frame, uint8_t frameLen);
ac_device_command_t xStringToCommand(const char *cmd);
bool bUartFrame(const ac_device_command_t cmd, uint32_t val, ac_uart_packet_t *pUartData);

#endif