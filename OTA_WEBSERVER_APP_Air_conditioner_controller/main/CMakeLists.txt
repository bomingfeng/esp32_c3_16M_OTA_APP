set(COMPONENT_SRCS ./LED_Seg7Menu/SegDynamicScan.c ./LED_Seg7Menu/key.c ./LED_Seg7Menu/LED_Seg7Menu.c  ./LED_Seg7Menu/menu_00.c  ./LED_Seg7Menu/menu_01.c  ./LED_Seg7Menu/menu_02.c  ./LED_Seg7Menu/menu_03.c  ./LED_Seg7Menu/Menu.c  ./LED_Seg7Menu/Message.c  ./MultiButton/multi_button.c ./MultiButton/MultiButton_poll_Task.c cpu_timer.c BLE_Client.c temperature_control_task.c ir_tx_task.c ir_rx_task.c ds18b20_Task.c led_Task.c app_init.c MyWiFi.c OTAServer.c tcp_client.c sntp_task.c ADC1_single_read_Task.c bt_hci_common.c ble_adv_scan_Task.c htmltomcu.c main.c)
set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_EMBED_TXTFILES WebsiteFiles/index.html WebsiteFiles/favicon.ico WebsiteFiles/jquery-3.4.1.min.js)

register_component()
