# Hệ thống nhận dạng ngọn lửa và cảnh báo mức độ đám cháy  
*(Flame Detection & Fire Severity Alert System – **DATH_Hồ Lê Duy Linh**)*

Hệ thống kết hợp **phần cứng, phần mềm nhúng, trí tuệ nhân tạo và giao diện web** nhằm *phát hiện sớm các nguy cơ hỏa hoạn* và *cảnh báo đa kênh* (còi, SMS GSM, MQTT, dashboard thời gian thực).

> Sinh viên thực hiện: **Hồ Lê Duy Linh – Đại học Tôn Đức Thắng**

---

## 1 · Cấu trúc thư mục

```text
.
├─ resource/
│   ├─ data/                # File nhị phân cấu hình (số ĐT, ngưỡng cảnh báo)
│   ├─ docs/                # Tài liệu thiết kế, sơ đồ mạch, báo cáo…
│   └─ model/               # Trọng số và notebook AI (YOLOv8-LSTM)
└─ source/
    ├─ esp32/               # Firmware PlatformIO cho ESP32
    │   ├─ include/
    │   ├─ lib/
    │   ├─ src/
    │   └─ platformio.ini
    ├─ raspberry/           # Edge-AI & giao tiếp Zigbee trên Raspberry Pi
    └─ web/                 # Front-end tĩnh (HTML/CSS/JS)
```

---

## 2 · Firmware ESP32

1. Cài **VS Code** và extension **PlatformIO IDE**.
2. Mở thư mục `source/esp32`.
3. Kết nối bo mạch **ESP32** qua USB, chọn đúng cổng COM.
4. Trong *PlatformIO*:  
   • **Build** `Ctrl + Alt + B` – biên dịch  
   • **Upload** `Ctrl + Alt + U` – nạp firmware  
   • **Monitor** `Ctrl + Alt + M` – theo dõi log nối tiếp (baud 115200)

### 2.1 · Điều chỉnh tham số

| File | Mô tả |
|------|-------|
| `resource/data/warning_value.bin` | Ngưỡng cảnh báo cho từng cảm biến |
| `resource/data/phone_number.bin`  | Danh sách số điện thoại nhận SMS |

Thay đổi giá trị bằng cách ghi lại tệp nhị phân thông qua **LittleFS Upload** hoặc chỉnh sửa hằng số trong mã nguồn rồi *Build & Upload* lại.

---

## 3 · Edge AI trên Raspberry Pi

Thư mục `source/raspberry` chứa:

- `merged_fire_system.py` – script chính: nhận dữ liệu cảm biến & video → suy luận mô hình YOLOv8-LSTM → gửi cảnh báo MQTT/Zigbee.
- `test_zigbee.py` – tiện ích kiểm tra kết nối Zigbee.
- `full-yolov8n-cls-lstm.ipynb` – notebook huấn luyện (nằm trong `resource/model`).

### 3.1 · Thiết lập nhanh

```bash
# Trên Raspberry Pi (Python >= 3.9)
python -m venv venv && source venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt   # Tạo file yêu cầu theo notebook/model
python merged_fire_system.py
```

---

## 4 · Web Dashboard

Thư mục `source/web` chứa front-end tĩnh. Chạy nhanh:

```bash
cd source/web
# Cách 1: Mở trực tiếp (Windows)
start index.html
# Cách 2: Serve cục bộ
python -m http.server 8080
```

Truy cập `http://localhost:8080` để xem dashboard (Realtime trạng thái cảm biến, log cảnh báo…).

---

## 5 · Luồng dữ liệu hệ thống

```mermaid
graph TD
    ESP32[(Cảm biến\nESP32)] -- MQTT/Zigbee --> RPi["Raspberry Pi\nEdge AI"]
    RPi -- MQTT/WebSocket --> Web["Web Dashboard"]
    ESP32 -- GSM/SMS --> Người_dùng["Người dùng"]
    ESP32 -- Còi/Buzzer --> Coi[Báo động cục bộ]
```

---

## 6 · Đóng góp & Giấy phép

Dự án phục vụ mục đích học tập và nghiên cứu. Vui lòng trích dẫn khi sử dụng. Pull request & Issue luôn được hoan nghênh!
