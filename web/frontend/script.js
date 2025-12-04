
// --- MQTT CONFIG (Đọc từ file config.js) ---
const broker = CONFIG.MQTT_BROKER_URL;
const options = {
    username: CONFIG.MQTT_USERNAME,
    password: CONFIG.MQTT_PASSWORD,
};
const client = mqtt.connect(broker, options);

const topicCmd = "home/relay1/cmd";
const topicStatus = "home/relay1/status";
const topicDistance = "home/relay1/distance";
const API_URL = CONFIG.API_URL;

// --- BIẾN TRẠNG THÁI ---
let turnOnDistance = null;
let turnOffDistance = null;
let currentDistance = null;
let lastKnownRelayState = null;

// --- DOM ELEMENTS ---
const statusSpan = document.getElementById("status");
const distanceValueSpan = document.getElementById("distance-value");
const turnOnInput = document.getElementById("turn-on-distance");
const turnOffInput = document.getElementById("turn-off-distance");
const turnOnDisplaySpan = document.getElementById("turn-on-display");
const turnOffDisplaySpan = document.getElementById("turn-off-display");
// Xóa biến checkbox vì không còn tồn tại
// const autoControlToggle = document.getElementById("auto-control-toggle");

// --- LOGIC MQTT ---
client.on("connect", () => {
    console.log("Connected to HiveMQ!");
    client.subscribe(topicStatus);
    client.subscribe(topicDistance);
    statusSpan.innerText = "Conntected, waiting for data...";
});

client.on("message", (topic, message) => {
    const msgString = message.toString();
    if (topic === topicStatus) {
        lastKnownRelayState = msgString;
        statusSpan.innerText = msgString;
    } else if (topic === topicDistance) {
        currentDistance = parseFloat(msgString);
        if (!isNaN(currentDistance)) {
            distanceValueSpan.innerText = currentDistance.toFixed(2);
            checkDistanceAndControlRelay();
        }
    }
});

// Hàm gửi lệnh MQTT đơn giản
function sendCmd(cmd) {
    client.publish(topicCmd, cmd);
}

// --- LOGIC ĐIỀU KHIỂN TỰ ĐỘNG ---

function setLimits() {
    const onValue = parseFloat(turnOnInput.value);
    const offValue = parseFloat(turnOffInput.value);

    if (isNaN(onValue) || isNaN(offValue)) {
        alert("Vui lòng nhập đủ cả hai ngưỡng.");
        return;
    }
    if (onValue >= offValue) {
        alert("Lỗi: Ngưỡng Bật phải bé hơn Ngưỡng Tắt.\n(Ví dụ: Bật khi <= 10cm, Tắt khi >= 80cm)");
        return;
    }

    turnOnDistance = onValue;
    turnOffDistance = offValue;

    turnOnDisplaySpan.innerText = `≤ ${turnOnDistance} cm`;
    turnOffDisplaySpan.innerText = `≥ ${turnOffDistance} cm`;
    console.log(`Đã kích hoạt chế độ tự động. Ngưỡng: Bật ≥ ${turnOnDistance}cm, Tắt ≤ ${turnOffDistance}cm`);

    checkDistanceAndControlRelay();
}
// hàm xóa ngưỡng
function clearLimits() {
    // 1. Đặt lại các biến ngưỡng
    turnOnDistance = null;
    turnOffDistance = null;

    // 2. Xóa giá trị trong các ô input
    turnOnInput.value = '';
    turnOffInput.value = '';

    // 3. Cập nhật lại hiển thị trạng thái ngưỡng
    turnOnDisplaySpan.innerText = 'Chưa đặt';
    turnOffDisplaySpan.innerText = 'Chưa đặt';

    // 4. In ra console để thông báo
    console.log("Đã xóa ngưỡng. Chế độ tự động đã bị vô hiệu hóa.");
}


function checkDistanceAndControlRelay() {
    if (turnOnDistance === null || turnOffDistance === null || currentDistance === null) {
        return;
    }

    let desiredState = lastKnownRelayState;

    if (currentDistance <= turnOnDistance) {
        desiredState = 'ON';
    } else if (currentDistance >= turnOffDistance) {
        desiredState = 'OFF';
    }

    if (desiredState !== lastKnownRelayState) {
        console.log(`Tự động: Khoảng cách ${currentDistance}cm. Ngưỡng [${turnOffDistance}, ${turnOnDistance}]. Đổi trạng thái relay thành ${desiredState}`);
        // Gửi lệnh mà không cần kiểm tra gì thêm
        originalSendCmd(desiredState);
    }
}


const originalSendCmd = sendCmd;




// --- SCHEDULER UI LOGIC ---

// Hàm lấy và hiển thị danh sách lịch
async function fetchSchedules() {
    try {
        const response = await fetch(`${API_URL}/schedules`);
        const schedules = await response.json();

        const list = document.getElementById('schedule-list');
        list.innerHTML = ''; // Xóa danh sách cũ

        schedules.forEach(sch => {
            const li = document.createElement('li');
            li.innerHTML = `
                <span>From <b>${sch.start_time}</b> to <b>${sch.end_time}</b></span>
                <button onclick="deleteSchedule(${sch.id})">Delete</button>
            `;
            list.appendChild(li);
        });
    } catch (error) {
        console.error("Failed to fetch schedules:", error);
    }
}

// Hàm thêm lịch mới
async function addSchedule(event) {
    event.preventDefault(); // Ngăn form submit theo cách truyền thống

    const startTime = document.getElementById('start_time').value;
    const endTime = document.getElementById('end_time').value;

    try {
        await fetch(`${API_URL}/schedules`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ start_time: startTime, end_time: endTime })
        });
        fetchSchedules(); // Tải lại danh sách sau khi thêm
    } catch (error) {
        console.error("Failed to add schedule:", error);
    }
}

// Hàm xóa một lịch
async function deleteSchedule(id) {
    if (!confirm('Are you sure you want to delete this schedule?')) return;

    try {
        await fetch(`${API_URL}/schedules/${id}`, { method: 'DELETE' });
        fetchSchedules(); // Tải lại danh sách
    } catch (error) {
        console.error("Failed to delete schedule:", error);
    }
}

// Tải danh sách lịch khi trang được mở lần đầu
document.addEventListener('DOMContentLoaded', fetchSchedules);