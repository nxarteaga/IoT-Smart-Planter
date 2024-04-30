// Ethernet Framework for Projects 1 and 2
// Spring 2024
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL w/ ENC28J60
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// ENC28J60 Ethernet controller on SPI0
//   MOSI (SSI0Tx) on PA5
//   MISO (SSI0Rx) on PA4
//   SCLK (SSI0Clk) on PA2
//   ~CS (SW controlled) on PA3
//   WOL on PB3
//   INT on PC6

// Pinning for IoT projects with wireless modules:
// N24L01+ RF transceiver
//   MOSI (SSI0Tx) on PA5
//   MISO (SSI0Rx) on PA4
//   SCLK (SSI0Clk) on PA2
//   ~CS on PE0
//   INT on PB2
// Xbee module
//   DIN (UART1TX) on PC5
//   DOUT (UART1RX) on PC4

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "eeprom.h"
#include "gpio.h"
#include "spi0.h"
#include "uart0.h"
#include "wait.h"
#include "timer.h"
#include "eth0.h"
#include "arp.h"
#include "ip.h"
#include "icmp.h"
#include "udp.h"
#include "tcp.h"
#include "dhcp.h"
#include "mqtt.h"
#include "plant.h"

// Pins
#define RED_LED PORTF,1
#define BLUE_LED PORTF,2
#define GREEN_LED PORTF,3
#define PUSH_BUTTON PORTF,4

// EEPROM Map
#define EEPROM_DHCP        1
#define EEPROM_IP          2
#define EEPROM_SUBNET_MASK 3
#define EEPROM_GATEWAY     4
#define EEPROM_DNS         5
#define EEPROM_TIME        6
#define EEPROM_MQTT        7
#define EEPROM_ERASED      0xFFFFFFFF

// Plant State Machine
#define LUX 1
#define TEMP 2
#define HUM 3
#define MOIST 4
#define VOLUME 5

// Plant Timer
#define PLANT_AUTO_PUB_S 10

// ----------------------------------------------------------------------------
// Globals
// ----------------------------------------------------------------------------

// Plant
uint16_t lux = 0;
uint8_t temp = 0, hum = 0;
uint16_t moist = 0, volume = 0;
uint8_t subbedTopicData = 0;
bool timeToPublish = true;

// MQTT
bool mqttEnabled = false;
bool mqttDisconnecting = false;
bool mqttConnectSent = false;
bool mqttSubAckNeeded = false;
bool mqttSubbed = false;
char subbedTopicDataStr[3] = {0, 0, NULL};
bool autoPublishEnabled = false;

//-----------------------------------------------------------------------------
// Subroutines                
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    // Enable clocks
    enablePort(PORTF);
    _delay_cycles(3);

    // Configure LED and pushbutton pins
    selectPinPushPullOutput(RED_LED);
    selectPinPushPullOutput(GREEN_LED);
    selectPinPushPullOutput(BLUE_LED);
    selectPinDigitalInput(PUSH_BUTTON);
    enablePinPullup(PUSH_BUTTON);
}

