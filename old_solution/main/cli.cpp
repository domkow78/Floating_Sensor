#include <SimpleCLI.h>
#include "EEPROM.h"
#include "config.h"
#include "cli.h"
//#include <PubSubClient.h>
#include "mqtt_client.h"
#include "mqttMsg.h"
#include "main.h"
#include "acmm.h"
#include "PTHsensorWrapper.h"
#include "freertos/task.h"

static const char* TAG = "CLI";
#include "esp_log.h"

void cli::test(cmd* commandPointer){
	ESP_LOGI(TAG, "Pretend to receive endless task...");
	msgProc.enqueue("ws/sensorID1/rpi1/measFreq", "{\"freq\": 2}");
	msgProc.enqueue("ws/sensorID1/rpi1/startMeas", "{\"count\": -1}");
}

void cli::mqttsendCbk(cmd* commandPointer){
	Command cmd(commandPointer); // Create wrapper class instance for the pointer
//	mqttClient.publish(
//			(const char*)cmd.getArg("topic").getValue().c_str(),
//			(const char*)cmd.getArg("payload").getValue().c_str());
	esp_mqtt_client_publish(
			mqttClient,
			(const char*)cmd.getArg("topic").getValue().c_str(),
			(const char*)cmd.getArg("payload").getValue().c_str(),
			0, 0, 0);
}

void cli::mqttrecCbk(cmd* commandPointer){
	Command cmd(commandPointer); // Create wrapper class instance for the pointer
	msgProc.enqueue(
			cmd.getArg("topic").getValue().c_str(),
			cmd.getArg("payload").getValue().c_str()
			);
}

void cli::reset(cmd* commandPointer){
	esp_restart();
}

//void cli::rtos(cmd* commandPointer){
//	char pcWriteBuffer[40*10];
////	vTaskGetRunTimeStats(pcWriteBuffer);
//	Serial.print(pcWriteBuffer);
//}

void cli::setMyIDCbk(cmd* commandPointer) {
	Command cmd(commandPointer); // Create wrapper class instance for the pointer
	if (!cmd.getArg().isSet())
		ESP_LOGI(TAG, "MyID is %s\n", myID);
	else{
		if (strcmp(cmd.getArg().getValue().c_str(),"hwid()")==0){
		    uint64_t _chipmacid = 0LL;
		    esp_efuse_mac_get_default((uint8_t*) (&_chipmacid));
			itoa(_chipmacid, myID, 16);
		}else
			strcpy(myID, cmd.getArg().getValue().c_str());
		ESP_LOGI(TAG, "MyID is set to %s\n", myID);
	}
}

void cli::setSsidCbk(cmd* commandPointer) {
	Command cmd(commandPointer); // Create wrapper class instance for the pointer
	if (!cmd.getArg().isSet())
		ESP_LOGI(TAG, "SSID is %s\n", wifiSsid);
	else{
		ESP_LOGI(TAG, "Setting SSID to %s\n", cmd.getArg().getValue().c_str());
		strcpy(wifiSsid, cmd.getArg().getValue().c_str());
	}
}

void cli::setPassCbk(cmd* commandPointer) {
	Command cmd(commandPointer); // Create wrapper class instance for the pointer
	if (!cmd.getArg().isSet())
		ESP_LOGI(TAG, "You can only set password, not print it.\n");
	else{
		ESP_LOGI(TAG, "Setting pass to %s\n", cmd.getArg().getValue().c_str());
		strcpy(wifiPass, cmd.getArg().getValue().c_str());
	}
}

void cli::connectCbk(cmd* commandPointer) {
//	wifiReConnectA(TAG, wifiSsid, wifiPass);
}

void cli::setHubCbk(cmd* commandPointer) {
	Command cmd(commandPointer); // Create wrapper class instance for the pointer
	if (!cmd.getArg().isSet())
		ESP_LOGI(TAG, "MQTT hub is: %s\n",mqttServ);
	else{
		ESP_LOGI(TAG, "Seeting MQTT hub to %s\n", cmd.getArg().getValue().c_str());
		strcpy(mqttServ, cmd.getArg().getValue().c_str());
	}
}

void cli::storeCbk(cmd* commandPointer) {
	ESP_LOGI(TAG, "Storing data to Flash...\n");
	EEPROM.begin(EEPROM_SIZE);
	char* p;
	for (p = wifiPass; true; p++){
		EEPROM.write(EEPROM_OFFS_PASS_32B + p-wifiPass,*p);
		if (*p=='\0')
			break;
	}
	for (p = wifiSsid; true; p++){
		EEPROM.write(EEPROM_OFFS_SSID_32B + p-wifiSsid,*p);
		if (*p=='\0')
			break;
	}
	for (p = mqttServ; true; p++){
		EEPROM.write(EEPROM_OFFS_MQTT_32B + p-mqttServ,*p);
		if (*p=='\0')
			break;
	}
	for (p = myID; true; p++){
		EEPROM.write(EEPROM_OFFS_MYID_32B + p-myID,*p);
		if (*p=='\0')
			break;
	}

	EEPROM.write(EEPROM_OFFS_PTH_1B, (uint8_t)(pthWrapper.type));
	EEPROM.write(EEPROM_OFFS_ACMMINTERLOCK_1B, (uint8_t)(acmm.interlockMode));
	EEPROM.write(EEPROM_OFFS_ACMMPULLUP_1B, (uint8_t)(acmm.selfPullupEn));

	EEPROM.write(EEPROM_OFFS_FLAG_1B,0x5A);
	EEPROM.commit();
	EEPROM.end();
}

