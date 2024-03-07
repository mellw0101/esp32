#include <Arduino.h>

#include <WiFi.h>
#include <vector>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <esp_wifi.h>
#include "nvs_flash.h"
#include <stdio.h>
#include <BlueTooth.h>
#include <base64.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <sstream>

using namespace std;

WiFiServer monitor_server(80);
// WiFiServer telnet_server(23); // Port 23 is the default for Telnet

#define BLINK_GPIO (gpio_num_t)5
#define CONFIG_BLINK_PERIOD 1000
static uint8_t s_led_state = 0;

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

static void blink_for(int __iter, int __period)
{
    for (int i = 0; i < (__iter + 1); ++i)
    {
        blink_led();
        s_led_state = !s_led_state;
        vTaskDelay(__period / portTICK_PERIOD_MS);
    }
}

class __wifi__
{
    #define WIFI_CONNECTED true
    #define WIFI_NOT_CONNECTED false

    public:
        bool connect()
        {
            WiFi.begin("comhem_3E2192", "26849bzs");
            Serial.println();
            Serial.print("Connecting to Wifi");
            for (int i(0); WiFi.status() != WL_CONNECTED; ++i)
            {
                delay(200);
                Serial.print(".");
                if (i > 40)
                {
                    Serial.println(" Could not connect to wifi.");
                    return WIFI_NOT_CONNECTED;
                }
            }
            Serial.println(" Done!");
            Serial.print("Local IP address: ");
            Serial.println(WiFi.localIP());
            Serial.println();
            return WIFI_CONNECTED;
        }

        void scanNetworks()
        {
            Serial.println("Scanning WiFi networks.");
            WiFi.mode(WIFI_STA);
            // WiFi.disconnect();
            // delay(100);

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
};
static __wifi__ *wifi(nullptr);

class __clients__
{
    #define MAX_CLIENTS 8

    public:
        vector<WiFiClient *> list;

        void send_to_clients(const string &__str)
        {
            if (list.empty()) return;

            for (uint8_t i(0); i < list.size(); ++i)
            {
                list[i]->print(__str.c_str());
            }
        }

        void print(const string &__s)
        {
            if (list.empty()) return;

            for (uint8_t i(0); i < list.size(); ++i)
            {
                list[i]->print(__s.c_str());
            }
        }

        void println(const char *__s = "")
        {
            if (list.empty()) return;

            for (uint8_t i(0); i < list.size(); ++i)
            {
                list[i]->println(__s);
            }
        }

        // void check_for_new_clients()
        // {
        //     if (list.size() >= MAX_CLIENTS) return;

        //     WiFiClient wc = telnet_server.available();

        //     if (wc)
        //     {
        //         if (wc.connected())
        //         {
        //             wc.print("Connected");
        //             wc.println();
        //             WiFiClient *wc_ptr = new WiFiClient(wc);
        //             this->list.push_back(wc_ptr);
        //         }
        //     }
        // }
};
static __clients__ *clients(nullptr);

class __api__
{
    public:
        String get(const char* serverName)
        {
            if (WiFi.status() != WL_CONNECTED)
            {
                return "Wifi not connected";
            }
            
            HTTPClient http;
            http.begin(serverName);
            int httpResponseCode = http.GET();
            
            if (httpResponseCode > 0)
            {
                String response = http.getString();
                http.end();
                return response;
            }
            else
            {
                http.end();
                return "Error on HTTP request";
            }
        }

        String get(const char* serverName, const char *__key, const char *__secret)
        {
            if (WiFi.status() != WL_CONNECTED)
            {
                return "Wifi not connected";
            }
            
            HTTPClient http;
            http.begin(serverName);
            http.addHeader("X-API-KEY", __key);
            http.addHeader("X-API-SECRET", __secret);

            int httpResponseCode = http.GET();
            
            if (httpResponseCode > 0)
            {
                String response = http.getString();
                http.end();
                return response;
            }
            else
            {
                http.end();
                return "Error on HTTP request";
            }
        }

        String makePostAuthRequest(const String& url, const String& postData, const String& publicApiKey, const String& privateApiKey) 
        {
            if (WiFi.status() != WL_CONNECTED) 
            {
                return "WiFi Disconnected";
            }

            HTTPClient http;
            http.begin(url);
            http.addHeader("Content-Type", "application/x-www-form-urlencoded");

            // Prepare the Basic Auth header
            String auth = publicApiKey + ":" + privateApiKey;
            auth = base64::encode(auth);
            http.addHeader("Authorization", "Basic " + auth);

            int httpResponseCode = http.POST(postData);
            String response = "";

            if (httpResponseCode > 0)
            {
                response = http.getString();
            } 
            else 
            {
                response = "Error on HTTP request: " + String(httpResponseCode);
            }

            http.end();
            return response;
        }

