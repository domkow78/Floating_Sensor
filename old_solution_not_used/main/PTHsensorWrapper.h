/*
 * PTHsensorWrapper.h
 *
 *  Created on: 2 cze 2021
 *      Author: JablonskiP
 */

#ifndef MAIN_PTHSENSORWRAPPER_H_
#define MAIN_PTHSENSORWRAPPER_H_

//#include "freeRTOS.h"
#include <sht3x.h>
#include <esp8266_wrapper.h>
//#include <Wire.h>
//#include <SPI.h>

enum class pthType_t : uint8_t { BME280 = 0x01, SHT = 0x02, DHT = 0x03 };

#define PTH_DEFAULT_TYPE pthType_t::BME280

#define I2C_BUS       I2C_NUM_0
#define I2C_SCL_PIN   GPIO_NUM_22
#define I2C_SDA_PIN   GPIO_NUM_21
#define USE_PTH_V_PIN
#ifdef USE_PTH_V_PIN
	#define PTH_V_PIN		GPIO_NUM_25
#endif

#define PTH_PRESSURE_DEFAULT 101300

#ifdef USE_SHT
  #include <sht3x.h>
  #include <esp8266_wrapper.h>
//  #include <Wire.h>
//  #include <SPI.h>
#endif

#ifdef USE_BME
//	#include <Wire.h>
//	#include <SPI.h>
	#include "bme280.h"
#endif

#ifdef USE_DHT
  #include "DHT.h"  //triple sensor BME
#endif


class PTHsensorWrapper {
	sht3x_sensor_t* sht3x;
#ifdef USE_BME
//	uint32_t ts;
	uint32_t req_delay;
#endif
	//variables for the other sensors are declared in their libraries, stupid mess.
public:
	int temperature;
	int humidity;
	int pressure;
	pthType_t type = PTH_DEFAULT_TYPE;
	PTHsensorWrapper();
	bool init();
	void hold_up_when_sleep(bool enable);
	bool measure(void);
};

extern PTHsensorWrapper pthWrapper;

#endif /* MAIN_PTHSENSORWRAPPER_H_ */
