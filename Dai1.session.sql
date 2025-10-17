USE iot_project;

CREATE TABLE schedules (
    id INT AUTO_INCREMENT PRIMARY KEY,
    start_time TIME NOT NULL,
        end_time TIME NOT NULL,
        is_enabled BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
