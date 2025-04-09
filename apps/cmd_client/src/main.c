#include <stdio.h>
#include "delay.h"

#include "agic_cmdqueue.h"
#include "agic_cmdqueue_extend.h"
#include "c2c_machine.h"
#include "agic_sprintf.h"

#define KERNEL_IMG_LOAD_ADDR 0x45000000
#define KERNEL_DTB_LOAD_ADDR 0x46f00000
#define RAMDISK_LOAD_ADDR 0x43000000
#define FIRMWARE_BUFFER_ADDR 0x48000000
#define CMDQUEUE_BUFFER_ADDR 0x47000000

typedef struct AGICUploadDescription {
	unsigned char *firmware_buffer;
	unsigned int firmware_size;
	unsigned int crc;
	char name[10];
} AGICUploadDescription;

typedef struct PowerInfo {
	unsigned int usage; // current power, unit: W
	unsigned int cap; // peak power, unit: W
} PowerInfo;

static char heart_msg[] = "ok";
static unsigned int temperature = 99;
static char version[] = "1.07";
static unsigned int fanSpeed = 70;
static unsigned int freq = 0x40000000;
PowerInfo powerInfo = { 120, 150 };
const char *command_trigger_irq = "devmem 0xfe280038 32 0x80000000";

static agic_cmdqueue_handler_t cmdq_handler;
static char *firmware_buffer = NULL;

#if IS_SUPPORT_DOWNLOAD
static int download_firmware_from_rc(char *msg)
{
	int ret = 0;
	unsigned int size = 0;
	unsigned int locate = 0;
	AGICUploadDescription upload_desc;
	int fd;
	char file_name[50] = "/tmp/";

	if (access(file_name, F_OK) == -1) {
		printf("%s does not exist\n", file_name);
		if (mkdir(file_name, 0777) == 0) {
			printf("%s creat success\n", file_name);
		} else {
			printf("%s creat fail\n", file_name);
			return -1;
		}
	} else {
		printf("%s exist\n", file_name);
	}

	memcpy(&upload_desc, msg, sizeof(AGICUploadDescription));

	strcat(file_name, upload_desc.name);
	fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC);
	if (fd == -1) {
		printf("open %s fail\n", file_name);
		return -1;
	}
	if (write(fd, firmware_buffer, upload_desc.firmware_size) == -1) {
		printf("write %s fail\n", file_name);
		return -1;
	}

	close(fd);
	return 0;
}
#endif

static inline void writel(uint32_t value, void *address)
{
	uintptr_t addr = (uintptr_t)address;

	*((volatile uint32_t *)(addr)) = value;
}

static inline uint32_t readl(void *address)
{
	uintptr_t addr = (uintptr_t)address;

	return *((volatile uint32_t *)(addr));
}

