#include <Wire.h>
#include <Adafruit_AMG88xx.h>
#include <esp_now.h>
#include <WiFi.h>

// --- Pin Definitions for ESP32-C3 ---
#define PIR_WAKE_PIN  2  // AM312 PIR Output
#define MOSFET_PIN    3  // AO3401 Gate (Power Control)
#define SDA_PIN       8  // AMG8833 SDA
#define SCL_PIN       9  // AMG8833 SCL

Adafruit_AMG88xx amg;

// REPLACE THIS with the MAC Address of your Smart Cubby Station Receiver!
// Format: {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}
uint8_t RECEIVER_MAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 

// Structure to hold the 64 thermal pixels
typedef struct struct_message {
  float pixels[64];
} struct_message;

struct_message thermalData;
esp_now_peer_info_t peerInfo;

void setup() {
  // 1. Instantly power the sensor via MOSFET to allow it to boot
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, LOW); // LOW turns ON the P-Channel MOSFET

  // 2. Initialize Wi-Fi in Station mode (required for ESP-NOW)
  WiFi.mode(WIFI_STA);

  // 3. Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    goToSleep(); // If it fails, don't waste battery. Just sleep.
  }

  // 4. Register the Smart Cubby Station as a peer
  memcpy(peerInfo.peer_addr, RECEIVER_MAC, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  // 5. Initialize the AMG8833 Thermal Sensor
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(50); // Give the sensor a tiny moment to stabilize voltage
  
  if (!amg.begin()) {
    goToSleep(); // If sensor isn't found, abort and sleep
  }

  // 6. Read the 8x8 matrix
  amg.readPixels(thermalData.pixels);

  // 7. Transmit the data to the Smart Cubby Station
  esp_now_send(RECEIVER_MAC, (uint8_t *) &thermalData, sizeof(thermalData));

  // 8. Wait a tiny bit to ensure transmission completes, then sleep
  delay(10); 
  goToSleep();
}

void loop() {
  // Never reached. Device goes to deep sleep in setup().
}

void goToSleep() {
  // Cut power to the thermal sensor
  digitalWrite(MOSFET_PIN, HIGH); 
  
  // Configure ESP32-C3 to wake up when PIR pin goes HIGH
  esp_deep_sleep_enable_gpio_wakeup(1ULL << PIR_WAKE_PIN, ESP_GPIO_WAKEUP_GPIO_HIGH);
  
  // Enter Deep Sleep
  esp_deep_sleep_start();
}