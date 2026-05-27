/*
 * read this before anything
 * https://docs.espressif.com/projects/esp-idf/en/release-v4.0/api-reference/system/power_management.html
 * */

//#include <Arduino.h>
#include "acmm.h"
#include "config.h"
#include "main.h"
#ifdef USE_ARIMA
#include "arima_tools.h"
#endif

#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
//#include "timer_group_struct.h"
#include <driver/adc.h>
#include "battMeas.h"
#include <esp_pm.h>
#include "utils.h"
#include "tp.h"
#include "phaseMeas.h"

#define DEFAULT_VREF    1100
#define NOMINAL_VCC		3300
#define LDO_DROPOUT		190

static const char* TAG = "ACMM";

/*The function esp_log_level_set() cannot set logging levels higher than specified by CONFIG_LOG_DEFAULT_LEVEL.
 * To increase log level for a specific file at compile time, use the macro LOG_LOCAL_LEVEL before include esp_log.h
 */
//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

const char *acmmInterlockStr[3] = {"gnd","hiz","off"};

Acmm_t acmm(&moisture);

#define TIMER_DIVIDER_SH				4
#define TIMER_DIVIDER					(1<<TIMER_DIVIDER_SH)//  Hardware timer clock divider
#define TIMER_MSEC_2_TICK(msec)			((uint64_t)msec * ((TIMER_BASE_CLK >> TIMER_DIVIDER_SH) >> 10))

#define ACMM_TASK_HEAP_SIZE	2800
#define TASK_HEAP_MIN_FREE	100

#ifdef USE_ARIMA
static ARIMAModel arimaMod;
static std::vector<double> obs;
Predictions preds;
#endif


void taskAcmmCbk(void* ptr){
	uint32_t acmmMin, acmmMinPrev = UINT32_MAX;
	for (;;){
		xQueueReceive(acmm.qu, &acmmMin, portMAX_DELAY);

		if (acmmMinPrev != UINT32_MAX)	{	//this is the first run after reset, skip it until you have something real to fill prev
			if (battMeas.raw < (NOMINAL_VCC + LDO_DROPOUT)){
				acmmMin = uint32_t(acmmMin * (float)(NOMINAL_VCC) / (battMeas.raw - LDO_DROPOUT) );
			}

			/*do some lp bessel filtering on acmmMin*/
			(*acmm.pMoisture) = (acmm.a1 * acmmMin + acmm.a2 * acmmMinPrev + acmm.b1 * (*acmm.pMoisture)) / (acmm.a1a2b1);

			ESP_LOGI(TAG, "taskAcmmCbk: Moisture %4.2f, Off: %dms, On: %dms, ilock %s",
					*(acmm.pMoisture), ACMM_SLEEP_MS, ACMM_WORK_MS(acmm.freq, acmm.aggOver), acmmInterlockStr[(uint8_t)acmm.interlockMode]);

			if (acmm.timerLags)
				ESP_LOGW(TAG, "taskAcmmCbk: esp_timer lag! Should: %lld, is %lld", (uint64_t)(1000 * (ACMM_FREQ_2_MS(acmm.freq))), acmm.realTmrPeriodus);


	#ifdef USE_ARIMA
			if (obs.size() < arimaMod.ar_params.size())
				obs.push_back(acmmMin);
			else{
				std::rotate(obs.begin(),obs.begin()+1,obs.end());
				obs.back() = acmmMin;
				preds = ARIMAForecaster::forecast(arimaMod, 1, obs);
				ESP_LOGI(TAG, "ARIMA: %f\n",preds.values[0]);
			}
	#endif
		}
		acmmMinPrev = acmmMin;
		UBaseType_t minFreeHeap = uxTaskGetStackHighWaterMark( NULL );
		if ( minFreeHeap < TASK_HEAP_MIN_FREE)
			ESP_LOGW(TAG, " HEAP is short! Only %d left.", minFreeHeap);
		taskYIELD();
	}
}


/**
 * @brief Callback for esp idf timer
 * @param  timeMsec - timer intr period
 * @return none
 */
