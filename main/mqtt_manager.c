#include "mqtt_manager.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "relay_control.h"
#include "esp_crt_bundle.h"

#define MQTT_BROKER "mqtts://09db723ea0574876a727418f489b0600.s1.eu.hivemq.cloud:8883"
#define MQTT_USERNAME "relay1"
#define MQTT_PASSWORD "Dai24102004@#"

#define MQTT_TOPIC_CMD     "home/relay1/cmd"
#define MQTT_TOPIC_STATUS  "home/relay1/status"

static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t client = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    client = event->client;

    switch ((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected to HiveMQ Cloud");
            esp_mqtt_client_subscribe(client, MQTT_TOPIC_CMD, 0);
            esp_mqtt_client_publish(client, MQTT_TOPIC_STATUS, "READY", 0, 1, 0);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Message on %.*s: %.*s",
                     event->topic_len, event->topic,
                     event->data_len, event->data);

            if (strncmp(event->data, "ON", event->data_len) == 0)
            {
                relay_set(1);
                esp_mqtt_client_publish(client, MQTT_TOPIC_STATUS, "ON", 0, 1, 0);
            }
            else if (strncmp(event->data, "OFF", event->data_len) == 0)
            {
                relay_set(0);
                esp_mqtt_client_publish(client, MQTT_TOPIC_STATUS, "OFF", 0, 1, 0);
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "❌ MQTT connection error!");
            break;

        default:
            break;
    }
}

void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = MQTT_BROKER,
            .verification.crt_bundle_attach = esp_crt_bundle_attach, 
        },
        .credentials = {
            .username = MQTT_USERNAME,
            .authentication.password = MQTT_PASSWORD,
        },
        .network = {
            .disable_auto_reconnect = false,
        },
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    ESP_LOGI(TAG, "🔌 MQTT client started");
}