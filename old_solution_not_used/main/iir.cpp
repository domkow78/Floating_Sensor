/*
 * iir.cpp
 *
 *  Created on: 3 sie 2022
 *      Author: JablonskiP
 */

#include "iir.h"
//#include "esp_dsp.h"
#include "esp_log.h"


static const char* TAG = "IIR";


iir_t::iir_t(): avgAbs(0), maxAbs(0), cnt(0), wn0(0), wn1(0), wn2(0), a1(0), a2(0), b0(0), b1(0), b2(0), yn(0){
	xSemaphore = xSemaphoreCreateMutex();

	/*The priority of a task that 'takes' a mutex will be temporarily raised if another task of higher priority attempts to obtain the same mutex. The task that owns the mutex 'inherits' the priority of the task attempting to 'take' the same mutex. This means the mutex must always be 'given' back - otherwise the higher priority task will never be able to obtain the mutex, and the lower priority task will never 'disinherit' the priority.*/
	/*https://www.freertos.org/CreateMutex.html*/
	/* A simple semaphore will block completely, I checked!*/
}

//		dsps_biquad_f32_ae32(NULL, NULL, 3, NULL, NULL);
//		dsps_fir_f32_ansi(fir, input, output, len)

/* bi-quad direct form 2 IIR implementation, see https://en.wikipedia.org/wiki/Digital_biquad_filter */
/* compatible with std. digital filter equation, where numerator coefficients b, denominator coefficients a, assume a0==1 */
/* "High-order IIR filters can be highly sensitive to quantization of their coefficients, and can easily become unstable" */
/* https://www.mathworks.com/help/dsp/ref/dsp.biquadfilter-system-object.html */
/* https://www.mathworks.com/help/signal/ref/zp2sos.html?s_tid=doc_ta */
float iir_t::tick(float xn){
	wn0 = xn - a1*wn1 - a2*wn2;
	yn = b0*wn0 + b1*wn1 + b2*wn2;
	wn2 = wn1;
	wn1 = wn0;
	/* this function can be used by some other task than readAvgAndReset */
	if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE ){
		avgAbs = avgAbs + abs(yn);
		if (abs(yn)>maxAbs)
			maxAbs = abs(yn);
		cnt++;
	}else{
		ESP_LOGW(TAG, "Could not get semaphore");
		return 0;
	}
	xSemaphoreGive( xSemaphore );
	return yn;
}

void iir_t::reset(void){
	wn0 = 0;
	wn1 = 0;
	wn2 = 0;
	yn = 0;
}

float iir_t::readAvgAbsAndReset(void){
	float ret;
	if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE ){
		ret = avgAbs / cnt;
		avgAbs = 0;
		cnt = 0;
	}else{
		ESP_LOGW(TAG, "Could not get semaphore");
		return 0;
	}
	xSemaphoreGive( xSemaphore );
	return ret;
}

float iir_t::readMaxAbsAndReset(void){
	float ret;
	if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE ){
		ret = maxAbs;
		maxAbs = 0;
	}else{
		ESP_LOGW(TAG, "Could not get semaphore");
		return 0;
	}
	xSemaphoreGive( xSemaphore );
	return ret;
}
