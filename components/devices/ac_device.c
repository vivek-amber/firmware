
#include "ac_device.h"
#include "proto.h"
#include "uart_manager.h"
#include "mqtt_manager.h"
#include "queue_manager.h"

/* Private variables */
static const char *TAG = "AC_DEVICE";
static uint8_t rx_buffer[512];
static size_t rx_len = 0;
static pending_command_t pendingCmd;
#define PENDING_COMMAND_TIMEOUT_MS 2000U // 2sec
static TimerHandle_t xAcDeviceTimer = NULL;
#define COMPANY_ID "amber03AABCA3456E1Z9"

/* Private functions declarations */
static void vProcessUartData(void);
static void vTelemetrySendToCloud(const uint8_t *frame, const uint8_t frame_len);
static bool bTelemetrySendToDevice(const char *cmd, const uint32_t val);
static void vHandleModuleConfig(const uint8_t *frame);

/************************************************************************/
/*                            Command from device                       */
/***********************************************************************/

/* 1. Copies coming data and calls processing func */
void vAcDeviceProcessUartFrame(uart_msg_t *msg)
{
    /* Rx buff overflow protection */
    if ((rx_len + msg->len) > sizeof(rx_buffer))
    {
        ESP_LOGE(TAG, "RX buffer overflow");

        memset(rx_buffer, 0, sizeof(rx_buffer));
        rx_len = 0;

        return;
    }

    /* Copy coming data locally */
    memcpy(&rx_buffer[rx_len], msg->data, msg->len);
    rx_len += msg->len;

    /* Process received data */
    vProcessUartData();
}

/* 2. Processes Uart data from device , validates checksum and passes to telemetry */
static void vProcessUartData(void)
{

    while (rx_len >= 6) // Need at least header + direction + packetLen
    {

        uint16_t header = rx_buffer[0] | (rx_buffer[1] << 8);

        if (header != AC_DEVICE_HEADER)
        {
            /* Resync */
            memmove(rx_buffer, &rx_buffer[1], rx_len - 1);

            rx_len--;
            continue;
        }

        /* packetLen field */
        uint16_t packet_len = (rx_buffer[4] << 8) | rx_buffer[5];
        if (packet_len > sizeof(rx_buffer) - 7)
        {
            ESP_LOGE(TAG, "Invalid packet len %u", packet_len);

            memmove(rx_buffer,
                    &rx_buffer[1],
                    rx_len - 1);

            rx_len--;
            continue;
        }

        /* Full frame size */
        uint8_t frame_len = packet_len + 7;

        /* Wait for remaining bytes */
        if (rx_len < frame_len)
        {
            break;
        }

        /* Validate Check sum and send to telemetry */
        if (bIsValidCheckSum(rx_buffer, frame_len))
        {

            uint16_t code = rx_buffer[2] << 8 | rx_buffer[3];

            if (code == MODULE_CONFIG_CODE)
            {

                vHandleModuleConfig(rx_buffer);
            }
            else
            {

                vTelemetrySendToCloud(rx_buffer, frame_len);
            }
        }
        else
        {

            ESP_LOGE(TAG, "Check sum did not match");
        }

        /* Remove processed frame */
        memmove(rx_buffer, &rx_buffer[frame_len], rx_len - frame_len);
        rx_len -= frame_len;
    }
}

