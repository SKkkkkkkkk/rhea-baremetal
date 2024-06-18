#ifndef __XMODEM_H__
#define __XMODEM_H__

typedef void (*action_t)(unsigned char *buf, int buflen);
int xmodemReceiveWithAction(action_t action, int destsz);

#endif