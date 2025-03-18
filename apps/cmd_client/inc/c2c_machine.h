#ifndef __C2C_MACHINE__H
#define __C2C_MACHINE__H

#define GPU_DIE_NUMBER				(1U)
#define GPU_TILE_NUMBER				(16U)

struct tile_stats {
	int tile;
	int fd;
	int ctl_index;
	int status; /* current status */
};

struct gpu_card {
	int die;
	struct tile_stats tile[GPU_TILE_NUMBER];
};

void c2c_gpu_machine_init(void);
void c2c_gpu_response_cmd( void *arg , void *result);

#endif
