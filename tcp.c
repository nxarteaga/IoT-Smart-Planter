// TCP Library
// Modified by Nestor Arteaga
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
#include "uart0.h"
#include <string.h>
#include "arp.h"
#include "tcp.h"
#include "dhcp.h" //Make a function in dhcp.h/c to return the hardware address of the server. This will be used in sendTcpMessage. serverHW_Address is part of Ethernet frame
#include "timer.h"
#include "mqtt.h"

// ------------------------------------------------------------------------------
//  Globals
// ------------------------------------------------------------------------------

#define MAX_TCP_PORTS 4
#define TCP 6
#define FIN_ACK 0x0011
#define PSH_ACK 0x0018
#define idk 0010000000000000
//// TCP states
//#define TCP_CLOSED 0
//#define TCP_LISTEN 1
//#define TCP_SYN_RECEIVED 2
//#define TCP_SYN_SENT 3
//#define TCP_ESTABLISHED 4
//#define TCP_FIN_WAIT_1 5
//#define TCP_FIN_WAIT_2 6
//#define TCP_CLOSING 7
//#define TCP_CLOSE_WAIT 8
//#define TCP_LAST_ACK 9
//#define TCP_TIME_WAIT 10
bool SYN_ACK_RECEIVED = false;
bool FIN_ACK_RECEIVED_GoToCloseWait = false;
bool remainClosed = false;

uint32_t finAcknowledgementValue =0;
uint32_t finSequenceValue = 0;
uint16_t tcpPorts[MAX_TCP_PORTS];
uint8_t tcpPortCount = 0;
uint8_t tcpState[MAX_TCP_PORTS];
uint32_t sequenceNumber = 0;// This might need to be randomly generated. Sequences should not start with value of zero as they become vulnerable to attacks.
uint32_t sourcePort = 4910; //MQTT address is 127.0.0.1
uint32_t remotePort = 1883;
uint32_t ack = 0;

char str[40];
//Things to think about:
//The maximum segment size (MSS) option (2) is sent with the original SYN and is usually 1280
//Maybe have a function to generate sequence number.
//socket gets information during state machine/connection
//tcp->offsetFields will need to be or'ed with ACK|flags


// ------------------------------------------------------------------------------
//  Structures
// ------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

//similar to getOptions in DHCP. Getting ptr of TCP
uint8_t *getTCPHeaderPtr(etherHeader *ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    //uint8_t ipHeaderLength = ip->size * 4;
    //TCP header frame
    tcpHeader* tcp = (tcpHeader*)((uint8_t*)ip+(ip->size*4));
    return (uint8_t*)tcp;
}

void isSYN_ACK(etherHeader *ether){
    tcpHeader *tcp = (tcpHeader*)getTCPHeaderPtr(ether);
       if((htons(tcp->offsetFields) & ACK ) && (htons(tcp->offsetFields) & SYN))
       {
           SYN_ACK_RECEIVED = true;
           //return true;
       }

       //return false;
}

void processTcpResponse(etherHeader *ether)
{
    tcpHeader *tcp = (tcpHeader*)getTCPHeaderPtr(ether);
    if((htons(tcp->offsetFields) & ACK ) && (htons(tcp->offsetFields) & SYN))
    {
        SYN_ACK_RECEIVED = true;
    }else if((htons(tcp->offsetFields) & ACK ) && (htons(tcp->offsetFields) & FIN))
    {
        finAcknowledgementValue = tcp->sequenceNumber + htonl(1);
        finSequenceValue = tcp->acknowledgementNumber;
        FIN_ACK_RECEIVED_GoToCloseWait = true;
        setTcpState(0, TCP_CLOSE_WAIT);
    }else if((htons(tcp->offsetFields) & ACK ) && (getTcpState(0) == TCP_LAST_ACK))
    {

        if(tcp->sequenceNumber == finSequenceValue && tcp->acknowledgementNumber == finAcknowledgementValue){
            remainClosed = true;
            setTcpState(0, TCP_CLOSED);
        }

    }
}

void closeTCPconnection()
{

}


// Set TCP state
void setTcpState(uint8_t instance, uint8_t state)
{
    tcpState[instance] = state;
}

// Get TCP state
uint8_t getTcpState(uint8_t instance)
{
    return tcpState[instance];
}

// Determines whether packet is TCP packet
// Must be an IP packet
bool isTcp(etherHeader* ether)
{
      //uint8_t i = 0;
      // IP header
     ipHeader* ip = (ipHeader*)ether->data;
     if(ip->protocol == TCP)
     {
         return true;
     }else
     {
         return false;
     }

}

bool isTcpSyn(etherHeader *ether)
{
    return false;
}







bool isTcpAck(etherHeader *ether)
{
     tcpHeader *tcp = (tcpHeader*)getTCPHeaderPtr(ether);
     //Make sure to check for SYN flag as well
     //check the flags are 0x012
     //((tcp->offsetFields & ACK) == ACK)
     if((htons(tcp->offsetFields) & 0x0010) >> 4){
         return true;
     }else{
         return false;
    }
}

void establishSocket(etherHeader *ether,uint8_t ipFrom[], uint8_t ipTo[])
{
    ///uint8_t localIpAddress[4];
    //getIpAddress(localIpAddress);
    sendArpRequest(ether,ipFrom,ipTo);
}


