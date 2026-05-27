/*
 * comb.h
 *
 *  Created on: 3 sie 2022
 *      Author: JablonskiP
 */

#ifndef MAIN_COMB_H_
#define MAIN_COMB_H_

#include "freertos/FreeRTOS.h"
#include <deque>
#include "freertos/semphr.h"

class comb_t{
private:
	SemaphoreHandle_t xSemaphore;
	float sum;
	float max;
	uint32_t cnt;
	float yn1;
	std::deque<float> deq;
public:
	comb_t();
	float yn;
	float a;	//comb filter coefficient
	uint8_t k;	//comb filter length
	float tick(float xn);
	float readAvgAndReset(void);
	float readMaxAndReset(void);
};



#endif /* MAIN_COMB_H_ */
