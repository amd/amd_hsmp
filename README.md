# amd_hsmp

NOTE: The AMD HSMP driver is now part of the Linux kernel upstream starting in v5.18-rc1. Review kernel.org for updated versions of this driver.

AMD HSMP module to provide user interface to system management features

The Host System Management Port (HSMP) kernel module provides user level
access to the HSMP mailboxes implemented by the firmware in the
System Management Unit (SMU). Full documentation of the HSMP can
be found in the Processor Programming Reference (PPR) for Family
19h on AMD's Developer Central.

https://developer.amd.com/resources/epyc-resources/epyc-specifications

E-SMI library provides C API fo the user space application on top of this
module.


Disclaimer
===========

The amd_hsmp module is supported only on AMD Family 19h (including
third-generation AMD EPYC processors (codenamed "Milan")) or later
CPUs. Using the amd_hsmp module on earlier CPUs could produce unexpected
results, and may cause the processor to operate outside of your motherboard
or system specifications. Correspondingly, defaults to only executing on
AMD Family 19h Model 0 ~ Fh, 10h ~ 1Fh, 30h ~ 3Fh, & A0 ~ AFh,
family 1Ah Model 0 ~ 1fh EPYC processors and family 19h model 90 ~ 9fh
MI300A processors.


Interface
---------

See amd_hsmp.h for details about the IOCTL interface.


BIOS configuration
------------------

HSMP PCIe interface needs to be enabled in the BIOS. The CBS option can be found
by navigating to the following path

#####  ```Advanced > AMD CBS > NBIO Common Options > SMU Common Options > HSMP Support```
#####  ```	BIOS Default:     “Auto” (Disabled)```

  If the option is disabled, calls to the SMU will result in a timeout.


Build and Install
-----------------

Kernel development packages for the running kernel need to be installed
prior to building the HSMP module. A Makefile is provided which should
work with most kernel source trees.

To build the kernel module:

#> make

To install the kernel module:

#> sudo make modules_install

To clean the kernel module build directory:

#> make clean


Loading
-------

If the HSMP modules are installed in standard path you should use the modprobe command to
load the module.

To load hsmp modules on platforms with ACPI supported BIOS (Family 0x1A, model 0x00-0x1F and future platforms):
#> sudo modprobe hsmp_acpi

To load hsmp module on non-ACPI BIOS(Family 0x19, model 0x00-0x1F, 0x30-0x3F, 0x90-0x9F, 0xA0-0xAF):
#> sudo modprobe amd_hsmp


DKMS support
------------

Building Module with running version of kernel

Add the module to DKMS tree:
#> sudo dkms add ../hsmp_driver

Build the module using DKMS:
#> sudo dkms build amd_hsmp/2.4

Install the module using DKMS:
#> sudo dkms install amd_hsmp/2.4

Load the module:
#> sudo modprobe amd_hsmp

Building Module with specific version of kernel

Add the module to DKMS tree:
#> sudo dkms add ../hsmp_driver

Build the module using DKMS:
#> sudo dkms build amd_hsmp/2.4 -k linux_version

Install the module using DKMS:
#> sudo dkms install amd_hsmp/2.4 -k linux_version
Module is built: /lib/modules/linux_version/updates/dkms/

Notes: It is required to have specific linux verion header in /usr/src

To remove module from dkms tree
#> sudo dkms remove -m amd_hsmp/2.4 --all
#> sudo rm -rf /usr/src/amd_hsmp-2.4/
