#include "phaseMeas.h"
#include "freertos/FreeRTOS.h"
#include <driver/adc.h>
#include "esp_adc_cal.h"
#include "acmm.h"
#include "driver/ledc.h"
#include "soc/io_mux_reg.h"
#include "hal/gpio_hal.h"
#include "esp_rom_gpio.h"
//#include "hal/ledc_hal.h"
#include <math.h>
#include "main.h"
#include "soc/ledc_periph.h"

static const char* TAG = "PHASE";

/*The function esp_log_level_set() cannot set logging levels higher than specified by CONFIG_LOG_DEFAULT_LEVEL.
 * To increase log level for a specific file at compile time, use the macro LOG_LOCAL_LEVEL before include esp_log.h
 */
//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#define DEFAULT_VREF    1100

// drive it via EL1, then signal comes  through laundry, then back to EL2
#define PH_DRIVE_PIN          ACMM_H_PIN_1
//#define PH_DRIVE_PIN			GPIO_NUM_14
#define PH_GPIO_DEF_FUNC		2
#define PH_TRANS_AMP_EN_PIN     GPIO_NUM_32
//https://www.upesy.com/blogs/tutorials/esp32-pinout-reference-gpio-pins-ultimate-guide


#define PH_TIMER              	LEDC_TIMER_0
#define PH_MODE               	LEDC_LOW_SPEED_MODE
#define PH_CHANNEL            	LEDC_CHANNEL_0
#define PH_DUTY_RES           	LEDC_TIMER_2_BIT
#define PH_DUTY_START           70
#define PH_PERCENT2DUTY(x)		(((1 << PH_DUTY_RES) - 1) * x/100)
#define PH_FREQUENCY          	4000
#define PH_HPOINT	          	0
//#define PH_CLK_SRC				LEDC_AUTO_CLK
#define PH_CLK_SRC				LEDC_USE_REF_TICK
//#define PH_CLK_SRC				LEDC_USE_RTC8M_CLK 	//unstable


//LEDC_USE_RTC8M_CLK

#define TASK_PHASE_HEAP	2048+512
#define TASK_HEAP_MIN_FREE 50

#define PH_FILTER_READY_DELAY(percent)	(- PH_TAU_MS * log(1-float(percent)/100))
#define PH_FILTER_READY_PERCENT 		95
#define PH_SAMPLE_PERIOD_MS				(int)(PH_FILTER_READY_DELAY(PH_FILTER_READY_PERCENT))
#define PH_FREQ_STEPS	10
#define PH_AO_BITS		3
#define PH_AO			(1<<PH_AO_BITS)

Phase_meas_t phaseMeas;


Phase_meas_t::Phase_meas_t() {
	adc_chars = NULL;
	freqChangeTs = 0;
	acmmOpModeTemp = acmmOpMode_t::stop;
	value = 0;
	taskid = NULL;
	isInit = false;
}

static void taskPhaseCbk(void* ptr){
	TickType_t ts;
	uint16_t aFreq = phaseMeas.freq;
	uint8_t iCycle = 0;
	uint32_t temp = 0;
	for (;;){
		ts = xTaskGetTickCount();

//		while (!phaseMeas.isFilterReady(PH_FILTER_READY_PERCENT))
//			vTaskDelay(100);

		temp += phaseMeas.measMv();
		iCycle++;
		if (iCycle >= (PH_AO-1)){
			phaseMeas.freq = aFreq;		//so that phaseMeas.freq is a freq at which sample was taken
			phaseMeas.value = temp >> PH_AO_BITS;
			temp = 0;
			iCycle = 0;
			ESP_LOGI(TAG, "taskPhaseCbk: value %d at %d Hz", phaseMeas.value, phaseMeas.freq);
			aFreq = aFreq + (phaseMeas.freqHi - phaseMeas.freqLo)/PH_FREQ_STEPS;
			(aFreq <= phaseMeas.freqHi) || (aFreq = phaseMeas.freqLo);
			ESP_ERROR_CHECK(ledc_set_freq(PH_MODE, PH_TIMER, aFreq));
			if (phaseMeas.freqHi != phaseMeas.freqLo)
				phaseMeas.freqChangeTs = pdTICKS_TO_MS(xTaskGetTickCount());
		}

		UBaseType_t minFreeHeap = uxTaskGetStackHighWaterMark( NULL );
		if ( minFreeHeap < TASK_HEAP_MIN_FREE)
			ESP_LOGW(TAG, " HEAP is short! Only %d left.", minFreeHeap);
		vTaskDelay(pdMS_TO_TICKS(PH_SAMPLE_PERIOD_MS) - (xTaskGetTickCount()-ts));
	}
}

