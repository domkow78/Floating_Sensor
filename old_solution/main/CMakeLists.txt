# Edit following two lines to set component requirements (see docs)
#set(COMPONENT_REQUIRES DhtSensorLibrary ArduinoJson Adafruit_BME280_Library AdafruitUnifiedSensor SimpleCLI comp1 PubSubClient arimaTools TaskScheduler QueueArray)
#set(COMPONENT_PRIV_REQUIRES )

#set(COMPONENT_SRCS main.cpp acmm.cpp cli.cpp logit.cpp mqttMsg.cpp msgProcessor.cpp msgSender.cpp sleep.cpp taskMeasure.cpp utils.cpp blink.cpp battMeas.cpp)
#set(COMPONENT_ADD_INCLUDEDIRS "")

#register_component()

idf_component_register(
	SRCS main.cpp acmm.cpp cli.cpp logit.cpp mqttMsg.cpp msgProcessor.cpp msgSender.cpp sleep.cpp taskMeasure.cpp 
	utils.cpp blink.cpp battMeas.cpp otaUpgrade.cpp timeespt.cpp serialEncap.cpp fileLoger.cpp PTHsensorWrapper.cpp 
	tp.cpp wifiScan.cpp wifiaaa.c phaseMeas.cpp eeprom.cpp iir.cpp comb.cpp runtimestats.cpp 
	INCLUDE_DIRS "."
	REQUIRES  esp_hw_support esp_adc_cal bme280 SimpleCLI comp1  arimaTools QueueArray app_update esp_http_client esp_https_ota spiffs esp8266_wrapper sht3x wpa_supplicant mqtt nvs_flash esp-dsp 
    # AdafruitUnifiedSensor DhtSensorLibrary PubSubClient ArduinoJson
    # Embed the server root certificate into the final binary
	EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem
	)

