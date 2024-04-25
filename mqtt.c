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

#define MAX_BUFF_SIZE 64

// ------------------------------------------------------------------------------
//  Globals
// ------------------------------------------------------------------------------

// ------------------------------------------------------------------------------
//  Structures
// ------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void connectMqtt(etherHeader *ether, socket *s)
{
    // MQTT "Header"
    uint8_t buffer[MAX_BUFF_SIZE];
    mqttHeader* mqtt = (mqttHeader*) buffer;

    mqtt->headerFlags = 0x10;   // Connect Flag

    // MQTT Connect Payload
    mqttConnect *payload = (mqttConnect*) mqtt->lengthPayload;

    payload->protocolNameLength = htons(0x0004);    // Length of MQTT
    payload->protocolName[0] = 0x4D;                // M
    payload->protocolName[1] = 0x51;                // Q
    payload->protocolName[2] = 0x54;                // T
    payload->protocolName[3] = 0x54;                // T
    payload->version = 0x04;                        // Version v3.1.1
    payload->connectFlags = 0x02;                   // Clean session
    payload->keepAlive = htons(0x003c);             // Keep alive 60s
    payload->clientIdLength = htons(0x0);           // Length of client ID

    // adjust lengths
    mqtt->msgLen = sizeof(mqttConnect);        
    uint8_t dataSize = sizeof(mqttHeader) + mqtt->msgLen;

    // Send the MQTT Connect message
    sendTcpMessage(ether, s, PSH | ACK, (uint8_t *)mqtt, dataSize);
}

void disconnectMqtt(etherHeader *ether, socket *s)
{
    // MQTT "Header"
    uint8_t buffer[MAX_BUFF_SIZE];
    mqttHeader* mqtt = (mqttHeader*) buffer;

    mqtt->headerFlags = 0x10;   // Connect Flag
    mqtt->lengthPayload[0] = 0x0;

    // adjust lengths
    mqtt->msgLen = 0x0;    
    uint8_t dataSize = sizeof(mqttHeader) + mqtt->msgLen;

    sendTcpMessage(ether, s, PSH | ACK, (uint8_t *)mqtt, dataSize);
}

void publishMqtt(etherHeader *ether, socket *s, char strTopic[], char strData[])
{
    // MQTT "Header"
    uint8_t buffer[MAX_BUFF_SIZE];
    mqttHeader* mqtt = (mqttHeader*) buffer;
    uint16_t payloadSize = 0;
    char *strPtr;
    char *payloadPtr;

    mqtt->headerFlags = 0x30;   // Connect Flag

    // MQTT Publish Payload
    mqttPublish *payload = (mqttPublish*) mqtt->lengthPayload;

    strPtr = strTopic;  // Point to the start of topic to be copied
    payloadPtr = payload->topic; // Point to the start of topic in payload

    // Topic name loop
    while (*strPtr)
    {
        payloadSize++;          // Increment the payload size
        *payloadPtr = *strPtr;  // Copy the topic name
        
        // Move pointer
        payloadPtr++;
        strPtr++;
    }

    payload->topicLength = htons(payloadSize); // Set the topic length

    strPtr = strData;  // Point to the start of message to be copied
    payloadPtr++; // Point to the start of message in payload

    // FIXME: Adjust length for message (cuts off last character)
    // Message loop
    while (*strPtr)
    {
        payloadSize++;          // Increment the payload size
        *payloadPtr = *strPtr;  // Copy the message
        
        // Move pointer
        payloadPtr++;
        strPtr++;
    }

    // adjust lengths
    mqtt->msgLen = sizeof(mqttPublish) + payloadSize;
    uint8_t dataSize = sizeof(mqttHeader) + mqtt->msgLen;

    // Send the MQTT Connect message
    sendTcpMessage(ether, s, PSH | ACK, (uint8_t *)mqtt, dataSize);
}

void subscribeMqtt(etherHeader *ether, socket *s, char strTopic[])
{

}

void unsubscribeMqtt(etherHeader *ether, socket *s, char strTopic[])
{

}

