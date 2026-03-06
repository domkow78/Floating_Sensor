//#include <Arduino.h>
//#include <TaskScheduler.h>
#include "taskMeasure.h"
//#include <PubSubClient.h>
#include "logit.h"
#include "mqttMsg.h"
#include "main.h"
#ifdef DEFERRED_SEND
	#include "msgSender.h"
#endif
#include "timeespt.h"
#include "PTHsensorWrapper.h"
#include "tp.h"
#include "phaseMeas.h"

extern int humidity;
extern int temperature;
extern int pressure;
extern float moisture;  //acmm moisture, will be measured async. by acmm (not here) but reported here
//extern TaskHandle_t taskMeasure;

//extern PubSubClient mqttClient; //we will publish here the results
extern char hubID[16];

int taskMeasurePeriodMs = TASK_MEASURE_DEF_PERIOD_MS;
#define TASK_HEAP_MIN_FREE 100
static const char* TAG = "taskMeasure";
/*The function esp_log_level_set() cannot set logging levels higher than specified by CONFIG_LOG_DEFAULT_LEVEL.
 * To increase log level for a specific file at compile time, use the macro LOG_LOCAL_LEVEL before include esp_log.h
 */
//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"


/**
 * @brief Function executed by taskScheduler's taskMeasure periodically.
 * Do here only non-blocking operations.
 * Store data in global variables
 */
void taskMeasureCbk(void* ptr){
//	TickType_t xPreviousWakeTime = xTaskGetTickCount();
	TickType_t ts;
	for (;;){
		if (ulTaskNotifyTake(pdFALSE, portMAX_DELAY) > 1)
			jobState = JOB_STATE_UNFINISHED;
		// DHT11
		//unsigned long start = millis();
		ts = xTaskGetTickCount();
		pthWrapper.measure();
		//tp.measure(&imp, &impFilt);	//this comes now async.
		char payload[128];
		sprintf(payload, "{\"ts\":%llu,\"t\":%3i,\"h\":%3i,\"p\":%4i,"
				"\"acmm\":%3i,"
				"\"imp\":%4i,\"impi\":%4i,\"impc\":%4i,"
				"\"ph\":%4i,\"phf\":%4i}",
				timeEsp.getMs(), pthWrapper.temperature, pthWrapper.humidity, pthWrapper.pressure,
				(int)moisture,
				imp, (int16_t)tp.iir1.readMaxAbsAndReset(), (int16_t)tp.iir2.readMaxAbsAndReset(),
				phaseMeas.value, phaseMeas.freq);
		mqttMsg aMsg(TOPIC_DOMAIN, hubID, myID, TOPIC_MEASUREMENT, payload);
		//{"ts":1659952185088,"t":30,"h":25,"p":99922,"acmm":945,"imp":838,"impi":0,"impc":0,"ph":65,"phf":4600}

		//	cJSON *root = cJSON_CreateObject();
		//	cJSON_AddNumberToObject(root, "ts", static_cast<double>(ii));
		//	cJSON_AddStringToObject(root, "lm", strptr);
		//	cJSON_AddNumberToObject(root, "V", volt);
		//	cJSON_AddNumberToObject(root, "rssi", -info.rssi);
		//	cJSON_AddStringToObject(root, "fw", running_app_info.version);
		//	char *my_json_string = cJSON_Print(root);
		//	ESP_LOGI(TAG, "cJSON is %s",my_json_string);
		//	cJSON_Delete(root);

		ESP_LOGV(TAG, "taskMeasureCbk: %s", payload);

		msgSender.post(aMsg);

		UBaseType_t minFreeHeap = uxTaskGetStackHighWaterMark( NULL );
		if ( minFreeHeap < TASK_HEAP_MIN_FREE)
			ESP_LOGW(TAG, " HEAP is short! Only %d left.", minFreeHeap);

//		vTaskDelayUntil(&xPreviousWakeTime, pdMS_TO_TICKS(taskMeasurePeriodMs)); //this scenario fails with ulTaskNotifyTake
		vTaskDelay(pdMS_TO_TICKS(taskMeasurePeriodMs - (xTaskGetTickCount()-ts)));
	}
}


