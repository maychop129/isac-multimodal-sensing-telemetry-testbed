/**
 * @file unit2_lora_target.ino
 * @brief Unit 2: LoRa Target & GPS Telemetry Node (ESP32)
 * @details Firmware for low-cost ISAC telemetry testbed.
 *          - Serial2 (GPIO 16/17): u-blox NEO-6M GPS NMEA stream ingestion
 *          - Serial1 (GPIO 26/27): UART Grove / E32 LoRa transceiver transmission & AT control
 */

#include <Arduino.h>

// GPS Port Definitions (Serial 2)
#define GPS_RX_PIN    16  // Labeled as RX2 / IO16
#define GPS_TX_PIN    17  // Labeled as TX2 / IO17

// Grove LoRa Port Definitions (Serial 1)
#define LORA_RX_PIN   26  // Labeled as IO26 (Receives from LoRa TX)
#define LORA_TX_PIN   27  // Labeled as IO27 (Transmits to LoRa RX)

void setup() {
  Serial.begin(115200);
  
  // Initialize Hardware Serial 2 for GPS
  Serial2.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  
  // Initialize Hardware Serial 1 for Grove LoRa Module (Standard 9600 or 115200 baud)
  Serial1.begin(9600, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);
  
  delay(1000);
  
  // Send basic AT test command to LoRa module to verify communication
  Serial1.println("AT");
  
  Serial.println("UNIT 2: LoRa Target Initialized & Ready.");
}

void loop() {
  // Read incoming GPS data and forward it over LoRa
  if (Serial2.available()) {
    String gpsData = "";
    while (Serial2.available()) {
      char c = Serial2.read();
      gpsData += c;
    }
    
    // Transmit GPS string over LoRa radio module
    Serial1.print("AT+CMSWSTART\r\n"); // Optional command depending on your specific LoRa firmware
    Serial1.print(gpsData);
    Serial.print("TRANSMITTED VIA LORA: ");
    Serial.println(gpsData);
  }

  // Read any response coming back from the LoRa module
  if (Serial1.available()) {
    Serial.write(Serial1.read());
  }
  
  delay(1000);
}
