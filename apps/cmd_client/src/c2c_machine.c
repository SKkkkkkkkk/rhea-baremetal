#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include "c2c_machine.h"
#include "a5xx_alink.h"
#include "c2c_ioctl.h"
#include "c2c_ioctl.h"

struct gpu_card gpu_machine[GPU_DIE_NUMBER];

void c2c_gpu_machine_init(void)
{
	uint32_t cnti;

	for ( cnti = 0; cnti < GPU_DIE_NUMBER; cnti++) {
		gpu_machine[cnti].die = cnti;
		gpu_machine[cnti].tile[0].tile = 0x02;
		gpu_machine[cnti].tile[0].status = C2C_PREPARE;
		gpu_machine[cnti].tile[0].ctl_index = 0;
		gpu_machine[cnti].tile[0].fd = c2c_ioctl_fd_init( 0x02, gpu_machine[cnti].tile[0].ctl_index);

		gpu_machine[cnti].die = cnti;
		gpu_machine[cnti].tile[1].tile = 0x02;
		gpu_machine[cnti].tile[1].status = C2C_PREPARE;
		gpu_machine[cnti].tile[1].ctl_index = 1;
		gpu_machine[cnti].tile[1].fd = c2c_ioctl_fd_init( 0x02, gpu_machine[cnti].tile[1].ctl_index);

		gpu_machine[cnti].die = cnti;
		gpu_machine[cnti].tile[2].tile = 0x03;
		gpu_machine[cnti].tile[2].status = C2C_PREPARE;
		gpu_machine[cnti].tile[2].ctl_index = 0;
		gpu_machine[cnti].tile[2].fd = c2c_ioctl_fd_init( 0x03, gpu_machine[cnti].tile[2].ctl_index);

		gpu_machine[cnti].die = cnti;
		gpu_machine[cnti].tile[3].tile = 0x03;
		gpu_machine[cnti].tile[3].status = C2C_PREPARE;
		gpu_machine[cnti].tile[3].ctl_index = 1;
		gpu_machine[cnti].tile[3].fd = c2c_ioctl_fd_init( 0x03, gpu_machine[cnti].tile[3].ctl_index);

		gpu_machine[cnti].die = cnti;
		gpu_machine[cnti].tile[4].tile = 72;
		gpu_machine[cnti].tile[4].status = C2C_PREPARE;
		gpu_machine[cnti].tile[4].ctl_index = 0;
		gpu_machine[cnti].tile[4].fd = c2c_ioctl_fd_init( 72, gpu_machine[cnti].tile[4].ctl_index);

		gpu_machine[cnti].die = cnti;
		gpu_machine[cnti].tile[5].tile = 72;
		gpu_machine[cnti].tile[5].status = C2C_PREPARE;
		gpu_machine[cnti].tile[5].ctl_index = 1;
		gpu_machine[cnti].tile[5].fd = c2c_ioctl_fd_init( 72, gpu_machine[cnti].tile[5].ctl_index);

		gpu_machine[cnti].die = cnti;
		gpu_machine[cnti].tile[6].tile = 73;
		gpu_machine[cnti].tile[6].status = C2C_PREPARE;
		gpu_machine[cnti].tile[6].ctl_index = 0;
		gpu_machine[cnti].tile[6].fd = c2c_ioctl_fd_init( 73, gpu_machine[cnti].tile[6].ctl_index);

		gpu_machine[cnti].die = cnti;
		gpu_machine[cnti].tile[7].tile = 73;
		gpu_machine[cnti].tile[7].status = C2C_PREPARE;
		gpu_machine[cnti].tile[7].ctl_index = 1;
		gpu_machine[cnti].tile[7].fd = c2c_ioctl_fd_init( 73, gpu_machine[cnti].tile[7].ctl_index);
	}
}

static struct tile_stats *c2c_find_tile( struct gpu_card *card, uint8_t die, uint8_t tile, uint8_t ctl_id)
{
	uint8_t cnti, cntj;

