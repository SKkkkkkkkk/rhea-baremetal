// SPDX-License-Identifier: GPL-2.0-only
/*
 * Command Queue API
 *
 * Copyright (c) 2024, AGIC micro.
 */
#include "agic_cmdqueue.h"

void agic_cmdqueue_cmd_put(agic_cmdqueue_handler_t *cmdq_handler, short cmd_id,
			   const char *append_msg)
{
	int i = 0;
	unsigned char *char_ptr = NULL;
	struct agic_cmd *local_cmd = NULL;

	while (cmdq_handler->cmdqueue->get_lock)
		;
	cmdq_handler->cmdqueue->put_lock = 1;

	if (cmdq_handler->cmdqueue->cmd_item_tail == CMDQUEUE_MAX_BOUNDARY)
		cmdq_handler->cmdqueue->cmd_item_tail = 0;

	local_cmd = (struct agic_cmd *)(cmdq_handler->cmdqueue_buffer +
					cmdq_handler->cmdqueue->cmd_item_tail);

	local_cmd->cmd_id = cmd_id;
	for (i = 0; i < AGIC_APPEND_MSGLEN; i++) {
		local_cmd->append_msg[i] = append_msg[i];
	}

	char_ptr = (unsigned char *)local_cmd;
	local_cmd->check_sum = 0;

	for (i = 0; i < (sizeof(short) + sizeof(char) * AGIC_APPEND_MSGLEN);
	     i++) {
		local_cmd->check_sum += char_ptr[i];
	}

	cmdq_handler->cmdqueue->cmd_item_tail += CMDQUEUE_ITEM_SIZE;

	cmdq_handler->cmdqueue->put_lock = 0;
}

void agic_cmdqueue_cmd_put_head(agic_cmdqueue_handler_t *cmdq_handler,
				short cmd_id, const char *append_msg)
{
	int i = 0;
	unsigned char *char_ptr = NULL;
	struct agic_cmd *local_cmd = NULL;

	while (cmdq_handler->cmdqueue->get_lock)
		;
	cmdq_handler->cmdqueue->put_head_lock = 1;

	if (cmdq_handler->cmdqueue->cmd_item_head == 0) {
		cmdq_handler->cmdqueue->cmd_item_head =
			CMDQUEUE_MAX_BOUNDARY - CMDQUEUE_ITEM_SIZE;
	} else {
		cmdq_handler->cmdqueue->cmd_item_head -= CMDQUEUE_ITEM_SIZE;
	}

	local_cmd = (struct agic_cmd *)(cmdq_handler->cmdqueue_buffer +
					cmdq_handler->cmdqueue->cmd_item_head);

	local_cmd->cmd_id = cmd_id;
	for (i = 0; i < AGIC_APPEND_MSGLEN; i++) {
		local_cmd->append_msg[i] = append_msg[i];
	}

	char_ptr = (unsigned char *)local_cmd;
	local_cmd->check_sum = 0;

	for (i = 0; i < (sizeof(short) + sizeof(char) * AGIC_APPEND_MSGLEN);
	     i++) {
		local_cmd->check_sum += char_ptr[i];
	}
	cmdq_handler->cmdqueue->put_head_lock = 0;
}

int agic_cmdqueue_cmd_get(agic_cmdqueue_handler_t *cmdq_handler,
			  struct agic_cmd *cmd)
{
	unsigned int i = 0;
	unsigned int check_sum = 0;
	unsigned char *char_ptr = NULL;
	struct agic_cmd *local_cmd = NULL;

	while (cmdq_handler->cmdqueue->put_lock ||
	       cmdq_handler->cmdqueue->put_head_lock)
		;
	cmdq_handler->cmdqueue->get_lock = 1;

	if (cmd == NULL || cmdq_handler->cmdqueue == NULL)
		return -1;

