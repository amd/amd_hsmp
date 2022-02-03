// SPDX-License-Identifier: GPL-2.0
/*
 * AMD HSMP Platform Driver
 * Copyright (c) 2022, AMD.
 * All Rights Reserved.
 *
 * This file provides a device implementation for HSMP interface
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <asm/amd_nb.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include "amd_hsmp.h"	/* this will come from linux kernel as UAPI header */

#define DRIVER_NAME		"amd_hsmp"
#define DRIVER_VERSION		"1.0"

/* HSMP Status / Error codes */
#define HSMP_STATUS_NOT_READY	0x00
#define HSMP_STATUS_OK		0x01
#define HSMP_ERR_INVALID_MSG	0xFE
#define HSMP_ERR_INVALID_INPUT	0xFF

/* Timeout in millsec */
#define HSMP_MSG_TIMEOUT	100
#define HSMP_SHORT_SLEEP	1

#define HSMP_WR			true
#define HSMP_RD			false

/*
 * To access specific HSMP mailbox register, s/w writes the SMN address of HSMP mailbox
 * register into the SMN_INDEX register, and reads/writes the SMN_DATA reg.
 * Below are required SMN address for HSMP Mailbox register offsets in SMU address space
 */
#define SMN_HSMP_MSG_ID		0x3B10534
#define SMN_HSMP_MSG_RESP	0x3B10980
#define SMN_HSMP_MSG_DATA	0x3B109E0

#define HSMP_INDEX_REG		0xc4
#define HSMP_DATA_REG		0xc8

static struct semaphore *hsmp_sem;

static struct miscdevice hsmp_device;

/* List of "Configure/SET" msgs */
static u32 hsmp_set_msgs[] = {
	HSMP_SET_SOCKET_POWER_LIMIT,
	HSMP_SET_BOOST_LIMIT,
	HSMP_SET_BOOST_LIMIT_SOCKET,
	HSMP_SET_XGMI_LINK_WIDTH,
	HSMP_SET_DF_PSTATE,
	HSMP_AUTO_DF_PSTATE,
	HSMP_SET_NBIO_DPM_LEVEL
};

/* List of "Monitor/GET" msgs */
static u32 hsmp_get_msgs[] = {
	HSMP_TEST,
	HSMP_GET_SMU_VER,
	HSMP_GET_PROTO_VER,
	HSMP_GET_SOCKET_POWER,
	HSMP_GET_SOCKET_POWER_LIMIT,
	HSMP_GET_SOCKET_POWER_LIMIT_MAX,
	HSMP_GET_BOOST_LIMIT,
	HSMP_GET_PROC_HOT,
	HSMP_GET_FCLK_MCLK,
	HSMP_GET_CCLK_THROTTLE_LIMIT,
	HSMP_GET_C0_PERCENT,
	HSMP_GET_DDR_BANDWIDTH,
	HSMP_GET_TEMP_MONITOR
};

static int amd_hsmp_rdwr(struct pci_dev *root, u32 address,
			 u32 *value, bool write)
{
	int ret;

	ret = pci_write_config_dword(root, HSMP_INDEX_REG, address);
	if (ret)
		return ret;

	ret = (write ? pci_write_config_dword(root, HSMP_DATA_REG, *value)
		     : pci_read_config_dword(root, HSMP_DATA_REG, value));

	return ret;
}

/*
 * Send a message to the HSMP port via PCI-e config space registers.
 *
 * The caller is expected to zero out any unused arguments.
 * If a response is expected, the number of response words should be greater than 0.
 *
 * Returns 0 for success and populates the requested number of arguments.
 * Returns a negative error code for failure.
 */
