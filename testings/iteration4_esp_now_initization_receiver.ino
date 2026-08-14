#include <WiFi.h>
#include <esp_now.h>

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("========== RECEIVER TEST ==========");

    WiFi.mode(WIFI_STA);

    Serial.println("Initializing ESP-NOW...");

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ESP-NOW Initialization Failed");
        while (1);
    }

    Serial.println("ESP-NOW Initialized Successfully");
    Serial.println("Receiver Ready");
}

void loop()
{
    delay(1000);
}
