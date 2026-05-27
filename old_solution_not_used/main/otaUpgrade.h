/*
 * otaUpgrade.h
 *
 *  Created on: 12 lis 2020
 *      Author: JablonskiP
 */

#ifndef MAIN_OTAUPGRADE_H_
#define MAIN_OTAUPGRADE_H_

#define OTA_FILENAME_DEF "SmartSensorESP.bin"
#define OTA_URL_SIZE 256
#define OTA_PORT "8070"


extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

void otaUpgrade(const char*);



#endif /* MAIN_OTAUPGRADE_H_ */
