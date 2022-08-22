#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "OTAServer.h"
#include "MyWiFi.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "esp_partition.h"
#include "esp_task.h"
#include "esp_ota_ops.h"
#include "freertos/message_buffer.h"
#include "app_inclued.h"
#include "tcp_client.h"

/* FreeRTOS event group to signal when we are connected*/
extern char * tcprx_buffer;
extern MessageBufferHandle_t tcp_send_data;
httpd_handle_t OTA_server;
extern uint8_t ip_addr1,ip_addr2,ip_addr3,ip_addr4;
extern nvs_handle_t my_handle;

extern uint32_t sse_data[sse_len];

void MyWiFi_init(void)
{
	xEventGroupClearBits(APP_event_group,APP_event_WIFI_AP_CONNECTED_BIT | APP_event_WIFI_STA_CONNECTED_BIT | APP_event_tcp_client_send_BIT);
/*	if((sleep_keep & sleep_keep_WIFI_AP_OR_STA_BIT) == sleep_keep_WIFI_AP_OR_STA_BIT)
	{
		//sleep_keep |= sleep_keep_WIFI_AP_OR_STA_BIT;
		init_wifi_softap(&OTA_server);
		printf("Welcome to esp32-c3 AP\r\n");
		tcprx_buffer = "Welcome to esp32-c3 AP";
	}
	else
	{
		//sleep_keep &= ~sleep_keep_WIFI_AP_OR_STA_BIT;
		init_wifi_station(&OTA_server);
		printf("Welcome to esp32-c3 STA.\r\n");
		tcprx_buffer = "Welcome to esp32-c3 STA.";
	}*/
	init_wifi_station(&OTA_server);
	//printf("Welcome to esp32-c3 STA.\r\n");
}

static void ip_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    switch (event_id) {
		case IP_EVENT_STA_GOT_IP: 
		{
 			ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
			/*ip_addr1 = esp_ip4_addr1(&event->ip_info.ip);	
			ip_addr2 = esp_ip4_addr2(&event->ip_info.ip);
			ip_addr3 = esp_ip4_addr3(&event->ip_info.ip);
			ip_addr4 = esp_ip4_addr4(&event->ip_info.ip);	*/	
        	ESP_LOGI("MyWiFi","got ip:" IPSTR, IP2STR(&event->ip_info.ip));ESP_LOGI("MyWiFi","\r\n");
			xEventGroupSetBits(APP_event_group,APP_event_WIFI_STA_CONNECTED_BIT);	
			xEventGroupClearBits(APP_event_group, APP_event_WIFI_AP_CONNECTED_BIT);
			/* Start the web server */
			start_OTA_webserver();
			break;
		}
		case IP_EVENT_STA_LOST_IP: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_STA_LOST_IP\r\n");
			break;
		}
		case IP_EVENT_GOT_IP6: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_GOT_IP6\r\n");
			break;
		}
		case IP_EVENT_ETH_GOT_IP: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_ETH_GOT_IP\r\n");
			break;
		}
		default: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_ OTHER\r\n");
			break;
		}	
    }
    return;
}


