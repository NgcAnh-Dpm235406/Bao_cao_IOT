# 🌿 Hệ Thống Vườn Thông Minh (Smart Greenhouse)

Hệ thống giám sát và điều khiển nhà kính thông minh tích hợp IoT Đám mây (**ThingSpeak**) và giao thức điều khiển thời gian thực (**MQTT**) sử dụng vi điều khiển **ESP32 DevKit V1**. Dự án được xây dựng và mô phỏng trên nền tảng **Wokwi** và kiến trúc đa nhiệm **FreeRTOS**.

---

## 📌 1. Tính Cấp Thiết

Trong bối cảnh biến đổi khí hậu và thời tiết cực đoan, canh tác nông nghiệp truyền thống gặp nhiều thách thức. Sự phát triển của **Nông nghiệp 4.0 (Agriculture 4.0)** và **IoT** mở ra giải pháp giám sát vi khí hậu trực tiếp và tự động hóa quá trình chăm sóc cây trồng:
* Giám sát điều kiện sinh trưởng (nhiệt độ, độ ẩm không khí, độ ẩm đất, ánh sáng) theo thời gian thực từ xa.
* Tự động kích hoạt thiết bị chấp hành (bơm, quạt, đèn), tiết kiệm tài nguyên điện/nước và tối ưu năng suất.

---

## 🎯 2. Đặt Vấn Đề & Giải Pháp

### Bài toán
* **Tại chỗ (Edge):** Tự động phản hồi nhanh các thay đổi vi khí hậu dưới 1s và hiển thị thông số rõ ràng lên màn hình OLED.
* **Từ xa (Cloud & MQTT):** Đồng bộ dữ liệu lên mây ThingSpeak để theo dõi biểu đồ thời gian thực và cho phép điều khiển 2 chiều (Manual Override) từ xa qua giao thức MQTT với độ trễ cực thấp.

### Kiến trúc giải pháp
* **Vi điều khiển trung tâm:** ESP32 DevKit V1 (240MHz, tích hợp Wi-Fi/BLE, hỗ trợ FreeRTOS dual-core).
* **Đa nhiệm FreeRTOS:**
  * **Core 1:** Tập trung 100% tài nguyên xử lý lắng nghe sự kiện MQTT (độ trễ ~0ms) và quét cảm biến Analog mỗi 1 giây.
  * **Core 0:** Đảm nhận tác vụ đọc cảm biến DHT22 (mỗi 2.5s) và đẩy dữ liệu lên ThingSpeak Cloud (mỗi 25s), giúp việc đẩy dữ liệu Cloud không làm ảnh hưởng đến thời gian thực của các tác vụ khác.

---

## 🛠️ 3. Danh Mục Linh Kiện Phần Cứng

| STT | Tên thiết bị / Linh kiện | Thông số kỹ thuật / Ghi chú | SL | Đơn giá ước tính (VNĐ) |
|---|---|---|---|---|
| 1 | Bo mạch ESP32 DevKit V1 | 30 chân, tích hợp Wi-Fi/BLE | 1 | 75.000 |
| 2 | Cáp Micro-USB | Cáp sạc & truyền dữ liệu | 1 | 15.000 |
| 3 | Breadboard MB-102 | Bảng cắm thử nghiệm 830 lỗ | 1 | 30.000 |
| 4 | Bộ dây cắm Jumper Wires | 20cm, 40 sợi/bẹ | 2 | 36.000 |
| 5 | Module DHT11 / DHT22 | Cảm biến nhiệt độ & độ ẩm không khí | 1 | 20.000 |
| 6 | Cảm biến Độ ẩm đất | Module cảm biến độ ẩm đất Analog | 1 | 15.000 |
| 7 | Cảm biến Ánh sáng (LDR) | Module quang trở LDR | 1 | 10.000 |
| 8 | Màn hình OLED 0.96 inch | SSD1306 128x64 (I2C) | 1 | 55.000 |
| 9 | Module Relay 5V 2 kênh | Cách ly Opto, kích HIGH/LOW | 1 | 35.000 |
| 10 | Máy bơm nước mini 5V + Ống | Bơm chìm 5V DC | 1 | 18.000 |
| 11 | Quạt tản nhiệt 4010 5V | Quạt thông gió 4x4cm | 1 | 14.000 |
| 12 | Đèn LED sân vườn + Trở | LED 5mm + Điện trở 1kΩ | 1 | 4.000 |
| 13 | Pin AA | 3 viên pin AA mắc nối tiếp (4.5V) | 1 | 25.000 |
| **Tổng** | | | | **352.000 VNĐ** |

