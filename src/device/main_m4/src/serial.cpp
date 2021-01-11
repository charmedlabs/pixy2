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

#include <string.h>
#include "serial.h"
#include "spi.h"
#include "spi2.h"
#include "i2c.h"
#include "uart.h"
#include "analogdig.h"
#include "conncomp.h"
#include "param.h"
#include "pixy_init.h"
#include "exec.h"
#include "camera.h"
#include "led.h"
#include "rcservo.h"
#include "line.h"
#include "progvideo.h"
#include "calc.h"

static const ProcModule g_module[] =
{
	{
	"ser_packet",
	(ProcPtr)ser_packetChirp, 
	{CRP_UINT8, CRP_UINTS8, END}, 
	"Send program-related sensor data based on request"
	"@p type request type identifier"
	"@p data request data"
	"@r returns 0 regardless and return data array of bytes based on request"
	},
	END
};

struct BrightnessQ
{
	bool m_valid;
	uint8_t m_brightness;
};

int8_t g_interface = -1;
int8_t g_angle = 0;
bool g_oldProtocol = false;
static uint8_t g_state = 0;
static Iserial *g_serial = 0;
static uint8_t g_txBuf[SER_TXBUF_SIZE]; 
static uint8_t *g_tx;
static uint16_t g_txReadIndex; // current read index
static uint16_t g_txLen; // current length
static bool g_newPacket = false; 
static BrightnessQ g_brightnessQ;
static bool g_ready = false;