void displayConnectionInfo()
{
    uint8_t i;
    char str[20];
    uint8_t mac[6];
    uint8_t ip[4];
    getEtherMacAddress(mac);
    putsUart0("  HW:    ");
    for (i = 0; i < HW_ADD_LENGTH; i++)
    {
        snprintf(str, sizeof(str), "%02"PRIu8, mac[i]);
        putsUart0(str);
        if (i < HW_ADD_LENGTH-1)
            putcUart0(':');
    }
    putcUart0('\n');
    getIpAddress(ip);
    putsUart0("  IP:    ");
    for (i = 0; i < IP_ADD_LENGTH; i++)
    {
        snprintf(str, sizeof(str), "%"PRIu8, ip[i]);
        putsUart0(str);
        if (i < IP_ADD_LENGTH-1)
            putcUart0('.');
    }
    if (isDhcpEnabled())
        putsUart0(" (dhcp)");
    else
        putsUart0(" (static)");
    putcUart0('\n');
    getIpSubnetMask(ip);
    putsUart0("  SN:    ");
    for (i = 0; i < IP_ADD_LENGTH; i++)
    {
        snprintf(str, sizeof(str), "%"PRIu8, ip[i]);
        putsUart0(str);
        if (i < IP_ADD_LENGTH-1)
            putcUart0('.');
    }
    putcUart0('\n');
    getIpGatewayAddress(ip);
    putsUart0("  GW:    ");
    for (i = 0; i < IP_ADD_LENGTH; i++)
    {
        snprintf(str, sizeof(str), "%"PRIu8, ip[i]);
        putsUart0(str);
        if (i < IP_ADD_LENGTH-1)
            putcUart0('.');
    }
    putcUart0('\n');
    getIpDnsAddress(ip);
    putsUart0("  DNS:   ");
    for (i = 0; i < IP_ADD_LENGTH; i++)
    {
        snprintf(str, sizeof(str), "%"PRIu8, ip[i]);
        putsUart0(str);
        if (i < IP_ADD_LENGTH-1)
            putcUart0('.');
    }
    putcUart0('\n');
    getIpTimeServerAddress(ip);
    putsUart0("  Time:  ");
    for (i = 0; i < IP_ADD_LENGTH; i++)
    {
        snprintf(str, sizeof(str), "%"PRIu8, ip[i]);
        putsUart0(str);
        if (i < IP_ADD_LENGTH-1)
            putcUart0('.');
    }
    putcUart0('\n');
    getIpMqttBrokerAddress(ip);
    putsUart0("  MQTT:  ");
    for (i = 0; i < IP_ADD_LENGTH; i++)
    {
        snprintf(str, sizeof(str), "%"PRIu8, ip[i]);
        putsUart0(str);
        if (i < IP_ADD_LENGTH-1)
            putcUart0('.');
    }
    putcUart0('\n');
    if (isDhcpEnabled())
    {
        putsUart0("  Lease: ");
        uint32_t s, m, h, d;
        s = getDhcpLeaseSeconds();
        d = s / (24*60*60);
        s -= d * (24*60*60);
        h = s / (60*60);
        s -= h * (60*60);
        m = s / 60;
        snprintf(str, sizeof(str), "%"PRIu32"d:%02"PRIu32"h:%02"PRIu32"m\n", d, h, m);
        putsUart0(str);
    }
    if (isEtherLinkUp())
        putsUart0("  Link is up\n");
    else
        putsUart0("  Link is down\n");
}

void readConfiguration()
{
    uint32_t temp;
    uint8_t* ip;

    if (readEeprom(EEPROM_DHCP) == EEPROM_ERASED)
    {
        enableDhcp();
    }
    else
    {
        disableDhcp();
        temp = readEeprom(EEPROM_IP);
        if (temp != EEPROM_ERASED)
        {
            ip = (uint8_t*)&temp;
            setIpAddress(ip);
        }
        temp = readEeprom(EEPROM_SUBNET_MASK);
        if (temp != EEPROM_ERASED)
        {
            ip = (uint8_t*)&temp;
            setIpSubnetMask(ip);
        }
        temp = readEeprom(EEPROM_GATEWAY);
        if (temp != EEPROM_ERASED)
        {
            ip = (uint8_t*)&temp;
            setIpGatewayAddress(ip);
        }
        temp = readEeprom(EEPROM_DNS);
        if (temp != EEPROM_ERASED)
        {
            ip = (uint8_t*)&temp;
            setIpDnsAddress(ip);
        }
        temp = readEeprom(EEPROM_TIME);
        if (temp != EEPROM_ERASED)
        {
            ip = (uint8_t*)&temp;
            setIpTimeServerAddress(ip);
        }
        temp = readEeprom(EEPROM_MQTT);
        if (temp != EEPROM_ERASED)
        {
            ip = (uint8_t*)&temp;
            setIpMqttBrokerAddress(ip);
        }
    }
}

