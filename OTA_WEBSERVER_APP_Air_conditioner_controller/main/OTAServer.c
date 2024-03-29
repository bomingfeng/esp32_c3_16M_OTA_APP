#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_log.h>
#include <sys/param.h>
#include <esp_http_server.h>
#include "esp_ota_ops.h"
#include "freertos/event_groups.h"
#include "app_inclued.h"

extern httpd_handle_t OTA_server = NULL;
int8_t flash_status = 0;
TimerHandle_t	xTimer_ota;
MessageBufferHandle_t HtmlToMcuData;

// Embedded Files. To add or remove make changes is component.mk file as well. 
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");
extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[]   asm("_binary_favicon_ico_end");
extern const uint8_t jquery_3_4_1_min_js_start[] asm("_binary_jquery_3_4_1_min_js_start");
extern const uint8_t jquery_3_4_1_min_js_end[]   asm("_binary_jquery_3_4_1_min_js_end");
extern TimerHandle_t io_sleep_timers;

void vTimerotaCallback(TimerHandle_t xTimer)
{
	xTimerStop(xTimer_ota,portMAX_DELAY);
	xEventGroupSetBits(APP_event_group,APP_event_tcp_client_send_BIT);

	EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
	if((uxBits & APP_event_io_sleep_timer_BIT) != 0)
	{
		xTimerReset(io_sleep_timers,portMAX_DELAY);
	}
}

void OTA_Task_init(void)
{
	
	gpio_config_t io_conf;
	//interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = (1ULL<<CONFIG_INPUT_GPIO);
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
	//disable pull-down mode
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

	xTimer_ota = xTimerCreate("xTimer_ota",(60000 / portTICK_PERIOD_MS)/*min*/ * 5,pdFALSE,( void * ) 0,vTimerotaCallback);//5min

	// Clear the bit
	xEventGroupClearBits(APP_event_group,APP_event_REBOOT_BIT | APP_event_deepsleep_BIT | APP_event_IO_wakeup_sleep_BIT | APP_event_io_sleep_timer_BIT);

}


/*****************************************************
 
	systemRebootTask()
 
	NOTES: This had to be a task because the web page needed
			an ack back. So i could not call this in the handler
 
 *****************************************************/
void systemRebootTask(void * parameter)
{
	uint8_t keycon = 0;
	const esp_partition_t *partition = NULL;
	for (;;)
	{
		// Wait here until the bit gets set for reboot
		EventBits_t staBits = xEventGroupWaitBits(APP_event_group, APP_event_REBOOT_BIT | APP_event_deepsleep_BIT | APP_event_IO_wakeup_sleep_BIT, pdTRUE,	\
												pdFALSE, 1000 / portTICK_PERIOD_MS);
		// Did portMAX_DELAY ever timeout, not sure so lets just check to be sure
		if ((staBits & APP_event_REBOOT_BIT) != 0)
		{
			ESP_LOGI("OTA", "Reboot Command, Restarting");
			vTaskDelay(3000 / portTICK_PERIOD_MS);
			esp_restart();
		}

		if((staBits & APP_event_deepsleep_BIT) != 0)
		{
			//printf("restart to deep_sleep");
			vTaskDelay(3000 / portTICK_PERIOD_MS);
			esp_deep_sleep(1000000LL * 5);	//2s
		}
/*
		if(gpio_get_level(CONFIG_INPUT_GPIO) == 0x00)
		{
			keycon++;
			if(keycon >= 10)
			{
				xEventGroupSetBits(APP_event_group,APP_event_IR_LED_flags_BIT);
				keycon = 0;
				partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP,    \
				ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
				if (partition == NULL) 
				{
					partition = esp_ota_get_next_update_partition(NULL);
				}
				printf("restart to boot,%d\r\n",gpio_get_level(CONFIG_INPUT_GPIO));
				esp_ota_set_boot_partition(partition);
				xEventGroupSetBits(APP_event_group,APP_event_REBOOT_BIT);	
			}
		}
		else
		{
			keycon = 0;
		}
*/
		/*if((staBits & APP_event_IO_wakeup_sleep_BIT) != 0)
		{
    		const int ext_wakeup_pin_1 = 2;
    		const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin_1;
			printf("Enabling EXT1 wakeup on pins GPIO%d\n", ext_wakeup_pin_1);
            esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask, ESP_EXT1_WAKEUP_ALL_LOW);

			// Isolate GPIO12 pin from external circuits. This is needed for modules
			// which have an external pull-up resistor on GPIO12 (such as ESP32-WROVER)
			// to minimize current consumption.
			rtc_gpio_isolate(GPIO_NUM_12);

			printf("Entering deep sleep\n");
			sleep_keep &= ~sleep_keep_WIFI_AP_OR_STA_BIT;
			esp_deep_sleep_start();
		}*/
	}
		
}



