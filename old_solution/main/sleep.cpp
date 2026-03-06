//#include <Arduino.h>
#include "sleep.h"
//#include <TaskSchedulerDeclarations.h>

void SleepTimerCbk(unsigned long aT)
{
    /*deep sleep causes WiFi reset, sucks*/
    // uint64_t deepSleepMax = ESP.deepSleepMax();
    // aT = uS_TO_mS * aT;
    // unsigned long aPart; 
    // for (; aT > 0; aT -= aPart){
    //     aPart = (deepSleepMax > aT) ? aT : deepSleepMax; 
    //     ESP.deepSleep(aPart, RF_NO_CAL);
    // }
    //delay(aT);
//    vTaskDelay(aT);
    /*
    this should let enter automatic light sleep mode, as specified by wifi_set_sleep_type(LIGHT_SLEEP_T);
    on condition that we are in WiFi.mode(WIFI_STA)
    expriments showed that behaviour does not change even if delay(1) instead of delay(aT)
    maybe because every 1ms (tick) the system is woken up anyhow, disregarding how many ms we specify in aT
    */
}

void SleepRSTCbk(unsigned long aT)
{
    //ESP.deepSleep(0,WAKE_RFCAL);   
}
