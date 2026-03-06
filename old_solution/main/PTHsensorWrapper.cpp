/*
 * PTHsensorWrapper.cpp
 *
 *  Created on: 2 cze 2021
 *      Author: JablonskiP
 */

#include "PTHsensorWrapper.h"
#include "esp_log.h"
//#include "DHT.h"  //triple sensor BME
#include "blink.h"
#include "driver/i2c.h"
#include "driver/rtc_io.h"

#define NOP() asm volatile ("nop")


#ifdef USE_SHT
//	#include <Wire.h>
//	#include <SPI.h>
    sht3x_sensor_t* sht3x;
#endif

#ifdef USE_DHT
  #define DHTPIN 0        // pin where sensor is connected - GPIO0 - D3
  #define DHTTYPE DHT11   // DHT 11
  DHT dht(DHTPIN, DHTTYPE);
#endif

#ifdef USE_BME
	#include "bme280.h"
	#define BME_I2C_ADDR 0x76
	#define BME_I2C_SPEED 1000000
#endif

static const char* TAG = "PTH";

static uint8_t dev_addr; // this really needs be not-in-class, as it is passed somehow in secrecy by beloved Bosch coders...
static struct bme280_dev bme_dev;	// this too

extern int temperature;
extern int humidity;
extern int pressure;

static void i2c_master_init (i2c_port_t bus, gpio_num_t scl, gpio_num_t sda, uint32_t freq)
{
//	https://stackoverflow.com/questions/10828294/c-and-c-partial-initialization-of-automatic-structure
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
	i2c_config_t conf = {
    		.mode = I2C_MODE_MASTER,
    		.sda_io_num = sda,
    		.scl_io_num = scl,
    		.sda_pullup_en = GPIO_PULLUP_ENABLE,
    		.scl_pullup_en = GPIO_PULLUP_ENABLE,
			{.master = {.clk_speed = freq}},
    		.clk_flags = 0,
    };
	#pragma GCC diagnostic pop
    i2c_param_config(bus, &conf);
    i2c_driver_install(bus, I2C_MODE_MASTER, 0, 0, 0);
}

/*!
 * @brief User function to implement I2C write, compliant to Bosch BME280 API
 *
 * @param[in] reg_addr      : Register address to which the data is written.
 * @param[in] reg_data     : Pointer to data buffer in which data to be written
 *                            is stored.
 * @param[in] len           : Number of bytes of data to be written.
 * @param[in, out] intf_ptr : Void pointer that can enable the linking of descriptors
 *                            for interface related call backs
							The parameter intf_ptr can be used as a variable to store the I2C address of the device
 *
 * @retval   0   -> Success.
 * @retval Non zero value -> Fail.
 */
static BME280_INTF_RET_TYPE BME280_I2C_bus_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
	esp_err_t espRc;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (*(reinterpret_cast<uint8_t*>(intf_ptr)) << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, reg_addr, true);
	i2c_master_write(cmd, const_cast<uint8_t*>(reg_data), len, true);
	i2c_master_stop(cmd);
	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	return (espRc == ESP_OK)? BME280_INTF_RET_SUCCESS : !BME280_INTF_RET_SUCCESS;
}

/*!
 * @brief User function to implement I2C read, compliant to Bosch BME280 API
 *
 * @param[in] reg_addr       : Register address from which data is read.
 * @param[out] reg_data     : Pointer to data buffer where read data is stored.
 * @param[in] len            : Number of bytes of data to be read.
 * @param[in, out] intf_ptr  : Void pointer that can enable the linking of descriptors
 *                                  for interface related call backs.
 *	The parameter intf_ptr can be used as a variable to store the I2C address of the device
 *
 * @retval   0 -> Success.
 * @retval Non zero value -> Fail.
 *
 */
static BME280_INTF_RET_TYPE BME280_I2C_bus_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
	esp_err_t espRc;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (*(reinterpret_cast<uint8_t*>(intf_ptr)) << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, reg_addr, true);

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (*(reinterpret_cast<uint8_t*>(intf_ptr)) << 1) | I2C_MASTER_READ, true);

	if (len > 1) {
		i2c_master_read(cmd, reg_data, len-1, I2C_MASTER_ACK);
	}
	i2c_master_read_byte(cmd, reg_data+len-1, I2C_MASTER_NACK);
	i2c_master_stop(cmd);

	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);

	i2c_cmd_link_delete(cmd);

	return (espRc == ESP_OK)? BME280_INTF_RET_SUCCESS : !BME280_INTF_RET_SUCCESS;
}

/*!
 * @brief Delay function pointer which should be mapped to
 * delay function of the user
 *
 * @param[in] period              : Delay in microseconds.
 * @param[in, out] intf_ptr       : Void pointer that can enable the linking of descriptors
 *                                  for interface related call backs
 *
 */