uint16_t lego_getData(uint8_t *buf, uint32_t buflen)
{
	static uint16_t lastReverse = 0xffff;
	static uint8_t lastLamp = 0;
	uint8_t c, reverse, lamp;
	uint16_t d, x, y;
	bool sat;
	int8_t turn;
	int16_t turn16;
	uint16_t numBlobs;
	uint32_t temp, width, height, r, g, b;
	Iserial *serial = ser_getSerial();
	bool error = false;
	static int8_t ccc = -1;
	static int8_t line = -1;
	static int8_t video = -1;
	
	if (serial->receive(&c, 1)==0)
		return 0;

	if (c>=0x50 && c<=0x59)
	{
		if (ccc<0)
			ccc = exec_getProgIndex("color_connected_components");
		error = exec_setProgIndex(ccc)!=ccc;
	}
	else if (c>=0x5a && c<=0x5d)
	{
		if (line<0)
			line = exec_getProgIndex("line_tracking");
		error = exec_setProgIndex(line)!=line;
	}
	else if (c==0x5e)
	{
		if (video<0)
			video = exec_getProgIndex("video");
		error = exec_setProgIndex(video)!=video;
	}
	
		
#if 1
	if (c==0x00)
	{
		const char *str = "V0.4";
		strcpy((char *)buf, str);
		return 5;
	}
	if (c==0x08)
	{
		const char *str = "Pixy2";
		strcpy((char *)buf, str);
		return 6;
	}
	else if (c==0x10)
	{
		const char *str = "Pixy2";
		strcpy((char *)buf, str);
		return 6;		
	}
	else 
#endif
	if (c==0x50)
	{
		BlobA *max;
#if 0
		buf[0] = 1;
		buf[1] = 2;
		buf[2] = 3;
		buf[3] = 4;
		buf[4] = 5;
		buf[5] = 6;
		buf[6] = 7;
#else
		max = (BlobA *)g_blobs->getMaxBlob();
		if (max==0)
			memset(buf, 0, 7);
		else if (max==(BlobA *)-1 || error)
			memset(buf, -1, 7);
		else
		{
			width = max->m_right - max->m_left;
			height = max->m_bottom - max->m_top;
			*(uint16_t *)buf = max->m_model; // signature
			temp = ((max->m_left + width/2)*829)>>10;
			buf[2] = temp; // x
			temp = ((max->m_top + height/2)*1262)>>10;
			buf[3] = temp; // y
			temp = (width*829)>>10;
			buf[4] = temp; // width
			temp = (height*1262)>>10;
			buf[5] = temp; // height
			if (max->m_model>CL_NUM_SIGNATURES)
			{
				temp = ((int32_t)max->m_angle*91)>>7;
				g_angle = temp;
			}
		}
#endif		
		return 6;
	}
	else if (c==0x60)
	{
		buf[0] = g_angle;
		return 1;
	}
	else if (c>=0x51 && c<=0x57)
	{
#if 0
		buf[0] = 1;
		buf[1] = 2;
		buf[2] = 3;
		buf[3] = 4;
		buf[4] = 5;
#else
		BlobA *max;
		max = g_blobs->getMaxBlob(c-0x50, &numBlobs);
		if (max==0)
			memset(buf, 0, 5);
		else if (max==(BlobA *)-1 || error)
			memset(buf, -1, 5);
		else
		{
			width = max->m_right - max->m_left;
			height = max->m_bottom - max->m_top;
			buf[0] = numBlobs; // number of blocks that match signature
			temp = ((max->m_left + width/2)*829)>>10;
			buf[1] = temp; // x
			temp = ((max->m_top + height/2)*1262)>>10;
			buf[2] = temp;	// y
			temp = (width*829)>>10;
			buf[3] = temp; // width
			temp = (height*1262)>>10;
			buf[4] = temp; // height
		}
#endif
		return 5;
	}
	else if (c==0x58)
	{
		BlobA *max;
		if (serial->receive((uint8_t *)&d, 2)<2) // receive cc signature to look for
			return 0;
#if 0
		buf[0] = 1;
		buf[1] = 2;
		buf[2] = 3;
		buf[3] = 4;
		buf[4] = 5;
		buf[5] = 6;
#else
		max = (BlobA *)g_blobs->getMaxBlob(d, &numBlobs); 
		if (max==0)
			memset(buf, 0, 6);
		else if (max==(BlobA *)-1 || error)
			memset(buf, -1, 6);
		else
		{
			width = max->m_right - max->m_left;
			height = max->m_bottom - max->m_top;
			buf[0] = numBlobs; // number of cc blocks that match 
			temp = ((max->m_left + width/2)*829)>>10;
			buf[1] = temp; // x
			temp = ((max->m_top + height/2)*1262)>>10;
			buf[2] = temp; // y
			temp = (width*829)>>10;
			buf[3] = temp; // width
			temp = (height*1262)>>10;
			buf[4] = temp; // height
			temp = ((int32_t)max->m_angle*91)>>7;
			buf[5] = temp; // angle
		}
#endif
		return 6;
	}
	else if (c==0x5a)
	{
		if (serial->receive((uint8_t *)&turn, 1)==1 && 
			serial->receive((uint8_t *)&reverse, 1)==1 &&
			serial->receive((uint8_t *)&lamp, 1)==1 &&
			error==false)
		{
			turn16 = turn;
			turn16 *= 180; // scale accordingly
			turn16 /= 127;
			line_setNextTurnAngle(turn16);
			if (lastReverse!=0xffff && lastReverse!=reverse)
				line_reversePrimary();
			if (lastLamp!=lamp)
			{
				if (lamp)
					led_setLamp(0xff, 0xff);
				else
					led_setLamp(0, 0);
			}
			
			lastReverse = reverse;
			lastLamp = lamp;
			return line_legoLineData(buf, buflen);
			}
		else 
		{
			memset(buf, -1, 4);
			return 4;
		}
	}
	else if (c==0x5e) // get RGB
	{
		if (serial->receive(buf, 3)==3 && 
			error==false)
		{
			x = buf[0]*(CAM_RES2_WIDTH-1)/255;
			y = buf[1]*(CAM_RES2_HEIGHT-1)/255;
			sat = buf[2];
			temp = getRGB(x, y, sat);
			rgbUnpack(temp, &r, &g, &b);
			buf[0] = r;
			buf[1] = g;
			buf[2] = b;
			return 3;
		}
		else
		{
			memset(buf, -1, 3);
			return 3;
		}
	}
	else if (c==0x62)
	{
		if (serial->receive((uint8_t *)&lamp, 1)==1 && 
			error==false)
			{
				if (exec_getProg(ccc)==true)
					led_setLamp(lamp ? 0xff : 0, 0);
				else if (lamp)
					led_setLamp(0xff, 0xff);
				else
					led_setLamp(0, 0);
			}
		buf[0] = 1;	
		return 1;
	}
	else  
	{
#if 0
		static uint8_t c = 0;

		buf[0] = c++;
#else
		//printf("%x\n", c);														  

		if (c==0x42) // this works in port view mode on the ev3's LCD
		{
			BlobA *max;
			max = g_blobs->getMaxBlob();
			if (max==0 || max==(BlobA *)-1)
				buf[0] = 0;
			else
			{
				width = max->m_right - max->m_left;
				temp = ((max->m_left + width/2)*829)>>10;
				buf[0] = temp;
			}
		}
		else
			buf[0] = 1;	 // need to return nonzero value for other inquiries or LEGO brick will think we're an analog sensor

#endif
		return 1;
	}
}