static void espIdfTimerCbk(void* arg){

	if (acmm.opMode==acmmOpMode_t::run){
		if (acmm.espTmrTs != 0){
			acmm.realTmrPeriodus = esp_timer_get_time() - acmm.espTmrTs;
			acmm.timerLags = (acmm.realTmrPeriodus > acmm.timerPeriodus + ACMM_TIMER_LAG_ALLOW_US);
		}
		acmm.espTmrTs = esp_timer_get_time();
	}

	if (acmm.switchOnly){
		acmm.switchH();
		return;
	}

	if (acmm.getOpMode() == acmmOpMode_t::sleep){ //we were sleeping, time to run
		acmm.setOpMode(acmmOpMode_t::run);
		// we had (in sleep) both outputs set to HiZ,
		//to prevent any pattern, set outputs to random state
		acmm.setHRandom();
		//and skip any sampling this time, to allow proper capa saturation
		return;
	}

	static uint32_t iterClean=0, iterAgg=0;
	static uint32_t acmmInnerMin=INT16_MAX, acmmInnerMax = 0, acmmClean = 0, acmmMin = INT16_MAX;
	int raw;

	if (acmm.takeSampleOnH1 == (acmm.state==acmmHbridgeState_t::H1pup)){
//		portENTER_CRITICAL(&acmm.adc1Mux);
		esp_pm_lock_acquire(pmLock);
		raw = esp_adc_cal_raw_to_voltage(adc1_get_raw(ACMM_INPIN), acmm.adc_chars);
		esp_pm_lock_release(pmLock);
//		portEXIT_CRITICAL(&acmm.adc1Mux);
		acmm.switchH();
	}else{
		acmm.switchH();
		return;
	}

	acmmClean += raw;
	if (acmmInnerMin>raw){
		acmmInnerMin = raw;
	} else if(acmmInnerMax<raw){
		acmmInnerMax = raw;
	}

	if (++iterClean == ACMM_CLEANOVER){

		acmmClean -= acmmInnerMin;
		acmmClean -= acmmInnerMax;
		acmmClean = acmmClean / (ACMM_CLEANOVER - 2);

		iterClean = 0; acmmInnerMax = 0; acmmInnerMin = INT16_MAX;

		if (iterAgg==0){
			acmmMin = INT16_MAX;
		}
		if (acmmMin > acmmClean){
			acmmMin = acmmClean;
		}
		if (++iterAgg == acmm.aggOver){

//			xQueueSendFromISR(acmm.qu, &acmmMin, NULL);
			/*if esp_timer callback is dispatched from task ESP_TIMER_TASK,
			 not ISR, then this is not needed*/
			xQueueSend(acmm.qu, &acmmMin, 20);
			/* sent new min for smooth calculations, may rest for some time*/

			iterAgg = 0;
			acmm.setOpMode(acmmOpMode_t::sleep);
		}
	}
}

void Acmm_t::setB1(float aValue){
	b1 = aValue;
	a1 = (1-aValue)/2;
	a2 = a1;
	a1a2b1 = 1;
}


/**
 * @brief Initialize idf timer used for ACMM
 * @param  none
 * @return true on success
 */
bool Acmm_t::initTimer(void) {
	const esp_timer_create_args_t etc = {
			.callback = &espIdfTimerCbk,
			.arg = NULL,
			.dispatch_method = ESP_TIMER_TASK,
			.name = NULL,
			.skip_unhandled_events = false	//otherwise, it will not wake from automatic sleep...
	    };
	//read about PM and also esp_timer, very unclear...
	if (ESP_OK != esp_timer_create(&etc, &espIdfTimer)){
		ESP_LOGE(TAG, "Failed to create esp idf timer.");
		return false;
	}
	ESP_LOGI(TAG, "Acmm::initTimer: ok.");
	return true;
}

/**
 * @brief Configure ACMM H-bridge driving and adc input pins
 * @param  none
 * @return none
 */
bool Acmm_t::initH(void){
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	//set as output Open Drain mode
	io_conf.mode = GPIO_MODE_OUTPUT_OD;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask =  ((1ULL<<ACMM_H_PIN_1) | (1ULL<<ACMM_H_PIN_2));
	//disable pull-down mode
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	//disable pull-up mode
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	//configure GPIO with the given settings
	if (ESP_OK != gpio_config(&io_conf)){
		ESP_LOGW(TAG, "Acmm::initH: failed gpio_config on ACMM_H_PIN_1/2.");
		return false;
	}

	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.pin_bit_mask = (1ULL << ACMM_INPIN);
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	if (ESP_OK != gpio_config(&io_conf)){
		ESP_LOGW(TAG, "Acmm::initH: failed gpio_config on ACMM_INPIN.");
		return false;
	}
	setH(acmmHbridgeState_t::Hiz);

	if (selfPullupEn){
		io_conf.intr_type = GPIO_INTR_DISABLE;
		io_conf.pin_bit_mask = ((1ULL << ACMM_VEL1_PIN) | (1ULL << ACMM_VEL2_PIN));
		io_conf.mode = GPIO_MODE_OUTPUT;
		io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
		io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
		if (ESP_OK != gpio_config(&io_conf)){
			ESP_LOGW(TAG, "Acmm::initH: failed gpio_config on ACMM_VEL1/2_PIN.");
			return false;
		}
		gpio_set_level(ACMM_VEL1_PIN, 1);
		gpio_set_level(ACMM_VEL2_PIN, 1);
		ESP_LOGI(TAG, "Acmm::initH: self-pullup active");
	}

	ESP_LOGI(TAG, "Acmm::initH: done.");
	return true;
}

