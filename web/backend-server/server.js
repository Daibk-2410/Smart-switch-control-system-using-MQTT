require('dotenv').config();

const express = require('express');
const mysql = require('mysql2/promise');
const mqtt = require('mqtt');
const cors = require('cors');
const bcrypt = require('bcryptjs');
const jwt = require('jsonwebtoken');

// --- CẤU HÌNH (Đọc từ file .env) ---
const MYSQL_CONFIG = {
    host: process.env.DB_HOST,
    user: process.env.DB_USER,
    password: process.env.DB_PASSWORD,
    database: process.env.DB_NAME
};

const MQTT_BROKER = process.env.MQTT_BROKER_URL;
const MQTT_OPTIONS = {
    username: process.env.MQTT_USERNAME,
    password: process.env.MQTT_PASSWORD
};
const MQTT_TOPIC_CMD = "home/relay1/cmd";

const PORT = 3000; // Cổng backend server sẽ chạy
const JWT_SECRET = process.env.JWT_SECRET;

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

// Đăng ký
app.post('/api/auth/register', async (req, res) => {
    try {
        const { username, email, password } = req.body;
        if (!username || !email || !password) {
            return res.status(400).json({ message: 'Vui lòng điền đầy đủ thông tin.' });
        }

        // Kiểm tra xem username hoặc email đã tồn tại chưa
        const [existingUser] = await dbConnection.query('SELECT * FROM users WHERE username = ? OR email = ?', [username, email]);
        if (existingUser.length > 0) {
            return res.status(409).json({ message: 'Tên người dùng hoặc email đã tồn tại.' });
        }

        // Băm mật khẩu
        const hashedPassword = await bcrypt.hash(password, 10);

        // Lưu người dùng vào database
        await dbConnection.execute(
            'INSERT INTO users (username, email, password) VALUES (?, ?, ?)',
            [username, email, hashedPassword]
        );

        res.status(201).json({ message: 'Đăng ký thành công!' });
    } catch (error) {
        console.error("Lỗi đăng ký:", error);
        res.status(500).json({ message: 'Lỗi máy chủ.' });
    }
});

// API Đăng nhập
app.post('/api/auth/login', async (req, res) => {
    try {
        // Thay đổi: Nhận 'email' thay vì 'username' từ body
        const { email, password } = req.body;
        if (!email || !password) {
            return res.status(400).json({ message: 'Vui lòng điền email và mật khẩu.' });
        }

        // Tìm người dùng trong database bằng EMAIL
        const [users] = await dbConnection.query('SELECT * FROM users WHERE email = ?', [email]);
        if (users.length === 0) {
            // Giữ thông báo chung chung để bảo mật
            return res.status(401).json({ message: 'Email hoặc mật khẩu không đúng.' });
        }
        const user = users[0];

        // So sánh mật khẩu (không đổi)
        const isMatch = await bcrypt.compare(password, user.password);
        if (!isMatch) {
            return res.status(401).json({ message: 'Email hoặc mật khẩu không đúng.' });
        }

        // Tạo JWT (không đổi)
        const token = jwt.sign(
            { id: user.id, username: user.username }, // Vẫn giữ username trong token
            JWT_SECRET,
            { expiresIn: '1d' }
        );

        res.json({ message: 'Đăng nhập thành công!', token });

    } catch (error) {
        console.error("Lỗi đăng nhập:", error);
        res.status(500).json({ message: 'Lỗi máy chủ.' });
    }
});
// === MIDDLEWARE BẢO VỆ ===
const protect = (req, res, next) => {
    let token;
    if (req.headers.authorization && req.headers.authorization.startsWith('Bearer')) {
        try {
            token = req.headers.authorization.split(' ')[1];
            const decoded = jwt.verify(token, JWT_SECRET);
            // Gắn thông tin người dùng vào request để các hàm sau có thể dùng
            req.user = decoded;
            next(); // Cho phép đi tiếp
        } catch (error) {
            res.status(401).json({ message: 'Token không hợp lệ, truy cập bị từ chối.' });
        }
    }
    if (!token) {
        res.status(401).json({ message: 'Không có token, truy cập bị từ chối.' });
    }
};

// API lấy thông tin người dùng
app.get('/api/auth/me', protect, async (req, res) => {
    if (req.user) {
        res.json({
            id: req.user.id,
            username: req.user.username
        });
    } else {
        res.status(404).json({ message: 'Không tìm thấy người dùng.' });
    }
});

// API để lấy tất cả lịch trình
app.get('/api/schedules', protect, async (req, res) => {
    try {
        const [rows] = await dbConnection.query(
            "SELECT id, DATE_FORMAT(start_time, '%Y-%m-%dT%H:%i:%s') as start_time, DATE_FORMAT(end_time, '%Y-%m-%dT%H:%i:%s') as end_time, is_enabled FROM schedules ORDER BY start_time"
        );
        res.json(rows);
    } catch (error) {
        console.error("Lỗi lấy danh sách lịch trình:", error);
        res.status(500).json({ message: 'Lỗi máy chủ khi lấy danh sách lịch trình.' });
    }
});


// API để thêm một lịch trình mới
app.post('/api/schedules', protect, async (req, res) => {
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
app.delete('/api/schedules/:id', protect, async (req, res) => {
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
        // Lấy thời gian hiện tại của server
        const now = new Date();

        // Truy vấn để tìm bất kỳ lịch trình nào đang hoạt động
        // Tức là thời gian hiện tại nằm giữa start_time và end_time
        const [activeSchedules] = await dbConnection.query(
            'SELECT * FROM schedules WHERE is_enabled = TRUE AND ? BETWEEN start_time AND end_time',
            [now]
        );

        // Nếu có ít nhất một lịch trình đang hoạt động
        const relayShouldBeOn = activeSchedules.length > 0;

        const desiredState = relayShouldBeOn ? 'ON' : 'OFF';

        if (desiredState !== lastStateByScheduler) {
            console.log(`⏰ Schedule check: An active schedule was found. Desired state: ${desiredState}. Sending command...`);
            mqttClient.publish(MQTT_TOPIC_CMD, desiredState);
            lastStateByScheduler = desiredState;
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