/*
 * otaUpgrade.cpp
 *
 *  Created on: 12 lis 2020
 *      Author: JablonskiP
 */


#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "otaUpgrade.h"

//#include "nvs.h"
//#include "nvs_flash.h"


static const char* TAG = "OTA";

/*The function esp_log_level_set() cannot set logging levels higher than specified by CONFIG_LOG_DEFAULT_LEVEL.
 * To increase log level for a specific file at compile time, use the macro LOG_LOCAL_LEVEL before include esp_log.h
 */
//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"


#define OTA_RECV_TIMEOUT_MS	(10*1000)



static esp_err_t validate_image_header(esp_app_desc_t *new_app_info)
{
	if (new_app_info == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	const esp_partition_t *running = esp_ota_get_running_partition();
	esp_app_desc_t running_app_info;
	if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
		ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
	}
	if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
		ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
		return ESP_FAIL;
	}
	return ESP_OK;
}

void otaUpgrade(const char* url){
	ESP_LOGI(TAG, "starting OTA from %s", url);
	esp_err_t ota_finish_err = ESP_OK;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
	esp_http_client_config_t config = {
			//.url = "https://192.168.000.107:8070/SmartSensorESP.bin",
			.url = url,
			.cert_pem = (char *)server_cert_pem_start,
			.timeout_ms = OTA_RECV_TIMEOUT_MS,
			.skip_cert_common_name_check = true
	};
	#pragma GCC diagnostic pop
	ESP_LOGI(TAG, "111");
	esp_https_ota_config_t ota_config = {
		.http_config = &config,
	};

	ESP_LOGI(TAG, "222");
	esp_https_ota_handle_t https_ota_handle = NULL;
	ESP_LOGI(TAG, "333");
	esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
	ESP_LOGI(TAG, "444");
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
		return;
	}
	ESP_LOGI(TAG, "555");

	esp_app_desc_t app_desc;
	err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
	ESP_LOGI(TAG, "666");

	if (err != ESP_OK) {
		ESP_LOGE(TAG, "esp_https_ota_read_img_desc failed");
		goto ota_end;
	}
	err = validate_image_header(&app_desc);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "image header verification failed");
		goto ota_end;
	}

	while (1) {
		err = esp_https_ota_perform(https_ota_handle);
		if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
			break;
		}
		// esp_https_ota_perform returns after every read operation which gives user the ability to
		// monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
		// data read so far.
		ESP_LOGI(TAG, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
	}

	if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
		// the OTA image was not completely received and user can customise the response to this situation.
		ESP_LOGE(TAG, "Complete data was not received.");
	}

ota_end:
	ota_finish_err = esp_https_ota_finish(https_ota_handle);
	if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
		ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		esp_restart();
	} else {
		if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
			ESP_LOGE(TAG, "Image validation failed, image is corrupted");
		}
		ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed %d", ota_finish_err);
		return;
	}
}

/*
Enter a directory where holds the root of the HTTPS server, e.g. cd build.
To create a new self-signed certificate and key, you can simply run command openssl req -x509 -newkey rsa:2048 -keyout ca_key.pem -out ca_cert.pem -days 365 -nodes.
When prompted for the Common Name (CN), enter the name of the server that the ESP32 will connect to. Regarding this example, it is probably the IP address. The HTTPS client will make sure that the CN matches the address given in the HTTPS URL.
To start the HTTPS server, you can simply run command openssl s_server -WWW -key ca_key.pem -cert ca_cert.pem -port 8070.
In the same directory, there should be the firmware (e.g. hello-world.bin) that ESP32 will download later. It can be any other ESP-IDF application as well, as long as you also update the Firmware Upgrade URL in the menuconfig. The only difference is that when flashed via serial the binary is flashed to the "factory" app partition, and an OTA update flashes to an OTA app partition.
Notes: If you have any firewall software running that will block incoming access to port 8070, configure it to allow access while running the example.
*/
