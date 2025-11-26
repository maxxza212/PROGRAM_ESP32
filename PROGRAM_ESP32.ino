#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// WiFi
const char* ssid = "SirsSpot-RSML";
const char* password = "rsmljayasirs1";

// Server
const char* serverUrl = "http://192.168.110.3:8000/api";

// DHT Sensor
#define DHTPIN1 4
#define DHTPIN2 14
#define DHTTYPE DHT11
DHT dht1(DHTPIN1, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);

// LED Indikator WiFi
#define LED_PIN 5  // Pin untuk LED (bisa diganti sesuai kebutuhan)

// Config Device - ESP32 Board 1 di Ruang TI
const int SENSOR_1_ID = 3;
const int SENSOR_2_ID = 4;

// Variable untuk LED blinking
unsigned long previousLEDMillis = 0;
bool ledState = false;

// Variable untuk timing pembacaan sensor dengan State Machine
unsigned long previousSensorMillis = 0;
unsigned long sensor1ReadTime = 0;
const unsigned long sensorInterval = 60000;  // 60 detik
enum SensorState { IDLE, SENSOR1_READ, WAIT_SENSOR2, SENSOR2_READ };
SensorState currentState = IDLE;

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 Board 1 - Ruang TI ===");
  
  // Setup LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  dht1.begin();
  dht2.begin();
  connectWiFi();
  
  // Mulai timer pertama kali
  previousSensorMillis = millis();
}

void loop() {
  // Update status LED berdasarkan koneksi WiFi (terus berjalan tanpa blocking)
  updateLEDStatus();
  
  unsigned long currentMillis = millis();
  
  // State machine untuk pembacaan sensor
  switch (currentState) {
    case IDLE:
      // Tunggu sampai 60 detik berlalu
      if (currentMillis - previousSensorMillis >= sensorInterval) {
        // Pastikan WiFi aktif
        if (WiFi.status() != WL_CONNECTED) {
          Serial.println("WiFi terputus! Mencoba konek ulang...");
          connectWiFi();
        }
        
        Serial.println("\n--- Ruang TI ---");
        currentState = SENSOR1_READ;
      }
      break;
      
    case SENSOR1_READ:
      // Baca sensor 1
      bacaDanKirimSensor(dht1, SENSOR_1_ID);
      sensor1ReadTime = currentMillis;
      currentState = WAIT_SENSOR2;
      break;
      
    case WAIT_SENSOR2:
      // Tunggu 3 detik setelah sensor 1
      if (currentMillis - sensor1ReadTime >= 3000) {
        currentState = SENSOR2_READ;
      }
      break;
      
    case SENSOR2_READ:
      // Baca sensor 2
      bacaDanKirimSensor(dht2, SENSOR_2_ID);
      Serial.println("Tunggu 60 detik...");
      previousSensorMillis = currentMillis;  // Reset timer untuk 60 detik berikutnya
      currentState = IDLE;
      break;
  }
}

// ======================================================
// FUNGSI
// ======================================================

void connectWiFi() {
  Serial.print("Menghubungkan ke WiFi ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  unsigned long startAttemptTime = millis();
  
  // LED kedip cepat saat mencoba koneksi
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Tersambung!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nGagal konek WiFi! Akan dicoba lagi nanti.");
  }
}

void updateLEDStatus() {
  unsigned long currentMillis = millis();
  
  if (WiFi.status() == WL_CONNECTED) {
    // WiFi terhubung: LED kedip setiap 5 detik (5000ms)
    if (currentMillis - previousLEDMillis >= 5000) {
      previousLEDMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
    }
  } else {
    // WiFi tidak terhubung: LED nyala terus
    digitalWrite(LED_PIN, HIGH);
    previousLEDMillis = currentMillis;  // Reset timer saat WiFi mati
  }
}

void bacaDanKirimSensor(DHT &dht, int sensorID) {
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();
  
  if (isnan(suhu) || isnan(kelembapan)) {
    Serial.println("Sensor gagal baca data!");
    return;
  }
  
  Serial.printf("Sensor %d -> Temp: %.2f C | Hum: %.2f%%\n", sensorID, suhu, kelembapan);
  
  // Kirim suhu
  if (kirimData("suhu", "{\"nilai_suhu\":" + String(suhu) + ",\"id_sensor\":" + String(sensorID) + "}")) {
    Serial.println("  Suhu OK");
  } else {
    Serial.println("  Gagal kirim suhu!");
  }
  
  delay(1500);  // Delay kecil untuk jeda antar request (masih acceptable)
  
  // Kirim kelembapan
  if (kirimData("kelembapan", "{\"nilai_kelembapan\":" + String(kelembapan) + ",\"id_sensor\":" + String(sensorID) + "}")) {
    Serial.println("  Kelembapan OK");
  } else {
    Serial.println("  Gagal kirim kelembapan!");
  }
}

bool kirimData(const String& endpoint, const String& payload) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi terputus!");
    return false;
  }
  
  HTTPClient http;
  http.begin(serverUrl + String("/") + endpoint);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000);
  
  bool sukses = false;
  for (int percobaan = 1; percobaan <= 3; percobaan++) {
    int code = http.POST(payload);
    
    if (code == 201) {
      sukses = true;
      break;
    } else if (percobaan < 3) {
      // Hanya tampilkan error jika bukan percobaan terakhir
      delay(1000);
    }
  }
  
  http.end();
  return sukses;
}