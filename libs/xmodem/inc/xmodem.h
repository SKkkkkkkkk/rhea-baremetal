#ifndef __XMODEM_H__
#define __XMODEM_H__

typedef int (*action_t)(unsigned char *buf, int buflen);
int xmodemReceiveWithAction(action_t action, int destsz);

#define XMODEM_ERROR_OK			 0
#define XMODEM_ERROR_CANCEL		-1
#define XMODEM_ERROR_SYNC		-2
#define XMODEM_ERROR_RETRY		-3
#define XMODEM_ERROR_USER_BEGIN	-6

#endif