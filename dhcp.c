// DHCP Library
// Jason Losh
// Modified by Nestor Arteaga

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

// THINGS TO check
//ARP in my laptop probably needs to be reversed in order to work properly. At school ipFrom is DhcpOfferedAddress.
//UNICAST REQUEST on DHCP_RENEWING. check on ciaddr inside sendDhcpMessage,
//Configure rest of timers. Regular request is one.

#include <stdio.h>
#include "uart0.h"
#include "dhcp.h"
#include "arp.h"
#include "timer.h"
#include "ip.h"
#include <inttypes.h>
//TYPES OF MESSAGES
#define DHCPDISCOVER 1
#define DHCPOFFER    2
#define DHCPREQUEST  3
#define DHCPDECLINE  4
#define DHCPACK      5
#define DHCPNAK      6
#define DHCPRELEASE  7
#define DHCPINFORM   8
//STATES OF DHCP
#define DHCP_DISABLED   0    //IF DHCP off then
#define DHCP_INIT       1
#define DHCP_SELECTING  2
#define DHCP_REQUESTING 3
#define DHCP_TESTING_IP 4
#define DHCP_BOUND      5
#define DHCP_RENEWING   6
#define DHCP_REBINDING  7
#define DHCP_INITREBOOT 8 // not used since ip not stored over reboot
#define DHCP_REBOOTING  9 // not used since ip not stored over reboot

//#define lease 60

// Max packet is calculated as:
// Ether frame header (18) + Max MTU (1500)
#define MAX_PACKET_SIZE 1518
// ------------------------------------------------------------------------------
//  Globals
// ------------------------------------------------------------------------------

//uint32_t xid = 0xAAAAAAAA;
uint32_t xid = 0xAAAAbbbb;
uint32_t leaseSeconds = 0;
uint32_t leaseT1 = 0;
uint32_t leaseT2 = 0;
uint32_t requestCounter = 0; // to control the amount of times the request is submitted. It should be 4 times @ 15 secs. After 1min go back
char str[40];
// use these variables if you want
bool declineNeeded = false;
bool sendArpNeeded = false;
bool discoverNeeded = false;
bool requestNeeded = false;
bool requestUnicastNeeded = false;
bool releaseNeeded = false;
bool rebind = false;
bool timeOutOne = false;
//Testing timers
bool ready = false;

bool ipConflictDetectionMode = false;

//Use this to store IP for sending Request
uint8_t dhcpOfferedIpAdd[4];
uint8_t dhcpServerIpAdd[4];
uint8_t serverHwAddress[6];

uint8_t dhcpState = DHCP_DISABLED;
bool    dhcpEnabled = true;

// ------------------------------------------------------------------------------
//  Structures
// ------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// State functions

void setDhcpState(uint8_t state)
{
    dhcpState = state;
}

uint8_t getDhcpState()
{
    return dhcpState;
}

// New address functions
// Manually requested at start-up
// Discover messages sent every 15 seconds

void callbackDhcpGetNewAddressTimer()
{
}

void requestDhcpNewAddress()
{
}

// Renew functions
ResetAllFlags()
{
     declineNeeded = false;
    sendArpNeeded = false;
   // bool discoverNeeded = false;
    requestNeeded = false;
    requestUnicastNeeded = false;
    releaseNeeded = false; //???????
    rebind = false;
    timeOutOne = false;

}

void renewDhcp()
{
    Kill_AllTimers(); //stop and kill all timers
    ResetAllFlags();
    setDhcpState(DHCP_RENEWING);
    requestUnicastNeeded = true;
}

void callbackDhcpT1PeriodicTimer()//This one is for REQUEST during DHCP_RENEWING STATE
{
    requestUnicastNeeded = true;
}

void callbackDhcpT1HitTimer()
{
    stopTimer(callbackDhcpT1HitTimer);
    KillTimer(callbackDhcpT1HitTimer);
    setDhcpState(DHCP_RENEWING);
    //requestUnicastNeeded = true;
}

// Rebind functions

void rebindDhcp()
{
}