void sendTcpPendingMessages(etherHeader *ether)
{

   uint8_t i = 0;
   uint8_t data[1024] ={0};
   uint16_t dataSize = 0;
  // TCB*  connections[MAX_TCP_PORTS];
   socket soc;
   tcpHeader *tcp = (tcpHeader*)getTCPHeaderPtr(ether);
  // uint8_t i = 0;
//
  //// case TCP_CLOSED;
   uint8_t serverIp[IP_ADD_LENGTH]={192,168,2,1};
//  uint8_t serverIp[IP_ADD_LENGTH]={192,168,2,1};
   //uint8_t serverIp[IP_ADD_LENGTH]={10,37,129,2};
   //uint8_t serverIp[IP_ADD_LENGTH]={127,0,0,1};
   //uint8_t remoteHwAddress[HW_ADD_LENGTH] = {0x0C,0xE4,0x41,0xF0,0xF3,0x77};//Remote address of M1
   uint8_t remoteHwAddress[HW_ADD_LENGTH] = {0x00,0xE0,0x4C,0x68,0x4D,0x50}; //Bridge
  // uint8_t remoteHwAddress[HW_ADD_LENGTH] = {0x00,0x1C,0x42,0xC4,0x34,0x47}; //Remote address of Virtual machine

  // uint8_t remoteHwAddress[HW_ADD_LENGTH] = {0xD4,0x5D,0x64,0xB2,0x1F,0x41}; //PC



           if(getTcpState(0) == TCP_CLOSED &&(remainClosed == false)){

               for(i = 0; i < IP_ADD_LENGTH; i++)
               {
                   soc.remoteIpAddress[i] = serverIp[i];//remote ip of device
               }
               soc.localPort = sourcePort;
               soc.remotePort = remotePort;
               //0c:e4:41:f0:f3:77

              for(i = 0;i < HW_ADD_LENGTH;i++)
              {
                  soc.remoteHwAddress[i] = remoteHwAddress[i];

              }
               soc.sequenceNumber = sequenceNumber;
               soc.acknowledgementNumber = 0;
               soc.state = TCP_SYN_SENT;
               ack = 0;
               sendTcpMessage(ether, &soc, SYN, data, dataSize);
               setTcpState(0, TCP_SYN_SENT);
           }else if((getTcpState(0) == TCP_SYN_SENT) && (SYN_ACK_RECEIVED == true))
           {

             // soc.sequenceNumber = tcp->sequenceNumber;

              //Server sends its ServerISN in packet tcp->sequenceNumber. The third step on three way handshake
               // will be an ACK. This ack will will have ACK # = ServerISN+1
              soc.acknowledgementNumber = tcp->sequenceNumber + htonl(1);
              //
              soc.sequenceNumber = tcp->acknowledgementNumber + htonl(1);
               ack = 1;
               //soc.acknowledgementNumber +=1;
               dataSize = 0;


               sendTcpMessage(ether, &soc, ACK, data, dataSize);
               SYN_ACK_RECEIVED = false;
              // sendACK(ether,&soc);
               setTcpState(0, TCP_ESTABLISHED);
           }else if(getTcpState(0) == TCP_ESTABLISHED)
           {
               //putsUart0("\nEstablish state\n");
               //send subscribed topics in here?
               //MQTT messages
               dataSize = 39;
               if(getMqttState() == 0)
               {
                   putsUart0("\nEstablish state\n");
                   sendMqttMessage(ether,&soc, PSH_ACK, data, dataSize);
                   setTcpState(0,1);
               }

           }else if(getTcpState(0) == TCP_FIN_WAIT_1)
           {
              // datasize = 0;
              // data = 0;
               //sendTcpMessage(ether, &soc, FIN, data, dataSize);
           }
           else if(getTcpState(0) == TCP_FIN_WAIT_2)
           {

           }else if(getTcpState(0) == TCP_CLOSING)
           {

           }else if((getTcpState(0) == TCP_CLOSE_WAIT) && (FIN_ACK_RECEIVED_GoToCloseWait == true)) //Passive Close
           {
//               data = 0;
               dataSize = 0;

               soc.acknowledgementNumber = finAcknowledgementValue;
               soc.sequenceNumber = finSequenceValue;

               sendTcpMessage(ether, &soc, FIN_ACK, data, dataSize);
               setTcpState(0, TCP_LAST_ACK);
           }
           else if(getTcpState(0) == TCP_LAST_ACK)  //Passive Close
           {
//               data = 0;
               dataSize = 0;
               soc.acknowledgementNumber = finAcknowledgementValue;
               soc.sequenceNumber = finSequenceValue;

               //sendTcpMessage(ether, &soc, FIN_ACK, data, dataSize);
//               remainClosed = true;

           }else if(getTcpState(0) == TCP_TIME_WAIT)
           {

           }

}



void processTcpArpResponse(etherHeader *ether) //if we are not given hardware address, then send ARP request.
{
    //sendArpRequest(ether,localIpAddress,dhcpOfferedIpAdd);
}

void setTcpPortList(uint16_t ports[], uint8_t count)
{
}



bool isTcpPortOpen(etherHeader *ether)
{
    tcpHeader *tcp = (tcpHeader*)getTCPHeaderPtr(ether);
    if(htons(tcp->offsetFields) & ACK){
        return true;
    }else{

        return false;
    }

}

void sendTcpResponse(etherHeader *ether, socket* s, uint16_t flags)
{


    //TCP handshake goes here
}



// How to get socket?
// dataSize?
// Send TCP message
void sendTcpMessage(etherHeader *ether, socket *s, uint16_t flags, uint8_t data[], uint16_t dataSize)
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
       sequenceNumber += dataSize;

}

