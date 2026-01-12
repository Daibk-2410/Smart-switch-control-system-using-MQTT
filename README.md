# Hệ thống Giám sát Mực nước và Điều khiển Relay sử dụng MQTT

Đây là một dự án IoT hoàn chỉnh, xây dựng một hệ thống thông minh cho phép giám sát mực nước và điều khiển một công tắc (relay) từ xa thông qua giao thức MQTT. Hệ thống được xây dựng trên nền tảng vi điều khiển ESP32, kết hợp với giao diện web hiện đại để điều khiển và giám sát.

| Supported Targets | ESP32 |
| ----------------- | ----- |

## Tính năng chính

Hệ thống cung cấp một bộ tính năng đa dạng và linh hoạt, đáp ứng nhiều kịch bản sử dụng thực tế:

-   **Giám sát thời gian thực:** Liên tục đo khoảng cách (đại diện cho mực nước) bằng cảm biến siêu âm và hiển thị trực tiếp trên giao diện web.
-   **Hiển thị cục bộ:** Tích hợp màn hình OLED để hiển thị trạng thái của relay và thời gian hẹn giờ còn lại ngay trên thiết bị.
-   **Điều khiển đa phương thức:**
    -   **Thủ công:** Bật/tắt relay ngay lập tức thông qua các nút bấm trên giao diện web hoặc các nút bấm vật lý kết nối trực tiếp với thiết bị.
    -   **Hẹn giờ theo lịch trình:** Thêm/xóa các lịch bật/tắt relay theo ngày, tháng, năm, giờ, phút cụ thể.
    -   **Hẹn giờ nhanh:** Sử dụng một nút bấm vật lý, nhấn `N` lần để hẹn giờ cho relay chạy trong `N * 10` phút.
-   **Chế độ tự động thông minh:** Tự động bật/tắt máy bơm (relay) dựa trên hai ngưỡng mực nước (ngưỡng cạn và ngưỡng đầy) do người dùng cài đặt, phù hợp cho các ứng dụng bơm nước tự động.
-   **Xác thực người dùng:** Giao diện điều khiển được bảo vệ bằng hệ thống Đăng nhập/Đăng ký, đảm bảo chỉ những người dùng được cấp phép mới có thể truy cập và điều khiển hệ thống.

## Cấu trúc Hệ thống

Dự án được xây dựng dựa trên kiến trúc 3 thành phần chính:

1.  **Firmware (ESP32):** Viết bằng ngôn ngữ C, sử dụng framework ESP-IDF và hệ điều hành thời gian thực FreeRTOS. Chịu trách nhiệm đọc dữ liệu từ cảm biến, điều khiển phần cứng (relay, OLED, nút bấm) và giao tiếp với MQTT Broker.
2.  **Backend Server (Node.js/Express.js):** Xây dựng các API để quản lý người dùng (đăng ký, đăng nhập) và xử lý logic hẹn giờ theo lịch trình. Server kết nối với cơ sở dữ liệu MySQL để lưu trữ thông tin.
3.  **Frontend (HTML/CSS/JS):** Giao diện người dùng trên nền tảng web, cho phép giám sát và tương tác với hệ thống theo thời gian thực thông qua kết nối MQTT (qua WebSocket) và gọi các API của backend.

## Yêu cầu Phần cứng

*   Một bo mạch phát triển ESP32 (ví dụ: ESP32-DevKitC).
*   Module Relay 1 kênh (5V).
*   Cảm biến siêu âm (HC-SR04 hoặc tương tự).
*   Màn hình OLED 0.96 inch giao tiếp I2C (SSD1306).
*   3 nút bấm (push button).
*   Breadboard và dây cắm.
*   Cáp USB-C hoặc Micro-USB để cấp nguồn và nạp code.

## Hướng dẫn Cài đặt và Sử dụng

### 1. Cài đặt Firmware (ESP32)

1.  **Cài đặt ESP-IDF:** Làm theo hướng dẫn tại [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) của Espressif.
2.  **Clone dự án:** `git clone https://github.com/Daibk-2410/Smart-switch-control-system-using-MQTT.git`
3.  **Cấu hình:**
    *   Trong thư mục `main`, tạo file `wifi_credentials.h` (file này không được commit lên Git) với nội dung:
        ```c
        #define WIFI_SSID "TEN_WIFI_CUA_BAN"
        #define WIFI_PASS "MAT_KHAU_WIFI"
        ```
    *   Mở file `main/mqtt_manager.c` và cập nhật thông tin MQTT Broker của bạn (nếu có thay đổi).
4.  **Build và Nạp code:**
    *   Mở terminal ESP-IDF.
    *   `cd` đến thư mục dự án.
    *   Chạy `idf.py -p YOUR_PORT flash monitor` để build, nạp code và theo dõi log.

### 2. Cài đặt Backend Server

1.  **Di chuyển đến thư mục backend:** `cd web/backend-server`
2.  **Cài đặt các thư viện:** `npm install`
3.  **Cấu hình môi trường:**
    *   Tạo file `.env` 
    *   Điền các thông tin cần thiết: thông tin kết nối database MySQL, thông tin MQTT Broker, và `JWT_SECRET`.
4.  **Khởi động server:** `node server.js`
    *   Server sẽ chạy tại `http://localhost:3001` (hoặc cổng bạn đã cấu hình).

### 3. Chạy Frontend

1.  **Cấu hình:**
    *   Trong thư mục `web/frontend`, mở file `config.js`.
    *   Đảm bảo `API_URL` và thông tin MQTT trỏ đến đúng địa chỉ.
2.  **Mở file `login.html`** trực tiếp bằng trình duyệt web.
3.  Đăng ký tài khoản mới và đăng nhập để bắt đầu sử dụng.
