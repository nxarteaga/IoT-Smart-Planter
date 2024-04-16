// TCP Library
// Modified by Nestor Arteaga and Rolando Rosales (1001850424)
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
#include "timer.h"
#include "dhcp.h" //Make a function in dhcp.h/c to return the hardware address of the server. This will be used in sendTcpMessage. serverHW_Address is part of Ethernet frame
#include "gpio.h"

//-----------------------------------------------------------------------------
//  Globals
//-----------------------------------------------------------------------------

#define MAX_TCP_PORTS 4

#define GREEN_LED PORTF,3

uint16_t tcpPorts[MAX_TCP_PORTS];
uint8_t tcpPortCount = 0;
uint8_t tcpState[MAX_TCP_PORTS];
uint32_t sequenceNumber = 0;// This might need to be randomly generated. Sequences should not start with value of zero as they become vulnerable to attacks.
//Things to think about:
//Maybe have a function to generate sequence number.
//socket gets information during state machine/connection
//tcp->offsetFields will need to be or'ed with ACK|flags

//-----------------------------------------------------------------------------
//  Structures
//-----------------------------------------------------------------------------

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

//similar to getOptions in DHCP. Getting ptr of TCP
tcpHeader* getTcpHeaderPtr(etherHeader *ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    return (tcpHeader*)((uint8_t*)ip + (ip->size * 4));
}

// Determines whether packet is TCP packet
// Must be an IP packet
// Makes sure that packet is ours
bool isTcp(etherHeader* ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    uint8_t localHwAddress[6];
    uint8_t i = 0;
    bool ok = true;

    getEtherMacAddress(localHwAddress);

    if (ip->protocol == PROTOCOL_TCP)
    {
        for (i = 0; i < HW_ADD_LENGTH; i++)
        {
            if (ether->destAddress[i] != localHwAddress[i])
            {
                ok = false;
                break;
            }
        }
    }
    else
    {
        ok = false;
    }
    
    return ok;
}

bool isTcpSyn(etherHeader *ether)
{
    tcpHeader *tcp = getTcpHeaderPtr(ether);
    return ((tcp->offsetFields & SYN) == SYN) ? true : false;
}

bool isTcpAck(etherHeader *ether)
{
    tcpHeader *tcp = getTcpHeaderPtr(ether);
    return ((tcp->offsetFields & ACK) == ACK) ? true : false;
}

// TODO: write sendTcpPendingMessages state machine
void sendTcpPendingMessages(etherHeader *ether, socket *s)
{
    switch (getTcpState(0))
    {
        case TCP_CLOSED:
            sendTcpMessage(ether, s, SYN, 0, 0);
            setTcpState(0, TCP_SYN_SENT);
            break;
        
        case TCP_SYN_SENT:
            if (isTcp(ether))
            {
                sendTcpMessage(ether,s, ACK, 0, 0);
                setTcpState(0, TCP_ESTABLISHED);
            }
            break;

        case TCP_ESTABLISHED:
            setPinValue(GREEN_LED, 1);
            break;
    }
}

// TODO: write processTcpResponse function
void processTcpResponse(etherHeader *ether)
{
    if (isTcp(ether))
    {
        if (isTcpSyn(ether))
        {
            // do something
        }
        if (isTcpAck(ether))
        {
            // do something
        }
    }
}

// TODO: write processTcpArpResponse function
// This is where we will get the MAC and IP addresses
void processTcpArpResponse(etherHeader *ether)
{

}

// TODO: write setTcpPortList function
void setTcpPortList(uint16_t ports[], uint8_t count)
{
}

// TODO: write isTcpPortOpen function
bool isTcpPortOpen(etherHeader *ether)
{
    return false;
}

// TODO: write sendTcpResponse function
void sendTcpResponse(etherHeader *ether, socket* s, uint16_t flags)
{
}

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
    ip->flagsAndOffset = htons(0x4000); // "Don't Fragment" (DF) flag
    ip->ttl = 64; //64 | 128? I think, not sure.verify!!!
    // in lab, other TCP connections use 64 so its probably fine -r
    ip->protocol = PROTOCOL_TCP;
    ip->headerChecksum = 0;

    getIpAddress(localIpAddress);
    for(i = 0; i < IP_ADD_LENGTH; i++)
    {
        ip->sourceIp[i] = localIpAddress[i];
        ip->destIp[i] = s->remoteIpAddress[i]; //once again this might be with socket variable *s
        // I think this is also correct -r
    }
    uint8_t ipHeaderLength = ip->size * 4;

    // TCP header
    tcpHeader* tcp = (tcpHeader*)((uint8_t*)ip + (ip->size * 4));

    // Ports
    tcp->sourcePort = htons(s->localPort);
    tcp->destPort = htons(s->remotePort);

    // Seq/Ack nums
    tcp->sequenceNumber = htonl(s->sequenceNumber);
    tcp->acknowledgementNumber = htonl(s->acknowledgementNumber);

    // Sets data option and flag bits
    tcp->offsetFields = htons(((sizeof(tcpHeader) / 4) << OFS_SHIFT) | flags);

    tcp->windowSize = htons(1522); //how far back I can look to give you stuff that is missing. Small window due to constrains in the redboard.
    // changed it to 1500 based on Dr. Losh comments during lecture -r

    tcp->urgentPointer = htons(0);

    // Copy passed in data into tcp struct
    for (i = 0; i < dataSize; i++)
    {
        tcp->data[i] = data[i];
    }

    // adjust lengths
    tcpLength = sizeof(tcpHeader) + dataSize;
    ip->length = htons(ipHeaderLength + tcpLength);

    // 32-bit sum over ip header
    calcIpChecksum(ip);

    // set tcp length
    // no length in tcp struct

    // 32-bit sum over pseudo-header
    sum = 0;
    sumIpWords(ip->sourceIp, 8, &sum);
    tmp16 = ip->protocol;
    sum += (tmp16 & 0xff) << 8;
    tmp16 = htons(tcpLength);
    sum += tmp16;

    // add tcp header
    tcp->checksum = 0;
    sumIpWords(tcp, tcpLength, &sum);
    tcp->checksum = getIpChecksum(sum);

    // send packet with size = ether + udp hdr + ip header + udp_size
    putEtherPacket(ether, sizeof(etherHeader) + ipHeaderLength + tcpLength);
}
