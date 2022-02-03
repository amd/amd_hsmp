/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */

#ifndef _UAPI_ASM_X86_AMD_HSMP_H_
#define _UAPI_ASM_X86_AMD_HSMP_H_

#include <linux/types.h>

#pragma pack(4)

#define HSMP_MAX_MSG_LEN 8

/*
 * HSMP Message types supported
 */
enum hsmp_message_ids {
	HSMP_TEST = 1,
	HSMP_GET_SMU_VER,
	HSMP_GET_PROTO_VER,
	HSMP_GET_SOCKET_POWER,
	HSMP_SET_SOCKET_POWER_LIMIT,
	HSMP_GET_SOCKET_POWER_LIMIT,
	HSMP_GET_SOCKET_POWER_LIMIT_MAX,
	HSMP_SET_BOOST_LIMIT,
	HSMP_SET_BOOST_LIMIT_SOCKET,
	HSMP_GET_BOOST_LIMIT,
	HSMP_GET_PROC_HOT,
	HSMP_SET_XGMI_LINK_WIDTH,
	HSMP_SET_DF_PSTATE,
	HSMP_AUTO_DF_PSTATE,
	HSMP_GET_FCLK_MCLK,
	HSMP_GET_CCLK_THROTTLE_LIMIT,
	HSMP_GET_C0_PERCENT,
	HSMP_SET_NBIO_DPM_LEVEL,
	HSMP_RESERVED,
	HSMP_GET_DDR_BANDWIDTH,
	HSMP_GET_TEMP_MONITOR,
	HSMP_MSG_ID_MAX,
};

struct hsmp_message {
	__u32	msg_id;				/* Message ID */
	__u16	num_args;			/* Number of arguments in message */
	__u16	response_sz;			/* Number of expected response words */
	__u32	args[HSMP_MAX_MSG_LEN];		/* Argument(s) */
	__u32	response[HSMP_MAX_MSG_LEN];	/* Response word(s) */
	__u16	sock_ind;			/* socket number */
};

/* Reset to default packing */
#pragma pack()

/* Define unique ioctl command for hsmp msgs using generic _IOWR */
#define HSMP_BASE_IOCTL_NR			0xF8
#define HSMP_IOCTL_CMD				_IOWR(HSMP_BASE_IOCTL_NR, 0, struct hsmp_message)

#endif /*_ASM_X86_AMD_HSMP_H_*/
