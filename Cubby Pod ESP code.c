#include <esp_now.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// --- BLE Configuration ---
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool justConnected = false;

// --- Hardware Pins ---
const int NUM_PODS = 6;
const int redPins[NUM_PODS]   = {13, 14, 27, 26, 25, 33}; 
const int greenPins[NUM_PODS] = {12, 15, 2, 4, 5, 18};    

// --- Pod State Management ---
enum PodState { IDLE, WAITING, VERIFIED, PICKED_UP };
PodState podStates[NUM_PODS];
float expectedW[NUM_PODS];
float expectedT[NUM_PODS];

unsigned long previousMillis = 0;
bool toggleState = false;

// --- ESP-NOW Thermal Data Setup ---
typedef struct struct_message {
  float pixels[64];
} struct_message;

struct_message incomingThermalData;
volatile bool newThermalDataReceived = false;

// --- Mock Sensor Functions ---
float getPodWeight(int podIndex) { return 0.0; } // Replace with HX711 logic
float getPodTemp(int podIndex) { return 25.0; }  // Replace with DS18B20 logic

// --- ESP-NOW Callback ---
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingDataPtr, int len) {
  memcpy(&incomingThermalData, incomingDataPtr, sizeof(incomingThermalData));
  newThermalDataReceived = true; 
}

// --- BLE Callbacks ---
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      justConnected = true; 
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      BLEDevice::startAdvertising(); 
    }
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pChar) {
      std::string rxValue = pChar->getValue();
      if (rxValue.length() > 0) {
        
        // App sends: {"pod": 0, "w": 1.5, "t": 45.0}
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, rxValue);
        
        if (!error && doc.containsKey("pod")) {
          int targetPod = doc["pod"];
          if(targetPod >= 0 && targetPod < NUM_PODS) {
            expectedW[targetPod] = doc["w"];
            expectedT[targetPod] = doc["t"];
            
            // Set state to waiting and update LEDs
            podStates[targetPod] = WAITING;
            digitalWrite(redPins[targetPod], HIGH);
            digitalWrite(greenPins[targetPod], LOW);
          }
        }
      }
    }
};

void setup() {
  Serial.begin(115200);

  // 1. Initialize LEDs
  for(int i=0; i<NUM_PODS; i++) {
    pinMode(redPins[i], OUTPUT);
    pinMode(greenPins[i], OUTPUT);
    digitalWrite(redPins[i], LOW);
    digitalWrite(greenPins[i], LOW);
    podStates[i] = IDLE;
  }

  // 2. Initialize Wi-Fi for ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  // 3. Initialize BLE
  BLEDevice::init("SmartPlate_Cubby");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
                    
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks()); 
  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();
}

// Helper to send status to the app
void sendBLEPayload(String payload) {
  if (deviceConnected) {
    pCharacteristic->setValue(payload.c_str());
    pCharacteristic->notify();
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // --- 1. Master Pod (Pod 0) Connection Animations ---
  if (!deviceConnected) {
    if (currentMillis - previousMillis >= 500) {
      previousMillis = currentMillis;
      toggleState = !toggleState;
      digitalWrite(redPins[0], toggleState ? HIGH : LOW);
      digitalWrite(greenPins[0], LOW);
    }
  } 
  else if (justConnected) {
    for(int i=0; i<6; i++) { 
      digitalWrite(greenPins[0], i % 2 == 0 ? HIGH : LOW);
      digitalWrite(redPins[0], LOW);
      delay(150); 
    }
    justConnected = false;
    digitalWrite(greenPins[0], LOW); 
  }

  if (deviceConnected && !justConnected) {
    
    // --- 2. Process Incoming ESP-NOW Thermal Data ---
    if (newThermalDataReceived) {
      DynamicJsonDocument doc(2048);
      doc["type"] = "rush_data";
      JsonArray grid = doc.createNestedArray("grid");
      for (int i = 0; i < 64; i++) {
        grid.add(round(incomingThermalData.pixels[i] * 10.0) / 10.0); 
      }
      
      String payload;
      serializeJson(doc, payload);
      sendBLEPayload(payload);
      
      newThermalDataReceived = false;
    }

    // --- 3. Operational Logic for All Pods ---
    for (int i = 0; i < NUM_PODS; i++) {
      float currentW = getPodWeight(i);
      float currentT = getPodTemp(i);
      
      switch (podStates[i]) {
        case WAITING:
          if (currentW >= (expectedW[i] * 0.90) && currentT >= (expectedT[i] * 0.90)) {
            podStates[i] = VERIFIED;
            digitalWrite(redPins[i], LOW);
            digitalWrite(greenPins[i], HIGH);
            
            DynamicJsonDocument doc(128);
            doc["type"] = "status";
            doc["pod"] = i;
            doc["status"] = "received";
            String payload; serializeJson(doc, payload);
            sendBLEPayload(payload);
          }
          break;

        case VERIFIED:
          if (currentW <= 0.1) { 
            podStates[i] = PICKED_UP;
            digitalWrite(redPins[i], HIGH);
            digitalWrite(greenPins[i], HIGH);
            
            DynamicJsonDocument doc(128);
            doc["type"] = "status";
            doc["pod"] = i;
            doc["status"] = "picked_up";
            String payload; serializeJson(doc, payload);
            sendBLEPayload(payload);
          }
          break;
          
        case PICKED_UP:
        case IDLE:
          break;
      }
    }
  }
}