#pragma once

#include <esp_http_server.h>



void OTA_Task_init(void);
void systemRebootTask(void * parameter);
	
httpd_handle_t start_OTA_webserver(void);
void stop_OTA_webserver(httpd_handle_t server);