/**
 * @brief Constructs ACMM object, not touching hardware resources. Please initialize hardware with init().
 * @param  apMoisture - a pointer to where moisture data should be stored
 * @return none
 */
Acmm_t::Acmm_t(float* apMoisture){
	switchOnly = false;
	adc_chars = NULL;
	aggOver = ACMM_AGGOVER;
	a1 = ACMM_A1;
	a2 = ACMM_A2;
	b1 = ACMM_B1;
	a1a2b1 = ACMM_A1 + ACMM_A2 + ACMM_B1;
	freq = ACMM_FREQ_HZ;
	assert(apMoisture != nullptr);
	pMoisture = apMoisture;
	opMode = acmmOpMode_t::stop;
	timerPeriodus = (uint64_t)(1000 * (ACMM_FREQ_2_MS(freq)));
	espIdfTimer = NULL;
	hpinSetHL = &gpio_set_level;

	semaphore = xSemaphoreCreateMutex();

	qu = xQueueCreate(16, sizeof(uint32_t));
	tsk = NULL;
	xTaskCreate(&taskAcmmCbk, "taskAcmm", ACMM_TASK_HEAP_SIZE, NULL, tskIDLE_PRIORITY, &(tsk));
	state = acmmHbridgeState_t::Hiz;

#ifdef USE_ARIMA
	arimaMod.ar_params = ACMM_ARIMA_AR;
	arimaMod.d = ACMM_ARIMA_I;
	arimaMod.ma_params = ACMM_ARIMA_MA;
#endif

//	esp_pm_lock_create(
//			esp_pm_lock_type_t(ESP_PM_APB_FREQ_MAX | ESP_PM_CPU_FREQ_MAX | ESP_PM_NO_LIGHT_SLEEP),
//			0, "", &pmLock);
//	pmLocked = false;

	vPortCPUInitializeMutex(&adc1Mux);
}

/**
 * @brief Constructs ACMM completely, ready to run, but still stopped.
 * @param  apMoisture - a pointer to where moisture data should be stored
 * @return none
 */
bool Acmm_t::init(bool force){
	bool retval = true;
	if (force){
		retval &= (this->isHbridgeInit = this->initH()); 				// @suppress("Assignment in condition")
		retval &= (this->isAdcInit = this->initAdc()); 				// @suppress("Assignment in condition")
		retval &= (this->isTimerInit = this->initTimer()); 				// @suppress("Assignment in condition")
	} else {
		retval &= this->isHbridgeInit || (this->isHbridgeInit = this->initH()); 				// @suppress("Assignment in condition")
		retval &= this->isAdcInit || (this->isAdcInit = this->initAdc()); 				// @suppress("Assignment in condition")
		retval &= this->isTimerInit || (this->isTimerInit = this->initTimer()); 				// @suppress("Assignment in condition")
	}
	xQueueReset(this->qu);
	checkWires();
	return retval;
}

/**
 * @brief Runs ACMM by starting the timer. Starts with "run" state (not the "sleep" one)
 * @param none
 * @return true on ok
 */
bool Acmm_t::run(){
	this->isHbridgeInit || (this->isHbridgeInit = this->initH());
	this->isAdcInit || (this->isAdcInit = this->initAdc());
	this->isTimerInit || (this->isTimerInit = this->initTimer());

	bool retval = this->setOpMode(acmmOpMode_t::run);
	//pmLocked = pmLocked || (ESP_OK == esp_pm_lock_acquire(pmLock));
	if (retval){
//		ESP_LOGI(TAG, "Acmm_t::run: freq: %dHz, aggOver %d, sleep %d, interlock %s", freq, aggOver, ACMM_SLEEP_MS, acmmInterlockStr[(uint8_t)interlockMode]);
//		ESP_LOGI(TAG, "Acmm_t::run: freq: %dHz, aggOver %d, sleep %d", freq, aggOver, ACMM_SLEEP_MS);
	 	return true;
	}else{
		ESP_LOGW(TAG, "Acmm_t::run: timer_start failed.");
		return false;
	}
}

