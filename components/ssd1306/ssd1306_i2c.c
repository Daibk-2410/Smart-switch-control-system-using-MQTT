// File: components/ssd1306/ssd1306_i2c.c

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h" // Dùng driver mới
#include "esp_log.h"
#include "ssd1306.h"

#define TAG "SSD1306_I2C_NEW"

// --- QUAN TRỌNG: Giảm tốc độ xuống 100kHz để tránh lỗi NACK ---
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_TICKS_TO_WAIT 100

void i2c_master_init(SSD1306_t *dev, int16_t sda, int16_t scl, int16_t reset)
{
    ESP_LOGI(TAG, "Initializing I2C Master (SDA=%d, SCL=%d, RST=%d)", sda, scl, reset);

    // 1. Cấu hình I2C Bus
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1, // Để tự động chọn port
        .scl_io_num = scl,
        .sda_io_num = sda,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t i2c_bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c_bus_handle));

    // 2. Cấu hình thiết bị trên Bus
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x3C, // Địa chỉ cố định 0x3C
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    i2c_master_dev_handle_t i2c_dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &i2c_dev_handle));

    // 3. Xử lý chân Reset (Chỉ chạy nếu reset >= 0)
    if (reset >= 0)
    {
        gpio_reset_pin(reset);
        gpio_set_direction(reset, GPIO_MODE_OUTPUT);
        gpio_set_level(reset, 0);
        vTaskDelay(50 / portTICK_PERIOD_MS);
        gpio_set_level(reset, 1);
    }

    // 4. Lưu handle vào struct
    dev->_address = 0x3C;
    dev->_flip = false;
    dev->_i2c_bus_handle = i2c_bus_handle; // Lưu lại để dùng sau nếu cần
    dev->_i2c_dev_handle = i2c_dev_handle; // Quan trọng nhất
}

void i2c_init(SSD1306_t *dev, int width, int height)
{
    dev->_width = width;
    dev->_height = height;
    dev->_pages = height / 8;

    // Chuỗi lệnh khởi tạo CHUẨN cho SSD1306
    uint8_t out_buf[] = {
        OLED_CONTROL_BYTE_CMD_STREAM,
        OLED_CMD_DISPLAY_OFF,        // 0xAE
        OLED_CMD_SET_MUX_RATIO,      // 0xA8
        height - 1,                  // 0x3F (64-1)
        OLED_CMD_SET_DISPLAY_OFFSET, // 0xD3
        0x00,
        OLED_CMD_SET_DISPLAY_START_LINE, // 0x40
        OLED_CMD_SET_SEGMENT_REMAP_1,    // 0xA1
        OLED_CMD_SET_COM_SCAN_MODE,      // 0xC8
        OLED_CMD_SET_DISPLAY_CLK_DIV,    // 0xD5
        0x80,
        OLED_CMD_SET_COM_PIN_MAP, // 0xDA
        (height == 64) ? 0x12 : 0x02,
        OLED_CMD_SET_CONTRAST,      // 0x81
        0xFF,                       // Max contrast
        OLED_CMD_DISPLAY_RAM,       // 0xA4
        OLED_CMD_SET_VCOMH_DESELCT, // 0xDB
        0x40,
        OLED_CMD_SET_MEMORY_ADDR_MODE, // 0x20
        0x00,                          // Horizontal Addressing Mode
        OLED_CMD_SET_CHARGE_PUMP,      // 0x8D
        0x14,                          // Enable Charge Pump
        OLED_CMD_DISPLAY_NORMAL,       // 0xA6
        OLED_CMD_DISPLAY_ON            // 0xAF
    };

    // Gửi lệnh khởi tạo
    esp_err_t res = i2c_master_transmit(dev->_i2c_dev_handle, out_buf, sizeof(out_buf), I2C_TICKS_TO_WAIT);

    if (res == ESP_OK)
    {
        ESP_LOGI(TAG, "OLED init OK");
    }
    else
    {
        ESP_LOGE(TAG, "OLED init failed: %s", esp_err_to_name(res));
    }
}

void i2c_display_image(SSD1306_t *dev, int page, int seg, const uint8_t *images, int width)
{
    if (page >= dev->_pages)
        return;
    if (seg >= dev->_width)
        return;

    // Tính toán địa chỉ bộ nhớ (Horizontal Mode)
    // Nếu dùng Horizontal Mode (0x20, 0x00) ở hàm init, ta chỉ cần set lại vùng vẽ
    // Tuy nhiên để tương thích code cũ, ta set trỏ trang/cột thủ công

    uint8_t col_low = (seg & 0x0F);
    uint8_t col_high = ((seg >> 4) & 0x0F);

    uint8_t cmd_buf[] = {
        OLED_CONTROL_BYTE_CMD_STREAM,
        0x00 + col_low,  // Set Lower Column
        0x10 + col_high, // Set Higher Column
        0xB0 | page      // Set Page Start Address
    };

    // Gửi lệnh set vị trí
    i2c_master_transmit(dev->_i2c_dev_handle, cmd_buf, sizeof(cmd_buf), I2C_TICKS_TO_WAIT);

    // Gửi dữ liệu ảnh
    // Cần tạo buffer mới có byte đầu là DATA_STREAM (0x40)
    uint8_t *data_buf = malloc(width + 1);
    if (!data_buf)
        return;

    data_buf[0] = OLED_CONTROL_BYTE_DATA_STREAM; // 0x40
    memcpy(&data_buf[1], images, width);

    esp_err_t res = i2c_master_transmit(dev->_i2c_dev_handle, data_buf, width + 1, I2C_TICKS_TO_WAIT);
    free(data_buf);

    if (res != ESP_OK)
    {
        ESP_LOGE(TAG, "Write failed: %s", esp_err_to_name(res));
    }
}

void i2c_contrast(SSD1306_t *dev, int contrast)
{
    uint8_t cmd_buf[] = {
        OLED_CONTROL_BYTE_CMD_STREAM,
        OLED_CMD_SET_CONTRAST,
        contrast};
    i2c_master_transmit(dev->_i2c_dev_handle, cmd_buf, sizeof(cmd_buf), I2C_TICKS_TO_WAIT);
}

void i2c_hardware_scroll(SSD1306_t *dev, ssd1306_scroll_type_t scroll)
{
    // Để trống để tiết kiệm code nếu không dùng scroll
}