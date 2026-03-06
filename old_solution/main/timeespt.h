/*
 * timeespt.h
 *
 *  Created on: 20 lis 2020
 *      Author: JablonskiP
 */

#ifndef MAIN_TIMEESPT_H_
#define MAIN_TIMEESPT_H_

#include "time.h"


class time_esp_t {
	uint32_t tickStamp;
	uint64_t t; // UTC time is msec, to comply with JS
public:
	time_esp_t();
	void set(uint64_t utcMs);
	void set(struct tm* valp);
	void set(const char* str);
	void get(struct tm* valp);
	void gets(char* out);
	uint64_t getMs();
	virtual ~time_esp_t();
};

extern time_esp_t timeEsp;


#endif /* MAIN_TIMEESPT_H_ */
