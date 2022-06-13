#include "app_init.h"

void app_init(void)
{
    /* IO 初始化 */
	gpio_reset_pin(3);
    gpio_reset_pin(4);
    gpio_reset_pin(5);
    gpio_reset_pin(18);
    gpio_reset_pin(19);
    gpio_set_level(18, 0);//白
    gpio_set_level(19, 0);//橙
    gpio_set_level(3, 0); //三色灯红  
    gpio_set_level(4, 0); //三色灯绿
    gpio_set_level(5, 0); //三色灯橙
    gpio_set_direction(3, GPIO_MODE_OUTPUT);
    gpio_set_direction(4, GPIO_MODE_OUTPUT);
    gpio_set_direction(5, GPIO_MODE_OUTPUT);
    gpio_set_direction(18, GPIO_MODE_OUTPUT);
    gpio_set_direction(19, GPIO_MODE_OUTPUT);

    //Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
}