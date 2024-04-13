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
#include <string.h>
#include "arp.h"
#include "tcp.h"
#include "dhcp.h" //Make a function in dhcp.h/c to return the hardware address of the server. This will be used in sendTcpMessage. serverHW_Address is part of Ethernet frame
#include "timer.h"

// ------------------------------------------------------------------------------
//  Globals
// ------------------------------------------------------------------------------

#define MAX_TCP_PORTS 4
#define TCP 6
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

uint16_t tcpPorts[MAX_TCP_PORTS];
uint8_t tcpPortCount = 0;
uint8_t tcpState[MAX_TCP_PORTS];
uint32_t sequenceNumber = 0;// This might need to be randomly generated. Sequences should not start with value of zero as they become vulnerable to attacks.
//Things to think about:
//Maybe have a function to generate sequence number.
//socket gets information during state machine/connection
//tcp->offsetFields will need to be or'ed with ACK|flags


// ------------------------------------------------------------------------------
//  Structures
// ------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------





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

//similar to getOptions in DHCP. Getting ptr of TCP
uint8_t *getTCPHeaderPtr(etherHeader *ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    //uint8_t ipHeaderLength = ip->size * 4;
    //TCP header frame
    tcpHeader* tcp = (tcpHeader*)((uint8_t*)ip+(ip->size*4));
    return (uint8_t*)tcp;
}

bool isTcpAck(etherHeader *ether)
{
     tcpHeader *tcp = (tcpHeader*)getTCPHeaderPtr(ether);
     if((tcp->offsetFields & ACK) == ACK){
         return true;
     }else{
         return false;
    }
}

void sendTcpPendingMessages(etherHeader *ether)
{
//        //State machine???
//    uint8_t instance = MAX_TCP_PORTS;
//    while(instance < MAX_TCP_PORTS)
//    {
//        if(getTcpState[instance] == TCP_CLOSED){
//
//        }
//        i++
//    }
   uint8_t i = 0;
   uint8_t data[1024] ={0};
   uint16_t dataSize = 1000;
  // TCB*  connections[MAX_TCP_PORTS];
   socket * soc;
   tcpHeader *tcp = (tcpHeader*)getTCPHeaderPtr(ether);
  // uint8_t i = 0;
//
  // case TCP_CLOSED;
   uint8_t serverIp[4];


           if(getTcpState(0) == TCP_CLOSED){
               for(i = 0; i < IP_ADD_LENGTH; i++)
               {
                   soc->remoteIpAddress[i] = serverIp[i];//remote ip of device
               }
               //soc->remoteIpAddress
               //sendTcpMessage(etherHeader *ether, socket *s, uint16_t flags, uint8_t data[], uint16_t dataSize)
               //uint16_t syn = SYN;
               sendTcpMessage(ether, soc, SYN, data, dataSize);
               setTcpState(0, TCP_SYN_SENT);
           }else if((getTcpState(0) == TCP_SYN_SENT) && (isTcpAck(ether))==true)
           {

               setTcpState(0, TCP_SYN_SENT);
           }


//       }break;
//       case TCP_SYN_SENT;
//       {
//       }break;
//       case TCP_ESTABLISHED;
//       {
//       }break;
//       case TCP_FIN_WAIT_1;
//       {
//       }break;
//       case TCP_FIN_WAIT_2;
//       {
//       }break;
//       case TCP_CLOSING;
//       {
//       }break;
//       case TCP_CLOSE_WAIT;
//       {
//       }break;
//       case TCP_LAST_ACK;
//       {
//       }break;
//       case TCP_TIME_WAIT;
//       {
//       }break;

}

void processTcpResponse(etherHeader *ether)
{
}

void processTcpArpResponse(etherHeader *ether)
{

}

void setTcpPortList(uint16_t ports[], uint8_t count)
{
}

bool isTcpPortOpen(etherHeader *ether)
{
    return false;
}

void sendTcpResponse(etherHeader *ether, socket* s, uint16_t flags)
{
}



// How to get socket?
// dataSize?
// Send TCP message
void sendTcpMessage(etherHeader *ether, socket *s, uint16_t flags, uint8_t data[], uint16_t dataSize)
{
    uint16_t tcpHeaderLength = 0;
    uint8_t localHwAddress[6];
    uint8_t localIpAddress[4];
    uint8_t i = 0;
    uint32_t sum;
    uint16_t tmp16;
    uint16_t tcpLength = 0;

    // Ether frame
    getEtherMacAddress(localHwAddress);
    for(i = 0; i < HW_ADD_LENGTH;i++)
    {
        //ether->destAddress[i] = //Need to figure this one out. Maybe socket *s?
        ether->destAddress[i] = s->remoteHwAddress[i]; // I think this is correct -r
        ether->sourceAddress[i] = localHwAddress[i];
    }
    ether->frameType = htons(TYPE_IP);

    // IP header
    ipHeader* ip = (ipHeader*)ether->data;
    ip->rev = 0x4;
    ip->size = 0x5;
    ip->typeOfService = 0;
    ip->id = 0;
    ip->flagsAndOffset = 0;
    ip->ttl = 64; //64 | 128? I think, not sure.verify!!!
    // in lab, other TCP connections use 64 so its probably fine -r
    ip->protocol = PROTOCOL_TCP;
    ip->headerChecksum = 0;
    for(i = 0; i < IP_ADD_LENGTH; i++)
    {
        ip->sourceIp[i] = localIpAddress[i];
        ip->destIp[i] = s->remoteIpAddress[i]; //once again this might be with socket variable *s
        // I think this is also correct -r
    }
    uint8_t ipHeaderLength = ip->size * 4;

    // TCP header
    tcpHeader* tcp = (tcpHeader*)((uint8_t*)ip+(ip->size*4));

    tcp->sourcePort = htons(s->localPort);
    tcp->destPort = htons(s->remotePort);
    tcp->sequenceNumber = htonl(s->sequenceNumber);
    tcp->acknowledgementNumber = htonl(s->acknowledgementNumber);
 
    // TODO: offsetFields, data

    tcp->offsetFields = 0;
    //tcp->offsetFields This is 16bit flag. Some bits need to be changed in here.
    setFlags(tcp->offsetFields,flags);
    tcp->offsetFields &= ~(0xF000);
    tcpHeaderLength = ((sizeof(tcpHeader) / 4) << OFS_SHIFT);
    tcp->offsetFields |= tcpHeaderLength;
    tcp->offsetFields = htons(tcp->offsetFields);


    // checksum
    // adjust lengths
    tcpLength = sizeof(tcpHeader) + dataSize;
    ip->length = htons(sizeof(ipHeader) + tcpLength);

       // 32-bit sum over ip header
       calcIpChecksum(ip);

    // set tcp length
    // probably don't have to do this -r

       // 32-bit sum over pseudo-header
       sum = 0;
       sumIpWords(ip->sourceIp, 8, &sum);
       tmp16 = ip->protocol;
       sum += (tmp16 & 0xff) << 8;
       sumIpWords(&tmp16, 2, &sum);

    // add tcp header
       tcp->checksum = 0;
       sumIpWords(tcp, tcpLength, &sum);
       tcp->checksum = getIpChecksum(sum);

    // send packet with size = ether + udp hdr + ip header + udp_size
    putEtherPacket(ether, sizeof(etherHeader) + ipHeaderLength + tcpLength);

    sequenceNumber += dataSize;
}