void ser_sendResult(int32_t val, bool checksum)
{
	uint8_t *txData;
	ser_getTx(&txData);
	*(int32_t *)txData = val; // write val
			
	ser_setTx(SER_TYPE_RESPONSE_RESULT, sizeof(int32_t), checksum);				
}

void ser_sendError(int8_t error, bool checksum)
{
	uint8_t *txData;
	ser_getTx(&txData);
	txData[0] = error;
	ser_setTx(SER_TYPE_RESPONSE_ERROR, 1, checksum);	
}


void ser_packet(uint8_t type, const uint8_t *rxData, uint8_t len, bool checksum)
{
	uint8_t *txData;
	int res;
	
	// first check if current program can handle request 
	if (type>SER_TYPE_REQUEST_NO_PROG_MAX)
		exec_progPacket(type, rxData, len, checksum);
	else if (type==SER_TYPE_REQUEST_CHANGE_PROG)
	{
		res = exec_runProgName((const char *)rxData);
		if (res<0)
			ser_sendError(SER_ERROR_INVALID_REQUEST, checksum);
		else
			ser_sendResult(res, checksum);
	}
	else if (type==SER_TYPE_REQUEST_RESOLUTION)
	{
		if (len!=1)
			ser_sendError(SER_ERROR_INVALID_REQUEST, checksum);	
		else
			if (exec_progResolution(rxData[0], checksum)<0)
				ser_sendError(SER_ERROR_PROG_CHANGING, checksum);
	}
	else if (type==SER_TYPE_REQUEST_VERSION) // get version information
	{
		uint32_t hwVal;
		
		// this mechanism provides feedback to serial client that we're ready to accept input after "boot-up"
		// If g_ready is false, we're not ready...
		if (!g_ready)
		{
			ser_sendError(SER_ERROR_BUSY, checksum);
			return;
		}
				
		ser_getTx(&txData);
		// hw version, first 2 bytes
		hwVal = *(uint32_t *)(0x40045000+0x38);
		if (hwVal>>16==0xc1ab)
			*(uint16_t *)(txData + 0) = hwVal&0xffff;
		else // if value isn't set, assume Pixy version 1.3b 
			*(uint16_t *)(txData + 0) = 0x1301;				
		// fw version, next 4 bytes
		*(uint8_t *)(txData + 2) = FW_MAJOR_VER;
		*(uint8_t *)(txData + 3) = FW_MINOR_VER;
		*(uint16_t *)(txData + 4) = FW_BUILD_VER;
		
		// fw type, next 10 bytes
		strncpy((char *)txData+6, FW_TYPE, 10);
			
		ser_setTx(SER_TYPE_RESPONSE_VERSION, 16, checksum);
	}
	else if (type==SER_TYPE_REQUEST_BRIGHTNESS) // set brightness
	{
		if (len!=1)
			ser_sendError(SER_ERROR_INVALID_REQUEST, checksum);
		else
		{
			// "queue" up the brightness request because it goes out over i2c and takes time
			g_brightnessQ.m_valid = true;
			g_brightnessQ.m_brightness = rxData[0];
			ser_sendResult(0, checksum);
		}
	}
	else if (type==SER_TYPE_REQUEST_SERVO) // set servo positions
	{
		if (len!=4)
			ser_sendError(SER_ERROR_INVALID_REQUEST, checksum);
		else
		{
			rcs_setPos(0, *(uint16_t *)rxData);
			rcs_setPos(1, *(uint16_t *)&rxData[2]);
			ser_sendResult(0, checksum);
		}
	}
	else if (type==SER_TYPE_REQUEST_LED) // set LED 
	{
		if (len!=3)
			ser_sendError(SER_ERROR_INVALID_REQUEST, checksum);
		else
		{
			// set override, user is now in control
			cc_setLEDOverride(true);
			led_setRGB(rxData[0], rxData[1], rxData[2]);
			ser_sendResult(0, checksum);
		}
	}
	else if (type==SER_TYPE_REQUEST_LAMP) // set lamp
	{
		if (len!=2)
			ser_sendError(SER_ERROR_INVALID_REQUEST, checksum);
		else
		{
			led_setLamp(rxData[0], rxData[1]);
			ser_sendResult(0, checksum);				
		}				
	}
	else if (type==SER_TYPE_REQUEST_FPS) // get frames per second
	{
		float fps = cam_getFPS() + 0.5f;
		uint32_t val = (uint32_t)fps; // convert to int, round up or down
		ser_sendResult(val, checksum);				
	}
	else // not able to find handler, return error
		ser_sendError(SER_ERROR_TYPE_UNSUPPORTED, checksum);		
}

