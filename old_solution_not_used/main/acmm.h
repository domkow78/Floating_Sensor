	#ifndef ACMM_H
#define ACMM_H

#include "driver/timer.h"
#include <driver/adc.h>
//#include "freeRTOS.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_adc_cal.h"
#include <esp_pm.h>


#define ACMM_FREQ_HZ 400
#define ACMM_CLEANOVER 16
#define ACMM_AGGOVER 128
#define ACMM_SLEEP_MS 12000
#define ACMM_WORK_MS(freq,aggOver)	((int)(ACMM_CLEANOVER * 2 * ACMM_FREQ_2_MS(freq) * aggOver))
#define ACMM_A1 0.05f
#define ACMM_A2 0.05f
#define ACMM_B1 0.9f
#define ACMM_ARIMA_AR {0, 0}
#define ACMM_ARIMA_I {0}
#define ACMM_ARIMA_MA {0.2}

#define ACMM_ADC_SHORTCUT_TOL 0x05
#define ACMM_SPEED_CHECK_DROP 0.95

#define ACMM_SELF_PULLUP_EN true

//#define ACMM_H_PIN_1 GPIO_NUM_13
#define ACMM_H_PIN_1 GPIO_NUM_5
//#define ACMM_H_PIN_2 GPIO_NUM_27
#define ACMM_H_PIN_2 GPIO_NUM_27

//#define	ACMM_PU_PIN	GPIO_NUM_26
#define	ACMM_VEL1_PIN	GPIO_NUM_33
#define	ACMM_VEL2_PIN	GPIO_NUM_26

//	#define ACMM_INPIN ADC1_GPIO32_CHANNEL
//	#define ACMM_INPIN ADC1_GPIO39_CHANNEL	//ADC1.3, SensVN, RtcIo3
#define ACMM_INPIN ADC1_CHANNEL_3


#define ACMM_BITS ADC_WIDTH_BIT_10
#define ACMM_ADC_ATTEN ADC_ATTEN_DB_0

/*https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html
The ESP32 integrates two 12-bit SAR (Successive Approximation Register) ADCs supporting a total of 18 measurement channels (analog enabled pins).
The ADC driver API supports ADC1 (8 channels, attached to GPIOs 32 - 39), and ADC2 (10 channels, attached to GPIOs 0, 2, 4, 12 - 15 and 25 - 27). However, the usage of ADC2 has some restrictions for the application:
ADC2 is used by the Wi-Fi driver. Therefore the application can only use ADC2 when the Wi-Fi driver has not started.
Some of the ADC2 pins are used as strapping pins (GPIO 0, 2, 15) thus cannot be used freely. Such is the case in the following official Development Kits:
ESP32 DevKitC: GPIO 0 cannot be used due to external auto program circuits.
ESP-WROVER-KIT: GPIO 0, 2, 4 and 15 cannot be used due to external connections for different purposes.*/

#define ACMM_FREQ_2_MS(freq) 1000.0 / freq

#define ACMM_TIMER_IDX TIMER_0
#define ACMM_TIMER_GRP TIMER_GROUP_0
#define ACMM_TIMER_INTR TIMER_INTR_T0
#define ACMM_TIMER_LAG_ALLOW_US 200

enum class acmmHbridgeState_t : uint8_t { H1pup = 0x01, H2pup = 0x02, Hiz = 0x03 };
enum class acmmOpMode_t : uint8_t { stop = 0x00, sleep = 0x01, run = 0x02 };
enum class acmmInterlockMode_t : uint8_t {gnd = 0x0, hiz = 0x1, off = 0x2}; //electric configuration when interlocked

class Acmm_t{
private:

	bool isTimerInit = false;
	bool isHbridgeInit = false;
	bool isAdcInit = false;
    bool initTimer(void);
    //esp_pm_lock_handle_t pmLock;
    //bool pmLocked;
    esp_timer_handle_t espIdfTimer;
    SemaphoreHandle_t semaphore = NULL;
public:
	acmmOpMode_t opMode;
    bool switchOnly;
	esp_adc_cal_characteristics_t *adc_chars;
	QueueHandle_t qu;
	TaskHandle_t tsk;
    float* pMoisture;
    acmmHbridgeState_t state;
    uint64_t timerPeriodus;
    bool takeSampleOnH1 = true;
    uint32_t aggOver;
    float a1, a2, b1;
    float a1a2b1;
    uint32_t freq;
    portMUX_TYPE adc1Mux;
    bool selfPullupEn = ACMM_SELF_PULLUP_EN;
    acmmInterlockMode_t interlockMode = acmmInterlockMode_t::off;
    uint64_t realTmrPeriodus = 0;
    uint64_t espTmrTs = 0;
    bool timerLags = false;
    esp_err_t (*hpinSetHL)(gpio_num_t gpio_num, uint32_t value);


    Acmm_t(float* apMoisture);
    void activatePup(bool arg);
    bool run(void);
    bool stop(void);
    bool init(bool force = false);
    bool initH(void);
	bool initAdc(void);
	void setB1(float aValue);
    acmmOpMode_t getOpMode(void);
    bool setOpMode(acmmOpMode_t mode);
    void setH(acmmHbridgeState_t aState);
    void setHRandom(void);
    bool checkWires(void);
    acmmHbridgeState_t switchH(void);
	uint16_t speedCheck(void);
	void switchSpeedCheck(void);
};

extern Acmm_t acmm;

#endif