static int s_retry_num = 0;
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    switch (event_id) {
		case WIFI_EVENT_WIFI_READY: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_WIFI_READY\r\n");
			break;
		}
		case WIFI_EVENT_SCAN_DONE: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_SCAN_DONE\r\n");
			break;
		}
		case WIFI_EVENT_STA_START: {
			esp_wifi_connect();
			ESP_LOGI("MyWiFi","WIFI_EVENT_STA_START\r\n");
			break;
		}
		case WIFI_EVENT_STA_STOP: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_STA_STOP\r\n");
			break;
		}
		case WIFI_EVENT_STA_CONNECTED: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_STA_CONNECTED\r\n");
			break;
		}
		case WIFI_EVENT_STA_DISCONNECTED: {
			xEventGroupClearBits(APP_event_group,APP_event_WIFI_STA_CONNECTED_BIT);
			if (s_retry_num < 10) 
			{
				esp_wifi_connect();
				s_retry_num++;
				ESP_LOGI("MyWiFi","retry to connect to the AP.(%d / %d)\n", s_retry_num,10);
			} 
			else
			{
				ESP_LOGI("MyWiFi","connect to the AP fail\r\n");
				xEventGroupSetBits(APP_event_group,APP_event_WIFI_AP_CONNECTED_BIT);

				s_retry_num = 0;
				
			}		
			break;
		}
		case WIFI_EVENT_STA_AUTHMODE_CHANGE: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_STA_AUTHMODE_CHANGE\r\n");
			break;
		}
		case WIFI_EVENT_STA_WPS_ER_SUCCESS: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_STA_WPS_ER_SUCCESS\r\n");
			break;
		}
		case WIFI_EVENT_STA_WPS_ER_FAILED: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_STA_WPS_ER_FAILED\r\n");
			break;
		}
		case WIFI_EVENT_STA_WPS_ER_TIMEOUT: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_STA_WPS_ER_TIMEOUT\r\n");
			break;
		}
		case WIFI_EVENT_STA_WPS_ER_PIN: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_STA_WPS_ER_PIN\r\n");
			break;
		}
		case WIFI_EVENT_AP_START: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_AP_START\r\n");
			break;
		}
		case WIFI_EVENT_AP_STOP: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_AP_STOP\r\n");
			break;
		}
		case WIFI_EVENT_AP_STACONNECTED: {
			wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
			/*ip_addr1 = 192;	
			ip_addr2 = 168;
			ip_addr3 = 0;
			ip_addr4 = 1;	*/
        	ESP_LOGI("MyWiFi","station "MACSTR" join, AID=%d\r\n",	\
               MAC2STR(event->mac), event->aid);
			xEventGroupSetBits(APP_event_group, APP_event_WIFI_AP_CONNECTED_BIT);
			xEventGroupClearBits(APP_event_group,APP_event_WIFI_STA_CONNECTED_BIT);
			/* Start the web server */
			start_OTA_webserver();
			break;
		}
		case WIFI_EVENT_AP_STADISCONNECTED: {
			wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        	ESP_LOGI("MyWiFi","station "MACSTR" leave, AID=%d\r\n",	\
                MAC2STR(event->mac), event->aid);
			xEventGroupClearBits(APP_event_group,APP_event_WIFI_AP_CONNECTED_BIT);
			break;
		}
		case WIFI_EVENT_AP_PROBEREQRECVED: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_AP_PROBEREQRECVED\r\n");
			break;
		}
		case WIFI_EVENT_MAX: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_MAX\r\n");
			break;
		}
		default: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_OTHER\r\n");
			//xEventGroupSetBits(APP_event_group, APP_event_WIFI_AP_CONNECTED_BIT);
			//xEventGroupClearBits(APP_event_group,APP_event_WIFI_STA_CONNECTED_BIT);
			//esp_wifi_disconnect();
			//esp_wifi_stop();
			break;
		}	
    }
    return;
}

/* -----------------------------------------------------------------------------
  start_dhcp_server(void)

  Notes:  
  
 -----------------------------------------------------------------------------*/
void start_dhcp_server(void) 
{
	tcpip_adapter_ip_info_t info;
	ESP_LOGI("test", "WIFI_MODE_AP12.");
	// initialize the tcp stack
	//ESP_ERROR_CHECK(esp_netif_init());
	//tcpip_adapter_init();
	ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &info));

	ESP_LOGI("test", "WIFI_MODE_AP13.");
	// stop DHCP server
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
	ESP_LOGI("test", "WIFI_MODE_AP14.");
	// assign a static IP to the network interface

    IP4_ADDR(&info.ip, 192, 168 , 88, 2);
    IP4_ADDR(&info.gw, 192, 168 , 88, 2);
    IP4_ADDR(&info.netmask, 255, 255 , 255, 0); 
	 
	ESP_LOGI("test", "WIFI_MODE_AP15.");
	ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
	
	ESP_LOGI("test", "WIFI_MODE_AP16.");
	// start the DHCP server   
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
	ESP_LOGI("WiFI","DHCP server started \n");
}
/* -----------------------------------------------------------------------------
  printStationList(void)

 print the list of connected stations  
  
 -----------------------------------------------------------------------------*/
