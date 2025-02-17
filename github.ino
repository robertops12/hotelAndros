#include <WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h> // Librería MQTT
#include <HTTPClient.h>
#include <Update.h>

// Credenciales WiFi
const char* ssid = "PRIEGO";
const char* password = "qazwsxedc";

// Configuración MQTT
const char* mqttServer = "192.168.2.185"; // Dirección del servidor MQTT
const int mqttPort = 1883;
const char* mqttTopic = "prueba/variable"; // Topic MQTT

// URL del firmware en GitHub
const char* firmware_url = "https://github.com/robertops12/hotelAndros/blob/cbd3e0f9d14e521cc314ba5d5f79378715ca4d87/github.ino";

// Variables globales
int prueba = 0;
WiFiClient espClient;
PubSubClient client(espClient);
WiFiServer telnetServer(23);
WiFiClient telnetClient;
const int otaPort = 8266;

void setup() {
  Serial.begin(115200);

  // Conectar WiFi
  WiFi.begin(ssid, password);
  int wifiConnectAttempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++wifiConnectAttempts > 10) {
      Serial.println("\nError al conectar a WiFi. Reiniciando...");
      ESP.restart();
    }
  }

  Serial.println("\nWiFi conectado. IP: " + WiFi.localIP().toString());

  // Configurar OTA
  ArduinoOTA.setHostname("ESP32-OTA");
  ArduinoOTA.setPort(otaPort);
  ArduinoOTA.onStart([]() { Serial.println("Inicio de actualización OTA"); });
  ArduinoOTA.onEnd([]() { Serial.println("\nActualización completa. Reiniciando..."); ESP.restart(); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progreso: %u%%\n", (progress * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error OTA [%u]: ", error);
  });
  ArduinoOTA.begin();
  Serial.println("OTA listo.");

  // Iniciar Telnet
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  Serial.println("Telnet listo en el puerto 23");

  // Configuración MQTT
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  connectToMqtt();

  // Buscar actualización en GitHub
  updateFirmware();
}

void loop() {
  ArduinoOTA.handle();
  
  // Manejo de Telnet
  if (telnetServer.hasClient()) {
    if (telnetClient) telnetClient.stop();
    telnetClient = telnetServer.available();
    Serial.println("Cliente Telnet conectado");
  }
  if (telnetClient && telnetClient.connected()) {
    while (telnetClient.available()) telnetClient.read();
    telnetClient.println("ESP32 dice: Hola desde OTA!");
  }

  // Mantener MQTT activo
  if (!client.connected()) {
    connectToMqtt();
  }
  client.loop();

  // Publicar datos en MQTT
  prueba++;
  char pruebaStr[10];
  itoa(prueba, pruebaStr, 10);
  client.publish(mqttTopic, pruebaStr);
  Serial.print("Publicado por MQTT: ");
  Serial.println(prueba);
  delay(5000);
}

// Conectar a MQTT
void connectToMqtt() {
  while (!client.connected()) {
    Serial.print("Conectando a MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Conectado");
    } else {
      Serial.print("Error MQTT, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

// Función callback para MQTT
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Mensaje recibido en [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
  }
  Serial.println();
}

// Descargar y flashear firmware desde GitHub
void updateFirmware() {
  WiFiClient client;
  HTTPClient http;
  Serial.println("Buscando actualización en GitHub...");

  http.begin(client, firmware_url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();
    if (contentLength > 0) {
      WiFiClient* stream = http.getStreamPtr();
      if (Update.begin(contentLength)) {
        Serial.println("Descargando firmware...");
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
    Serial.println("No se encontró actualización en GitHub");
  }

  http.end();
}
