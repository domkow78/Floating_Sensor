
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
//#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "wifiaaa.h"

extern bool booting;	//this comes from main

/*  when this get compiled in C++, cannot connect !!!!!!!!!!!!!!!!!!!!!!!!!!! */

/* The examples use WiFi configuration that you can set via project configuration menu
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "XXXXXXXXXXXXX"
#define EXAMPLE_ESP_WIFI_PASS      "1111111111111"
#define WIFIAAA_MAX_RETRY_ON_BOOT		1
#define WIFIAAA_MAX_RETRY_ON_RUN		50
#define WIFIAAA_SCAN_LIST_SIZE			8

ssidpass_t ssidpass[5] = {
		{.ssid = "",	.pass = ""},
		{.ssid = "pj",			 				.pass = "Wtyczka1"},
	    {.ssid = "BTIA_MIM5LOD", 				.pass = "B1d1qJiKaiIb"},
		{.ssid = "BTIA_MIM5LOD", 				.pass = "99192525"},
	    {.ssid = "borki",		 				.pass = "Wtyczka1"}
	};

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifiaaa";

static int s_retry_num = 0;

static wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,
//		 .listen_interval,   /**< Listen interval for ESP32 station to receive beacon when WIFI_PS_MAX_MODEM is set. Units: AP beacon intervals. Defaults to 3 if set to 0. */
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };


static void wifi_eh(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (booting){
        	if (s_retry_num < WIFIAAA_MAX_RETRY_ON_BOOT){
        		s_retry_num++;
				ESP_LOGI(TAG, "retry to connect %s %s", wifi_config.sta.ssid, wifi_config.sta.password);
				esp_wifi_connect();
        	}else {
        		s_retry_num = 0;
				ESP_LOGI(TAG,"connect to %s %s failed.", wifi_config.sta.ssid, wifi_config.sta.password);
				if (s_wifi_event_group)
					xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        	}
        }else{	//not booting
        	if (s_retry_num < WIFIAAA_MAX_RETRY_ON_RUN){
				s_retry_num++;
				ESP_LOGI(TAG, "retry to connect %s %s", wifi_config.sta.ssid, wifi_config.sta.password);
				esp_wifi_connect();
        	}else {
        		s_retry_num = 0;
				ESP_LOGE(TAG,"connect to %s %s failed too many times. Rebooting.", wifi_config.sta.ssid, wifi_config.sta.password);
				vTaskDelay(pdMS_TO_TICKS(1000));
				esp_restart();
        	}
        }
    } //WIFI_EVENT_STA_DISCONNECTED
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "gw ip:" IPSTR, IP2STR(&event->ip_info.gw));
        s_retry_num = 0;
        if (booting && s_wifi_event_group)
        	xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


/* Scans wifi for ssids known in a_pssidpass and returns index of found*/
static int wifi_scan(ssidpass_t* a_pssidpass, uint8_t a_len)
{
//	ESP_ERROR_CHECK(esp_netif_init());
//    ESP_ERROR_CHECK(esp_event_loop_create_default());
//    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
//    assert(sta_netif);
//
//    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//    ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_scan_start(NULL, true);
    ESP_LOGI(TAG, "scan: esp_wifi_scan_start done");

    uint16_t number = WIFIAAA_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[WIFIAAA_SCAN_LIST_SIZE];
//    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    int retval = esp_wifi_scan_get_ap_records(&number, ap_info);
    if (retval==0){
		ESP_LOGI(TAG, "scan: Total APs scanned by get ap records=%u", number);
    }else{
    	ESP_LOGI(TAG, "scan: esp_wifi_scan_get_ap_records error %d", retval);
    }
//    retval = esp_wifi_scan_get_ap_num(&ap_count);
//    if (retval==0){
//		ESP_LOGI(TAG, "Total APs scanned by get ap num= %u", ap_count);
//	}else{
//		ESP_LOGI(TAG, "esp_wifi_scan_get_ap_num error %d", retval);
//	}

    esp_wifi_scan_stop();
    esp_wifi_stop();

	for (int i = 0; (i < WIFIAAA_SCAN_LIST_SIZE) && (i < number); i++) {
		ESP_LOGI(TAG, "scan found: ssid: %s, rssi: %d", ap_info[i].ssid, ap_info[i].rssi);
		for (int j = 0; j < a_len; j++){
			ESP_LOGI(TAG, "scan: compare against %s", a_pssidpass[j].ssid);
			if (strcasecmp((const char*)ap_info[i].ssid, a_pssidpass[j].ssid) == 0){
				ESP_LOGI(TAG, "scan: %s =!MATCH!= %s", ap_info[i].ssid, a_pssidpass[j].ssid);
        		return j;
        	}
        }
		ESP_LOGI(TAG, "scan: ssid not recognized");
    }
    return -1;
}


void wifi_start(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    int matchedIdx = wifi_scan(ssidpass, sizeof(ssidpass)/sizeof(ssidpass_t));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_eh,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_eh,
                                                        NULL,
                                                        &instance_got_ip));


    for (uint8_t ssidindex = 0; ssidindex < sizeof(ssidpass)/sizeof(ssidpass_t); ssidindex++){
		if (matchedIdx>=0)
			if (strcmp(ssidpass[ssidindex].ssid, ssidpass[matchedIdx].ssid) != 0)
				continue;
		if (strlen(ssidpass[ssidindex].ssid) == 0)
			continue;
    	strcpy((char*)wifi_config.sta.ssid, ssidpass[ssidindex].ssid);
		strcpy((char*)wifi_config.sta.password, ssidpass[ssidindex].pass);
    	ESP_ERROR_CHECK(esp_wifi_stop() );
		ESP_LOGI(TAG, "Try to connect %s %s", wifi_config.sta.ssid, wifi_config.sta.password);
    	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
		ESP_ERROR_CHECK(esp_wifi_start() );
		/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
		 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
		EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

		if (bits & WIFI_CONNECTED_BIT) {
			ESP_LOGI(TAG, "CONNECTED to %s %s", wifi_config.sta.ssid, wifi_config.sta.password);
			break;
		} else if (bits & WIFI_FAIL_BIT) {
			ESP_LOGI(TAG, "Failed to connect %s %s", wifi_config.sta.ssid, wifi_config.sta.password);
			continue;
		} else {
			break;
			ESP_LOGE(TAG, "UNEXPECTED EVENT");
		}
    }
    /* The event will not be processed after unregister */
//    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
//    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);	//remember not to reference this group after this happens!!!
}


