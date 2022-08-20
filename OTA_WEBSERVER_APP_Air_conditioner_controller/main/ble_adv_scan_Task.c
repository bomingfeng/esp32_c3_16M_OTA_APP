/*
 * BLE Combined Advertising and Scanning Example.
 *
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include "app_inclued.h"

#ifdef XL0801
static const char *TAG = "BLE_ADV_SCAN";

typedef struct {
    char scan_local_name[32];
    uint8_t name_len;
} ble_scan_local_name_t;

typedef struct {
    uint8_t *q_data;
    uint16_t q_data_len;
} host_rcv_data_t;

MessageBufferHandle_t ble_degC;  //换算2831 = 28.31
MessageBufferHandle_t ble_humidity;
MessageBufferHandle_t ble_Voltage;
TimerHandle_t Read_ble_xTimer;

static uint8_t hci_cmd_buf[128];

static QueueHandle_t adv_queue;

uint32_t degC_ble;
uint32_t humidity_ble;
uint32_t Voltage_ble;
uint8_t num = 0;

extern uint32_t sse_data[sse_len];

/*
 * @brief: BT controller callback function, used to notify the upper layer that
 *         controller is ready to receive command
 */
static void controller_rcv_pkt_ready(void)
{
    ESP_LOGI(TAG, "controller rcv pkt ready");
}

/*
 * @brief: BT controller callback function to transfer data packet to
 *         the host
 */
static int host_rcv_pkt(uint8_t *data, uint16_t len)
{
    host_rcv_data_t send_data;
    uint8_t *data_pkt;
    /* Check second byte for HCI event. If event opcode is 0x0e, the event is
     * HCI Command Complete event. Sice we have recieved "0x0e" event, we can
     * check for byte 4 for command opcode and byte 6 for it's return status. */
    if (data[1] == 0x0e) {
        if (data[6] == 0) {
            ESP_LOGI(TAG, "Event opcode 0x%02x success.", data[4]);
        } else {
            ESP_LOGE(TAG, "Event opcode 0x%02x fail with reason: 0x%02x.", data[4], data[6]);
            return ESP_FAIL;
        }
    }

    data_pkt = (uint8_t *)malloc(sizeof(uint8_t) * len);
    if (data_pkt == NULL) {
        ESP_LOGE(TAG, "Malloc data_pkt failed!");
        return ESP_FAIL;
    }
    memcpy(data_pkt, data, len);
    send_data.q_data = data_pkt;
    send_data.q_data_len = len;
    if (xQueueSend(adv_queue, (void *)&send_data, ( TickType_t ) 0) != pdTRUE) {
        ESP_LOGD(TAG, "Failed to enqueue advertising report. Queue full.");
        /* If data sent successfully, then free the pointer in `xQueueReceive'
         * after processing it. Or else if enqueue in not successful, free it
         * here. */
        free(data_pkt);
    }
    return ESP_OK;
}

static esp_vhci_host_callback_t vhci_host_cb = {
    controller_rcv_pkt_ready,
    host_rcv_pkt
};

static void hci_cmd_send_reset(void)
{
    uint16_t sz = make_cmd_reset (hci_cmd_buf);
    esp_vhci_host_send_packet(hci_cmd_buf, sz);
}

static void hci_cmd_send_set_evt_mask(void)
{
    /* Set bit 61 in event mask to enable LE Meta events. */
    uint8_t evt_mask[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20};
    uint16_t sz = make_cmd_set_evt_mask(hci_cmd_buf, evt_mask);
    esp_vhci_host_send_packet(hci_cmd_buf, sz);
}

static void hci_cmd_send_ble_scan_params(void)
{
    /* Set scan type to 0x01 for active scanning and 0x00 for passive scanning. */
    uint8_t scan_type = 0x01;

    /* Scan window and Scan interval are set in terms of number of slots. Each slot is of 625 microseconds. */
    uint16_t scan_interval = 0x50; /* 50 ms */
    uint16_t scan_window = 0x30; /* 30 ms */

    uint8_t own_addr_type = 0x00; /* Public Device Address (default). */
    uint8_t filter_policy = 0x00; /* Accept all packets excpet directed advertising packets (default). */
    uint16_t sz = make_cmd_ble_set_scan_params(hci_cmd_buf, scan_type, scan_interval, scan_window, own_addr_type, filter_policy);
    esp_vhci_host_send_packet(hci_cmd_buf, sz);
}

