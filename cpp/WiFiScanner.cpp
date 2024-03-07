#include "WiFiScanner.h"
#include <WiFi.h> // For ESP32
// #include <ESP8266WiFi.h> // For ESP8266, uncomment this line and comment out the above include

WiFiScanner::WiFiScanner() {}

void WiFiScanner::scanNetworks()
{
    Serial.println("Scanning WiFi networks.");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    int n = WiFi.scanNetworks();
    if (n == 0)
    {
        Serial.println("No networks found.");
        return;
    }

    Serial.println();
    for (int i = 0; i < n; ++i)
    {
        Serial.print(i + 1);
        if (i < 9)
        {
            Serial.print(":  ");
        }
        else
        {
            Serial.print(": ");
        }
        Serial.print(WiFi.SSID(i));
        if (WiFi.SSID(i).end() == " ")
        {
            Serial.print(" ");
        }
        for (int j(0); j < (20 - WiFi.SSID(i).length()); ++j)
        {
            Serial.print(" ");
        }
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(" dBm) ");
        Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Encrypted");
    }

    Serial.println();
    Serial.println("WiFi Scan Complete.");
}