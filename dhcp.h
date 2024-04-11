// DHCP Library
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

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#ifndef DHCP_H_
#define DHCP_H_

#include <stdint.h>
#include <stdbool.h>
#include "udp.h"

typedef struct _dhcpFrame // 240 or more bytes
{
  uint8_t op;
  uint8_t htype;
  uint8_t hlen;
  uint8_t hops;
  uint32_t  xid;
  uint16_t secs;
  uint16_t flags;
  uint8_t ciaddr[4];
  uint8_t yiaddr[4];
  uint8_t siaddr[4];
  uint8_t giaddr[4];
  uint8_t chaddr[16];
  uint8_t data[192];
  uint32_t magicCookie;
  uint8_t options[0];
} dhcpFrame;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool isDhcpResponse(etherHeader *ether);
void sendDhcpMessage(etherHeader *ether, uint8_t type);
void sendDhcpPendingMessages(etherHeader *ether);
void processDhcpResponse(etherHeader *ether);
void processDhcpArpResponse(etherHeader *ether);

void enableDhcp(void);
void disableDhcp(void);
bool isDhcpEnabled(void);

void renewDhcp(void);
void releaseDhcp(void);

uint32_t getDhcpLeaseSeconds();

#endif