/* Send index.html Page */
esp_err_t OTA_index_html_handler(httpd_req_t *req)
{
	xEventGroupClearBits(APP_event_group,APP_event_tcp_client_send_BIT);
	xTimerStop(io_sleep_timers,portMAX_DELAY);
	xTimerReset(xTimer_ota,portMAX_DELAY);

	ESP_LOGI("OTA", "index.html Requested");

	// Clear this every time page is requested 每次请求页面时清除此内容
	flash_status = 0;
	
	httpd_resp_set_type(req, "text/html");

	httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);

	return ESP_OK;
}

/* Send .ICO (icon) file  */
esp_err_t OTA_favicon_ico_handler(httpd_req_t *req)
{
	ESP_LOGI("OTA", "favicon_ico Requested");
    
	httpd_resp_set_type(req, "image/x-icon");

	httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_end - favicon_ico_start);

	return ESP_OK;
}

/* jquery GET handler */
esp_err_t jquery_3_4_1_min_js_handler(httpd_req_t *req)
{
	ESP_LOGI("OTA", "jqueryMinJs Requested");

	httpd_resp_set_type(req, "application/javascript");

	httpd_resp_send(req, (const char *)jquery_3_4_1_min_js_start, (jquery_3_4_1_min_js_end - jquery_3_4_1_min_js_start)-1);

	return ESP_OK;
}

