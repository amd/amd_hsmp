# SPDX-License-Identifier: GPL-2.0
#
# Kbuild for AMD Host System Management Port driver
#
# Copyright (C) 2019 Advanced Micro Devices, Inc.
#
# Author:
#   Lewis Carroll <lewis.carroll@amd.com>
#

hsmp_acpi-objs := acpi.o
obj-m := hsmp_acpi.o

hsmp_common-objs := hsmp.o
obj-m += hsmp_common.o

amd_hsmp-objs := plat.o
obj-m += amd_hsmp.o
