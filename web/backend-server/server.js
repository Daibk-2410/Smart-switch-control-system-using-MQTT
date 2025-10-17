// ===== server.js =====

const express = require('express');
const mysql = require('mysql2/promise');
const mqtt = require('mqtt');
const cors = require('cors');

// --- CẤU HÌNH ---
const MYSQL_CONFIG = {
    host: 'localhost',      // Địa chỉ MySQL server
    user: 'root',           // Username MySQL
    password: 'Dai24102004@#', // Password MySQL
    database: 'iot_project' // Tên database
};

const MQTT_BROKER = "wss://09db723ea0574876a727418f489b0600.s1.eu.hivemq.cloud:8884/mqtt";
const MQTT_OPTIONS = {
    username: "relay1",     // Dùng username/password giống ESP32 hoặc tạo user mới
    password: "Dai24102004@#"
};
const MQTT_TOPIC_CMD = "home/relay1/cmd";

const PORT = 3000; // Cổng backend server sẽ chạy

// --- KHỞI TẠO ---
const app = express();
app.use(cors()); // Cho phép cross-origin requests
app.use(express.json()); // Middleware để đọc JSON body

let dbConnection;
const mqttClient = mqtt.connect(MQTT_BROKER, MQTT_OPTIONS);

mqttClient.on('connect', () => {
    console.log('✅ Connected to MQTT Broker');
});

// Biến để theo dõi trạng thái relay do lịch trình điều khiển
// Giúp tránh gửi lệnh lặp đi lặp lại
let lastStateByScheduler = null; 

// --- API ENDPOINTS ---

// API để lấy tất cả lịch trình
app.get('/schedules', async (req, res) => {
    try {
        const [rows] = await dbConnection.query('SELECT id, TIME_FORMAT(start_time, "%H:%i") as start_time, TIME_FORMAT(end_time, "%H:%i") as end_time, is_enabled FROM schedules ORDER BY start_time');
        res.json(rows);
    } catch (error) {
        res.status(500).json({ message: 'Error fetching schedules', error });
    }
});

// API để thêm một lịch trình mới
app.post('/schedules', async (req, res) => {
    try {
        const { start_time, end_time } = req.body;
        if (!start_time || !end_time) {
            return res.status(400).json({ message: 'Start time and end time are required' });
        }
        const [result] = await dbConnection.execute(
            'INSERT INTO schedules (start_time, end_time) VALUES (?, ?)',
            [start_time, end_time]
        );
        res.status(201).json({ id: result.insertId, start_time, end_time });
    } catch (error) {
        res.status(500).json({ message: 'Error adding schedule', error });
    }
});

// API để xóa một lịch trình
app.delete('/schedules/:id', async (req, res) => {
    try {
        const { id } = req.params;
        await dbConnection.execute('DELETE FROM schedules WHERE id = ?', [id]);
        res.status(204).send(); // 204 No Content
    } catch (error) {
        res.status(500).json({ message: 'Error deleting schedule', error });
    }
});

// --- LOGIC KIỂM TRA LỊCH TRÌNH ---
async function checkSchedules() {
    try {
        const [schedules] = await dbConnection.query('SELECT start_time, end_time FROM schedules WHERE is_enabled = TRUE');
        
        const now = new Date();
        const currentTime = now.toTimeString().slice(0, 8); // Format HH:MM:SS

        let relayShouldBeOn = false;
        for (const schedule of schedules) {
            if (currentTime >= schedule.start_time && currentTime < schedule.end_time) {
                relayShouldBeOn = true;
                break;
            }
        }

        const desiredState = relayShouldBeOn ? 'ON' : 'OFF';
        
        // Chỉ gửi lệnh nếu trạng thái mong muốn khác với trạng thái cuối cùng
        if (desiredState !== lastStateByScheduler) {
            console.log(`⏰ Time: ${currentTime}, Desired state: ${desiredState}. Sending command...`);
            mqttClient.publish(MQTT_TOPIC_CMD, desiredState);
            lastStateByScheduler = desiredState; // Cập nhật trạng thái
        }

    } catch (error) {
        console.error("❌ Error checking schedules:", error);
    }
}


// --- HÀM MAIN ĐỂ KHỞI ĐỘNG SERVER ---
async function main() {
    try {
        dbConnection = await mysql.createConnection(MYSQL_CONFIG);
        console.log('✅ Connected to MySQL Database');

        // Chạy hàm kiểm tra lịch trình mỗi 5 giây
        setInterval(checkSchedules, 5000);

        app.listen(PORT, () => {
            console.log(`🚀 Server is running on http://localhost:${PORT}`);
        });
    } catch (error) {
        console.error('❌ Failed to start server:', error);
    }
}

main();