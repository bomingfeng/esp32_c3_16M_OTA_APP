/* LwIP SNTP example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sntp_task.h"

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

extern uint32_t sse_data[sse_len];

MessageBufferHandle_t time_hour_min;

static void obtain_time(void);
static void initialize_sntp(void);

void sntp_task(void *pvParam)
{
    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];
    uint16_t hour_min;
    xEventGroupWaitBits(APP_event_group, \
                APP_event_WIFI_STA_CONNECTED_BIT, \
                pdFALSE,                               \
                pdFALSE,                               \
                portMAX_DELAY);
    //time(&now);
    //localtime_r(&now, &timeinfo);
    //ESP_LOGI("sntp", "Time is not set yet. Connecting to WiFi and getting time over NTP.");
    obtain_time();

    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
        printf("Waiting for system time to be set...\r\n");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    // update 'now' variable with current time
    //time(&now);
    // Set timezone to Eastern Standard Time and print local time
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    //localtime_r(&now, &timeinfo);
    //strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    //ESP_LOGI("sntp", "The current date/time in New York is: %s", strftime_buf);

    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    while(1)
    {
        time(&now);

        sse_data[0] = now; 

        localtime_r(&now, &timeinfo);
        hour_min = (timeinfo.tm_hour << 8) | timeinfo.tm_min;
        xMessageBufferSend(time_hour_min,&hour_min,2,portMAX_DELAY);

        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        //printf("The current date/time in Shanghai is: %s.\r\n", strftime_buf);
        vTaskDelay(8000 / portTICK_PERIOD_MS);
    }
/*
    if (sntp_get_sync_mode() == SNTP_SYNC_MODE_SMOOTH) {
        struct timeval outdelta;
        while (sntp_get_sync_status() == SNTP_SYNC_STATUS_IN_PROGRESS) {
            adjtime(NULL, &outdelta);
            ESP_LOGI("sntp", "Waiting for adjusting time ... outdelta = %li sec: %li ms: %li us",
                        (long)outdelta.tv_sec,
                        outdelta.tv_usec/1000,
                        outdelta.tv_usec%1000);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
*/
}

static void obtain_time(void)
{
    /**
     * NTP server address could be aquired via DHCP,
     * see following menuconfig options:
     * 'LWIP_DHCP_GET_NTP_SRV' - enable STNP over DHCP
     * 'LWIP_SNTP_DEBUG' - enable debugging messages
     *
     * NOTE: This call should be made BEFORE esp aquires IP address from DHCP,
     * otherwise NTP option would be rejected by default.
     */
#ifdef LWIP_DHCP_GET_NTP_SRV
    sntp_servermode_dhcp(1);      // accept NTP offers from DHCP server, if any
#endif

    initialize_sntp();



}

static void initialize_sntp(void)
{
    ESP_LOGI("sntp", "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);

/*
 * If 'NTP over DHCP' is enabled, we set dynamic pool address
 * as a 'secondary' server. It will act as a fallback server in case that address
 * provided via NTP over DHCP is not accessible
 */
#if LWIP_DHCP_GET_NTP_SRV && SNTP_MAX_SERVERS > 1
    sntp_setservername(1, "pool.ntp.org");

#if LWIP_IPV6 && SNTP_MAX_SERVERS > 2          // statically assigned IPv6 address is also possible
    ip_addr_t ip6;
    if (ipaddr_aton("2a01:3f7::1", &ip6)) {    // ipv6 ntp source "ntp.netnod.se"
        sntp_setserver(2, &ip6);
    }
#endif  /* LWIP_IPV6 */

#else   /* LWIP_DHCP_GET_NTP_SRV && (SNTP_MAX_SERVERS > 1) */
    // otherwise, use DNS address from a pool
    sntp_setservername(0, "pool.ntp.org");
    sntp_setservername(1, "cn.ntp.org.cn");
    sntp_setservername(2, "edu.ntp.org.cn");
#endif


#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    sntp_init();

    ESP_LOGI("sntp", "List of configured NTP servers:");

    for (uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i){
        if (sntp_getservername(i)){
            ESP_LOGI("sntp", "server %d: %s", i, sntp_getservername(i));
        } else {
            // we have either IPv4 or IPv6 address, let's print it
            char buff[INET6_ADDRSTRLEN];
            ip_addr_t const *ip = sntp_getserver(i);
            if (ipaddr_ntoa_r(ip, buff, INET6_ADDRSTRLEN) != NULL)
                ESP_LOGI("sntp", "server %d: %s", i, buff);
        }
    }
}