#define MAX_CHARS 80
char strInput[MAX_CHARS+1];
char* token;
uint8_t count = 0;

uint8_t asciiToUint8(const char str[])
{
    uint8_t data;
    if (str[0] == '0' && tolower(str[1]) == 'x')
        sscanf(str, "%hhx", &data);
    else
        sscanf(str, "%hhu", &data);
    return data;
}

char* convertIntToString(uint32_t num, char str[])
{
    uint8_t len = 0, i = 0;
    uint64_t mod = 10;

    while ((num % mod) != num)
    {
        mod *= 10;
        len++;
    }

    for (i = 0; i <= len; i++)
        str[i] = (num % mod / (mod /= 10) + 48);

    str[len + 1] = NULL;

    return str;
}

int32_t convertStringToInt(char str[])
{
    int8_t len = 0, i = 0;
    uint32_t num = 0, tens = 1;
  
    while (*str != NULL)
    {
        len++;
        str++;
    }
  
    str -= len;
    tens = 10 * (len - 1);
  
    for (i = 0; i < len; i++)
    {
        num += (str[i] - 48) * tens;
        tens = tens / 10;
    }
    return num;
}

void processShell(etherHeader *ether, socket *s)
{

    if (kbhitUart0())
    {
        bool end;
        char c = getcUart0();
        uint8_t i;
        uint8_t ip[IP_ADD_LENGTH];
        uint32_t* p32;
        char *topic, *data;

        end = (c == 13) || (count == MAX_CHARS);
        if (!end)
        {
            if ((c == 8 || c == 127) && count > 0)
                count--;
            if (c >= ' ' && c < 127)
                strInput[count++] = c;
        }
        else
        {
            strInput[count] = '\0';
            count = 0;
            token = strtok(strInput, " ");
            if (strcmp(token, "dhcp") == 0)
            {
                token = strtok(NULL, " ");
                if (strcmp(token, "renew") == 0)
                {
                    renewDhcp();
                }
                else if (strcmp(token, "release") == 0)
                {
                    releaseDhcp();
                }
                else if (strcmp(token, "on") == 0)
                {
                    enableDhcp();
                    writeEeprom(EEPROM_DHCP, EEPROM_ERASED);
                }
                else if (strcmp(token, "off") == 0)
                {
                    disableDhcp();
                    writeEeprom(EEPROM_DHCP, 0);
                }
                else
                    putsUart0("Error in dhcp argument\r");
            }
            if (strcmp(token, "mqtt") == 0)
            {
                token = strtok(NULL, " ");
                if (strcmp(token, "connect") == 0)
                {
                    if (mqttEnabled)
                    {
                        putsUart0("MQTT already connected\n");
                    }
                    else
                    {
                        mqttEnabled = true;
                        sendTcpArpRequest();
                    }
                }
                if (strcmp(token, "disconnect") == 0)
                {
                    if (mqttEnabled)
                    {
                        mqttEnabled = false;
                        mqttConnectSent = false;
                        mqttSubAckNeeded = false;
                        autoPublishEnabled = false;
                        disconnectMqtt(ether, s);
                    }
                    else
                    {
                        putsUart0("MQTT isn't even connected yet 🤔\n");
                    }
                }
                if (strcmp(token, "publish") == 0)
                {
                    if (isMqttConAcked())
                    {
                        topic = strtok(NULL, " ");

                        if ((strcmp(topic, "lux") == 0) || strcmp(topic, "uta/plant/lux") == 0)
                        {
                            convertIntToString(lux, data);
                            topic = "uta/plant/lux";
                        }
                        else if ((strcmp(topic, "temp") == 0) || strcmp(topic, "uta/plant/temp") == 0)
                        {
                            convertIntToString(temp, data);
                            topic = "uta/plant/temp";
                        }
                        else if ((strcmp(topic, "humidity") == 0) || strcmp(topic, "uta/plant/humidity") == 0)
                        {
                            convertIntToString(hum, data);
                            topic = "uta/plant/temp";
                        }
                        else if ((strcmp(topic, "moisture") == 0) || strcmp(topic, "uta/plant/moisture") == 0)
                        {
                            convertIntToString(moist, data);
                            topic = "uta/plant/moisture";
                        }
                        else if ((strcmp(topic, "reservoir") == 0) || strcmp(topic, "uta/plant/reservoir") == 0)
                        {
                            convertIntToString(volume, data);
                            topic = "uta/plant/reservoir";
                        }
                        else if ((strcmp(topic, "setpoint") == 0) || strcmp(topic, "uta/plant/moisture_set_point") == 0)
                        {
                            data[0] = '4';
                            data[1] = '5';
                            data[2] = NULL;
                            topic = "uta/plant/moisture_set_point";
                        }
                        else
                        {
                            topic = NULL;
                            data = NULL;
                            putsUart0("Invalid topic\n");
                        }

                        if (topic != NULL && data != NULL)
                        {
                            publishMqtt(ether, s, topic, data);
                        }
                    }
                    else
                    {
                        putsUart0("MQTT not connected yet\n");
                    }
                }
                if (strcmp(token, "subscribe") == 0)
                {
                    topic = strtok(NULL, " ");
                    if (topic != NULL)
                    {
                        char *topicPtr = topic;
                        uint16_t topicLen = 0;

                        while (*topicPtr != NULL)
                        {
                            topicPtr++;
                            topicLen++;
                        }

                        setSubbedTopicLengh(topicLen);
                        mqttSubAckNeeded = true;
                        subscribeMqtt(ether, s, topic);
                        uint8_t k = 0;
                    }
                }
                if (strcmp(token, "unsubscribe") == 0)
                {
                    topic = strtok(NULL, " ");
                    if (topic != NULL)
                    {
                        mqttSubAckNeeded = false;
                        // unsubscribeMqtt(topic);
                        uint8_t placeholder = 0;
                    }
                }
            }
            if (strcmp(token, "ip") == 0)
            {
                displayConnectionInfo();
            }
            if (strcmp(token, "autopub") == 0)
            {
                if (isMqttConAcked())
                {
                    if (!autoPublishEnabled)
                    {
                        autoPublishEnabled = true;
                        putsUart0("Auto publish enabled\n");
                    }
                    else
                    {
                        autoPublishEnabled = false;
                        putsUart0("Auto publish disabled\n");
                    }
                }
                else
                {
                    putsUart0("MQTT not connected yet\n");
                }
            }
            if (strcmp(token, "ping") == 0)
            {
                for (i = 0; i < IP_ADD_LENGTH; i++)
                {
                    token = strtok(NULL, " .");
                    ip[i] = asciiToUint8(token);
                }
                //removed from this version to save space: sendPingRequest(ip)
            }
            if (strcmp(token, "reboot") == 0)
            {
                NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
            }
            if (strcmp(token, "set") == 0)
            {
                token = strtok(NULL, " ");
                if (strcmp(token, "ip") == 0)
                {
                    for (i = 0; i < IP_ADD_LENGTH; i++)
                    {
                        token = strtok(NULL, " .");
                        ip[i] = asciiToUint8(token);
                    }
                    setIpAddress(ip);
                    p32 = (uint32_t*)ip;
                    writeEeprom(EEPROM_IP, *p32);
                }
                if (strcmp(token, "sn") == 0)
                {
                    for (i = 0; i < IP_ADD_LENGTH; i++)
                    {
                        token = strtok(NULL, " .");
                        ip[i] = asciiToUint8(token);
                    }
                    setIpSubnetMask(ip);
                    p32 = (uint32_t*)ip;
                    writeEeprom(EEPROM_SUBNET_MASK, *p32);
                }
                if (strcmp(token, "gw") == 0)
                {
                    for (i = 0; i < IP_ADD_LENGTH; i++)
                    {
                        token = strtok(NULL, " .");
                        ip[i] = asciiToUint8(token);
                    }
                    setIpGatewayAddress(ip);
                    p32 = (uint32_t*)ip;
                    writeEeprom(EEPROM_GATEWAY, *p32);
                }
                if (strcmp(token, "dns") == 0)
                {
                    for (i = 0; i < IP_ADD_LENGTH; i++)
                    {
                        token = strtok(NULL, " .");
                        ip[i] = asciiToUint8(token);
                    }
                    setIpDnsAddress(ip);
                    p32 = (uint32_t*)ip;
                    writeEeprom(EEPROM_DNS, *p32);
                }
                if (strcmp(token, "time") == 0)
                {
                    for (i = 0; i < IP_ADD_LENGTH; i++)
                    {
                        token = strtok(NULL, " .");
                        ip[i] = asciiToUint8(token);
                    }
                    setIpTimeServerAddress(ip);
                    p32 = (uint32_t*)ip;
                    writeEeprom(EEPROM_TIME, *p32);
                }
                if (strcmp(token, "mqtt") == 0)
                {
                    for (i = 0; i < IP_ADD_LENGTH; i++)
                    {
                        token = strtok(NULL, " .");
                        ip[i] = asciiToUint8(token);
                    }
                    setIpMqttBrokerAddress(ip);
                    p32 = (uint32_t*)ip;
                    writeEeprom(EEPROM_MQTT, *p32);
                }
            }

            if (strcmp(token, "help") == 0)
            {
                putsUart0("Commands:\n");
                putsUart0("  dhcp on|off|renew|release\n");
                putsUart0("  mqtt ACTION [USER [PASSWORD]]\n");
                putsUart0("    where ACTION = {connect|disconnect|publish TOPIC DATA\n");
                putsUart0("                   |subscribe TOPIC|unsubscribe TOPIC}\n");
                putsUart0("  autopub\n");
                putsUart0("  ip\n");
                putsUart0("  ping w.x.y.z\n");
                putsUart0("  reboot\n");
                putsUart0("  set ip|gw|dns|time|mqtt|sn w.x.y.z\n");
            }
        }
    }
}

