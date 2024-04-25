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

#include <stdio.h>
#include <string.h>
#include "mqtt.h"
#include "tcp.h"
#include "socket.h" 
#include "ip.h"
#include "eth0.h"
#include "timer.h"

// ------------------------------------------------------------------------------
//  Globals
// ------------------------------------------------------------------------------

// ------------------------------------------------------------------------------
//  Structures
// ------------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void connectMqtt(etherHeader *ether, socket *s)
{
    uint8_t buffer[100];
    mqttHeader* data = (mqttHeader*) buffer;

    data->headerFlags = 0x10;
    data->msgLen = 0; // Need to update later

    mqttConnect *connect = (mqttConnect*) data->lengthPayload;

    connect->protocolNameLength = htons(0x0004);    // Length of MQTT
    connect->protocolName[0] = 'M';
    connect->protocolName[1] = 'Q';
    connect->protocolName[2] = 'T';
    connect->protocolName[3] = 'T';
    connect->version = 0x04;                        // Version v3.1.1
    connect->connectFlags = 0x02;                   // Clean session
    connect->keepAlive = htons(0x003c);             // Keep alive 60s
    connect->clientIdLength = htons(0x0);           // Length of client ID

    data->msgLen = sizeof(mqttConnect);             // Length of the message

    uint8_t dataSize = sizeof(mqttHeader) + data->msgLen;

    // TODO: Update ack number
    sendTcpMessage(ether, s, PSH | ACK, (uint8_t *)data, dataSize);
}

void disconnectMqtt(etherHeader *ether, socket *s)
{

}

void publishMqtt(etherHeader *ether, socket *s, char strTopic[], char strData[])
{

}

void subscribeMqtt(etherHeader *ether, socket *s, char strTopic[])
{

}

void unsubscribeMqtt(etherHeader *ether, socket *s, char strTopic[])
{

}