	for ( cnti = 0; cnti < GPU_DIE_NUMBER; cnti++) {
			for ( cntj = 0; cntj < GPU_TILE_NUMBER; cntj++) {
				if( card[cnti].tile[cntj].tile == tile && card[cnti].tile[cntj].ctl_index == ctl_id) {
					if( card[cnti].tile[cntj].fd != -1 )
						return  &card[cnti].tile[cntj];
					else
					 	return  NULL;
				}
			}
	}

	return NULL;
}

static void c2c_config( struct tile_stats *tile_machine, struct c2c_comm_die *tile_cfg,  struct c2c_receive *r_state)
{
	int ret;
	struct c2c_tile_cfg_io tile_set;
	struct c2c_tile_cfg *pci_cfg = &tile_cfg->cfg_msg.msg.c2c_pcie_cfg;
	if( tile_machine->status != C2C_PREPARE )
	{
		r_state->state = C2C_CFG_INITED;
		printf("current die %d tile %d initialized.\r\n", tile_cfg->cfg_msg.msg.c2c_pcie_cfg.die, tile_cfg->cfg_msg.msg.c2c_pcie_cfg.local_tile_id);
		return;
	}

	/* setup tile */
	tile_set.local_tile_id = pci_cfg->local_tile_id;
	tile_set.bif_en = pci_cfg->bif_en;
	tile_set.lane_min = pci_cfg->lane_min;
	tile_set.lane_max = pci_cfg->lane_max;
	tile_set.mode = pci_cfg->mode;
	tile_set.rate = pci_cfg->rate;
	tile_set.bdf = pci_cfg->bdf;

	ret = c2c_ioctl_cfg(tile_machine->fd, &tile_set);
	if( ret < 0 ) {
		tile_machine->status = C2C_PREPARE;
		r_state->state = C2C_FAIL;
	}
	else {
		tile_machine->status = C2C_CONFIG_DONE;
		r_state->state = C2C_SUCCEED;
	}
	/* setup done */
	printf("tile %d ctl %d\r\n", tile_cfg->cfg_msg.msg.c2c_pcie_cfg.local_tile_id, pci_cfg->ctr_index);
}

void *c2c_link_thread(void *arg)
{
	struct tile_stats *tile_machine = arg;
	struct c2c_tile_link_ctr link;
	int *ret = malloc(4);

	link.ctl_index = tile_machine->ctl_index;
	link.tile_id = tile_machine->tile;
	link.onoff = 1;

	*ret = c2c_ioctl_link_ctl( tile_machine->fd, &link);

	return ret;
}

static void c2c_link_enbale(struct tile_stats *tile_machine, struct c2c_link_ctr *tile_cfg,  struct c2c_receive *r_state)
{
	struct c2c_tile_link_ctr link;
	int ret;

	if( tile_machine->status != C2C_CONFIG_DONE )
	{
		r_state->state = C2C_STATE_EXCEPTION;
		printf("current die %d tile %d fail.\r\n", tile_cfg->die, tile_cfg->tile_id);
		return;
	}

	link.tile_id = tile_cfg->tile_id;
	link.ctl_index = tile_cfg->ctr_index;
	link.onoff = tile_cfg->onoff;

	/* enable link ctl */
	ret = c2c_ioctl_link_ctl( tile_machine->fd, &link);
	if(ret != 0) {
		r_state->state = C2C_FAIL;
		printf("c2c A510_C2C_TILE_LINK_CTL fail \n");
		goto err;
	}

		// ret = c2c_ioctl_link_step( tile_machine->fd, &setup);
		// if(ret != 0) {
		// 	r_state->state = C2C_FAIL;
		// 	printf("c2c A510_C2C_TILE_LINK_CTL fail \n");
		// 	goto err;

		// }

	tile_machine->status = C2C_LINK_ON;
	r_state->state = C2C_SUCCEED;
err:
	return;
}

static void c2c_link_setup( struct tile_stats *tile_machine,  struct c2c_link_ctr *tile_cfg, struct c2c_receive *r_state )
{
	struct c2c_tile_link_setup setup;
	int ret;
	setup.tile_id = tile_cfg->tile_id;
	setup.ctl_index = tile_cfg->ctr_index;
	setup.msi_capable = 0x1;

	if ( tile_machine->status != C2C_LINK_ON ) {
		printf("link init fail \r\n");
		r_state->state = C2C_FAIL;
		return;
	}

	ret = c2c_ioctl_link_step( tile_machine->fd, &setup);
	if(ret != 0) {
		r_state->state = C2C_FAIL;
		printf("c2c A510_C2C_TILE_LINK_CTL fail \n");
		goto err;
	}

	tile_machine->status = C2C_LINK_WORKING;
	r_state->state = C2C_SUCCEED;
err:
	return;
}

