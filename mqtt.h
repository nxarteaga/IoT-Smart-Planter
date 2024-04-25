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

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------
//bool mqttConnectFlag = false;
typedef struct _mqttHeader
{
  uint8_t headerFlags;
  uint8_t messageLength;
  uint16_t protocolNameLength;
  //char protocolName[] = {'M','Q','I','s','d','p','\0'};
  char protocolName[7];
  uint8_t version;
  uint8_t connectFlags;
  uint16_t keepAlive;
  uint16_t clientIdLength;
  uint16_t clientId;
  uint8_t data[0];
} mqttHeader;


//void connectMqtt(etherHeader *ether, uint8_t *data, uint16_t size);
void connectMqtt();
void setMqttState(uint8_t state);
uint8_t getMqttState();
void sendMqttMessage(etherHeader *ether, socket* s, uint16_t flags, uint8_t *data, uint16_t dataSize);
//void sendMqttMessage();
void disconnectMqtt();
void publishMqtt(char strTopic[], char strData[]);
void subscribeMqtt(char strTopic[]);
void unsubscribeMqtt(char strTopic[]);

#endif

