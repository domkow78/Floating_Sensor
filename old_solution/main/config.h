#ifndef CONFIG_H
#define CONFIG_H

#define DEF_PUB_TOPIC  "ws/rpi1/sensorID1/measurements"
#define DEF_SUB_TOPIC  "ws/+/rpi1/#"
#define DEF_DOMAIN      "ws"


#define WIFI_SSID "pj"
#define WIFI_PASS "Wtyczka1"
//#define MQTT_SERV "mqtt://192.168.215.139"
//#define MQTT_SERV "mqtt://192.168.001.101"
#define MQTT_SERV "mqtt://10.248.139.227"


#define MYID "sensorID1"

//#define WIFI_SSID "borki7"
//#define WIFI_PASS "Wtyczka1"
//#define MQTT_SERV "192.168.1.183"
//#define MYID "sensorID1"

//#define WIFI_SSID "BTIA_MIM5LOD"
//#define WIFI_PASS "B1d1qJiKaiIb"
//#define MQTT_SERV "10.248.139.227"
//#define MYID "sensorID1"

//#define WIFI_SSID "BTIA_MIM5LOD"
//#define WIFI_PASS "rX258Vq0Lhds"
//#define MQTT_SERV "10.248.139.239"
//#define MYID "sensorID1"

//#define WIFI_SSID "AndroidAP"
//#define WIFI_PASS "hackersoft"
//#define MQTT_SERV "192.168.1.114"
//#define MYID "sensorID1"

#define EEPROM_SIZE         160
#define EEPROM_OFFS			    		0
#define EEPROM_OFFS_SSID_32B    		0
#define EEPROM_OFFS_PASS_32B    		32
#define EEPROM_OFFS_MQTT_32B    		64
#define EEPROM_OFFS_MYID_32B    		96
#define EEPROM_OFFS_SLEEP_ON_STILL_4B 	128
#define EEPROM_OFFS_PTH_1B 				132
#define EEPROM_OFFS_JOB_STATE_1B	    133
#define EEPROM_OFFS_ACMMINTERLOCK_1B	134
#define EEPROM_OFFS_ACMMPULLUP_1B		135
#define EEPROM_OFFS_FLAG_1B    			136



#endif

