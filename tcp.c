// Nestor Arteaga and Rolando Rosales
// TCP Library
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
#include "led_builtin.h" // just for debugging, remove when finished
#include "wait.h"

#define MAX_TCP_PORTS 4

//-----------------------------------------------------------------------------
//  Globals
//-----------------------------------------------------------------------------

uint16_t tcpPorts[MAX_TCP_PORTS];
uint8_t tcpPortCount = 0;
uint8_t tcpState[MAX_TCP_PORTS];

bool synNeeded = false;
bool ackNeeded = false;
bool arpNeeded = false;
bool finNeeded = false;

//-----------------------------------------------------------------------------
//  Structures
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void sendAck(void)
{
    ackNeeded = true;
}

void sendTcpFin(void)
{
    finNeeded = true;
}

void sendTcpArpRequest(void)
{
    arpNeeded = true;
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

void restartTcpStateMachine(void)
{
    synNeeded = false;
    ackNeeded = false;
    arpNeeded = false;
    finNeeded = false;
    setTcpState(0, TCP_CLOSED);
}

void callbackNotEstablished(void)
{
    restartTcpStateMachine();
}

//similar to getOptions in DHCP. Getting ptr of TCP
tcpHeader* getTcpHeaderPtr(etherHeader *ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    return (tcpHeader*)((uint8_t*)ip + (ip->size * 4));
}

void updateTcpSeqAck(etherHeader *ether, socket *s)
{
    tcpHeader *tcp = getTcpHeaderPtr(ether);
    s->acknowledgementNumber = ntohl(tcp->sequenceNumber) + 1;
    s->sequenceNumber = ntohl(tcp->acknowledgementNumber);
}

// Determines whether packet is TCP packet
// Must be an IP packet
// Additionally, this fn makes sure that packet is ours (hw addr)
bool isTcp(etherHeader* ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    uint8_t localHwAddress[6];
    uint8_t i = 0;

    getEtherMacAddress(localHwAddress);

    if (ip->protocol == PROTOCOL_TCP)
    {
        for (i = 0; i < HW_ADD_LENGTH; i++)
        {
            if (ether->destAddress[i] != localHwAddress[i])
            {
                return false;
            }
        }
        return true;
    }
    
    return false;
}

// TODO: isTcpSyn is now fixed, but may need further testing
bool isTcpSyn(etherHeader *ether)
{
    tcpHeader *tcp = getTcpHeaderPtr(ether);
    return ((tcp->offsetFields & htons(SYN)) == htons(SYN)) ? true : false;
}

// TODO: isTcpAck is now fixed, but may need further testing
bool isTcpAck(etherHeader *ether)
{
    tcpHeader *tcp = getTcpHeaderPtr(ether);
    return ((tcp->offsetFields & htons(ACK)) == htons(ACK)) ? true : false;
}

// TODO: isTcpAck is now fixed, but may need further testing
bool isTcpFin(etherHeader *ether)
{
    tcpHeader *tcp = getTcpHeaderPtr(ether);
    return ((tcp->offsetFields & htons(FIN)) == htons(FIN)) ? true : false;
}

bool isTcpRst(etherHeader *ether)
{
    tcpHeader *tcp = getTcpHeaderPtr(ether);
    return ((tcp->offsetFields & htons(RST)) == htons(RST)) ? true : false;
}

// TODO: finish sendTcpPendingMessages state machine
void sendTcpPendingMessages(etherHeader *ether, socket *s)
{
    if (arpNeeded)
    {
        // TODO: Add timer functionality to initial arp request
        uint8_t localIpAddress[4];
        uint8_t ipGwAddress[4];

        getIpAddress(localIpAddress);
        getIpGatewayAddress(ipGwAddress);
        
        sendArpRequest(ether, localIpAddress, ipGwAddress);
        arpNeeded = false;

        startOneshotTimer(callbackNotEstablished, 5);
    }
    if (synNeeded)
    {
        sendTcpMessage(ether, s, SYN, 0, 0);
        setTcpState(0, TCP_SYN_SENT);
        synNeeded = false;
    }
    if (ackNeeded)
    {
        sendTcpMessage(ether, s, ACK, 0, 0);
        ackNeeded = false;
    }
    if (finNeeded)
    {
        sendTcpMessage(ether, s, FIN | ACK, 0, 0);
        finNeeded = false;
    }
}

// TODO: finish processTcpResponse state machine
void processTcpResponse(etherHeader *ether, socket *s)
{
    if (isTcpRst(ether))
    {
        setTcpState(0, TCP_CLOSED);
        s->localPort += 1;
        return;
    }
    
    switch (getTcpState(0))
    {
        case TCP_CLOSED:
            break;
        case TCP_SYN_SENT:
            if (isTcpAck(ether) && isTcpSyn(ether))
            {
                updateTcpSeqAck(ether, s);

                stopTimer(callbackNotEstablished);
                setTcpState(0, TCP_ESTABLISHED);
                enableRedLED();
                
                ackNeeded = true;
            }
            break;
        case TCP_ESTABLISHED:
            if (isTcpFin(ether))
            {
                updateTcpSeqAck(ether, s);

                ackNeeded = true;
                setTcpState(0, TCP_CLOSE_WAIT);
            }

            // Not waiting
            if (getTcpState(0) != TCP_CLOSE_WAIT)
            {
                break;
            }
        case TCP_CLOSE_WAIT:
            updateTcpSeqAck(ether, s);

            finNeeded = true;
            setTcpState(0, TCP_LAST_ACK);
            break;
        case TCP_FIN_WAIT_1:
            if (isTcpAck(ether))
            {
                setTcpState(0, TCP_FIN_WAIT_2);
            }
            if (isTcpFin(ether))
            {
                ackNeeded = true;
                setTcpState(0, TCP_CLOSING);
            }
            break;
        case TCP_FIN_WAIT_2:
            if (isTcpFin(ether))
            {
                ackNeeded = true;
                setTcpState(0, TCP_TIME_WAIT);
                // Start time-wait timer
            }
            break;
        case TCP_CLOSING:
            if (isTcpAck(ether))
            {
                setTcpState(0, TCP_TIME_WAIT);
                // Start time-wait timer
            }
            break;
        case TCP_TIME_WAIT:
            // Closed by timer
            break;
        case TCP_LAST_ACK:
            if (isTcpAck(ether))
            {
                disableRedLED();
                setTcpState(0, TCP_CLOSED);
                s->localPort += 1;
            }
            break;
    }
}

// TODO: Make sure processTcpArp works correctly
// This is where we will get the hardware address
void processTcpArpResponse(etherHeader *ether, socket *s)
{
    if (getTcpState(0) == TCP_CLOSED)
    {
        arpPacket *arp = (arpPacket*)ether->data;
        uint8_t i;

        for (i = 0; i < HW_ADD_LENGTH; i++)
        {
            s->remoteHwAddress[i] = arp->sourceAddress[i];
        }
        
        synNeeded = true;  
    }
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

    tcp->windowSize = htons(1500); //how far back I can look to give you stuff that is missing. Small window due to constrains in the redboard.
    // changed it to 1500 based on Dr. Losh comments during lecture -r

    tcp->urgentPointer = htons(0);

    // Copy passed in data into tcp struct
    for (i = 0; i < dataSize; i++)
    {
        tcp->data[i] = data[i];
    }

    // Increment seq num by data size
    s->sequenceNumber += dataSize;

    // adjust lengths
    tcpLength = sizeof(tcpHeader) + dataSize;
    ip->length = htons(ipHeaderLength + tcpLength);

    // 32-bit sum over ip header
    calcIpChecksum(ip);

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

    // send packet
    putEtherPacket(ether, sizeof(etherHeader) + ipHeaderLength + tcpLength);
}
