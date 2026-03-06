#ifndef MSG_PROCESSOR_H
#define MSG_PROCESSOR_H

#include "QueueArray.h"
//#include <ArduinoJson.h>
#include "mqttMsg.h"
#include "cJSON.h"

void sendStatus(const char* src, const char* lastSmallTopic);

class MsgProcessor{
  private:
    QueueArray<mqttMsg> *m_pMsgQueue;
//    DynamicJsonDocument* m_jsonRoot; //initialize it in MsgProcessor constructor, please, and destruct this too
//    DeserializationError m_jsonError;

    cJSON* jroot;
    // void log(char*);
    // void log(const __FlashStringHelper* in);
    // void log(const char* in);

  public:
    MsgProcessor();
    ~MsgProcessor();

    bool enqueue(mqttMsg);
    bool enqueue(const char* topic, const int topiclen, const char* payload, const int payloadlen);
    bool enqueue(const char* topic, const char* payload, const int payloadlen);
    bool enqueue(const char* topic, const char* payload);
    bool enqueue(const char*, const uint8_t*, const unsigned int);
    bool execute(void);
    void flush(void);
};

#endif
