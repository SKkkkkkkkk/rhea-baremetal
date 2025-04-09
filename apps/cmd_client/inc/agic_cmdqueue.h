#ifndef _AGIC_CMDQUEUE_H_
#define _AGIC_CMDQUEUE_H_

#ifndef NULL
#define NULL ((void *)0)
#endif

#define AGIC_CMD0	0x0 /* for download firmware to device */
#define AGIC_CMD1	0x1 /* for device change machine status to L1S */
#define AGIC_CMDNULL	0xff /* for send NULL command  to device */

#define CMDQUEUE_OFFSET (0xFE0000)
#define CMDQUEUE_ITEM_SIZE (sizeof(struct agic_cmd))
#define CMDQUEUE_ITEM_COUNT (500000)
#define CMDQUEUE_MAX_BOUNDARY (CMDQUEUE_ITEM_SIZE * CMDQUEUE_ITEM_COUNT)
#define STATUS_MACHINE_OFFSET (0xFF0000)

typedef struct __agic_cmdqueue agic_cmdqueue_t;
typedef struct agic_status_machine agic_status_machine_t;
typedef struct agic_cmdqueue_handler {
	char *cmdqueue_buffer;
	agic_cmdqueue_t *cmdqueue;
	agic_status_machine_t *status_machine;
} agic_cmdqueue_handler_t;

struct __agic_cmdqueue {
	unsigned int cmd_item_head;
	unsigned int cmd_item_tail;
	unsigned int put_lock;
	unsigned int put_head_lock;
	unsigned int get_lock;
};

#define AGIC_APPEND_MSGLEN 26
struct agic_cmd {
	short cmd_id;
	char append_msg[AGIC_APPEND_MSGLEN];
	unsigned int check_sum;
};

void agic_cmdqueue_create(agic_cmdqueue_handler_t *cmdq_handler,
			  char *cmdqueue_locate);
void agic_cmdqueue_fetch(agic_cmdqueue_handler_t *cmdq_handler,
			 char *cmdqueue_locate);
int agic_cmdqueue_cmd_get(agic_cmdqueue_handler_t *cmdq_handler,
			  struct agic_cmd *cmd);
void agic_cmdqueue_cmd_put(agic_cmdqueue_handler_t *cmdq_handler, short cmd_id,
			   const char *append_msg);
void agic_cmdqueue_cmd_put_head(agic_cmdqueue_handler_t *cmdq_handler,
				short cmd_id, const char *append_msg);

#define AGIC_STATUS_L0 0x10
#define AGIC_STATUS_L1 0x11
#define AGIC_STATUS_L1S 0x12
#define AGIC_STATUS_L2 0x13

#define AGIC_SUBSTATUS_DOING 'I'
#define AGIC_SUBSTATUS_DONE 'D'
#define AGIC_SUBSTATUS_IDLE 'X'
#define AGIC_SUBSTATUS_FAILD 'F'
#define AGIC_SUBSTATUS_END 'E'

#define AGIC_STATUSLEN 8 /* status_machine + cmd_id + substatus */
#define AGIC_MSGLEN 16

struct agic_status_machine {
	char status;
	char substatus;
	short cmd_id;
	char append_msg[AGIC_MSGLEN];
	unsigned int set_lock;
	unsigned int get_lock;
	unsigned int check_sum;
};

void agic_status_machine_set(agic_cmdqueue_handler_t *cmdq_handler, char status,
			     char substatus, short cmd_id,
			     const char *append_msg);
void agic_status_machine_set_substatus(agic_cmdqueue_handler_t *cmdq_handler,
				       char substatus);
char agic_status_machine_get_status(agic_cmdqueue_handler_t *cmdq_handler);
char agic_status_machine_get_substatus(agic_cmdqueue_handler_t *cmdq_handler);
short agic_status_machine_get_cmd_id(agic_cmdqueue_handler_t *cmdq_handler);
int agic_status_machine_get_msg(agic_cmdqueue_handler_t *cmdq_handler,
				char *append_msg);

#endif
