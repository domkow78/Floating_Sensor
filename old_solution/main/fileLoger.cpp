/*
 * FileLoger.cpp
 *
 *  Created on: 17 mar 2021
 *      Author: JablonskiP
 */
#include "fileLoger.h"
#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"
#include "stdio.h"
#include "string.h"
#include "esp_log.h"
#include "mqttMsg.h"
#include "main.h"
#include "msgSender.h"

#define FILELOGER_FILES_N	3

static const char* TAG = "FILELOG";
//static portMUX_TYPE mux;



FileLoger_t fileLoger;

FileLoger_t::FileLoger_t() {
//	spinlock_initialize(&lock);
//	vPortCPUInitializeMutex(&mux);
	xSemaphore = xSemaphoreCreateMutex();
	xSemaphoreGive(xSemaphore);
	isReady = false;
}


uint8_t FileLoger_t::init() {
	bool gapFound = false;
	bool fileExists;
	char tempName[LOGFILENAME_LEN];
	FILE* fr;
	FILE* fw;
	isReady = false;
	if (!initSpiffs()){
		return 1;
	}
	uint8_t iFile = 0;
	for (iFile = 0; iFile < FILELOGER_FILES_N; iFile++){
		snprintf(tempName, LOGFILENAME_LEN, "/spiffs/%c.log", iFile+48);
		if (xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(100)) != pdTRUE){
			ESP_LOGE(TAG, "fileLoger::init: Could not get semaphore for reading %s", tempName);
			return 2;
		}
		fr = fopen(tempName, "r");
		fileExists = (fr != NULL);
		fclose(fr);
		if (fileExists)
			ESP_LOGI(TAG, "fileLoger::init: found %s", tempName);
		xSemaphoreGive(xSemaphore);
		if (!fileExists) {	//does not exist, so create it and use, delete the next
			ESP_LOGI(TAG, "fileLoger::init: filegap %s found", tempName);
			gapFound = true;
			break;
		}
	}
	if (!gapFound){
		ESP_LOGI(TAG, "FileLoger::couldn't fine any gap, default to 0.");
		iFile = 0;
		remove("/spiffs/0.log");
	}

	//delete the next one
	if (iFile < FILELOGER_FILES_N - 1){
		snprintf(tempName, LOGFILENAME_LEN, "/spiffs/%c.log", 48+iFile+1);
		remove(tempName);
		ESP_LOGI(TAG, "fileLoger::init: file %s deleted", tempName);
	}else{
		remove("/spiffs/0.log");
		ESP_LOGI(TAG, "fileLoger::init: file /spiffs/0.log deleted");
	}

	snprintf(fname, LOGFILENAME_LEN, "/spiffs/%c.log", iFile+48);
	if (xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(100)) != pdTRUE){
		ESP_LOGE(TAG, "fileLoger::init: Could not get semaphore for writing %s", fname);
		return 3;
	}
	fw = fopen(fname, "w");
	if (fw == NULL){
		ESP_LOGE(TAG, "Failed to create a log file %s", fname);
		xSemaphoreGive(xSemaphore);
		return 4;
	}
	fprintf(fw, "SmartSensor Error/Warning %s file.\n", fname);
	fclose(fw);
	xSemaphoreGive(xSemaphore);
	isReady = true;
	ESP_LOGI(TAG, "FileLoger::init done, will use %s", fname);
	return 0;
}

bool FileLoger_t::initSpiffs() {
	if (esp_spiffs_mounted(NULL)){
		//ESP_LOGI(TAG, "SPIFFS already mounted");
		return true;
	}

	ESP_LOGI(TAG, "initSpiffs: Initializing SPIFFS");
	esp_vfs_spiffs_conf_t conf = {
	  .base_path = "/spiffs",
	  .partition_label = NULL,
	  .max_files = FILELOGER_FILES_N,
	  .format_if_mount_failed = true
	};
	//portENTER_CRITICAL(&mux);
	esp_err_t ret = esp_vfs_spiffs_register(&conf);
	//portEXIT_CRITICAL(&mux);
	switch (ret){
		case ESP_OK:
			break;
		case ESP_FAIL:
			ESP_LOGE(TAG, "initSpiffs: Failed to mount or format filesystem");
			return false;
		case ESP_ERR_NOT_FOUND:
			ESP_LOGE(TAG, "initSpiffs: Failed to find SPIFFS partition");
			return false;
		default:
			ESP_LOGE(TAG, "initSpiffs: Failed to initialize SPIFFS");//esp_err_to_name(ret)
			return false;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "initSpiffs: Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
		return false;
	} else {
		ESP_LOGI(TAG, "initSpiffs: Partition size: total: %d, used: %d", total, used);
	}
	return true;
}


