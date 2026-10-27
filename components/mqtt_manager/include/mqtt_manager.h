
#ifndef __MQTT_MANAGER_H
#define __MQTT_MANAGER_H

#include "app_types.h"
#include "app_context.h"

esp_err_t xMqttManagerInit(void);
bool bMqttmanagerSendData( const char * topic, const uint8_t *buff, const uint32_t len);

esp_err_t xMqttManagerAckTopic(char *topic,size_t topic_size);
esp_err_t xMqttManagerStateTopic(char *topic,size_t topic_size);

#endif