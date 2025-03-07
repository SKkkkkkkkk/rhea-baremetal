#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "vendor.h"
#include "dw_mmc.h"

/* tag for vendor check */
#define VENDOR_TAG			0x4147a510
/* The Vendor partition contains the number of Vendor blocks */
#define VENDOR_PART_NUM		4
/* align to 64 bytes */
#define VENDOR_BTYE_ALIGN	0x3
#define VENDOR_BLOCK_SIZE	512

#define PAGE_ALGIN_SIZE		(4096uL)
#define PAGE_ALGIN_MASK		(~(PAGE_ALGIN_SIZE - 1))

/* --- Emmc define --- */
/* Starting address of the Vendor in memory. */
#define EMMC_VENDOR_PART_OFFSET		0xc00
/* The number of memory blocks used by each Vendor structure */
#define EMMC_VENDOR_PART_BLKS		2
#define EMMC_VENDOR_INFO_SIZE		(EMMC_VENDOR_PART_BLKS * VENDOR_BLOCK_SIZE)
/* The maximum number of items in each Vendor block */
#define EMMC_VENDOR_ITEM_NUM		62

struct vendor_hdr {
	uint32_t	tag;
	uint32_t	version;
	uint16_t	next_index;
	uint16_t	item_num;
	uint16_t	free_offset; /* Free space offset */
	uint16_t	free_size; /* Free space size */
};

/**
 * version2: Together with hdr->version, it is used to
 * ensure the current Vendor block content integrity.
 *   (version2 == hdr->version):Data valid;
 *   (version2 != hdr->version):Data invalid.
 */
struct vendor_info {
	struct vendor_hdr *hdr;
	struct vendor_item *item;
	uint8_t *data;
	uint32_t *hash;
	uint32_t *version2;
};

/*
 * Calculate the offset of each field for emmc.
 */
#define EMMC_VENDOR_INFO_SIZE	(EMMC_VENDOR_PART_BLKS * VENDOR_BLOCK_SIZE)
#define EMMC_VENDOR_DATA_OFFSET	(sizeof(struct vendor_hdr) + EMMC_VENDOR_ITEM_NUM * sizeof(struct vendor_item))
#define EMMC_VENDOR_HASH_OFFSET (EMMC_VENDOR_INFO_SIZE - 8)
#define EMMC_VENDOR_VERSION2_OFFSET (EMMC_VENDOR_INFO_SIZE - 4)

/* vendor info */
static struct vendor_info vendor_info;
static uint8_t vendor_buffer[EMMC_VENDOR_INFO_SIZE] __aligned(16) = {0};
static char vendor_inited = 0;

static uint32_t cal_check_sum(uint8_t *buf, uint32_t size)
{
	uint32_t i;
	uint32_t sum = 0;
	uint32_t *data = NULL;
	
	if (size % 4) {
		printf("[Vendor]:Data size is not aligned to 4 bytes.\n");
		return 0;
	}
	data = (uint32_t *) buf;
	size /= 4;

	for (i = 0; i < size; i++) {
		sum += data[i];
	}

	return sum;
}

static size_t vendor_ops(uintptr_t buffer, uint32_t offset, uint32_t n_sec, int write)
{
	size_t ret_size = 0;
	unsigned int lba = 0;

	lba = EMMC_VENDOR_PART_OFFSET + offset;

	if (write) {
	    ret_size = mmc_write_blocks(lba, buffer, n_sec);
	} else {
	    ret_size = mmc_read_blocks(lba, buffer, n_sec);
	}

	pr_dbg("[Vendor]:op=%s, size=0x%lx\n", write ? "write" : "read", ret_size);

	return ret_size;
}

/*
 * The VendorStorage partition is divided into four parts
 * (vendor 0-3) and its structure is shown in the following figure.
 * The init function is used to select the latest and valid vendor.
 *
 * -------------------------------------------------
 * |  vendor0  |  vendor1  |  vendor2  |  vendor3  |
 * -------------------------------------------------
 * Notices:
 *   1. "version" and "version2" are used to verify that the vendor
 *      is valid (equal is valid).
 *   2. the "version" value is larger, indicating that the current
 *      verndor data is new.
 */