/**
 * @brief Pauses ACMM by stopping the idf timer and setting Hbridge to HiZ.
 * @param none
 * @return true on ok
 */
bool Acmm_t::stop(){
	bool retval = setOpMode(acmmOpMode_t::stop);
	//pmLocked = pmLocked && !(ESP_OK == esp_pm_lock_release(pmLock));
	return retval;
}

/**
 * @brief Runs, stops, puts to sleep the ACMM. Really does the job
 * @param acmmOpMode_t
 * @return true on ok
 */
bool Acmm_t::setOpMode(acmmOpMode_t aMode){
	bool retval = true;

	if (! xSemaphoreTake( semaphore, pdMS_TO_TICKS(2000))){
		ESP_LOGW(TAG, "::setOpMode: mutex not available! requested mode: %d", (uint8_t)aMode);
		return false;
	}

	esp_timer_stop(espIdfTimer);	//it will not generate more callback events
	opMode = aMode;
	switch (aMode){
	case acmmOpMode_t::run:
		phaseMeas.pause();
		if (interlockMode != acmmInterlockMode_t::off)
			tp.suspend();
		if (selfPullupEn)
			activatePup(true);
		phaseMeas.connectTransAmp(false);	//disconnect this thing, lest it mess in
		vTaskResume(tsk);
		ESP_LOGI(TAG, "::setOpMode: RUN for %dms, ilock %s", ACMM_WORK_MS(acmm.freq, acmm.aggOver), acmmInterlockStr[(uint8_t)interlockMode]);
		timerPeriodus = (uint64_t)(1000 * (ACMM_FREQ_2_MS(freq)));
		espTmrTs = 0;	//prevent fake warning about lagging timer
		retval = (ESP_OK == esp_timer_start_periodic(espIdfTimer, timerPeriodus));
		break;
	case acmmOpMode_t::sleep:
		ESP_LOGI(TAG, "::setOpMode: SLEEP for %dms, ilock %s", ACMM_SLEEP_MS, acmmInterlockStr[(uint8_t)interlockMode]);
		timerPeriodus = (uint64_t)(1000 * (ACMM_SLEEP_MS));
		retval = (ESP_OK == esp_timer_start_periodic(espIdfTimer, timerPeriodus));
		if (interlockMode != acmmInterlockMode_t::off){
			if (interlockMode == acmmInterlockMode_t::gnd)
				setH(acmmHbridgeState_t::H1pup);	// then H2 is tied to gnd
			else {	//then make all HiZ
				setH(acmmHbridgeState_t::Hiz);
				if (selfPullupEn)
					activatePup(false);
			}
		} else {
			if (selfPullupEn)
				activatePup(false); //if we can control pullup, switching off saves energy
			else
				setH(acmmHbridgeState_t::Hiz);
		}
		tp.resume();
		activatePup(false);
		setH(acmmHbridgeState_t::Hiz);
		phaseMeas.resume();
		break;
	case acmmOpMode_t::stop:
		ESP_LOGI(TAG, "::setOpMode: stop");
		retval = (ESP_OK == esp_timer_stop(espIdfTimer));
		vTaskSuspend(tsk);
		if (interlockMode != acmmInterlockMode_t::off){
			if (interlockMode == acmmInterlockMode_t::gnd)
				setH(acmmHbridgeState_t::H1pup);	// then H2 is tied to gnd
			else {	//then make all HiZ
				setH(acmmHbridgeState_t::Hiz);
				if (selfPullupEn)
					activatePup(false);
			}
		} else{
			if (selfPullupEn)
				activatePup(false); //if we can control pullup, switching off saves energy
			else
				setH(acmmHbridgeState_t::Hiz);
		}
		tp.resume();
		activatePup(false);
		setH(acmmHbridgeState_t::Hiz);
		phaseMeas.resume();
		break;
	default:
		break;
	}
	xSemaphoreGive(semaphore);
	return retval;
}

