
/* Hardware configuration and other compilation switches
need to be specified globally, i.e. via -D compiler switch in compilation environment.
like these:

  -DVERBOSE_THR=0
	-DDEBUG=1
	-DUSE_BME //use this...
  -DUSE_DHT // ...xor this, never both
	-D_TASK_SLEEP_ON_IDLE_RUN
	-DDEFERRED_SEND

  */

/********************************************************************/
/************************* INCLUDES *********************************/
/********************************************************************/
//#include <Arduino.h>

//#define ARDUINO nonsense

#include <SimpleCLI.h>
#include "EEPROM.h"
//#include <WiFi.h>
//#include <PubSubClient.h> //mqtt
#include "config.h"
#include "msgProcessor.h"
#include "taskMeasure.h"
#include "acmm.h"
#include "logit.h"
#include "cli.h"
#include "utils.h"
#include "sleep.h"
#ifdef DEFERRED_SEND
  #include "msgSender.h"    
#endif
//#include "serialEncap.h"
#include "esp_pm.h"
//#include "esp_app_format.h"
#include "esp_task_wdt.h"
#include "blink.h"
#include "battMeas.h"
#include "esp_intr_alloc.h"
#include "otaUpgrade.h"
#include "timeespt.h"
//#include "serialEncap.h"
#include "esp_wifi.h"
#include "driver/uart.h"
#include "esp_spiffs.h"
#include "fileLoger.h"
#include "PTHsensorWrapper.h"
#include "tp.h"
#include "main.h"
#include "wifiScan.h"
#include "mqtt_client.h"
#include "wifiaaa.h"
#include "phaseMeas.h"
#include "esp_private/pm_impl.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "esp_mac.h"

/*The function esp_log_level_set() cannot set logging levels higher than specified by CONFIG_LOG_DEFAULT_LEVEL.
 * To increase log level for a specific file at compile time, use the macro LOG_LOCAL_LEVEL before include esp_log.h
*/
//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"


/******************************************************************************************/
/*********************** declare and define local variables ******************************/
/******************************************************************************************/

static const char* TAG_LOOP = "MAIN_LOOP";
static const char* TAG_SETUP = "SETUP";
static const char* TAG_MQTT = "MQTT";

static SimpleCLI scli;
static char subscribeTopic[32] = DEF_SUB_TOPIC;
//static WiFiClient wlanClient;

static vprintf_like_t logFuncOriginal;

#define SERIAL_BAUD					115200
#define SERIAL_PORT					UART_NUM_0
#define CLI_ENTER_TIMEOUT_MS				5000
#define CLI_LEAVE_TIMEOUT_MS				60*1000

#define MAIN_LOOP_DELAY_MS						500
#define MAIN_LOOP_DELAY_TICKS					pdMS_TO_TICKS(MAIN_LOOP_DELAY_MS)
#define DEEP_SLEEP_ON_STILL_TIMEOUT_MS			(4*60*60*1000)
#define MS_2_MAIN_LOOPS(ms)						(ms / MAIN_LOOP_DELAY_MS)
#define WAKE_PIN								GPIO_NUM_2
/*Only GPIOs which are have RTC functionality can be used: 0,2,4,12-15,25-27,32-39.*/
#define DEEP_SLEEP_TIME_MS			(10*60*1000)
static uint32_t deepSleepRemainLoops = MS_2_MAIN_LOOPS(DEEP_SLEEP_ON_STILL_TIMEOUT_MS);
static RTC_DATA_ATTR uint32_t sleepOnStill = DEEP_SLEEP_ON_STILL_TIMEOUT_MS;

#define WIFI_CONNECT_TRIALS_ON_BOOT		10
#define WIFI_CONNECT_TRIALS_DELAY_MS	5000

#define MQTT_CONNECT_TRIALS_ON_BOOT		5
#define MQTT_CONNECT_TRIALS_DELAY_MS	5000

#define BATTERY_CHECK_PERIOD_MS		2*60*1000
#define BATTERY_CHECK_LOOP_CNT		MS_2_MAIN_LOOPS(BATTERY_CHECK_PERIOD_MS)
static uint32_t batteryCheckLoopCnt;

#define MIN_HEAP_WARN_LEVEL	1024

#define LOG_FILENAME	"/spiffs/log.txt"

#define CONSOLE_BUF_LEN 32
static char consoleBuf[CONSOLE_BUF_LEN+1];
#define ANSI_COLOR_CODES_LEN 7

#define SHAKE_SMOOTH_BITS 5

/******************************************************************************************/
/*********************** declare and define global variables ******************************/
/******************************************************************************************/

//PubSubClient mqttClient(wlanClient);
esp_mqtt_client_handle_t mqttClient;

TaskHandle_t taskMeasure;
MsgProcessor msgProc;

RTC_DATA_ATTR uint16_t imp;
RTC_DATA_ATTR float moisture;

bool preventLightSleep = false;
TickType_t lastShakeTsMs;
uint32_t avgShakeMs = 0;

bool booting;
uint8_t jobState;
bool connectedInSetup = false;
bool sendAck = false;

char hubID[16] = "rpi1";
//const char* publishTopic = DEF_PUB_TOPIC;

char mqttServ[23] = MQTT_SERV;
char myID[16] = MYID;

esp_log_level_t logLevelToSendViaMqtt = ESP_LOG_WARN;

esp_pm_lock_handle_t pmLock;
#define LOG_BUFFER_LEN	100
char buffer[LOG_BUFFER_LEN];


/******************************************************************************************/
/*********************** function bodies **************************************************/
/******************************************************************************************/

  /*
   * void setSleepOnStill(int val)
   * Inputs:
  */
void setSleepOnStill(int val) {
	sleepOnStill = val;

	EEPROM.begin(EEPROM_SIZE);
	EEPROM.writeUInt(EEPROM_OFFS_SLEEP_ON_STILL_4B, sleepOnStill);
	EEPROM.commit();
	EEPROM.end();

	deepSleepRemainLoops = MS_2_MAIN_LOOPS(sleepOnStill);
}

