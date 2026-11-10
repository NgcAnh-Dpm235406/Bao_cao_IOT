#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include "ThingSpeak.h"
#include <PubSubClient.h>

// 1. Cấu hình Wi-Fi 
const char* ssid = "AGU";
const char* password = "";

// 2. Thông tin kênh ThingSpeak
unsigned long myChannelNumber = 3542246;
const char* myWriteAPIKey = "E2C1MWBLWUUUZKFO";

// 3. Cấu hình MQTT
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;

const char* topic_pump  = "vuonthongminh/pump";
const char* topic_fan   = "vuonthongminh/fan";
const char* topic_light = "vuonthongminh/light";

// 4. Khai báo chân cảm biến & thiết bị
#define DHT_PIN     15   // Chân OUT cảm biến DHT11
#define DHT_TYPE    DHT11
#define SOIL_PIN    34   // Chân AO cảm biến độ ẩm đất
#define LDR_PIN     35   // Chân AO cảm biến ánh sáng
#define PUMP_PIN    26   // Chân IN1 Relay (Máy bơm)
#define FAN_PIN     27   // Chân IN2 Relay (Quạt)
#define LIGHT_PIN   2    // Chân Đèn LED vàng

// Mức kích Relay
#define RELAY_ON    LOW
#define RELAY_OFF   HIGH

Adafruit_SSD1306 display(128, 64, &Wire, -1);
DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient tsClient;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

int pumpStatus = 0;
int fanStatus = 0;
int lightStatus = 0;

bool manualPump = false;
bool manualFan = false;
bool manualLight = false;

bool lastSoilDry = false;
bool lastTempHigh = false;
bool lastLightDark = false;
bool firstRun = true;
bool oledOK = false;
volatile bool needOledUpdate = false;

unsigned long lastSensorRead = 0;
unsigned long lastMqttRetry = 0;

float currentTemp = 28.5;
float currentHumi = 65.0;
int currentSoilPercent = 100;
int currentLdrRaw = 0;

void updateOLED() {
  if (!oledOK) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("SMART GREENHOUSE IOT");
  display.drawLine(0, 10, 128, 10, WHITE);

  display.setCursor(0, 15);
  display.print("Temp: "); display.print(currentTemp, 1); display.println(" C");
  
  display.setCursor(0, 27);
  display.print("Air Humi: "); display.print(currentHumi, 1); display.println(" %");
  
  display.setCursor(0, 39);
  display.print("Soil Humi: "); display.print(currentSoilPercent); display.println(" %");

  display.setCursor(0, 51);
  display.print("P:"); display.print(pumpStatus);
  display.print(" F:"); display.print(fanStatus);
  display.print(" L:"); display.print(lightStatus);
  display.display();
}

void applyAutoControl(bool forceUpdate) {
  bool isSoilDry   = lastSoilDry   ? (currentSoilPercent < 45) : (currentSoilPercent < 30);
  bool isTempHigh  = lastTempHigh  ? (currentTemp > 33.5)      : (currentTemp > 35.0); 
  bool isLightDark = lastLightDark ? (currentLdrRaw > 1800)    : (currentLdrRaw > 2600);

  if (!manualPump && (forceUpdate || isSoilDry != lastSoilDry)) {
    pumpStatus = isSoilDry ? 1 : 0;
    digitalWrite(PUMP_PIN, pumpStatus ? RELAY_ON : RELAY_OFF);
    lastSoilDry = isSoilDry;
  }

  if (!manualFan && (forceUpdate || isTempHigh != lastTempHigh)) {
    fanStatus = isTempHigh ? 1 : 0;
    digitalWrite(FAN_PIN, fanStatus ? RELAY_ON : RELAY_OFF);
    lastTempHigh = isTempHigh;
  }

  if (!manualLight && (forceUpdate || isLightDark != lastLightDark)) {
    lightStatus = isLightDark ? 1 : 0;
    digitalWrite(LIGHT_PIN, lightStatus ? HIGH : LOW);
    lastLightDark = isLightDark;
  }
}

// HÀM NHẬN MQTT: Bấm nút là nhảy chân Relay ngay lập tức ở dòng đầu tiên!
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();

  String t = String(topic);
  t.toLowerCase();

  if (message.equalsIgnoreCase("AUTO")) {
    if (t == "vuonthongminh/pump") manualPump = false;
    else if (t == "vuonthongminh/fan") manualFan = false;
    else if (t == "vuonthongminh/light") manualLight = false;
    applyAutoControl(true);
    needOledUpdate = true;
    return;
  }

  bool isOn = (message == "1" || message.equalsIgnoreCase("ON") || message.equalsIgnoreCase("true"));

  // Kích hoạt Relay NGAY LẬP TỨC trước khi in Serial hay vẽ màn hình OLED
  if (t == "vuonthongminh/pump") {
    manualPump = true;
    pumpStatus = isOn ? 1 : 0;
    digitalWrite(PUMP_PIN, pumpStatus ? RELAY_ON : RELAY_OFF);
  } 
  else if (t == "vuonthongminh/fan") {
    manualFan = true;
    fanStatus = isOn ? 1 : 0;
    digitalWrite(FAN_PIN, fanStatus ? RELAY_ON : RELAY_OFF);
  } 
  else if (t == "vuonthongminh/light") {
    manualLight = true;
    lightStatus = isOn ? 1 : 0;
    digitalWrite(LIGHT_PIN, lightStatus ? HIGH : LOW);
  }

  Serial.printf("Nhan MQTT [%s]: %s -> Da kich Relay ngay!\n", topic, message.c_str());
  needOledUpdate = true;
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); // Cực kỳ quan trọng: Tắt ngủ Wi-Fi để nhận gói tin 0ms
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);

  Serial.print("Dang ket noi WiFi: ");
  Serial.print(ssid);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  Serial.println();
}