/**
 * @brief Returns ACMM run/sleep mode. ISR safe.
 * @param
 * @return acmmOpMode_t::
 */
acmmOpMode_t Acmm_t::getOpMode(void) {
	return opMode;
}

void Acmm_t::activatePup(bool arg){
	if (arg){	// drive high
		gpio_set_direction(ACMM_VEL1_PIN, GPIO_MODE_OUTPUT);
		gpio_set_level(ACMM_VEL1_PIN, 1);
		gpio_set_direction(ACMM_VEL2_PIN, GPIO_MODE_OUTPUT);
		gpio_set_level(ACMM_VEL2_PIN, 1);
	}else{		// set HiZ
		gpio_set_direction(ACMM_VEL1_PIN, GPIO_MODE_INPUT);
		gpio_set_direction(ACMM_VEL2_PIN, GPIO_MODE_INPUT);
	}
}

/**
 * @brief Sets Hbridge to a state. ISR safe. Not IRAM compatible ???
 * @param
 * @return none
 */
void Acmm_t::setH(acmmHbridgeState_t aState){
	switch (aState)
	{
	case acmmHbridgeState_t::H1pup:
		hpinSetHL(ACMM_H_PIN_1, 1);
		hpinSetHL(ACMM_H_PIN_2, 0);
		break;
	case acmmHbridgeState_t::H2pup:
		hpinSetHL(ACMM_H_PIN_1, 0);
		hpinSetHL(ACMM_H_PIN_2, 1);
		break;
	case acmmHbridgeState_t::Hiz:
		hpinSetHL(ACMM_H_PIN_1, 1);
		hpinSetHL(ACMM_H_PIN_2, 1);
		break;
	default:
		ESP_LOGW(TAG, "setH invalid argument");
		break;
	}
	state = aState;
}


/**
 * @brief Sets ACMM Hbridge to a random polarity.
 * @param
 * @return none
 */
void Acmm_t::setHRandom(void){
	setH(static_cast<acmmHbridgeState_t>((esp_random() & 0x01) + 1));
}


/**
 * @brief Switches ACMM Hbridge polarity. If Hbridge polarity was HiZ, then random.
 * @param
 * @return none
 */
acmmHbridgeState_t Acmm_t::switchH(void){
	/*
	  bit-wise arithmetic does not work on enum class (works on traditional enums)
	  if (state != acmmHbridgeState_t::Hiz)
	    set((acmmHbridgeState_t)((state ^ 0x03) & 0x03 ));
	  else
	    setRandomLR();
	 */
	switch (state)
	{
	case acmmHbridgeState_t::H1pup:
		setH(acmmHbridgeState_t::H2pup);
		break;
	case acmmHbridgeState_t::H2pup:
		setH(acmmHbridgeState_t::H1pup);
		break;
	default:
		setHRandom();
	}
	return state;
}


/**
 * @brief Checks ACMM wiring, decide on which half-wave to sample
 * @param
 * @return true on ok
 */
bool Acmm_t::checkWires(void) {
	const uint8_t rounds = 5;
	const int delayUs = 20000;
	int rawH1pup=0, rawH2pup=0;

	this->isHbridgeInit || (this->isHbridgeInit = this->initH());
	this->isAdcInit || (this->isAdcInit = this->initAdc());

	for (uint8_t i=0; i<rounds; i++){
		//ESP_LOGV(TAG, "checkWires: adc = %d\n",analogRead(ACMM_INPIN));
		setH(acmmHbridgeState_t::H1pup);
		//ESP_LOGV(TAG, "checkWires: H1up...");
		delayUsbyNops(delayUs);
		//ESP_LOGV(TAG, " ... ");
		rawH1pup += adc1_get_raw(ACMM_INPIN);
		//ESP_LOGV(TAG, " read: %d.\n", rawH1pup);

		setH(acmmHbridgeState_t::H2pup);
		//ESP_LOGV(TAG, "checkWires: H2up...");
		delayUsbyNops(delayUs);
		//ESP_LOGV(TAG, " ... ");
		rawH2pup += adc1_get_raw(ACMM_INPIN);
		//ESP_LOGV(TAG, " read: %d.\n", rawH2pup);
		taskYIELD();
	}
	ESP_LOGI(TAG, "checkWires: rawH1pup=%d, rawH2pup=%d", rawH1pup/rounds, rawH2pup/rounds );

	if (abs(rawH1pup - rawH2pup) < ACMM_ADC_SHORTCUT_TOL * rounds){
		ESP_LOGW(TAG,"checkWires: ############################################\n");
		ESP_LOGW(TAG,"checkWires: Wrong connection or short-circuit on electrodes !!!!!! \n");
		ESP_LOGW(TAG,"checkWires: I did not detect much difference on ADC after switching the H-bridge polarity.\n");
		ESP_LOGW(TAG,"checkWires: Exchange electrodes' driving lines or remove short-circuit, or I don't know what...\n");
		ESP_LOGW(TAG,"checkWires: then stop and restart measurement.\n");
		ESP_LOGW(TAG,"checkWires: Will continue as standalone ADC input pin.\n");
		ESP_LOGW(TAG,"checkWires: ############################################\n");
	}
	takeSampleOnH1 = rawH1pup > rawH2pup;
	ESP_LOGI(TAG, "Chose to sample on %s", (takeSampleOnH1) ? "H1up" : "H2up" );
	return true;
}