int getSleepOnStill(void) {
	return sleepOnStill;
}

/*
 * static void IRAM_ATTR gpio_isr_handler(void* arg)
 * Inputs: void* arg - ignored here
 * Outputs:
 * Shake sensor should fire this and reset the countdown-to-deep-sleep,
 * also, if movement detected, request the blinking task to suspend, to save energy
 */
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    //uint32_t gpio_num = (uint32_t) arg;
	deepSleepRemainLoops = MS_2_MAIN_LOOPS(sleepOnStill);

	avgShakeMs = ((avgShakeMs << SHAKE_SMOOTH_BITS)  + (xTaskGetTickCount() - lastShakeTsMs)) >> SHAKE_SMOOTH_BITS;
	lastShakeTsMs = xTaskGetTickCount();

	blink.requestSuspend();
}

/* Enable wake-up from deep sleep by WAKE_PIN */
bool enableWakePin(const char* tag){
	if (ESP_OK == esp_sleep_enable_ext0_wakeup(WAKE_PIN, !gpio_get_level(WAKE_PIN))){
			ESP_LOGI(tag, "esp_sleep_enable_ext0_wakeup on WAKE_PIN ok");
			return true;
	}else{
			ESP_LOGE(tag, "esp_sleep_enable_ext0_wakeup on WAKE_PIN failed");
			return false;
	}
}


int vprintfWithPmLock(const char * format, va_list arg ){
	esp_pm_lock_acquire(pmLock);
	preventLightSleep = true;
	int cnt = vsnprintf(buffer, sizeof(buffer), format, arg);
	esp_log_level_t msgLevel = ESP_LOG_VERBOSE;

	switch (buffer[ANSI_COLOR_CODES_LEN]){
		case 'V': msgLevel = ESP_LOG_VERBOSE; break;
		case 'D': msgLevel = ESP_LOG_DEBUG; break;
		case 'I': msgLevel = ESP_LOG_INFO; break;
		case 'W': msgLevel = ESP_LOG_WARN; break;
		case 'E': msgLevel = ESP_LOG_ERROR; break;
		default: msgLevel = ESP_LOG_VERBOSE;
	}
	if (msgLevel <= logLevelToSendViaMqtt)
		printf(">>..");
//		uart_write_bytes(SERIAL_PORT, ">>..", 5);
	buffer[LOG_BUFFER_LEN-1] = 0;	//double check that it ends
	buffer[LOG_BUFFER_LEN-2] = 10;	//double check LF

	printf(buffer);
//	uart_write_bytes(SERIAL_PORT, buffer, sizeof(buffer)-4);
//	uart_wait_tx_done(SERIAL_PORT, pdMS_TO_TICKS(1000));

	if (cnt>LOG_BUFFER_LEN)
		printf("...(log trunc)\n");
//		uart_write_bytes(SERIAL_PORT, "...(log msg truncated)", 23);

	if (msgLevel <= logLevelToSendViaMqtt){
		mqttMsg aMsg(TOPIC_DOMAIN, hubID, myID, TOPIC_LOG, buffer);
		msgSender.post(aMsg);

		fileLoger.write(buffer);
	}

	esp_pm_lock_release(pmLock);
	preventLightSleep = false;
	return ((cnt > LOG_BUFFER_LEN) ? LOG_BUFFER_LEN : cnt);
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG_SETUP, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)(event_data);
    esp_mqtt_client_handle_t client = event->client;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_SETUP, "MQTT_EVENT_CONNECTED");
		strcpy(subscribeTopic, DEF_SUB_TOPIC);
		esp_mqtt_client_subscribe(client, subscribeTopic, 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_SETUP, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG_SETUP, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
//        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
    	ESP_LOGD(TAG_SETUP, "MQTT_EVENT_UNSUBSCRIBED");
        break;
    case MQTT_EVENT_PUBLISHED:
    	ESP_LOGD(TAG_SETUP, "MQTT_EVENT_UNSUBSCRIBED");
        break;
    case MQTT_EVENT_DATA:
//        ESP_LOGI(TAG_LOOP, "MQTT_EVENT_DATA");
    	//looks like topic is not ended by null, but just right after the topic there comes payload
        ESP_LOGI(TAG_MQTT, "eh: TOPIC=%.*s DATA=%.*s", event->topic_len, event->topic, event->data_len, event->data);
//        ESP_LOGI(TAG_MQTT, "TOPIC=%s\r\n", event->topic);
        //payload is also not ended with null
//        ESP_LOGI(TAG_MQTT, "DATA=%.*s", event->data_len, event->data);
//        ESP_LOGI(TAG_MQTT, "DATA=%s\r\n", event->data);

    	if (!msgProc.enqueue(event->topic, event->topic_len, event->data, event->data_len))
    		ESP_LOGW(TAG_MQTT,"!!! Message Queue is full, discarding this message!\n");
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGW(TAG_LOOP, "MQTT_EVENT_ERROR");
//        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        break;
    case MQTT_EVENT_BEFORE_CONNECT:
    	break;
    case MQTT_EVENT_DELETED:
		break;
	default:
        ESP_LOGW(TAG_LOOP, "MQTT unknown event id:%d", event->event_id);
        break;
    }
}

bool preventLightSleepCb(void){
	return preventLightSleep;
}

void powerSaveMode(bool on){
	static_assert(CONFIG_PM_ENABLE == 1, "Enable CONFIG_PM_ENABLE !!!");
	ESP_LOGI(TAG_SETUP, "Configuring PM to %s", (on) ? "on" : "off");
	esp_pm_config_esp32_t pmc;

	if (on){
		pmc.light_sleep_enable = true;
		pmc.max_freq_mhz = CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ;
		pmc.min_freq_mhz = 	CONFIG_ESP32_XTAL_FREQ;
	}else{
		pmc.light_sleep_enable = false;
		pmc.max_freq_mhz = CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ;
		pmc.min_freq_mhz = 	CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ;
	}
	esp_err_t res = esp_pm_configure(&pmc);
	switch (res)
	{
	case ESP_OK:
		ESP_LOGI(TAG_SETUP, "setup: PM configured.");
		break;
	case ESP_ERR_INVALID_ARG:
		ESP_LOGI(TAG_SETUP, "setup: PM not configured, invalid args.");
		break;
	case ESP_ERR_NOT_SUPPORTED:
		ESP_LOGI(TAG_SETUP, "setup: PM not configured, CONFIG_PM_ENABLE is not enabled or what...");
		break;
	}
}