static void c2c_link_check_state(struct tile_stats *tile_machine, struct c2c_recv_send_state *tile_cfg, struct c2c_receive *r_state)
{
	int check_status = 0;
	struct c2c_tile_link_state status;
	if( tile_machine->status != C2C_LINK_ON &&  tile_machine->status != C2C_LINK_WORKING )
	{
		r_state->state = C2C_STATE_EXCEPTION;
		printf("current die %d tile %d fail.\r\n", tile_cfg->die, tile_cfg->tile_id);
		return;
	}

	/* check link status */
	check_status = c2c_ioctl_get_link_status(tile_machine->fd, &status);
	printf("die %d tile %d \r\n", tile_cfg->die, tile_cfg->tile_id);
	if( check_status < 0 ) {
		r_state->state = C2C_FAIL;
	}
	else {
		r_state->state = C2C_SUCCEED;
		r_state->rcv_msg.link_state.link_state = status.link_status;
	}
}

static void c2c_link_buffer(struct tile_stats *tile_machine, struct c2c_buffer_info *tile_cfg, struct c2c_receive *r_state)
{
	int ret;
	struct c2c_recv_buffer_io buffer;
	if( tile_machine->status != C2C_LINK_ON &&  tile_machine->status != C2C_LINK_WORKING )
	{
		r_state->state = C2C_FAIL;
		printf("current die %d tile %d fail.\r\n", tile_cfg->die, tile_cfg->tile_id);
		return;
	}

	ret =  c2c_ioctl_get_buffer(tile_machine->fd, &buffer);
	if ( ret < 0 ) {
		r_state->state = C2C_FAIL;
	}
	else {
		r_state->state = C2C_SUCCEED;
	}
	/* get buffer */
	printf("die %d tile %d \r\n", tile_cfg->die, tile_cfg->tile_id);
	r_state->rcv_msg.buffer.buffer_addr = buffer.buffer_addr;
	r_state->rcv_msg.buffer.c2c_buffer_size = buffer.c2c_buffer_size;

}

static void c2c_link_speed(struct tile_stats *tile_machine, struct c2c_link_speed *tile_cfg, struct c2c_receive *r_state)
{
	int ret;
	struct c2c_tile_speed speed;
	if( tile_machine->status != C2C_LINK_ON &&  tile_machine->status != C2C_LINK_WORKING )
	{
		r_state->state = C2C_FAIL;
		printf("current die %d tile %d fail.\r\n", tile_cfg->die, tile_cfg->tile_id);
		return;
	}

	/* get speed */
	ret = c2c_ioctl_get_speed(tile_machine->fd, &speed);
	if ( ret < 0 ) {
		r_state->state = C2C_FAIL;
	}
	else {
		r_state->state = C2C_SUCCEED;
	}

	printf("die %d tile %d \r\n", tile_cfg->die, tile_cfg->state);
	r_state->rcv_msg.rate.rate =  speed.rate;
	r_state->rcv_msg.rate.lane_min = speed.lane_min;
	r_state->rcv_msg.rate.lane_max = speed.lane_max;
	r_state->rcv_msg.rate.lane_mode = speed.tile_mode;

}