static int __hsmp_send_message(struct pci_dev *root, struct hsmp_message *msg)
{
	unsigned long timeout, short_sleep;
	u32 mbox_status;
	u32 arg_num;
	int ret;

	/* Clear the status register */
	mbox_status = HSMP_STATUS_NOT_READY;
	ret = amd_hsmp_rdwr(root, SMN_HSMP_MSG_RESP, &mbox_status, HSMP_WR);
	if (ret) {
		pr_err("Error %d clearing mailbox status register\n", ret);
		return ret;
	}

	arg_num = 0;
	/* Write any message arguments */
	while (arg_num < msg->num_args) {
		ret = amd_hsmp_rdwr(root, SMN_HSMP_MSG_DATA + (arg_num << 2),
				    &msg->args[arg_num], HSMP_WR);
		if (ret) {
			pr_err("Error %d writing message argument %d\n", ret, arg_num);
			return ret;
		}
		arg_num++;
	}

	/* Write the message ID which starts the operation */
	ret = amd_hsmp_rdwr(root, SMN_HSMP_MSG_ID, &msg->msg_id, HSMP_WR);
	if (ret) {
		pr_err("Error %d writing message ID %u\n", ret, msg->msg_id);
		return ret;
	}

	/*
	 * Depending on when the trigger write completes relative to the SMU
	 * firmware 1 ms cycle, the operation may take from tens of us to 1 ms
	 * to complete. Some operations may take more. Therefore we will try
	 * a few short duration sleeps and switch to long sleeps if we don't
	 * succeed quickly.
	 */
	short_sleep = jiffies + msecs_to_jiffies(HSMP_SHORT_SLEEP);
	timeout	= jiffies + msecs_to_jiffies(HSMP_MSG_TIMEOUT);

	while (time_before(jiffies, timeout)) {
		ret = amd_hsmp_rdwr(root, SMN_HSMP_MSG_RESP, &mbox_status, HSMP_RD);
		if (ret) {
			pr_err("Error %d reading mailbox status\n", ret);
			return ret;
		}

		if (mbox_status != HSMP_STATUS_NOT_READY)
			break;
		if (time_before(jiffies, short_sleep))
			usleep_range(50, 100);
		else
			usleep_range(1000, 2000);
	}

	if (unlikely(mbox_status == HSMP_STATUS_NOT_READY)) {
		return -ETIMEDOUT;
	} else if (unlikely(mbox_status == HSMP_ERR_INVALID_MSG)) {
		return -ENOMSG;
	} else if (unlikely(mbox_status == HSMP_ERR_INVALID_INPUT)) {
		return -EINVAL;
	} else if (unlikely(mbox_status != HSMP_STATUS_OK)) {
		pr_err("Message ID %u unknown failure (status = 0x%X)\n",
		       msg->msg_id, mbox_status);
		return -EIO;
	}

	/* SMU has responded OK. Read response data */
	arg_num = 0;
	while (arg_num < msg->response_sz) {
		ret = amd_hsmp_rdwr(root, SMN_HSMP_MSG_DATA + (arg_num << 2),
				    &msg->response[arg_num], HSMP_RD);
		if (ret) {
			pr_err("Error %d reading response %u for message ID:%u\n",
			       ret, arg_num, msg->msg_id);
			break;
		}
		arg_num++;
	}

	return ret;
}

int hsmp_send_message(struct hsmp_message *msg)
{
	struct amd_northbridge *nb;
	int ret;

	if (!msg)
		return -EINVAL;

	nb = node_to_amd_nb(msg->sock_ind);
	if (!nb || !nb->root)
		return -ENODEV;

	if (msg->msg_id < HSMP_TEST || msg->msg_id >= HSMP_MSG_ID_MAX)
		return -EINVAL;

	if (msg->num_args > HSMP_MAX_MSG_LEN || msg->response_sz > HSMP_MAX_MSG_LEN)
		return -EINVAL;

	/*
	 * The time taken by smu operation to complete is between
	 * 10us to 1ms. Sometime it may take more time.
	 * In SMP system timeout of 100 millisecs should
	 * be enough for the previous thread to finish the operation
	 */
	ret = down_timeout(&hsmp_sem[msg->sock_ind],
			   msecs_to_jiffies(HSMP_MSG_TIMEOUT));
	if (ret < 0)
		return ret;

	ret = __hsmp_send_message(nb->root, msg);

	up(&hsmp_sem[msg->sock_ind]);

	return ret;
}
EXPORT_SYMBOL_GPL(hsmp_send_message);

static int hsmp_test(u16 sock_ind)
{
	struct hsmp_message msg = { 0 };
	struct amd_northbridge *nb;
	int ret = -ENODEV;

	nb = node_to_amd_nb(sock_ind);
	if (!nb || !nb->root)
		return ret;

	/*
	 * Test the hsmp port by performing TEST command. The test message takes
	 * one argument and returns the value of that argument + 1.
	 */
	msg.sock_ind	= sock_ind;
	msg.response_sz = 1;
	msg.num_args	= 1;
	msg.msg_id	= HSMP_TEST;
	msg.args[0]	= 0xDEADBEEF;

	ret = __hsmp_send_message(nb->root, &msg);
	if (ret)
		return ret;

	if (msg.response[0] != (msg.args[0] + 1)) {
		pr_err("Socket %d test message failed, Expected 0x%08X, received 0x%08X\n",
		       sock_ind, msg.args[0] + 1, msg.response[0]);
		return -EBADE;
	}

	return 0;
}