/**/
//consider using taskENTER_CRITICAL() or  vTaskSuspendAll ();
bool FileLoger_t::write(const char *text) {
	size_t total = 0, used = 0;
//	ESP_LOGI(TAG, "fileLoger::write a");
	if (!isReady)
		return false;
//	ESP_LOGI(TAG, "fileLoger::write b");
	if (esp_spiffs_info(NULL, &total, &used) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information");//esp_err_to_name(ret)
		return false;
	}
//	ESP_LOGI(TAG, "fileLoger::write c");

	if ((total - used) < strlen(text) + 2) // not enough space, need to re-init
		if (!init())
			return false;
//	ESP_LOGI(TAG, "fileLoger::write d");

	FILE* fw;
	if (xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(3000)) != pdTRUE){
		ESP_LOGE(TAG, "fileLoger::write: Could not get semaphore for writing %s", fname);
		return false;
	}
	fw = fopen(fname, "a");
	if (fw == NULL){
		xSemaphoreGive(xSemaphore);
		return false;
	}
//	ESP_LOGI(TAG, "fileLoger::write e");
	int printed;
	printed = fprintf(fw, text);
	fclose(fw);
	xSemaphoreGive(xSemaphore);
//	ESP_LOGI(TAG, "fileLoger::write f printed %d", printed);
	return (printed > 0);
}

/*
 * returns a number of failures reported by *f
 * *f should return false on publishing failure
 * */
int FileLoger_t::readAll(size_t maxTopicSize, size_t maxPayloadSize, bool (*f)(const char* aPayload)){
	int ret = 0;
	char tempName[LOGFILENAME_LEN];
	char* payload = (char*)malloc(maxPayloadSize);
//	char* topic = (char*)malloc(maxTopicSize);
	int ch;
	long int pos;
	bool iseof = false;
	FILE *fr;
//	sprintf(topic, "ws/%s/%s/%s", hubID, myID, TOPIC_LOG);
	uint8_t currentFileIdx = fname[8]-'0';
	uint8_t fileNr;
	//for (uint8_t fileNr = 0; fileNr < FILELOGER_FILES_N; fileNr++){
	for (uint8_t i = FILELOGER_FILES_N; i>0 ; i--){
		fileNr = (i + currentFileIdx) % FILELOGER_FILES_N;
		sprintf(tempName, "/spiffs/%d.log", fileNr);
//		sprintf(payload, "\n--%c.log--\n", fileNr+48);
//		f(topic, payload) || ret++;
		pos = 0;
		do {
			if (xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(3000)) != pdTRUE){
				ESP_LOGE(TAG, "fileLoger::readAll: Could not get semaphore for reading %s", tempName);
				return 1;
			}
			fr = fopen(tempName, "r");
			if (fr != NULL){
				fseek(fr, pos, SEEK_SET);
				memset(payload, 0, maxPayloadSize);
				for (uint8_t offs = 0; offs < maxPayloadSize-1; offs++)
				{
					ch = getc(fr);
					if (ch == EOF)
						break;
					payload[offs] = char(ch);
				}
				pos = ftell(fr);
				iseof = feof(fr);
			}
			fclose(fr);
			xSemaphoreGive(xSemaphore);
			ESP_LOGI(TAG, "fileLoger::readAll: file %s pos %ld", tempName, pos);
			//mqttClient.publish(topic, payload);
			if (f(payload)){
				ESP_LOGI(TAG, "fileLoger::readAll: published %d bytes", strlen(payload));
			}else{
				ret++;
				ESP_LOGI(TAG, "fileLoger::readAll: failed to publish %d bytes", strlen(payload));
			}
		} while (!iseof);
	}
//	free(topic);
	free(payload);
	ESP_LOGI(TAG, "fileLoger::readAll: done with %d failures.", ret);
	return ret;
}
