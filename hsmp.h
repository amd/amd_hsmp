/* SPDX-License-Identifier: GPL-2.0 */
/*
 * AMD HSMP Platform Driver
 * Copyright (c) 2024, AMD.
 * All Rights Reserved.
 *
 * Header file for HSMP driver
 */

#ifndef HSMP_H
#define HSMP_H

#include <linux/compiler_types.h>
#include <linux/device.h>
#include <linux/hwmon.h>
#include <linux/kconfig.h>
#include <linux/miscdevice.h>
#include <linux/pci.h>
#include <linux/semaphore.h>
#include <linux/sysfs.h>

/*
 * Helper macros to handle API changes across kernel versions:
 * - Kernels >= 6.14.0: bin_attribute and attribute_group become const
 * - Kernels 6.14.0 to 6.17.x: use .read_new and .bin_attrs_new
 * - Kernels < 6.14.0 and >= 6.18.0: use .read and .bin_attrs
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 14, 0)
#define HSMP_CONST		const
#else
#define HSMP_CONST
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 14, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(6, 18, 0)
#define HSMP_BIN_READ		.read_new
#define HSMP_BIN_ATTRS_FIELD	.bin_attrs_new
#else
#define HSMP_BIN_READ		.read
#define HSMP_BIN_ATTRS_FIELD	.bin_attrs
#endif

#define HSMP_METRICS_TABLE_NAME	"metrics_bin"

#define HSMP_ATTR_GRP_NAME_SIZE	10

#define MAX_AMD_SOCKETS 8

#define HSMP_CDEV_NAME		"hsmp_cdev"
#define HSMP_DEVNODE_NAME	"hsmp"
#define ACPI_HSMP_DEVICE_HID    "AMDI0097"

#define DRIVER_VERSION		"3.0"

struct hsmp_mbaddr_info {
	u32 base_addr;
	u32 msg_id_off;
	u32 msg_resp_off;
	u32 msg_arg_off;
	u32 size;
};

struct hsmp_socket {
	struct bin_attribute hsmp_attr;
	struct hsmp_mbaddr_info mbinfo;
	void __iomem *metric_tbl_addr;
	void __iomem *virt_base_addr;
	struct semaphore hsmp_sem;
	char name[HSMP_ATTR_GRP_NAME_SIZE];
	struct pci_dev *root;
	struct device *dev;
	u16 sock_ind;
	int (*amd_hsmp_rdwr)(struct hsmp_socket *sock, u32 off, u32 *val, bool rw);
};

struct hsmp_plat_device {
	struct miscdevice mdev;
	struct hsmp_socket *sock;
	u32 proto_ver;
	u16 num_sockets;
	bool is_probed;
};

int hsmp_cache_proto_ver(u16 sock_ind);
int hsmp_test(u16 sock_ind, u32 value);
long hsmp_ioctl(struct file *fp, unsigned int cmd, unsigned long arg);
void hsmp_misc_deregister(void);
int hsmp_misc_register(struct device *dev);
int hsmp_get_tbl_dram_base(u16 sock_ind);
ssize_t hsmp_metric_tbl_read(struct hsmp_socket *sock, char *buf, size_t size);
struct hsmp_plat_device *get_hsmp_pdev(void);
#if IS_ENABLED(CONFIG_HWMON)
int hsmp_create_sensor(struct device *dev, u16 sock_ind);
#else
static inline int hsmp_create_sensor(struct device *dev, u16 sock_ind) { return 0; }
#endif
int hsmp_msg_get_nargs(u16 sock_ind, u32 msg_id, u32 *data, u8 num_args);
#endif /* HSMP_H */
