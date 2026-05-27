#ifndef BLINK_H
#define BLINK_H

#include <driver/i2s.h>

#define BLINK_PERIOD_MS			2000
#define BLINK_CODE_LEN			64
#define BLINK_BIT_MS 			BLINK_PERIOD_MS / BLINK_CODE_LEN
//#define BLINK_PIN_RED			GPIO_NUM_11	//https://www.upesy.com/blogs/tutorials/esp32-pinout-reference-gpio-pins-ultimate-guide
//#define BLINK_PIN_RED			GPIO_NUM_27
//#define BLINK_PIN_GREEN			GPIO_NUM_10	//https://www.upesy.com/blogs/tutorials/esp32-pinout-reference-gpio-pins-ultimate-guide
//#define BLINK_PIN_GREEN			GPIO_NUM_14

#define BLINK_PIN_RED			GPIO_NUM_16
#define BLINK_PIN_GREEN			GPIO_NUM_17

#define BLINK_PAUSE_AFTER		1000
#define BLINK_RESUME_MS			5000
#define BLINK_QUEUE_LEN			4


#define BLINK_STACK_DEPTH	1800
#define TASK_HEAP_MIN_FREE 	100

#define BLINK_BATTERY_ERROR	0x00000000f0f0f0f0
#define BLINK_WIFI_ERROR	0x0000000000f0f0f0
#define BLINK_MQTT_ERROR	0x000000000000f0f0
#define BLINK_SENSOR_ERROR	0x00000000000000f0

#define BLINK_HEALTHY		0x1000000000000000


enum class Blink_led {red, green};

class Blink_t{
private:
	bool m_allowSuspend = false;
public:
	bool m_requestSuspend = false;
	bool isInitialized = false;
	QueueHandle_t qu;
    bool isEmpty = true;
    Blink_t();
    bool init();
    bool show(uint64_t code, Blink_led led, bool blocking=false);
    void allowSuspend(bool yes);
    void requestSuspend(void);
};

extern Blink_t blink;

#endif