static int vendor_storage_init(void)
{
	int ret = 0;
	int ret_size;
	uint32_t size, i;
	uint32_t max_ver = 0;
	uint32_t max_index = 0;
	uint32_t check_sum = 0;
	uint32_t info_size = 0;
	uint16_t data_offset, hash_offset, part_num;
	uint16_t version2_offset, part_size;

	if (!vendor_inited) {
    	ret = dw_mmc_init();
        if (ret) {
			printf("[Vendor]:Storage device init failed.\n");
			return -ENODEV;
		}
	}

    size = EMMC_VENDOR_INFO_SIZE;
	info_size = EMMC_VENDOR_INFO_SIZE;
    part_size = EMMC_VENDOR_PART_BLKS;
    data_offset = EMMC_VENDOR_DATA_OFFSET;
    hash_offset = EMMC_VENDOR_HASH_OFFSET;
    version2_offset = EMMC_VENDOR_VERSION2_OFFSET;
    part_num = VENDOR_PART_NUM;

	/* Pointer initialization */
	vendor_info.hdr = (struct vendor_hdr *)vendor_buffer;
	vendor_info.item = (struct vendor_item *)(vendor_buffer + sizeof(struct vendor_hdr));
	vendor_info.data = vendor_buffer + data_offset;
	vendor_info.hash = (uint32_t *)(vendor_buffer + hash_offset);
	vendor_info.version2 = (uint32_t *)(vendor_buffer + version2_offset);

	/* Find valid and up-to-date one from (vendor0 - vendor3) */
	for (i = 0; i < part_num; i++) {
		ret_size = vendor_ops((uintptr_t) vendor_info.hdr,
				      part_size * i, info_size, 0);
		if (ret_size != info_size) {
			ret = -EIO;
			goto out;
		}
		if ((vendor_info.hdr->tag == VENDOR_TAG) &&
		    (*(vendor_info.version2) == vendor_info.hdr->version)) {
			if (max_ver < vendor_info.hdr->version) {
				max_index = i;
				max_ver = vendor_info.hdr->version;
			}
		}
	}

	if (max_ver) {
		pr_dbg("[Vendor]:max_ver=%d, vendor_id=%d.\n", max_ver, max_index);
		/*
		 * Keep vendor_info the same as the largest
		 * version of vendor
		 */
		if (max_index != (part_num - 1)) {
			ret_size = vendor_ops((uintptr_t) vendor_info.hdr,
					       part_size * max_index, info_size, 0);
			if (ret_size != info_size) {
				ret = -EIO;
				goto out;
			}
			check_sum = cal_check_sum((uint8_t *) vendor_info.hdr,
								EMMC_VENDOR_INFO_SIZE - 8);
			if (*(vendor_info.hash) != check_sum) {
				printf("[Verdor]:Verification failed.\n");
				ret = -EIO;
				goto out;
			}
		}
	} else {
		pr_dbg("[Vendor]:Reset vendor info...\n");
		memset((uint8_t *)vendor_info.hdr, 0, size);
		vendor_info.hdr->version = 1;
		vendor_info.hdr->tag = VENDOR_TAG;
		/* data field length */
		vendor_info.hdr->free_size =
			((uint32_t)(size_t)vendor_info.hash
			- (uint32_t)(size_t)vendor_info.data);
		*(vendor_info.version2) = vendor_info.hdr->version;
	}
	pr_dbg("[Vendor]:ret=%d.\n", ret);
	vendor_inited = 1;
out:
	return ret;
}

/*
 * @id: item id
 * @pbuf: read data buffer;
 * @size: read bytes;
 *
 * return: bytes equal to @size is success, other fail;
 */