void printStationList(void) 
{
	wifi_sta_list_t wifi_sta_list;
	tcpip_adapter_sta_list_t adapter_sta_list;
   
	memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
	memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));

	// Give some time to gather intel of whats connected
	vTaskDelay(500 / portTICK_PERIOD_MS);
	
	ESP_ERROR_CHECK(esp_wifi_ap_get_sta_list(&wifi_sta_list));	
	ESP_ERROR_CHECK(tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list));

	//printf(" Connected Station List:\n");
	//printf("--------------------------------------------------\n");
	
	

	if (adapter_sta_list.num >= 1)
	{
		for (int i = 0; i < adapter_sta_list.num; i++) 
		{
		
			tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
			/*printf("%d - mac: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x - IP:" IPSTR "\n",
				i + 1,
				station.mac[0],
				station.mac[1],
				station.mac[2],
				station.mac[3],
				station.mac[4],
				station.mac[5],
				//ip4addr_ntoa(&(station.ip)));
				IP2STR(&(station.ip)));*/
		}

		//printf("\r\n");
	}
	else
	{
		//printf("No Sations Connected\r\n");
	}

}
/* -----------------------------------------------------------------------------
  init_wifi_softap(void)

  Notes:  
  
  // https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/wifi/esp_wifi.html
  // https://github.com/sankarcheppali/esp_idf_esp32_posts/blob/master/esp_softap/main/esp_softap.c
  
 -----------------------------------------------------------------------------*/
void init_wifi_softap(void *arg)
{
	esp_log_level_set("wifi", ESP_LOG_NONE);    // disable wifi driver logging
		
	//ESP_ERROR_CHECK(esp_netif_init());
	tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
	
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	//ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	
	
	// configure the wifi connection and start the interface
	wifi_config_t ap_config = {
		.ap = {
			.ssid = CONFIG_AP_SSID,
		.password = CONFIG_AP_PASSPHARSE,
		.ssid_len = 0,
		.channel = 0,
		.authmode = AP_AUTHMODE,
		.ssid_hidden = AP_SSID_HIDDEN,
		.max_connection = AP_MAX_CONNECTIONS,
		.beacon_interval = AP_BEACON_INTERVAL,			
		},
	};
	
	
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
	
	ESP_ERROR_CHECK(esp_wifi_start());

	start_dhcp_server();
	
	//printf("ESP WiFi started in AP mode \n");
	
	
	 
	//  Spin up a task to show who connected or disconected
	 
	 
	//xTaskCreate(&print_sta_info, "print_sta_info", 4096, NULL, 1, NULL);

	// https://demo-dijiudu.readthedocs.io/en/latest/api-reference/wifi/esp_wifi.html#_CPPv225esp_wifi_set_max_tx_power6int8_t
	// This can only be placed after esp_wifi_start();
	ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(8));
	
}
/* -----------------------------------------------------------------------------
  init_wifi_station(void)

  Notes:  
  
  // https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/wifi/esp_wifi.html
  
 -----------------------------------------------------------------------------*/
void init_wifi_station(void *arg)
{
	esp_log_level_set("wifi", ESP_LOG_NONE);     // disable wifi driver logging

	//ESP_ERROR_CHECK(esp_netif_init());
	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
	
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &ip_event_handler,
                                                        NULL,
                                                        NULL));


	//ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	
	
	wifi_config_t wifi_config = {
		.sta = {
			.ssid = CONFIG_STATION_SSID,
		.password = CONFIG_STATION_PASSPHRASE
		},
	};

	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI("WiFi", "Station Set to SSID:%s Pass:%s\r\n", CONFIG_STATION_SSID, CONFIG_STATION_PASSPHRASE);

}

/* -----------------------------------------------------------------------------
  print_sta_info(void)

  Notes:  
 
 -----------------------------------------------------------------------------*/
