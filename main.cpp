#include "Arduino.h"
// #include <stdio.h>

// #include <Arduino.h>

// #include <WiFi.h>
// #include <vector>
// #include <ArduinoOTA.h>
// #include <HTTPClient.h>
// #include <esp_wifi.h>
// #include "nvs_flash.h"
// #include <stdio.h>
// #include <base64.h>
// #include <lwip/sockets.h>
// #include <lwip/netdb.h>
// #include <sstream>
// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/gpio.h"
// #include "esp_log.h"
// #include "sdkconfig.h"

// #define BLINK_GPIO (gpio_num_t)5
// #define CONFIG_BLINK_PERIOD 1000
// static uint8_t s_led_state = 0;

// static void blink_led(void)
// {
//     /* Set the GPIO level according to the state (LOW or HIGH)*/
//     gpio_set_level(BLINK_GPIO, s_led_state);
// }

// static void configure_led(void)
// {
//     gpio_reset_pin(BLINK_GPIO);
//     gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
// }

// static void blink_for(int __iter, int __period)
// {
//     for (int i = 0; i < (__iter + 1); ++i)
//     {
//         blink_led();
//         s_led_state = !s_led_state;
//         vTaskDelay(__period / portTICK_PERIOD_MS);
//     }
// }

// extern "C" void app_main()
// {
//     // initArduino();
//     // pinMode(4, OUTPUT);
//     // digitalWrite(4, HIGH);
//     // // Do your own thing

//     printf("hello\n");

//     configure_led();
//     blink_for(20, 20000);
// }

#include <stdio.h>
#include "stdio.h"
#include "common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "spi_flash_mmap.h"
#include "lwip/sockets.h"
#include <esp_log.h>
#include "lwip/sys.h"
#include <string.h>
#include "driver/gpio.h"
#include "sdkconfig.h"

#include <WiFi.h>
#include <vector>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include "nvs_flash.h"
#include <stdio.h>
#include <base64.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <sstream>

using namespace std;

class __esp__
{
    public:
        void initialize_nvs(void) const
        {
            esp_err_t err = nvs_flash_init();
            if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
            {
                ESP_ERROR_CHECK(nvs_flash_erase());
                err = nvs_flash_init();
            }
            ESP_ERROR_CHECK(err);
        }


    public:
        __esp__() {}
};
static __esp__ *esp(nullptr);

class __blink__
{
    static constexpr char *TAG = "LED";

    #define BLINK_GPIO (gpio_num_t)5
    #define CONFIG_BLINK_PERIOD 1000

    uint8_t s_led_state;

    private:
        void blink_led(void)
        {
            /* Set the GPIO level according to the state (LOW or HIGH)*/
            gpio_set_level(BLINK_GPIO, s_led_state);
        }

        static void configure_led(void)
        {
            ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
            gpio_reset_pin(BLINK_GPIO);
            /* Set the GPIO as a push/pull output */
            gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
        }

    public:
        void for_iter(int __iter, int __period)
        {
            for (int i = 0; i < (__iter + 1); ++i)
            {
                ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
                blink_led();
                /* Toggle the LED state */
                s_led_state = !s_led_state;
                vTaskDelay(__period / portTICK_PERIOD_MS);
            }
        }

        __blink__()
        {
            s_led_state = 0;
            configure_led();
        }
};
static __blink__ *blink(nullptr);

#define TELNET_PORT 23
#define MAX_CONN 1 // Maximum number of simultaneous connections
#define SUCCESSFULL_CONNECTION_MSG "Connected.\n\r"

