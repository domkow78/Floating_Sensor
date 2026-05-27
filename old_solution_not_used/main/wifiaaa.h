/*
 * wifiaaa.h
 *
 *  Created on: 21 kwi 2022
 *      Author: JablonskiP
 */

#ifndef MAIN_WIFIAAA_H_
#define MAIN_WIFIAAA_H_

#define SSID_SIZE_MAX	12
#define PASS_SIZE_MAX	12


#ifdef __cplusplus
extern "C" {
#endif

	typedef struct{
		char ssid[SSID_SIZE_MAX+1];      		/**< SSID of target AP. */
		char pass[PASS_SIZE_MAX+1];		  /**< Password of target AP. */
	} ssidpass_t;

	extern ssidpass_t ssidpass[];
	// all of your legacy C code here
	void wifi_start(void);

#ifdef __cplusplus
}
#endif



#endif /* MAIN_WIFIAAA_H_ */