	if (cmdq_handler->cmdqueue->cmd_item_tail !=
	    cmdq_handler->cmdqueue->cmd_item_head) {
		if (cmdq_handler->cmdqueue->cmd_item_head ==
		    CMDQUEUE_MAX_BOUNDARY)
			cmdq_handler->cmdqueue->cmd_item_head = 0;
		local_cmd = (struct agic_cmd *)(cmdq_handler->cmdqueue_buffer +
						cmdq_handler->cmdqueue
							->cmd_item_head);

		char_ptr = (unsigned char *)local_cmd;
		for (i = 0;
		     i < (sizeof(short) + sizeof(char) * AGIC_APPEND_MSGLEN);
		     i++) {
			check_sum += char_ptr[i];
		}
		if (check_sum != local_cmd->check_sum) {
			cmdq_handler->cmdqueue->get_lock = 0;
			return -2;
		}

		cmd->cmd_id = local_cmd->cmd_id;
		for (i = 0; i < AGIC_APPEND_MSGLEN; i++) {
			cmd->append_msg[i] = local_cmd->append_msg[i];
		}

		cmdq_handler->cmdqueue->cmd_item_head += CMDQUEUE_ITEM_SIZE;
	} else {
		cmdq_handler->cmdqueue->get_lock = 0;
		return -3;
	}

	cmdq_handler->cmdqueue->get_lock = 0;
	return 0;
}

void agic_cmdqueue_create(agic_cmdqueue_handler_t *cmdq_handler,
			  char *cmdqueue_locate)
{
	cmdq_handler->cmdqueue_buffer = cmdqueue_locate;
	cmdq_handler->cmdqueue =
		(agic_cmdqueue_t *)(cmdqueue_locate + CMDQUEUE_OFFSET);
	cmdq_handler->status_machine =
		(agic_status_machine_t *)(cmdqueue_locate +
					  STATUS_MACHINE_OFFSET);

	cmdq_handler->cmdqueue->cmd_item_head = 0;
	cmdq_handler->cmdqueue->cmd_item_tail = 0;
	cmdq_handler->cmdqueue->put_lock = 0;
	cmdq_handler->cmdqueue->put_head_lock = 0;
	cmdq_handler->cmdqueue->get_lock = 0;

	cmdq_handler->status_machine->get_lock = 0;
	cmdq_handler->status_machine->set_lock = 0;
}

void agic_cmdqueue_fetch(agic_cmdqueue_handler_t *cmdq_handler,
			 char *cmdqueue_locate)
{
	cmdq_handler->cmdqueue_buffer = cmdqueue_locate;
	cmdq_handler->cmdqueue =
		(agic_cmdqueue_t *)(cmdqueue_locate + CMDQUEUE_OFFSET);
	cmdq_handler->status_machine =
		(agic_status_machine_t *)(cmdqueue_locate +
					  STATUS_MACHINE_OFFSET);
}

void agic_status_machine_set(agic_cmdqueue_handler_t *cmdq_handler, char status,
			     char substatus, short cmd_id,
			     const char *append_msg)
{
	int i;
	unsigned char *char_ptr = NULL;

	while (cmdq_handler->status_machine->get_lock)
		;
	cmdq_handler->status_machine->set_lock = 1;

	cmdq_handler->status_machine->status = status;
	cmdq_handler->status_machine->substatus = substatus;
	cmdq_handler->status_machine->cmd_id = cmd_id;

	for (i = 0; i < AGIC_MSGLEN; i++) {
		cmdq_handler->status_machine->append_msg[i] = append_msg[i];
	}

	cmdq_handler->status_machine->check_sum = 0;

	char_ptr = (unsigned char *)cmdq_handler->status_machine;
	for (i = 0;
	     i < (sizeof(agic_status_machine_t) - (sizeof(unsigned int) * 3));
	     i++) {
		cmdq_handler->status_machine->check_sum += char_ptr[i];
	}

	cmdq_handler->status_machine->set_lock = 0;
}