static void telnet_server_task(void *pvParameters)
{
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(TELNET_PORT);
    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    listen(listen_sock, MAX_CONN);

    while (true)
    {
        struct sockaddr_in source_addr;
        socklen_t addr_len = sizeof(source_addr);
        int sock;
        if((sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len)) < 0)
        {
            ESP_LOGE("Telnet Server", "Unable to accept connection: errno %d", errno);
            break;
        }
        ESP_LOGI("Telnet Server", "Socket accepted");
        send(sock, SUCCESSFULL_CONNECTION_MSG, sizeof(SUCCESSFULL_CONNECTION_MSG), 0);

        // Handle the connection
        char rx_buffer[128];
        while (true)
        {
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len < 0)
            {
                ESP_LOGE("Telnet Server", "recv failed: errno %d", errno);
                break;
            }
            else if (len == 0)
            {
                ESP_LOGI("Telnet Server", "Connection closed");
                break;
            }
            else
            {
                // Null-terminate whatever we received and treat it like a string
                rx_buffer[len] = 0;
                ESP_LOGI("Telnet Server", "Received %d bytes: %s", len, rx_buffer);

                // Echo back received data
                // send(sock, rx_buffer, len, 0);
            }
        }

        if (sock != -1)
        {
            ESP_LOGI("Telnet Server", "Shutting down socket");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

extern "C" void start_telnet_server()
{
    xTaskCreate(telnet_server_task, "telnet_server_task", 4096, NULL, 5, NULL);
}

#define MMU_PAGE_SIZE_KB 64
#define KB_TO_MB_DIVISOR 1024
#define INST 1
#define FLASH 2

extern "C" void get_flash_size(int __type)
{
    if (__type & INST)
    {
        uint32_t freePages = spi_flash_mmap_get_free_pages(SPI_FLASH_MMAP_INST);
        uint32_t freeFlashSizeKB = freePages *MMU_PAGE_SIZE_KB;
        float freeFlashSizeMB = (float)freeFlashSizeKB / KB_TO_MB_DIVISOR;
        ESP_LOGI("get_flash_size", "Free Instruction Memory Size %.2f MB", freeFlashSizeMB);
    }
    
    if (__type & FLASH)
    {
        uint32_t freePages = spi_flash_mmap_get_free_pages(SPI_FLASH_MMAP_DATA);
        uint32_t freeFlashSizeKB = freePages *MMU_PAGE_SIZE_KB;
        float freeFlashSizeMB = (float)freeFlashSizeKB / KB_TO_MB_DIVISOR;
        ESP_LOGI("get_flash_size", "Free Flash Memory Size: %.2f MB", freeFlashSizeMB);
    }
}

class __wifi__
{
    #define WIFI_CONNECTED true
    #define WIFI_NOT_CONNECTED false
    #define IS_WIFI_CONNECTED wifi->is_wifi_connected()

    private:
        wifi_config_t wifi_config;

        void set_wifi_config(const char* ssid, const char* password)
        {   
            // Ensure that you do not exceed the maximum length for SSID and password.
            // For example, ESP32 SSID max length is 32 characters including the null terminator.
            // Password max length is 64 characters including the null terminator.
            
            memset(&this->wifi_config, 0, sizeof(this->wifi_config)); // Initialize the structure to zeros
            
            memcpy(this->wifi_config.sta.ssid, ssid, strlen(ssid) + 1); // +1 to include null terminator
            memcpy(this->wifi_config.sta.password, password, strlen(password) + 1); // +1 to include null terminator

            // Now, wifi_config.sta.ssid and wifi_config.sta.password are set with your values
        }

    public:
        void esp_connect()
        {
            uint16_t ret = nvs_flash_init();
            if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
            {
                if (nvs_flash_erase())
                {
                    printf("nvs_flash_erase Failed.");
                }
                ret = nvs_flash_init();
            }
            ESP_ERROR_CHECK(ret);
            ESP_ERROR_CHECK(esp_netif_init());
            ESP_ERROR_CHECK(esp_event_loop_create_default());

            esp_netif_create_default_wifi_sta();

            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK(esp_wifi_init(&cfg));

            set_wifi_config("comhem_3E2192", "26849bzs");

            // wifi_config.sta =
            // {
            //     .ssid = "comhem_3E2192",
            //     .password = "26849bzs"
            // };

            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
            ESP_ERROR_CHECK(esp_wifi_start());
            
            esp_err_t err = esp_wifi_connect();
            if (err == ESP_OK)
            {
                blink->for_iter(3, 1000);
            }
            else if (err == ESP_ERR_WIFI_SSID)
            {
                blink->for_iter(10, 250);
            }
        }

        bool connect()
        {
            WiFi.begin("comhem_3E2192", "26849bzs");
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
            Serial.println("Done!!!");
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
                if (WiFi.SSID(i).end() == string(" "))
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

        bool is_wifi_connected() const
        {
            if (WiFi.status() == WL_CONNECTED) return WIFI_CONNECTED;

            return WIFI_NOT_CONNECTED;
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
        void init()
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

        void handle() const
        {
            ArduinoOTA.handle();
        }

    public:
        __ota__() {}
};
static __ota__ *ota(nullptr);

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

        int send__(const string &__str) const
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
                Serial.println("Telnet Server: Unable to accept connection");
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
                if (accept__() == ACCEPT_FAILED) break;

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

                    ss << '\n';
                    printf(ss.str().c_str());
                    // Serial.println(ss.str().c_str());
                    ss.clear();
                }

                if (sock != -1)
                {
                    // Serial.println("Telnet Server: Shutting down socket.");
                    printf("Telnet Server: Shutting down socket.\n");
                    shutdown(sock, 0);
                    close(sock);
                }
            }
        }

    public:
        __telnet_server__() {}
};
static __telnet_server__ *telnet_server(nullptr);

enum Type_Mask
{
    CLIENTS       = 1 << 0,
    API           = 1 << 1,
    WIFI          = 1 << 2,
    TELNET_SERVER = 1 << 3,
    BLINK         = 1 << 4,
    OTA           = 1 << 5,
    ESP_BASE      = 1 << 6
};

void init_class_memory(int __type_mask)
{
    if (__type_mask & CLIENTS)
    {
        clients = new __clients__;
    }

    if (__type_mask & API)
    {
        api = new __api__;
    }

    if (__type_mask & WIFI)
    {
        wifi = new __wifi__;
    }

    if (__type_mask & TELNET_SERVER)
    {
        telnet_server = new __telnet_server__;
    }

    if (__type_mask & BLINK)
    {
        blink = new __blink__;
    }

    if (__type_mask & OTA)
    {
        ota = new __ota__;
    }

    if (__type_mask & ESP_BASE)
    {
        esp = new __esp__;
    }
}

extern "C" void app_main()
{
    init_class_memory(BLINK | TELNET_SERVER | WIFI);
    Serial.begin(115200);

    initArduino();
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);

    wifi->esp_connect();
    
    get_flash_size(FLASH | INST);

    sleep(2);
    blink->for_iter(40, 100);

    telnet_server->init();

    while (true)
    {
        telnet_server->run();
    }
}