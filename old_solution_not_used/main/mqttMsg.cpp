//#include <Arduino.h>
#include "mqttMsg.h"
//#include "logit.h"
//#include <string.h>

static const char* TAG = "MQTTmsg";
#include "esp_log.h"

extern char myID[16];

//#include <string>              // std::string
//#include <cstring>             // strlen()
//typedef std::string String;
//static String str;

mqttMsg::mqttMsg(){
    init("empty/empty/empty/empty",nullptr,0);
}

//mqttMsg::mqttMsg(std::string t, std::string p) : mqttMsg((char*)t.c_str(), (uint8_t*)p.c_str(), (unsigned int)p.length()){} //tadam

mqttMsg::mqttMsg(const char* aTopic, const int aTopiclen, const char* aPayload, const int aPayloadlen){
	  init(aTopic, aTopiclen, aPayload, aPayloadlen);

	  if (!target || !source || !smallTopic){
	    ESP_LOGE(TAG,"mqttMsg::mqttMsg: Invalid MQTT topic format.");
	    init("badFormat/badFormat/badFormat/badFormat",nullptr,0);
	  }
	  else{
	    //logit(LOGLVL_MQTT,"mqttMsg::mqttMsg: domain: %s, target %s, source %s, smallTopic %s \n",domain,target,source,smallTopic);
	  }
}

mqttMsg::mqttMsg(const char* aTopic, const uint8_t* aPayload, const unsigned int aPLen){
  init(aTopic, aPayload, aPLen);
  
  if (!target || !source || !smallTopic){
    ESP_LOGE(TAG,"mqttMsg::mqttMsg: Invalid MQTT topic format.");
    init("badFormat/badFormat/badFormat/badFormat",nullptr,0);
  }
  else{
    //logit(LOGLVL_MQTT,"mqttMsg::mqttMsg: domain: %s, target %s, source %s, smallTopic %s \n",domain,target,source,smallTopic);
  }
}

mqttMsg::mqttMsg(const char* aTopic, const char* aPayload){
  init(aTopic, (const uint8_t*)aPayload, strlen(aPayload));
  
  if (!target || !source || !smallTopic){
	ESP_LOGE(TAG, "mqttMsg::mqttMsg: Invalid MQTT topic format."); // @suppress("Ambiguous problem")
    init("badFormat/badFormat/badFormat/badFormat",nullptr,0);
  }
  else{
    //logit(LOGLVL_MQTT,"mqttMsg::mqttMsg: domain: %s, target %s, source %s, smallTopic %s \n",domain,target,source,smallTopic);
  }
}

mqttMsg::mqttMsg(const char* aDomain, const char* aTarget, const char* aSource, const char* aSmallTopic, const char* aPayload){
  strcpy(topic,"ws/");
  strcat(topic,aTarget);
  strcat(topic,"/");
  strcat(topic,aSource);
  strcat(topic,"/");
  strcat(topic,aSmallTopic);

  init(topic, (const uint8_t*)aPayload, strlen(aPayload));
  
  if (!target || !source || !smallTopic){
	  ESP_LOGE(TAG,"mqttMsg::mqttMsg: Invalid MQTT topic format");
    init("badFormat/badFormat/badFormat/badFormat",nullptr,0);
  }
  else{
    //logit(LOGLVL_MQTT,"mqttMsg::mqttMsg: domain: %s, target %s, source %s, smallTopic %s \n",domain,target,source,smallTopic);
  }
}
    
mqttMsg::mqttMsg(const char* aDomain, const char* aTarget, const char* aSource, const char* aSmallTopic, const uint8_t* p, size_t len){

  strcpy(topic,"ws/");
  strcat(topic,aTarget);
  strcat(topic,"/");
  strcat(topic,aSource);
  strcat(topic,"/");
  strcat(topic,aSmallTopic);

  init(topic, p, len);
  
  if (!target || !source || !smallTopic){
	  ESP_LOGE(TAG,"mqttMsg::mqttMsg: Invalid MQTT topic format");
    init("badFormat/badFormat/badFormat/badFormat",nullptr,0);
  }
  else{
    //logit(LOGLVL_MQTT,"mqttMsg::mqttMsg: domain: %s, target %s, source %s, smallTopic %s \n",domain,target,source,smallTopic);
  }
}