bool Phase_meas_t::init() {
	if (ESP_OK != adc1_config_width(PH_MEAS_BITS)){
		ESP_LOGW(TAG, "PhaseMeas::init: failed adc1_config_width(PH_BITS)");
		return false;
	}
	if (ESP_OK != adc1_config_channel_atten(PH_MEAS_PIN, PH_MEAS_ATTEN)){
		ESP_LOGW(TAG, "PhaseMeas::init: failed adc1_config_channel_atten(PH_MEAS_PIN, PH_MEAS_ATTEN)");
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
    		ADC_UNIT_1, PH_MEAS_ATTEN, PH_MEAS_BITS, DEFAULT_VREF, adc_chars);

	ledc_timer_config_t ledc_timer = {
	   .speed_mode       = PH_MODE,
	   {.duty_resolution  = PH_DUTY_RES},
	   .timer_num        = PH_TIMER,
	   .freq_hz          = PH_FREQUENCY,  // Set output frequency at 5 kHz
	   .clk_cfg          = PH_CLK_SRC
	};
	ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

	// Prepare and then apply the LEDC PWM channel configuration
	ledc_channel_config_t ledc_channel = {
	   .gpio_num       = PH_DRIVE_PIN,
	   .speed_mode     = PH_MODE,
	   .channel        = PH_CHANNEL,
	   .intr_type      = LEDC_INTR_DISABLE,
	   .timer_sel      = PH_TIMER,
	   .duty           = PH_PERCENT2DUTY(PH_DUTY_START),
	   .hpoint         = PH_HPOINT,
	   .flags = {.output_invert = 0}
	};
	ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

	gpio_set_drive_capability(PH_DRIVE_PIN, GPIO_DRIVE_CAP_DEFAULT);
	GPIO.pin[PH_DRIVE_PIN].pad_driver = 1;
	//	gpio_set_direction(PH_DRIVE_PIN, GPIO_MODE_OUTPUT_OD);// takiego wa³a, æwoki
	//GPIO_MODE_OUTPUT_OD to jest suma dwóch bitów, i jeden z nich odpala jakiœ hal-rom-mux tralala
	// i wywala mi LEDC

	ledc_timer_pause(PH_MODE, PH_TIMER);
	isTimerRunning = false;

	acmm.hpinSetHL = &Phase_meas_t::setPinHL;

	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask =  (1ULL << PH_TRANS_AMP_EN_PIN);
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	if (ESP_OK != gpio_config(&io_conf)){
		ESP_LOGW(TAG, "init: PH_TRANS_AMP_EN_PIN config problem ");
		return false;
	}
	gpio_set_level(PH_TRANS_AMP_EN_PIN, 1);

	xTaskCreate(&taskPhaseCbk, "phase_read_task", TASK_PHASE_HEAP, NULL, tskIDLE_PRIORITY, &taskid);
    vTaskSuspend(taskid);

    isInit = true;

    ESP_LOGI(TAG, "init: ok, sample period %dms, AO: %d", PH_SAMPLE_PERIOD_MS, PH_AO);
	return true;
}

uint16_t Phase_meas_t::measMv(void){
	uint32_t raw = 0;
//	portENTER_CRITICAL(&acmm.adc1Mux);
	for (uint8_t i = 0; i < PH_MEAS_OVERSAMPLES; i++){
		raw += esp_adc_cal_raw_to_voltage(adc1_get_raw(PH_MEAS_PIN), adc_chars);
	}
//	portEXIT_CRITICAL(&acmm.adc1Mux);
	raw = raw >> PH_MEAS_OVERSAMPLE_BITS;
	return (uint16_t)(PH_RES_DIVIDER * raw);
}

esp_err_t Phase_meas_t::setPinHL(gpio_num_t pin, uint32_t value){
	if (phaseMeas.isTimerRunning){
		ledc_timer_pause(PH_MODE, PH_TIMER);
		phaseMeas.isTimerRunning = false;
	}

	if (!phaseMeas.isPinAsGPIO)
		phaseMeas.setPinAsGpio();

	return gpio_set_level(pin, value);
}

