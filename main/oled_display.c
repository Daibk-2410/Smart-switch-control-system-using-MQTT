#include "ssd1306.h"
#include "font8x8_basic.h"
#include "oled_display.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
static const char *TAG = "OLED";
SSD1306_t dev;

void oled_init(void)
{
    // oled display initialization

    ESP_LOGI(TAG, "Khoi tao SSD1306 voi Driver moi...");

    // 1. Reset cứng màn hình trước (Nếu màn hình đang bị đơ)
    // Rút dây nguồn cắm lại nếu cần thiết.
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // 2. Khởi tạo I2C
    // Tham số: &dev, SDA=21, SCL=22, RESET=-1
    // LƯU Ý: -1 là RẤT QUAN TRỌNG để không bị lỗi NACK
    i2c_master_init(&dev, 21, 22, -1);

    // 3. Khởi tạo màn hình
    ssd1306_init(&dev, 128, 64);

    // 4. Xóa màn hình
    ssd1306_clear_screen(&dev, false);

    // 5. Chỉnh độ tương phản
    ssd1306_contrast(&dev, 0xFF);

    ESP_LOGI(TAG, "Hien thi OLED...");

    // In chữ
    ssd1306_display_text(&dev, 0, "Relay Control", 13, false);
    ssd1306_display_text(&dev, 2, "Initializing...", 15, false);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    // Kết thúc phần OLED
}
void oled_update(oled_display_state_t state, int remaining_seconds)
{
    char line1[32];
    char line2[32];
    char line3[32];

    switch (state)
    {
    case OLED_STATE_OFF:
        snprintf(line1, sizeof(line1), "Relay: OFF");
        snprintf(line2, sizeof(line2), "Thoi gian: 0");
        snprintf(line3, sizeof(line3), "             ");
        break;
    case OLED_STATE_ON_INDEFINITE:
        snprintf(line1, sizeof(line1), "Relay: ON");
        snprintf(line2, sizeof(line2), "Thoi gian:");
        snprintf(line3, sizeof(line3), "->Khong gioi han");
        break;
    case OLED_STATE_ON_TIMER:
        int minutes = remaining_seconds / 60;
        int seconds = remaining_seconds % 60;
        snprintf(line1, sizeof(line1), "Relay: ON(Timer)");
        snprintf(line2, sizeof(line2), "Con lai: %02d:%02d", minutes, seconds);
        snprintf(line3, sizeof(line3), "             ");
        break;
    }

    ssd1306_clear_screen(&dev, false);
    ssd1306_display_text(&dev, 0, line1, strlen(line1), false);
    ssd1306_display_text(&dev, 2, line2, strlen(line2), false);
    ssd1306_display_text(&dev, 4, line3, strlen(line3), false);
}
