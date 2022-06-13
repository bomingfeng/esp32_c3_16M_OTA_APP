/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "freertos/message_buffer.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "driver/gpio.h"

#include "app_inclued.h"
#include "tcp_client.h"

#if defined(CONFIG_EXAMPLE_IPV4)
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
#elif defined(CONFIG_EXAMPLE_IPV6)
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV6_ADDR
#else
#define HOST_IP_ADDR ""
#endif

#define AP_HOST_IP_ADDR CONFIG_AP_EXAMPLE_IPV4_ADDR

#define PORT CONFIG_EXAMPLE_PORT

static const char *TAG = "example";
//static const char *payload = "Message from ESP32 ";

MessageBufferHandle_t tcp_send_data;
char * tcprx_buffer;

void tcp_client_send(uint32_t data)
{
    char senddata[8];
    uint8_t i;
    senddata[0] = (data & 0XF0000000) >> 28;senddata[1] = (data & 0X0F000000) >> 24;
    senddata[2] = (data & 0X00F00000) >> 20;senddata[3] = (data & 0X000F0000) >> 16;
    senddata[4] = (data & 0X0000F000) >> 12;senddata[5] = (data & 0X00000F00) >> 8;
    senddata[6] = (data & 0X000000F0) >> 4;senddata[7] = data & 0x0000000F;
    for(i = 0;i < 8;i++)
    {
        if(senddata[i] < 10)
            senddata[i] += 0x30;
        else
            senddata[i] = senddata[i] - 10 + 'A';
    }

    tcprx_buffer = (char*)&senddata;
    EventBits_t uxBits = xEventGroupWaitBits(APP_event_group, \
			APP_event_tcp_client_send_BIT, \
			pdTRUE,                               \
			pdFALSE,                               \
			1000 / portTICK_PERIOD_MS);
    if((uxBits & APP_event_tcp_client_send_BIT) != 0)
    {
        xMessageBufferSend(tcp_send_data,tcprx_buffer,8, 1000 / portTICK_PERIOD_MS);
    }
}


 void tcp_client_task(void *pvParameters)
{
    char rx_buffer[128];
    size_t xReceivedBytes;
    int addr_family = 0;
    int ip_protocol = 0;
    char AP_host_ip[] = AP_HOST_IP_ADDR;
    char host_ip[] = HOST_IP_ADDR;

    while (1) 
    {
        xEventGroupWaitBits(APP_event_group, \
			APP_event_WIFI_AP_CONNECTED_BIT | APP_event_WIFI_STA_CONNECTED_BIT, \
			pdFALSE,                               \
			pdFALSE,                               \
			portMAX_DELAY);

#if defined(CONFIG_EXAMPLE_IPV4)
        struct sockaddr_in dest_addr;
        if((sleep_keep & sleep_keep_WIFI_AP_OR_STA_BIT) == sleep_keep_WIFI_AP_OR_STA_BIT)
        {
            dest_addr.sin_addr.s_addr = inet_addr(AP_host_ip);
        }
        else
        {
            dest_addr.sin_addr.s_addr = inet_addr(host_ip);
        }        
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
#elif defined(CONFIG_EXAMPLE_IPV6)
        struct sockaddr_in6 dest_addr = { 0 };
        if((sleep_keep & sleep_keep_WIFI_AP_OR_STA_BIT) == sleep_keep_WIFI_AP_OR_STA_BIT)
        {
            inet6_aton(AP_host_ip, &dest_addr.sin6_addr);
        }
        else
        {
            inet6_aton(host_ip, &dest_addr.sin6_addr);
        }         
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(PORT);
        dest_addr.sin6_scope_id = esp_netif_get_netif_impl_index(EXAMPLE_INTERFACE);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
#elif defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
        struct sockaddr_storage dest_addr = { 0 };
        ESP_ERROR_CHECK(get_addr_from_stdin(PORT, SOCK_STREAM, &ip_protocol, &addr_family, &dest_addr));
#endif
        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) 
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            xEventGroupClearBits(APP_event_group,APP_event_tcp_client_send_BIT);
            //break;
        }
        else
        {
            if((sleep_keep & sleep_keep_WIFI_AP_OR_STA_BIT) == sleep_keep_WIFI_AP_OR_STA_BIT)
            {
                ESP_LOGI(TAG, "Socket created, connecting to %s:%d", AP_host_ip, PORT);
            }
            else
            {
                ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, PORT);
            } 
            int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
            if (err != 0) 
            {
                ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
                xEventGroupClearBits(APP_event_group,APP_event_tcp_client_send_BIT);
                //break;
            }
            else
            {
                ESP_LOGI(TAG, "Successfully connected");
                xEventGroupSetBits(APP_event_group,APP_event_tcp_client_send_BIT);
                while (1) 
                {
                    xReceivedBytes = xMessageBufferReceive(tcp_send_data,(void *)rx_buffer,sizeof( rx_buffer),portMAX_DELAY);
                    int err = send(sock,rx_buffer, xReceivedBytes, 0);
                    if (err < 0) 
                    {
                        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                        xEventGroupClearBits(APP_event_group,APP_event_tcp_client_send_BIT);
                        break;
                    }
                    else
                    {
                        xEventGroupSetBits(APP_event_group,APP_event_tcp_client_send_BIT);
                    }
                    /*
                    int err = send(sock, payload, strlen(payload), 0);
                    if (err < 0) {
                        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                        break;
                    }

                    int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
                    // Error occurred during receiving
                    if (len < 0) {
                        ESP_LOGE(TAG, "recv failed: errno %d", errno);
                        break;
                    }
                    // Data received
                    else {
                        rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                        if((sleep_keep & sleep_keep_WIFI_AP_OR_STA_BIT) == sleep_keep_WIFI_AP_OR_STA_BIT)
                        {
                            ESP_LOGI(TAG, "Received %d bytes from %s:", len, AP_host_ip);
                        }
                        else
                        {
                            ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
                        } 
                        
                        ESP_LOGI(TAG, "%s", rx_buffer);
                    }
                    vTaskDelay(2000 / portTICK_PERIOD_MS);
                    */
                }
            }
        }
//        if (sock != -1) 
//        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
//       }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
