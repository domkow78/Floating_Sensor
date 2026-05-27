/*
 * comb.cpp
 *
 *  Created on: 3 sie 2022
 *      Author: JablonskiP
 */

#include "comb.h"
#include "esp_log.h"


static const char* TAG = "COMB";

#define COMB_K	2

comb_t::comb_t(): sum(0), max(0), cnt(0), yn1(0), yn(0), a(0), k(COMB_K){
	xSemaphore = xSemaphoreCreateMutex();
	if (xSemaphore == NULL)
		ESP_LOGE(TAG, "comb_t(): Could not create semaphore");
}

/*  */
float comb_t::tick(float xn){
	if (a != 0){
		if (deq.size() >= k){
			yn1 = deq[0];
			yn = a * yn1 + xn;
			deq.pop_front();
		}
		deq.push_back(yn);
	}
	if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE ){
		sum = sum + yn;
		if (yn>max)
			max = yn;
		cnt++;
	}else{
		ESP_LOGW(TAG, "Could not get semaphore");
		return 0;
	}
	xSemaphoreGive( xSemaphore );
	return yn;
}

float comb_t::readAvgAndReset(void){
	float ret;
	if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE ){
		ret = sum / cnt;
		sum = 0;
		cnt = 0;
	}else{
		ESP_LOGW(TAG, "Could not get semaphore");
		return 0;
	}
	xSemaphoreGive( xSemaphore );
	return ret;
}

float comb_t::readMaxAndReset(void){
	float ret;
	if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE ){
		ret = max;
		max = 0;
	}else{
		ESP_LOGW(TAG, "Could not get semaphore");
		return 0;
	}
	xSemaphoreGive( xSemaphore );
	return ret;
}
