# amd_hsmp
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
AMD Family 19h Model (0 ~ Fh & 30h ~ 3Fh) server line of processors.


Interface
---------

See amd_hsmp.c for details about the SysFS interface.


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

If the HSMP module was installed you should use the modprobe command to
load the module.

#> sudo modprobe amd_hsmp

The HSMP module can also be loaded using insmod if the module was not
installed:

#> sudo insmod ./amd_hsmp.ko
