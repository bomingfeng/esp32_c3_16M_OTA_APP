#include "ir_rx_task.h"

rmt_channel_t example_rx_channel = RMT_CHANNEL_2;
MessageBufferHandle_t ir_rx_data;

extern char * tcprx_buffer;
extern MessageBufferHandle_t tcp_send_data;

void ir_rx_task_init(void)
{

}

/**
 * @brief RMT Receive Task
 *
 */
void example_ir_rx_task(void *arg)
{
    rmt_item32_t *items = NULL;
    size_t length = 0;
    RingbufHandle_t rb = NULL;
    uint8_t rx_data[13];
    uint32_t data1,data2,data3,data4;
    rmt_config_t rmt_rx_config = RMT_DEFAULT_CONFIG_RX(CONFIG_EXAMPLE_RMT_RX_GPIO, example_rx_channel);
    rmt_config(&rmt_rx_config);
    rmt_driver_install(example_rx_channel, 1000, 0);
    ir_parser_config_t ir_parser_config = IR_PARSER_DEFAULT_CONFIG((ir_dev_t)example_rx_channel);//??？？
    ir_parser_config.flags |= IR_TOOLS_FLAGS_PROTO_EXT; // Using extended IR protocols (both NEC and RC5 have extended version)
    ir_parser_t *ir_parser = NULL;
    ir_parser = ir_parser_rmt_new(&ir_parser_config);
    //get RMT RX ringbuffer
    rmt_get_ringbuf_handle(example_rx_channel, &rb);
    assert(rb != NULL);
    // Start receive
    rmt_rx_start(example_rx_channel, true);
    while(1)
    {
        items = (rmt_item32_t *) xRingbufferReceive(rb, &length, portMAX_DELAY);
        if (items) {
            length /= 4; // one RMT = 4 Bytes
            if (ir_parser->input(ir_parser, items, length) == ESP_OK) 
            {
                if (ir_parser->get_scan_code(ir_parser, &data1,&data2,&data3,&data4) == ESP_OK) 
                {
#ifdef  Gree                   
                    if((data1 & 0xf0000000) == 0x50000000)
                    {
                        gpio_set_level(18, 1);
                        rx_data[3] = data1 & 0x000000ff;rx_data[2] = (data1 & 0x0000ff00) >> 8;
                        rx_data[1] = (data1 & 0x00ff0000) >> 16;rx_data[0] = (data1 & 0xff000000) >> 24;

                        rx_data[7] = data2 & 0x000000ff;rx_data[6] = (data2 & 0x0000ff00) >> 8;
                        rx_data[5] = (data2 & 0x00ff0000) >> 16;rx_data[4] = (data2 & 0xff000000) >> 24;
                        xMessageBufferSend(ir_rx_data,rx_data,13,portMAX_DELAY);
                        printf("ir ok\r\n");
                    }
#endif

#ifdef  Auxgroup                   
                    if((data1 & 0xFF000000) == 0xC3000000)
                    {
                        gpio_set_level(18, 1);
                        rx_data[3] = data1 & 0x000000ff;rx_data[2] = (data1 & 0x0000ff00) >> 8;
                        rx_data[1] = (data1 & 0x00ff0000) >> 16;rx_data[0] = (data1 & 0xff000000) >> 24;

                        rx_data[7] = data2 & 0x000000ff;rx_data[6] = (data2 & 0x0000ff00) >> 8;
                        rx_data[5] = (data2 & 0x00ff0000) >> 16;rx_data[4] = (data2 & 0xff000000) >> 24;

                        rx_data[11] = data3 & 0x000000ff;rx_data[10] = (data3 & 0x0000ff00) >> 8;
                        rx_data[9] = (data3 & 0x00ff0000) >> 16;rx_data[8] = (data3 & 0xff000000) >> 24;

                        rx_data[12] = (data4 & 0xff000000) >> 24;
                        xMessageBufferSend(ir_rx_data,rx_data,13,portMAX_DELAY);
                        printf("ir ok\r\n");
                    }
#endif
                }
            }
            //after parsing the data, return spaces to ringbuffer.
            vRingbufferReturnItem(rb, (void *) items);
        }
    }
    ir_parser->del(ir_parser);
    rmt_driver_uninstall(example_rx_channel);
    vTaskDelete(NULL);
}

