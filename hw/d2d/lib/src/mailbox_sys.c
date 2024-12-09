#include <mailbox_sys.h>

#include "clci_errno.h"

/*
* As the master, send specific length data to the slave via the mailbox
* The communication of mailbox is synchronous, which means before the slave received the data,
* handled the data, and send back the result, this function do not return
* parameters:                
* in_buff: points to the data to be send
* in_len: the length of the data to be send
* out_buff: points to memory to output the result getting from the slave
* out_len: length of the output buffer pointing by out_buff
* timeout_ms: the maximum time given to this function to executing, if the actual time is too long
*            CLCI_E_TIMEOUT will return.
* return value:
* positive interger: the size of data return back from the slave
* CLCI_E_TIMEOUT: timeout
* CLCI_E_DEVICE: the mailbox device is error
*/

int mailbox_sys_send(uint8_t *in_buff, uint8_t in_len, uint8_t *out_buff, uint8_t out_len, uint32_t timeout_ms)
{
	int ret;

	ret = mailbox_send((uint8_t *)in_buff, in_len, timeout_ms);

	if (CLCI_SUCCESS != ret)
		return ret;

	ret = mailbox_rev(out_buff, out_len, timeout_ms);

	return ret;
}
/*Init the mailbox as master*/
void mailbox_sys_init(void)
{
	mailbox_init(MAILBOX_ROLE_MASTER);
}