/*
 * void setup() 
 * Inputs: 
 * Outputs:
 * This is invoked by Arduino framework before loop function 
 */
void setup() {
	booting = true;
	deepSleepRemainLoops = MS_2_MAIN_LOOPS(sleepOnStill);
	/*
	 * deepSleepRemainLoops is global static, so is initialized while starting,
	 * sleepOnStill is kept in special memory to survive deepSleep reset
	 * so I have to re-sync deepSleepRemainLoops with info surviving in sleepOnStill
	*/
	//TODO: SSH server !!!

	//+++++++++++++++++++ initialize Serial +++++++++++++++++++++
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
	uart_config_t uart_config = {
	        .baud_rate = SERIAL_BAUD,
	        .data_bits = UART_DATA_8_BITS,
	        .parity    = UART_PARITY_DISABLE,
	        .stop_bits = UART_STOP_BITS_1,
	        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
//	        .source_clk = UART_SCLK_APB,
			.source_clk = UART_SCLK_REF_TICK,
	    };
	    int intr_alloc_flags = 0;
	#pragma GCC diagnostic pop

	#if CONFIG_UART_ISR_IN_IRAM
	    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
	#endif

	ESP_ERROR_CHECK(uart_driver_install(SERIAL_PORT, UART_FIFO_LEN+1, 0, 0, NULL, intr_alloc_flags));
	ESP_ERROR_CHECK(uart_param_config(SERIAL_PORT, &uart_config));
	ESP_ERROR_CHECK(uart_set_pin(SERIAL_PORT, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

//	Serial.begin(SERIAL_BAUD);
	ESP_LOGI(TAG_SETUP,"Serial is working at %d", SERIAL_BAUD);

//	timeEsp.set(1606512934913); //"Sun Nov 29 2020 23:31:02 GMT+0100 (czas œrodkowoeuropejski standardowy)"


//	tp.iir.a1 = -1.9119564625308074;
//	tp.iir.a2 = 0.9287216224372518;
//	tp.iir.b0 = 0.03563918878137419;
//	tp.iir.b1 = 0;
//	tp.iir.b2 = -0.03563918878137419;
//
//	ESP_LOGI(TAG_SETUP, "IIR: %6.3f", tp.iir.tick((float)1000));
//	ESP_LOGI(TAG_SETUP, "IIR: %6.3f", tp.iir.tick((float)0));
//	ESP_LOGI(TAG_SETUP, "IIR: %6.3f", tp.iir.tick((float)1000));
//	ESP_LOGI(TAG_SETUP, "IIR: %6.3f", tp.iir.tick((float)0));
//	ESP_LOGI(TAG_SETUP, "IIR: %6.3f", tp.iir.tick((float)1000));
//	ESP_LOGI(TAG_SETUP, "IIR: %6.3f", tp.iir.tick((float)0));
//	ESP_LOGI(TAG_SETUP, "IIR: %6.3f", tp.iir.tick((float)1000));
//	ESP_LOGI(TAG_SETUP, "IIR: %6.3f", tp.iir.tick((float)0));
//	ESP_LOGI(TAG_SETUP, "IIR: %6.3f", tp.iir.tick((float)1000));
//	ESP_LOGI(TAG_SETUP, "IIR: %6.3f", tp.iir.tick((float)0));
//	ESP_LOGI(TAG_SETUP, "IIR: %6.3f", tp.iir.tick((float)1000));
//	ESP_LOGI(TAG_SETUP, "IIR: %6.3f", tp.iir.tick((float)0));
//	ESP_LOGI(TAG_SETUP, "IIR: %6.3f", tp.iir.tick((float)1000));
//	ESP_LOGI(TAG_SETUP, "IIR: %6.3f", tp.iir.tick((float)0));
//
//	vTaskDelay(pdMS_TO_TICKS(10000));


	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );

//	char datetimestr[40];
//	timeEsp.gets(datetimestr);
//	ESP_LOGI(TAG_SETUP, "\t\ttimeesp test: %llu %s ??????? Fri Nov 27 2020 22:35:34 GMT+0100 ???", timeEsp.getMs(), datetimestr);
//	vTaskDelay(pdMS_TO_TICKS(10*1000));
//	timeEsp.gets(datetimestr);
//	ESP_LOGI(TAG_SETUP, "\t\ttimeesp test: %llu %s ??????? Fri Nov 27 2020 22:35:44 GMT+0100 ???", timeEsp.getMs(), datetimestr);

	//	vTaskDelay(pdMS_TO_TICKS(60*1000));
//	timeEsp.gets(datetimestr);
//	ESP_LOGI(TAG_SETUP, "\t\ttimeesp test: %llu %s ??????? Fri Nov 27 2020 22:36:44 GMT+0100 ???", timeEsp.getMs(), datetimestr);

	//++++++++++++++++++++++++++++++++++++++++++++++++++ initialize Blink +++++++++++++++++++++
	blink.init();

	//++++++++++++++++++++++++++++++++++++++++++++++++++  Check battery +++++++++++++++++++++
	battMeas.init();
	battState_t batState = battMeas.meas();
	ESP_LOGI(TAG_SETUP,"Battery voltage: %d mv", battMeas.raw);
	switch (batState){
	case battState_t::battDead:
		ESP_LOGE(TAG_SETUP, "BATTERY DEAD !!! ENTERING DEEP SLEEP. CHARGE ME, I will wake up in %dsec. and check again.", DEEP_SLEEP_TIME_MS>>10);
		blink.show(0xF0F0F0F0FFFFFFFF, Blink_led::red);
		vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD_MS));
		esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);	//do not wake on shake, to avoid repetitive wake-ups in drum
		esp_deep_sleep(DEEP_SLEEP_TIME_MS << 10);
		break;
	case battState_t::battLow:
		ESP_LOGW(TAG_SETUP, "BATTERY LOW, CHARGE IT ASAP!");
		break;
	default:
		ESP_LOGI(TAG_SETUP, "BATTERY OK.");
	}
	vTaskDelay(pdMS_TO_TICKS(10));

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++ check TWDT state  +++++++++++++++++++++
	bool wdtWasEnabled = (ESP_OK == esp_task_wdt_status(NULL));
	if (wdtWasEnabled){
		/* unsubscribe from Task Watchdog Timer, as some actions may take a while */
		ESP_LOGI(TAG_SETUP, "Task is subscribed to TWDT, I will unsubscribe for some time.\n");
		esp_task_wdt_delete(NULL);
		taskYIELD();
	}
	esp_task_wdt_init(10000, false);

	//	esp_app_desc_t *app_desc;
	//	 = esp_ota_get_app_description();
	//	ESP_LOGI(TAG, "App. version: %s", app_desc->version);
	//	ESP_LOGI(TAG, "Project name: %s", app_desc->project_name);
	//	ESP_LOGI(TAG, "App. time: %s", app_desc->time);
	//	ESP_LOGI(TAG, "App. date: %s", app_desc->date);
	//	ESP_LOGI(TAG, "IDF: %s", app_desc->idf_ver);


	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++  get data from EEPROM +++++++++++++++++++++
	if (EEPROM.begin(EEPROM_SIZE))
		ESP_LOGI(TAG_SETUP, "EEPROM initialised");
	else
		ESP_LOGE(TAG_SETUP, "EEPROM failure");

	jobState = EEPROM.readByte(EEPROM_OFFS_JOB_STATE_1B);

	sleepOnStill = EEPROM.readUInt(EEPROM_OFFS_SLEEP_ON_STILL_4B);
	if (sleepOnStill == 0xFFFFFFFF){
		setSleepOnStill(DEEP_SLEEP_ON_STILL_TIMEOUT_MS);
		ESP_LOGI(TAG_SETUP, "SleepOnStill is %d min. (default hard-coded)", sleepOnStill / 60000);
	}else
		ESP_LOGI(TAG_SETUP, "SleepOnStill is %d min. (from EEPROM)", sleepOnStill / 60000);

	unsigned char useEEdata = EEPROM.read(EEPROM_OFFS_FLAG_1B);
	if (useEEdata == 0x5A){
		ESP_LOGI(TAG_SETUP,"Using CLI-entered settings from EEPROM(emulation):");

		for (char* p = mqttServ; true; p++){
			*p = EEPROM.read(EEPROM_OFFS_MQTT_32B + p-mqttServ);
			if (*p==0x00)
				break;
		}
		for (char* p = ssidpass[0].ssid; true; p++){
			*p = EEPROM.read(EEPROM_OFFS_SSID_32B + p-ssidpass[0].ssid);
			if (*p==0x00)
				break;
		}
		for (char* p = ssidpass[0].pass; true; p++){
			*p = EEPROM.read(EEPROM_OFFS_PASS_32B + p-ssidpass[0].pass);
			if (*p==0x00)
				break;
		}
		for (char* p = myID; true; p++){
			*p = EEPROM.read(EEPROM_OFFS_MYID_32B + p-myID);
			if (*p==0x00)
				break;
		}
		pthWrapper.type = (pthType_t)EEPROM.read(EEPROM_OFFS_PTH_1B);
		acmm.interlockMode = (acmmInterlockMode_t)EEPROM.read(EEPROM_OFFS_ACMMINTERLOCK_1B);
		acmm.selfPullupEn = (bool)EEPROM.read(EEPROM_OFFS_ACMMPULLUP_1B);
	} else
	{
		ESP_LOGI(TAG_SETUP,"Using hard-coded settings. Use CLI to override.");
		uint8_t mac[6];
		memset(mac, 0, sizeof(mac));
		esp_read_mac(mac, ESP_MAC_WIFI_STA);
		memset(myID, 0, sizeof(myID));
		uint8_t nib = 0;
		for (uint8_t iter = 0; iter < 6; ++iter){
			nib = *(myID+iter) >> 4;
			if (nib < 10)
				myID[2*iter+0] = 48 + nib;
			else
				myID[2*iter+0] = 65 + nib - 10;

			nib = *(myID+iter) & 0x0f;
			if (nib < 10)
				myID[2*iter+1] = 48 + nib;
			else
				myID[2*iter+1] = 65 + nib - 10;
		}
	}
	EEPROM.end();

	ESP_LOGI(TAG_SETUP,"MQTT brokr addr: %s", mqttServ);
	ESP_LOGI(TAG_SETUP,"WiFi default SSID: %s", ssidpass[0].ssid);
	ESP_LOGI(TAG_SETUP,"MyID: %s",myID);

	switch (pthWrapper.type){
		case pthType_t::BME280:
			ESP_LOGI(TAG_SETUP, "PTH is BME280");
			break;
		case pthType_t::SHT:
			ESP_LOGI(TAG_SETUP, "PTH is SHT");
			break;
		case pthType_t::DHT:
			ESP_LOGI(TAG_SETUP, "PTH is DHT");
			break;
		default:
			break;
	}

	switch (acmm.interlockMode){
		case acmmInterlockMode_t::gnd:
			ESP_LOGI(TAG_SETUP, "ACMM interlock mode is GND");
			break;
		case acmmInterlockMode_t::hiz:
			ESP_LOGI(TAG_SETUP, "ACMM interlock mode is HiZ");
			break;
		case acmmInterlockMode_t::off:
			ESP_LOGI(TAG_SETUP, "ACMM interlock mode is OFF");
			break;
	}
	ESP_LOGI(TAG_SETUP, "ACMM self pull-up is %s", acmm.selfPullupEn ? "true" : "false");


	//++++++++++++++++++++++++++++++++++++++++++++++++++ add CLI commands +++++++++++++++++++++++++
	scli.addSingleArgCmd("myid",cli::setMyIDCbk);
	scli.addSingleArgCmd("ssid",cli::setSsidCbk);
	scli.addSingleArgCmd("pass",cli::setPassCbk);
	scli.addCmd("connect",cli::connectCbk);
	scli.addSingleArgCmd("hub",cli::setHubCbk);
	scli.addCmd("store",cli::storeCbk);
	scli.addCmd("reset",cli::reset);
	Command cmdMqttrec = scli.addCmd("mqttrec",cli::mqttrecCbk);
	cmdMqttrec.addPosArg("topic");
	cmdMqttrec.addPosArg("payload","");
	Command cmdMqttsend = scli.addCmd("mqttsend",cli::mqttsendCbk);
	cmdMqttrec.addPosArg("topic");
	cmdMqttrec.addPosArg("payload","");
	scli.addCmd("h1up",cli::h1up);
	scli.addCmd("h2up",cli::h2up);
	scli.addCmd("hiz",cli::hiz);
	scli.addCmd("adc",cli::adc);
	scli.addCmd("acmmspeed",cli::acmmspeed);
	scli.addSingleArgCmd("autoLightSleep", cli::autoLightSleep);
	scli.addSingleArgCmd("pth", cli::pth);
	scli.addSingleArgCmd("selfpullupen", cli::selfPullupEn);
	scli.addSingleArgCmd("interlockmode", cli::interlockMode);