void bme280_delay_us(uint32_t period, void *intf_ptr){
//	delayMicroseconds(period);

	    uint64_t m = (uint64_t)esp_timer_get_time();
	    if(period){
	        uint64_t e = (m + period);
	        if(m > e){ //overflow
	            while((uint64_t)esp_timer_get_time() > e){
	                NOP();
	            }
	        }
	        while((uint64_t)esp_timer_get_time() < e){
	            NOP();
	        }
	    }
}

PTHsensorWrapper::PTHsensorWrapper() {
	sht3x = NULL;
	req_delay = 0;
}

bool PTHsensorWrapper::init(){
#ifdef USE_PTH_V_PIN
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask =  (1ULL<<PTH_V_PIN);
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	//configure GPIO with the given settings
	if (ESP_OK != gpio_config(&io_conf)){
		ESP_LOGW(TAG, "PTHsensorWrapper::init: failed gpio_config on PTH_V_PIN");
		return false;
	}
	gpio_set_drive_capability(PTH_V_PIN, GPIO_DRIVE_CAP_3);
	gpio_set_level(PTH_V_PIN, 1);
//	https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf
//	When configured as RTC GPIOs, the output pads can still retain the output level value when the chip is in Deepsleep mode, and the input pads can wake up the chip from Deep-sleep.
//	gpio_hold_en(PTH_V_PIN);
//	gpio_deep_sleep_hold_en(void);
//	gpio_sleep_set_pull_mode
//	gpio_sleep_sel_en
//	rtc_gpio_init
//	rtc_gpio_set_level
//	rtc_gpio_set_direction_in_sleep
//	rtc_gpio_set_drive_capability
//	rtc_gpio_hold_en
//
#endif

	switch (type) {
	case pthType_t::BME280:{
#ifdef USE_BME
		ESP_LOGI(TAG, "Initializing BME280...");

		i2c_master_init(I2C_BUS, I2C_SCL_PIN, I2C_SDA_PIN, BME_I2C_SPEED);
		int8_t rslt = BME280_OK;
		dev_addr = BME280_I2C_ADDR_PRIM;

		bme_dev.intf_ptr = &dev_addr;
		bme_dev.intf = BME280_I2C_INTF;
		bme_dev.read = BME280_I2C_bus_read;
		bme_dev.write = BME280_I2C_bus_write;
		bme_dev.delay_us = bme280_delay_us;

		rslt = bme280_init(&bme_dev);
		if (rslt != BME280_OK)
			ESP_LOGE(TAG, "Failed initializing BME280.");

		uint8_t settings_sel;

		bme_dev.settings.osr_h = BME280_OVERSAMPLING_16X;
		bme_dev.settings.osr_p = BME280_OVERSAMPLING_16X;
		bme_dev.settings.osr_t = BME280_OVERSAMPLING_16X;
		bme_dev.settings.filter = BME280_FILTER_COEFF_OFF;

		settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

		rslt = bme280_set_sensor_settings(settings_sel, &bme_dev);
		if (rslt != BME280_OK)
			ESP_LOGE(TAG, "Failed in BME280 settings.");

		/*Calculate the minimum delay required between consecutive measurement based upon the sensor enabled
		 *  and the oversampling configuration. */
		req_delay = bme280_cal_meas_delay(&bme_dev.settings);

		if (rslt != 0){
			ESP_LOGE(TAG, "bme initialization failed, please check your address or the wire you connected");
			return false;
		}
		ESP_LOGI(TAG, "success. Fastest read period is %d ms", req_delay);
		vTaskDelay(pdMS_TO_TICKS(10));
#else
		ESP_LOGE(TAG, "BME280 support not compiled");
		return false;
#endif
	} break;
	case pthType_t::SHT:{
#ifdef USE_SHT
		ESP_LOGI(TAG, "Initializing I2C for SHT3x...");
		i2c_init(I2C_BUS, I2C_SCL_PIN, I2C_SDA_PIN, I2C_FREQ_100K);
		ESP_LOGI(TAG, "Initializing SHT3x...");
		sht3x = sht3x_init_sensor(I2C_BUS, SHT3x_ADDR_1);
		if (sht3x == NULL) {
			ESP_LOGE(TAG, "sht3x_init_sensor failed, please check your address or the wire you connected");
			return false;
		}
		ESP_LOGI(TAG, "sht3x_init_sensor done");
#else
		ESP_LOGE(TAG, "SHT support not compiled");
		return false;
#endif
	}break;
	case pthType_t::DHT:{
#ifdef USE_DHT
		dht.begin();
#else
		ESP_LOGE(TAG, "SHT support not compiled");
		return false;
#endif
	}break;
	default:{
		ESP_LOGE(TAG, "selected PTH sensor was not enabled in compilation");
		return false;
	}break;
	}
	return true;
}