int32_t ser_packetChirp(const uint8_t &type, const uint32_t &len, const uint8_t *request, Chirp *chirp)
{
	// handle packet without checksum
	ser_packet(type, request, len, false);
	// send result data minus the header data, which we'll bring out explicitly (type, length, no sync)
	CRP_RETURN(chirp, UINT8(g_tx[2]) /* type */, UINTS8(g_tx[3] /* len */, g_tx+SER_MIN_PACKET_HEADER) /* raw data */, END);
	
	// return 0 regardless.  Actual result is returned in the g_tx data.
	return 0;
}

// TX data return mechanism for old serial protocol (v1.0-2.0)
uint32_t txCallback(uint8_t *data, uint32_t len)
{
	if (g_interface==SER_INTERFACE_LEGO)
		return lego_getData(data, len);
	else 
		return g_blobs->getBlock(data, len);
}

// TX data return mechanism for new serial protocol (v3.0--)
uint8_t ser_getByte(uint8_t *c)
{
	if (g_txReadIndex>=g_txLen)
		return 0;
	*c = g_tx[g_txReadIndex++];
	return 1;
}

void ser_rxCallback()
{
	// parse, figure out if the message was intended for us, otherwise pass to currently running program
	uint8_t i, a, oldState, buf[SPI2_RECEIVEBUF_SIZE];
	uint16_t csCalc;
	static uint16_t w, csStream;
	static uint8_t lastByte, type, len;
	
	while(1)
	{
		oldState = g_state;
		switch(g_state)
		{	
		case 0: // reset 
			lastByte = 0xff;  // This is not part of any of the sync word most significant bytes
			g_state = 1;
		 	break;

		case 1:	// sync word
			if (g_serial->receive(&a, 1))
			{
				w = a << 8;
				w |= lastByte;
				lastByte = a;
				g_state = 2;	// compare
			}
			break;
			
		case 2:	 // receive data byte(s)
			if (w==SER_SYNC_NO_CHECKSUM)
			{	// read rest of data
				if (g_serial->receiveLen()>=2)
				{
					g_serial->receive(&type, 1);
					g_serial->receive(&len, 1);
					
					g_state = 3;
				}
			}
			else if (w==SER_SYNC_CHECKSUM)
			{
				if (g_serial->receiveLen()>=4)
				{
					g_serial->receive(&type, 1);
					g_serial->receive(&len, 1);
					g_serial->receive((uint8_t *)&csStream, 2);
					
					g_state = 4;
				}
			}
			else
				g_state = 1;
			break;

		case 3:
			if (len<=SPI2_RECEIVEBUF_SIZE)
			{
				if (g_serial->receiveLen()>=len)
				{
					g_serial->receive(buf, len);
					g_state = 5;
				}
			}
			else
				g_state = 0;
			break;
			
		case 4:
			if (len<=SPI2_RECEIVEBUF_SIZE)
			{
				if (g_serial->receiveLen()>=len)
				{
					g_serial->receive(buf, len);
					for (i=0, csCalc=0; i<len; i++)
						csCalc += buf[i];
					if (csCalc==csStream)
						g_state = 5;
					else 
						g_state = 0;
				}
			}
			else
				g_state = 0;
			break;
			
		case 5:
			ser_packet(type, buf, len, true);
			g_state = 0;
			break;
		
		default:
			g_state = 0; // try another whole word
			break;
		}
		if (oldState==g_state)
			break;
	}
}

