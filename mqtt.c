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
#include "eth0.h"
#include "tcp.h"
#include "mqtt.h"
#include "timer.h"


//IMPORTANT
//Code QS1 for MQTT
//TYPES OF MESSAGES
#define MQTT_CONNECT 0
#define MQTT_CONNACK 1
#define MQTT_PUBLISH 2
#define MQTT_PUBACK 3
#define MQTT_PUBREC 4
#define MQTT_PUBREL 5
#define MQTT_PUBCOMP 6
#define MQTT_SUBSCRIBE 7
#define MQTT_SUBACK 8
#define MQTT_UNSUBSCRIBE 9
#define MQTT_UNSUBACK 10
#define MQTT_PINGREQ 11
#define MQTT_PINGRESP 12
#define MQTT_DISCONNECT 13
#define MQTT_AUTH 14





// ------------------------------------------------------------------------------
//  Globals
// ------------------------------------------------------------------------------
//bool mqttOptionFieldFlags = true;//
uint8_t mqttMessageType = 0;
// ------------------------------------------------------------------------------
//  Structures
// ------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

//void connectMqtt(etherHeader *ether, uint8_t *data, uint16_t size)
//{
//}
void setMqttState(uint8_t state)
{
    mqttMessageType = state;
}

uint8_t getMqttState()
{
    return mqttMessageType;
}
void connectMqtt()
{

    setMqttState(MQTT_CONNECT);

}
//etherHeader *ether, uint8_t *data, uint16_t size
void sendMqttMessage(etherHeader *ether, socket *s, uint16_t flags, uint8_t data[], uint16_t dataSize)
{

    uint16_t tcpHeaderLength = 0;
    uint8_t localHwAddress[6];
    uint8_t localIpAddress[4];
        uint8_t i =0;
        uint32_t sum;
        uint16_t tmp16;
        uint16_t tcpLength = 0;
        //Part of ethernet Frame
        getEtherMacAddress(localHwAddress);
        for(i = 0; i < HW_ADD_LENGTH;i++)
        {
            //ether->destAddress[i] = ; //Need to figure this one out. Maybe socket *s?
            ether->destAddress[i] = s->remoteHwAddress[i];
            ether->sourceAddress[i] = localHwAddress[i];
        }

        ether->frameType = htons(TYPE_IP);

        //
        // IP header
        ipHeader* ip = (ipHeader*)ether->data;
        ip->rev = 0x4;
        ip->size = 0x5;
        ip->typeOfService = 0;
        ip->id = 0;

        //ip->flagsAndOffset = htons(16384);
        ip->flagsAndOffset = 0;
        ip->ttl = 128; //64 | 128? I think, not sure.verify!!!
        ip->protocol = PROTOCOL_TCP;
        ip->headerChecksum = 0;
        uint8_t ipHeaderLength = ip->size * 4;
        getIpAddress(localIpAddress);
        for(i = 0; i < IP_ADD_LENGTH; i++)
        {
            ip->sourceIp[i] = localIpAddress[i];
            //snprintf(str, sizeof(str), "Remote ip address %d\n",s->remoteIpAddress[i]);
            //putsUart0(str);
            ip->destIp[i] = s->remoteIpAddress[i]; //once again this might be with socket variable *s

        }

        //TCP header frame
        tcpHeader* tcp = (tcpHeader*)((uint8_t*)ip+(ip->size*4));
        tcp->sourcePort = htons(s->localPort); //use htons here?
        tcp->destPort = htons(s->remotePort); //
        tcp->sequenceNumber = s->sequenceNumber;
        tcp->acknowledgementNumber = s->acknowledgementNumber;
        //tcp->acknowledgementNumber = ;
        tcp->windowSize = 1;//htons(0); //how far back I can look to give you stuff that is missing. Small window due to constrains in the redboard.
        tcp->urgentPointer = 0;
        tcp->offsetFields = 0;
        //tcp->offsetFields This is 16bit flag. Some bits need to be changed in here. T
        setFlags(tcp->offsetFields,flags);
        //tcpLength = sizeof(tcpHeader)+dataSize;
        tcpLength = sizeof(tcpHeader)+dataSize; //TCP syn has no data

        //clears the first 4 upper bits
        tcp->offsetFields &= ~(0xF000);
        tcpHeaderLength = ((sizeof(tcpHeader)/4) << OFS_SHIFT);
        //tcpHeaderLength = (sizeof(tcpHeader)/4);
        tcp->offsetFields |= tcpHeaderLength;
        tcp->offsetFields =htons(tcp->offsetFields);


        //char client[] = "smartPlant";
            //tcpHeader *tcp = (tcpHeader*)getTCPHeaderPtr(ether);
            mqttHeader *mqtt=(mqttHeader*)tcp->data;
            mqtt->headerFlags = 0x10;
            mqtt->messageLength = 37;
            mqtt->protocolNameLength = htons(6);
            //mqtt->protocolName = "MQIsdp";
            strcpy(mqtt->protocolName,"MQIsdp");
            mqtt->version = htons(5);
            mqtt->connectFlags = 0x02;
            mqtt->keepAlive = htonl(5);
            mqtt->clientIdLength = htons(3);
            mqtt->clientId = 999;



        ip->length = htons(sizeof(ipHeader)+tcpLength);
           // 32-bit sum over ip header
           calcIpChecksum(ip);
           // set udp length
          // udp->length = htons(udpLength);


           // 32-bit sum over pseudo-header
           sum = 0;
           sumIpWords(ip->sourceIp, 8, &sum);
           tmp16 = ip->protocol;
           sum += (tmp16 & 0xff) << 8;
           tmp16 = htons(tcpLength);
           sumIpWords(&tmp16, 2, &sum);
           // add udp header
           tcp->checksum = 0;
           sumIpWords(tcp, tcpLength, &sum);
           tcp->checksum = getIpChecksum(sum);
           putEtherPacket(ether, sizeof(etherHeader) + ipHeaderLength + tcpLength);
         //  sequenceNumber += dataSize;


}
void disconnectMqtt()
{
}

void publishMqtt(char strTopic[], char strData[])
{
}

void subscribeMqtt(char strTopic[])
{
}

void unsubscribeMqtt(char strTopic[])
{
}