void print_sta_info(void *pvParam) 
{
	//printf("print_sta_info task started \n");
	
	while (1) 
	{	
		EventBits_t staBits = xEventGroupWaitBits(APP_event_group, APP_event_WIFI_STA_CONNECTED_BIT | APP_event_WIFI_AP_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
		
		if (staBits != 0)
		{
			//printf("New station connected\n\n");
		}
		else
		{
			//printf("A station disconnected\n\n");
		}
		
		printStationList();
	}
}

void wifi_ap_sta(void *pvParam)
{
	EventBits_t staBits;
	for(;;)
	{
		vTaskDelay(400000/portTICK_PERIOD_MS);
		staBits = xEventGroupGetBits(APP_event_group);
		if((staBits & APP_event_WIFI_STA_CONNECTED_BIT) != APP_event_WIFI_STA_CONNECTED_BIT)
		{		
			//ESP_ERROR_CHECK(esp_wifi_start());
			//ESP_LOGI("WiFi", "Station Set to SSID:%s Pass:%s\r\n", CONFIG_STATION_SSID, CONFIG_STATION_PASSPHRASE);
			//esp_wifi_stop();
			esp_wifi_connect();
			printf("Connectiing To SSID:%s : Pass:%s\r\n", CONFIG_STATION_SSID, CONFIG_STATION_PASSPHRASE);
		}
		
		//	stop_OTA_webserver(OTA_server);
		
	}
/*	
	if((sleep_keep & sleep_keep_WIFI_AP_OR_STA_BIT) == sleep_keep_WIFI_AP_OR_STA_BIT)
	{
		staBits = xEventGroupWaitBits(APP_event_group, \
			APP_event_WIFI_AP_CONNECTED_BIT, \
			pdFALSE,                               \
			pdFALSE,                               \
			portMAX_DELAY);
		xMessageBufferReset(tcp_send_data);
		xTaskCreate(tcp_client_task, "tcp_client", 3072, NULL,ESP_TASK_PRIO_MIN + 1, NULL);
	
		//printf("Create tcp_client AP\r\n");

		EventBits_t uxBits = xEventGroupWaitBits(APP_event_group, \
									APP_event_tcp_client_send_BIT, \
									pdTRUE,                               \
									pdFALSE,                               \
									30000 / portTICK_PERIOD_MS);
		if((uxBits & APP_event_tcp_client_send_BIT) != 0)
		{
			xMessageBufferSend(tcp_send_data,tcprx_buffer,strlen(tcprx_buffer), 1000 / portTICK_PERIOD_MS);
		}
		tcp_client_send(ip_addr1);
		tcp_client_send(ip_addr2);
		tcp_client_send(ip_addr3);
		tcp_client_send(ip_addr4);
	}
	else
	{
		staBits = xEventGroupWaitBits(APP_event_group, \
			APP_event_WIFI_STA_CONNECTED_BIT, \
			pdFALSE,                               \
			pdFALSE,                               \
			200000 / portTICK_PERIOD_MS);
		if((staBits & APP_event_WIFI_STA_CONNECTED_BIT) != 0)
		{
			xMessageBufferReset(tcp_send_data);
			xTaskCreate(tcp_client_task, "tcp_client", 3072, NULL,ESP_TASK_PRIO_MIN + 1, NULL);	

			//printf("Create tcp_client STA.\r\n");

			EventBits_t uxBits = xEventGroupWaitBits(APP_event_group, \
										APP_event_tcp_client_send_BIT, \
										pdTRUE,                               \
										pdFALSE,                               \
										30000 / portTICK_PERIOD_MS);
			if((uxBits & APP_event_tcp_client_send_BIT) != 0)
			{
				xMessageBufferSend(tcp_send_data,tcprx_buffer,strlen(tcprx_buffer), 1000 / portTICK_PERIOD_MS);
			}
			
			tcp_client_send(ip_addr1);
			tcp_client_send(ip_addr2);
			tcp_client_send(ip_addr3);
			tcp_client_send(ip_addr4);
		}
		else
		{
			sleep_keep |= sleep_keep_WIFI_AP_OR_STA_BIT;
    		//printf("wifi Switch to AP \r\n");
			vTaskDelay(100 / portTICK_PERIOD_MS);
			xEventGroupSetBits(APP_event_group,APP_event_deepsleep_BIT);
		}
	}
	vTaskDelete(NULL);
	*/
}
#if 1

///////////////////////////////////////////////////////////////////////////////////////////////////////
static void WIFI_Mode_Save(wifi_mode_t WifiMode)
{
	// Open
	printf("\n");
	printf("Opening Non-Volatile Storage (NVS) handle... ");
	esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
		printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
	} else {
		printf("Done\n");
	}   

	// Write
	printf("Updating restart counter in NVS ... ");
	
	
	err=nvs_set_i8(my_handle,"wifi_mode",WifiMode);
	printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

	

	// Commit written value.
	// After setting any values, nvs_commit() must be called to ensure changes are written
	// to flash storage. Implementations may write to storage at other times,
	// but this is not guaranteed.
	printf("Committing updates in NVS ... ");
	err = nvs_commit(my_handle);
	printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

	// Close
	nvs_close(my_handle);
}
static wifi_mode_t WIFI_Mode_Check(void)
{
    wifi_mode_t enWifiMode = WIFI_MODE_MAX;
    int8_t u8ProvAvlFlag = 1;

    // Open
    //printf("\n");
    //printf("Opening Non-Volatile Storage (NVS) handle... ");
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) 
    {
       //printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else 
    {
        //printf("Done\n");

        // Read
        //printf("Reading restart counter from NVS ... \n");
        
        err = nvs_get_i8(my_handle, "wifi_mode", &u8ProvAvlFlag);
        switch (err) 
        {
            case ESP_OK:
                //printf("Done\n");
				switch (u8ProvAvlFlag){
					case WIFI_MODE_NULL:
						enWifiMode = WIFI_MODE_NULL;
					break;
					case WIFI_MODE_STA:
						enWifiMode = WIFI_MODE_STA;
					break;
					case WIFI_MODE_AP:
						enWifiMode = WIFI_MODE_AP;
					break;
					case WIFI_MODE_APSTA:
						enWifiMode = WIFI_MODE_APSTA;
					break;
					case WIFI_MODE_MAX:
						enWifiMode = WIFI_MODE_MAX;
					break;
					default :
						enWifiMode = WIFI_MODE_AP;
					break;
				}
                printf("wifi_mode:%d\n",enWifiMode);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                //printf("The value is not initialized yet!\n");
                break;
            default :
                //printf("Error (%s) reading!\n", esp_err_to_name(err));
                break;
        }       
        // Close
        nvs_close(my_handle);
    }
    return enWifiMode;
}
/*************************************************
 * Function Implementation (External and Local)
 ************************************************/