int main(int argc, char **argv)
{
	printf("AGIC A510 Card, Hello World ...\n");

	struct agic_cmd rcmd;
	int ret = 0;
	char result[AGIC_MSGLEN];
	void *mapped_mem;
	unsigned int val = 0;

	// c2c_gpu_machine_init();


	mapped_mem = (char *)CMDQUEUE_BUFFER_ADDR;

	// firmware_buffer = mapped_mem + 0x1000000;
	agic_cmdqueue_create(&cmdq_handler, mapped_mem);

	agic_status_machine_set(&cmdq_handler, AGIC_STATUS_L2,
				AGIC_SUBSTATUS_IDLE, AGIC_CMDNULL, "");
	printf("AGIC: Change to L2 ...\n");
	agic_status_machine_set(&cmdq_handler, AGIC_STATUS_L2,
		AGIC_SUBSTATUS_DONE, AGIC_CMDNULL, "");

	while (1) {
		udelay(10000);
		if (agic_status_machine_get_substatus(&cmdq_handler) !=
		    AGIC_SUBSTATUS_END) {
			printf("AGIC: Last CMD is not Done ...\n");
			continue;
		} else {
			ret = agic_cmdqueue_cmd_get(&cmdq_handler, &rcmd);
			if (ret != 0)
				continue;
		}
		switch (rcmd.cmd_id) {
		case AGIC_HEART:
			printf("AGIC: Get AGIC_HEART ...\n");
			agic_status_machine_set(&cmdq_handler, AGIC_STATUS_L2,
						AGIC_SUBSTATUS_DOING,
						AGIC_HEART, "");
			printf("AGIC: Change to L2 Doing AGIC_HEART ...\n");
			agic_status_machine_set(&cmdq_handler, AGIC_STATUS_L2,
						AGIC_SUBSTATUS_DONE, AGIC_HEART,
						heart_msg);
			printf("AGIC: Change to L2 Done AGIC_HEART ...\n");
			break;
		case AGIC_CMD4:
			printf("AGIC: Get CMD4 ...\n");
			agic_status_machine_set(&cmdq_handler, AGIC_STATUS_L2,
						AGIC_SUBSTATUS_DOING, AGIC_CMD4,
						"");
			printf("AGIC: Change to L2 Doing CMD4 ...\n");
			agic_sprintf(result, "%u", temperature);
			agic_status_machine_set(&cmdq_handler, AGIC_STATUS_L2,
						AGIC_SUBSTATUS_DONE, AGIC_CMD4,
						result);
			printf("AGIC: Change to L2 Done CMD4 ...\n");
			break;
		case AGIC_CMD5:
			printf("AGIC: Get CMD5 ...\n");
			agic_status_machine_set(&cmdq_handler, AGIC_STATUS_L2,
						AGIC_SUBSTATUS_DOING, AGIC_CMD5,
						"");
			printf("AGIC: Change to L2 Doing CMD5 ...\n");
			agic_status_machine_set(&cmdq_handler, AGIC_STATUS_L2,
						AGIC_SUBSTATUS_DONE, AGIC_CMD5,
						version);
			printf("AGIC: Change to L2 Done CMD5 ...\n");
			break;
		case AGIC_CMD6:
			printf("AGIC: Get CMD6 ...\n");
			agic_status_machine_set(&cmdq_handler, AGIC_STATUS_L2,
						AGIC_SUBSTATUS_DOING, AGIC_CMD6,
						"");
			printf("AGIC: Change to L2 Doing CMD6 ...\n");
			agic_sprintf(result, "%u", fanSpeed);
			agic_status_machine_set(&cmdq_handler, AGIC_STATUS_L2,
						AGIC_SUBSTATUS_DONE, AGIC_CMD6,
						result);
			printf("AGIC: Change to L2 Done CMD6 ...\n");
			break;
		case AGIC_CMD7:
			printf("AGIC: Get CMD7 ...\n");
			agic_status_machine_set(&cmdq_handler, AGIC_STATUS_L2,
						AGIC_SUBSTATUS_DOING, AGIC_CMD7,
						"");
			printf("AGIC: Change to L2 Doing CMD7 ...\n");
			agic_sprintf(result, "%u", freq);
			agic_status_machine_set(&cmdq_handler, AGIC_STATUS_L2,
						AGIC_SUBSTATUS_DONE, AGIC_CMD7,
						result);
			printf("AGIC: Change to L2 Done CMD7 ...\n");
			break;
		case AGIC_CMD8:
			printf("AGIC: Get CMD8 ...\n");
			agic_status_machine_set(&cmdq_handler, AGIC_STATUS_L2,
						AGIC_SUBSTATUS_DOING, AGIC_CMD8,
						"");
			printf("AGIC: Change to L2 Doing CMD8 ...\n");
			agic_status_machine_set(&cmdq_handler, AGIC_STATUS_L2,
						AGIC_SUBSTATUS_DONE, AGIC_CMD8,
						(char *)&powerInfo);
			printf("AGIC: Change to L2 Done CMD8 ...\n");
			break;
#if IS_SUPPORT_DOWNLOAD
		case AGIC_CMD9:
			printf("AGIC: Get CMD9 ...\n");
			agic_status_machine_set(&cmdq_handler, AGIC_STATUS_L2,
						AGIC_SUBSTATUS_DOING, AGIC_CMD9,
						"");
			printf("AGIC: Change to L2 Doing CMD9 ...\n");
			ret = download_firmware_from_rc(rcmd.append_msg);
			if (ret == 0)
				agic_status_machine_set(&cmdq_handler,
							AGIC_STATUS_L2,
							AGIC_SUBSTATUS_DONE,
							AGIC_CMD9, "success");
			else
				agic_status_machine_set(&cmdq_handler,
							AGIC_STATUS_L2,
							AGIC_SUBSTATUS_DONE,
							AGIC_CMD9, "fail");
			printf("AGIC: Change to L2 Done CMD9 ...\n");
			break;
		case AGIC_CMD10:
			printf("AGIC: Get CMD10 ...\n");
			agic_status_machine_set(&cmdq_handler, AGIC_STATUS_L2,
						AGIC_SUBSTATUS_DOING,
						AGIC_CMD10, "");
			c2c_gpu_response_cmd(firmware_buffer, result);
			agic_status_machine_set(&cmdq_handler, AGIC_STATUS_L2,
						AGIC_SUBSTATUS_DONE, AGIC_CMD10,
						result);
			printf("AGIC: Change to L2 Done CMD10 ...\n");
			break;
#endif
		case AGIC_CMD11:
			break;
		default:
			printf("AGIC: Get unknown CMD:%d ...\n", rcmd.cmd_id);
			break;
		}
	}

	return 0;
}
