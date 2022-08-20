#include "BLE_Client.h"

nvs_handle_t BLe_battery_handle;
int32_t BLe_battery;

#ifdef  LYWSD03MMC

TimerHandle_t Read_ble_xTimer;

/*在此示例中，有一个应用程序配置文件，其ID定义为：*/
#define PROFILE_NUM      1
#define PROFILE_A_APP_ID 0

#define INVALID_HANDLE   0

static const char remote_device_name[] = "LYWSD03MMC";

MessageBufferHandle_t ble_degC;  //换算2831 = 28.31
MessageBufferHandle_t ble_humidity;
MessageBufferHandle_t ble_Voltage;

uint32_t degC_ble = 0;
uint32_t humidity_ble = 0;
uint32_t Voltage_ble = 0;
uint8_t con = 0;



extern uint32_t sse_data[sse_len];
extern char * tcprx_buffer;
extern MessageBufferHandle_t tcp_send_data;
// LSB <--------------------------------------------------------------------------------> MSB 
//uint8_t static remote_uuid[16] = {0xEB,0xE0,0xCC,0xB0,0x7A,0x0A,0x4B,0x0C,0x8A,0x1A,0x6F,0xF2,0x99,0x7D,0xA3,0xA6};
//uint8_t static remote_uuid[16] = {0xA6,0xA3,0x7D,0x99,0xF2,0x6F,0x1A,0x8A,0x0C,0x4B,0x0A,0x7A,0xB0,0xCC,0xE0,0xEB};

static bool connect    = false;
static bool get_server = false;
static esp_gattc_char_elem_t *char_elem_result   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result = NULL;

/* Declare static functions 
    esp_gap_cb（）和esp_gattc_cb（）函数处理 BLE 堆栈生成的所有事件。*/
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

/* 应用程序配置文件表数组的初始化包括为每个配置文件定义回调函数。它们分别是gattc_profile_a_event_handler() and gattc_profile_a_event_handler()，
另外，GATT接口初始化为ESP_GATT_IF_NONE默认值。稍后，在注册应用程序概要文件时，BLE堆栈返回一个GATT接口实例来与该就用程序概要文件一起使用。*/
static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);


/*  注意：16bit和32bit的UUID只需要设置UUID的前16bit或者32bit，剩余的bit使用的是Bluetooth_Base_UUID：xxxxxxxx-0000-1000-8000-00805F9B34FB
#define REMOTE_SERVICE_UUID        0x00FF
#define REMOTE_NOTIFY_CHAR_UUID    0xFF01
......
//Server UUID
static esp_bt_uuid_t remote_filter_service_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = REMOTE_SERVICE_UUID,},
};
//notify UUID
static esp_bt_uuid_t remote_filter_char_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = REMOTE_NOTIFY_CHAR_UUID,},
};
//Characteristic UUID
static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};

//Server UUID
static esp_bt_uuid_t remote_filter_service_uuid = {
    .len = ESP_UUID_LEN_128,
    // LSB <--------------------------------------------------------------------------------> MSB 
    .uuid = {.uuid128 = {0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0xe0,0x93,0xf3,0xa3,0xb5,0x01,0x00,0x40,0x6e},},
};
//notify UUID
static esp_bt_uuid_t remote_filter_char_uuid = {
    .len = ESP_UUID_LEN_128,
	// LSB <--------------------------------------------------------------------------------> MSB 
    .uuid = {.uuid128 = {0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0xe0,0x93,0xf3,0xa3,0xb5,0x03,0x00,0x40,0x6e},},
};
//Characteristic UUID
static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};
*/

//Temperature and Humidity
//Server uuid:EBE0CCB0-7A0A-4B0C-8A1A-6FF2997DA3A6 
//Characteristic uuid:EBE0CCC1-7A0A-4B0C-8A1A-6FF2997DA3A6 READ NOTIFY

//Server UUID
static esp_bt_uuid_t LYWSD03MMC_Temperature_Humidity = {
    .len = ESP_UUID_LEN_128,

    // LSB <--------------------------------------------------------------------------------> MSB 
    .uuid = {.uuid128 = {0xA6,0xA3,0x7D,0x99,0xF2,0x6F,0x1A,0x8A,0x0C,0x4B,0x0A,0x7A,0xB0,0xCC,0xE0,0xEB},},
};

