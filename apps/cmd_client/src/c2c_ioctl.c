#include "c2c_ioctl.h"
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
// #include <sys/ioctl.h>

#define A510_C2C_NODE_NAME					"agic_c2c"
#define NO_CHRDEV_NODE						(0U)

int c2c_ioctl_fd_init( uint8_t tile_id, uint8_t ctl_id )
{
	int c2c_fd = -1;
	char chdev_name[64];

	memset(chdev_name, 0, sizeof(chdev_name));

	sprintf(chdev_name, "/dev/%s_%d_%d", A510_C2C_NODE_NAME, tile_id, ctl_id);

	printf("open node: %s \r\n", chdev_name);
#if NO_CHRDEV_NODE
	c2c_fd = open(chdev_name, O_RDWR);
	if( c2c_fd <= 0 ) {
		printf("open %d fail \r\n", chdev_name);
		return -1;
	}
#endif

	return c2c_fd;
}

int c2c_ioctl_cfg(int fd, struct c2c_tile_cfg_io *tile_cfg)
{
	int ret = 0;
#if NO_CHRDEV_NODE
	ret = ioctl( fd, A510_C2C_TILE_INIT, tile_cfg);
	if(ret != 0) {
		printf("c2c init fail \r\n");
	}
#endif
	return ret;
}

int c2c_ioctl_link_ctl(int fd, struct c2c_tile_link_ctr *link)
{
	int ret = 0;
#if NO_CHRDEV_NODE
	ret = ioctl( fd, A510_C2C_TILE_LINK_CTL, link);
	if(ret != 0) {
		printf("c2c init fail \r\n");
	}
#endif
	return ret;
}

int c2c_ioctl_link_step( int fd, struct c2c_tile_link_setup *setup)
{
	int ret = 0;

#if NO_CHRDEV_NODE
	ret = ioctl(fd, A510_C2C_TILE_LINK_SETUP, setup);
	if(ret != 0) {
		printf("c2c init fail \r\n");
	}
#endif

	return ret;
}

int c2c_ioctl_get_speed(int fd, struct c2c_tile_speed *speed)
{
	int ret = 0;
#if NO_CHRDEV_NODE
	ret = ioctl(fd, A510_C2C_TILE_LINK_CTL, speed);
	if(ret != 0) {
		printf("c2c init fail \r\n");
	}
#endif
	return ret;
}

int c2c_ioctl_get_link_status(int fd, struct c2c_tile_link_state *status)
{
	int ret = 0;
#if NO_CHRDEV_NODE
	ret = ioctl(fd, A510_C2C_TILE_GET_STATUS, status);
	if(ret != 0) {
		printf("c2c init fail \r\n");
	}
#endif
	return ret;
}

int c2c_ioctl_get_buffer(int fd, struct c2c_recv_buffer_io *buffer)
{
	int ret = 0;
#if NO_CHRDEV_NODE
	ret = ioctl(fd, A510_C2C_TILE_GET_BUFFER, buffer);
	if(ret != 0) {
		printf("c2c init fail \r\n");
	}
#endif
	return ret;
}

int c2c_delinit(int fd)
{
	int ret = 0;
#if NO_CHRDEV_NODE
	ret = ioctl(fd, A510_C2C_TILE_GET_BUFFER, buffer);
	if(ret != 0) {
		printf("c2c init fail \r\n");
	}
#endif
	return ret;
}
