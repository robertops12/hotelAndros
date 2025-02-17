#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

const char* ssid = "PRIEGO";
const char* password = "qazwsxedc";
const char* firmware_url = "https://raw.githubusercontent.com/tu-usuario/tu-repositorio/main/firmware.bin"; // Cambia esto por tu URL real

void updateFirmware() {
  WiFiClient client;
  HTTPClient http;

  Serial.println("Descargando firmware desde GitHub...");
  http.begin(client, firmware_url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();
    if (contentLength > 0) {
      WiFiClient* stream = http.getStreamPtr();
      if (Update.begin(contentLength)) {
        Serial.println("Flasheando...");
        size_t written = Update.writeStream(*stream);
        if (written == contentLength) {
          Serial.println("Flasheo completo!");
          if (Update.end(true)) {
            Serial.println("Reiniciando...");
            ESP.restart();
          }
        }
      }
    }
  } else {
    Serial.println("Error descargando firmware");
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");

  updateFirmware();  // Llamar la actualización al iniciar
}

void loop() {}