//	scli.addCmd("rtos",cli::rtos);
	scli.addCmd("test",cli::test);

	//++++++++++++++++++++++++++++++++++++++++++++++++++ CLI ++++++++++++++++++++++++++++++++++++++
	ESP_LOGI(TAG_SETUP,"To enter boot CLI, type \'cli\' within 3 sec:");
	readUntil(SERIAL_PORT, consoleBuf, CONSOLE_BUF_LEN, 3000);
	if (strcmp(consoleBuf, "cli")==0){
		ESP_LOGI(TAG_SETUP,"CLI mode. Type commands. Type exit or do nothing for 1 min. to exit.");
		do{
			uart_write_bytes(SERIAL_PORT, "\x0A\x0Dws>>",7);
			readUntil(SERIAL_PORT, consoleBuf, CONSOLE_BUF_LEN, 60000);
			if (strcmp(consoleBuf, "exit")==0 || strcmp(consoleBuf, "quit")==0)
				break;
//			uart_write_bytes(SERIAL_PORT, "\x0A\x0Dws!!", 7);
//			uart_write_bytes(SERIAL_PORT, consoleBuf, strlen(consoleBuf));
			scli.parse(strcat(consoleBuf,"\n"));

			if (scli.errored()) {
				CommandError cmdError = scli.getError();

				printf("ERROR: ");
//				printf(cmdError.toString());

				if (cmdError.hasCommand()) {
					printf("Did you mean \"");
//					printf(cmdError.getCommand().toString());
					printf("\"?\n");
				}
			}
		} while (true);
	}


	//++++++++++++++++++++++++++++++++++++++++++++++++++ ACMM ++++++++++++++++++++++++++++++++++++++
	ESP_LOGI(TAG_SETUP, "ACMM init.");
	if (!acmm.init())
		ESP_LOGW(TAG_SETUP, "ACMM init failed!\n");
	vTaskDelay(pdMS_TO_TICKS(10));
	ESP_LOGI(TAG_SETUP, "ACMM run");
	acmm.run();

