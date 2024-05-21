#include <stdio.h>
#include <stdlib.h>
#include "gicv3.h"
#include "mailbox.h"
// #include "systimer.h"

#define Mailbox_IRQn	59

static inline void udelay(uint64_t x)
{
	for(uint32_t i = x; i!=0; i--)
		asm volatile("");
}


static volatile uint32_t mailbox_i = 0U;
void mailbox_irqhandler(void)
{
  	uint32_t sta;
	sta = MAILBOX_B2A->b2a_status;
  	for(int i = 0; i < 4; i++) {
  	  	if(((sta >> i)&1) == 1) {
			printf("A55 receive: cmd: %u  data: %u by ch%u\n\r", a_get_cmd(i), a_get_data(i), i);
			switch (i) {
			case 0:
				MAILBOX_B2A->b2a_status = 0x01;
				break;
			case 1:
				MAILBOX_B2A->b2a_status = 0x02;
				break;
			case 2:
				MAILBOX_B2A->b2a_status = 0x04;
				break;
			case 3:
				MAILBOX_B2A->b2a_status = 0x08;
				break;
			default:
				break;
			}
  	  	}
  	}
}

static void a2b_init()
{
	MAILBOX_A2B->a2b_status = 0xf; // Clear the interrupt by writing 1 to this bit.
    IRQ_SetHandler(Mailbox_IRQn, mailbox_irqhandler);
    GIC_SetPriority(Mailbox_IRQn, 0 << 3);
    GIC_EnableIRQ(Mailbox_IRQn);
}

int main()
{
	GIC_Distributor_Init();
	GIC_Redistributor_Init();
	GIC_CPUInterfaceInit();

  	a2b_init();

	for(uint32_t i = 0; i < 4; i++) {
		a2b_send(i, i, i);
		printf("A55 send cmd: %u, data: %u by ch%u\n\r", i, i, i);
	}


	while(1) {
		asm volatile ("nop");
	};
}