static int search_msg_list(u32 msgid, u32 *list, int size)
{
	int i;

	for (i = 0; i < size; i++) {
		if (msgid == list[i])
			return 0;
	}
	return -EINVAL;
}

static long hsmp_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	int __user *arguser = (int  __user *)arg;
	struct hsmp_message msg = { 0 };
	int ret;

	if (copy_struct_from_user(&msg, sizeof(msg), arguser, sizeof(struct hsmp_message)))
		return -EFAULT;

	switch (fp->f_mode & (FMODE_WRITE | FMODE_READ)) {
	case FMODE_WRITE:
		/* check if the msgid is a set msg */
		ret = search_msg_list(msg.msg_id, hsmp_set_msgs, ARRAY_SIZE(hsmp_set_msgs));
		if (ret)
			return ret;

		return hsmp_send_message(&msg);
	case FMODE_READ:
		/* check if the msgid is a get msg */
		ret = search_msg_list(msg.msg_id, hsmp_get_msgs, ARRAY_SIZE(hsmp_get_msgs));
		if (ret)
			return ret;
		break;
	case FMODE_READ | FMODE_WRITE:
		ret = search_msg_list(msg.msg_id, hsmp_set_msgs, ARRAY_SIZE(hsmp_set_msgs));
		if (!ret)
			return hsmp_send_message(&msg);

		ret = search_msg_list(msg.msg_id, hsmp_get_msgs, ARRAY_SIZE(hsmp_get_msgs));
		if (ret)
			return ret;

		break;
	default:
		return -EINVAL;
	}

	ret =  hsmp_send_message(&msg);
	if (ret)
		return ret;

	if (copy_to_user(arguser, &msg, sizeof(struct hsmp_message)))
		return -EFAULT;

	return 0;
}

static const struct file_operations hsmp_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= hsmp_ioctl,
	.compat_ioctl	= hsmp_ioctl,
};

static int hsmp_pltdrv_probe(struct platform_device *pdev)
{
	int ret, i;

	hsmp_sem = devm_kzalloc(&pdev->dev,
				(amd_nb_num() * sizeof(struct semaphore)),
				GFP_KERNEL);
	if (!hsmp_sem)
		return -ENOMEM;

	for (i = 0; i < amd_nb_num(); i++)
		sema_init(&hsmp_sem[i], 1);

	hsmp_device.name	= "hsmp_cdev";
	hsmp_device.minor	= MISC_DYNAMIC_MINOR;
	hsmp_device.fops	= &hsmp_fops;
	hsmp_device.parent	= &pdev->dev;
	hsmp_device.nodename	= "hsmp";
	hsmp_device.mode	= 0644;

	ret = misc_register(&hsmp_device);
	if (ret)
		return ret;

	return 0;
}

static int hsmp_pltdrv_remove(struct platform_device *pdev)
{
	misc_deregister(&hsmp_device);

	return 0;
}

static struct platform_driver amd_hsmp_driver = {
	.probe		= hsmp_pltdrv_probe,
	.remove		= hsmp_pltdrv_remove,
	.driver		= {
		.name	= DRIVER_NAME,
	},
};

static struct platform_device *amd_hsmp_platdev;

static int __init hsmp_plt_init(void)
{
	int ret = -ENODEV;
	u16 num_sockets;
	int i;

	/*
	 * amd_nb_num() returns number of SMN/DF interfaces present in the system
	 * if we have N SMN/DF interfaces that ideally means N sockets
	 */
	num_sockets = amd_nb_num();
	if (num_sockets == 0)
		return ret;

	/* Test the hsmp interface on each socket */
	for (i = 0; i < num_sockets; i++) {
		ret = hsmp_test(i);
		if (ret)
			return ret;
	}

	ret = platform_driver_register(&amd_hsmp_driver);
	if (ret)
		return ret;

	amd_hsmp_platdev = platform_device_alloc(DRIVER_NAME, -1);
	if (!amd_hsmp_platdev) {
		ret = -ENOMEM;
		goto drv_unregister;
	}

	ret = platform_device_add(amd_hsmp_platdev);
	if (ret) {
		platform_device_put(amd_hsmp_platdev);
		goto drv_unregister;
	}

	return 0;

drv_unregister:
	platform_driver_unregister(&amd_hsmp_driver);
	return ret;
}

static void __exit hsmp_plt_exit(void)
{
	platform_device_unregister(amd_hsmp_platdev);
	platform_driver_unregister(&amd_hsmp_driver);
}

device_initcall(hsmp_plt_init);
module_exit(hsmp_plt_exit);

MODULE_DESCRIPTION("AMD HSMP Platform Interface Driver");
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL v2");