////	ESP_LOGI(TAG_SETUP, "ACMM SpeedCheck = %d\n", acmm.speedCheck());
////	vTaskDelay(pdMS_TO_TICKS(200));
//	ESP_LOGI(TAG_SETUP, "ACMM run.");
//	acmm.run();
//	acmm.switchSpeedCheck();

	//++++++++++++++++++++++++++++++++++++++++++++++++++ initialize PhaseMeas +++++++++++++++++++++
//	ESP_LOGI(TAG_SETUP, "1) exec phaseMeas init");
//	phaseMeas.init();
//	ESP_LOGI(TAG_SETUP, "phaseMeas sample %d", phaseMeas.measMv());
//
//	ESP_LOGI(TAG_SETUP, "2) exec phaseMeas run");
//	phaseMeas.run(10000, 12000);
//	vTaskDelay(pdMS_TO_TICKS(5000));
//	ESP_LOGI(TAG_SETUP, "phaseMeas sample %d", phaseMeas.measMv());
//
//	ESP_LOGI(TAG_SETUP, "3) exec phaseMeas pause");
//	phaseMeas.pause();
//	vTaskDelay(pdMS_TO_TICKS(5000));
//
//	ESP_LOGI(TAG_SETUP, "4) exec phaseMeas run");
//	phaseMeas.run(20000, 21000);
//	vTaskDelay(pdMS_TO_TICKS(5000));
//	ESP_LOGI(TAG_SETUP, "phaseMeas sample %d", phaseMeas.measMv());
//
//	ESP_LOGI(TAG_SETUP, "5) exec phaseMeas pause");
//	phaseMeas.pause();
//	vTaskDelay(pdMS_TO_TICKS(5000));
//
//	ESP_LOGI(TAG_SETUP, "6) exec phaseMeas resume");
//	phaseMeas.resume();
//	vTaskDelay(pdMS_TO_TICKS(5000));
//
//	ESP_LOGI(TAG_SETUP, "7) exec phaseMeas pause");
//	phaseMeas.pause();
//	vTaskDelay(pdMS_TO_TICKS(5000));
//
//
//	ESP_LOGI(TAG_SETUP, "8) setH H1pup");
//	acmm.activatePup(true);
//	acmm.setH(acmmHbridgeState_t::H1pup);
//	vTaskDelay(pdMS_TO_TICKS(5000));
//
//	ESP_LOGI(TAG_SETUP, "9) setH H2pup");
//	acmm.setH(acmmHbridgeState_t::H2pup);
//	vTaskDelay(pdMS_TO_TICKS(5000));
//
//	ESP_LOGI(TAG_SETUP, "10) setH Hiz");
//	acmm.setH(acmmHbridgeState_t::Hiz);
//	vTaskDelay(pdMS_TO_TICKS(5000));
//
//	ESP_LOGI(TAG_SETUP, "11) phaseMeas resume");
//	acmm.activatePup(false);
//	phaseMeas.resume();
//	vTaskDelay(pdMS_TO_TICKS(5000));
//
//	ESP_LOGI(TAG_SETUP, "12) ACMM sim.");
//	acmm.activatePup(true);
//	phaseMeas.pause();
//
//	for (uint32_t i = 100; i>0; i--){
//		acmm.switchH();
//		vTaskDelay(pdMS_TO_TICKS(30));
//	}
//	acmm.setH(acmmHbridgeState_t::Hiz);
//
//	ESP_LOGI(TAG_SETUP, "13) exec phaseMeas resume");
//	acmm.activatePup(false);
//	phaseMeas.resume();
//	vTaskDelay(pdMS_TO_TICKS(15000));