/**
 * @brief Initializes ADC1 used for ACMM: attenuation and bit-width.
 * @param
 * @return true on ok
 */
bool Acmm_t::initAdc(void) {
	if (ESP_OK != adc1_config_width(ACMM_BITS)){
		ESP_LOGW(TAG, "Acmm::initAdc: failed adc1_config_width(ACMM_BITS)");
		return false;
	}
	if (ESP_OK != adc1_config_channel_atten(ACMM_INPIN, ACMM_ADC_ATTEN)){
		ESP_LOGW(TAG, "Acmm::initAdc: failed adc1_config_channel_atten(ACMM_INPIN, ADC_ATTEN_DB_0)");
		return false;
	}

	//Check Vref is burned into eFuse
	if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
		ESP_LOGI(TAG, "eFuse Vref: Supported");
	} else {
		ESP_LOGI(TAG, "eFuse Vref: NOT supported");
	}

    adc_chars = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(
    		ADC_UNIT_1, ACMM_ADC_ATTEN, ACMM_BITS, DEFAULT_VREF, adc_chars);

	ESP_LOGI(TAG, "Acmm::initAdc: ok");
	return true;
}


/**
 * @brief Checks how fast ACMM can go, not to see input channel saturation effects.
 * @param
 * @return minimal half-period in ms
 */
uint16_t Acmm_t::speedCheck(){

	uint16_t raw;
	uint16_t raw1 = 0;
	uint32_t hwms = 1 << 9;
	uint32_t hwmsMin = 0;

	this->isAdcInit || this->initAdc();
	this->isHbridgeInit || this->initH();

	setHRandom();

	while (true)
	{
		raw = 0;
		for (int smallIter=0; smallIter < 32; smallIter++){
			if (takeSampleOnH1 == (state==acmmHbridgeState_t::H1pup)){
				raw += adc1_get_raw(ACMM_INPIN);
				switchH();
			}else{
				switchH();
			}
			delayUsbyNops(hwms << 10);
		}
		raw = raw >> 4;
		ESP_LOGI(TAG,"acmmSpeedCheck: hw=%4d sample=%5d", hwms, raw);
		if (raw >= ACMM_SPEED_CHECK_DROP * raw1){
			hwmsMin = hwms;
			ESP_LOGI(TAG," > %1.2f, OK.\n", ACMM_SPEED_CHECK_DROP);
		} else
			ESP_LOGI(TAG," < %1.2f, too fast.\n", ACMM_SPEED_CHECK_DROP);

		raw1 = raw;
		hwms = hwms >> 1;

		if (hwms==0)
			break;
	}
	return hwmsMin;

}


void Acmm_t::switchSpeedCheck(void){
	switchOnly = true;
	for (uint64_t i = 10000; i > 50; i = i-50){
		esp_timer_stop(espIdfTimer);
		ESP_ERROR_CHECK(esp_timer_start_periodic(espIdfTimer, timerPeriodus));
		espTmrTs = 0;
		timerLags = false;
		vTaskDelay(pdMS_TO_TICKS(100));
		if (timerLags){
			ESP_LOGI(TAG, "Timer lags at %lld", i);
			break;
		}
	}
}



/*
 * void taskAcmmCbkFast(void)
 * Is used to switch H-bridge polarity, take a sample, do computations.
 * Will run ACMM_SMALL_ITER * ACMM_AGGOVER times, then yields control to a slower sleep task
 */