/* Status */
esp_err_t OTA_update_status_handler(httpd_req_t *req)
{
	char ledJSON[100];
	
	ESP_LOGI("OTA", "Status Requested");

	sprintf(ledJSON, "{\"status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}", flash_status, __TIME__, __DATE__);
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, ledJSON, strlen(ledJSON));
	
	// This gets set when upload is complete
	if (flash_status == 1)
	{
		// We cannot directly call reboot here because we need the 
		// browser to get the ack back. 
		xEventGroupSetBits(APP_event_group,APP_event_REBOOT_BIT);	
	}
	return ESP_OK;
}
/* Receive .Bin file */
esp_err_t OTA_update_post_handler(httpd_req_t *req)
{
	esp_ota_handle_t ota_handle; 
	
	char ota_buff[1024];
	int content_length = req->content_len;
	int content_received = 0;
	int recv_len;
	bool is_req_body_started = false;
	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

	// Unsucessful Flashing 
	flash_status = -1;
	
	do
	{
		/* Read the data for the request */
		if ((recv_len = httpd_req_recv(req, ota_buff, MIN(content_length, sizeof(ota_buff)))) < 0) 
		{
			if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) 
			{
				ESP_LOGI("OTA", "Socket Timeout");
				/* Retry receiving if timeout occurred */
				continue;
			}
			ESP_LOGI("OTA", "OTA Other Error %d", recv_len);
			return ESP_FAIL;
		}

		//printf("OTA RX: %d of %d\r", content_received, content_length);
		
	    // Is this the first data we are receiving 这是我们收到的第一个数据吗？
		// If so, it will have the information in the header we need. 如果是这样，它将在我们需要的标头中包含信息
		if (!is_req_body_started)
		{
			is_req_body_started = true;
			
			// Lets find out where the actual data staers after the header info		
			char *body_start_p = strstr(ota_buff, "\r\n\r\n") + 4;	
			int body_part_len = recv_len - (body_start_p - ota_buff);
			
			//int body_part_sta = recv_len - body_part_len;
			//printf("OTA File Size: %d : Start Location:%d - End Location:%d\r\n", content_length, body_part_sta, body_part_len);
			//printf("OTA File Size: %d\r\n", content_length);

			esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
			if (err != ESP_OK)
			{
				//printf("Error With OTA Begin, Cancelling OTA\r\n");
				return ESP_FAIL;
			}
			else
			{
				//printf("Writing to partition subtype %d at offset 0x%x\r\n", update_partition->subtype, update_partition->address);
			}

			// Lets write this first part of data out
			esp_ota_write(ota_handle, body_start_p, body_part_len);
		}
		else
		{
			// Write OTA data
			esp_ota_write(ota_handle, ota_buff, recv_len);
			
			content_received += recv_len;
		}
 
	} while (recv_len > 0 && content_received < content_length);

	// End response
	//httpd_resp_send_chunk(req, NULL, 0);

	
	if (esp_ota_end(ota_handle) == ESP_OK)
	{
		// Lets update the partition
		if(esp_ota_set_boot_partition(update_partition) == ESP_OK) 
		{
			const esp_partition_t *boot_partition = esp_ota_get_boot_partition();

			// Webpage will request status when complete 
			// This is to let it know it was successful
			flash_status = 1;
		
			ESP_LOGI("OTA", "Next boot partition subtype %d at offset 0x%x", boot_partition->subtype, boot_partition->address);
			ESP_LOGI("OTA", "Please Restart System...");
		}
		else
		{
			ESP_LOGI("OTA", "\r\n\r\n !!! Flashed Error !!!");
		}
		
	}
	else
	{
		ESP_LOGI("OTA", "\r\n\r\n !!! OTA End Error !!!");
	}
	
	return ESP_OK;

}



httpd_uri_t OTA_index_html = {
	.uri = "/",
	.method = HTTP_GET,
	.handler = OTA_index_html_handler,
	/* Let's pass response string in user
	 * context to demonstrate it's usage */
	.user_ctx = NULL
};

httpd_uri_t OTA_favicon_ico = {
	.uri = "/favicon.ico",
	.method = HTTP_GET,
	.handler = OTA_favicon_ico_handler,
	/* Let's pass response string in user
	 * context to demonstrate it's usage */
	.user_ctx = NULL
};
httpd_uri_t OTA_jquery_3_4_1_min_js = {
	.uri = "/jquery-3.4.1.min.js",
	.method = HTTP_GET,
	.handler = jquery_3_4_1_min_js_handler,
	/* Let's pass response string in user
	 * context to demonstrate it's usage */
	.user_ctx = NULL
};

httpd_uri_t OTA_update = {
	.uri = "/update",
	.method = HTTP_POST,
	.handler = OTA_update_post_handler,
	.user_ctx = NULL
};
httpd_uri_t OTA_status = {
	.uri = "/status",
	.method = HTTP_POST,
	.handler = OTA_update_status_handler,
	.user_ctx = NULL
};


esp_err_t HtmlToMcu_handler(httpd_req_t *req)
{
	char ota_buff[96];
	int content_length = req->content_len;
	int content_received = 0;
	int recv_len;
	uint8_t i;
	do
	{
		/* Read the data for the request */
		if ((recv_len = httpd_req_recv(req, ota_buff, MIN(content_length, sizeof(ota_buff)))) < 0) 
		{
			if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) 
			{
				printf("Socket Timeout");
				/* Retry receiving if timeout occurred */
				continue;
			}
			printf("OTA Other Error %d", recv_len);
			return ESP_FAIL;
		}
		else
		{
			content_received += recv_len;
			//for(i = 0;i < content_length;i++)
//				printf("ota_buff:%s;\r\n",ota_buff);
			xMessageBufferSend(HtmlToMcuData,&ota_buff,content_length,portMAX_DELAY);
		}
	} while (recv_len > 0 && content_received < content_length);
	return ESP_OK;
}

