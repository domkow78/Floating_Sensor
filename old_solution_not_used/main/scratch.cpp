//#include <string.h>
//#include <strings.h>
//#include "freertos/FreeRTOS.h"
//#include "freertos/event_groups.h"
//#include "esp_wifi.h"
//#include "esp_log.h"
//#include "esp_event.h"
//#include "nvs_flash.h"
////#include <map>
//
//
//typedef struct{
//	const char* ssid;      /**< SSID of target AP. */
//	const char* pass;  /**< Password of target AP. */
//} ssidpass_t;
//
//ssidpass_t ssidpass[5] = {
//		{.ssid = "btia_mim5lod", 	.pass = "99192525"},
//		{.ssid = "BTIA_MIM5LOD", 	.pass = "99192525"},
//		{.ssid = "pj", 				.pass = "Wtyczka1"},
//	    {.ssid = "BTIA_MIM5LOD", 	.pass = "B1d1qJiKaiIb"},
//	    {.ssid = "borki", 			.pass = "Wtyczka1"}
//	};
//
//#define EXAMPLE_ESP_WIFI_PASS "99192525"
//#define EXAMPLE_ESP_WIFI_SSID "BTIA_MIM5LOD"
///* FreeRTOS event group to signal when we are connected*/
//static EventGroupHandle_t s_wifi_event_group;
///* The event group allows multiple bits for each event, but we only care about two events:
// * - we are connected to the AP with an IP
// * - we failed to connect after the maximum amount of retries */
//#define WIFI_CONNECTED_BIT BIT0
//#define WIFI_FAIL_BIT      BIT1
//
//#define DEFAULT_SCAN_LIST_SIZE 5
//#define WIFI_MAXIMUM_RETRY 5
//static int s_retry_num = 0;
//
//static const char *TAG = "scan";
//
//static int wifi_scan(ssidpass_t* a_pssidpass);
//
//
///* registered to a default esp_event_loop, uff */
//static void event_handler(void* arg, esp_event_base_t event_base,
//                                int32_t event_id, void* event_data)
//{
//	ESP_LOGI(TAG, "EH %d", event_id);
//    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
//        esp_wifi_connect();
//    }
//    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
//            s_retry_num++;
//            ESP_LOGI(TAG, "retry to connect to the AP");
//            esp_wifi_connect();
//        } else {
//			ESP_LOGI(TAG,"connect to the AP fail");
//			s_retry_num = 0;
//            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
//        }
//    }
//    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
//        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
//        s_retry_num = 0;
//        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
//    }
//}
//
//
//static void fast_scan(void)
//{
//    ESP_ERROR_CHECK(esp_netif_init());
//    ESP_ERROR_CHECK(esp_event_loop_create_default());
//
//    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
//
//    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
//    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));
//
//    // Initialize default station as network interface instance (esp-netif)
//    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
//    assert(sta_netif);
//
//    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//
//    // Initialize and start WiFi
//    wifi_config_t wifi_config;
//    strcpy((char*)wifi_config.sta.ssid, "pj");
//	strcpy((char*)wifi_config.sta.password, "Wtyczka1");
//	wifi_config.sta.scan_method = WIFI_FAST_SCAN;
//	wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
//    wifi_config.sta.threshold.rssi = -127;
//	wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
//    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
//    ESP_ERROR_CHECK(esp_wifi_start());
//
//	s_wifi_event_group = xEventGroupCreate();
//    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
//            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
//            pdFALSE,
//            pdFALSE,
//            portMAX_DELAY);
//
//    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
//     * happened. */
//    if (bits & WIFI_CONNECTED_BIT) {
//        ESP_LOGI(TAG, "connected to SSID:%s pass:%s",
//        		wifi_config.sta.ssid, wifi_config.sta.password);
//    } else if (bits & WIFI_FAIL_BIT) {
//        ESP_LOGI(TAG, "Failed to connect to SSID:%s, pass:%s",
//        		wifi_config.sta.ssid, wifi_config.sta.password);
//    } else {
//        ESP_LOGE(TAG, "UNEXPECTED EVENT");
//    }
//
//}
//
//void wifi_init_sta__(void)
//{
////    fast_scan();
////    return;
//
//	s_wifi_event_group = xEventGroupCreate();
//
//    ESP_ERROR_CHECK(esp_netif_init());
//
//    ESP_ERROR_CHECK(esp_event_loop_create_default());
//    esp_netif_create_default_wifi_sta();
//
//    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
//
//    esp_event_handler_instance_t instance_any_id;
//    esp_event_handler_instance_t instance_got_ip;
//    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
//                                                        ESP_EVENT_ANY_ID,
//                                                        &event_handler,
//                                                        NULL,
//                                                        &instance_any_id));
//    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
//                                                        IP_EVENT_STA_GOT_IP,
//                                                        &event_handler,
//                                                        NULL,
//                                                        &instance_got_ip));
//
//    wifi_config_t wifi_config;
//
//    for (int ssidindx=0; ssidindx < sizeof(ssidpass)/sizeof(ssidpass_t); ssidindx++){
//	    strcpy((char*)wifi_config.sta.ssid, ssidpass[ssidindx].ssid);
//	    strcpy((char*)wifi_config.sta.password, ssidpass[ssidindx].pass);
////		strcpy((char*)wifi_config.sta.ssid, "pj");
////		strcpy((char*)wifi_config.sta.password, "Wtyczka1");
////		wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
//		wifi_config.sta.pmf_cfg.capable = true;
//		wifi_config.sta.pmf_cfg.required = false;
//
//		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
//		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
//		ESP_ERROR_CHECK(esp_wifi_start() );
//
//		ESP_LOGI(TAG, "wifi start, %s %s", wifi_config.sta.ssid, wifi_config.sta.password);
//
//		/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
//		 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
//		EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
//				WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
//				pdTRUE,
//				pdFALSE,
//				portMAX_DELAY);
//
//		/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
//		 * happened. */
//		if (bits & WIFI_CONNECTED_BIT) {
//			ESP_LOGI(TAG, "connected to SSID:%s pass:%s", wifi_config.sta.ssid, wifi_config.sta.password);
//			break;
//		} else if (bits & WIFI_FAIL_BIT) {
//			ESP_LOGI(TAG, "Failed to connect to SSID:%s, pass:%s", wifi_config.sta.ssid, wifi_config.sta.password);
//			esp_wifi_stop();
//			continue;
//		} else {
//			ESP_LOGE(TAG, "UNEXPECTED EVENT");
//			break;
//		}
//    }
//    ESP_LOGI(TAG, "unregister");
//    /* The event will not be processed after unregister */
//    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
//    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
//    vEventGroupDelete(s_wifi_event_group);
//}
//
///* Scans wifi for ssids known in a_pssidpass and returns index of found*/
//static int wifi_scan(ssidpass_t* a_pssidpass)
//{
////	ESP_ERROR_CHECK(esp_netif_init());
////    ESP_ERROR_CHECK(esp_event_loop_create_default());
////    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
////    assert(sta_netif);
////
////    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
////    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
//
//
//    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//    ESP_ERROR_CHECK(esp_wifi_start());
//    esp_wifi_scan_start(NULL, true);
//    ESP_LOGI(TAG, "esp_wifi_scan_start done");
//
//    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
//    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
////    uint16_t ap_count = 0;
//    memset(ap_info, 0, sizeof(ap_info));
//
//    int retval = esp_wifi_scan_get_ap_records(&number, ap_info);
//    if (retval==0){
//		ESP_LOGI(TAG, "Total APs scanned by get ap records= %u", number);
//    }else{
//    	ESP_LOGI(TAG, "esp_wifi_scan_get_ap_records error %d", retval);
//    }
////    retval = esp_wifi_scan_get_ap_num(&ap_count);
////    if (retval==0){
////		ESP_LOGI(TAG, "Total APs scanned by get ap num= %u", ap_count);
////	}else{
////		ESP_LOGI(TAG, "esp_wifi_scan_get_ap_num error %d", retval);
////	}
//
//    esp_wifi_scan_stop();
//    esp_wifi_stop();
//
//	for (int j = 0; j < sizeof(*a_pssidpass)/sizeof(ssidpass_t); j++)
//	{
//		for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < number); i++) {
////        print_auth_mode(ap_info[i].authmode);
////        ESP_LOGI(TAG, "Channel \t\t%d\n", ap_info[i].primary);
//			ESP_LOGI(TAG, "SSID %s\tRSSI %d", ap_info[i].ssid, ap_info[i].rssi);
//			if (strcasecmp((const char*)ap_info[i].ssid, ssidpass[j].ssid)==0){
//				ESP_LOGI(TAG, "%s==%s match", ap_info[i].ssid, ssidpass[j].ssid);
//        		return j;
//        	}
//        }
////        if (ap_info[i].authmode != WIFI_AUTH_WEP) {
////            print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
////        }
//    }
//    return -1;
//}
//
////
////
////std::map<const char*, const char*> logins;
////	logins["btia"] = "haslo";
////	logins["borki"] = "innehaslo";
////
////std::map<std::string, int>::iterator it = mapOfWords.begin();
////    while(it != mapOfWords.end())
////    {
////        std::cout<<it->first<<" :: "<<it->second<<std::endl;
////        it++;
////    }
//
//
//
///* WiFi station Example
//   This example code is in the Public Domain (or CC0 licensed, at your option.)
//   Unless required by applicable law or agreed to in writing, this
//   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
//   CONDITIONS OF ANY KIND, either express or implied.
//*/
