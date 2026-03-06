/*
 * fileLoger.h
 *
 *  Created on: 17 mar 2021
 *      Author: JablonskiP
 */

#ifndef MAIN_FILELOGER_H_
#define MAIN_FILELOGER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "stdio.h"

//enum class msgSenderThr_t : byte { Full = 0x00, Half = 0x01, Quarter = 0x03 };
//#define TASK_SEND_HEAP 1950

//extern int ;
#define LOGFILENAME_LEN	14


class FileLoger_t{
  private:
    bool initSpiffs();
	SemaphoreHandle_t xSemaphore;
//    spinlock_t lock;
  public:
    char fname[LOGFILENAME_LEN];
    bool isReady;
    FileLoger_t();
    uint8_t init();
    bool write(const char* text);
    int readAll(size_t maxTopicSize, size_t maxPayloadSize, bool (*f)(const char* aPayload));
};

extern FileLoger_t fileLoger;

#endif /* MAIN_FILELOGER_H_ */
