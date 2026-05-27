/*
 * This is where we decode MQTT messages, process it, etc.
 * Pls. avoid long-blocking actions here, it is only to decode, extract data, set parameters or variables, setup tasks, and leave
 */

//#include <Arduino.h>
#include "msgSender.h"

//#include <TaskSchedulerDeclarations.h>
//#include <PubSubClient.h>
#include "mqtt_client.h"
#include "acmm.h"
//#include "logit.h"
#include "main.h"
static const char* TAG = "taskSend";
/*The function esp_log_level_set() cannot set logging levels higher than specified by CONFIG_LOG_DEFAULT_LEVEL.
 * To increase log level for a specific file at compile time, use the macro LOG_LOCAL_LEVEL before include esp_log.h
 */
//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#define TASK_HEAP_MIN_FREE 200
#define MIN_HEAP_SIZE 400

#define TASK_SEND_HEAP 4000
#define SENDER_QUEUE_MAX_SIZE 20

void taskSendCbk(void* ptr){
#ifdef DEFERRED_SEND
	TickType_t xPreviousWakeTime = xTaskGetTickCount();
	for (;;){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		ESP_LOGV(TAG, "taskSendCbk: Will send %d msgs...", msgSender.qlen());
		msgSender.forceSend();
		ESP_LOGV(TAG, " done.");

		UBaseType_t minFreeHeap = uxTaskGetStackHighWaterMark( NULL );
		if ( minFreeHeap < TASK_HEAP_MIN_FREE)
			ESP_LOGW(TAG, " HEAP is short! Only %d left.", minFreeHeap);

		vTaskDelayUntil(&xPreviousWakeTime, pdMS_TO_TICKS(msgSender.taskSendIntervalMs));
	}
#endif
}

MsgSender msgSender;

//// declare a pointer to member function
//void (className::* ptfptr) (int) = &X::f;
// call member function
// className obj;
//  (obj.*ptfptr) (20);

MsgSender::MsgSender(){
	sendThreshold = msgSenderThr_t::Half;
#ifdef DEFERRED_SEND
	spinlock_initialize(&lock);
	xTaskCreate(&taskSendCbk, "taskSend", TASK_SEND_HEAP, NULL, tskIDLE_PRIORITY, &htask);
	vTaskSuspend(htask);
//	vPortCPUInitializeMutex(&m_mux);
#endif
	return;
}

void MsgSender::init(void){
	vTaskResume(htask);
}

MsgSender::~MsgSender(){
	//delete what should;
}

void MsgSender::flush(void){
#ifdef DEFERRED_SEND
	while (!m_msgQueue.isEmpty())
		m_msgQueue.dequeue();
#endif
}

int MsgSender::qlen(void){
#ifdef DEFERRED_SEND
	return m_msgQueue.count();
#else
	return 0;
#endif
}


/* posts a message to a queue for later sending, or directly to mqtt client*/
bool MsgSender::post(mqttMsg msg){
	bool res = true;

	//esp_get_free_heap_size( void ) eee???

#ifdef DEFERRED_SEND
	if (m_msgQueue.count() < SENDER_QUEUE_MAX_SIZE){
		//portENTER_CRITICAL(&m_mux);
		//SPINLOCK_WAIT_FOREVER
		if (!spinlock_acquire(&lock, pdMS_TO_TICKS(2000) )){
			ESP_LOGW(TAG, "enqueue: could not get spinlock, skipping this message");
			return false;
		}
		m_msgQueue.enqueue(msg);
		//	portEXIT_CRITICAL(&m_mux);
		spinlock_release(&lock);
		xTaskNotify(htask, 1, eIncrement);
	} else
		ESP_LOGW(TAG, "MsgSender::post: sender queue size unreasonably huge, skipping.");

#else
	res = res & (-1 != esp_mqtt_client_publish(mqttClient, (const char *)msg.topic, (const char *)msg.payload, 0, 1, 0));
#endif

	//uint32_t* hfree; uint16_t* hmax; byte_t* hfrag;
	//ESP.getHeapStats(hfree, hmax, hfrag);
	//ESP.getFreeHeap()
	if (heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) < MIN_HEAP_SIZE){
		ESP_LOGW(TAG, "MsgSender::enqueue: running short of heap space , sending all mqtt data.\n");
		sendFromQueue();
	}
	return res;
}

/* posts a message to a queue for later sending, or directly to mqtt client*/
bool MsgSender::post(char* topic, uint8_t* payload, unsigned int payLen){
	mqttMsg msg = mqttMsg(topic,payload,payLen);
	return this->post(msg);
}

bool MsgSender::forceSend(){
	return sendFromQueue();
}

bool MsgSender::sendFromQueue(void){
	ESP_LOGV(TAG, "MsgSender::send: start\n");
	bool res = true;
#ifdef DEFERRED_SEND
	mqttMsg msg;
	while (!m_msgQueue.isEmpty()){
		msg = m_msgQueue.dequeue();
		ESP_LOGV(TAG, "MsgSender::send: got msg\n");
		res = res & (-1 != esp_mqtt_client_publish(mqttClient, (const char *)msg.topic, (const char *)msg.payload, 0, 1, 0));
		ESP_LOGV(TAG, "MsgSender::send: publish\n");
	}
	ESP_LOGV(TAG, "MsgSender::send: end.\n");
#endif
	return res;
}