void agic_status_machine_set_substatus(agic_cmdqueue_handler_t *cmdq_handler,
				       char substatus)
{
	int i;
	unsigned char *char_ptr = NULL;

	while (cmdq_handler->status_machine->get_lock)
		;
	cmdq_handler->status_machine->set_lock = 1;

	cmdq_handler->status_machine->substatus = substatus;
	cmdq_handler->status_machine->check_sum = 0;

	char_ptr = (unsigned char *)cmdq_handler->status_machine;
	for (i = 0;
	     i < (sizeof(agic_status_machine_t) - (sizeof(unsigned int) * 3));
	     i++) {
		cmdq_handler->status_machine->check_sum += char_ptr[i];
	}

	cmdq_handler->status_machine->set_lock = 0;
}

char agic_status_machine_get_status(agic_cmdqueue_handler_t *cmdq_handler)
{
	int i = 0;
	unsigned char *char_ptr = NULL;
	unsigned int check_sum = 0;
	char status;

	while (cmdq_handler->status_machine->set_lock)
		;
	cmdq_handler->status_machine->get_lock = 1;

	char_ptr = (unsigned char *)cmdq_handler->status_machine;
	for (i = 0;
	     i < (sizeof(agic_status_machine_t) - (sizeof(unsigned int) * 3));
	     i++) {
		check_sum += char_ptr[i];
	}

	if (check_sum != cmdq_handler->status_machine->check_sum) {
		cmdq_handler->status_machine->get_lock = 0;
		return -1;
	}

	status = cmdq_handler->status_machine->status;
	cmdq_handler->status_machine->get_lock = 0;

	return status;
}

char agic_status_machine_get_substatus(agic_cmdqueue_handler_t *cmdq_handler)
{
	int i = 0;
	unsigned char *char_ptr = NULL;
	unsigned int check_sum = 0;
	char substatus;

	while (cmdq_handler->status_machine->set_lock)
		;
	cmdq_handler->status_machine->get_lock = 1;

	char_ptr = (unsigned char *)cmdq_handler->status_machine;
	for (i = 0;
	     i < (sizeof(agic_status_machine_t) - (sizeof(unsigned int) * 3));
	     i++) {
		check_sum += char_ptr[i];
	}

	if (check_sum != cmdq_handler->status_machine->check_sum) {
		cmdq_handler->status_machine->get_lock = 0;
		return -1;
	}

	substatus = cmdq_handler->status_machine->substatus;
	cmdq_handler->status_machine->get_lock = 0;

	return substatus;
}

short agic_status_machine_get_cmd_id(agic_cmdqueue_handler_t *cmdq_handler)
{
	int i = 0;
	unsigned char *char_ptr = NULL;
	unsigned int check_sum = 0;
	short cmd_id = 0;

	while (cmdq_handler->status_machine->set_lock)
		;
	cmdq_handler->status_machine->get_lock = 1;

	char_ptr = (unsigned char *)cmdq_handler->status_machine;
	for (i = 0;
	     i < (sizeof(agic_status_machine_t) - (sizeof(unsigned int) * 3));
	     i++) {
		check_sum += char_ptr[i];
	}

	if (check_sum != cmdq_handler->status_machine->check_sum) {
		cmdq_handler->status_machine->get_lock = 0;
		return -1;
	}

	cmd_id = cmdq_handler->status_machine->cmd_id;
	cmdq_handler->status_machine->get_lock = 0;
	return cmd_id;
}

int agic_status_machine_get_msg(agic_cmdqueue_handler_t *cmdq_handler,
				char *append_msg)
{
	int i = 0;
	unsigned char *char_ptr = NULL;
	unsigned int check_sum = 0;

	while (cmdq_handler->status_machine->set_lock)
		;
	cmdq_handler->status_machine->get_lock = 1;

	char_ptr = (unsigned char *)cmdq_handler->status_machine;
	for (i = 0;
	     i < (sizeof(agic_status_machine_t) - (sizeof(unsigned int) * 3));
	     i++) {
		check_sum += char_ptr[i];
	}

	if (check_sum != cmdq_handler->status_machine->check_sum) {
		cmdq_handler->status_machine->get_lock = 0;
		return -1;
	}

	for (i = 0; i < AGIC_MSGLEN; i++) {
		append_msg[i] = cmdq_handler->status_machine->append_msg[i];
	}

	cmdq_handler->status_machine->get_lock = 0;

	return 0;
}