//	ESP_LOGI(TAG_SETUP, "14) ACMM run");
//	acmm.run();
//	vTaskDelay(pdMS_TO_TICKS(200000));

//	ESP_LOGI(TAG_SETUP, "15) exec phaseMeas resume");
//	acmm.stop();
//	phaseMeas.resume();
//	vTaskDelay(pdMS_TO_TICKS(5000));
//
//	ESP_LOGI(TAG_SETUP, "16) ACMM run");
//	acmm.run();




//
//	tp.init(&imp);
//	tp.start(100);
//
//	vTaskDelay(pdMS_TO_TICKS(60000));
//
//	phaseMeas.init();	//this init seems not to slow down acmm
//	phaseMeas.run(10000, 20000);

	phaseMeas.init();
	phaseMeas.run(1000, 10000);

	wifi_start();

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
//	https://stackoverflow.com/questions/10828294/c-and-c-partial-initialization-of-automatic-structure
	esp_mqtt_client_config_t mqtt_cfg = {
		.uri = mqttServ,
		.disable_auto_reconnect = false,
		.reconnect_timeout_ms = 5000,
	};
	#pragma GCC diagnostic pop
	mqttClient = esp_mqtt_client_init(&mqtt_cfg);
	/* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
	esp_mqtt_client_register_event(mqttClient, (esp_mqtt_event_id_t)MQTT_EVENT_ANY, mqtt_event_handler, NULL);
	esp_mqtt_client_start(mqttClient);

	//++++++++++++++++++++++++++++++++++++++++++++++++++  SENSORS ++++++++++++++++++++++++++++++++++++++
#if defined(USE_DHT) || defined(USE_SHT) || defined(USE_BME)
	if (!pthWrapper.init()){
		ESP_LOGE(TAG_SETUP, "PTH sensor init failed. Will restart.");
		blink.show(BLINK_SENSOR_ERROR, Blink_led::red, true);
		vTaskDelay(pdMS_TO_TICKS(5000));
		esp_restart();
	}
	pthWrapper.hold_up_when_sleep(true); //keep powered up in light/modem sleep, if this feature is enabled internally
	pthWrapper.measure();
	ESP_LOGI(TAG_SETUP, "PTH: %d Pa, %d, %d", pthWrapper.pressure, pthWrapper.temperature, pthWrapper.humidity);
	pthWrapper.measure();
	ESP_LOGI(TAG_SETUP, "PTH: %d Pa, %d, %d", pthWrapper.pressure, pthWrapper.temperature, pthWrapper.humidity);
	pthWrapper.measure();
	ESP_LOGI(TAG_SETUP, "PTH: %d Pa, %d, %d", pthWrapper.pressure, pthWrapper.temperature, pthWrapper.humidity);
#else
	#error Define any of the two compiler optins -DUSE_DHT or -DUSE_BME
#endif

	//++++++++++++++++++++++++++++++++++++++++++++++++++ TASKS ++++++++++++++++++++++++++++++++++++++
	ESP_LOGI(TAG_SETUP, "Tasks...");

	msgSender.init();

	xTaskCreate(&taskMeasureCbk, "taskMeasure", TASK_MEASURE_HEAP, NULL, tskIDLE_PRIORITY, &taskMeasure);

	//	The function esp_log_level_set() cannot set logging levels higher than specified by CONFIG_LOG_MAXIMUM_LEVEL. To increase log level for a specific file above this maximum at compile time, use the macro LOG_LOCAL_LEVEL (see the details below).
	//	esp_log_level_set("taskSend", ESP_LOG_VERBOSE);
	//	esp_log_level_set("taskMeasure", ESP_LOG_VERBOSE);


//	ESP_LOGI(TAG_SETUP, "Test message sender with dummy messages...");
//	char payload[70];
//	sprintf(payload,"{\"temperature\":%3i,\"humidity\":%3i,\"pressure\":%4i,\"acmm\":%3i}", 21, 60, 1013,(int)122);
//	mqttMsg aMsg(TOPIC_DOMAIN, hubID, myID, TOPIC_MEASUREMENT, payload);
//	msgSender.enqueue(aMsg);
//	msgSender.enqueue(aMsg);
//	msgSender.enqueue(aMsg);
//	ESP_LOGI(TAG_SETUP, "\t...enqueued 3 msgs. Done. If DEFERRED_SEND enabled, may fail in a moment...");
//	vTaskDelay(pdMS_TO_TICKS(1000));

//	ESP_LOGI(TAG_SETUP, "Schedule 3 rounds of taskMeasure. Will run async.");
//	xTaskNotify(taskMeasure, 3, eSetValueWithOverwrite);
//	vTaskDelay(pdMS_TO_TICKS(100));


	//++++++++++++++++++++++++++++++++++++++++++++++++++ TP ++++++++++++++++++++++++++++++++++++++
	tp.init(&imp, false);
	uint16_t impRaw;
	tp.measure(&imp, &impRaw);
	ESP_LOGI(TAG_SETUP, "TP: %d, raw: %d", imp, impRaw);
	tp.start(100);

	//++++++++++++++++++++++++++++++++++++++++++++++++++ WDT ++++++++++++++++++++++++++++++++++++++
	if (wdtWasEnabled){
		ESP_LOGI(TAG_SETUP, "Task is subscribed back to TWDT");
		esp_task_wdt_add(NULL);
	}

//++++++++++++++++++++++++++++++++++++++++++++++++++ WAKE PIN ++++++++++++++++++++++++++++++++++++++

	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_NEGEDGE;
	io_conf.pin_bit_mask = (1ULL << WAKE_PIN);
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE ;
	if (ESP_OK == gpio_config(&io_conf))
		ESP_LOGI(TAG_SETUP, "WAKE_PIN config ok");
	else
		ESP_LOGW(TAG_SETUP, "WAKE_PIN config fail");

	esp_err_t res;
	res = gpio_install_isr_service(0);
	switch (res){
		case ESP_ERR_NO_MEM:
			ESP_LOGW(TAG_SETUP, "gpio_install_isr_service: No memory to install this service");
			break;
		case ESP_ERR_INVALID_STATE:
			ESP_LOGW(TAG_SETUP, "gpio_install_isr_service: ISR service already installed");
			break;
		case ESP_ERR_NOT_FOUND:
			ESP_LOGW(TAG_SETUP, "gpio_install_isr_service: No free interrupt found with the specified flags");
			break;
		case ESP_ERR_INVALID_ARG:
			ESP_LOGW(TAG_SETUP, "gpio_install_isr_service: ESP_ERR_INVALID_ARG GPIO error");
			break;
		case ESP_OK:
			ESP_LOGI(TAG_SETUP, "WAKE_PIN ISR install: ok");
			break;
		default:
			ESP_LOGW(TAG_SETUP, "gpio_install_isr_service: unknown error");
	}

	ESP_LOGI(TAG_SETUP, "hook isr handler for WAKE_PIN...");
	res = gpio_isr_handler_add(WAKE_PIN, gpio_isr_handler, (void*) WAKE_PIN);
	switch (res){
		case ESP_ERR_INVALID_STATE:
			ESP_LOGW(TAG_SETUP, "gpio_isr_handler_add: 	Wrong state, the ISR service has not been initialized.");
			break;
		case ESP_ERR_INVALID_ARG:
			ESP_LOGW(TAG_SETUP, "gpio_isr_handler_add: Parameter error");
			break;
		case ESP_OK:
			ESP_LOGI(TAG_SETUP, "WAKE_PIN ISR handler install: ok");
			break;
		default:
			ESP_LOGW(TAG_SETUP, "gpio_isr_handler_add: unknown error");
	}

	enableWakePin(TAG_SETUP);

	/*
      https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/power_management.html
	 */


	//++++++++++++++++++++++++++++++++++++++++++++++++++ PM ++++++++++++++++++++++++++++++++++++++

	static_assert(CONFIG_FREERTOS_USE_TICKLESS_IDLE == 1, "Enable CONFIG_FREERTOS_USE_TICKLESS_IDLE !!!");

	powerSaveMode(false);

	esp_pm_lock_create(esp_pm_lock_type_t(ESP_PM_CPU_FREQ_MAX), 0, "", &pmLock);