static void c2c_setup_state(struct tile_stats *tile_machine, struct c2c_recv_send_state *tile_cfg, struct c2c_receive *r_state)
{
	int ret;
	struct c2c_tile_link_ctr link;
	switch(tile_cfg->state ) {
		case C2C_PREPARE:
			tile_machine->status = C2C_PREPARE;
			/* function call */
			ret = c2c_delinit(tile_machine->fd);
			if( ret < 0 ) {
				r_state->state = C2C_FAIL;
			}
			else {
				r_state->state = C2C_SUCCEED;
			}
			break;
		case C2C_LINK_DOWN:
			
			/* function call */
			link.onoff = CLINK_LINK_OFF;
			ret = c2c_ioctl_link_ctl(tile_machine->fd, &link);
			if( ret < 0 ) {
				r_state->state = C2C_FAIL;
			}
			else {
				r_state->state = C2C_SUCCEED;
				tile_machine->status = C2C_LINK_DOWN;
			}
			break;
		case C2C_LINK_ON:
			
			/* function call */
			link.onoff = CLINK_LINK_ON;
			ret = c2c_ioctl_link_ctl(tile_machine->fd, &link);
			if( ret < 0 ) {
				r_state->state = C2C_FAIL;
			}
			else {
				tile_machine->status = C2C_LINK_ON;
				r_state->state = C2C_SUCCEED;
			}
			break;
		default:
			r_state->state = C2C_FAIL;
			break;
	}
	printf("die %d tile %d \r\n", tile_cfg->die, tile_cfg->tile_id);
}

void c2c_gpu_response_cmd( void *arg , void *result)
{
	uint8_t cmd = 0;
	struct tile_stats *ptile;
	struct c2c_comm_die *c2c_comm;
	struct c2c_receive *pc2c_result = (struct c2c_receive *)result;
	uint8_t *pdata = arg;

	c2c_comm = malloc(sizeof(struct c2c_comm_die));

	memcpy( c2c_comm, arg, sizeof(struct c2c_comm_die));

	pc2c_result->die = c2c_comm->cfg_msg.msg.c2c_pcie_cfg.die;
	pc2c_result->tile = c2c_comm->cfg_msg.msg.c2c_pcie_cfg.local_tile_id;
	pc2c_result->cmd = c2c_comm->cfg_msg.c2c_cmd;

	printf("pc2c_result->tile 0x%x \r\n", pc2c_result->tile);
	printf("pc2c_result->die 0x%x \r\n", pc2c_result->die);
	printf("pc2c_result->cmd 0x%x \r\n", pc2c_result->cmd);

	ptile = c2c_find_tile(gpu_machine, c2c_comm->cfg_msg.msg.c2c_pcie_cfg.die, c2c_comm->cfg_msg.msg.c2c_pcie_cfg.local_tile_id, c2c_comm->cfg_msg.msg.c2c_pcie_cfg.ctr_index);
	if( ptile == NULL ) {
		printf("not font tile %d:%d\r\n", c2c_comm->cfg_msg.msg.c2c_pcie_cfg.die, c2c_comm->cfg_msg.msg.c2c_pcie_cfg.local_tile_id);
		pc2c_result->state = C2C_FAIL;
		return;
	}
	cmd = c2c_comm->cfg_msg.c2c_cmd;

	switch (cmd) {
		case C2C_CONFIG:
			c2c_config(ptile, c2c_comm, pc2c_result);
			break;
		case C2C_LINK_ENABLE:
			c2c_link_enbale(ptile,  &c2c_comm->cfg_msg.msg.ctr_link, pc2c_result);
			break;
		case C2C_LINK_CHECK:
			c2c_link_check_state(ptile, &c2c_comm->cfg_msg.msg.state, pc2c_result);
			break;

		case C2C_RELINK:
			/* TODO */
			break;

		case C2C_BUFFER:
			c2c_link_buffer( ptile, &c2c_comm->cfg_msg.msg.c2c_buffer, pc2c_result);
			break;

		case C2C_SPEED:
			c2c_link_speed(ptile, &c2c_comm->cfg_msg.msg.c2c_speed, pc2c_result);
			break;

		case C2C_RESPONSE:
			pc2c_result->state = C2C_SUCCEED;
			pc2c_result->rcv_msg.link_state.link_state = ptile->status;
			break;

		case C2C_SETUP_STATE:
			c2c_setup_state(ptile, &c2c_comm->cfg_msg.msg.state, pc2c_result);
			break;
		case C2C_TEST_QUEUE_DELAY:
			pc2c_result->state = C2C_SUCCEED;
			break;
		case C2C_LINK_SETUP:
			c2c_link_setup( ptile, &c2c_comm->cfg_msg.msg.ctr_link, pc2c_result);
			break;
	}

	free(c2c_comm);
}