httpd_uri_t HtmlToMcu = {
	.uri = "/HtmlToMcu",
	.method = HTTP_POST,
	.handler = HtmlToMcu_handler,
	.user_ctx = NULL
};

uint32_t sse_id = 0x0,sse_data[sse_len] = {0};
esp_err_t McuToHtml_handler(httpd_req_t *req)
{

	httpd_resp_set_type(req, "text/event-stream;charset=utf-8");

	 /* \n是一个字符。buf_len长度要包函\n */
	//httpd_resp_send(req, "id:1\ndata:test\n\n",16);//以data: 开头会默认触发页面中message事件，以\n\n结尾结束一次推送。
	
	//httpd_resp_send(req, "id:1\nevent:foo\ndata:test\n\n",26);//'event:' + 事件名 + '\n'，这样就会触发页面中的foo事件而不是message事件，以\n\n结尾结束一次推送。


	/*转换*/
	char ledJSON[96];
	if(sse_id == 1){
		sprintf(ledJSON, "id:%d\ndata:%d\n\n",sse_id,sse_data[sse_id]);
	}
	else if(sse_id == 2){
		sprintf(ledJSON, "id:%d\ndata:%d\n\n",sse_id,sse_data[sse_id]);
	}
	else if(sse_id == 3){
		sprintf(ledJSON, "id:%d\ndata:%d\n\n",sse_id,sse_data[sse_id]);
	}
	else if(sse_id == 4){
		sprintf(ledJSON, "id:%d\ndata:%d\n\n",sse_id,sse_data[sse_id]);
	}
	else if(sse_id == 5){
		sprintf(ledJSON, "id:%d\ndata:%d\n\n",sse_id,sse_data[sse_id]);
	}
	else if(sse_id == 6){
		sprintf(ledJSON, "id:%d\ndata:%d\n\n",sse_id,sse_data[sse_id]);
	}
	else if(sse_id == 7){
		sprintf(ledJSON, "id:%d\ndata:%d\n\n",sse_id,sse_data[sse_id]);
	}
	else{
		sse_id = 0;
		sprintf(ledJSON, "id:%d\ndata:%d\n\n",sse_id,sse_data[sse_id]);
	}
	
	httpd_resp_send(req, ledJSON, strlen(ledJSON));
	sse_id++;

	return ESP_OK;
}

httpd_uri_t McuToHtml = {
	.uri = "/McuToHtml",
	.method = HTTP_GET,
	.handler = McuToHtml_handler,
	.user_ctx = NULL
};

httpd_handle_t start_OTA_webserver(void)
{
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	
	// Lets change this from port 80 (default) to 8080
	//config.server_port = 8080;
	
	
	// Lets bump up the stack size (default was 4096)
	config.stack_size = 8192;
	
	
	// Start the httpd server
	ESP_LOGI("OTA", "Starting server on port: '%d'", config.server_port);
	
	if (httpd_start(&OTA_server, &config) == ESP_OK) 
	{
		// Set URI handlers
		ESP_LOGI("OTA", "Registering URI handlers");
		httpd_register_uri_handler(OTA_server, &OTA_index_html);
		httpd_register_uri_handler(OTA_server, &OTA_favicon_ico);
		httpd_register_uri_handler(OTA_server, &OTA_jquery_3_4_1_min_js);
		httpd_register_uri_handler(OTA_server, &OTA_update);
		httpd_register_uri_handler(OTA_server, &HtmlToMcu);
		httpd_register_uri_handler(OTA_server, &McuToHtml);
		httpd_register_uri_handler(OTA_server, &OTA_status);
		return OTA_server;
	}

	ESP_LOGI("OTA", "Error starting server!");
	return NULL;
}

void stop_OTA_webserver(httpd_handle_t server)
{
	// Stop the httpd server
	httpd_stop(server);
}

