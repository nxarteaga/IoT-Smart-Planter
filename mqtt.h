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

#ifndef MQTT_H_
#define MQTT_H_

#include <stdint.h>
#include <stdbool.h>
#include "tcp.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void connectMqtt(etherHeader *ether, socket *s);
void disconnectMqtt(etherHeader *ether, socket *s);
void publishMqtt(etherHeader *ether, socket *s, char strTopic[], char strData[]);
void subscribeMqtt(etherHeader *ether, socket *s, char strTopic[]);
void unsubscribeMqtt(etherHeader *ether, socket *s, char strTopic[]);

#endif

