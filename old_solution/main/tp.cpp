#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/touch_pad.h"
#include "soc/rtc.h"
#include "esp_log.h"
#include "tp.h"
#include "msgSender.h"
#include "timeespt.h"
#include "main.h"
#include "esp_dsp.h"
#include <math.h>

static const char* TAG = "TP";
#include "esp_log.h"

//#define TOUCH_PAD_NO_CHANGE   (-1)
//#define TOUCH_THRESH_NO_USE   (0)
//#define TOUCH_FILTER_MODE_EN  (1)
//#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)
//#define TP_NO	TOUCH_PAD_GPIO4_CHANNEL
#define TP_NO	TOUCH_PAD_NUM0	/*!< Touch pad channel 0 is GPIO4(ESP32) */

#define TP_MEAS_CYCLE 0xFFFF	// in SW mode, this will determine time spend per each touch_pad_read
#define TP_SLEEP_CYCLE 0x0FFF	//this is ignored when SW mode
#define TP_MIN_OVER	50
#define TP_IIR_FB	0.95
#define TP_SAMPLE_PERIOD_MS	100
#define TP_SAMPLE_FREQ		(1000 / TP_SAMPLE_PERIOD_MS)

#define TASK_TP_HEAP	2048+512
#define TASK_HEAP_MIN_FREE 50

#define TASK_TP_FFT_HEAP 2048+2048

tp_t tp;

//#include <deque>
//#include <sstream>

//static std::deque<uint16_t> deq(0);
//static std::stringstream ss;



//static void filter_cb(uint16_t *raw_value, uint16_t *filtered_value){
//	tp.wasCbkRun = true;
//	if (tp.ptrVal != NULL)
//		*(tp.ptrVal) = raw_value[TP_NO];
//
//		//		*(tp.ptrVal) = *raw_value;
//}


static TickType_t ts;
static 	uint16_t minOverCnt = tp.minOver;
static 	uint16_t tpVal;
static 	uint16_t tpMinVal = INT16_MAX;

#define TP_FFT_SAMPLES 128

//__attribute__((aligned(16)))
//float fft[TP_FFT_SAMPLES];

void taskTpFftCbk(void* ptr){
	for (;;){
		ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
		// FFT Radix-4
		ESP_LOGI(TAG, "FFT starts");
		unsigned int fftStartCycleCnt = dsp_get_cpu_cycle_count();
		dsps_fft4r_fc32(tp.pFft, TP_FFT_SAMPLES>>1);
		ESP_LOGI(TAG, "FFT bit reverse starts");
		// Bit reverse
		dsps_bit_rev4r_fc32(tp.pFft, TP_FFT_SAMPLES>>1);
		// Convert one complex vector with length N/2 to one real spectrum vector with length N/2
		ESP_LOGI(TAG, "FFT cplx2real starts");
		dsps_cplx2real_fc32(tp.pFft, TP_FFT_SAMPLES>>1);
		ESP_LOGI(TAG, "FFT took %d cpu cycles.", dsp_get_cpu_cycle_count() - fftStartCycleCnt);

		float re,im;
		for (int i = 0 ; i < TP_FFT_SAMPLES/2 ; i++) {
			re = *(tp.pFft + i*2 + 0);
			im = *(tp.pFft + i*2 + 1);
			*(tp.pFft+i) = 10 * log10f((re * re + im * im + 0.0000001)/TP_FFT_SAMPLES);
		}
		dsps_view(tp.pFft, TP_FFT_SAMPLES/2, 128, 10,  -60, 40, '|');
	}
}

/*
 * do here something like ACMM min over longer time, then IIR
 */
void taskTpCbk(void* ptr){
	uint32_t i=0;
	for (;;){
		ts = xTaskGetTickCount();
//		uint64_t tsus = esp_timer_get_time();

		touch_pad_read(TP_NO, &tpVal);	//here we spend time, defined by TP_MEAS_CYCLE, max ~8ms

		if (tpVal < tpMinVal)
			tpMinVal = tpVal;

		if (tp.useFft){
			*(tp.pFft+i) = (float)tpVal;
			i++;
			if (i == TP_FFT_SAMPLES){
				i = 0;
				xTaskNotify(tp.taskFft, 0, eNoAction);
			}
		}

		minOverCnt--;

//		ESP_LOGI(TAG, "IIR: %6.3f => %6.3f", (float)tpVal, tp.iir.tick((float)tpVal));
//		tp.comb.tick((float)tpVal);
		tp.iir1.tick((float)tpVal);
		tp.iir2.tick((float)tpVal);

		if (minOverCnt <= 0){
			*(tp.ptrVal) = (uint16_t)((double)(*(tp.ptrVal)) * TP_IIR_FB + (double)tpMinVal * (1-TP_IIR_FB));
			minOverCnt = tp.minOver;
			tpMinVal = INT16_MAX;

			UBaseType_t minFreeHeap = uxTaskGetStackHighWaterMark( NULL );
			if ( minFreeHeap < TASK_HEAP_MIN_FREE)
				ESP_LOGW(TAG, " HEAP is short! Only %d left.", minFreeHeap);

			ESP_LOGI(TAG, "taskTpCbk(Fs:%dHz, minOver:%dms): tpFltr: %d; lastRaw: %d",
					TP_SAMPLE_FREQ, TP_SAMPLE_PERIOD_MS*tp.minOver, *(tp.ptrVal), tpVal);
		}
		vTaskDelay(pdMS_TO_TICKS(TP_SAMPLE_PERIOD_MS) - (xTaskGetTickCount()-ts));
	}
}

