#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// WiFi
const char* ssid = "SirsSpot-RSML";
const char* password = "rsmljayasirs1";

// Server
const char* serverUrl = "http://192.168.108.14:8000/api";

// DHT Sensor
#define DHTPIN1 4
#define DHTPIN2 14
#define DHTTYPE DHT11

DHT dht1(DHTPIN1, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);

// Config Device - ESP32 Board 1 di Ruang TI
const int SENSOR_1_ID = 1;  // Sensor di ESP32 Board 1
const int SENSOR_2_ID = 2;  // Sensor di ESP32 Board 1

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 Board 1 - Ruang TI ===");

  dht1.begin();
  dht2.begin();

  connectWiFi();
}

void loop() {
  // Pastikan WiFi aktif
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi terputus! Mencoba konek ulang...");
    connectWiFi();
  }

  Serial.println("\n--- Ruang TI ---");

  bacaDanKirimSensor(dht1, SENSOR_1_ID);
  delay(3000);
  bacaDanKirimSensor(dht2, SENSOR_2_ID);

  Serial.println("Tunggu 60 detik...");
  delay(60000);
}

// ======================================================
// FUNGSI
// ======================================================

void connectWiFi() {
  Serial.print("Menghubungkan ke WiFi ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n WiFi Tersambung!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n Gagal konek WiFi! Akan dicoba lagi nanti.");
  }
}

void bacaDanKirimSensor(DHT &dht, int sensorID) {
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();

  if (isnan(suhu) || isnan(kelembapan)) {
    Serial.println("Sensor gagal baca data!");
    return;
  }

  Serial.printf("Sensor %d → Temp: %.2f°C | Hum: %.2f%%\n", sensorID, suhu, kelembapan);

  // Kirim suhu
  if (kirimData("suhu", "{\"nilai_suhu\":" + String(suhu) + ",\"id_sensor\":" + String(sensorID) + "}")) {
    Serial.println("Suhu OK");
  } else {
    Serial.println("Gagal kirim suhu!");
  }

  delay(1500); // jeda antar request

  // Kirim kelembapan
  if (kirimData("kelembapan", "{\"nilai_kelembapan\":" + String(kelembapan) + ",\"id_sensor\":" + String(sensorID) + "}")) {
    Serial.println("Kelembapan OK");
  } else {
    Serial.println("Gagal kirim kelembapan!");
  }
}

bool kirimData(const String& endpoint, const String& payload) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi tidak terhubung, kirim dibatalkan!");
    return false;
  }

  HTTPClient http;
  http.begin(serverUrl + String("/") + endpoint);
  http.addHeader("Content-Type", "application/json");

  bool sukses = false;
  for (int percobaan = 1; percobaan <= 3; percobaan++) {
    int code = http.POST(payload);
    Serial.printf("[Laravel] Percobaan %d → Status: %d\n", percobaan, code);

    if (code == 201) {
      sukses = true;
      break;
    }

    delay(1000); // tunggu sebentar sebelum retry
  }

  http.end();
  return sukses;
}
