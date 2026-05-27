/*
 * This is where we decode MQTT messages, process it, etc.
 * Pls. avoid long-blocking actions here, it is only to decode, extract data, set parameters or variables, setup tasks, and leave
 */

#include "msgProcessor.h"
#include "battMeas.h"
#include "esp_wifi_types.h"
#include "esp_wifi.h"
#include "eeprom.h"
//#include <PubSubClient.h>
#include "mqtt_client.h"
#include "acmm.h"
#include "taskMeasure.h"
#include "main.h"
#ifdef DEFERRED_SEND
	#include "msgSender.h"
#endif
#include "otaUpgrade.h"
#include <esp_ota_ops.h>
#include "timeespt.h"
#include "fileLoger.h"
#include "config.h"
#include "esp_sleep.h"
#include "phaseMeas.h"
#include "tp.h"
#include "runtimestats.h"

static const char* TAG = "MQTT";

/*The function esp_log_level_set() cannot set logging levels higher than specified by CONFIG_LOG_DEFAULT_LEVEL.
 * To increase log level for a specific file at compile time, use the macro LOG_LOCAL_LEVEL before include esp_log.h
 */
//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#define MSGPROC_QUEUE_WARN	10


MsgProcessor::MsgProcessor(){
	//check on https://arduinojson.org/v6/assistant/ for the biggest type of a message You envision
//	const size_t jsonCapacity = JSON_OBJECT_SIZE(5) + 70;
//	m_jsonRoot = new DynamicJsonDocument(jsonCapacity);
	m_pMsgQueue = (QueueArray<mqttMsg>*) new QueueArray<mqttMsg>(5);
	jroot = NULL;
	return;
}

MsgProcessor::~MsgProcessor(){
//	delete m_jsonRoot;
}

void MsgProcessor::flush(void){
	while (!m_pMsgQueue->isEmpty())
		m_pMsgQueue->dequeue();
}

bool MsgProcessor::enqueue(mqttMsg msg){
	if (m_pMsgQueue->count() >= MSGPROC_QUEUE_WARN ){
		ESP_LOGW(TAG, "queue is over %d, discarding this msg", MSGPROC_QUEUE_WARN);
		return false;
	}else{
		m_pMsgQueue->enqueue(msg);
		return true;
	}
}

bool MsgProcessor::enqueue(const char* topic, const int topiclen, const char* payload, const int payloadlen){
	ESP_LOGV(TAG, "MsgProcessor::enqueue...");
	mqttMsg msg = mqttMsg(topic, topiclen, payload, payloadlen);
	ESP_LOGV(TAG, ": %s/%s/%s/%s\n",msg.domain, msg.target, msg.source, msg.smallTopic);
	return this->enqueue(msg);
}

bool MsgProcessor::enqueue(const char* topic, const char* payload, const int payloadlen){
	return enqueue(topic, strlen(topic), payload, payloadlen);
}

bool MsgProcessor::enqueue(const char* topic, const char* payload){
	return enqueue(topic, strlen(topic), payload, strlen(payload));
}

void sendStatus(const char* src, const char* lastSmallTopic){
	ESP_LOGV(TAG, "sendStatus: begin...");

	wifi_ap_record_t info;
	info.rssi = 0;
	if (ESP_OK != esp_wifi_sta_get_ap_info(&info)){
		ESP_LOGE(TAG, "sendStatus: failed to get RSSI.");
	}
	ESP_LOGV(TAG, "sendStatus: 111");

	const esp_partition_t *running = esp_ota_get_running_partition();
	esp_app_desc_t running_app_info;
	strcpy(running_app_info.version, "unknown");
	if (running != NULL){
		if (esp_ota_get_partition_description(running, &running_app_info) != ESP_OK) {
			ESP_LOGE(TAG, "Running firmware version: failed");
		}
	}
	double volt = (double)battMeas.measMv()/1000;
	//double volt = 3.7;
	const char* strptr = "unknown";
	if (lastSmallTopic != NULL)
		strptr = lastSmallTopic;
	uint64_t ii = timeEsp.getMs();


//	cJSON *root = cJSON_CreateObject();
//	cJSON_AddNumberToObject(root, "ts", static_cast<double>(ii));
//	cJSON_AddStringToObject(root, "lm", strptr);
//	cJSON_AddNumberToObject(root, "V", volt);
//	cJSON_AddNumberToObject(root, "rssi", -info.rssi);
//	cJSON_AddStringToObject(root, "fw", running_app_info.version);
//	char *my_json_string = cJSON_Print(root);
//	ESP_LOGI(TAG, "cJSON is %s",my_json_string);
//	cJSON_Delete(root);


	char payload[90];
	//memset(payload, 0, 90);
		//"x: %"PRId64"
	sprintf(payload,"{\"ts\":%llu,\"lm\":\"%.9s\",\"V\":%1.2f,\"rssi\":%2i,\"fw\":\"%.15s\"}",
					ii, strptr, volt, -info.rssi, running_app_info.version);
	mqttMsg aMsg(TOPIC_DOMAIN, src, myID, TOPIC_STATUS, payload);
//	mqttClient.publish(aMsg.topic, aMsg.payload);
	esp_mqtt_client_publish(
				mqttClient, (const char*)(aMsg.topic), (const char*)(aMsg.payload), 0, 2, 0);

	ESP_LOGV(TAG, "sendStatus : %s %s", aMsg.topic, payload);
	vTaskDelay(pdMS_TO_TICKS(200));
	printRunTimeStats();

}