void mqttMsg::init(const char* aTopic, const uint8_t* p, unsigned int len){
	init(aTopic, strlen(aTopic), (const char*)p, (int)len);
}

void mqttMsg::init(const char* aTopic, const int aTopiclen, const char* aPayload, const int aPayloadlen){
	int temp = (aTopiclen < TOPIC_MAX_SIZE-1) ? aTopiclen : TOPIC_MAX_SIZE-1;
	strncpy(topic, aTopic, temp);
    topic[temp] = '\0';	//make sure it is null-terminated, even if not all was copied
    if (aTopiclen > TOPIC_MAX_SIZE-1){	//show by dots when topic was truncated
    	topic[TOPIC_MAX_SIZE-2] = '.';
    	topic[TOPIC_MAX_SIZE-3] = '.';
    	topic[TOPIC_MAX_SIZE-4] = '.';
    }

    temp = (aPayloadlen < PAYLOAD_MAX_SIZE-1) ? aPayloadlen : PAYLOAD_MAX_SIZE-1;
    strncpy(payload, aPayload, temp);
	payload[aPayloadlen] = '\0';
    if (aPayloadlen > PAYLOAD_MAX_SIZE-1){	//show by dots when topic was truncated
        	payload[PAYLOAD_MAX_SIZE-2] = '.';
        	payload[PAYLOAD_MAX_SIZE-3] = '.';
        	payload[PAYLOAD_MAX_SIZE-4] = '.';
	}
    //char* ptr = domain; // to avoid char[] vs. char* 'assigment incompatibility'
    //ptr = 
    strcpy(buf, topic);
    strtok(buf, "/");
    target = strtok(NULL, "/");
    source = strtok(NULL, "/");
    smallTopic = strtok(NULL, "/");
    domain = buf;
    //logit(LOGLVL_MQTT, "mqttMsg::init: domain:%s, target:%s, source:%s, smallTopic:%s\n", domain, target, source, smallTopic);
  }
  

/*
  QueueArray keeps objects on heap. To make this class work with QueueArray, we need a copy constructor.
  Used in QueueArray::dequeue (line with 'item = contents[..]')
  Copy constructor is called when a new object is created from an existing object, 
  as a copy of the existing object. 
  An assignment operator is called when an already initialized object is assigned a new value from another existing object.
*/
mqttMsg::mqttMsg(const mqttMsg& origRef){
  memcpy(topic,origRef.topic,TOPIC_MAX_SIZE);
  memcpy(buf,origRef.buf,TOPIC_MAX_SIZE);
  memcpy(payload,origRef.payload,PAYLOAD_MAX_SIZE);
  target = buf + (origRef.target - origRef.buf);
  source = buf + (origRef.source - origRef.buf);
  smallTopic = buf + (origRef.smallTopic - origRef.buf);
  domain = buf;
  //logit(LOGLVL_MQTT,"mqttMsg::copyctor: domain: %s, target %s, source %s, smallTopic %s \n",domain,target,source,smallTopic);
}
/*
  QueueArray keeps objects on heap. To make this class work with QueueArray, we need a copy operator.
  Used in QueueArray::enqueue (line with 'contents[..] = i')
  https://en.cppreference.com/w/cpp/language/operators#Assignment_operator
  Copy constructor is called when a new object is created from an existing object, 
  as a copy of the existing object. 
  An assignment operator is called when an already initialized object is assigned a new value from another existing object.
  And the latter happens when enqueing, because the queue elements are already there!
*/
mqttMsg& mqttMsg::operator=(const mqttMsg& orig){
  //logit(LOGLVL_MQTT,"mqttMsg copy operator: ");
  if (this!=&orig){
    memcpy(topic,orig.topic,TOPIC_MAX_SIZE);
    memcpy(buf,orig.buf,TOPIC_MAX_SIZE);
    memcpy(payload,orig.payload,PAYLOAD_MAX_SIZE);
    target = buf + (orig.target - orig.buf);
    //logit(LOGLVL_MQTT,"mqttMsg::operator=: domain: (%p) %s, target: (%p) %s\n",(void*)domain,domain,(void*)target,target);
    source = buf + (orig.source - orig.buf);
    smallTopic = buf + (orig.smallTopic - orig.buf);
    domain = buf;
  }
  return *this;
}


  //or just use 
  //int snprintf ( char * s, size_t n, const char * format, ... )
