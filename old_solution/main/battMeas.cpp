#include "battMeas.h"
#include <driver/adc.h>
#include "esp_adc_cal.h"
#include "acmm.h"
static const char* TAG = "BATT";

/*The function esp_log_level_set() cannot set logging levels higher than specified by CONFIG_LOG_DEFAULT_LEVEL.
 * To increase log level for a specific file at compile time, use the macro LOG_LOCAL_LEVEL before include esp_log.h
 */
//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"


#define DEFAULT_VREF    1100


Batt_meas_t battMeas;


Batt_meas_t::Batt_meas_t() {
	adc_chars = NULL;
}

bool Batt_meas_t::init(bool force) {
	if (ESP_OK != adc1_config_width(BATT_MEAS_BITS)){
		ESP_LOGW(TAG, "BattMeas::init: failed adc1_config_width(ACMM_BITS)");
		return false;
	}
	if (ESP_OK != adc1_config_channel_atten(BATT_MEAS_PIN, BATT_MEAS_ATTEN)){
		ESP_LOGW(TAG, "BattMeas::init: failed adc1_config_channel_atten(BATT_MEAS_PIN, BATT_MEAS_ATTEN)");
		return false;
	}

	if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
		ESP_LOGI(TAG, "eFuse Two Point ADC calibration: Supported");
	} else
	{
		ESP_LOGI(TAG, "eFuse Two Point ADC calibration: NOT Supported, will try single point...");
		if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
			ESP_LOGI(TAG, "eFuse Vref ADC calibration: Supported");
		}else
		{
			ESP_LOGW(TAG, "eFuse Vref ADC calibration: NOT Supported, will use default VREF!");
		}
	}

	adc_chars = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(
    		ADC_UNIT_1, BATT_MEAS_ATTEN, BATT_MEAS_BITS, DEFAULT_VREF, adc_chars);

	ESP_LOGI(TAG, "BattMeas::init: ok");
	return true;
}

uint16_t Batt_meas_t::measMv(void){
	uint32_t raw = 0;
//	portENTER_CRITICAL(&acmm.adc1Mux);
	for (uint8_t i = 0; i < BATT_MEAS_OVERSAMPLES; i++){
		raw += esp_adc_cal_raw_to_voltage(adc1_get_raw(BATT_MEAS_PIN), adc_chars);
	}
//	portEXIT_CRITICAL(&acmm.adc1Mux);
	raw = raw >> BATT_MEAS_OVERSAMPLE_BITS;
	raw = (uint16_t)(BATT_RES_DIVIDER * raw);
	return (uint16_t)raw;
}


battState_t Batt_meas_t::meas(void) {
	raw = measMv();
	if (raw >= BATT_FULL_MV)
		return battState_t::battFull;
	if (raw >= BATT_HALF_MV)
			return battState_t::battHalf;
	if (raw >= BATT_LOW_MV)
			return battState_t::battLow;
	return battState_t::battDead;
}
