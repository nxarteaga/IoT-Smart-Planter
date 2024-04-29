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

bool mqttConnected = false;

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
    payload->connectFlags = 0x02;                   // Clean session... More suitable for Publish only client.
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

    mqttConnected = false;

    // MQTT "Header"
    uint8_t buffer[MAX_BUFF_SIZE];
    mqttHeader* mqtt = (mqttHeader*) buffer;

    mqtt->headerFlags = 0xE0;   // Connect Flag
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
    while (*strPtr != NULL)
    {
        payloadSize++;          // Increment the payload size
        *payloadPtr = *strPtr;  // Copy the topic name
        
        // Move pointer
        payloadPtr++;
        strPtr++;
    }

    payload->topicLength = htons(payloadSize); // Set the topic length

    strPtr = strData;  // Point to the start of message to be copied
    // payloadPtr += 1; // Point to the start of message in payload

    // Message loop
    while (*strPtr != NULL)
    {
        payloadSize++;          // Increment the payload size
        *payloadPtr = *strPtr;  // Copy the message
        
        // Move pointer
        payloadPtr++;
        strPtr++;
    }
    // payloadSize += 1;
  
    // adjust lengths
    mqtt->msgLen = sizeof(mqttPublish) + payloadSize;
    uint8_t dataSize = sizeof(mqttHeader) + mqtt->msgLen;

    // Send the MQTT Connect message
    sendTcpMessage(ether, s, PSH | ACK, (uint8_t *)mqtt, dataSize);
}

void subscribeMqtt(etherHeader *ether, socket *s, char strTopic[])
{
    // MQTT "Header"
    uint8_t buffer[MAX_BUFF_SIZE];
    mqttSubscribeHeader* mqtt = (mqttSubscribeHeader*) buffer;
    uint16_t payloadSize = 0;
    char *strPtr;
    char *payloadPtr;

    mqtt->headerFlags = 0x82;   // Connect Flag
    mqtt->MessageIdentifier = htons(10);

    // MQTT Publish Payload
    mqttSubscribe *payload = (mqttSubscribe*) mqtt->lengthPayload;

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

    *payloadPtr = 0x0; //assigns the value 0 to QoS

     // adjust lengths
    payload->topicLength = htons(payloadSize);// Set the topic length
    mqtt->msgLen = (sizeof(mqttSubscribeHeader) + payloadSize+1); //add 1 to payload when not using pointer
    uint8_t dataSize = (sizeof(mqttSubscribe) + mqtt->msgLen);//size of pointer is 8bytes
    dataSize = dataSize - 2;
    // Send the MQTT Connect message
    sendTcpMessage(ether, s, PSH | ACK, (uint8_t *)mqtt, dataSize);
}

void unsubscribeMqtt(etherHeader *ether, socket *s, char strTopic[])
{
    // MQTT "Header"
    uint8_t buffer[MAX_BUFF_SIZE];
    mqttSubscribeHeader* mqtt = (mqttSubscribeHeader*) buffer;
    uint16_t payloadSize = 0;
    char *strPtr;
    char *payloadPtr;

    mqtt->headerFlags = 0xA2;   // Unsubscribe
    mqtt->MessageIdentifier = htons(11);

    // MQTT Publish Payload
    mqttSubscribe *payload = (mqttSubscribe*) mqtt->lengthPayload;

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

    //*payloadPtr = 0x0; //assigns the value 0 to QoS.. Not needed for unsubscribed

     // adjust lengths
    payload->topicLength = htons(payloadSize);// Set the topic length
    mqtt->msgLen = (sizeof(mqttSubscribeHeader) + payloadSize); //
    uint8_t dataSize = (sizeof(mqttSubscribe) + mqtt->msgLen);//size of pointer is 8bytes
    dataSize = dataSize - 1;
    // Send the MQTT Connect message
    sendTcpMessage(ether, s, PSH | ACK, (uint8_t *)mqtt, dataSize);
}

void checkMqttConAck(etherHeader *ether, socket *s)
{
    tcpHeader* tcp = getTcpHeaderPtr(ether);
    uint8_t headerFlags = tcp->data[0];

    if (headerFlags == 0x20)
    {
        // updateTcpSeqAck(ether, s);
        s->acknowledgementNumber += 4;
        sendAck();
        mqttConnected = true;
    }
    else
    {
        mqttConnected = false;
    }
}

bool isMqttConAcked()
{
    return mqttConnected;
}

