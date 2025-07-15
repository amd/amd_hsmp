# Host System Management Driver(HSMP)

HSMP driver provides user interface to system management features.

# Changes Notes

## Highlights of release tag 2.0 [July 15, 2025]
*  HSMP is made compilable for different versions of Kernel(5.10 to 6.16)
*  README is updated
*  amd_nb_num() API is replaced with topology_max_packages() API
*  Sysfs is supported for ACPI driver (cherry-picked from upstream kernel)
*  Hwmon support added for reporting power (cherry-picked from upstream kernel)
*  Additional print messages added (cherry-picked from upstream kernel)
*  Cherrypicked commits from upstream kernel, to recheck the return status of SMU after final sleep.
   * replace down_timeout() with down_interruptible()
   * move final timeout check to after final sleep

## Highlights of release tag 1.0 [April 9, 2025]
*  DKMS config updated
*  HSMP is made compilable for different versions of Kernel
*  README is updated
*  RELEASENOTES.md file is newly added