int vendor_storage_read(uint16_t id, void *pbuf, uint16_t size)
{
	int ret = 0;
	uint32_t i;
	uint16_t offset;
	struct vendor_item *item;

	/* init vendor storage */
	if (!vendor_inited) {
		ret = vendor_storage_init();
		if (ret < 0)
			return ret;
	}

	item = vendor_info.item;
	for (i = 0; i < vendor_info.hdr->item_num; i++) {
		if ((item + i)->id == id) {
			pr_dbg("[Vendor]:Find the matching item, id=%d\n", id);
			/* Correct the size value */
			if (size > (item + i)->size)
				size = (item + i)->size;
			offset = (item + i)->offset;
			memcpy(pbuf, (vendor_info.data + offset), size);
			return size;
		}
	}
	printf("[Vendor]:No matching item, id=%d\n", id);

	return -EINVAL;
}

/*
 * @id: item id, first 4 id is occupied:
 *	VENDOR_SN_ID
 *	VENDOR_WIFI_MAC_ID
 *	VENDOR_LAN_MAC_ID
 *	VENDOR_BLUETOOTH_ID
 * @pbuf: write data buffer;
 * @size: write bytes;
 *
 * return: bytes equal to @size is success, other fail;
 */
int vendor_storage_write(uint16_t id, void *pbuf, uint16_t size)
{
	int cnt, ret = 0;
	uint32_t i, next_index, align_size, info_size;
	struct vendor_item *item;
	uint16_t part_size, max_item_num, offset, part_num;

	/* init vendor storage */
	if (!vendor_inited) {
		ret = vendor_storage_init();
		if (ret < 0)
			return ret;
	}

	info_size = EMMC_VENDOR_INFO_SIZE;
    part_size = EMMC_VENDOR_PART_BLKS;
    max_item_num = EMMC_VENDOR_ITEM_NUM;
    part_num = VENDOR_PART_NUM;

	next_index = vendor_info.hdr->next_index;
	/* algin to 64 bytes*/
	align_size = (size + VENDOR_BTYE_ALIGN) & (~VENDOR_BTYE_ALIGN);
	if (size > align_size)
		return -EINVAL;

	item = vendor_info.item;
	/* If item already exist, update the item data */
	for (i = 0; i < vendor_info.hdr->item_num; i++) {
		if ((item + i)->id == id) {
			pr_dbg("[Vendor]:Find the matching item, id=%d\n", id);
			offset = (item + i)->offset;
			memcpy((vendor_info.data + offset), pbuf, size);
			(item + i)->size = size;
			vendor_info.hdr->version++;
			*(vendor_info.version2) = vendor_info.hdr->version;
			vendor_info.hdr->next_index++;
			if (vendor_info.hdr->next_index >= part_num)
				vendor_info.hdr->next_index = 0;
			*(vendor_info.hash) = cal_check_sum((uint8_t *) vendor_info.hdr,
									EMMC_VENDOR_INFO_SIZE - 8);
			cnt = vendor_ops((uintptr_t) vendor_info.hdr,
						part_size * next_index, info_size, 1);
			return (cnt == info_size) ? size : -EIO;
		}
	}

	/*
	 * If item does not exist, and free size is enough,
	 * creat a new one
	 */
	if ((vendor_info.hdr->item_num < max_item_num) &&
	    (vendor_info.hdr->free_size >= align_size)) {
		pr_dbg("[Vendor]:Create new Item, id=%d\n", id);
		item = vendor_info.item + vendor_info.hdr->item_num;
		item->id = id;
		item->offset = vendor_info.hdr->free_offset;
		item->size = size;

		vendor_info.hdr->free_offset += align_size;
		vendor_info.hdr->free_size -= align_size;
		memcpy((vendor_info.data + item->offset), pbuf, size);
		vendor_info.hdr->item_num++;
		vendor_info.hdr->version++;
		vendor_info.hdr->next_index++;
		*(vendor_info.version2) = vendor_info.hdr->version;
		if (vendor_info.hdr->next_index >= part_num)
			vendor_info.hdr->next_index = 0;

		*(vendor_info.hash) = cal_check_sum((uint8_t *) vendor_info.hdr,
									EMMC_VENDOR_INFO_SIZE - 8);
		cnt = vendor_ops((uintptr_t) vendor_info.hdr, part_size * next_index, info_size, 1);
		return (cnt == info_size) ? size : -EIO;
	}
	printf("[Vendor]:Vendor has no space left!\n");

	return -ENOMEM;
}