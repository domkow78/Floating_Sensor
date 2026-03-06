#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
//#include "timer_group_struct.h"
#include <driver/i2s.h>
#include "blink.h"

static const char* TAG = "BLINK";

/*The function esp_log_level_set() cannot set logging levels higher than specified by CONFIG_LOG_DEFAULT_LEVEL.
 * To increase log level for a specific file at compile time, use the macro LOG_LOCAL_LEVEL before include esp_log.h
 */
//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

Blink_t blink;


void taskBlinkCbk(void* ptr){
	uint64_t code;
	gpio_num_t gpionum;
	for (;;){
		if (blink.m_requestSuspend){
			//ESP_LOGV(TAG, "blink::cbk: suspend start...");
			vTaskDelay(pdMS_TO_TICKS(BLINK_RESUME_MS));
			blink.m_requestSuspend = false;
			//ESP_LOGV(TAG, "blink::cbk: suspend end.");
		}else{
			xQueueReceive(blink.qu, &code, portMAX_DELAY);

			gpionum = (code & 0x0000000000000001) ? BLINK_PIN_GREEN : BLINK_PIN_RED;
			//ESP_LOGV(TAG, "blink::cbk: %llx...", code);
			for (int bitpos = 0; bitpos < BLINK_CODE_LEN-1; bitpos++){
				gpio_set_level(gpionum, !(code & 0x8000000000000000)); // we use OpenDrain
				code = code << 1;
				vTaskDelay(pdMS_TO_TICKS(BLINK_BIT_MS));
			}
			gpio_set_level(gpionum, 1); // we use OpenDrain
			vTaskDelay(pdMS_TO_TICKS(BLINK_BIT_MS));
			//ESP_LOGV(TAG, "blink::cbk: done.");

			UBaseType_t minFreeHeap = uxTaskGetStackHighWaterMark( NULL ); // @suppress("Invalid arguments")
			if ( minFreeHeap < TASK_HEAP_MIN_FREE)
				ESP_LOGW(TAG, " HEAP is short! Only %d left.", minFreeHeap);

			blink.isEmpty = (uxQueueMessagesWaiting(blink.qu)==0);
			vTaskDelay(pdMS_TO_TICKS(BLINK_PAUSE_AFTER));
			//taskYIELD();
		}
	}
}


/**
 * @brief Initializes hardware used by this class
 * @param
 * @return True on success
 */
bool Blink_t::init() {

	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT_OD;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask =  ((1ULL<<BLINK_PIN_RED) | (1ULL<<BLINK_PIN_GREEN));
	//disable pull-down mode
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	//disable pull-up mode
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	//configure GPIO with the given settings
	ESP_LOGI(TAG, "Blink_t::init(): gpio_config...");
	if (ESP_OK != gpio_config(&io_conf)){
		ESP_LOGW(TAG, "Blink_t::init(): failed gpio_config on BLINK_PIN");
		return false;
	}
	gpio_set_level(BLINK_PIN_RED, 1); // we use OpenDrain
	gpio_set_level(BLINK_PIN_GREEN, 1); // we use OpenDrain

	ESP_LOGI(TAG, "Blink_t::init(): done.");
	isInitialized = true;
	return true;
}

Blink_t::Blink_t() {
	qu = xQueueCreate(BLINK_QUEUE_LEN, sizeof(uint64_t));
	xTaskCreate(&taskBlinkCbk, "taskBlink", BLINK_STACK_DEPTH, NULL, 0, NULL);
	return;
}

/**
 * @brief Dispatch and schedule a code to blink
 * @param
 * 	uint64_t code: a code to be blinked out
 * 	Blink_led led: led color to use
 * 	bool blocking: should it wait until done or leave immediately, defaults false
 * @return True on success
 */
bool Blink_t::show(uint64_t code, Blink_led led, bool blocking) {
	if (!isInitialized)
		return false;
	switch (led){
	case Blink_led::green:
		code |= 0x0000000000000001;
		break;
	case Blink_led::red:
		code &= 0xFFFFFFFFFFFFFFFE;
		break;
	}
	bool ret;
	ret = (pdTRUE == xQueueSend(blink.qu, &code, 0));
	if (ret){
		if (blocking){
			while(!isEmpty){
				taskYIELD();
			}
		}
	}
	return ret;
	//taskYIELD();
}

void Blink_t::allowSuspend(bool yes) {
	m_allowSuspend = yes;
}

void Blink_t::requestSuspend(void) {
	if (m_allowSuspend){
		m_requestSuspend = true;
	}
}
