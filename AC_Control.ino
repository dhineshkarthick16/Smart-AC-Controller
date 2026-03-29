#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "DHT.h"

// 🔹 BLE UUIDs (same as your app)
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcd1234-5678-1234-5678-abcdef123456"

// 🔹 Pins
#define DHTPIN 4
#define DHTTYPE DHT11
#define LED_PIN 18   // 🔥 LED added

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

// 🔹 Sensor setup
DHT dht(DHTPIN, DHTTYPE);

// 🔹 AC State (simulated using LED)
bool acState = false;

// 🔹 Server Callbacks
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Device connected ✅");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Device disconnected ❌");

    BLEDevice::getAdvertising()->start();
    Serial.println("Advertising restarted...");
  }
};

void setup() {
  Serial.begin(115200);

  // 🔹 BLE Setup
  BLEDevice::init("ESP32_ALERT_DEVICE");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setValue("START");

  pService->start();
  BLEDevice::getAdvertising()->start();

  Serial.println("BLE advertising started 🚀");

  // 🔹 Sensor + LED init
  dht.begin();
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // initially OFF
}

void loop() {
  if (deviceConnected) {

    float temp = dht.readTemperature();

    if (isnan(temp)) {
      Serial.println("Sensor error ❌");
      return;
    }

    // 🔹 Send temperature to app
    String state = acState ? "ON" : "OFF";
String msg = "\nTemperature: " + String(temp)+ " C\n" + "State: " + state;

pCharacteristic->setValue(msg.c_str());
pCharacteristic->notify();

Serial.println("Sent: " + msg);
  

    // 🔥 CONTROL LOGIC (LED instead of AC)

    // Turn ON (simulate AC ON)
    if (temp > 25 && !acState) {
      digitalWrite(LED_PIN, HIGH);
      acState = true;

      pCharacteristic->setValue("AC TURNED ON");
      pCharacteristic->notify();
      Serial.println("AC Turned ON (LED ON)");
    }

    // Turn OFF (simulate AC OFF)
    if (temp < 22 && acState) {
      digitalWrite(LED_PIN, LOW);
      acState = false;

      pCharacteristic->setValue("AC TURNED OFF");
      pCharacteristic->notify();
      Serial.println("AC Turned OFF (LED OFF)");
    }
  }

  delay(3000); // update every 3 sec
}