void callbackDhcpT2PeriodicTimer()
{
    requestUnicastNeeded = false;
    rebind = true;
}

void callbackDhcpT2HitTimer()
{
   // stopTimer(callbackDhcpT1PeriodicTimer);
    //KillTimer(callbackDhcpT1PeriodicTimer);
    stopTimer(callbackDhcpT2HitTimer);
    KillTimer(callbackDhcpT2HitTimer);
    setDhcpState(DHCP_REBINDING);
}

// End of lease timer
void callbackDhcpLeaseEndTimer()
{
//    Kill_AllTimers();
//    ResetAllFlags();
//    setDhcpState(DHCP_INIT);
//    discoverNeeded = true; //ADD Discover flag to true
    releaseDhcp();
}

// Release functions

void releaseDhcp()
{

    Kill_AllTimers();
    ResetAllFlags();
    //uint8_t localIpAddress[4] = {0};
//    setIpAddress(localIpAddress);
//    setIpSubnetMask(localIpAddress);
//    setIpDnsAddress(localIpAddress);                         //Delete ip and subnet mask
    releaseNeeded = true;
}

// IP conflict detection

void callbackDhcpIpConflictWindow()
{
}

void requestDhcpIpConflictTest()
{
}

bool isDhcpIpConflictDetectionMode()
{
    return ipConflictDetectionMode;
}

// Lease functions

uint32_t getDhcpLeaseSeconds()
{
    return leaseSeconds;
}

// Gets pointer to UDP payload of frame
uint8_t * getUdpPort(etherHeader *ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    //uint8_t ipHeaderLength = ip->size * 4;
    //udpHeader *udp = (udpHeader*)((uint8_t*)ip + ip->size);
    return ip->data;
}


uint8_t* getDhcpOption(etherHeader *ether, uint8_t option, uint8_t* length)
{
    // IP header
    ipHeader* ip = (ipHeader*)ether->data;
    // UDP header
    udpHeader* udp = (udpHeader*)((uint8_t*)ip + (ip->size * 4));
    //DHCP header
    dhcpFrame* dhcp = (dhcpFrame*)(uint8_t*)udp->data;
    //
    uint8_t *options = (uint8_t*)(dhcp + 1);

    // Iterate through options to find the desired one
     while (*options != 0xFF) { // DHCP option end marker
         if (*options == option) { // Check if option matches
             if (length != NULL) {
                 *length = *(options + 1); // Store length of option
             }
             return options + 2; // Return pointer to option value
         }
         // Move to the next option
         options += options[1] + 2; // Increment by option length + 2 (type + length)
     }

    return 0;
}


// Determines whether packet is DHCP
// Must be a UDP packet
bool isDhcpResponse(etherHeader* ether)
{

    bool ok = false;


    uint8_t* messageTypeField = getUdpData(ether);
    dhcpFrame* dhcpF = (dhcpFrame*)(uint8_t*)messageTypeField;
    if(*messageTypeField == 2 && dhcpF->xid == xid)
    {
       // putsUart0("  It is a DHCP response \n  ");
        ok = true;
    }
        return ok;
}