static void WIFI_vInitCommon(void)
{
	// Set the default wifi logging
	//esp_log_level_set("wifi", ESP_LOG_ERROR);
    //esp_log_level_set("wifi", ESP_LOG_WARN);
    //esp_log_level_set("wifi", ESP_LOG_INFO);

	/* Initialize TCP/IP 
	 * Alternative for ESP_ERROR_CHECK(esp_netif_init()); 
	 */
	tcpip_adapter_init();

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
	/* Create the event group to handle wifi events */


    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

	/* Initialize Wi-Fi */
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
}


static void WIFI_vConfigSoftAP(void *arg)
{
	// configure the wifi connection and start the interface
	wifi_config_t ap_config = {
		.ap = {
			.ssid = CONFIG_AP_SSID,
		.password = CONFIG_AP_PASSPHARSE,
		.ssid_len = 0,
		.channel = 0,
		.authmode = AP_AUTHMODE,
		.ssid_hidden = AP_SSID_HIDDEN,
		.max_connection = AP_MAX_CONNECTIONS,
		.beacon_interval = AP_BEACON_INTERVAL		
		},
	};
	
	
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));


	

	ESP_LOGI("test", "WIFI_MODE_AP11.");
	//printf("ESP WiFi started in AP mode \n");
	
	
	 
	//  Spin up a task to show who connected or disconected
	 
	 
	//xTaskCreate(&print_sta_info, "print_sta_info", 4096, NULL, 1, NULL);

	// https://demo-dijiudu.readthedocs.io/en/latest/api-reference/wifi/esp_wifi.html#_CPPv225esp_wifi_set_max_tx_power6int8_t
	// This can only be placed after esp_wifi_start();
	//ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(8));
	ESP_LOGI("test", "WIFI_MODE_AP12.");
}