static void hci_cmd_send_ble_scan_start(void)
{
    uint8_t scan_enable = 0x01; /* Scanning enabled. */
    uint8_t filter_duplicates = 0x00; /* Duplicate filtering disabled. */
    uint16_t sz = make_cmd_ble_set_scan_enable(hci_cmd_buf, scan_enable, filter_duplicates);
    esp_vhci_host_send_packet(hci_cmd_buf, sz);
    ESP_LOGI(TAG, "BLE Scanning started..");
}



static esp_err_t get_local_name (uint8_t *data_msg, uint8_t data_len, ble_scan_local_name_t *scanned_packet)
{
    uint8_t curr_ptr = 0, curr_len, curr_type;
    while (curr_ptr < data_len) {
        curr_len = data_msg[curr_ptr++];
        curr_type = data_msg[curr_ptr++];
        if (curr_len == 0) {
            return ESP_FAIL;
        }

        /* Search for current data type and see if it contains name as data (0x08 or 0x09). */
        if (curr_type == 0x08 || curr_type == 0x09) {
            for (uint8_t i = 0; i < curr_len - 1; i += 1) {
                scanned_packet->scan_local_name[i] = data_msg[curr_ptr + i];
            }
            scanned_packet->name_len = curr_len - 1;
            return ESP_OK;
        } else {
            /* Search for next data. Current length includes 1 octate for AD Type (2nd octate). */
            curr_ptr += curr_len - 1;
        }
    }
    return ESP_FAIL;
}