//Characteristic UUID
static esp_bt_uuid_t Temperature_Humidity = {
    .len = ESP_UUID_LEN_128,

    // LSB <--------------------------------------------------------------------------------> MSB 
    .uuid = {.uuid128 = {0xA6,0xA3,0x7D,0x99,0xF2,0x6F,0x1A,0x8A,0x0C,0x4B,0x0A,0x7A,0xC1,0xCC,0xE0,0xEB},},
};

//notify UUID
static esp_bt_uuid_t notify_Temperature_Humidity = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};


static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;    //GATT客户机回调函数
    uint16_t gattc_if;          //此配置文件的GATT客户端接口号
    uint16_t app_id;            //应用程序配置文件ID号
    uint16_t conn_id;           //连接id
    uint16_t service_start_handle;  //服务启动句柄
    uint16_t service_end_handle;    //服务端句柄
    uint16_t char_handle;           //字符句柄
    esp_bd_addr_t remote_bda;       //连接到此客户端的远程设备地址。
};

/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT 
应用程序配置文件表数组的初始化包括为每个配置文件定义回调函数。它们分别是gattc_profile_a_event_handler() and gattc_profile_a_event_handler()，
另外，GATT接口初始化为ESP_GATT_IF_NONE默认值。稍后，在注册应用程序概要文件时，BLE堆栈返回一个GATT接口实例来与该就用程序概要文件一起使用。*/
static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gattc_cb = gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};