static void WIFI_vConfigStation(void *arg)
{
	size_t len;
	char ssid[32];      
    char password[64];

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = CONFIG_STATION_SSID,
			.password = CONFIG_STATION_PASSPHRASE,

			/* Setting a password implies station will connect to all security modes including WEP/WPA.
			* However these modes are deprecated and not advisable to be used. Incase your Access point
			* doesn't support WPA2, these mode can be enabled by commenting below line */
			//.threshold.authmode = WIFI_AUTH_WPA2_PSK,
			.pmf_cfg = {
				.capable = true,
				.required = false
			},
		},
	};

	// Open
    //printf("\n");
    //printf("Opening Non-Volatile Storage (NVS) handle... ");

    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) 
    {
       //printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else 
    {
        //printf("Done\n");

        // Read
        //printf("Reading restart counter from NVS ... \n");
        len = sizeof(ssid);
        err = nvs_get_str(my_handle, "wifissid", ssid, &len);
        switch (err) 
        {
            case ESP_OK:
                //printf("Done\n");
				strcpy((char *)wifi_config.sta.ssid, ssid);
                printf("wifissid:(%s)\n",wifi_config.sta.ssid);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                //printf("The value is not initialized yet!\n");
                break;
            default :
                //printf("Error (%s) reading!\n", esp_err_to_name(err));
                break;
        }
        len = sizeof(password);
        err = nvs_get_str(my_handle, "wifipass", password, &len);
        switch (err) 
        {
            case ESP_OK:
                //printf("Done\n");
				strcpy((char *)wifi_config.sta.password,password);
                printf("wifipass:(%s)\n",wifi_config.sta.password);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                //printf("The value is not initialized yet!\n");
                break;
            default :
                //printf("Error (%s) reading!\n", esp_err_to_name(err));
                break;
        }

        

        // Close
        nvs_close(my_handle);
    }
	
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	
	ESP_LOGI("WiFi", "Station Set to SSID:(%s) Pass:(%s)\r\n", wifi_config.sta.ssid, wifi_config.sta.password);
}