//	esp_pm_register_skip_light_sleep_callback(&preventLightSleepCb);

	ESP_LOGI(TAG_SETUP, "Setting WiFi PowerSave mode");
	esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

	vTaskDelay(pdMS_TO_TICKS(50));

	//++++++++++++++++++++++++++++++++++++++++++++++++++ VPRINTF ++++++++++++++++++++++++++++++++++++++

	ESP_LOGI(TAG_SETUP, "Overriding ESP_LOGx vprintf");
	logFuncOriginal = esp_log_set_vprintf(NULL);
	esp_log_set_vprintf(vprintfWithPmLock);

//	vTaskDelay(pdMS_TO_TICKS(4000));

//	ESP_LOGI(TAG_SETUP, "Sample LOGI");
//	vTaskDelay(pdMS_TO_TICKS(4000));
//	ESP_LOGW(TAG_SETUP, "Sample LOGW");
//	vTaskDelay(pdMS_TO_TICKS(400));
//	ESP_LOGE(TAG_SETUP, "Sample LOGE");
//	vTaskDelay(pdMS_TO_TICKS(400));

	if (fileLoger.init() == 0){
		ESP_LOGI(TAG_SETUP, "FileLoger will write Ws and Es to %s", fileLoger.fname);
	}else
		ESP_LOGE(TAG_SETUP, "fileLoger init failed");

	blink.allowSuspend(true);

	//++++++++++++++++++++++++++++++++++++++++++++++++++ RESET REASON ++++++++++++++++++++++++++++++++++++++

	switch (esp_reset_reason()){
	case ESP_RST_UNKNOWN: 	ESP_LOGW(TAG_SETUP, "Reset reason could not be determined"); break;
	case ESP_RST_POWERON: 	ESP_LOGW(TAG_SETUP, "Reset was due to power-on event"); break;
	case ESP_RST_EXT: 		ESP_LOGW(TAG_SETUP, "Reset was by external pin (not applicable for ESP32)"); break;
	case ESP_RST_SW: 		ESP_LOGW(TAG_SETUP, "Reset was via software esp_restart"); break;
	case ESP_RST_PANIC: 	ESP_LOGW(TAG_SETUP, "Reset was due to exception/panic"); break;
	case ESP_RST_INT_WDT: 	ESP_LOGW(TAG_SETUP, "Reset was (software or hardware) due to interrupt watchdog"); break;
	case ESP_RST_TASK_WDT: 	ESP_LOGW(TAG_SETUP, "Reset was due to task watchdog"); break;
	case ESP_RST_WDT: 		ESP_LOGW(TAG_SETUP, "Reset was due to other watchdogs"); break;
	case ESP_RST_DEEPSLEEP:	ESP_LOGW(TAG_SETUP, "Reset was after exiting deep sleep mode"); break;
	case ESP_RST_BROWNOUT:	ESP_LOGW(TAG_SETUP, "Reset was due to brownout (software or hardware)"); break;
	case ESP_RST_SDIO: 		ESP_LOGW(TAG_SETUP, "Reset was over SDIO"); break;
	default: break;
	}

	blink.show(0xF0F0F0F0FFFFFFFF, Blink_led::green, true);
	ESP_LOGW(TAG_SETUP, "############## Setup ENDED ##############\n");