//void taskAcmmCbkFast(){
//  //if (~taskAcmm.isFirstIteration())
//  static int iterInner=0, iterAgg=0;
//  static int acmmInnerMin=INT16_MAX, acmmInnerMax = 0, acmmClean = 0, acmmMin = INT16_MAX;
//  static int acmmMinPrev;
//  static unsigned long lastTime = millis();
//  int raw;
//
//  unsigned long thisTime = millis();
//  if (thisTime - lastTime > (taskAcmm.getInterval() + 1)){
//    ESP_LOGI(TAG, " taskAcmmCbkFast: too late !!!, real delay is %d, wanted %d\n", thisTime - lastTime, taskAcmm.getInterval());
//  }
//  lastTime = millis();
//
//  if (takeSampleOnH1 == (bridge.state==acmmHbridgeState_t::H1pup)){
//    raw = analogRead(ACMM_INPIN);
//    bridge.switchLR();
//  }else{
//    bridge.switchLR();
//    return;
//  }
//
//  acmmClean += raw;
//  if (acmmInnerMin>raw){
//    acmmInnerMin = raw;
//  } else if(acmmInnerMax<raw){
//    acmmInnerMax = raw;
//  }
//
//  // logit(
//  //   LOGLVL_MEAS,"taskAcmmCbkFast: iterInner=%d, raw=%d, innerMin=%d, innerMax=%d\n",
//  //   iterInner, raw, acmmInnerMin, acmmInnerMax);
//
//  if (++iterInner == ACMM_SMALL_ITER){
//    acmmClean -= acmmInnerMin;
//    acmmClean -= acmmInnerMax;
//    acmmClean = acmmClean / (ACMM_SMALL_ITER - 2);
//
//    iterInner = 0; acmmInnerMax = 0; acmmInnerMin = INT16_MAX;
//
//    //ESP_LOGI(TAG, "taskAcmmCbkFast: \t\t\tacmmClean=%d\n", acmmClean);
//
//    if (iterAgg==0){
//      acmmMin = INT16_MAX;
//    }
//    if (acmmMin > acmmClean){
//      acmmMin = acmmClean;
//    }
//    if (++iterAgg == acmm.aggOver){
//      /*do some lp bessel filtering on acmmMin,
//       * don't publish - better do it in slower regime or not at all, just leave it for measurement messages
//       */
//      moisture = (ACMM_A1 * acmmMin + ACMM_A2 * acmmMinPrev + ACMM_B1 * moisture) / (ACMM_A1 + ACMM_A2 + ACMM_B1);
//      /* new smooth value is calculated, may rest for some time*/
//      ESP_LOGI(TAG,"taskAcmmCbkFast: \t\t\t\t moisture=%4.1f (sr:%3dms, ao:%3d)\n", moisture, taskAcmm.getInterval(), acmm.aggOver);
//
//      #ifdef USE_ARIMA
//        if (obs.size() < arimaMod.ar_params.size())
//          obs.push_back(acmmMin);
//        else{
//          std::rotate(obs.begin(),obs.begin()+1,obs.end());
//          obs.back() = acmmMin;
//          preds = ARIMAForecaster::forecast(arimaMod, 1, obs);
//          ESP_LOGI(TAG, "ARIMA: %f\n",preds.values[0]);
//        }
//      #endif
//
//      acmmMinPrev = acmmMin;
//
//      taskAcmm.setInterval(ACMM_SLEEP);
//      taskAcmm.setCallback(&taskAcmmCbkSlow);
//      bridge.set(acmmHbridgeState_t::Hiz);
//      ESP_LOGI(TAG,"taskAcmmCbkFast: ACMM goes to sleep for %dms.\n", ACMM_SLEEP);
//      iterAgg = 0;
//
//    }
//  }
//}
//
///*
// * void taskAcmmCbkSlow(void)
// * Is used as a periodical sleep state of acmm.
// * When finished, H bridge should be set to random polarity state,
// * to avoid dc bias due to starting always from same polarity
// */
//void taskAcmmCbkSlow(void){
//  taskAcmm.setInterval(ACMM_FREQ_2_MS(acmm.freq));
//  taskAcmm.setCallback(&taskAcmmCbkFast);
//  bridge.setRandomLR();
//  ESP_LOGI(TAG,"taskAcmmCbkSlow: ACMM woke up. Will work for %dms\n",ACMM_SMALL_ITER*2*taskAcmm.getInterval()*acmm.aggOver);
//}