void vTaskWifiHandler( void * pvParameters )
{
    wifi_mode_t enWifiMode;
	esp_netif_t * my_netif; 
	xEventGroupClearBits(APP_event_group,APP_event_WIFI_AP_CONNECTED_BIT | APP_event_WIFI_STA_CONNECTED_BIT | APP_event_tcp_client_send_BIT);

	enWifiMode = WIFI_Mode_Check();
	enWifiMode = WIFI_MODE_AP;
    /* Initializing Wi-Fi Station / AccessPoint*/
    WIFI_vInitCommon();
    switch(enWifiMode)
    {
		case WIFI_MODE_NULL:
		break;
		case WIFI_MODE_APSTA:
		break;
		case WIFI_MODE_MAX:
		break;

        case WIFI_MODE_STA:
			ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            WIFI_vConfigStation(&OTA_server);
			if((xEventGroupWaitBits(APP_event_group, APP_event_WIFI_STA_CONNECTED_BIT, pdFALSE, pdFALSE, (1000 / portTICK_PERIOD_MS)/*min*/ * 220) & APP_event_WIFI_STA_CONNECTED_BIT) != APP_event_WIFI_STA_CONNECTED_BIT){
				xEventGroupSetBits(APP_event_group,APP_event_WIFI_AP_CONNECTED_BIT);
				xEventGroupClearBits(APP_event_group,APP_event_WIFI_STA_CONNECTED_BIT);
			}
        break;

        case WIFI_MODE_AP:
			ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
            WIFI_vConfigSoftAP(&OTA_server);
        break;

        default:
        break;
    }
	/* Start Wi-Fi Station / AccessPoint*/
	ESP_ERROR_CHECK(esp_wifi_start());
	//ESP_LOGI(TAG, "Main task: waiting for connection to the wifi network... ");
	start_dhcp_server();
	sse_data[4] |= BIT4;
	vTaskDelay(60000 / portTICK_PERIOD_MS);

    for(;;)
	{
		
        if(enWifiMode == WIFI_MODE_STA)
        {
            //sse_data[4] &= ~BIT4;
            // wait for connection or disconnect event
            EventBits_t bits = xEventGroupWaitBits(APP_event_group, APP_event_WIFI_AP_CONNECTED_BIT, pdFALSE, pdFALSE, (100 / portTICK_PERIOD_MS));
            ESP_LOGI("test", "WIFI_MODE_AP0.");
			if ((bits & APP_event_WIFI_AP_CONNECTED_BIT) == APP_event_WIFI_AP_CONNECTED_BIT) 
            {
				enWifiMode = WIFI_MODE_AP;
				ESP_LOGI("test", "WIFI_MODE_AP5.");
				WIFI_Mode_Save(enWifiMode);
				ESP_LOGI("test", "WIFI_MODE_AP4.");
				stop_OTA_webserver(OTA_server);
				ESP_LOGI("test", "WIFI_MODE_AP1.");
				//esp_wifi_disconnect();
				ESP_LOGI("test", "WIFI_MODE_AP2.");
                //If connection is lost clear the provisioning available flag and stop wifi and re-start in AccessPoint Mode
                //ESP_LOGI(TAG, "Connection lost! Stopping Wifi to switch to AccessPoint Mode.");
                ESP_ERROR_CHECK(esp_wifi_stop());
				ESP_LOGI("test", "WIFI_MODE_AP3.");

				vTaskDelay(100 / portTICK_PERIOD_MS);
				//esp_netif_destroy(my_netif);
				ESP_LOGI("test", "WIFI_MODE_AP6.");
				vTaskDelay(100 / portTICK_PERIOD_MS);
				//my_netif = esp_netif_create_default_wifi_ap();
				ESP_LOGI("test", "WIFI_MODE_AP7.");
                ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
				ESP_LOGI("test", "WIFI_MODE_AP8.");
                WIFI_vConfigSoftAP(&OTA_server);
				ESP_LOGI("test", "WIFI_MODE_AP9.");
				ESP_ERROR_CHECK(esp_wifi_start());
				start_dhcp_server();
			}
            else 
            {
                ESP_LOGI("MyWiFi","enWifiMode == WIFI_MODE_STA\r\n");
            }
        }
        else if(enWifiMode == WIFI_MODE_AP)
        {
			//sse_data[4] |= BIT4;
            enWifiMode = WIFI_Mode_Check();
            ESP_LOGI("MyWiFi","WIFI_Mode_Check: %d",enWifiMode);
            //received new Provisioning Data, but not tested yet.
            if(enWifiMode == WIFI_MODE_STA)
            {
				stop_OTA_webserver(OTA_server);
				//esp_wifi_disconnect();
				// stop DHCP server
				ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
                ESP_LOGI("MyWiFi","New Provisioning data received. Stopping Wifi to switch to Station Mode.");
                ESP_ERROR_CHECK(esp_wifi_stop());
				vTaskDelay(100 / portTICK_PERIOD_MS);
				//esp_netif_destroy(my_netif);
				vTaskDelay(100 / portTICK_PERIOD_MS);
				//my_netif = esp_netif_create_default_wifi_sta();
                ESP_LOGI("MyWiFi","Wifi Station initializing!");
                ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
                WIFI_vConfigStation(&OTA_server);
                ESP_ERROR_CHECK(esp_wifi_start());
				if((xEventGroupWaitBits(APP_event_group, APP_event_WIFI_STA_CONNECTED_BIT, pdFALSE, pdFALSE, (1000 / portTICK_PERIOD_MS)/*min*/ * 220) & APP_event_WIFI_STA_CONNECTED_BIT) != APP_event_WIFI_STA_CONNECTED_BIT){
					xEventGroupSetBits(APP_event_group,APP_event_WIFI_AP_CONNECTED_BIT);
					xEventGroupClearBits(APP_event_group,APP_event_WIFI_STA_CONNECTED_BIT);
				}
            }
            else
            {
				ESP_LOGI("MyWiFi","enWifiMode == WIFI_MODE_AP\r\n");
            }	
        }
		vTaskDelay(10000 / portTICK_PERIOD_MS);



	}
    vTaskDelete(NULL);
}
#endif