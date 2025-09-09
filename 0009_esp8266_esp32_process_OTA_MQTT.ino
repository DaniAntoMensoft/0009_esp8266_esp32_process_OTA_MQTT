#include <WiFi.h>
#include <WiFiClient.h>
#include <Arduino_MQTT_Client.h>
#include <OTA_Firmware_Update.h>
#include <ThingsBoard.h>
#include <Espressif_Updater.h>
#include <array>

// -------------------- Configuración --------------------
constexpr char WIFI_SSID[] = "danianto";
constexpr char WIFI_PASSWORD[] = "danianto";

constexpr char TOKEN[] = "zfr87CTUUFoEU1rUx0RQ";
constexpr char THINGSBOARD_SERVER[] = "demo.thingsboard.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U; //1883

constexpr uint16_t MAX_RECV = 512;
constexpr uint16_t MAX_SEND = 512;
constexpr uint32_t SERIAL_BAUD = 115200;

constexpr char FW_TITLE[] = "TEST";
constexpr char FW_VERSION[] = "1.0.0";

constexpr uint16_t FW_PACKET_SIZE = 4096;
constexpr uint8_t FW_RETRIES = 12;

// -------------------- Variables globales --------------------
WiFiClient wifiClient;
Arduino_MQTT_Client mqtt(wifiClient);
OTA_Firmware_Update<> ota;
Espressif_Updater<> updater;

std::array<IAPI_Implementation*, 1> apis = { &ota };
ThingsBoard tb(mqtt, MAX_RECV, MAX_SEND, Default_Max_Stack_Size, apis);

bool fwInfoSent = false;
bool updateRequested = false;

// -------------------- Callbacks --------------------
void updateStart() { /* nada */ }

void finishedCallback(const bool &success) {
  if(success) {
    Serial.println("OTA completada, reiniciando...");
    esp_restart();
  } else {
    Serial.println("Fallo en OTA");
  }
}

void progressCallback(const size_t &current, const size_t &total) {
  Serial.printf("Progreso: %.2f%%\n", float(current*100)/total);
}

// -------------------- Funciones --------------------
void connectWiFi() {
  Serial.print("Conectando a WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado");
}

bool reconnectWiFi() {
  if(WiFi.status() == WL_CONNECTED) return true;
  connectWiFi();
  return true;
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);
  connectWiFi();
}

// -------------------- Loop --------------------
void loop() {
  delay(1000);
  if(!reconnectWiFi()) return;

  if(!tb.connected()) {
    Serial.printf("Conectando a ThingsBoard con token %s\n", TOKEN);
    if(!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
      Serial.println("Error al conectar");
      return;
    }
  }

  if(!fwInfoSent) {
    fwInfoSent = ota.Firmware_Send_Info(FW_TITLE, FW_VERSION);
  }

  if(!updateRequested) {
    OTA_Update_Callback callback(FW_TITLE, FW_VERSION, &updater,
                                 &finishedCallback,
                                 &progressCallback,
                                 &updateStart,
                                 FW_RETRIES, FW_PACKET_SIZE);
    updateRequested = ota.Start_Firmware_Update(callback);
  }

  tb.loop();
}
