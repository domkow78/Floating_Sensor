#include "serialEncap.h"
//#include "esp_log.h"

//int serialEncap::timedRead() {
//    int c;
//    unsigned long _startMillis = millis();
//    do {
//        c = Serial.read();
//        if(c >= 0)
//            return c;
//        if(Serial.getTimeout() == 0)
//            return -1;
//        vTaskDelay(pdMS_TO_TICKS(10));
//    } while(millis() - _startMillis < Serial.getTimeout());
//    return -1;     // -1 indicates timeout
//}


//String serialEncap::readStringUntil(const char* terminators) {
//    String ret;
//
//    int c = timedRead();
//    while (c >= 0 && (NULL==strchr(terminators,c))){
//        ret += (char) c;
//        c = timedRead();
//    }
//    return ret;
//}

