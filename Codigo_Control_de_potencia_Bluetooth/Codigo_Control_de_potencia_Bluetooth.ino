#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Adafruit_NeoPixel.h>

#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcd1234-5678-1234-5678-abcdef123456"

#define RELAY_PIN 47  

// ===== LED RGB INTEGRADO ESP32-S3 =====
#define LED_PIN 48
#define NUM_LEDS 1

Adafruit_NeoPixel ledRGB(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void mostrarColor(uint8_t r, uint8_t g, uint8_t b) {
  ledRGB.setPixelColor(0, ledRGB.Color(r, g, b));
  ledRGB.show();
  delay(2000); // Encendido 2 segundos
  ledRGB.clear();
  ledRGB.show();
}

bool dispositivoConectado = false;

// Variables anti-duplicados
unsigned long ultimoTiempo = 0;
String ultimoMensaje = "";

// ======================
// CALLBACKS SERVIDOR
// ======================
class MyServerCallbacks: public BLEServerCallbacks {

  void onConnect(BLEServer* pServer) {
    dispositivoConectado = true;
    Serial.println("Celular conectado correctamente");
    
    // AZUL = conectado
    mostrarColor(0, 0, 255);
  }

  void onDisconnect(BLEServer* pServer) {
    dispositivoConectado = false;
    Serial.println("Celular desconectado");
    
    BLEDevice::startAdvertising();
    Serial.println("Esperando nueva conexion...");

    // Apagar foco por seguridad
    digitalWrite(RELAY_PIN, HIGH);

    // ROJO = desconectado
    mostrarColor(255, 0, 0);

    // VERDE = vuelve a esperar conexion
    mostrarColor(0, 255, 0);
  }
};

// ======================
// CALLBACKS CARACTERISTICA
// ======================
class MyCallbacks: public BLECharacteristicCallbacks {

  void onWrite(BLECharacteristic* pCharacteristic) {

    String rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {

      rxValue.trim();
      rxValue.replace("\n", "");
      rxValue.replace("\r", "");
      rxValue.toLowerCase();

      // Anti-duplicado
      if (rxValue == ultimoMensaje && millis() - ultimoTiempo < 400) {
        Serial.println("Mensaje duplicado ignorado");
        return;
      }

      ultimoMensaje = rxValue;
      ultimoTiempo = millis();

      Serial.print("Comando recibido: ");
      Serial.println(rxValue);

      if (rxValue == "encender") {
        digitalWrite(RELAY_PIN, LOW);   
        Serial.println("FOCO ENCENDIDO");
      }
      else if (rxValue == "apagar") {
        digitalWrite(RELAY_PIN, HIGH);  
        Serial.println("FOCO APAGADO");
      }
      else {
        Serial.println("Comando desconocido");
      }
    }
  }
};

// ======================
// SETUP
// ======================
void setup() {

  Serial.begin(115200);
  delay(1000);   // IMPORTANTE para ESP32-S3

  Serial.println("================================");
  Serial.println("Iniciando sistema BLE ESP32-S3");
  Serial.println("================================");

  // Configuración del relé
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  // Inicializar LED RGB
  ledRGB.begin();
  ledRGB.clear();
  ledRGB.show();

  BLEDevice::init("ESP32_Control_Foco_Jaquikisitos");

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_WRITE_NR
  );

  pCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();

  Serial.println("BLE listo.");
  Serial.println("Esperando conexion desde la app...");

  // VERDE = esperando conexion
  mostrarColor(0, 255, 0);
}

// ======================
// LOOP
// ======================
void loop() {

}