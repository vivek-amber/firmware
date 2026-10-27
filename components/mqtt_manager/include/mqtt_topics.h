

#ifndef __MQTT_TOPICS_H
#define __MQTT_TOPICS_H

#include "mqtt_client.h"
#include "app_context.h"
#include "app_types.h"

void vMqttSubscribeTopics(esp_mqtt_event_handle_t  event);

esp_err_t xMqttBuildStatusTopic(char *topic, size_t topicSize);
size_t xMqttBuildStatusPayload(bool online,uint8_t *buffer,size_t bufferSize);

#endif