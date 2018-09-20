//
// begin license header
//
// This file is part of Pixy CMUcam5 or "Pixy" for short
//
// All Pixy source code is provided under the terms of the
// GNU General Public License v2 (http://www.gnu.org/licenses/gpl-2.0.html).
// Those wishing to use Pixy source code, software and/or
// technologies under different licensing terms should contact us at
// cmucam@cs.cmu.edu. Such licensing terms are available for
// all portions of the Pixy codebase presented here.
//
// end license header
//

#ifndef _SERIAL_H
#define _SERIAL_H
#include "iserial.h"
#include "chirp.hpp"

// different interfaces
#define SER_INTERFACE_ARDUINO_SPI     0	  // arduino ICMP SPI (auto slave select)
#define SER_INTERFACE_SS_SPI          1	  // spi with slave select
#define SER_INTERFACE_I2C             2
#define SER_INTERFACE_UART            3
#define SER_INTERFACE_ADX             4
#define SER_INTERFACE_ADY             5 
#define	SER_INTERFACE_LEGO            6

// protocol stuff


#define SER_SYNC_MASK                 0xfff8
#define SER_CHECKSUM_FLAG             0x0001
#define SER_START_FLAG                0x0002
#define SER_END_FLAG                  0x0004
#define SER_SYNC_BASE                 0xc1a8

#define SER_SYNC_NO_CHECKSUM          (SER_SYNC_BASE|SER_START_FLAG|SER_END_FLAG)
#define SER_SYNC_CHECKSUM             (SER_SYNC_NO_CHECKSUM|SER_CHECKSUM_FLAG)
#define SER_MAXLEN                    0xff
#define SER_MIN_PACKET_HEADER         4
#define SER_PACKET_HEADER_CS_SIZE     sizeof(uint16_t) // size of checksum
#define SER_MAX_PACKET_HEADER         (SER_MIN_PACKET_HEADER + SER_PACKET_HEADER_CS_SIZE) // header + checksum
#define SER_TXBUF_SIZE                (SER_MAXLEN+SER_MAX_PACKET_HEADER) 

// types

// Requests and responses
#define SER_TYPE_RESPONSE_RESULT      1
#define SER_TYPE_RESPONSE_ERROR       3

#define SER_TYPE_REQUEST_RESOLUTION   0x0c  
#define SER_TYPE_RESPONSE_RESOLUTION  0x0d  
#define SER_TYPE_REQUEST_VERSION      0x0e
#define SER_TYPE_RESPONSE_VERSION     0x0f

#define SER_TYPE_REQUEST_CHANGE_PROG  0x02   
#define SER_TYPE_REQUEST_BRIGHTNESS   0x10
#define SER_TYPE_REQUEST_SERVO        0x12
#define SER_TYPE_REQUEST_LED          0x14
#define SER_TYPE_REQUEST_LAMP         0x16
#define SER_TYPE_REQUEST_FPS          0x18
#define SER_TYPE_REQUEST_NO_PROG_MAX  0x1f

// error codes
#define SER_ERROR_GENERAL             -1
#define SER_ERROR_BUSY                -2
#define SER_ERROR_INVALID_REQUEST     -3
#define SER_ERROR_TYPE_UNSUPPORTED    -4
#define SER_ERROR_BUTTON_OVERRIDE     -5
#define SER_ERROR_PROG_CHANGING       -6

int ser_init(Chirp *chirp);
int32_t ser_packetChirp(const uint8_t &type, const uint32_t &len, const uint8_t *request, Chirp *chirp=NULL);
int ser_setInterface(uint8_t interface);
uint8_t ser_getInterface();
uint8_t ser_getTx(uint8_t **data);
void ser_setTx(uint8_t type, uint8_t len, bool checksum);

void ser_sendResult(int32_t val, bool checksum);
void ser_sendError(int8_t error, bool checksum);
uint8_t ser_getByte(uint8_t *c);
bool ser_newPacket();
void ser_rxCallback();
void ser_update();
void ser_setReady();

Iserial *ser_getSerial();

void ser_loadParams();

extern int8_t g_interface;
extern bool g_oldProtocol;

#endif