// Send DHCP message
void sendDhcpMessage(etherHeader *ether, uint8_t type)
{
    uint8_t i;
    uint32_t sum;
    uint16_t tmp16;
   // uint32_t udpLength;
    uint16_t udpLength;


    uint8_t localHwAddress[6];
    uint8_t localIpAddress[4];

    // Ether frame
    getEtherMacAddress(localHwAddress);
    getIpAddress(localIpAddress);

    if((type == 3 && requestUnicastNeeded == true)||(type == 7 && releaseNeeded == true)){
        for (i = 0; i < HW_ADD_LENGTH; i++)
           {

                 ether->destAddress[i] = serverHwAddress[i];
                 ether->sourceAddress[i] = localHwAddress[i];

           }
    }else if(requestUnicastNeeded == false){
        for (i = 0; i < HW_ADD_LENGTH; i++)
                  {
                 //ether->destAddress[i] = s.remoteHwAddress[i];
                 ether->destAddress[i] = 0xFF;
                 ether->sourceAddress[i] = localHwAddress[i];
                  }
    }

    ether->frameType = htons(TYPE_IP);


    // IP header
    ipHeader* ip = (ipHeader*)ether->data;
    ip->rev = 0x4;
    ip->size = 0x5;
    ip->typeOfService = 0;
    ip->id = 0;
    ip->flagsAndOffset = 0;
    ip->ttl = 128;
    ip->protocol = PROTOCOL_UDP;
    ip->headerChecksum = 0;

    if((type == 3 && requestUnicastNeeded == true) || (type == 7 && releaseNeeded == true)){
        for (i = 0; i < IP_ADD_LENGTH; i++)
       {
           ip->destIp[i] = dhcpServerIpAdd[i];
           ip->sourceIp[i] = localIpAddress[i];
       }
    }else if(requestUnicastNeeded == false)
    {
        for (i = 0; i < IP_ADD_LENGTH; i++)
        {
            ip->destIp[i] = 0xff;
            ip->sourceIp[i] = localIpAddress[i];
        }
    }

    uint8_t ipHeaderLength = ip->size * 4;

    // UDP header
    udpHeader* udp = (udpHeader*)((uint8_t*)ip + (ip->size * 4));

    //udp->sourcePort = htons(s.localPort);
    udp->sourcePort = htons(68);
    //udp->destPort = htons(s.remotePort);
    udp->destPort = htons(67);

    dhcpFrame* dhcp = (dhcpFrame*)(uint8_t*)udp->data;
    dhcp->op = 1; //Message Type --Boot Request(1)
    dhcp->htype = 1;
    dhcp->hlen = 6;
    dhcp->hops = 0;
    dhcp->xid = xid;
    dhcp->secs = 0;
//    if(type == 1){
       // dhcp->flags = 0x0080;
//    }else{
//        dhcp->flags = 0x0000;
//    }

        if((requestUnicastNeeded == true) && type == 3)
        {
            dhcp->flags = 0x0000;  //page 39 rfc2131, Renewing state, Request is sent as UNICAST
            for (i = 0; i < IP_ADD_LENGTH; i++)
                  {
                   dhcp->ciaddr[i] = dhcpOfferedIpAdd[i];
                   dhcp->yiaddr[i] = 0;
                   dhcp->siaddr[i] = 0;
                   dhcp->giaddr[i] = 0;
                  }
            //include ciaddr must be equal to current dhcpIpOffered
            //TURN OFF FLAG AFTER ALL information is set and ready to be submitted.
            //requestUnicastNeeded = false;
        }else if((requestUnicastNeeded == false)){
            dhcp->flags = 0x0080;
            for (i = 0; i < IP_ADD_LENGTH; i++)
                  {
                   //dhcp->ciaddr[i] = localIpAddress[i];
                   dhcp->ciaddr[i] = 0;
                   dhcp->yiaddr[i] = 0;
                   dhcp->siaddr[i] = 0;
                   dhcp->giaddr[i] = 0;
                  }
        }


        //Maybe move this block to the else above and code a new one for requestUnicastNeeded
//    for (i = 0; i < IP_ADD_LENGTH; i++)
//       {
//        dhcp->ciaddr[i] = localIpAddress[i];
//        dhcp->yiaddr[i] = 0;
//        dhcp->siaddr[i] = 0;
//        dhcp->giaddr[i] = 0;
//       }

    for (i = 0; i < HW_ADD_LENGTH; i++)
        {

            dhcp->chaddr[i] = localHwAddress[i];
        }
    for (i = 6; i < 16; i++)
            {

                dhcp->chaddr[i] = 0;
            }

    for (i = 0; i < 192; i++)
            {
                dhcp->data[i] = 0;
            }
    dhcp->magicCookie = htonl(0x63825363);


    //options
    uint8_t j = 0;
    if(type == 1)
    {
        dhcp->options[j++] = 53;
        dhcp->options[j++] = 1;
        dhcp->options[j++] = 1;
       // dhcp->options[j++]= 255;
    }else if(type == 3) //options will be different and the dhcp frame will be created uint8_t* messageTypeField = getUdpData(data);
    {
        //This is for request
        //option 53
        dhcp->options[j++] = 53;
        dhcp->options[j++] = 1;
        dhcp->options[j++] = 3; // option 53 1 3=Request.


        //INSERT option 61 here needs a for loop because it is MAC address
        //Option 61 Len = 4, ip address offered
        dhcp->options[j++] = 61;
        dhcp->options[j++] = 7;
        dhcp->options[j++] = 0x01;
        for (i = 0; i < HW_ADD_LENGTH; i++)
           {
               dhcp->options[j++] = localHwAddress[i];
           }

        if(requestUnicastNeeded == false && rebind == false){
            //Option 54 Len = 4, SERVER IP address
            dhcp->options[j++] = 54;
            dhcp->options[j++] = 4;
            dhcp->options[j++] = dhcpServerIpAdd[0];
            dhcp->options[j++] = dhcpServerIpAdd[1];
            dhcp->options[j++] = dhcpServerIpAdd[2];
            dhcp->options[j++] = dhcpServerIpAdd[3];


            //Option 50 Len = 4, IP address offered
             dhcp->options[j++] = 50;
             dhcp->options[j++] = 4;
             dhcp->options[j++] = dhcpOfferedIpAdd[0];
             dhcp->options[j++] = dhcpOfferedIpAdd[1];
             dhcp->options[j++] = dhcpOfferedIpAdd[2];
             dhcp->options[j++] = dhcpOfferedIpAdd[3];
        }



    }else if(type == 4){ //This is for DECLINE
        dhcp->options[j++] = 53;
        dhcp->options[j++] = 1;
        dhcp->options[j++] = 4;
    }else if(type == 7){ //This is for Release
        dhcp->options[j++] = 53;
        dhcp->options[j++] = 1;
        dhcp->options[j++] = 7;


        //INSERT option 61 here needs a for loop because it is MAC address
             //Option 61 Len = 4, ip address offered
             dhcp->options[j++] = 61;
             dhcp->options[j++] = 7;
             dhcp->options[j++] = 0x01;
             for (i = 0; i < HW_ADD_LENGTH; i++)
                {
                    dhcp->options[j++] = localHwAddress[i];
                }
    }
    requestUnicastNeeded = false;
    rebind = false;
    //option 255 end
    dhcp->options[j++]= 255;


    // adjust lengths
    //udpLength = sizeof(udpHeader)+ dhcp + options("#may vary"); //// ADD THIS AT THE END before putting packet together
    udpLength = sizeof(udpHeader)+ sizeof(dhcpFrame) +(sizeof(uint8_t)* j);


    ip->length = htons(ipHeaderLength + udpLength);
    // 32-bit sum over ip header
    calcIpChecksum(ip);
    // set udp length
    udp->length = htons(udpLength);


    // 32-bit sum over pseudo-header
    sum = 0;
    sumIpWords(ip->sourceIp, 8, &sum);
    tmp16 = ip->protocol;
    sum += (tmp16 & 0xff) << 8;
    sumIpWords(&udp->length, 2, &sum);
    // add udp header
    udp->check = 0;
    sumIpWords(udp, udpLength, &sum);
    udp->check = getIpChecksum(sum);


    putEtherPacket(ether, sizeof(etherHeader) + ipHeaderLength + udpLength);
}