void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED || mqttClient.connected()) return;

  Serial.print("Dang ket noi MQTT...");
  String clientId = "ESP32_FastIoT_" + String(random(0xffff), HEX);
  if (mqttClient.connect(clientId.c_str())) {
    Serial.println("Thanh cong!");
    mqttClient.subscribe(topic_pump, 0);  // QoS 0 nhanh nhất
    mqttClient.subscribe(topic_fan, 0);
    mqttClient.subscribe(topic_light, 0);
  } else {
    Serial.print("Loi MQTT: ");
    Serial.println(mqttClient.state());
  }
}

// TÁC VỤ CHẠY NGẦM TRÊN CORE 0: Chuyên đọc DHT11 và gửi ThingSpeak
// Dù ThingSpeak hay DHT11 có chậm mấy giây cũng KHÔNG BAO GIỜ làm lag MQTT ở Core 1
void backgroundTask(void * parameter) {
  unsigned long lastTS = 0;
  for (;;) {
    // 1. Đọc DHT11 mỗi 2.5 giây
    float humi = dht.readHumidity();
    float temp = dht.readTemperature();
    if (!isnan(humi) && !isnan(temp)) {
      currentTemp = temp;
      currentHumi = humi;
    }

    // 2. Gửi ThingSpeak mỗi 25 giây
    if (WiFi.status() == WL_CONNECTED && (millis() - lastTS > 25000)) {
      lastTS = millis();
      ThingSpeak.setField(1, currentTemp);
      ThingSpeak.setField(2, currentHumi);
      ThingSpeak.setField(3, currentSoilPercent);
      ThingSpeak.setField(4, lightStatus);
      ThingSpeak.setField(5, pumpStatus);
      ThingSpeak.setField(6, fanStatus);
      ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    }

    vTaskDelay(2500 / portTICK_PERIOD_MS); // Nghỉ 2.5s trên luồng phụ
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(LIGHT_PIN, OUTPUT);
  
  digitalWrite(PUMP_PIN, RELAY_OFF);
  digitalWrite(FAN_PIN, RELAY_OFF);
  digitalWrite(LIGHT_PIN, LOW);
  
  dht.begin();
  tsClient.setTimeout(2000);
  ThingSpeak.begin(tsClient);

  Wire.begin(21, 22);
  Wire.setClock(400000); // Tăng tốc độ vẽ màn hình OLED lên gấp 4 lần (Fast I2C)
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C) || display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    oledOK = true;
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 15);
    display.println("Khoi dong he thong...");
    display.display();
  }

  initWiFi();

  espClient.setNoDelay(true); // Tắt thuật toán gom gói tin TCP -> Nhận lệnh tức thì
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(30);
  mqttClient.setSocketTimeout(5);
  connectMQTT();

  updateOLED();

  // Khởi chạy luồng phụ ở Core 0 cho DHT11 và ThingSpeak
  xTaskCreatePinnedToCore(backgroundTask, "BG_Task", 8192, NULL, 1, NULL, 0);
}

void loop() {
  // CORE 1 CHỈ LÀM DUY NHẤT VIỆC LẮNG NGHE MQTT -> PHẢN HỒI LẬP TỨC 100%
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      if (millis() - lastMqttRetry >= 3000) {
        lastMqttRetry = millis();
        connectMQTT();
      }
    } else {
      mqttClient.loop(); 
    }
  }

  // Cập nhật OLED ngay khi có lệnh bấm nút
  if (needOledUpdate) {
    needOledUpdate = false;
    updateOLED();
  }

  // Đọc Analog (Đất & Ánh sáng chỉ mất 0.001 giây) mỗi 1 giây
  if (millis() - lastSensorRead >= 1000) {
    lastSensorRead = millis();

    int soilRaw = analogRead(SOIL_PIN);
    currentSoilPercent = constrain(map(soilRaw, 0, 4095, 100, 0), 0, 100);

    long ldrSum = 0;
    for (int i = 0; i < 5; i++) ldrSum += analogRead(LDR_PIN);
    currentLdrRaw = ldrSum / 5;

    if (firstRun) {
      applyAutoControl(true);
      firstRun = false;
    } else {
      applyAutoControl(false);
    }
    updateOLED();
  }
}