void PTHsensorWrapper::hold_up_when_sleep(bool enable) {
#ifdef USE_PTH_V_PIN
	(enable) ? rtc_gpio_hold_en(PTH_V_PIN) : rtc_gpio_hold_dis(PTH_V_PIN);
#endif
}

bool PTHsensorWrapper::measure(void){
	switch (type) {
	case pthType_t::BME280:
#ifdef USE_BME
	{
		//vTaskDelay(pdMS_TO_TICKS(100));
		int8_t rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, &bme_dev);	//this looks like really starts the measurement!
		vTaskDelay(pdMS_TO_TICKS(req_delay + 10));
		//esp_timer_get_time()
		//while (micros() - ts < 100*req_delay);
		struct bme280_data comp_data;
		rslt += bme280_get_sensor_data(BME280_ALL, &comp_data, &bme_dev);
		if (rslt == 0){
			temperature = comp_data.temperature/100;
			humidity = comp_data.humidity/1000;
			pressure = comp_data.pressure;
		}else{
			ESP_LOGE(TAG, "measure error. code: %d", rslt);
			return false;
		}
		return true;
	}
#else
		ESP_LOGE(TAG, "BME support not compiled");
		return false;
#endif
	break;
	case pthType_t::SHT:
#ifdef USE_SHT
	{
		float temperature_sht;
		float humidity_sht;
		if(sht3x_measure (sht3x, &temperature_sht, &humidity_sht)){
			humidity = (int)humidity_sht;
			temperature = (int)temperature_sht;
			pressure = (int)PTH_PRESSURE_DEFAULT;
			return true;
		}
		else{
			ESP_LOGW(TAG, "sht3x_measure failed.");
			if (sht3x->error_code & SHT3x_MEAS_NOT_STARTED)
				ESP_LOGW(TAG, "SHT3x_MEAS_NOT_STARTED");
			if (sht3x->error_code & SHT3x_MEAS_ALREADY_RUNNING)
				ESP_LOGW(TAG, "SHT3x_MEAS_ALREADY_RUNNING");
			if (sht3x->error_code & SHT3x_MEAS_STILL_RUNNING)
				ESP_LOGW(TAG, "SHT3x_MEAS_STILL_RUNNING");
			if (sht3x->error_code & SHT3x_READ_RAW_DATA_FAILED)
				ESP_LOGW(TAG, "SHT3x_READ_RAW_DATA_FAILED");

			if (sht3x->error_code & SHT3x_SEND_FETCH_CMD_FAILED)
				ESP_LOGW(TAG, "SHT3x_SEND_FETCH_CMD_FAILED");
			if (sht3x->error_code & SHT3x_SEND_MEAS_CMD_FAILED)
				ESP_LOGW(TAG, "SHT3x_SEND_MEAS_CMD_FAILED");
			if (sht3x->error_code & SHT3x_SEND_RESET_CMD_FAILED)
				ESP_LOGW(TAG, "SHT3x_SEND_RESET_CMD_FAILED");
			if (sht3x->error_code & SHT3x_SEND_STATUS_CMD_FAILED)
				ESP_LOGW(TAG, "SHT3x_SEND_STATUS_CMD_FAILED");

			if (sht3x->error_code & SHT3x_WRONG_CRC_HUMIDITY)
				ESP_LOGW(TAG, "SHT3x_WRONG_CRC_HUMIDITY");
			if (sht3x->error_code & SHT3x_WRONG_CRC_TEMPERATURE)
				ESP_LOGW(TAG, "SHT3x_WRONG_CRC_TEMPERATURE");

			if (sht3x->error_code & SHT3x_I2C_BUSY)
				ESP_LOGW(TAG, "SHT3x_I2C_BUSY");
			if (sht3x->error_code & SHT3x_I2C_READ_FAILED)
				ESP_LOGW(TAG, "SHT3x_I2C_READ_FAILED");
			if (sht3x->error_code & SHT3x_I2C_SEND_CMD_FAILED)
				ESP_LOGW(TAG, "SHT3x_I2C_SEND_CMD_FAILED");
			return false;
		}
	}
#else
		ESP_LOGE(TAG, "SHT support not compiled");
		return false;
#endif
		break;
	case pthType_t::DHT:
#ifdef USE_DHT
	{
		humidity = (int)dht.readHumidity();
		temperature = (int)dht.readTemperature();
		pressure = (int)PTH_PRESSURE_DEFAULT;
		return true;
	}
#else
		ESP_LOGE(TAG, "DHT support not compiled");
		return false;
#endif
	}
	return true;
}

PTHsensorWrapper pthWrapper;