//



// Determines whether packet is DHCP offer response to DHCP discover
// Must be a UDP packet
//bool isDhcpOffer(etherHeader *ether, uint8_t ipOfferedAdd[])
bool isDhcpOffer(etherHeader *ether)
{
    bool ok = false;

        // IP header
        ipHeader* ip = (ipHeader*)ether->data;
        // UDP header
        udpHeader* udp = (udpHeader*)((uint8_t*)ip + (ip->size * 4));

        dhcpFrame* dhcp = (dhcpFrame*)(uint8_t*)udp->data;

    if(dhcp->options[2] == 2 && dhcp->xid == xid)
    {
        ok = true;
    }
    return ok;
}

// Determines whether packet is DHCP ACK response to DHCP request
// Must be a UDP packet
bool isDhcpAck(etherHeader *ether)
{
    bool ok = false;

            // IP header
            ipHeader* ip = (ipHeader*)ether->data;
            // UDP header
            udpHeader* udp = (udpHeader*)((uint8_t*)ip + (ip->size * 4));

            dhcpFrame* dhcp = (dhcpFrame*)(uint8_t*)udp->data;

        if(dhcp->options[2] == 5 && dhcp->xid == xid)
        {
            ok = true;
        }

        return ok;
}
bool isDhcpNack(etherHeader *ether)
{
    bool ok = false;

            // IP header
            ipHeader* ip = (ipHeader*)ether->data;
            // UDP header
            udpHeader* udp = (udpHeader*)((uint8_t*)ip + (ip->size * 4));

            dhcpFrame* dhcp = (dhcpFrame*)(uint8_t*)udp->data;

        if(dhcp->options[2] == 6 && dhcp->xid == xid)
        {
            ok = true;
        }

        return ok;
}
void GetOfferedIPAndServerIP(etherHeader *ether){

    uint8_t i = 0;
    // IP header
   ipHeader* ip = (ipHeader*)ether->data;
   // UDP header
   udpHeader* udp = (udpHeader*)((uint8_t*)ip + (ip->size * 4));
   dhcpFrame* dhcp = (dhcpFrame*)(uint8_t*)udp->data;
//   uint8_t* leng = &(dhcp->options[4]);
   uint8_t length = 4;
   uint8_t* serverIP = getDhcpOption(ether,54,&length);

       for (i = 0; i < IP_ADD_LENGTH; i++)
           {
               dhcpOfferedIpAdd[i] = dhcp->yiaddr[i];
               dhcpServerIpAdd[i] = serverIP[i];
           }
}