void cli::h1up(cmd* commandPointer) {
	acmm.setH(acmmHbridgeState_t::H1pup);
}

void cli::h2up(cmd* commandPointer) {
	acmm.setH(acmmHbridgeState_t::H2pup);
}

void cli::hiz(cmd* commandPointer) {
	acmm.setH(acmmHbridgeState_t::Hiz);
}

void cli::selfPullupEn(cmd* commandPointer) {
	Command cmd(commandPointer); // Create wrapper class instance for the pointer
	if (!cmd.getArg().isSet()){
		ESP_LOGI(TAG, "%s", (acmm.selfPullupEn)? "selfPullup on\n" : "selfPullup off\n");
		ESP_LOGI(TAG, "Use on/off to change\n");
	}
	else{
		if (strcmp(cmd.getArg().getValue().c_str(),"on")==0){
			acmm.selfPullupEn = true;
			acmm.init(true);
		}else{
			acmm.selfPullupEn = false;
			acmm.init(true);
		}
	}
}

void cli::interlockMode(cmd* commandPointer) {
	Command cmd(commandPointer); // Create wrapper class instance for the pointer
	if (!cmd.getArg().isSet()){
		if (acmm.interlockMode != acmmInterlockMode_t::off){
			if (acmm.interlockMode == acmmInterlockMode_t::gnd)
				ESP_LOGI(TAG, "interlockMode gnd\n");
			else
				ESP_LOGI(TAG, "interlockMode hiz\n");
		} else
			ESP_LOGI(TAG, "interlockMode off\n");
		ESP_LOGI(TAG, "Use gnd/hiz/off to change\n");
	}
	else{
		if (strcmp(cmd.getArg().getValue().c_str(),"gnd")==0){
			acmm.interlockMode = acmmInterlockMode_t::gnd;
		}else if (strcmp(cmd.getArg().getValue().c_str(),"hiz")==0){
			acmm.interlockMode = acmmInterlockMode_t::hiz;
		} else
			acmm.interlockMode = acmmInterlockMode_t::off;
	}
}

void cli::adc(cmd* commandPointer) {
//	ESP_LOGI(TAG, "ADC sample: %d\n", analogRead(ACMM_INPIN));
}

void cli::acmmspeed(cmd* commandPointer) {
	ESP_LOGI(TAG, "setup : acmmSpeedCheck = %d\n", acmm.speedCheck());
}

void cli::autoLightSleep(cmd* commandPointer) {
	Command cmd(commandPointer); // Create wrapper class instance for the pointer
	if (!cmd.getArg().isSet())
		ESP_LOGI(TAG, "Argument [on/off] missing.");
	else{
		if (strcmp(cmd.getArg().getValue().c_str(),"on")==0){
			;
		}else if (strcmp(cmd.getArg().getValue().c_str(),"off")==0){
			;
		}else
		ESP_LOGI(TAG, "Invalid argument, expected [on/off]");
	}
}

void cli::mqttLogLevel(cmd* commandPointer) {
	Command cmd(commandPointer); // Create wrapper class instance for the pointer
	if (!cmd.getArg().isSet())
		switch (logLevelToSendViaMqtt){
		case ESP_LOG_INFO:
			ESP_LOGI(TAG, "info");
			break;
		case ESP_LOG_WARN:
			ESP_LOGI(TAG, "warn");
			break;
		case ESP_LOG_ERROR:
			ESP_LOGI(TAG, "error");
			break;
		default:
			ESP_LOGI(TAG, "unknown");
		}
	else{
		if (strcmp(cmd.getArg().getValue().c_str(),"warn")==0){
			logLevelToSendViaMqtt = ESP_LOG_WARN;
		}else if (strcmp(cmd.getArg().getValue().c_str(),"error")==0){
			logLevelToSendViaMqtt = ESP_LOG_ERROR;
		}else if (strcmp(cmd.getArg().getValue().c_str(),"info")==0){
			logLevelToSendViaMqtt = ESP_LOG_INFO;
		}else
		ESP_LOGI(TAG, "Invalid argument, expected [info/warn/error]");
	}
}

void cli::pth(cmd* commandPointer) {
	Command cmd(commandPointer); // Create wrapper class instance for the pointer
	if (!cmd.getArg().isSet()){
		switch (pthWrapper.type){
		case pthType_t::BME280:
			ESP_LOGI(TAG, "PTH is BME280");
			break;
		case pthType_t::SHT:
			ESP_LOGI(TAG, "PTH is SHT");
			break;
		case pthType_t::DHT:
			ESP_LOGI(TAG, "PTH is DHT");
			break;
		default:
			break;
		}
	}
	else{
		if (strcmp(cmd.getArg().getValue().c_str(),"bme")==0){
			pthWrapper.type = pthType_t::BME280;
			ESP_LOGI(TAG, "PTH set to BME280");
		}else if (strcmp(cmd.getArg().getValue().c_str(),"sht")==0){
			pthWrapper.type = pthType_t::SHT;
			ESP_LOGI(TAG, "PTH set to SHT");
		}else if (strcmp(cmd.getArg().getValue().c_str(),"dht")==0){
			pthWrapper.type = pthType_t::DHT;
			ESP_LOGI(TAG, "PTH set to DHT");
		}else
			ESP_LOGI(TAG, "Invalid argument, expected [bme/sht/dht]");
	}
}

