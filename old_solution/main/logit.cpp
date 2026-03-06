//#include <Arduino.h>

#define LOG_BUFFER_LEN 150
/*
void logit(unsigned int aDebugLevel, const char* formattedMsg, ...)
Input: 
    aDebugLevel - if lower than compiler's -DVERBOSE_THR, then do nothing
    formattedMsg - acc. to printf syntax
NOT MORE THAN 50 CHARS !!!!
*/
//void logit(unsigned int aDebugLevel, const char* formattedMsg, ...){
//
//    if (aDebugLevel >= VERBOSE_THR){
//
//        va_list args;
//        va_start(args,formattedMsg);
//        char buffer[LOG_BUFFER_LEN];
//        int cnt = vsnprintf(buffer,sizeof(buffer),formattedMsg,args);
//        va_end(args);
//        Serial.print(buffer);
//        if (cnt>LOG_BUFFER_LEN)
//            Serial.print("...(log message truncated)\n");
//    }
//}
//void logit(unsigned int aDebugLevel, const __FlashStringHelper* formattedMsg, ...){
//    va_list args;
//    va_start(args,formattedMsg);
//    logit(aDebugLevel,(const char*)formattedMsg,args);
//    va_end(args);
// }
 