void callbackPublishPlantData()
{
    timeToPublish = true;
    KillTimer(callbackPublishPlantData);
}

void autoPublishPlantData(etherHeader *data, socket *s)
{
    static uint8_t plant_state = LUX;

    if (timeToPublish)
    {
        char buf[4] = {0, 0, 0, 0};
        switch (plant_state)
        {
            case LUX:
                // snprintf(strInput, sizeof(strInput), "Lux: %"PRIu16" lx\n", volume);
                // putsUart0(strInput);
                publishMqtt(data, s, "uta/plant/lux", convertIntToString(lux, buf));
                plant_state = TEMP;
                break;
            case TEMP:
                snprintf(strInput, sizeof(strInput), "Temp: %"PRIu8" C\n", temp);
                // putsUart0(strInput);
                publishMqtt(data, s, "uta/plant/temp", convertIntToString(temp, buf));
                plant_state = HUM;
                break;
            case HUM:
                // snprintf(strInput, sizeof(strInput), "Hum: %"PRIu8" C\n", temp);
                // putsUart0(strInput);
                publishMqtt(data, s, "uta/plant/humidity", convertIntToString(hum, buf));
                plant_state = MOIST;
                break;
            case MOIST:
                // snprintf(strInput, sizeof(strInput), "Moisture: %"PRIu16"%%\n", moist);
                // putsUart0(strInput);
                publishMqtt(data, s, "uta/plant/moisture", convertIntToString(moist, buf));
                plant_state = VOLUME;
                break;
            case VOLUME:
                // snprintf(strInput, sizeof(strInput), "Volume: %"PRIu16" mL\n", volume);
                // putsUart0(strInput);
                publishMqtt(data, s, "uta/plant/reservoir", convertIntToString(volume, buf));
                plant_state = LUX;
                break;
        }

        timeToPublish = false;
        startOneshotTimer(callbackPublishPlantData, PLANT_AUTO_PUB_S);
    }
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

// Max packet is calculated as:
// Ether frame header (18) + Max MTU (1500)
#define MAX_PACKET_SIZE 1518

int main(void)
    {
    uint8_t* udpData;
    uint8_t buffer[MAX_PACKET_SIZE];
    etherHeader *data = (etherHeader*) buffer;
    socket s;

    // Clears buffer from last reboot
    uint16_t i = 0;
    for (i = 0; i < MAX_PACKET_SIZE; i++)
    {
        buffer[i] = 0;
    }

    // Init controller
    initHw();

    // Setup UART0
    initUart0();
    setUart0BaudRate(115200, 40e6);

    // Init timer
    initTimer();

    // Init sockets
    initSockets();

    // Init ethernet interface (eth0)
    putsUart0("\nStarting eth0\n");
    initEther(ETHER_UNICAST | ETHER_BROADCAST | ETHER_HALFDUPLEX);
    setEtherMacAddress(2, 3, 4, 5, 6, 0x66);

    // Init EEPROM
    // FIXME: EEPROM is seemingly not storing values between resets?
    initEeprom();
    readConfiguration();

    setPinValue(GREEN_LED, 1);
    waitMicrosecond(100000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(100000);

    // Init plant
    initPlant(); /**************** Comment this out, otherwise i2c errors */

    disableDhcp();

    // TODO: Remove manual IP stuff once EEPROM is fixed

    uint8_t tempLocalIpAddress[4];
    uint8_t tempSn[4];
    uint8_t tempGw[4];

    tempLocalIpAddress[0] = 192;
    tempLocalIpAddress[1] = 168;
    tempLocalIpAddress[2] = 1;
    tempLocalIpAddress[3] = 102;

    tempSn[0] = 255;
    tempSn[1] = 255;
    tempSn[2] = 255;
    tempSn[3] = 0;

    tempGw[0] = 192;
    tempGw[1] = 168;
    tempGw[2] = 1;
    // tempGw[3] = 236; // Lab PC
    tempGw[3] = 115; // Broker Team
    // tempGw[3] = 163; // Laptop

    setIpAddress(tempLocalIpAddress);
    setIpSubnetMask(tempSn);
    setIpGatewayAddress(tempGw);
    setIpDnsAddress(tempGw);

    // Socket info
    // TODO: Write function that does all of the socket stuff

    // IP
    s.remoteIpAddress[0] = tempGw[0];
    s.remoteIpAddress[1] = tempGw[1];
    s.remoteIpAddress[2] = tempGw[2];
    s.remoteIpAddress[3] = tempGw[3];

    // Ports
    s.remotePort = 1883; // Unencrypted MQTT Port
    s.localPort = 50143; // Gets random port, start at 50000 for testing

    // SEQ/ACK Nums
    s.sequenceNumber = htonl(2); // Starts at 1
    s.acknowledgementNumber = htonl(0); // Will be set by the server

    // State
    s.state = TCP_CLOSED; // Closed on startup

    // setWaterPumpSpeed(800);

    // Main Loop
    // RTOS and interrupts would greatly improve this code,
    // but the goal here is simplicity
    while (true)
    {
        // Get plant data 4 seconds
        getPlantData(&lux, &temp, &hum, &moist, &volume);

        // Moisture Setpoint Pseudocode
        // if (moisture > moisture_set_point)
        // {
        //      pumpwater()
        // }

        // Auto publishes plant data
        if (autoPublishEnabled)
        {
            autoPublishPlantData(data, &s);
        }

        // Put terminal processing here
        processShell(data, &s);
        
        /*
        // DHCP maintenance
        if (isDhcpEnabled())
        {
            sendDhcpPendingMessages(data);
        }
        */

        // TCP pending messages
        sendTcpPendingMessages(data, &s);

        // Sends MQTT Connect message if not sent
        if (!mqttConnectSent && (getTcpState(0) == TCP_ESTABLISHED) && mqttEnabled)
        {
            connectMqtt(data, &s);
            mqttConnectSent = true;
        }

        // Packet processings
        if (isEtherDataAvailable())
        {
            if (isEtherOverflow())
            {
                /*
                setPinValue(RED_LED, 1);
                waitMicrosecond(100000);
                setPinValue(RED_LED, 0);
                */
            }

            // Get packet
            getEtherPacket(data, MAX_PACKET_SIZE);

            // Route ARP response to appropriate handlers
            // DHCP uses ARP response to verify address granted is not in use
            // TCP active open uses ARP response to get the HW address to establish the socket
            if (isArpResponse(data))
            {
                // processDhcpArpResponse(data);
                processTcpArpResponse(data, &s);
            }

            // Handle ARP request
            if (isArpRequest(data))
            {
                sendArpResponse(data);
            }

            if (getTcpState(0) == TCP_CLOSED)
            {
                waitMicrosecond(1000);
            }

            // Handle IP datagram
            if (isIp(data))
            {
            	if (isIpUnicast(data))
            	{
                    // Handle ICMP ping request
                    if (isPingRequest(data))
                    {
                        sendPingResponse(data);
                    }

                    // Handle UDP datagram
                    if (isUdp(data))
                    {
                        udpData = getUdpData(data);
                        if (strcmp((char*)udpData, "on") == 0)
                        {
                            setPinValue(GREEN_LED, 1);
                        }
                        if (strcmp((char*)udpData, "off") == 0)
                        {
                            setPinValue(GREEN_LED, 0);
                        }
                        getSocketInfoFromUdpPacket(data, &s);
                        sendUdpMessage(data, s, (uint8_t*)"Received", 9);
                    }

                    // Handle TCP datagram
                    if (isTcp(data))
                    {
                        processTcpResponse(data, &s);
                        /*
                        if (isTcpPortOpen(data))
                        {

                           // isSYN_ACK(data);
                           processTcpResponse(data);
                            // processTcpResponse
                        }
                        else
                        {
                            sendTcpResponse(data, &s, ACK | RST);
                        }
                        */

                        // MQTT Connect Ack handler
                        if (mqttConnectSent && !isMqttConAcked())
                        {
                            checkMqttConAck(data, &s);
                        }

                        // MQTT Subscribe Ack handler
                        if (mqttSubAckNeeded && !isMqttSubAcked())
                        {
                            checkMqttSubAck(data, &s);
                            if (isMqttSubAcked())
                            {
                                mqttSubAckNeeded = false;
                                mqttSubbed = true;
                            }
                        }

                        if (mqttSubbed)
                        {
                            if (processPubMessage(data, &s, subbedTopicDataStr))
                            {
                                subbedTopicData = convertStringToInt(subbedTopicDataStr);
                            }
                        }

//                        if(isMqttConAcked() == true)
//                        {
//                            //subscribe
//                            //subscribeMqtt(data, &s,"uta/plant_lux");
//                            subscribeMqtt(data, &s,"uta/weather/wind_direction");
//                        }
                        // MQTT Disconnect Fin handler
                        if (mqttDisconnecting)
                        {
                            if (!isTcpFin(data))
                            {
                                sendTcpFin();
                            }
                            mqttDisconnecting = false;
                        }
                    }
                }
                /*
            	// Handle DHCP response
                else
                {
                    if (isUdp(data))
                        if (isDhcpResponse(data))
                        {   
                            processDhcpResponse(data);
                        }
                }
                */
            }
        }
    }
}
