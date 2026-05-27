#ifndef CLI_H
#define CLI_H

namespace cli{
    void setMyIDCbk(cmd* commandPointer);
    void setSsidCbk(cmd* commandPointer);
    void setPassCbk(cmd* commandPointer);
    void connectCbk(cmd* commandPointer);
    void storeCbk(cmd* commandPointer);
    void setHubCbk(cmd* commandPointer);
    void reset(cmd* commandPointer);
    void mqttrecCbk(cmd* commandPointer);
    void mqttsendCbk(cmd* commandPointer);
    void h1up(cmd* commandPointer);
    void h2up(cmd* commandPointer);
    void hiz(cmd* commandPointer);
    void adc(cmd* commandPointer);
    void acmmspeed(cmd* commandPointer);
    void autoLightSleep(cmd* commandPointer);
    void mqttLogLevel(cmd* commandPointer);
    void pth(cmd* commandPointer);
    void selfPullupEn(cmd* commandPointer);
    void interlockMode(cmd* commandPointer);
//    void rtos(cmd* commandPointer);
    void test(cmd* commandPointer);
}

#endif
