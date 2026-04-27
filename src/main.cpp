#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "DHT.h"

// ─── Pin definitions ───
#define DHT_PIN      21    // DHT11 data pin
#define GREEN_LED   16    // Green LED (temp OK)
#define RED_LED     17    // Red LED (too hot)

// ─── Thresholds ───
#define TEMP_TOO_HOT_F  77.0   // Above this → red LED

// ─── DHT setup ───
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// ─── Shared sensor data (protected by queue) ───
typedef struct {
    float tempF;
    float humidity;
    bool  valid;
} SensorReading;

QueueHandle_t sensorQueue;

// ════════════════════════════════════════════
// TASK 1: Read Sensor  (Priority 2)
// Reads DHT11 every 2 seconds, sends to queue
// ════════════════════════════════════════════
void taskReadSensor(void *pv) {
    SensorReading reading;

    while (true) {
        float h = dht.readHumidity();
        float c = dht.readTemperature();     // Celsius
        float f = dht.readTemperature(true); // Fahrenheit

        if (isnan(h) || isnan(f)) {
            // Bad read — mark invalid
            reading.valid    = false;
            reading.tempF    = 0;
            reading.humidity = 0;
            Serial.println("[Sensor] Read failed — check wiring!");
        } else {
            reading.valid    = true;
            reading.tempF    = f;
            reading.humidity = h;
            Serial.printf("[Sensor] Temp: %.1f°F  Humidity: %.1f%%\n", f, h);
        }

        // Overwrite queue with latest reading (never blocks)
        xQueueOverwrite(sensorQueue, &reading);

        vTaskDelay(pdMS_TO_TICKS(2000)); // DHT11 needs 2s between reads
    }
}

// ════════════════════════════════════════════
// TASK 2: Control LEDs  (Priority 1)
// Reads queue and updates LEDs accordingly
// ════════════════════════════════════════════
void taskControlLEDs(void *pv) {
    SensorReading reading;

    while (true) {
        // Wait up to 3 seconds for a new reading
        if (xQueueReceive(sensorQueue, &reading, pdMS_TO_TICKS(3000)) == pdTRUE) {

            if (!reading.valid) {
                // Blink both LEDs to signal error
                digitalWrite(GREEN_LED, HIGH);
                digitalWrite(RED_LED,   HIGH);
                vTaskDelay(pdMS_TO_TICKS(200));
                digitalWrite(GREEN_LED, LOW);
                digitalWrite(RED_LED,   LOW);

            } else if (reading.tempF > TEMP_TOO_HOT_F) {
                // Too hot → red on, green off
                digitalWrite(RED_LED,   HIGH);
                digitalWrite(GREEN_LED, LOW);
                Serial.printf("[LED] TOO HOT: %.1f°F\n", reading.tempF);

            } else {
                // OK → green on, red off
                digitalWrite(GREEN_LED, HIGH);
                digitalWrite(RED_LED,   LOW);
                Serial.printf("[LED] OK: %.1f°F\n", reading.tempF);
            }
        } else {
            // No reading received — sensor may be disconnected
            // Flash red slowly to warn
            digitalWrite(RED_LED, !digitalRead(RED_LED));
            digitalWrite(GREEN_LED, LOW);
            Serial.println("[LED] No data from sensor!");
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ════════════════════════════════════════════
// TASK 3: Serial Monitor  (Priority 1)
// Prints a status summary every 5 seconds
// ════════════════════════════════════════════
void taskSerialMonitor(void *pv) {
    SensorReading reading;

    while (true) {
        if (xQueuePeek(sensorQueue, &reading, 0) == pdTRUE && reading.valid) {
            Serial.println("─────────────────────────");
            Serial.printf("  Temp:     %.1f°F\n", reading.tempF);
            Serial.printf("  Humidity: %.1f%%\n",  reading.humidity);
            Serial.printf("  Status:   %s\n",
                reading.tempF > TEMP_TOO_HOT_F ? "TOO HOT" : "OK");
            Serial.println("─────────────────────────");
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// ════════════════════════════════════════════
// SETUP
// ════════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    dht.begin();

    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED,   OUTPUT);

    // Start with both off
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED,   LOW);

    // Queue holds exactly 1 reading (always the latest)
    sensorQueue = xQueueCreate(1, sizeof(SensorReading));

    xTaskCreate(taskReadSensor,    "ReadSensor",  2048, NULL, 2, NULL);
    xTaskCreate(taskControlLEDs,   "ControlLEDs", 2048, NULL, 1, NULL);
    xTaskCreate(taskSerialMonitor, "SerialMon",   2048, NULL, 1, NULL);

    Serial.println("DHT11 Monitor started!");
}

void loop() {
    vTaskDelay(portMAX_DELAY); // FreeRTOS takes over
}