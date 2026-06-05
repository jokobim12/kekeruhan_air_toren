#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// Helper untuk token dan RTDB
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#define SENSOR_PIN 34

// ==========================================
// TODO: GANTI DENGAN KREDENSIAL WIFI & FIREBASE ANDA
// ==========================================
#define WIFI_SSID "Pixel 6a"
#define WIFI_PASSWORD "87654321"
#define DATABASE_URL "https://waterturbiditymonitor-default-rtdb.asia-southeast1.firebasedatabase.app" // Contoh: https://xxxx.firebaseio.com/ atau https://xxxx.asia-southeast1.firebasedatabase.app/
#define DATABASE_SECRET "IUioUNg7b1zdgad7y5JowM9A1mEjluReZwjnopWd"
// ==========================================

// Objek Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
unsigned long sendInterval = 5000; // Default 5 detik (bisa berubah dinamis)
unsigned long checkSettingsPrevMillis = 0;
const long checkSettingsInterval = 10000; // Cek pengaturan setiap 10 detik

// Batas ambang default (bisa berubah dinamis dari Firebase)
int cleanThreshold = 1900;
int cloudyThreshold = 1850;

void setup() {
  Serial.begin(115200);

  // Mulai koneksi Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan ke Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Terhubung ke Wi-Fi dengan IP: ");
  Serial.println(WiFi.localIP());

  // Konfigurasi Firebase
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;

  Firebase.reconnectWiFi(true);
  Firebase.begin(&config, &auth);
}

void loop() {
  // Cek update pengaturan delay dan ambang batas dari Firebase secara berkala (setiap 10 detik)
  if (Firebase.ready() && (millis() - checkSettingsPrevMillis > checkSettingsInterval || checkSettingsPrevMillis == 0)) {
    checkSettingsPrevMillis = millis();
    
    // 1. Baca delay
    int tempDelay = 0;
    if (Firebase.RTDB.getInt(&fbdo, "/turbidity/settings/delay", &tempDelay)) {
      if (tempDelay > 0) {
        sendInterval = tempDelay * 1000; // Ubah detik ke milidetik
        Serial.print("Interval pengiriman diperbarui secara dinamis: ");
        Serial.print(tempDelay);
        Serial.println(" detik");
      }
    }

    // 2. Baca threshold bersih
    int tempClean = 0;
    if (Firebase.RTDB.getInt(&fbdo, "/turbidity/thresholds/clean", &tempClean)) {
      cleanThreshold = tempClean;
      Serial.print("Batas BERSIH diperbarui: ");
      Serial.println(cleanThreshold);
    } else {
      // Tulis default jika belum ada di database
      Firebase.RTDB.setInt(&fbdo, "/turbidity/thresholds/clean", cleanThreshold);
    }

    // 3. Baca threshold keruh
    int tempCloudy = 0;
    if (Firebase.RTDB.getInt(&fbdo, "/turbidity/thresholds/cloudy", &tempCloudy)) {
      cloudyThreshold = tempCloudy;
      Serial.print("Batas KERUH diperbarui: ");
      Serial.println(cloudyThreshold);
    } else {
      // Tulis default jika belum ada di database
      Firebase.RTDB.setInt(&fbdo, "/turbidity/thresholds/cloudy", cloudyThreshold);
    }

    // 4. Kirim Wi-Fi SSID saat ini ke Firebase agar terbaca di halaman Profil Android
    Firebase.RTDB.setString(&fbdo, "/turbidity/status/wifi_ssid", WiFi.SSID());
  }

  // Kirim data setiap interval yang ditentukan
  if (Firebase.ready() && (millis() - sendDataPrevMillis > sendInterval || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    int sensorValue = analogRead(SENSOR_PIN);
    String status = "";

    if (sensorValue > cleanThreshold) {
      status = "BERSIH";
    } else if (sensorValue > cloudyThreshold) {
      status = "KERUH";
    } else {
      status = "KOTOR";
    }

    Serial.print("Nilai Sensor: ");
    Serial.println(sensorValue);
    Serial.print("Status: ");
    Serial.println(status);

    // Kirim data nilai sensor ke Firebase RTDB
    if (Firebase.RTDB.setInt(&fbdo, "/turbidity/value", sensorValue)) {
      Serial.println("Berhasil mengirim nilai sensor ke Firebase");
    } else {
      Serial.print("Gagal mengirim nilai sensor: ");
      Serial.println(fbdo.errorReason());
    }

    // Kirim data status ke Firebase RTDB
    if (Firebase.RTDB.setString(&fbdo, "/turbidity/status", status)) {
      Serial.println("Berhasil mengirim status ke Firebase");
    } else {
      Serial.print("Gagal mengirim status: ");
      Serial.println(fbdo.errorReason());
    }

    Serial.println("----------------");
  }
}