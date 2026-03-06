/*
 * tp.h
 *
 *  Created on: 17 cze 2021
 *      Author: JablonskiP
 */

#ifndef MAIN_TP_H_
#define MAIN_TP_H_

//#include "freeRTOS.h"
#include "freertos/FreeRTOS.h"
#include "iir.h"
//#include "comb.h"

class tp_t{
private:
	bool isTpInit = false;
	bool enabled = true;
	bool started = false;
	TaskHandle_t taskid;
public:
	TaskHandle_t taskFft;
//	comb_t comb;
	iir_t iir1;
	iir_t iir2;
	bool useFft = false;
	uint16_t* ptrVal = NULL;
	uint16_t minOver;
	float* pFft;
	bool unfinished = false;
	tp_t();
    bool init(uint16_t* aPtrVal, bool aUseFtt=false);
    bool start(int16_t reset = -1);
    void enable(bool yes);
    void stop();
    void resume();
    void suspend();
    bool measure(uint16_t* aPtrVal, uint16_t* aPtrValRaw);
};

extern tp_t tp;




#endif /* MAIN_TP_H_ */
