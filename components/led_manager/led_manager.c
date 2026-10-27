
#include "led_manager.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define LED_PIN         GPIO_NUM_32
volatile uint32_t       delayMscs;
static const char *TAG = "LED_MANAGER";

static void led_task( void * param );


esp_err_t xLedManagerInit( void ){

    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN,GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN,0);
    delayMscs = 0;

    BaseType_t res = xTaskCreate(led_task,"led_task",1024,NULL,1,NULL);

    if(res != pdPASS){
        
        ESP_LOGI(TAG,"Error while creating task");
        return res;
    }

    return ESP_OK;
}


static void led_task( void * param){


    while(1){

        if(delayMscs ==0){

            gpio_set_level(LED_PIN,0);

        }else{

            vTaskDelay(pdMS_TO_TICKS(delayMscs));
            gpio_set_level(LED_PIN,1);

            vTaskDelay(pdMS_TO_TICKS(delayMscs));
            gpio_set_level(LED_PIN,0);

        }

        vTaskDelay(pdMS_TO_TICKS(1000));

    }
}


void vLedManagerSetDelay( uint32_t delayMs ){
    delayMscs = delayMs;
}