/* this code needs standard functions memcpy() and memset()
   and input/output functions _inbyte() and _outbyte().

   the prototypes of the input/output functions are:
     int _inbyte(unsigned short timeout); // msec timeout
     void _outbyte(int c);

 */
#include <stdint.h>
#include <string.h>
#include "crc.h"
#include "xmodem.h"

int _inbyte(uint64_t timeout); // msec timeout
void _outbyte(int c);

#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18
#define CTRLZ 0x1A

#define DLY_US 1000000
#define MAXRETRANS 25

static int check(int crc, const unsigned char *buf, int sz)
{
	if (crc) {
		unsigned short crc = crc16_ccitt(0, buf, sz);
		unsigned short tcrc = (buf[sz]<<8)+buf[sz+1];
		if (crc == tcrc)
			return 1;
	}
	else {
		int i;
		unsigned char cks = 0;
		for (i = 0; i < sz; ++i) {
			cks += buf[i];
		}
		if (cks == buf[sz])
		return 1;
	}

	return 0;
}

static void flushinput(void)
{
	while (_inbyte(((DLY_US)*3)>>1) >= 0)
		;
}

int xmodemReceiveWithAction(action_t action, int destsz)
{
	unsigned char xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
	unsigned char *p;
	int bufsz, crc = 0;
	unsigned char trychar = 'C';
	unsigned char packetno = 1;
	int i, c, len = 0;
	int retry, retrans = MAXRETRANS;

	for(;;) {
		for( retry = 0; retry < INT32_MAX; ++retry) {
			if (trychar) _outbyte(trychar);
			if ((c = _inbyte((DLY_US)<<1)) >= 0) {
				switch (c) {
				case SOH:
					bufsz = 128;
					goto start_recv;
				case STX:
					bufsz = 1024;
					goto start_recv;
				case EOT:
					flushinput();
					_outbyte(ACK);
					return len; /* normal end */
				case CAN:
					if ((c = _inbyte(DLY_US)) == CAN) {
						flushinput();
						_outbyte(ACK);
						return -1; /* canceled by remote */
					}
					break;
				default:
					break;
				}
			}
		}
		if (trychar == 'C') { trychar = NAK; continue; }
		flushinput();
		_outbyte(CAN);
		_outbyte(CAN);
		_outbyte(CAN);
		return -2; /* sync error */

	start_recv:
		if (trychar == 'C') crc = 1;
		trychar = 0;
		p = xbuff;
		*p++ = c;
		for (i = 0;  i < (bufsz+(crc?1:0)+3); ++i) {
			if ((c = _inbyte(DLY_US)) < 0) goto reject;
			*p++ = c;
		}

		if (xbuff[1] == (unsigned char)(~xbuff[2]) && 
			(xbuff[1] == packetno || xbuff[1] == (unsigned char)packetno-1) &&
			check(crc, &xbuff[3], bufsz)) {
			if (xbuff[1] == packetno)	{
				register int count = destsz - len;
				if (count > bufsz) count = bufsz;
				if (count > 0) {
					action(&xbuff[3], count);
					len += count;
				}
				++packetno;
				retrans = MAXRETRANS+1;
			}
			if (--retrans <= 0) {
				flushinput();
				_outbyte(CAN);
				_outbyte(CAN);
				_outbyte(CAN);
				return -3; /* too many retry error */
			}
			_outbyte(ACK);
			continue;
		}
	reject:
		flushinput();
		_outbyte(NAK);
	}
}

#ifndef XMODEM_RECEIVE_ONLY
int xmodemTransmit(unsigned char *src, int srcsz)
{
	unsigned char xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
	int bufsz, crc = -1;
	unsigned char packetno = 1;
	int i, c, len = 0;
	int retry;

	for(;;) {
		for( retry = 0; retry < 16; ++retry) {
			if ((c = _inbyte((DLY_US)<<1)) >= 0) {
				switch (c) {
				case 'C':
					crc = 1;
					goto start_trans;
				case NAK:
					crc = 0;
					goto start_trans;
				case CAN:
					if ((c = _inbyte(DLY_US)) == CAN) {
						_outbyte(ACK);
						flushinput();
						return -1; /* canceled by remote */
					}
					break;
				default:
					break;
				}
			}
		}
		_outbyte(CAN);
		_outbyte(CAN);
		_outbyte(CAN);
		flushinput();
		return -2; /* no sync */

		for(;;) {
		start_trans:
			xbuff[0] = SOH; bufsz = 128;
			xbuff[1] = packetno;
			xbuff[2] = ~packetno;
			c = srcsz - len;
			if (c > bufsz) c = bufsz;
			if (c >= 0) {
				memset (&xbuff[3], 0, bufsz);
				if (c == 0) {
					xbuff[3] = CTRLZ;
				}
				else {
					memcpy (&xbuff[3], &src[len], c);
					if (c < bufsz) xbuff[3+c] = CTRLZ;
				}
				if (crc) {
					unsigned short ccrc = crc16_ccitt(0, &xbuff[3], bufsz);
					xbuff[bufsz+3] = (ccrc>>8) & 0xFF;
					xbuff[bufsz+4] = ccrc & 0xFF;
				}
				else {
					unsigned char ccks = 0;
					for (i = 3; i < bufsz+3; ++i) {
						ccks += xbuff[i];
					}
					xbuff[bufsz+3] = ccks;
				}
				for (retry = 0; retry < MAXRETRANS; ++retry) {
					for (i = 0; i < bufsz+4+(crc?1:0); ++i) {
						_outbyte(xbuff[i]);
					}
					if ((c = _inbyte(DLY_US)) >= 0 ) {
						switch (c) {
						case ACK:
							++packetno;
							len += bufsz;
							goto start_trans;
						case CAN:
							if ((c = _inbyte(DLY_US)) == CAN) {
								_outbyte(ACK);
								flushinput();
								return -1; /* canceled by remote */
							}
							break;
						case NAK:
						default:
							break;
						}
					}
				}
				_outbyte(CAN);
				_outbyte(CAN);
				_outbyte(CAN);
				flushinput();
				return -4; /* xmit error */
			}
			else {
				for (retry = 0; retry < 10; ++retry) {
					_outbyte(EOT);
					if ((c = _inbyte((DLY_US)<<1)) == ACK) break;
				}
				flushinput();
				return (c == ACK)?len:-5;
			}
		}
	}
}
#endif