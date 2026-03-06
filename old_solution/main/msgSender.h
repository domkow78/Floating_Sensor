#ifndef MSG_SENDER_H
#define MSG_SENDER_H

#include "mqttMsg.h"
#include "QueueArray.h"
#include "spinlock.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

enum class msgSenderThr_t : uint8_t { Full = 0x00, Half = 0x01, Quarter = 0x03 };

//https://www.hivemq.com/blog/mqtt-essentials-part-6-mqtt-quality-of-service-levels/
#define MQTT_QOS_AT_MOST_ONCE	0
//The minimal QoS level is zero. This service level guarantees a best-effort delivery. There is no guarantee of delivery. The recipient does not acknowledge receipt of the message and the message is not stored and re-transmitted by the sender. QoS level 0 is often called “fire and forget” and provides the same guarantee as the underlying TCP protocol.
#define MQTT_QOS_AT_LEAST_ONCE	1
//QoS level 1 guarantees that a message is delivered at least one time to the receiver. The sender stores the message until it gets a PUBACK packet from the receiver that acknowledges receipt of the message. It is possible for a message to be sent or delivered multiple times.
#define MQTT_QOS_EXACTLY_ONCE	2
//QoS 2 is the highest level of service in MQTT. This level guarantees that each message is received only once by the intended recipients. QoS 2 is the safest and slowest quality of service level. The guarantee is provided by at least two request/response flows (a four-part handshake) between the sender and the receiver. The sender and receiver use the packet identifier of the original PUBLISH message to coordinate delivery of the message.
#define MQTT_QOS_DEFAULT	MQTT_QOS_AT_MOST_ONCE

#define TASK_SEND_INTERVAL_MS 12000

class MsgSender{
  private:
    QueueArray <mqttMsg> m_msgQueue;
    bool sendFromQueue(void);
    spinlock_t lock;
  public:
    TaskHandle_t htask;
    msgSenderThr_t sendThreshold;
    MsgSender();
    void init(void);
    ~MsgSender();
    int taskSendIntervalMs = TASK_SEND_INTERVAL_MS;
    bool post(mqttMsg);
    bool post(char*,uint8_t*,unsigned int);
    bool forceSend(void);
    void flush(void);
    int qlen(void);
};

extern MsgSender msgSender;

void taskSendCbk(void* ptr);

#endif