bool tp_t::start(int16_t reset){
	if (!enabled){
		ESP_LOGW(TAG, "::enable first!");
		return false;
	}
	if (!isTpInit){
		ESP_LOGW(TAG, "::init first!");
		return false;
	}
	if (reset >=0 )
		*ptrVal = (uint16_t)reset;
	vTaskResume(tp.taskid);
	started = true;
	ESP_LOGI(TAG, "tp enabled, %d", *(tp.ptrVal));
	return true;
}

void tp_t::enable(bool yes){
	enabled = yes;
	if (enabled){
		ESP_LOGI(TAG, "enabled");
//		resume();
	}
	else{
		ESP_LOGI(TAG, "disabled");
		suspend();
	}
}

void tp_t::resume(){
	if (started && enabled)
		if (tp.taskid)
			vTaskResume(tp.taskid);
}

void tp_t::suspend(){
	//check that taskid is not null, or you suspend current task!!!
	if (tp.taskid)
		vTaskSuspend(tp.taskid);
}

bool tp_t::measure(uint16_t* aPtrVal, uint16_t* aPtrValRaw){
	if (aPtrValRaw != NULL){
		touch_pad_read(TP_NO, aPtrValRaw);
	} else
		return false;

	if (aPtrVal != NULL)
		*aPtrVal = *ptrVal;
	return true;
}

tp_t::tp_t() {
	taskid = NULL;
	taskFft = NULL;
	pFft = NULL;
	minOver = TP_MIN_OVER;
}

bool tp_t::init(uint16_t* aPtrVal, bool aUseFtt) {
	ptrVal = aPtrVal;
	ESP_ERROR_CHECK(touch_pad_io_init(TP_NO));
    // Initialize touch pad peripheral.
    // The default fsm mode is software trigger mode.
    ESP_ERROR_CHECK(touch_pad_init());
    // Set reference voltage for charging/discharging
    // In this case, the high reference valtage will be 2.7V - 1V = 1.7V
    // The low reference voltage will be 0.5
    // The larger the range, the larger the pulse count value.
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);

	touch_pad_config(TP_NO, 0);
//	touch_pad_set_cnt_mode(TP_NO, TOUCH_PAD_SLOPE_MAX, TOUCH_PAD_TIE_OPT_LOW);
//	touch_pad_set_cnt_mode(TP_NO, TOUCH_PAD_SLOPE_4, TOUCH_PAD_TIE_OPT_LOW);
	//~725 all-built free; 585 on plate's edge => 80% atten
	touch_pad_set_cnt_mode(TP_NO, TOUCH_PAD_SLOPE_7, TOUCH_PAD_TIE_OPT_LOW);
	//~1295 all-built free; 1048 on plate's edge => 80% atten


//	touch_pad_set_filter_read_cb(filter_cb);
	touch_pad_set_meas_time(TP_SLEEP_CYCLE, TP_MEAS_CYCLE);
	touch_pad_set_fsm_mode(TOUCH_FSM_MODE_SW);

	tp.minOver = TP_MIN_OVER;
//	ESP_LOGI(TAG, "TP sleep time is %d ms", TP_SLEEP_CYCLE / (rtc_clk_slow_freq_get_hz()>>10));
//	ESP_LOGI(TAG, "TP meas. time is %d ms", TP_MEAS_CYCLE / (1<<13), );
	ESP_LOGI(TAG, "TP SP %dms, AO %d", TP_SAMPLE_PERIOD_MS, tp.minOver);

    xTaskCreatePinnedToCore(&taskTpCbk, "tp", TASK_TP_HEAP, NULL, 25, &taskid, 1);
    vTaskSuspend(taskid);

    useFft = aUseFtt;
    if (useFft){
		pFft = (float*)aligned_alloc(16, TP_FFT_SAMPLES * sizeof(float));
		if (!pFft){
			ESP_LOGE(TAG, "Not possible to get memory for FFT data");
			return false;
		}
		esp_err_t ret;
		ret = dsps_fft4r_init_fc32(NULL, TP_FFT_SAMPLES>>1);
		if (ret != ESP_OK){
			ESP_LOGE(TAG, "Not possible to initialize FFT4R. Error = %i", ret);
			return false;
		}
		xTaskCreatePinnedToCore(&taskTpFftCbk, "tpfft", TASK_TP_FFT_HEAP, NULL, 1, &taskFft, 1);
		if (taskFft)
			ESP_LOGI(TAG, "FFT initialized ok.");
		else
			ESP_LOGE(TAG, "FFT failed to initialize.");
    }

    isTpInit = true;
	return true;
}

void tp_t::stop() {
	vTaskSuspend(tp.taskid);
	started = false;
	ESP_LOGI(TAG, "tp stopped, %d", *(tp.ptrVal));
}

//void tp_t::run(uint32_t count){
//	xTaskNotify(taskid, count, eSetValueWithOverwrite);
//	vTaskResume(taskid);
//}
//
//void tp_t::stop(bool letFinish){
//	xTaskNotify(taskid, 0, eSetValueWithOverwrite);
//	if (letFinish)
//		vTaskSuspend(taskid);
//}
