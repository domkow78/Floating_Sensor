/*
 * battMeas.h
 *
 *  Created on: 2 lis 2020
 *      Author: JablonskiP
 */

#ifndef MAIN_BATTMEAS_H_
#define MAIN_BATTMEAS_H_

#include <driver/adc.h>
#include "esp_adc_cal.h"


//#include "esp_adc_cal.h"	//this comes from arduino

//#define BATT_MEAS_PIN	ADC1_GPIO36_CHANNEL
#define BATT_MEAS_PIN	ADC1_CHANNEL_0
#define BATT_MEAS_ATTEN	ADC_ATTEN_DB_0
#define BATT_MEAS_BITS	ADC_WIDTH_BIT_10

#define BATT_MEAS_OVERSAMPLE_BITS	3
#define BATT_MEAS_OVERSAMPLES		(1 << BATT_MEAS_OVERSAMPLE_BITS)

#define BATT_RES_DIVIDER	4.242

#define BATT_FULL_MV	4200
#define BATT_HALF_MV	3700
#define BATT_LOW_MV		3600
#define BATT_DEAD_MV	3500



enum class battState_t:uint16_t {
	battFull = BATT_FULL_MV,
	battHalf = BATT_HALF_MV,
	battLow = BATT_LOW_MV,
	battDead = BATT_DEAD_MV};

class Batt_meas_t{
private:
public:
	esp_adc_cal_characteristics_t *adc_chars;
	uint16_t raw;
	Batt_meas_t();
    bool init(bool force = false);
	uint16_t measMv(void);
	battState_t meas(void);
};

extern Batt_meas_t battMeas;


#endif /* MAIN_BATTMEAS_H_ */
