#ifndef MAIN_H
#define MAIN_H

/*
Include this file in all non-main-modules to be able to refer to variables defined in main.cpp
*/

//#include <TaskSchedulerDeclarations.h>
//#include <PubSubClient.h> //mqtt
#include "mqtt_client.h"
#include "esp_log.h"
#include "msgProcessor.h"
#include "wifiaaa.h"
#include "esp_pm.h"

#ifdef USE_DHT
  #include "DHT.h"
#endif
#ifdef USE_SHT
  #include <sht3x.h>
#endif


#define JOB_STATE_NONE					0xFF
#define JOB_STATE_UNFINISHED			0x00
#define JOB_STATE_FINISHED_MARKED		JOB_STATE_NONE
#define JOB_STATE_FINISHED_UNMARKED		0xFE


extern uint16_t imp;
extern uint16_t impFilt;
extern uint16_t phase;
extern bool booting;
extern esp_pm_lock_handle_t pmLock;

//extern int acceleration;
//extern int battery;
extern float moisture;
extern bool sendAck;
extern uint8_t jobState;

extern esp_mqtt_client_handle_t mqttClient;
//extern Scheduler runner;
extern TaskHandle_t taskMeasure;
extern MsgProcessor msgProc;
extern char hubID[16];

#define wifiSsid ssidpass[0].ssid
#define wifiPass ssidpass[0].pass

extern char mqttServ[23];
extern char myID[16];

extern esp_log_level_t logLevelToSendViaMqtt;

#ifdef USE_DHT
  extern DHT dht;
#endif
#ifdef USE_SHT
  extern sht3x_sensor_t* sht3x;
#endif

typedef uint8_t byte;

//extern int wifiReConnectA(const char* tag, const char* ssid, const char* pass, unsigned int driverWaitMs = 15000);
extern bool enableWakePin(const char* tag);
extern void setSleepOnStill(int val);
extern int getSleepOnStill(void);
extern void powerSaveMode(bool on);

#endif
