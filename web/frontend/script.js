// --- MQTT CONFIG ---
const broker = "wss://09db723ea0574876a727418f489b0600.s1.eu.hivemq.cloud:8884/mqtt";
const options = {
    username: "relay1",
    password: "Dai24102004@#",
};
const client = mqtt.connect(broker, options);

const topicCmd = "home/relay1/cmd";
const topicStatus = "home/relay1/status";

// --- API CONFIG ---
const API_URL = "http://localhost:3000"; // Địa chỉ backend server của bạn

// --- MQTT LOGIC ---
client.on("connect", () => {    
    console.log("Connected to HiveMQ!");
    client.subscribe(topicStatus);
    document.getElementById("status").innerText = "Connected";
});

client.on("message", (topic, message) => {
    if (topic === topicStatus) {
        document.getElementById("status").innerText = message.toString();
    }
});

function sendCmd(cmd) {
    client.publish(topicCmd, cmd);
}

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