/* 3.1 Handles uart data from ProcessedUartData for sending to mqtt queue */
static void vTelemetrySendToCloud(const uint8_t *frame, const uint8_t frame_len)
{
    /* Local Variable */
    uint8_t cmId = frame[6];
    const char *cmd = pcCommandToString(cmId);
    char topic[128];

    if (cmd == NULL)
    {
        ESP_LOGW(TAG, "Unknown command %u", cmId);
        return;
    };

    uint32_t value = u32ReadBigEndianValue(frame, frame_len);

    /* Handle sucess and failure in both pending and non-pending command situation */

    // 1. Decide is its pending command
    bool isPendingCommand = false;
    if (pendingCmd.busy && (strcmp(pendingCmd.cmd, cmd) == 0) && pendingCmd.val == value)
    {
        isPendingCommand = true;
    }

    uint8_t encodedBuff[128] = {0};
    pb_ostream_t stream = pb_ostream_from_buffer(encodedBuff, sizeof(encodedBuff));

    // 2. If pending clear the flag and treat as Acknowledge
    if (isPendingCommand)
    {

        if (xMqttManagerAckTopic(topic, sizeof(topic)) != ESP_OK)
        {

            ESP_LOGE(TAG, "ACK topic creation failed.");
            return;
        }

        device_DeviceAck ack = {
            .cmi = pendingCmd.cmi,
            .sts = true,
            .val = pendingCmd.val,
            .vrs = pendingCmd.vrs,
        };
        strncpy(ack.cmd, pendingCmd.cmd, sizeof(ack.cmd) - 1);
        ack.cmd[sizeof(ack.cmd) - 1] = '\0';

        if (!pb_encode(&stream, device_DeviceAck_fields, &ack))
        {

            ESP_LOGE(TAG, "ACK encoding failed: %s", PB_GET_ERROR(&stream));
            return;
        }

        if (bMqttmanagerSendData(topic, encodedBuff, stream.bytes_written))
        {

            ESP_LOGI(TAG, "ACK sent cmd: %s , val: %d", cmd, value);
            pendingCmd.busy = false;
        }
        else
        {
            // TODO: HAndle Failure with retry
            ESP_LOGE(TAG, "ACK sent sending failure");
        }

        return;
    }

    // 3 Will be treated as state now
    // 3.1 ERROR
    if (strcmp(cmd, "eror") == 0)
    {
        if (xMqttManagerErrorTopic(topic, sizeof(topic)) != ESP_OK)
        {

            ESP_LOGE(TAG, "Error topic creation failed.");
            return;
        }

        device_DeviceError eror = {
            .value = value,
        };

        if (!pb_encode(&stream, device_DeviceError_fields, &eror))
        {

            ESP_LOGE(TAG, "Error encoding failed: %s", PB_GET_ERROR(&stream));
            return;
        }

        if (bMqttmanagerSendData(topic, encodedBuff, stream.bytes_written))
        {

            ESP_LOGI(TAG, "Error sent cmd: %s , val: %d", cmd, value);
        }
        else
        {
            // TODO: HAndle Failure with retry
            ESP_LOGE(TAG, "Error sending failure");
        }

        return;
    }

    // 3.2 OTHER
    if (xMqttManagerStateTopic(topic, sizeof(topic)) != ESP_OK)
    {

        ESP_LOGE(TAG, "ACK topic creation failed.");
        return;
    }

    device_DeviceState state = {
        .val = value,
    };
    strncpy(state.cmd, cmd, sizeof(state.cmd) - 1);
    state.cmd[sizeof(state.cmd) - 1] = '\0';

    if (!pb_encode(&stream, device_DeviceState_fields, &state))
    {

        ESP_LOGE(TAG, "State encoding failed: %s", PB_GET_ERROR(&stream));
        return;
    }

    if (bMqttmanagerSendData(topic, encodedBuff, stream.bytes_written))
    {

        ESP_LOGI(TAG, "State sent cmd: %s , val: %d", cmd, value);
    }
    else
    {
        // TODO: HAndle Failure with retry
        ESP_LOGE(TAG, "State sending failure");
    }
}

/* 3.2 Hamdle uart data from ProcessedUartData for configuring module */
static void vHandleModuleConfig(const uint8_t *frame)
{

    uint8_t cmdId = frame[2];
    if (cmdId == CMD_PROVISION)
    {
        xEventPost(APP_EVENT_WIFI_PROVISIONING_START);
    }
}

/************************************************************************/
/*                            Command from server                       */
/***********************************************************************/