// These routines (getTx and setTx) are expected to be called from within an ISR, otherwise there will be a race condition between writing to 
// the tx buffer and the txCallback reading the tx buffer. 
uint8_t ser_getTx(uint8_t **data)
{
	*data = g_txBuf+SER_MAX_PACKET_HEADER; // make room for header
	return SER_TXBUF_SIZE-SER_MAX_PACKET_HEADER;
}

void ser_setTx(uint8_t type, uint8_t len, bool checksum)
{
	uint8_t i;
	uint16_t cs;
	
	g_txReadIndex = 0;
	g_txLen = SER_MIN_PACKET_HEADER + len;
	if (checksum)
	{	
		g_tx = g_txBuf;
		*(uint16_t *)g_tx = SER_SYNC_CHECKSUM;
		for (i=0, cs=0; i<len; i++)
			cs += g_txBuf[SER_MAX_PACKET_HEADER + i];
		*(uint16_t *)(g_tx+4) = cs;
		g_txLen += SER_PACKET_HEADER_CS_SIZE;
	}
	else
	{
		g_tx = g_txBuf + SER_PACKET_HEADER_CS_SIZE;
		*(uint16_t *)g_tx = SER_SYNC_NO_CHECKSUM;
	}
	g_tx[2] = type;
	g_tx[3] = len;
	g_newPacket = true;
	g_serial->startTransmit();
}

bool ser_newPacket()
{
	bool result = g_newPacket;
	g_newPacket = false;
	return result;
}

int ser_init(Chirp *chirp)
{
	chirp->registerModule(g_module);
	i2c_init(txCallback);
	uart_init(txCallback);
	ad_init();

	ser_loadParams();
		
	return 0;	
}