void Phase_meas_t::enable(bool yes){
	if (yes){

	}else{
		pause();
	}
	enabled = yes;
}


uint8_t Phase_meas_t::run(uint16_t aFreqLo, uint16_t aFreqHi){
	if (!isInit){
		ESP_LOGW(TAG, "::init first!");
		return 0;
	}
	if (!enabled){
			ESP_LOGW(TAG, "::enable first!");
			return 0;
	}
	setPinAsLedc();
	freqLo = aFreqLo;
	freqHi = aFreqHi;
	freq = aFreqLo;

	connectTransAmp(true);
	ESP_ERROR_CHECK(ledc_set_freq(PH_MODE, PH_TIMER, aFreqLo));
	ESP_ERROR_CHECK(ledc_timer_resume(PH_MODE, PH_TIMER));
	isTimerRunning = true;

	freqChangeTs = pdTICKS_TO_MS(xTaskGetTickCount());
	vTaskResume(taskid);
	hasRun = true;
	ESP_LOGI(TAG, "::run at %dHz", aFreqLo);
	freqLo = aFreqLo;
	esp_pm_lock_acquire(pmLock);
	return 0;
}

void Phase_meas_t::setPinAsLedc(void) {
	if (isPinAsLedc)
		return;
	GPIO.pin[PH_DRIVE_PIN].pad_driver = 0;	// PP push-pull
	gpio_iomux_out(PH_DRIVE_PIN, PIN_FUNC_GPIO, false);
	esp_rom_gpio_connect_out_signal(PH_DRIVE_PIN, ledc_periph_signal[PH_MODE].sig_out0_idx + PH_CHANNEL, 0, 0);
	isPinAsGPIO = false;
	isPinAsLedc = true;
}

void Phase_meas_t::setPinAsGpio(void) {
	if (isPinAsGPIO)
		return;
	//	from gpio_hal.h:
	//	func The function number of the peripheral pin to output pin.
	//	  *        One of the ``FUNC_X_*`` of specified pin (X) in ``soc/io_mux_reg.h``.
	gpio_iomux_out(PH_DRIVE_PIN, PIN_FUNC_GPIO, false);
	GPIO.pin[PH_DRIVE_PIN].pad_driver = 1;	//OD open drain
	esp_rom_gpio_connect_out_signal(PH_DRIVE_PIN, SIG_GPIO_OUT_IDX, false, false);
	isPinAsGPIO = true;
	isPinAsLedc = false;
}

bool Phase_meas_t::isFilterReady(uint8_t percent){
	return ((pdTICKS_TO_MS(xTaskGetTickCount()) - freqChangeTs) > - PH_TAU_MS * log(1-float(percent)/100));
}

void Phase_meas_t::stop(void){
	ledc_stop(PH_MODE, PH_CHANNEL, 0);
	setPinAsGpio();
	isTimerRunning = false;
	isInit = false;
	hasRun = false;
	esp_pm_lock_release(pmLock);
	ESP_LOGI(TAG, "PhaseMeas::stopped");
}

void Phase_meas_t::pause(void){
	if (isInit && hasRun){
		ledc_timer_pause(PH_MODE, PH_TIMER);
		isTimerRunning = false;
		setPinAsGpio();
		vTaskSuspend(taskid);
		esp_pm_lock_release(pmLock);
		ESP_LOGI(TAG, "PhaseMeas::paused");
	}
//	else
//		ESP_LOGI(TAG, "PhaseMeas::pause failed, run it first");

}

void Phase_meas_t::resume(void){
	if (enabled){
		if (hasRun){
			esp_pm_lock_acquire(pmLock);
			setPinAsLedc();
			connectTransAmp(true);
			isTimerRunning = true;
			ledc_timer_resume(PH_MODE, PH_TIMER);
			vTaskResume(taskid);
			ESP_LOGI(TAG, "PhaseMeas::resumed");
			return;
		}
		ESP_LOGI(TAG, "PhaseMeas::resume failed, run it first");
	}
}