// Handle a DHCP ACK
void handleDhcpAck(etherHeader *ether)//GET the lease time? setIP, setG
{
     uint8_t i = 0;
     uint8_t length = 4;
     uint8_t* lease = getDhcpOption(ether,51,&length);
     //uint8_t* lease;

    // uint8_t* subnetMask = getDhcpOption(ether,1,&length);


     //@home use this
     uint8_t sub[4];
     sub[0] = 255;
     sub[1] = 255;
     sub[2] = 0;
     sub[3] = 0;
     for(i = 0; i < 6; i++)
     {
         serverHwAddress[i] = ether->sourceAddress[i];
     }

     leaseSeconds = (((uint32_t)lease[0] << 24 ) | ((uint32_t)lease[1] << 16 ) | ((uint32_t)lease[2] << 8 )| (lease[3]));

     //leaseSeconds = 25;
     leaseT1 = (leaseSeconds/2);
     leaseT2 = (uint32_t)(leaseSeconds * 7.0/8);

     //leaseT1 = 40;
     //leaseT2 = 50;

     //setIpAddress(dhcpOfferedIpAdd);


     //setIpSubnetMask(subnetMask); Set this for school
     setIpSubnetMask(sub);
     setIpDnsAddress(dhcpServerIpAdd);
     startOneshotTimer(callbackDhcpLeaseEndTimer,leaseSeconds);
     startOneshotTimer(callbackDhcpT1HitTimer,leaseT1);
     startOneshotTimer(callbackDhcpT2HitTimer,leaseT2);


}

// Message requests
void isDhcpDiscoverNeeded()
{
    setDhcpState(DHCP_INIT);
    discoverNeeded = true;
}
//bool isDhcpDiscoverNeeded()
//{
//    discoverNeeded = true;
//}

bool isDhcpRequestNeeded()
{
    return true;
}

bool isDhcpReleaseNeeded()
{
    return false;
}

void KillAllTimers()
{

}


void GoToBoundState(){
    stopTimer(GoToBoundState);
    KillTimer(GoToBoundState);
    timeOutOne = true;

}

