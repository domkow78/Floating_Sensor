/*
 * runtimestats.cpp
 *
 *  Created on: 12 paü 2022
 *      Author: JablonskiP
 */


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <cstring>

static const char* TAG = "STATS";


//configGENERATE_RUN_TIME_STATS
//configUSE_TRACE_FACILITY

void printRunTimeStats(void) {
#if configUSE_TRACE_FACILITY
	volatile UBaseType_t uxArraySize, x;
	uint32_t ulTotalRunTime, ulStatsAsPercentage;
	const char* states = "Rrbsdi";	//running, ready, blocked, suspended, deleted, invalid
	char *pcWriteBuffer = (char*)malloc((size_t)64);
	memset(pcWriteBuffer, 0, (size_t)64);
	//	configMAX_TASK_NAME_LEN
	uxArraySize = uxTaskGetNumberOfTasks();
	TaskStatus_t *pxTaskStatusArray = (TaskStatus_t*) pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

	if (pxTaskStatusArray != NULL) {
		uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize,
				&ulTotalRunTime);

		ulTotalRunTime /= 100UL;

		if (ulTotalRunTime > 0) {
			ESP_LOGI(TAG, "%16.16s\t%8s\t%4.4s\t%6.6s\tState/Core",
					"taskname", "time", "/100", "WM" );
			// For each populated position in the pxTaskStatusArray array,
			// format the raw data as human readable ASCII data
			for (x = 0; x < uxArraySize; x++) {
				// ulTotalRunTimeDiv100 has already been divided by 100.
				ulStatsAsPercentage = pxTaskStatusArray[x].ulRunTimeCounter
						/ ulTotalRunTime;

				ESP_LOGI(TAG, "%16.16s\t%8d\t%4d\t%6d\t%1.1s"
#ifdef configTASKLIST_INCLUDE_COREID
						"\t%2d"
#endif
						,pxTaskStatusArray[x].pcTaskName
						,pxTaskStatusArray[x].ulRunTimeCounter%1000000
						,ulStatsAsPercentage
						,pxTaskStatusArray[x].usStackHighWaterMark%1000
						,states + pxTaskStatusArray[x].eCurrentState
#ifdef configTASKLIST_INCLUDE_COREID
						,pxTaskStatusArray[x].xCoreID
#endif
				);

			}
		} else
			ESP_LOGW(TAG, "total run time is 0");
		vPortFree(pxTaskStatusArray);
	}else
		ESP_LOGW(TAG, "pxTaskStatusArray is NULL");
	vPortFree(pcWriteBuffer);

#endif	configUSE_TRACE_FACILITY
}

