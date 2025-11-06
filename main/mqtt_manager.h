#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

void mqtt_app_start(void);
// Thêm khai báo hàm này vào file .h
void mqtt_publish(const char *topic, const char *data);
#endif
