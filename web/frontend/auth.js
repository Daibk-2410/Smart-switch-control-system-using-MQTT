// File: frontend/auth.js
const API_URL = 'http://localhost:3000'; // Đảm bảo khớp với cổng backend

const loginForm = document.getElementById('login-form');
const registerForm = document.getElementById('register-form');

if (loginForm) {
    loginForm.addEventListener('submit', async (e) => {
        e.preventDefault();

        const email = document.getElementById('email').value;
        const password = document.getElementById('password').value;

        try {
            const res = await fetch(`${API_URL}/api/auth/login`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                // Gửi 'email' thay vì 'username'
                body: JSON.stringify({ email, password })
            });
            const data = await res.json();
            if (res.ok) {
                localStorage.setItem('token', data.token);
                window.location.href = 'control.html';
            } else {
                alert(data.message);
            }
        } catch (error) {
            alert('Lỗi kết nối đến máy chủ.');
        }
    });
}

if (registerForm) {
    registerForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        const username = document.getElementById('username').value;
        const email = document.getElementById('email').value;
        const password = document.getElementById('password').value;

        try {
            const res = await fetch(`${API_URL}/api/auth/register`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ username, email, password })
            });
            const data = await res.json();
            if (res.ok) {
                alert('Đăng ký thành công! Vui lòng đăng nhập.');
                window.location.href = 'login.html';
            } else {
                alert(data.message);
            }
        } catch (error) {
            alert('Lỗi kết nối đến máy chủ.');
        }
    });
}