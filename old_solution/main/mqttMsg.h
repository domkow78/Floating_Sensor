#ifndef MQTT_MSG_H
#define MQTT_MSG_H


//#include <Arduino.h>
#include <string.h>

// smartSensor/rpi1/sensorID1/ is 27 chars

#define TOPIC_MAX_SIZE 40
//#define PAYLOAD_MAX_SIZE (128 - 7 - TOPIC_MAX_SIZE)
#define PAYLOAD_MAX_SIZE 128


#define TOPIC_REGISTER_ME "registerMe"
#define TOPIC_STATUS "diag"
#define TOPIC_MEASUREMENT "measurement"
#define TOPIC_PHASE "phase"
#define TOPIC_DOMAIN "ws"
#define TOPIC_LOG "log"
#define TOPIC_UNFINISHED "unfinished"
#define TOPIC_CAP	"cap"


class mqttMsg{
  private:
    void init(const char* t, const uint8_t* p, unsigned int len);
    void init(const char* aTopic, const int aTopiclen, const char* aPayload, const int aPayloadlen);
    char buf[TOPIC_MAX_SIZE];
  public:
    char payload[PAYLOAD_MAX_SIZE];
    const char* domain;
    const char* source;
    const char* target;
    const char* smallTopic;
    char topic[TOPIC_MAX_SIZE];
    mqttMsg();
//    mqttMsg(std::string t, string p);

    mqttMsg(const char* aTopic, const int aTopiclen, const char* aPayload, const int aPayloadlen);
    mqttMsg(const char* aTopic, const uint8_t* aPayload, const unsigned int aPLen);
    mqttMsg(const char* aTopic, const char* aPayload);
    
    mqttMsg(const char* aDomain, const char* aTarget, const char* aSource, const char* aSmallTopic, const uint8_t* aPayload, size_t aPLen);
    mqttMsg(const char* aDomain, const char* aTarget, const char* aSource, const char* aSmallTopic, const char* aPayload);
    
    mqttMsg(const mqttMsg& origRef);
    mqttMsg& operator=(const mqttMsg& orig);
//    static char* topicCat(const char* topic, const char* target);
//    static char* topicCat(const char* topic, const char* target, const char* source);
};

#endif