//	vTaskDelay(pdMS_TO_TICKS(400));

//	onMqttReceive((char*)"smartSensor/sensorID1/rpi1/readLog", NULL, 0);
//	vTaskDelay(pdMS_TO_TICKS(1000));

	if (jobState == JOB_STATE_UNFINISHED){
		ESP_LOGI(TAG_SETUP, "Job was unfinished, sending request for orders");
		mqttMsg msgRep(TOPIC_DOMAIN, hubID, myID, TOPIC_UNFINISHED, "foo");
//		mqttClient.publish(msgRep.topic, msgRep.payload);
		esp_mqtt_client_publish(mqttClient, msgRep.topic, (const char*)msgRep.payload, 0, 0, 0);
	}

	booting = false;
}/*setup*/



/*
 * void loop() 
 * Inputs: 
 * Outputs:
 * This is invoked by Arduino framework after setup function 
*/
void loop() {

	TickType_t mainLoopTs = xTaskGetTickCount();

	//++++++++++++++++++++++++++++++++++++++++++ silent CLI in normal run mode ++++++++++++++++++++++++++++++++++++++++
	if (uart_read_bytes(SERIAL_PORT, consoleBuf, CONSOLE_BUF_LEN, 100)>0){
		if ((consoleBuf[0] == '\x01')){	//Ctrl-A to begin talk
			blink.show(BLINK_HEALTHY, Blink_led::green, true);
			if (readUntil(SERIAL_PORT, consoleBuf, CONSOLE_BUF_LEN, 15000)){
				scli.parse(consoleBuf);
			}
		}
	}

	if (sleepOnStill != 0){	//sleepOnStill == 0 means this feature is disabled at all
		deepSleepRemainLoops--;
		if (deepSleepRemainLoops == 0){
			ESP_LOGI(TAG_LOOP, "Configure deep sleep wake-up source");
			/*Only GPIOs which are have RTC functionality can be used: 0,2,4,12-15,25-27,32-39.*/
			if (enableWakePin(TAG_LOOP)){
				vTaskDelay(pdMS_TO_TICKS(100)); //let this be printed...
				deepSleepRemainLoops = MS_2_MAIN_LOOPS(sleepOnStill);
				ESP_LOGW(TAG_LOOP, "Deep Sleep start 88888....");
				vTaskDelay(pdMS_TO_TICKS(1000));
				pthWrapper.hold_up_when_sleep(false); //power down the PTH, if this feature is enabled internally
				esp_deep_sleep_start();
			}
		}
		if ((sleepOnStill) && (deepSleepRemainLoops % (MS_2_MAIN_LOOPS(sleepOnStill) / 10) == 0))
			ESP_LOGI(TAG_LOOP, "deepSleepRemainLoops == %d / 100", 100*deepSleepRemainLoops/ MS_2_MAIN_LOOPS(sleepOnStill));
	}

	batteryCheckLoopCnt++;
	if (batteryCheckLoopCnt >= BATTERY_CHECK_LOOP_CNT){
		batteryCheckLoopCnt = 0;
		battMeas.meas();	//keeps the measured value in battMeas.raw, to be used by ACMM correction
		ESP_LOGI(TAG_LOOP, "Battery: %d mV", battMeas.raw);
	}

	if (blink.isEmpty)
		blink.show(BLINK_HEALTHY, Blink_led::green);

	if (ESP_OK == esp_task_wdt_status(NULL)){ //if I use twdt, reset it
		esp_task_wdt_reset();
	}

	msgProc.execute(); //could also be made a separate task
	//msgProc.flush();
	if (jobState == JOB_STATE_FINISHED_UNMARKED)
		if (msgSender.qlen() == 0){
			jobState = JOB_STATE_FINISHED_MARKED;
			EEPROM.begin(EEPROM_SIZE);
			EEPROM.writeByte(EEPROM_OFFS_JOB_STATE_1B, JOB_STATE_FINISHED_MARKED);
			EEPROM.commit();
			ESP_LOGI(TAG_LOOP, "Job has been finished and marked in NVM.");
		}
	mainLoopTs = xTaskGetTickCount() - mainLoopTs;	//what has elapsed in this loop
	if (MAIN_LOOP_DELAY_TICKS > mainLoopTs)
		vTaskDelay(MAIN_LOOP_DELAY_TICKS - mainLoopTs);
	//heap_caps_get_free_size(MALLOC_CAP_8BIT)
	if (esp_get_minimum_free_heap_size() < MIN_HEAP_WARN_LEVEL)
		ESP_LOGW(TAG_LOOP, "Low on heap!");
}

// xtensa-esp32-elf-addr2line -e c:\Users\jablonskip\eclipse-workspace\lis\build\lis.elf
// esp_get_minimum_free_heap_size(void)

//#ifndef ARDUINO

#ifdef __cplusplus
extern "C" {
#endif
void app_main(){
	setup();
	while (true) loop();
}
#ifdef __cplusplus
}
#endif

//#endif