void sendDhcpPendingMessages(etherHeader *ether)
{
    uint8_t localIpAddress[4]={0};
    uint8_t i = 0;
    //getIpAddress(localIpAddress);
    if((getDhcpState() == DHCP_INIT)&&(discoverNeeded == true))
    {
        sendDhcpMessage(ether, 1); //Discover

        startPeriodicTimer(isDhcpDiscoverNeeded,10);
        setDhcpState(DHCP_SELECTING);
        i = countTimers();
        snprintf(str, sizeof(str), "@ DHCP_INIT timers: %d\n",i);
        putsUart0(str);
        discoverNeeded = false;


    }else if((getDhcpState() == DHCP_SELECTING) && (requestNeeded == true))
    {

            sendDhcpMessage(ether, DHCPREQUEST);
            requestNeeded = false;
            setDhcpState(DHCP_REQUESTING);


    }else if(getDhcpState() == DHCP_REQUESTING && sendArpNeeded == true)
    {
        sendArpRequest(ether,localIpAddress,dhcpOfferedIpAdd);
        setDhcpState(DHCP_TESTING_IP);


    }else if((getDhcpState() == DHCP_TESTING_IP))//wait in here for 15 seconds. USE IP and enter bound state
    {

         //After 15 second timeout.
         startOneshotTimer(GoToBoundState,15);


         setDhcpState(DHCP_BOUND);


    }else if(getDhcpState() == DHCP_BOUND && (ipConflictDetectionMode == false) && (timeOutOne == true) )//   && (ready == true)
    {

        sendArpNeeded = false;
        setIpAddress(dhcpOfferedIpAdd);
       // timeOutOne = false;

        //ready = false;
        //@ time = T1 the State will transition to the Renewing State
    }else if((getDhcpState() == DHCP_RENEWING) && (requestUnicastNeeded == false))//&& (requestUnicastNeeded == true)
    {

         startPeriodicTimer(callbackDhcpT1PeriodicTimer,5);


    }else if((getDhcpState() == DHCP_RENEWING) && (requestUnicastNeeded == true) &&(dhcpEnabled == true))//&& (ack == false)
    {
         //sendDhcpMessage(ether, DHCPREQUEST);

    }else if((getDhcpState() == DHCP_REBINDING) && (rebind==false))
    {
        startPeriodicTimer(callbackDhcpT2PeriodicTimer,10);

    }else if((getDhcpState() == DHCP_REBINDING) && (rebind == true))
    {
          sendDhcpMessage(ether, DHCPREQUEST); //This should send a broadcast message request to Rebind. To test, make T1 longer than T2.
//        stopTimer(callbackDhcpT2HitTimer);
//        KillTimer(callbackDhcpT2HitTimer);
//        startPeriodicTimer(callbackDhcpT2PeriodicTimer,3);

    }else if(ipConflictDetectionMode == true)//Send Decline message, Go to INIT, KILL any timers still on,
    {
        sendDhcpMessage(ether, 4);
        //
//        startPeriodicTimer(enableDhcp,10);
    }else if((releaseNeeded == true) && (dhcpEnabled == true))
    {
        sendDhcpMessage(ether, DHCPRELEASE);
        releaseNeeded = false;
        uint8_t localIpAddress[4] = {0};
        setIpAddress(localIpAddress);
        setDhcpState(DHCP_INIT);
        discoverNeeded = true;
    }else if((getDhcpState() == DHCP_DISABLED))
    {
        sendDhcpMessage(ether, DHCPRELEASE);
        uint8_t localIpAddress[4] = {0};
        setIpSubnetMask(localIpAddress);
        setIpDnsAddress(localIpAddress);
        localIpAddress[0] = 192;
        localIpAddress[1] = 168;
        localIpAddress[2] = 1;
        localIpAddress[3] = 102;
        // setDhcpState(DHCP_INIT);
        setIpAddress(localIpAddress);
        dhcpEnabled = false;
    }

}