void ser_loadParams()
{
#ifndef LEGO
	prm_add("Data out port", 0, PRM_PRIORITY_1, 
		"Selects the port that's used to output data (default Arduino ICSP SPI) @c Interface @s 0=Arduino_ICSP_SPI @s 1=SPI_with_SS @s 2=I2C @s 3=UART @s 4=analog/digital_x @s 5=analog/digital_y @s 6=LEGO_I2C", UINT8(0), END);
	prm_add("I2C address", PRM_FLAG_HEX_FORMAT, PRM_PRIORITY_1-1, 
		"@c Interface Sets the I2C address if you are using I2C data out port. (default 0x54)", UINT8(I2C_DEFAULT_SLAVE_ADDR), END);
	prm_add("UART baudrate", 0, PRM_PRIORITY_1-2, 
		"@c Interface Sets the UART baudrate if you are using UART data out port. (default 19200)", UINT32(19200), END);
	prm_add("Pixy 1.0 compatibility mode", PRM_FLAG_CHECKBOX, PRM_PRIORITY_1-3, 
		"@c Interface If this is set, Pixy will return data using the Pixy 1.0 protocol.  This only applies to color connected components program, not other programs. (default false)", UINT8(0), END);

	uint8_t interface, addr;
	uint32_t baudrate;

	prm_get("I2C address", &addr, END);
	g_i2c0->setSlaveAddr(addr);

	prm_get("UART baudrate", &baudrate, END);
	g_uart0->setBaudrate(baudrate);

	prm_get("Data out port", &interface, END);
	ser_setInterface(interface);

#else
	ser_setInterface(SER_INTERFACE_LEGO);
#endif
}

int ser_setInterface(uint8_t interface)
{
	if (interface>SER_INTERFACE_LEGO)
		return -1;
	
	if (g_serial!=NULL)
		g_serial->close();

	// get g_oldProtocol after we close to prevent race condition with spi interrupt routine
	prm_get("Pixy 1.0 compatibility mode", &g_oldProtocol, END);
	
	// reset variables
	g_state = 0;
	g_interface = interface;
	g_txReadIndex = 0; 
	g_txLen = 0; 
	g_tx = g_txBuf;
	g_brightnessQ.m_valid = false;

	switch (interface)
	{		    
	case SER_INTERFACE_SS_SPI:
		if (g_oldProtocol)
		{
			spi_deinit();
			spi2_deinit();
			spi_init(txCallback);
			g_serial = g_spi;
			g_spi->setAutoSlaveSelect(false);
		}
		else
		{
			spi_deinit();
			spi2_deinit();
			spi2_init();
			g_serial = g_spi2;
			g_spi2->setAutoSlaveSelect(false);
		}
		break;

	case SER_INTERFACE_I2C:     
		g_serial = g_i2c0;
		g_i2c0->setFlags(false, true);
		break;

	case SER_INTERFACE_UART:    
		g_serial = g_uart0;
		break;

	case SER_INTERFACE_ADX:      
		g_ad->setDirection(true);
		g_serial = g_ad;
		break;

	case SER_INTERFACE_ADY:
		g_ad->setDirection(false);
		g_serial = g_ad;
		break;		

	case SER_INTERFACE_LEGO:
		g_serial = g_i2c0;
 		g_i2c0->setSlaveAddr(0x01);
		g_i2c0->setFlags(true, false);
		g_oldProtocol = true;
		break;
		
	default:
	case SER_INTERFACE_ARDUINO_SPI:
		if (g_oldProtocol)
		{
			spi_deinit();
			spi2_deinit();
			spi_init(txCallback);
			g_serial = g_spi;
			g_spi->setAutoSlaveSelect(true);
		}
		else
		{
			spi_deinit();
			spi2_deinit();
			spi2_init();
			g_serial = g_spi2;
			g_spi2->setAutoSlaveSelect(true);
		}			
		break;
	}

	g_serial->open();

	return 0;
}

void ser_update()
{
	// handled queued commands 
	// Brightness change
	if (g_brightnessQ.m_valid)
	{
		cam_setBrightness(g_brightnessQ.m_brightness);
		g_brightnessQ.m_valid = false;
	}
	// update serial channel
	ser_getSerial()->update();
}
	
void ser_setReady()
{
	g_ready = true;
}


uint8_t ser_getInterface()
{
	return g_interface;
}

Iserial *ser_getSerial()
{
	return g_serial;
}