/*	A object method wrapper, to be able to pass as function pointer
 * 	Returns false on fail, often when packets too long*/
static bool publishLogWrapper(const char* aPayload){
	mqttMsg msg(TOPIC_DOMAIN, hubID, myID, TOPIC_LOG, aPayload);
//	return mqttClient.publish(msg.topic, msg.payload);
	return (-1 != esp_mqtt_client_publish(
			mqttClient, (const char*)(msg.topic), (const char*)(msg.payload), 0, 2, 0));
}


//				cJSON *root;
//				root = cJSON_CreateObject();
//				esp_chip_info_t chip_info;
//				esp_chip_info(&chip_info);
//				cJSON_AddStringToObject(root, "version", IDF_VER);
//				cJSON_AddNumberToObject(root, "cores", chip_info.cores);
//				cJSON_AddTrueToObject(root, "flag_true");
//				cJSON_AddFalseToObject(root, "flag_false");
//				//const char *my_json_string = cJSON_Print(root);
//				char *my_json_string = cJSON_Print(root);
//				ESP_LOGI(TAG_SETUP, "my_json_string\n%s",my_json_string);
//				cJSON_Delete(root);


bool MsgProcessor::execute(){
	enum StatusType { none, processed, unprocessed, failed, nottome };
	StatusType status = none;
	char st[20];
	cJSON* item;

	if (m_pMsgQueue->isEmpty()){
		return false;
	}
	ESP_LOGI(TAG, "MsgProcessor::execute: there is %i msgs in queue.",m_pMsgQueue->count());
	mqttMsg msg = m_pMsgQueue->pop();
	ESP_LOGI(TAG, "MsgProcessor::execute: Popped %s/%s/%s/%s",msg.domain, msg.target, msg.source,msg.smallTopic);

	if (strstr(msg.smallTopic,"registerCall")) // message identified, we know how to parse it
	{
		ESP_LOGI(TAG, "MsgProcessor::execute: Got a register call from %s. ", msg.source);
		mqttMsg msgRep(TOPIC_DOMAIN, msg.source, myID, TOPIC_REGISTER_ME, "foo");
//		ESP_LOGI(TAG, "Replying %s", msgRep.topic);
		esp_mqtt_client_publish(
						mqttClient, (const char*)(msgRep.topic), (const char*)(msgRep.payload), 0, 2, 0);
		sendStatus(msg.source, msg.smallTopic);
		status = processed;
	}else
	{
		if (strcmp(msg.target,myID)==0){ // this is to me

			strcpy(st,"startMeas");
			if (strstr(msg.smallTopic,st)) // message identified, we know how to parse it
			{
				int count = -1;
				jroot = cJSON_Parse(msg.payload);  //https://github.com/nopnop2002/esp-idf-json
				if (jroot != NULL){
					item = cJSON_GetObjectItem(jroot, "count");
					if (item){
						count = item->valueint;
						ESP_LOGI(TAG, "MsgProcessor::execute: %s: count: %i", st, count);
					}
					cJSON_Delete(jroot);
				}
				EEPROM.begin(EEPROM_SIZE);
				EEPROM.writeByte(EEPROM_OFFS_JOB_STATE_1B, JOB_STATE_UNFINISHED);
				EEPROM.commit();
				xTaskNotify(taskMeasure, count==-1 ? 0xFFFFFFFF : count, eSetValueWithOverwrite);
				vTaskResume(taskMeasure);

				/*in case acmmFreq was non-zero, make it one of the measurements*/
				if (acmm.freq != 0){
					acmm.run();
				}else{
					acmm.stop();
				}
				sendStatus(msg.source, msg.smallTopic);
				status = processed;
			}
//				m_jsonError = deserializeJson(*m_jsonRoot, msg.payload);
//				if (m_jsonError) {
//					ESP_LOGI(TAG, "MsgProcessor::execute: startMeas parsing failed!\n");
//				} else {
//					int count = -1;
//					if (m_jsonRoot->containsKey("count")){
//						count = (*m_jsonRoot)["count"].as<signed int>();
//						ESP_LOGI(TAG, "MsgProcessor::execute: startMeas: count: %i\n",count);
//					}
//					EEPROM.begin(EEPROM_SIZE);
//					EEPROM.writeByte(EEPROM_OFFS_JOB_STATE_1B, 0x00);
//					EEPROM.commit();
//					ESP_LOGI(TAG, "MsgProcessor::execute: setting taskMeasure iterations to %i\n",count);
//					xTaskNotify(taskMeasure, count==-1 ? 0xFFFFFFFF : count, eSetValueWithOverwrite);
//					vTaskResume(taskMeasure);
//					/*in case acmmFreq was non-zero, make it one of the measurements*/
//					if (acmm.freq != 0){
//						acmm.run();
//					}else{
//						acmm.stop();
//					}
//					sendStatus(msg.source, msg.smallTopic);
//					status = processed;
//				}

			strcpy(st, "pm");	//power management
			if (strstr(msg.smallTopic, st)){ // message identified, we know how to parse it
				ESP_LOGI(TAG, "MsgProcessor::execute: Got a %s from %s", st, msg.source);
				jroot = cJSON_Parse(msg.payload);  //https://github.com/nopnop2002/esp-idf-json
				if (jroot == NULL){
					ESP_LOGW(TAG, "MsgProcessor::execute: %s parsing failed!", st);
				}
				else{
					item = cJSON_GetObjectItem(jroot, "value");
					if (item){
						powerSaveMode(!strcmp(item->valuestring, "on"));
					}else
						ESP_LOGW(TAG, "MsgProcessor::execute: %s arg. parsing failed!", st);
					sendStatus(msg.source, msg.smallTopic);
					status = processed;
					cJSON_Delete(jroot);
				}
			}
			strcpy(st, "phase");	//enable phase meas
			if (strstr(msg.smallTopic, st)){ // message identified, we know how to parse it
				ESP_LOGI(TAG, "MsgProcessor::execute: Got a %s from %s", st, msg.source);
				jroot = cJSON_Parse(msg.payload);  //https://github.com/nopnop2002/esp-idf-json
				if (jroot == NULL){
					ESP_LOGW(TAG, "MsgProcessor::execute: %s parsing failed!", st);
				}
				else{
					item = cJSON_GetObjectItem(jroot, "value");
					if (item){
						phaseMeas.enable(!strcmp(item->valuestring, "on"));
					}else
						ESP_LOGW(TAG, "MsgProcessor::execute: %s arg. parsing failed!", st);
					sendStatus(msg.source, msg.smallTopic);
					status = processed;
					cJSON_Delete(jroot);
				}
			}
			strcpy(st, "tp");	//enable tp
			if (strstr(msg.smallTopic, st)){ // message identified, we know how to parse it
				ESP_LOGI(TAG, "MsgProcessor::execute: Got a %s from %s", st, msg.source);
				jroot = cJSON_Parse(msg.payload);  //https://github.com/nopnop2002/esp-idf-json
				if (jroot == NULL){
					ESP_LOGW(TAG, "MsgProcessor::execute: %s parsing failed!", st);
				}
				else{
					item = cJSON_GetObjectItem(jroot, "value");
					if (item){
//						ESP_LOGI(TAG, "MsgProcessor::execute: %s %s", st, item->valuestring);
						tp.enable(!strcmp(item->valuestring, "on"));
					}else
						ESP_LOGW(TAG, "MsgProcessor::execute: %s arg. parsing failed!", st);
					sendStatus(msg.source, msg.smallTopic);
					status = processed;
					cJSON_Delete(jroot);
				}
			}
			strcpy(st, "interlock");	//acmm vs tp interlock
			if (strstr(msg.smallTopic, st)){ // message identified, we know how to parse it
				ESP_LOGI(TAG, "MsgProcessor::execute: Got a %s from %s", st, msg.source);
				jroot = cJSON_Parse(msg.payload);  //https://github.com/nopnop2002/esp-idf-json
				if (jroot == NULL){
					ESP_LOGW(TAG, "MsgProcessor::execute: %s parsing failed!", st);
				}
				else{
					item = cJSON_GetObjectItem(jroot, "value");
					if (item){
						if (!strcmp(item->valuestring, "off"))
							acmm.interlockMode = acmmInterlockMode_t::off;
						else if (!strcmp(item->valuestring, "hiz"))
							acmm.interlockMode = acmmInterlockMode_t::hiz;
						else if (!strcmp(item->valuestring, "gnd"))
							acmm.interlockMode = acmmInterlockMode_t::gnd;
						else
							ESP_LOGW(TAG, "MsgProcessor::execute: wrong argument");
					}else
						ESP_LOGW(TAG, "MsgProcessor::execute: %s arg. parsing failed!", st);
					sendStatus(msg.source, msg.smallTopic);
					status = processed;
					cJSON_Delete(jroot);
				}
			}
			strcpy(st, "stopMeas");
			if (strstr(msg.smallTopic, st)){ // message identified, we know how to parse it
				ESP_LOGI(TAG, "MsgProcessor::execute: Got a %s from %s", st, msg.source);
				vTaskSuspend(taskMeasure);
				acmm.stop();
//				tp.stop();
//				phaseMeas.stop();
				sendStatus(msg.source, msg.smallTopic);
				status = processed;
			}
			strcpy(st, "clearAcmm");
			if (strstr(msg.smallTopic, st)) // message identified, we know how to parse it
			{
				jroot = cJSON_Parse(msg.payload);  //https://github.com/nopnop2002/esp-idf-json
				if (jroot == NULL){
					ESP_LOGW(TAG, "MsgProcessor::execute: %s parsing failed!", st);
				}
				else{
					int initValue = 0;
					item = cJSON_GetObjectItem(jroot, "initValue");
					if (item)
						initValue = item->valueint;
					ESP_LOGI(TAG, "MsgProcessor::execute: clearing Acmm to %i", initValue);
					acmm.stop();
					moisture = (float)initValue;
					acmm.run();
					sendStatus(msg.source, msg.smallTopic);
					status = processed;
					cJSON_Delete(jroot);
				}
			}
			strcpy(st, "clearImp");
			if (strstr(msg.smallTopic, st)) // message identified, we know how to parse it
			{
				jroot = cJSON_Parse(msg.payload);  //https://github.com/nopnop2002/esp-idf-json
				if (jroot == NULL){
					ESP_LOGW(TAG, "MsgProcessor::execute: %s parsing failed!", st);
				}
				else{
					int initValue = 0;
					item = cJSON_GetObjectItem(jroot, "initValue");
					if (item)
						initValue = item->valueint;
					ESP_LOGI(TAG, "MsgProcessor::execute: clearing Imp to %i", initValue);
					*(tp.ptrVal) = initValue;
					sendStatus(msg.source, msg.smallTopic);
					status = processed;
					cJSON_Delete(jroot);
				}
			}
			strcpy(st, "measFreq");
			if (strstr(msg.smallTopic, st)) // message identified, we know how to parse it
			{
				jroot = cJSON_Parse(msg.payload);
				if (jroot == NULL){
					ESP_LOGW(TAG, "MsgProcessor::execute: %s parsing failed!", st);
				}
				else{
					item = cJSON_GetObjectItem(jroot, "freq");
					if (item){
						int freq = item->valueint;
						ESP_LOGI(TAG, "MsgProcessor::execute: %s: freq %i\n", st, freq);
						taskMeasurePeriodMs = 1000*freq;
						sendStatus(msg.source, msg.smallTopic);
						status = processed;
					} else
						ESP_LOGW(TAG, "MsgProcessor::execute: %s parsing failed!", st);
					cJSON_Delete(jroot);
				}
//				m_jsonError = deserializeJson(*m_jsonRoot, msg.payload);
//				if (m_jsonError || !m_jsonRoot->containsKey("freq")) {
//					ESP_LOGI(TAG, "MsgProcessor::execute: measFreq parsing failed\n!");
//				} else {
//					int freq = (*m_jsonRoot)["freq"].as<signed int>();
//					ESP_LOGI(TAG, "MsgProcessor::execute: measFreq: freq %i\n",freq);
//					taskMeasurePeriodMs = 1000*freq;
//					sendStatus(msg.source, msg.smallTopic);
//					status = processed;
//				}
			}
#ifdef DEFERRED_SEND
			strcpy(st, "sendInterval");
			if (strstr(msg.smallTopic, st)) // message identified, we know how to parse it
			{
				jroot = cJSON_Parse(msg.payload);
				if (jroot == NULL){
					ESP_LOGW(TAG, "MsgProcessor::execute: %s parsing failed!", st);
				}
				else{
					item = cJSON_GetObjectItem(jroot, "sendInterval");
					if (item){
						msgSender.taskSendIntervalMs = item->valueint * 1000;
						ESP_LOGI(TAG, "MsgProcessor::execute: sendInterval %i ms", msgSender.taskSendIntervalMs);
						sendStatus(msg.source, msg.smallTopic);
						status = processed;
					}else
						ESP_LOGW(TAG, "MsgProcessor::execute: %s arg. parsing failed!", st);
					cJSON_Delete(jroot);
				}
			}
#endif
			strcpy(st,"setAcmm");
			if (strstr(msg.smallTopic, st)) // message identified, we know how to parse it
			{
				jroot = cJSON_Parse(msg.payload);
				if (jroot == NULL){
					ESP_LOGW(TAG, "MsgProcessor::execute: %s parsing failed!", st);
				}
				else{
					item = cJSON_GetObjectItem(jroot, "freq");
					if (item){
						acmm.freq = item->valueint;
						ESP_LOGI(TAG, "MsgProcessor::execute: %s: freq %i", st, acmm.freq);
					}
					item = cJSON_GetObjectItem(jroot, "aggOver");
					if (item){
						acmm.aggOver = item->valueint;
						ESP_LOGI(TAG, "MsgProcessor::execute: %s: aggOver %i", st, acmm.aggOver);
					}
					item = cJSON_GetObjectItem(jroot, "phLoF");
					if (item){
						phaseMeas.freqLo = item->valueint;
						ESP_LOGI(TAG, "MsgProcessor::execute: %s: phLoF %i", st, phaseMeas.freqLo);
					}
					item = cJSON_GetObjectItem(jroot, "phHiF");
					if (item){
						phaseMeas.freqHi = item->valueint;
						ESP_LOGI(TAG, "MsgProcessor::execute: %s: phLoF %i", st, phaseMeas.freqHi);
					}
					sendStatus(msg.source, msg.smallTopic);
					status = processed;
					cJSON_Delete(jroot);
				}
			}
			strcpy(st,"setIir");
			if (strstr(msg.smallTopic, st)) // message identified, we know how to parse it
			{
				jroot = cJSON_Parse(msg.payload);
				if (jroot == NULL){
					ESP_LOGW(TAG, "MsgProcessor::execute: %s parsing failed!", st);
				}
				else{
					item = cJSON_GetObjectItem(jroot, "value");
					if (item){
						char* pNext;
						tp.iir1.a1 = strtof(item->valuestring, &pNext);
						ESP_LOGI(TAG, "MsgProcessor::execute: %s a11: %1.5f", st, tp.iir1.a1);
						tp.iir1.a2 = strtof(pNext, &pNext);
						ESP_LOGI(TAG, "MsgProcessor::execute: %s a21: %1.5f", st, tp.iir1.a2);
						tp.iir1.b0 = strtof(pNext, &pNext);
						ESP_LOGI(TAG, "MsgProcessor::execute: %s b01: %1.5f", st, tp.iir1.b0);
						tp.iir1.b1 = strtof(pNext, &pNext);
						ESP_LOGI(TAG, "MsgProcessor::execute: %s b11: %1.5f", st, tp.iir1.b1);
						tp.iir1.b2 = strtof(pNext, &pNext);
						ESP_LOGI(TAG, "MsgProcessor::execute: %s b21: %1.5f", st, tp.iir1.b2);

						tp.iir2.a1 = strtof(pNext, &pNext);
						ESP_LOGI(TAG, "MsgProcessor::execute: %s a12: %1.5f", st, tp.iir2.a1);
						tp.iir2.a2 = strtof(pNext, &pNext);
						ESP_LOGI(TAG, "MsgProcessor::execute: %s a22: %1.5f", st, tp.iir2.a2);
						tp.iir2.b0 = strtof(pNext, &pNext);
						ESP_LOGI(TAG, "MsgProcessor::execute: %s b02: %1.5f", st, tp.iir2.b0);
						tp.iir2.b1 = strtof(pNext, &pNext);
						ESP_LOGI(TAG, "MsgProcessor::execute: %s b12: %1.5f", st, tp.iir2.b1);
						tp.iir2.b2 = strtof(pNext, NULL);
						ESP_LOGI(TAG, "MsgProcessor::execute: %s b22: %1.5f", st, tp.iir2.b2);
						tp.iir1.reset();
						tp.iir2.reset();

//						tp.comb.a = strtof(pNext, &pNext);
//						ESP_LOGI(TAG, "MsgProcessor::execute: %s a: %1.5f", st, tp.comb.a);
//						tp.comb.k = (uint8_t)strtof(pNext, NULL);
//						ESP_LOGI(TAG, "MsgProcessor::execute: %s k: %d", st, tp.comb.k);
					}
					sendStatus(msg.source, msg.smallTopic);
					status = processed;
					cJSON_Delete(jroot);
				}
			}
//			strcpy(st,"setComb");
//			if (strstr(msg.smallTopic, st)) // message identified, we know how to parse it
//			{
//				jroot = cJSON_Parse(msg.payload);
//				if (jroot == NULL){
//					ESP_LOGW(TAG, "MsgProcessor::execute: %s parsing failed!", st);
//				}
//				else{
//					item = cJSON_GetObjectItem(jroot, "value");
//					if (item){
//						char* pNext;
//						tp.comb.a = strtof(item->valuestring, &pNext);
//						tp.comb.k = strtof(pNext, NULL);
//						ESP_LOGI(TAG, "MsgProcessor::execute: %s: %s", st, item->valuestring);
//						sendStatus(msg.source, msg.smallTopic);
//						status = processed;
//						cJSON_Delete(jroot);
//					}
//				}
//			}
			strcpy(st, "sleepNow");
			if (strstr(msg.smallTopic, st)) // message identified, we know how to parse it
			{
				ESP_LOGI(TAG, "MsgProcessor::entering deep sleep!");
				status = processed;
				sendStatus(msg.source, msg.smallTopic);
				enableWakePin(TAG);
				esp_deep_sleep_start(); // @suppress("Invalid arguments")
			}
			strcpy(st, "sleepOnStill");
			if (strstr(msg.smallTopic, st)) // message identified, we know how to parse it
			{
				jroot = cJSON_Parse(msg.payload);
				if (jroot == NULL){
					ESP_LOGW(TAG, "MsgProcessor::execute: %s parsing failed!", st);
				}
				else{
					item = cJSON_GetObjectItem(jroot, "timeout");
					if (item){
						int val = item->valueint * 60 * 1000;
						setSleepOnStill(val);
						ESP_LOGI(TAG, "MsgProcessor::execute: sleepOnStill: timeout %i ms", val);
					}else
						ESP_LOGW(TAG, "MsgProcessor::execute: %s arg. parsing failed!", st);
					sendStatus(msg.source, msg.smallTopic);
					status = processed;
					cJSON_Delete(jroot);
				}
			}
			strcpy(st, "statusReq");
			if (strstr(msg.smallTopic, st)) // message identified, we know how to parse it
			{
				ESP_LOGI(TAG, "MsgProcessor::execute: Got a %s from %s. Replying.", st, msg.source);
				sendStatus(msg.source, msg.smallTopic);
				status = processed;
			}
			strcpy(st, "otaUpgrade");
			if (strstr(msg.smallTopic, st)) // message identified, we know how to parse it
			{
				jroot = cJSON_Parse(msg.payload);
				if (jroot){
					item = cJSON_GetObjectItem(jroot, "url");
					if (item){
						char url[OTA_URL_SIZE]= "";
						strcpy(url, item->valuestring);
						//"https://192.168.000.107:8070/SmartSensorESP.bin",
						if (strlen(url)<5){
							strcpy(url, "https://");
							strcat(url, mqttServ);
							strcat(url, ":");
							strcat(url, OTA_PORT);
							strcat(url, "/");
							strcat(url, OTA_FILENAME_DEF);
						}
						ESP_LOGI(TAG, "MsgProcessor::execute: : %s from %s...", st, url);
						otaUpgrade(url);
						sendStatus(msg.source, msg.smallTopic);
						status = processed;
					}
					cJSON_Delete(jroot);
				}
//				char url[OTA_URL_SIZE]= "";
//				m_jsonError = deserializeJson(*m_jsonRoot, msg.payload);
//				if (!m_jsonError && m_jsonRoot->containsKey("url"))
//					strcpy(url, (*m_jsonRoot)["url"].as<const char*>());
//				//"https://192.168.000.107:8070/SmartSensorESP.bin",
//				if (strlen(url)<5){
//					strcpy(url, "https://");
//					strcat(url, mqttServ);
//					strcat(url, ":");
//					strcat(url, OTA_PORT);
//					strcat(url, "/");
//					strcat(url, OTA_FILENAME_DEF);
//				}
//
//				ESP_LOGI(TAG, "MsgProcessor::execute: : otaUpgrade from %s...", url);
//				otaUpgrade(url);
//
//				sendStatus(msg.source, msg.smallTopic);
//				status = processed;
			}
			strcpy(st, "readLog");
			if (strstr(msg.smallTopic, st)) // message identified, we know how to parse it
			{
				// a nice lambda
				//[](PubSubClient &obj, const char* a, const char* b){obj.publish(a,b);}(mqttClient,"aa","bb");
				if (fileLoger.readAll(TOPIC_MAX_SIZE, PAYLOAD_MAX_SIZE, &publishLogWrapper) > 0)
					ESP_LOGI(TAG, "some errors occured while sending logs");
				sendStatus(msg.source, msg.smallTopic);
				status = processed;
			}
			strcpy(st, "setTime");
			if (strstr(msg.smallTopic, st)) // message identified, we know how to parse it
			{
				char* ptr = strchr(msg.payload,':');
				uint64_t i = 0;
				if (ptr != NULL){
					i = strtoll(ptr+1,NULL,10);
					ESP_LOGI(TAG, "msgProcessor:  strtoll parsed payload llu %llu", i);
					timeEsp.set(i);
					char datetimestr[40];
					timeEsp.gets(datetimestr);
					ESP_LOGI(TAG, "msgProcessor: setTime to: %s, %llu, recval: %llu", datetimestr, timeEsp.getMs(),i);
					sendStatus(msg.source, msg.smallTopic);
					status = processed;
				}

//				m_jsonError = deserializeJson(*m_jsonRoot, msg.payload);
//				if (m_jsonError) {
//					ESP_LOGI(TAG, "MsgProcessor::execute: setTime parsing failed!!!\n");
//				} else {
//					if (m_jsonRoot->containsKey("datetime")){
//						uint64_t val = (*m_jsonRoot)["datetime"].as<uint64_t>();
//						ESP_LOGI(TAG, "msgProcessor: jsondoc parsed llu %llu", val);
//						//looks like this shit wrongly converts to 64bit!!!
//						/* this makes a crash...
//						char str[70];
//						strcpy(str, (*m_jsonRoot)["datetime"].as<const char*>());
//						ESP_LOGI(TAG, "msgProcessor: received as char %s", str);
//						 */
//						timeEsp.set(i);
//						char datetimestr[40];
//						timeEsp.gets(datetimestr);
//						ESP_LOGI(TAG, "msgProcessor: setTime to: %s, %llu, recval: %llu", datetimestr, timeEsp.getMs(),val);
//					}
//					sendStatus(msg.source, msg.smallTopic);
//					status = processed;
//				}
			}
		}
		else
			status = nottome;
	}
	switch (status){
	case processed:
		if (strcmp(hubID, msg.source)!=0){
			strcpy(hubID, msg.source); //the last correctly decoded hub becomes the speaking partner
			ESP_LOGI(TAG, "MsgProcessor::execute: New hub %s?!?!?!\n",hubID);
		}
		ESP_LOGI(TAG, "MsgProcessor::execute: ... message processed.");
		break;
	case failed:
		ESP_LOGW(TAG, "MsgProcessor::execute: ... message processing somehow failed.");
		break;
	case nottome:
		ESP_LOGI(TAG, "MsgProcessor::execute: ... message was not to me.");
		break;
	default:
		break;
	}


	return status;
}

