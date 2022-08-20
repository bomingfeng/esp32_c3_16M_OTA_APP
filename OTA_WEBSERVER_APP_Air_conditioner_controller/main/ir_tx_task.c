#include "ir_tx_Task.h"

rmt_channel_t example_tx_channel = RMT_CHANNEL_0;
MessageBufferHandle_t ir_tx_data;

extern char * tcprx_buffer;
extern MessageBufferHandle_t tcp_send_data;
extern rmt_channel_t example_rx_channel;

void ir_tx_task_init(void)
{

}

/**
 * @brief RMT Transmit Task
 *
 */
void example_ir_tx_task(void *arg)
{   
    rmt_item32_t *items = NULL;
    size_t length = 0;
    ir_builder_t *ir_builder = NULL;
    uint8_t tx_buffer[13];
    
    rmt_config_t rmt_tx_config = RMT_CONFIG_TX(CONFIG_EXAMPLE_RMT_TX_GPIO, example_tx_channel);
    rmt_tx_config.tx_config.carrier_en = true;
    rmt_config(&rmt_tx_config);
    rmt_driver_install(example_tx_channel, 0, 0);
    ir_builder_config_t ir_builder_config = IR_BUILDER_CONFIG((ir_dev_t)example_tx_channel);
    ir_builder_config.flags |= IR_TOOLS_FLAGS_PROTO_EXT; // Using extended IR protocols (both NEC and RC5 have extended version)
    ir_builder = ir_builder_rmt_new(&ir_builder_config);
    while(1)
    {   
        xMessageBufferReceive(ir_tx_data,(void *)tx_buffer,sizeof(tx_buffer),portMAX_DELAY);
        rmt_rx_stop(example_rx_channel);
        // Send new key code
        ESP_ERROR_CHECK(ir_builder->build_frame(ir_builder,tx_buffer));
        ESP_ERROR_CHECK(ir_builder->get_result(ir_builder, &items, &length));
        //To send data according to the waveform items.
        rmt_write_items(example_tx_channel, items, length, false);  
        vTaskDelay(800 / portTICK_PERIOD_MS);
        for(length = 0;length < 13;length++)
        {
            //printf("tx_buffer[%d] = 0x%x;\r\n",length,tx_buffer[length]);
        }
        rmt_rx_start(example_rx_channel, true);
    }
    ir_builder->del(ir_builder);
    rmt_driver_uninstall(example_tx_channel);
    vTaskDelete(NULL);
}

