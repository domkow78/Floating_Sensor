/*
 * iir.h
 *
 *  Created on: 3 sie 2022
 *      Author: JablonskiP
 */

#ifndef MAIN_IIR_H_
#define MAIN_IIR_H_

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


class iir_t{
private:
	float avgAbs;
	float maxAbs;
	uint32_t cnt;
	SemaphoreHandle_t xSemaphore;
public:
	iir_t();
	float wn0, wn1, wn2;	//wn delayed by 0,1,2
	float a1, a2;			//denominator coeffs., assumed a0=1
	float b0, b1, b2;		//numerator coeffs.
	float yn;
	float readAvgAbsAndReset(void);
	float readMaxAbsAndReset(void);
	void reset(void);
	float tick(float xn);
};

#endif /* MAIN_IIR_H_ */
