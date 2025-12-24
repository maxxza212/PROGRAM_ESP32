#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= WIFI =================
const char* ssid = "SirsSpot-RSML";
const char* password = "rsmljayasirs1";

// ================= SERVER =================
const char* serverUrl = "http://10.107.108.22:6003/api";

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= DHT =================
#define DHTTYPE DHT11
#define DHTPIN1 4
#define DHTPIN2 14

DHT dht1(DHTPIN1, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);

// ================= SENSOR ID =================
const int SENSOR_1_ID = 7;
const int SENSOR_2_ID = 8;

// ================= TIMING =================
const unsigned long DHT_DELAY_MS = 3000;   // aman tanpa resistor
const unsigned long LOOP_INTERVAL = 60000; // target 1 menit

unsigned long lastLoop = 0;

// ======================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 MONITOR - TANPA RESISTOR ===");

  // I2C
  Wire.begin(21, 22);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ESP32 MONITOR");
  lcd.setCursor(0, 1);
  lcd.print("PAVILIUN SHOFA");
  delay(2000);

  // DHT (TANPA RESISTOR -> PAKAI INTERNAL PULLUP)
  pinMode(DHTPIN1, INPUT_PULLUP);
  pinMode(DHTPIN2, INPUT_PULLUP);
  dht1.begin();
  dht2.begin();

  // WiFi
  connectWiFi();
}

// ======================================================
void loop() {
  if (millis() - lastLoop < LOOP_INTERVAL) return;
  lastLoop = millis();

  // Pastikan WiFi aktif (tapi jangan reconnect tepat sebelum baca DHT)
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // ===== BACA SENSOR (DENGAN RETRY) =====
  float t1, h1, t2, h2;
  bool ok1 = bacaDHTDenganRetry(dht1, t1, h1);
  bool ok2 = bacaDHTDenganRetry(dht2, t2, h2);

  // ===== TAMPIL LCD =====
  tampilLCD(ok1, t1, h1, ok2, t2, h2);

  // ===== KIRIM API (YANG VALID SAJA) =====
  if (ok1) {
    kirimData("suhu",
      "{\"nilai_suhu\":" + String(t1) + ",\"id_sensor\":" + String(SENSOR_1_ID) + "}");
    delay(1200);
    kirimData("kelembapan",
      "{\"nilai_kelembapan\":" + String(h1) + ",\"id_sensor\":" + String(SENSOR_1_ID) + "}");
  }

  delay(2000);

  if (ok2) {
    kirimData("suhu",
      "{\"nilai_suhu\":" + String(t2) + ",\"id_sensor\":" + String(SENSOR_2_ID) + "}");
    delay(1200);
    kirimData("kelembapan",
      "{\"nilai_kelembapan\":" + String(h2) + ",\"id_sensor\":" + String(SENSOR_2_ID) + "}");
  }

  Serial.println("⏳ Siklus selesai (tanpa resistor)");
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
// DHT (RETRY TANPA RESISTOR)
// ======================================================
bool bacaDHTDenganRetry(DHT& dht, float& t, float& h) {
  for (int i = 1; i <= 3; i++) {
    delay(DHT_DELAY_MS); // penting tanpa resistor
    h = dht.readHumidity();
    t = dht.readTemperature();

    if (!isnan(t) && !isnan(h)) {
      return true;
    }
    Serial.printf("Retry DHT (%d/3)\n", i);
  }
  return false;
}

// ======================================================
// LCD
// ======================================================
void tampilLCD(bool ok1, float t1, float h1,
               bool ok2, float t2, float h2) {
  lcd.clear();

  // ===== BARIS 1 : SENSOR 1 =====
  lcd.setCursor(0, 0);
  if (ok1) {
    lcd.print("S1:");
    lcd.print(t1, 1);
    lcd.print("C ");
    lcd.print("K1:");
    lcd.print(h1, 0);
    lcd.print("%");
  } else {
    lcd.print("S1: ERROR     ");
  }

  // ===== BARIS 2 : SENSOR 2 =====
  lcd.setCursor(0, 1);
  if (ok2) {
    lcd.print("S2:");
    lcd.print(t2, 1);
    lcd.print("C ");
    lcd.print("K2:");
    lcd.print(h2, 0);
    lcd.print("%");
  } else {
    lcd.print("S2: ERROR     ");
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