void hci_evt_process(void *pvParameters)
{
    host_rcv_data_t *rcv_data = (host_rcv_data_t *)malloc(sizeof(host_rcv_data_t));
    if (rcv_data == NULL) {
        ESP_LOGE(TAG, "Malloc rcv_data failed!");
        return;
    }
    esp_err_t ret;

    while (1) {
        uint8_t sub_event, num_responses, total_data_len, data_msg_ptr, hci_event_opcode;
        uint8_t *queue_data = NULL, *event_type = NULL, *addr_type = NULL, *addr = NULL, *data_len = NULL, *data_msg = NULL;
        short int *rssi = NULL;
        uint16_t data_ptr;
        ble_scan_local_name_t *scanned_name = NULL;
        total_data_len = 0;
        data_msg_ptr = 0;
        if (xQueueReceive(adv_queue, rcv_data, portMAX_DELAY) != pdPASS) {
            ESP_LOGE(TAG, "Queue receive error");
        } else {

/*
                queue_data = rcv_data->q_data;
                for(data_ptr = 0;data_ptr < (rcv_data->q_data_len);data_ptr++)
                {
                printf("data:%x\r\n",queue_data[data_ptr]);
                }
*/

            /* `data_ptr' keeps track of current position in the received data. */
            data_ptr = 0;
            queue_data = rcv_data->q_data;

            /* Parsing `data' and copying in various fields. */
            hci_event_opcode = queue_data[++data_ptr];
            //printf("hci_event_opcode:0x%x\r\n",hci_event_opcode);
            if (hci_event_opcode == LE_META_EVENTS) {
                /* Set `data_ptr' to 4th entry, which will point to sub event. */
                data_ptr += 2;
                sub_event = queue_data[data_ptr++];
                //printf("sub_event:0x%x\r\n",sub_event);
                /* Check if sub event is LE advertising report event. */
                if (sub_event == HCI_LE_ADV_REPORT) {
                    /* Get number of advertising reports. */
                    num_responses = queue_data[data_ptr++];
                    //printf("num_responses:0x%x\r\n",num_responses);
                    event_type = (uint8_t *)malloc(sizeof(uint8_t) * num_responses);
                    if (event_type == NULL) {
                        ESP_LOGE(TAG, "Malloc event_type failed!");
                        goto reset;
                    }
                    for (uint8_t i = 0; i < num_responses; i += 1) {
                        event_type[i] = queue_data[data_ptr++];
                        //printf("event_type[%d]:0x%x\r\n",i,event_type[i]);
                    }

                    /* Get advertising type for every report. */
                    addr_type = (uint8_t *)malloc(sizeof(uint8_t) * num_responses);
                    if (addr_type == NULL) {
                        ESP_LOGE(TAG, "Malloc addr_type failed!");
                        goto reset;
                    }
                    for (uint8_t i = 0; i < num_responses; i += 1) {
                        addr_type[i] = queue_data[data_ptr++];
                        //printf("addr_type[%d]:0x%x\r\n",i,addr_type[i]);
                    }

                    /* Get BD address in every advetising report and store in
                     * single array of length `6 * num_responses' as each address
                     * will take 6 spaces. */
                    addr = (uint8_t *)malloc(sizeof(uint8_t) * 6 * num_responses);
                    if (addr == NULL) {
                        ESP_LOGE(TAG, "Malloc addr failed!");
                        goto reset;
                    }
                    for (int i = 0; i < num_responses; i += 1) {
                        for (int j = 0; j < 6; j += 1) {
                            addr[(6 * i) + j] = queue_data[data_ptr++];
                            //printf("addr[%d]:0x%x\r\n",(6 * i) + j,addr[(6 * i) + j]);
                        }
                    }

                    /* Get length of data for each advertising report. */
                    data_len = (uint8_t *)malloc(sizeof(uint8_t) * num_responses);
                    if (data_len == NULL) {
                        ESP_LOGE(TAG, "Malloc data_len failed!");
                        goto reset;
                    }
                    for (uint8_t i = 0; i < num_responses; i += 1) {
                        data_len[i] = queue_data[data_ptr];
                        total_data_len += queue_data[data_ptr++];
                        //printf("data_len[%d]:0x%x;total_data_len:%d\r\n",i,data_len[i],total_data_len);
                    }

                    if (total_data_len != 0) {
                        /* Get all data packets. */
                        data_msg = (uint8_t *)malloc(sizeof(uint8_t) * total_data_len);
                        if (data_msg == NULL) {
                            ESP_LOGE(TAG, "Malloc data_msg failed!");
                            goto reset;
                        }
                        for (uint8_t i = 0; i < num_responses; i += 1) {
                            for (uint8_t j = 0; j < data_len[i]; j += 1) {
                                data_msg[data_msg_ptr++] = queue_data[data_ptr++];
                                //printf("data_msg[%d]:0x%x;\r\n",data_msg_ptr - 1,data_msg[data_msg_ptr - 1]);
                            }
                        }
                    }
                    /* Counts of advertisements done. This count is set in advertising data every time before advertising. */
                    rssi = (short int *)malloc(sizeof(short int) * num_responses);
                    if (data_len == NULL) {
                        ESP_LOGE(TAG, "Malloc rssi failed!");
                        goto reset;
                    }
                    for (uint8_t i = 0; i < num_responses; i += 1) {
                        rssi[i] = -(0xFF - queue_data[data_ptr++]);
                        //printf("rssi[%d]:%d;\r\n",i,rssi[i]);
                    }

                    /* Extracting advertiser's name. */
                    data_msg_ptr = 0;
                    scanned_name = (ble_scan_local_name_t *)malloc(num_responses * sizeof(ble_scan_local_name_t));
                    if (data_len == NULL) {
                        ESP_LOGE(TAG, "Malloc scanned_name failed!");
                        goto reset;
                    }
                    for (uint8_t i = 0; i < num_responses; i += 1) {
                        ret = get_local_name(&data_msg[data_msg_ptr], data_len[i], scanned_name);

                        /* Print the data if adv report has a valid name. */
                        if (ret == ESP_OK) {
                            //printf("******** Response %d/%d ********\n", i + 1, num_responses);
                            //printf("Event type: %02x\nAddress type: %02x\nAddress: ", event_type[i], addr_type[i]);
                            for (int j = 5; j >= 0; j -= 1) {
                                //printf("%02x", addr[(6 * i) + j]);
                                if (j > 0) {
                                    //printf(":");
                                }
                            }

                            //printf("\nData length: %d", data_len[i]);
                            data_msg_ptr += data_len[i];
                            //printf("\nAdvertisement Name: ");
                            for (int k = 0; k < scanned_name->name_len; k += 1 ) {
                                //printf("%c", scanned_name->scan_local_name[k]);
                            }
                            //printf("\nRSSI: %ddB\n", rssi[i]);

                            if((total_data_len != 0) && (scanned_name->scan_local_name[0] == 'X') && (scanned_name->scan_local_name[1] == 'L') && (scanned_name->scan_local_name[2] == '0') && \
                               (scanned_name->scan_local_name[3] == '8') && (scanned_name->scan_local_name[4] == '0') && (scanned_name->scan_local_name[5] == '1'))
                            {
                                num++;
                                Voltage_ble += ((data_msg[13] << 8) + data_msg[14]) * 10;
                                degC_ble += ((data_msg[15] << 8) + data_msg[16]) * 10;
                                humidity_ble += data_msg[17] * 100;
                                if(num >= 18)
                                {
                                    degC_ble /= 18;humidity_ble /= 18;Voltage_ble /= 18;
                                    xMessageBufferSend( ble_degC,   \
                                                &degC_ble,  \
                                                4,portMAX_DELAY);
                                    xMessageBufferSend( ble_humidity,   \
                                                            &humidity_ble,  \
                                                            4,portMAX_DELAY);
                                    xMessageBufferSend( ble_Voltage,   \
                                                            &Voltage_ble,  \
                                                            4,portMAX_DELAY);   
                                    xTimerStop(Read_ble_xTimer,portMAX_DELAY);                        
                                    xEventGroupSetBits(APP_event_group,APP_event_BLE_CONNECTED_flags_BIT);

                                    sse_data[1] = (degC_ble << 16) | humidity_ble;
                                    sse_data[5] = Voltage_ble | 0x80000000;

                                    printf("degC_ble:%d( /100);humidity_ble:%d( /100);Voltage_ble:%dmv. \r\n",degC_ble,humidity_ble,Voltage_ble);
                                    Voltage_ble = 0;degC_ble = 0;humidity_ble = 0;num = 0;
                                }
                            }
                            else
                            {
                                if(xTimerIsTimerActive(Read_ble_xTimer) == pdFALSE)
                                {
                                    xTimerReset(Read_ble_xTimer,portMAX_DELAY); 
                                }
                            }
                        }
                    }
                    
                    /* Freeing all spaces allocated. */
reset:
                    free(scanned_name);
                    free(rssi);
                    free(data_msg);
                    free(data_len);
                    free(addr);
                    free(addr_type);
                    free(event_type);
                }
            }
#if (CONFIG_LOG_DEFAULT_LEVEL_DEBUG || CONFIG_LOG_DEFAULT_LEVEL_VERBOSE)
            printf("Raw Data:");
            for (uint8_t j = 0; j < rcv_data->q_data_len; j += 1) {
                printf(" %02x", queue_data[j]);
            }
            printf("\nQueue free size: %d\n", uxQueueSpacesAvailable(adv_queue));
#endif
            free(queue_data);
        }
        memset(rcv_data, 0, sizeof(host_rcv_data_t));
    }
}

