// MQTT Library (framework only)
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: -
// Target uC:       -
// System Clock:    -

// Hardware configuration:
// -

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#ifndef MQTT_H_
#define MQTT_H_

#include <stdint.h>
#include <stdbool.h>
#include "tcp.h"

// TCP Structures

typedef struct _mqttHeader
{
    uint8_t headerFlags;
    uint8_t msgLen;
    uint8_t lengthPayload[0];
} mqttHeader;

typedef struct _mqttConnect
{
    uint16_t protocolNameLength;
    char protocolName[4];
    uint8_t version;
    uint8_t connectFlags;
    uint16_t keepAlive;
    uint16_t clientIdLength;
    uint8_t clientId[0];
} mqttConnect;

typedef struct _mqttPublish
{
    uint16_t topicLength;
    char topic[0];
    char message[0];
} mqttPublish;

typedef struct _mqttSubscribeHeader
{
    uint8_t headerFlags;
    uint8_t msgLen;
    uint16_t MessageIdentifier;
    uint8_t lengthPayload[0];

} mqttSubscribeHeader;

typedef struct _mqttSubscribe
{
    uint16_t topicLength;
    char topic[0];
    uint8_t qos;
} mqttSubscribe;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void connectMqtt(etherHeader *ether, socket *s);
void disconnectMqtt(etherHeader *ether, socket *s);
void publishMqtt(etherHeader *ether, socket *s, char strTopic[], char strData[]);
void subscribeMqtt(etherHeader *ether, socket *s, char strTopic[]);
void unsubscribeMqtt(etherHeader *ether, socket *s, char strTopic[]);
void checkMqttConAck(etherHeader *ether, socket *s);
bool isMqttConAcked(void);

#endif