---

## 🔌 4. Sơ Đồ Đấu Nối Chân (Pinout)

| Linh kiện | Chân linh kiện | Chân kết nối trên ESP32 | Chức năng |
|---|---|---|---|
| **OLED SSD1306** | VCC / GND / SDA / SCL | 3.3V / GND / GPIO 21 / GPIO 22 | Hiển thị thông số vi khí hậu & trạng thái |
| **Cảm biến DHT** | VCC / GND / DATA | 3.3V / GND / GPIO 15 | Đọc nhiệt độ và độ ẩm không khí |
| **Cảm biến Đất** | VCC / GND / AO | 3.3V / GND / GPIO 34 | Đọc độ ẩm đất (Analog 0 - 4095) |
| **Cảm biến LDR** | VCC / GND / AO | 3.3V / GND / GPIO 35 | Đọc cường độ ánh sáng (Analog 0 - 4095) |
| **Relay 1 (Bơm)** | VCC / GND / IN1 | 5V / GND / GPIO 26 | Điều khiển máy bơm nước |
| **Relay 2 (Quạt)** | VCC / GND / IN2 | 5V / GND / GPIO 27 | Điều khiển quạt thông gió làm mát |
| **Đèn LED Vàng** | Anode (+) / Cathode (-) | GPIO 2 / GND | Chiếu sáng bổ sung khi trời tối |

---

## ⚙️ 5. Nguyên Lý Hoạt Động & Thuật Toán

### Chế độ Tự động (AUTO Mode)
Sử dụng thuật toán **Hysteresis** (vùng trễ) để tránh tình trạng bật/tắt thiết bị liên tục tại ngưỡng ranh giới:
* **Bơm nước:** Bật khi độ ẩm đất **< 30%**, Tắt khi độ ẩm đất **$\ge$ 45%**.
* **Quạt làm mát:** Bật khi nhiệt độ **$\ge$ 35.0°C**, Tắt khi nhiệt độ **$\le$ 33.5°C**.
* **Đèn sân vườn:** Bật khi giá trị LDR **> 2600**, Tắt khi LDR **$\le$ 1800**.

### Chế độ Điều khiển Thủ công (MANUAL Override via MQTT)
* Lắng nghe liên tục các topic MQTT:
  * `vuonthongminh/pump`
  * `vuonthongminh/fan`
  * `vuonthongminh/light`
* Khi nhận lệnh `1`/`ON` hoặc `0`/`OFF`, hệ thống chuyển ngay sang chế độ Manual và điều khiển Relay tức thì. Lệnh `AUTO` sẽ chuyển hệ thống về chế độ tự động theo cảm biến.

---

## 🔗 6. Liên Kết Trực Tiếp Dự Án

* 🎮 **Mô phỏng Wokwi:** [Link project Wokwi](https://wokwi.com/projects/478315169199685633)
* 📊 **Kênh ThingSpeak Cloud:** [ThingSpeak Channel 3542246](https://thingspeak.mathworks.com/channels/3542246)
* 🎬 **Video Demo Youtube:** [Xem Video Demo](https://youtu.be/Tg7j1kSKB4o?si=8WL4zY8FopbUQnaE)
* 🐙 **GitHub Repository:** [Bao_cao_IOT Repository](https://github.com/NgcAnh-Dpm235406/Bao_cao_IOT.git)

---

## 👥 7. Thông Tin Nhóm Thực Hiện

**Trường Đại Học An Giang - Khoa Công Nghệ Thông Tin**  
**Đồ án ngành Kỹ thuật Phần mềm**

* **Giảng viên hướng dẫn:** TS. Huỳnh Lý Thanh Nhàn
* **Thành viên thực hiện:**
  1. Nguyễn Thị Ngân Huệ - DPM235414
  2. Dương Thị Kim Anh – DPM235404
  3. Nguyễn Huỳnh Ngọc Anh - DPM235406
  4. Huỳnh Hiếu – DPM235419
