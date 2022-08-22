#include "app_init.h"
#include <sys/param.h>
#include "esp_ota_ops.h"



void app_init(void)
{
    uint8_t i,ii;
    const esp_partition_t *partition = NULL;
#if CONFIG_IDF_TARGET_ESP32
    gpio_reset_pin(22);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(22, GPIO_MODE_OUTPUT);
    gpio_set_level(22,0);

    gpio_reset_pin(23);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(23, GPIO_MODE_OUTPUT);
    gpio_set_level(23,1);

    gpio_reset_pin(0);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(0, GPIO_MODE_INPUT);
    gpio_set_level(0,0);
    for(i = 0;i < 40;i++)
    {
        if(gpio_get_level(CONFIG_INPUT_GPIO) == 0x00)
        {
            vTaskDelay(50 / portTICK_PERIOD_MS);
            if(gpio_get_level(CONFIG_INPUT_GPIO) == 0x00)
            {
                vTaskDelay(1950 / portTICK_PERIOD_MS);
                if(gpio_get_level(CONFIG_INPUT_GPIO) == 0x00)
                {
                    gpio_set_level(22,1);
                    ii = 0xaa;
                    ESP_LOGI("app_init","esp_ota_get_next_update_partition\r\n");
                    partition = esp_ota_get_next_update_partition(NULL);
                }
                vTaskDelay(1950 / portTICK_PERIOD_MS);
                if(gpio_get_level(CONFIG_INPUT_GPIO) == 0x00)
                {
                    gpio_set_level(23,0);
                    ii = 0xaa;
                    ESP_LOGI("app_init","ESP_PARTITION_SUBTYPE_APP_FACTORY\r\n");
                    partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP,    \
				    ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
                }
                if(ii == 0xaa)
                {
                    ESP_LOGI("app_init","restart to boot,%d\r\n",gpio_get_level(CONFIG_INPUT_GPIO));
                    esp_ota_set_boot_partition(partition);
                    vTaskDelay(2000 / portTICK_PERIOD_MS);
                    esp_restart();
                }
            }
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
       
    }
    gpio_set_level(23,0);

    /* IO 初始化 */
    WRITE_PERI_REG(GPIO_FUNC2_OUT_SEL_CFG_REG, READ_PERI_REG(GPIO_FUNC2_OUT_SEL_CFG_REG) | GPIO_FUNC2_OUT_SEL_M);
    gpio_reset_pin(2);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(2, GPIO_MODE_OUTPUT);
    gpio_set_level(2,0);
    gpio_set_pull_mode(2,GPIO_PULLUP_ONLY);

    WRITE_PERI_REG(GPIO_FUNC15_OUT_SEL_CFG_REG, READ_PERI_REG(GPIO_FUNC15_OUT_SEL_CFG_REG) | GPIO_FUNC15_OUT_SEL_M);
    gpio_reset_pin(15);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(15, GPIO_MODE_OUTPUT);
    gpio_set_level(15,0);
    gpio_set_pull_mode(15,GPIO_PULLUP_ONLY);
    
    WRITE_PERI_REG(GPIO_FUNC13_OUT_SEL_CFG_REG, READ_PERI_REG(GPIO_FUNC13_OUT_SEL_CFG_REG) | GPIO_FUNC13_OUT_SEL_M);
    gpio_reset_pin(13);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(13, GPIO_MODE_OUTPUT);
    gpio_set_level(13,0);

    WRITE_PERI_REG(GPIO_FUNC12_OUT_SEL_CFG_REG, READ_PERI_REG(GPIO_FUNC12_OUT_SEL_CFG_REG) | GPIO_FUNC12_OUT_SEL_M);
    gpio_reset_pin(12);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(12, GPIO_MODE_OUTPUT);
    gpio_set_level(12,0);
    gpio_set_pull_mode(12,GPIO_PULLUP_ONLY);

/*数码管IO初始化

*/
    //位
    gpio_reset_pin(21);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(21, GPIO_MODE_OUTPUT);
    gpio_set_level(21,0);
    gpio_set_pull_mode(21,GPIO_PULLUP_ONLY);

    gpio_reset_pin(19);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(19, GPIO_MODE_OUTPUT);
    gpio_set_level(19,0);
    gpio_set_pull_mode(19,GPIO_PULLUP_ONLY);

    gpio_reset_pin(18);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(18, GPIO_MODE_OUTPUT);
    gpio_set_level(18,0);
    gpio_set_pull_mode(18,GPIO_PULLUP_ONLY);

    gpio_reset_pin(5);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(5, GPIO_MODE_OUTPUT);
    gpio_set_level(5,0);
    gpio_set_pull_mode(5,GPIO_PULLUP_ONLY);   

//段
    //A
    gpio_reset_pin(17);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(17, GPIO_MODE_OUTPUT);
    gpio_set_level(17,0);
    gpio_set_pull_mode(17,GPIO_PULLUP_ONLY); 

    //c
    gpio_reset_pin(16);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(16, GPIO_MODE_OUTPUT);
    gpio_set_level(16,0);
    gpio_set_pull_mode(16,GPIO_PULLUP_ONLY);

    //b
    gpio_reset_pin(4);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(4, GPIO_MODE_OUTPUT);
    gpio_set_level(4,0);
    gpio_set_pull_mode(4,GPIO_PULLUP_ONLY);

    gpio_reset_pin(12);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(12, GPIO_MODE_OUTPUT);
    gpio_set_level(12,0);
    gpio_set_pull_mode(12,GPIO_PULLUP_ONLY);

    gpio_reset_pin(14);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(14, GPIO_MODE_OUTPUT);
    gpio_set_level(14,0);
    gpio_set_pull_mode(14,GPIO_PULLUP_ONLY);

    //G
    gpio_reset_pin(32);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(32, GPIO_MODE_OUTPUT);
    gpio_set_level(32,0);
    gpio_set_pull_mode(32,GPIO_PULLUP_ONLY);

    //E
    gpio_reset_pin(33);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(33, GPIO_MODE_OUTPUT);
    gpio_set_level(33,0);
    gpio_set_pull_mode(33,GPIO_PULLUP_ONLY);

    //DP
    gpio_reset_pin(26);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(26, GPIO_MODE_OUTPUT);
    gpio_set_level(26,0);
    gpio_set_pull_mode(26,GPIO_PULLUP_ONLY);
#elif CONFIG_IDF_TARGET_ESP32C3
    
#endif // CONFIG_IDF_TARGET_* 
    //Initialize NVS
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);

    
}