        String getWithAuth(const String& url, const String& publicApiKey, const String& privateApiKey)
        {
            if ((WiFi.status() != WL_CONNECTED)) 
            {
                return "WiFi Disconnected";
            }

            HTTPClient http;
            http.begin(url);

            // Prepare the Basic Auth header
            String auth = publicApiKey + ":" + privateApiKey;
            auth = base64::encode(auth);
            http.addHeader("Authorization", "Basic " + auth);

            int httpResponseCode = http.GET();
            String response = "";

            if (httpResponseCode > 0) 
            {
                response = http.getString();
            } 
            else 
            {
                response = "Error on HTTP request: " + String(httpResponseCode);
            }

            http.end();
            return response;
        }

    public:
        __api__() {}
};
static __api__ *api(nullptr);

class __ota__
{
    public:
        void setup_OTA()
        {
            ArduinoOTA.onStart([]()
            {
                String type;
                if (ArduinoOTA.getCommand() == U_FLASH)
                {
                    type = "sketch";
                }
                else // U_SPIFFS
                {
                    type = "filesystem";
                }
                // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                Serial.println("Start updating " + type);
            });
            
            ArduinoOTA.onEnd([]()
            {
                Serial.println("\nEnd");
                // clients->println("End");
            });
            
            ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
            {
                Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
                const char *progress_str = to_string(progress).c_str();
                clients->println(progress_str);
            });
            
            ArduinoOTA.onError([](ota_error_t error)
            {
                Serial.printf("Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR)
                {
                    Serial.println("Auth Failed");
                }
                else if (error == OTA_BEGIN_ERROR)
                {
                    Serial.println("Begin Failed");
                }
                else if (error == OTA_CONNECT_ERROR)
                {
                    Serial.println("Connect Failed");
                }
                else if (error == OTA_RECEIVE_ERROR)
                {
                    Serial.println("Receive Failed");
                }
                else if (error == OTA_END_ERROR)
                {
                    Serial.println("End Failed");
                }
            });

            ArduinoOTA.begin();
        }

    public:
        __ota__() {}
};

class __telnet_server__
{
    #define MAX_CONN 1
    #define TELNET_PORT 23
    #define SUCCESSFULL_CONNECTION_MSG "Connected.\n\r"

    private:
        int listen_sock, sock;
        struct sockaddr_in source_addr;
        socklen_t addr_len = sizeof(source_addr);

        #define SEND_SUCCESSFULL 1
        #define SEND_FAILED 0

        int send__(const string &__str)
        {
            if (send(sock, __str.c_str(), __str.length(), 0) < 0)
            {
                return SEND_FAILED;
            }

            return SEND_SUCCESSFULL;
        }

        #define ACCEPT_SUCCESSFULL 1
        #define ACCEPT_FAILED -1

        int accept__()
        {
            if((sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len)) <= ACCEPT_FAILED)
            {
                Serial.printf("Telnet Server", "Unable to accept connection: errno %d", errno);
                return ACCEPT_FAILED;
            }
            
            Serial.println("Telnet Server: Socket accepted");
            return ACCEPT_SUCCESSFULL;
        }
        
    public:
        void init()
        {
            int addr_family = AF_INET;
            int ip_protocol = IPPROTO_IP;
            struct sockaddr_in dest_addr;
            dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_port = htons(TELNET_PORT);
            listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
            bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            listen(listen_sock, MAX_CONN);
        }

        void run()
        {
            while (true)
            {
                if (accept__() <= ACCEPT_FAILED) break; 
                send__(SUCCESSFULL_CONNECTION_MSG);

                while (true)
                {
                    stringstream ss;
                    char __char = '\0';
                    while (read(sock, &__char, 1) > 0)
                    {
                        if (__char == '\0')
                        {
                            if (ss.str().empty())
                            {
                                ss << "\n";
                            }
                            break;
                        }
                        ss << __char;
                    }

                    if (ss.str().empty()) break;

                    Serial.println(ss.str().c_str());
                    ss.clear();
                }

                if (sock != -1)
                {
                    Serial.println("Telnet Server: Shutting down socket.");
                    shutdown(sock, 0);
                    close(sock);
                }
            }
        }

    public:
        __telnet_server__() {}
};
static __telnet_server__ *telnet_server(nullptr);

void setup()
{
    Serial.begin(115200);

    clients = new __clients__;
    wifi = new __wifi__;
    api = new __api__;

    if (wifi->connect() == WIFI_NOT_CONNECTED)
    {
        wifi->scanNetworks();
    }

    telnet_server = new __telnet_server__;
    telnet_server->init();
}

void loop()
{
    telnet_server->run();
}