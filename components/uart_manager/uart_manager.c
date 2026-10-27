

#include "uart_manager.h"
#include "driver/uart.h"
#include "queue_manager.h"
#include <stdbool.h>

/* Private Variables */
static const char *TAG = "UART_MANAGER";
static QueueHandle_t uart_event_queue = NULL; // ESP-IDF queue

#define UART_NUM UART_NUM_1
#define BUF_SIZE 1024U
#define BAUD_RATE 9600

#define TX_PIN 17
#define RX_PIN 16

static uint8_t rx_buffer[BUF_SIZE];

/* Private functions */
static void vUartEventTask(void *param);

/* Initializes UART Manager*/
esp_err_t xUartManagerInit(void)
{

    /* Install uart drivers */
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 10, &uart_event_queue, 0));

    /* Uart parameter config */
    uart_config_t uart_cfg = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .source_clk = UART_SCLK_DEFAULT};
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_cfg));

    /* Setup output pins handlers */
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    /* Create task for uart */
    xTaskCreate(vUartEventTask, "uart_event_task", 3072, NULL, PRIORITY_UART_TASK, NULL);

    ESP_LOGI(TAG, "Initialized");
    return ESP_OK;
}

static void vUartEventTask(void *param)
{

    /* Local variable */
    uart_event_t event;

    while (1)
    {

        /* Waiting for UART event */
        if (xQueueReceive(uart_event_queue, (void *)&event, portMAX_DELAY))
        {

            switch (event.type)
            {

            /* Uart event for receive Data */
            case UART_DATA:
            {

                uart_msg_t msg = {0};
                size_t len = event.size;

                if (len > BUF_SIZE)
                {
                    len = BUF_SIZE;
                }

                int read_len = uart_read_bytes(UART_NUM, rx_buffer, len, pdMS_TO_TICKS(100));

                /* Send to processing queue */
                if (read_len > 0)
                {

                    msg.len = read_len;
                    memcpy(msg.data, rx_buffer, read_len);

                    xUartPost(&msg);
                    xEventPost(APP_EVENT_UART_DATA);
                }
                break;
            }

                /* Uart event for receiver buff full */
            case UART_BUFFER_FULL:
            {

                break;
            }
            default:
            {
                break;
            }
            }
        }
    }
}

bool bSendDataToDevice(const uint8_t *buff, const uint8_t len)
{

    if (buff == NULL || len == 0)
    {
        ESP_LOGE(TAG, "Invalid UART transmit data");
        return false;
    }

    int written = uart_write_bytes(
        UART_NUM,
        (const char *)buff,
        len);

    if (written < 0)
    {
        ESP_LOGE(TAG, "uart_write_bytes failed");
        return false;
    }

    esp_err_t result = uart_wait_tx_done(UART_NUM, pdMS_TO_TICKS(100));

    if (result != ESP_OK)
    {
        ESP_LOGE(
            "UART",
            "UART transmit timeout: %s",
            esp_err_to_name(result));
        return false;
    }

    return true;
}