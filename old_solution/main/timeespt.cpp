/*
 * timeespt.cpp
 *
 *  Created on: 20 lis 2020
 *      Author: JablonskiP
 */

#include "timeespt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "timeespt.h"

static const char* TAG = "TIMEESP";
#include "esp_log.h"

time_esp_t timeEsp;

time_esp_t::time_esp_t() {
	// TODO Auto-generated constructor stub
	tickStamp =  xTaskGetTickCount();
	t = 0;
}

void time_esp_t::set(uint64_t utcMs) {
	tickStamp = xTaskGetTickCount();
	t = utcMs;
}

void time_esp_t::set(struct tm* valp) {
	tickStamp = xTaskGetTickCount();
	t = mktime(valp) * 1000;	// in ms now
}

void time_esp_t::set(const char* str){
	const char *cp;
	struct tm aTm;
	cp = strptime (str, "%c", &aTm);
	if (cp == NULL)
	{
		ESP_LOGE(TAG, "Wrong time format.");
		return;
	}
	tickStamp = xTaskGetTickCount();
	t = mktime(&aTm) * 1000;
}

void time_esp_t::get(struct tm* valp) {
	time_t t1 = (time_t)(getMs() / 1000);
	struct tm* pt2;
	pt2 = gmtime(&t1);
	memcpy(valp, pt2, sizeof(struct tm));
}

uint64_t time_esp_t::getMs() {
	return t + (1000 * (xTaskGetTickCount() - tickStamp))/configTICK_RATE_HZ;
}

void time_esp_t::gets(char* out){
	time_t s = (time_t)(getMs()/1000);
	struct tm * timeinfo;
	timeinfo = localtime (&s);
	strftime (out, 80, "%Y-%m-%d %H:%M:%S", timeinfo);
	return;
}

time_esp_t::~time_esp_t() {
	// TODO Auto-generated destructor stub
}

