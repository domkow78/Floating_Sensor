/*
 * phaseMeas.h
 *
 *  Created on: 2 lis 2020
 *      Author: phaseMEAS_H_
       */
#ifndef MAIN_PHASEMEAS_H_
#define MAIN_PHASEMEAS_H_

#include <driver/adc.h>
#include "esp_adc_cal.h"
#include "acmm.h"

#define PH_MEAS_PIN		ADC1_CHANNEL_6 //GPIO_NUM_34

#define PH_MEAS_ATTEN	ADC_ATTEN_DB_0
#define PH_MEAS_BITS	ADC_WIDTH_BIT_10

#define PH_MEAS_OVERSAMPLE_BITS	0
#define PH_MEAS_OVERSAMPLES		(1 << PH_MEAS_OVERSAMPLE_BITS)

#define PH_RES_DIVIDER	1.0
#define PH_TAU_MS	300

class Phase_meas_t{
private:
	acmmOpMode_t acmmOpModeTemp;
	bool isTimerRunning = false;
	bool enabled = true;
	bool hasRun = false;
	bool isInit = false;
	TaskHandle_t taskid;
	bool isPinAsLedc = false;
	bool isPinAsGPIO = false;
	void setPinAsLedc(void);
	void setPinAsGpio(void);
public:
	uint32_t freqChangeTs;
	uint16_t freq = 0;	//output value
	uint16_t freqLo = 0;
	uint16_t freqHi = 0;
	esp_adc_cal_characteristics_t *adc_chars;
	uint16_t value;	//output value
	Phase_meas_t();
	void enable(bool yes);
    bool init(void);
    uint8_t run(uint16_t aFreqLo, uint16_t aFreqHi);
    void stop(void);
    void pause(void);
    void resume(void);
    uint8_t test(uint16_t freq);
    bool isFilterReady(uint8_t percent);
	uint16_t measMv(void);
	static esp_err_t setPinHL(gpio_num_t pin, uint32_t value);
	void connectTransAmp(bool val);
};

extern Phase_meas_t phaseMeas;


#endif /* MAIN_PHASEMEAS_H_ */