uint8_t Phase_meas_t::test(uint16_t freq){

	uint32_t temp, temp1;
//	uint64_t ts;
	ESP_LOGI(TAG, "PhaseMeas::set PP directly !!!");
	GPIO.pin[PH_DRIVE_PIN].pad_driver = 0;
	connectTransAmp(true);

	ESP_ERROR_CHECK(ledc_timer_resume(PH_MODE, PH_TIMER));

	for (uint32_t f = 4000; f < 50000; f += 20000){
		ESP_LOGI(TAG, "PhaseMeas::set freq %d", f);
		ledc_set_freq(PH_MODE, PH_TIMER, f);

		for (uint8_t d = 0; d<=100; d = d+10){
			for (uint32_t h = 0; h<=15; h += 5){
				ledc_set_duty_with_hpoint(PH_MODE, PH_CHANNEL, PH_PERCENT2DUTY(d), h);
				ledc_update_duty(PH_MODE, PH_CHANNEL);
				vTaskDelay(pdMS_TO_TICKS(100));
				temp = ledc_get_duty(PH_MODE, PH_CHANNEL);
				temp1 = ledc_get_hpoint(PH_MODE, PH_CHANNEL);
				ESP_LOGI(TAG, "PhaseMeas:: duty set %d get %d, hpoint set %d get %d", d, temp, h, temp1);
				vTaskDelay(pdMS_TO_TICKS(3000));
			}
		}
	}

	ESP_LOGI(TAG, "PhaseMeas::set OD directly !!!");

	GPIO.pin[PH_DRIVE_PIN].pad_driver = 1;
	//	gpio_set_direction(PH_DRIVE_PIN, GPIO_MODE_OUTPUT_OD);// takiego wa³a, æwoki
	//GPIO_MODE_OUTPUT_OD to jest suma dwóch bitów, i jeden z nich odpala jakiœ hal-rom-mux tralala
	// i wywala mi LEDC

//	LEDC.date = 1;

	for (uint32_t f = 4000; f < 50000; f += 20000){
		ESP_LOGI(TAG, "PhaseMeas::set freq %d", f);
		ledc_set_freq(PH_MODE, PH_TIMER, f);

		for (uint8_t d = 0; d<=100; d = d+10){
			for (uint32_t h = 0; h<=15; h += 5){
				ledc_set_duty_with_hpoint(PH_MODE, PH_CHANNEL, PH_PERCENT2DUTY(d), h);
				ledc_update_duty(PH_MODE, PH_CHANNEL);
				vTaskDelay(pdMS_TO_TICKS(100));
				temp = ledc_get_duty(PH_MODE, PH_CHANNEL);
				temp1 = ledc_get_hpoint(PH_MODE, PH_CHANNEL);
				ESP_LOGI(TAG, "PhaseMeas:: duty set %d get %d, hpoint set %d get %d", d, temp, h, temp1);
				vTaskDelay(pdMS_TO_TICKS(3000));
			}
		}
	}


	ESP_LOGI(TAG, "PhaseMeas::timer pause and reset");
//	ESP_ERROR_CHECK(ledc_timer_pause(PH_MODE, PH_TIMER));
//	ESP_ERROR_CHECK(ledc_timer_rst(PH_MODE, PH_TIMER));
	ESP_ERROR_CHECK(ledc_timer_pause(PH_MODE, PH_TIMER));
	vTaskDelay(pdMS_TO_TICKS(10000));

	ESP_LOGI(TAG, "PhaseMeas::set level 1");
	ESP_ERROR_CHECK(gpio_set_level(PH_DRIVE_PIN, 1));
	vTaskDelay(pdMS_TO_TICKS(10000));

	ESP_LOGI(TAG, "PhaseMeas::set level 0");
	ESP_ERROR_CHECK(gpio_set_level(PH_DRIVE_PIN, 0));
	vTaskDelay(pdMS_TO_TICKS(10000));

	ESP_LOGI(TAG, "PhaseMeas::timer resume");
	ESP_ERROR_CHECK(ledc_timer_resume(PH_MODE, PH_TIMER));
	return 0;
}

/* Connects or disconnects transimpedance amplifier with its shottky diode*/
void Phase_meas_t::connectTransAmp(bool val){
	gpio_set_level(PH_TRANS_AMP_EN_PIN, val);	//this is NMOS, enable high, then even negative currents go through
}




