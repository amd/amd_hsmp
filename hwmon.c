// SPDX-License-Identifier: GPL-2.0
/*
 * AMD HSMP hwmon support
 * Copyright (c) 2025, AMD.
 * All Rights Reserved.
 *
 * This file provides hwmon implementation for HSMP interface.
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/types.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0)
#include <linux/units.h>
#endif

#include "hsmp.h"
#include "amd_hsmp.h"

#define HSMP_HWMON_NAME		"amd_hsmp_hwmon"

static int hsmp_hwmon_write(struct device *dev, enum hwmon_sensor_types type,
			    u32 attr, int channel, long val)
{
	u16 sock_ind = (uintptr_t)dev_get_drvdata(dev);
	struct hsmp_message msg = {};

	if (type != hwmon_power)
		return -EOPNOTSUPP;

	if (attr != hwmon_power_cap)
		return -EOPNOTSUPP;

	msg.num_args = 1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0)
	msg.args[0] = val / MICROWATT_PER_MILLIWATT;
#else
	msg.args[0] = val / 1000L;
#endif
	msg.msg_id = HSMP_SET_SOCKET_POWER_LIMIT;
	msg.sock_ind = sock_ind;
	return hsmp_send_message(&msg);
}

static int hsmp_hwmon_read(struct device *dev,
			   enum hwmon_sensor_types type,
			   u32 attr, int channel, long *val)
{
	u16 sock_ind = (uintptr_t)dev_get_drvdata(dev);
	struct hsmp_message msg = {};
	int ret;

	if (type != hwmon_power)
		return -EOPNOTSUPP;

	msg.sock_ind = sock_ind;
	msg.response_sz = 1;

	switch (attr) {
	case hwmon_power_input:
		msg.msg_id = HSMP_GET_SOCKET_POWER;
		break;
	case hwmon_power_cap:
		msg.msg_id = HSMP_GET_SOCKET_POWER_LIMIT;
		break;
	case hwmon_power_cap_max:
		msg.msg_id = HSMP_GET_SOCKET_POWER_LIMIT_MAX;
		break;
	default:
		return -EOPNOTSUPP;
	}

	ret = hsmp_send_message(&msg);
	if (!ret)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0)
		*val = msg.args[0] * MICROWATT_PER_MILLIWATT;
#else
		*val = msg.args[0] * 1000L;
#endif

	return ret;
}

static umode_t hsmp_hwmon_is_visble(const void *data,
				    enum hwmon_sensor_types type,
				    u32 attr, int channel)
{
	if (type != hwmon_power)
		return 0;

	switch (attr) {
	case hwmon_power_input:
		return 0444;
	case hwmon_power_cap:
		return 0644;
	case hwmon_power_cap_max:
		return 0444;
	default:
		return 0;
	}
}

static const struct hwmon_ops hsmp_hwmon_ops = {
	.read = hsmp_hwmon_read,
	.is_visible = hsmp_hwmon_is_visble,
	.write	= hsmp_hwmon_write,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
static const struct hwmon_channel_info * const hsmp_info[] = {
	HWMON_CHANNEL_INFO(power, HWMON_P_INPUT | HWMON_P_CAP | HWMON_P_CAP_MAX),
	NULL
};
#else
static const struct hwmon_channel_info *hsmp_info[] = {
	HWMON_CHANNEL_INFO(power, HWMON_P_INPUT | HWMON_P_CAP | HWMON_P_CAP_MAX),
	NULL
};
#endif

static const struct hwmon_chip_info hsmp_chip_info = {
	.ops = &hsmp_hwmon_ops,
	.info = hsmp_info,
};

int hsmp_create_sensor(struct device *dev, u16 sock_ind)
{
	struct device *hwmon_dev;

	hwmon_dev = devm_hwmon_device_register_with_info(dev, HSMP_HWMON_NAME,
							 (void *)(uintptr_t)sock_ind,
							 &hsmp_chip_info,
							 NULL);
	return PTR_ERR_OR_ZERO(hwmon_dev);
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
EXPORT_SYMBOL_NS(hsmp_create_sensor, "AMD_HSMP");
#else
EXPORT_SYMBOL_NS(hsmp_create_sensor, AMD_HSMP);
#endif
