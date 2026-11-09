/**
 * @file unit1_sensing_hub.ino
 * @brief Unit 1: Multi-Modal Sensing & Telemetry Hub (ESP32 FreeRTOS)
 * @details FreeRTOS dual-core firmware for low-cost ISAC multi-modal testbed.
 *          - Core 1: Sensor acquisition (HC-SR04 Ultrasonic + PIR Motion Sensor)
 *          - Core 0: Real-time telemetry processing & alert dispatch via inter-task FreeRTOS queue
 */

#include <Arduino.h>

// Sensor & LED Pins
#define TRIG_PIN      5
#define ECHO_PIN      35
#define PIR_PIN       34
#define CAM_LED_PIN   4

// Serial Communication Ports
#define GPS_RX_PIN    16
#define GPS_TX_PIN    17
#define LORA_RX_PIN   26
#define LORA_TX_PIN   27

// Data structure for the FreeRTOS Queue
struct SensorEvent {
  uint8_t type; // 1 = Ultrasonic Range, 2 = PIR Motion
  unsigned long timestamp;
  float value;
};

QueueHandle_t sensorQueue;
bool lastPirState = LOW;

// CORE 1 TASK: Sensing (Polling both Ultrasonic and PIR)
void sensingTask(void *pvParameters) {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  while(1) {
    // 1. Poll Ultrasonic
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    float distance = (duration == 0) ? -1.0 : (duration * 0.0343) / 2.0;

    SensorEvent evRange = {1, micros(), distance};
    xQueueSend(sensorQueue, &evRange, portMAX_DELAY);

    // 2. Poll PIR (Bypasses the emulator interrupt crash)
    bool currentPirState = digitalRead(PIR_PIN);
    if (currentPirState == HIGH && lastPirState == LOW) {
      SensorEvent evPir = {2, micros(), 1.0};
      xQueueSend(sensorQueue, &evPir, portMAX_DELAY);
    }
    lastPirState = currentPirState;

    vTaskDelay(pdMS_TO_TICKS(100)); // Cycle every 100ms
  }
}

// CORE 0 TASK: Telemetry (Processing Queue)
void telemetryTask(void *pvParameters) {
  pinMode(CAM_LED_PIN, OUTPUT);
  SensorEvent ev;

  while(1) {
    if (xQueueReceive(sensorQueue, &ev, portMAX_DELAY)) {
      unsigned long processTime = micros(); 
      unsigned long latency = processTime - ev.timestamp; 

      if (ev.type == 2) { 
        digitalWrite(CAM_LED_PIN, HIGH);
        Serial.printf("TIME:%lu | EVENT:PIR DETECTED | LATENCY:%lu us | [SNAPSHOT FLASH]\n", processTime, latency);
        vTaskDelay(pdMS_TO_TICKS(300));
        digitalWrite(CAM_LED_PIN, LOW);
      } 
      else if (ev.type == 1) { 
        Serial.printf("TIME:%lu | EVENT:RANGE | VAL:%.2fcm | LATENCY:%lu us\n", processTime, ev.value, latency);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("UNIT 1: Emulator-Safe FreeRTOS Hub Initialized.");

  sensorQueue = xQueueCreate(10, sizeof(SensorEvent));

  xTaskCreatePinnedToCore(sensingTask, "Sensing", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(telemetryTask, "Telemetry", 4096, NULL, 1, NULL, 0);
}

void loop() {
  vTaskDelete(NULL); 
}