void processDhcpResponse(etherHeader *ether)
{

        if(isDhcpOffer(ether) && getDhcpState() == DHCP_SELECTING){

           stopTimer(isDhcpDiscoverNeeded);
           discoverNeeded = false;

           KillTimer(isDhcpDiscoverNeeded);

            GetOfferedIPAndServerIP(ether);
            requestNeeded = true;

        }else if(isDhcpAck(ether)&& (getDhcpState()== DHCP_REQUESTING))//if it's an ack then get IP's and leaseTime
        {
            GetOfferedIPAndServerIP(ether);
            handleDhcpAck(ether);
            sendArpNeeded = true;
        }else if(isDhcpNack(ether)&& (getDhcpState()== DHCP_REQUESTING))//if it's an Nack then go back to INIT state
        {

            Kill_AllTimers();
            ResetAllFlags();
            discoverNeeded = true;
            setDhcpState(DHCP_INIT);

        }else if((isDhcpAck(ether)) && (getDhcpState()== DHCP_RENEWING)){

            //requestUnicastNeeded = false;
            timeOutOne = true;
            stopTimer(callbackDhcpT1PeriodicTimer);
            KillTimer(callbackDhcpT1PeriodicTimer);
            GetOfferedIPAndServerIP(ether);
            //THINK about inlcluding KILL ALL TIMERS BEFORE handleACk or in handle ack.
            Kill_AllTimers();
            handleDhcpAck(ether);
            setDhcpState(DHCP_BOUND);
            requestCounter++;

        }else if(isDhcpNack(ether) && (getDhcpState() == DHCP_RENEWING))
        {
            //Halt to Network
            Kill_AllTimers();
            setDhcpState(DHCP_INIT);
            ResetAllFlags();
            discoverNeeded = true;
           // releaseDhcp();

        }else if((isDhcpAck(ether)) && (getDhcpState()== DHCP_REBINDING)){

            stopTimer(callbackDhcpT2PeriodicTimer);
            KillTimer(callbackDhcpT2PeriodicTimer);
            GetOfferedIPAndServerIP(ether);
            //THINK about inlcluding KILL ALL TIMERS BEFORE handleACk or in handle ack.
            Kill_AllTimers();
            handleDhcpAck(ether);
            setDhcpState(DHCP_BOUND);


        }else if(isDhcpNack(ether) && (getDhcpState() == DHCP_REBINDING))
        {
            //Halt to Network
            Kill_AllTimers();
            setDhcpState(DHCP_INIT);
            ResetAllFlags();
            discoverNeeded = true;
           // releaseDhcp();

        }
}

void processDhcpArpResponse(etherHeader *ether)
{
    //If it is a DHCP ARP Response then set a flag
    //Send Decline


    if(getDhcpState() == DHCP_REQUESTING && sendArpNeeded == true){
        uint8_t i = 0;
        arpPacket *arp = (arpPacket*)ether->data;
        for(i = 0; i < 4; i++)
        {
            if((arp->sourceIp[i] == dhcpOfferedIpAdd[i]) && (arp->destIp[i] == 0)){
                ipConflictDetectionMode = true;
            }else{
                ipConflictDetectionMode = false;
            }
        }
    }


   if(ipConflictDetectionMode == true)
   {
       stopTimer(GoToBoundState);
       KillTimer(GoToBoundState);

   }
    //enableDhcp();
}

// DHCP control functions

void enableDhcp()
{
    //send DHCP discover
    dhcpEnabled = true;
    discoverNeeded = true;
    setDhcpState(DHCP_INIT);

}

void disableDhcp()    ///TAKE into account Release
{
    Kill_AllTimers();
    ResetAllFlags();
    discoverNeeded = false;
    //dhcpEnabled = false;
    uint8_t localIpAddress[4] = {0};
         setIpSubnetMask(localIpAddress);
         setIpDnsAddress(localIpAddress);
         localIpAddress[0] = 192;
         localIpAddress[1] = 168;
         localIpAddress[2] = 1;
         localIpAddress[3] = 102;
         // setDhcpState(DHCP_INIT);
         setIpAddress(localIpAddress);
         dhcpEnabled = false;
   // setDhcpState(DHCP_DISABLED);
}

bool isDhcpEnabled()
{
    //return true or false.
    if(dhcpEnabled)
    {
        return true;
    }else{
        return false;
    }

}