void Read_ble_xTimerCallback(TimerHandle_t xTimer)
{
    xTimerStop(Read_ble_xTimer,portMAX_DELAY);
    xEventGroupClearBits(APP_event_group,APP_event_BLE_CONNECTED_flags_BIT);
    sse_data[1] = 0;
    sse_data[5] = 0;
}

void ble_adv_scan_Task(void * arg)
{
    bool continue_commands = 1;
    int cmd_cnt = 0;
    esp_err_t ret;

    Read_ble_xTimer = xTimerCreate("Timer0",(60000 / portTICK_PERIOD_MS)/*min*/ * 5,pdFALSE,( void * ) 0,Read_ble_xTimerCallback);//1min

    xEventGroupClearBits(APP_event_group,APP_event_BLE_CONNECTED_flags_BIT);
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGI(TAG, "Bluetooth controller initialize failed: %s", esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_BLE)) != ESP_OK) {
        ESP_LOGI(TAG, "Bluetooth controller enable failed: %s", esp_err_to_name(ret));
        return;
    }

    /* A queue for storing received HCI packets. */
    adv_queue = xQueueCreate(15, sizeof(host_rcv_data_t));
    if (adv_queue == NULL) {
        ESP_LOGE(TAG, "Queue creation failed\n");
        return;
    }

    esp_vhci_host_register_callback(&vhci_host_cb);
    while (continue_commands) {
        if (continue_commands && esp_vhci_host_check_send_available()) {
            switch (cmd_cnt) {
            case 0:  hci_cmd_send_reset();++cmd_cnt; break;//
            case 1:  hci_cmd_send_set_evt_mask();++cmd_cnt; break;//
            /* Send scan commands. */
            case 2: hci_cmd_send_ble_scan_params(); ++cmd_cnt; break;
            case 3: hci_cmd_send_ble_scan_start(); ++cmd_cnt; break;
            default: continue_commands = 0; break;
            }
            ESP_LOGI(TAG, "BLE Advertise, cmd_sent: %d", cmd_cnt);
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    xTaskCreatePinnedToCore(&hci_evt_process, "hci_evt_process", 2048, NULL, 6, NULL, 0);
    vTaskDelete(NULL);
}
#endif