/* 1. Processes data coming from server and sends to sender func */
void vAcDeviceProcessCloudCommand(const mqtt_msg_t *msg)
{

    if (msg == NULL)
    {
        return;
    }

    /* Pending Command check */
    if (pendingCmd.busy)
    {
        /* Send fail status to cloud on timeout */
        if ((xTaskGetTickCount() - pendingCmd.timestamp) >
            pdMS_TO_TICKS(PENDING_COMMAND_TIMEOUT_MS))
        {

            char topic[128];
            if (xMqttManagerAckTopic(topic, sizeof(topic)) != ESP_OK)
            {

                ESP_LOGE(TAG, "ACK topic creation failed.");
                return;
            }
            uint8_t encodedBuff[128] = {0};
            pb_ostream_t stream = pb_ostream_from_buffer(encodedBuff, sizeof(encodedBuff));

            device_DeviceAck ack = {
                .cmi = pendingCmd.cmi,
                .sts = false,
                .val = pendingCmd.val,
                .vrs = pendingCmd.vrs};
            strncpy(ack.cmd, pendingCmd.cmd, sizeof(ack.cmd) - 1);
            ack.cmd[sizeof(ack.cmd) - 1] = '\0';

            if (!pb_encode(&stream, device_DeviceAck_fields, &ack))
            {
                ESP_LOGE(TAG, "ACK encoding failed");
            }

            if (bMqttmanagerSendData(topic, encodedBuff, stream.bytes_written))
            {

                ESP_LOGI(TAG, "ACK sent cmd: %s , val: %d", pendingCmd.cmd, pendingCmd.val);
            }
            else
            {
                // TODO: HAndle Failure with retry
                ESP_LOGE(TAG, "ACK sent sending failure");
            }

            // Clear pending state
            memset(&pendingCmd, 0, sizeof(pendingCmd));
        }
        else
        {
            ESP_LOGI(TAG, "Previous command already in flight");
            return;
        }
    }

    /* Decode coming command */
    device_DeviceCommand cmd = device_DeviceCommand_init_zero;

    pb_istream_t stream = pb_istream_from_buffer(
        msg->data,
        msg->len);

    bool status = pb_decode(
        &stream,
        device_DeviceCommand_fields,
        &cmd);

    if (!status)
    {
        ESP_LOGE(TAG, "Command decode failed");
        return;
    }

    /* Store command and block line till round trip complete */
    pendingCmd.cmi = cmd.cmi;
    pendingCmd.val = cmd.val;
    pendingCmd.vrs = cmd.vrs;
    pendingCmd.timestamp = xTaskGetTickCount();
    strncpy(pendingCmd.cmd, cmd.cmd, sizeof(cmd.cmd) - 1);
    pendingCmd.busy = true;
    pendingCmd.cmd[sizeof(pendingCmd.cmd) - 1] = '\0';

    if (!bTelemetrySendToDevice(cmd.cmd, cmd.val))
    {

        pendingCmd.busy = false;
        return;
    }
}

/* 2. Sends the processed command to device over uart */
static bool bTelemetrySendToDevice(const char *cmd, const uint32_t val)
{

    /*1. Get command id ac to device */
    ac_device_command_t command = xStringToCommand(cmd);

    if (command == CMD_INVALID)
    {
        ESP_LOGE(TAG, "Invalid command received from server : %s", cmd);
        return false;
    }
    ac_uart_packet_t packet;

    /* Packet formation validation */
    if (!bUartFrame(command, val, &packet))
    {

        ESP_LOGE(TAG, "Error while packet formation");
        return false;
    }

    /* Send to device */
    if (!bSendDataToDevice(packet.data, packet.len))
    {
        // TODO:
        ESP_LOGE(TAG, "Error while uart TX");
        return false;
    }

    return true;
}

/* 3. Periodic check for failed commands */
void vHandleTimeoutAck(void)
{
    if (pendingCmd.busy &&
        (xTaskGetTickCount() - pendingCmd.timestamp) >
            pdMS_TO_TICKS(PENDING_COMMAND_TIMEOUT_MS))
    {
        // send failure ack
        char topic[128];
        if (xMqttManagerAckTopic(topic, sizeof(topic)) != ESP_OK)
        {

            ESP_LOGE(TAG, "ACK topic creation failed.");
            return;
        }
        uint8_t encodedBuff[128] = {0};
        pb_ostream_t stream = pb_ostream_from_buffer(encodedBuff, sizeof(encodedBuff));

        device_DeviceAck ack = {
            .cmi = pendingCmd.cmi,
            .sts = false,
            .val = pendingCmd.val,
            .vrs = pendingCmd.vrs};
        strncpy(ack.cmd, pendingCmd.cmd, sizeof(ack.cmd) - 1);
        ack.cmd[sizeof(ack.cmd) - 1] = '\0';

        if (!pb_encode(&stream, device_DeviceAck_fields, &ack))
        {
            ESP_LOGE(TAG, "ACK encoding failed");
            return;
        }

        if (bMqttmanagerSendData(topic, encodedBuff, stream.bytes_written))
        {

            ESP_LOGI(TAG, "ACK sent cmd: %s , val: %d", pendingCmd.cmd, pendingCmd.val);
        }
        else
        {
            // TODO: HAndle Failure with retry
            ESP_LOGE(TAG, "ACK sent sending failure");
        }

        // Clear pending state
        memset(&pendingCmd, 0, sizeof(pendingCmd));
    }
}

// Timer CallBack
static void vAcDeviceTimerCb(TimerHandle_t xTimer)
{
    xEventPost(APP_EVENT_AC_TIMEOUT_CHECK);
}

void vDeviceInit(void)
{
    xAcDeviceTimer = xTimerCreate(
        "ac_tmr",
        pdMS_TO_TICKS(1000),
        pdTRUE,
        NULL,
        vAcDeviceTimerCb);

    if (xAcDeviceTimer != NULL)
    {
        xTimerStart(xAcDeviceTimer, 0);
    }
}