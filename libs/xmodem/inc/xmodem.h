#ifndef __XMODEM_H__
#define __XMODEM_H__

// action_t - action type
// @buf: buffer containing the data to be processed
// @buflen: length of the buffer
// @return: 0 on success, error code otherwise
typedef unsigned int (*action_t)(unsigned char *buf, int buflen);

// xmodemReceiveWithAction - XMODEM receive with action
// @action: action to be performed on the received data
// @maxsz:  maximum size of the received data
// @return: < 0 on error, size of the received data otherwise
int xmodemReceiveWithAction(action_t action, int maxsz);

#ifndef XMODEM_RECEIVE_ONLY
int xmodemTransmit(unsigned char *src, int srcsz);
#endif

#endif