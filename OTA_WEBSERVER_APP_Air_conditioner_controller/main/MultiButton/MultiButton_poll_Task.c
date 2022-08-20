#include "multi_button.h"
#include "MultiButton_poll_Task.h"

#define btn1_id 0 
struct Button btn1;

uint8_t read_button_GPIO(uint8_t button_id)
{
	// you can share the GPIO read function with multiple Buttons
	switch(button_id)
	{
		case btn1_id:
			return gpio_get_level(CONFIG_INPUT_GPIO);
			break;
/*
		case 1:
			return gpio_get_level(CONFIG_INPUT_GPIO);
			break;

		case 2:
			return gpio_get_level(CONFIG_INPUT_GPIO);
			break;

		case 3:
			return gpio_get_level(CONFIG_INPUT_GPIO);
			break;

		case 4:
			return gpio_get_level(CONFIG_INPUT_GPIO);
			break;	

		case 5:
			return gpio_get_level(CONFIG_INPUT_GPIO);
			break;

		case 6:
			return gpio_get_level(CONFIG_INPUT_GPIO);
			break;

		case 7:
			return gpio_get_level(CONFIG_INPUT_GPIO);
			break;
*/
		default:
			return 0;
			break;
	}
}

TimerHandle_t ButtonTimers;

void vButtonTimersCallback(TimerHandle_t xTimer)
{
    button_ticks();
}

void BTN1_SINGLE_Click_Handler(void* btn)
{
	//do something...
	printf("BTN1_SINGLE_Click_Handler!\r\n");
}

void BTN1_DOUBLE_Click_Handler(void* btn)
{
	//do something...
	printf("BTN1_DOUBLE_Click_Handler!\r\n");
}

void BTN1_PRESS_REPEAT_Handler(void* btn)
{
	//do something...
	printf("BTN1_PRESS_REPEAT_Handler!\r\n");
}

void BTN1_LONG_PRESS_START_Handler(void* btn)
{
	//do something...
	printf("BTN1_LONG_PRESS_START_Handler!\r\n");
}


void MultiButton_poll_Task(void *pvParam)
{
	static uint8_t btn1_event_val;
	
	button_init(&btn1, read_button_GPIO, 0, btn1_id);
	button_start(&btn1);
	
	//make the timer invoking the button_ticks() interval 5ms.
	//This function is implemented by yourself.
	//__timer_start(button_ticks, 0, 5); 
	ButtonTimers = xTimerCreate("ButtonTimers",5,pdTRUE,( void * ) 0,vButtonTimersCallback);
	xTimerStart(ButtonTimers,portMAX_DELAY);

	button_attach(&btn1, SINGLE_CLICK,     BTN1_SINGLE_Click_Handler);
	button_attach(&btn1, DOUBLE_CLICK,     BTN1_DOUBLE_Click_Handler);
	button_attach(&btn1, LONG_PRESS_START, BTN1_LONG_PRESS_START_Handler);

	vTaskDelete(NULL);
	while(1) 
	{
		if(btn1_event_val != get_button_event(&btn1)) {
			btn1_event_val = get_button_event(&btn1);
			
			if(btn1_event_val == PRESS_DOWN) {
				//do something
				printf("PRESS_DOWN!\r\n");
			} else if(btn1_event_val == PRESS_UP) {
				//do something
				printf("PRESS_UP!\r\n");
			} else if(btn1_event_val == LONG_PRESS_HOLD) {
				//do something
				printf("LONG_PRESS_HOLD!\r\n");
			}
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

