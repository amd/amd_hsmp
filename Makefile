# SPDX-License-Identifier: GPL-2.0
#
# Makefile for AMD Host System Management Port driver
#
# Copyright (C) 2019 Advanced Micro Devices, Inc.
#
# Author:
#   Lewis Carroll <lewis.carroll@amd.com>
#

# If KDIR is not specified, assume the development source link
# is in the modules directory for the running kernel
KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD modules
	$(MAKE) controller

debug:
	$(MAKE) CFLAGS_MODULE=-DDEBUG -C $(KDIR) M=$$PWD modules

modules: default

modules_install:
	$(MAKE) -C $(KDIR) M=$$PWD modules_install

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
	rm amd_uncore_controller

controller:
	gcc -o amd_uncore_controller controller.c

help:
	@echo "\nThe following make targets are supported:\n"
	@echo "default\t\tBuild the driver module (or if no make target is supplied)"
	@echo "modules\t\tSame as default"
	@echo "debug\t\tBuild the driver module with debug output"
	@echo "modules_install\tBuild and install the driver module"
	@echo "clean"
	@echo

.PHONY: default debug modules modules_install clean help

