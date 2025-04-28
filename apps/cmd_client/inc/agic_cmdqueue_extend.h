#ifndef _AGIC_CMDQUEUE_EXTEND_H_
#define _AGIC_CMDQUEUE_EXTEND_H_

#include "agic_cmdqueue.h"

#define AGIC_CMD2 0x2
#define AGIC_CMD3 0x3
#define AGIC_CMD4 0x4 /* for get device temperature info */
#define AGIC_CMD5 0x5 /* for get version information */
#define AGIC_CMD6 0x6 /* for get device fan speed info */
#define AGIC_CMD7 0x7 /* for get device frequency info */
#define AGIC_CMD8 0x8 /* for get device power info */
#define AGIC_CMD9 0x9 /* for send file to device */
#define AGIC_CMD10 0xa /* for send c2c message to device */
#define AGIC_CMD11 0xb /* for execute runtime app */
#define AGIC_HEART 0xfe /* for get heartbeat packet from device */

#endif
