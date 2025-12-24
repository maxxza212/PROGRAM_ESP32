#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= WIFI =================
const char* ssid = "SirsSpot-RSML";
const char* password = "rsmljayasirs1";

// ================= SERVER =================
const char* serverUrl = "http://10.107.108.22:6003/api";

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= DS18B20 =================
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ================= SENSOR ID =================
const int SENSOR_ID = 11;

// ================= TIMING =================
const unsigned long LOOP_INTERVAL = 60000; // 1 menit
unsigned long lastLoop = 0;

// ======================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 MONITOR DS18B20 ===");

  // I2C
  Wire.begin(21, 22);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ESP32 MONITOR");
  lcd.setCursor(0, 1);
  lcd.print("DS18B20 MODE");
  delay(2000);

  // DS18B20
  sensors.begin();

  // WiFi
  connectWiFi();
}

// ======================================================
void loop() {
  if (millis() - lastLoop < LOOP_INTERVAL) return;
  lastLoop = millis();

  // Pastikan WiFi aktif
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // ===== BACA SENSOR =====
  sensors.requestTemperatures();
  float suhu = sensors.getTempCByIndex(0);

  bool sensorOK = (suhu != DEVICE_DISCONNECTED_C);

  // ===== TAMPIL LCD =====
  tampilLCD(sensorOK, suhu);

  // ===== KIRIM API =====
  if (sensorOK) {
    kirimData("suhu",
      "{\"nilai_suhu\":" + String(suhu) +
      ",\"id_sensor\":" + String(SENSOR_ID) + "}");
  }

  Serial.println("Siklus selesai (DS18B20)");
}

// ======================================================
// WIFI
// ======================================================
void connectWiFi() {
  Serial.print("Menghubungkan WiFi ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi OK | IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi gagal");
  }
}

// ======================================================
// LCD
// ======================================================
void tampilLCD(bool ok, float suhu) {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("SENSOR SUHU");

  lcd.setCursor(0, 1);
  if (ok) {
    lcd.print("T:");
    lcd.print(suhu, 1);
    lcd.print(" C");
  } else {
    lcd.print("ERROR SENSOR");
  }
}

// ======================================================
// API
// ======================================================
bool kirimData(const String& endpoint, const String& payload) {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.begin(String(serverUrl) + "/" + endpoint);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST(payload);
  Serial.printf("POST %s -> %d\n", endpoint.c_str(), code);

  http.end();
  return (code == 200 || code == 201);
}