uint8_t ble_batty_low = 0;
static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
    case ESP_GATTC_REG_EVT:
        ESP_LOGI(GATTC_TAG, "REG_EVT");
        esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        if (scan_ret){
            ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
        }
        break;
    case ESP_GATTC_CONNECT_EVT:{
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CONNECT_EVT conn_id %d, if %d", p_data->connect.conn_id, gattc_if);
        gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->connect.conn_id;
        memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GATTC_TAG, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->connect.conn_id);
        if (mtu_ret){
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
        }
        break;
    }
    case ESP_GATTC_OPEN_EVT:
        if (param->open.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "open failed, status %d", p_data->open.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "open success");
        break;

    case ESP_GATTC_DIS_SRVC_CMPL_EVT:
        if (param->dis_srvc_cmpl.status != ESP_GATT_OK) {
            ESP_LOGE(GATTC_TAG, "discover service failed, status %d\n", param->dis_srvc_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "discover service complete conn_id %d\n", param->dis_srvc_cmpl.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, NULL);
        break;
    case ESP_GATTC_SEARCH_RES_EVT: 
        ESP_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d\n", p_data->search_res.conn_id, p_data->search_res.is_primary);
        ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d\n", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
        //printf("--------------------------\r\n");
        for(int i=0;i<16;i++){
            //printf("%02x",p_data->search_res.srvc_id.uuid.uuid.uuid128[i]);
        }
         //printf("  uuid_len = %d\r\n",p_data->search_res.srvc_id.uuid.len);
        //printf("--------------------------\r\n");
        
        bool cmp = false;
        for(uint8_t a = 0;a < 16;a++)
        {
            if(p_data->search_res.srvc_id.uuid.uuid.uuid128[a] == LYWSD03MMC_Temperature_Humidity.uuid.uuid128[a])
                cmp = true;
            else
            {
                cmp = false;
                break;
            }
            //printf("uuid128:0x%x, remote_uuid:0x%x\n",p_data->search_res.srvc_id.uuid.uuid.uuid128[a],LYWSD03MMC_Temperature_Humidity.uuid.uuid128[a]);
        }
        if ((p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_128) && (cmp == true)) {
            ESP_LOGI(GATTC_TAG, "service found\n");
            get_server = true;
            gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
            gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
            //ESP_LOGI(GATTC_TAG, "UUID16: %s\n", p_data->search_res.srvc_id.uuid.uuid.uuid128);
        }
        break;   

    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        break;
   
    case ESP_GATTC_SEARCH_CMPL_EVT:
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
            break;
        }
        if(p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE) {
            ESP_LOGI(GATTC_TAG, "Get service information from remote device");
        } else if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH) {
            ESP_LOGI(GATTC_TAG, "Get service information from flash");
        } else {
            ESP_LOGI(GATTC_TAG, "unknown service source");
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SEARCH_CMPL_EVT:%d",get_server);
        if (get_server){
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     p_data->search_cmpl.conn_id,
                                                                     ESP_GATT_DB_CHARACTERISTIC,
                                                                     gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                                     gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                                     INVALID_HANDLE,
                                                                     &count);
            if (status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }

            if (count > 0){
                char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                if (!char_elem_result){
                    ESP_LOGE(GATTC_TAG, "gattc no mem");
                }else{
                    status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                             p_data->search_cmpl.conn_id,
                                                             gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                             gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,


                                                            Temperature_Humidity,


                                                             char_elem_result,
                                                             &count);
                    if (status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
                    }

                    /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                    if (count > 0 && (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
                        gl_profile_tab[PROFILE_A_APP_ID].char_handle = char_elem_result[0].char_handle;
                        esp_ble_gattc_register_for_notify (gattc_if, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, char_elem_result[0].char_handle);
                    }
                }
                /* free char_elem_result */
                free(char_elem_result);
            }else{
                ESP_LOGE(GATTC_TAG, "no char found");
            }
        }
         break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "REG FOR NOTIFY failed: error status = %d", p_data->reg_for_notify.status);
        }else{
            uint16_t count = 0;
            uint16_t notify_en = 1;
            esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                         ESP_GATT_DB_DESCRIPTOR,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                                                         &count);
            if (ret_status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }
            if (count > 0){
                descr_elem_result = malloc(sizeof(esp_gattc_descr_elem_t) * count);
                if (!descr_elem_result){
                    ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
                }else{
                    ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                         p_data->reg_for_notify.handle,


                                                            notify_Temperature_Humidity,



                                                                         descr_elem_result,
                                                                         &count);
                    if (ret_status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                    }
                    /* Every char has only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
                    if (count > 0 && descr_elem_result[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
                        ret_status = esp_ble_gattc_write_char_descr( gattc_if,
                                                                     gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                     descr_elem_result[0].handle,
                                                                     sizeof(notify_en),
                                                                     (uint8_t *)&notify_en,
                                                                     ESP_GATT_WRITE_TYPE_RSP,
                                                                     ESP_GATT_AUTH_REQ_NONE);
                    }

                    if (ret_status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
                    }

                    /* free descr_elem_result */
                    free(descr_elem_result);
                }
            }
            else{
                ESP_LOGE(GATTC_TAG, "decsr not found");
            }

        }
        break;
    }
    case ESP_GATTC_NOTIFY_EVT:
        if (p_data->notify.is_notify)
        {
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive notify value:");
          
            if((p_data->notify.value_len == 5) && (p_data->notify.value[1] != 0))
            {
                con++;
                degC_ble += (((p_data->notify.value[1]) << 8) + (p_data->notify.value[0])); 
                humidity_ble += p_data->notify.value[2];
                Voltage_ble += (p_data->notify.value[4] << 8) + p_data->notify.value[3];
                if(con >= 18)
                {
                    degC_ble = degC_ble/18;
                    humidity_ble = humidity_ble/18;
                    Voltage_ble = Voltage_ble/18;
                    //printf("\r\n----------------------------------------------------\r\n");
                    printf("temperature:%ddecC,humidity:%d%%,Voltage:%dmV,value_len:%d\r\n",    \
                      degC_ble,humidity_ble,Voltage_ble,p_data->notify.value_len);
                    //printf("----------------------------------------------------\r\n");


                   // BaseType_t xHigherPriorityTaskWoken = pdFALSE; /* Initialised to pdFALSE. */
                    /* Attempt to send the string to the message buffer. */
                   /* xMessageBufferSendFromISR( ble_degC,   \
                                            &degC_ble,  \
                                            4,          \
                                            &xHigherPriorityTaskWoken );*/
                    xMessageBufferSend( ble_degC,   \
                                            &degC_ble,  \
                                            4,portMAX_DELAY);
                    xMessageBufferSend( ble_humidity,   \
                                            &humidity_ble,  \
                                            4,portMAX_DELAY);
                    xMessageBufferSend( ble_Voltage,   \
                                            &Voltage_ble,  \
                                            4,portMAX_DELAY);    
                    xEventGroupSetBits(APP_event_group,APP_event_BLE_CONNECTED_flags_BIT);   
                    xTimerStop(Read_ble_xTimer,portMAX_DELAY); 

                    sse_data[1] = (degC_ble << 16) | humidity_ble; 
                    sse_data[5] = Voltage_ble | 0x80000000;
                    degC_ble = 0;
                    humidity_ble = 0;
                    Voltage_ble = 0;
                    con = 0;
                }
            }
            else
            {
                con = 0;
                humidity_ble = 0;
                Voltage_ble = 0;
                degC_ble = 0;
            }
           
        }
        else
        {
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive indicate value:");
        }
        esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
        break;
    case ESP_GATTC_WRITE_DESCR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write descr success ");
        break;
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:");
        esp_log_buffer_hex(GATTC_TAG, bda, sizeof(esp_bd_addr_t));
        break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write char failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write char success ");
        break;
    case ESP_GATTC_DISCONNECT_EVT:
        connect = false;
        get_server = false;
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_DISCONNECT_EVT, reason = %d", p_data->disconnect.reason);
        xEventGroupClearBits(APP_event_group,APP_event_BLE_CONNECTED_flags_BIT);
        sse_data[1] = 0; 
        sse_data[5] = 0;
        break;
    default:
        break;
    }
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;
    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        //the unit of the duration is second
        uint32_t duration = 30;
        esp_ble_gap_start_scanning(duration);
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTC_TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "scan start success");

        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
            ESP_LOGI(GATTC_TAG, "searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
            ESP_LOGI(GATTC_TAG, "searched Device Name Len %d", adv_name_len);
            esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);

            ESP_LOGI(GATTC_TAG, "\n");

            if (adv_name != NULL) {
                if (strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0) {
                    ESP_LOGI(GATTC_TAG, "searched device %s\n", remote_device_name);
                    if (connect == false) {
                        connect = true;
                        ESP_LOGI(GATTC_TAG, "connect to the remote device.");
                        esp_ble_gap_stop_scanning();
                        esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
                    }
                }
            }
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "scan stop failed, error status = %x", param->scan_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "stop scan successfully");
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "adv stop failed, error status = %x", param->adv_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "stop adv successfully");
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(GATTC_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gattc_if == gl_profile_tab[idx].gattc_if) {
                if (gl_profile_tab[idx].gattc_cb) {
                    gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}

void Read_ble_xTimerCallback(TimerHandle_t xTimer)
{
    xTimerStop(Read_ble_xTimer,portMAX_DELAY);
    xEventGroupClearBits(APP_event_group,APP_event_BLE_CONNECTED_flags_BIT);
    sse_data[1] = 0; 
    sse_data[5] = 0;
}

void ble_init(void * arg)
{
    esp_err_t ret;
    xEventGroupSetBits(APP_event_group,APP_event_BLE_CONNECTED_flags_BIT);
    Read_ble_xTimer = xTimerCreate("Timer0",(60000 / portTICK_PERIOD_MS)/*min*/ * 5,pdFALSE,( void * ) 0,Read_ble_xTimerCallback);//1min

/*  整体结构上，蓝牙可分为控制器(Controller)和主机(Host)两大部分；  
场景一(ESP-IDF默认)：在 ESP32 的系统上，选择 BLUEDROID 为蓝⽛牙主机，并通过 VHCI（软件实现的虚拟 HCI 接⼝口）接⼝口，访问控制器器。

场景⼆：在 ESP32 上运⾏控制器器（此时设备将单纯作为蓝⽛控制器使⽤），外接⼀个运⾏蓝⽛主机的设备（如运⾏ BlueZ 的 Linux PC、运⾏BLUEDROID 的 Android等）。

场景三：此场景与场景二类似，特别之处在于，在 BQB（或其它认证）的控制器测试下，可以将 ESP32 作为 DUT，用 UART 作为 IO 接口，接上认证测试的 PC 机，即可完成认证

*/
/*  初始化控制器    */
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    //register the  callback function to the gap module
    ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret){
        ESP_LOGE(GATTC_TAG, "%s gap register failed, error code = %x\n", __func__, ret);
        return;
    }

    //register the callback function to the gattc module
    ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    if(ret){
        ESP_LOGE(GATTC_TAG, "%s gattc register failed, error code = %x\n", __func__, ret);
        return;
    }

    ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
    if (ret){
        ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x\n", __func__, ret);
    }
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
    uint32_t duration = 30;
    while(duration)
    {
        vTaskDelay(320000 / portTICK_PERIOD_MS);
        EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
        if((uxBits & APP_event_BLE_CONNECTED_flags_BIT) == 0)
        {
            esp_ble_gap_start_scanning(duration);
        }
        else
        {
            //duration = 0;
        }
        if(xTimerIsTimerActive(Read_ble_xTimer) == pdFALSE)
        {
            xTimerReset(Read_ble_xTimer,portMAX_DELAY); 
        }
    }
    vTaskDelete(NULL);
}

#endif
