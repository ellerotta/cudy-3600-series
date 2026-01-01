Field Repartioning of BOOTFS on eMMC devices
============================================
Field repartitioning of bootfs partitions on eMMC devices is achieved by INCREASING the size of BOOTFS while DECREASING the size of ROOTFS. Repartitioning feature requires the use of 2 files:
- repart.sh : A bash script which rewrites GPT partition table and invokes blkpg-part app
- blkg-part : A linux app which deletes/creates linux partitions 

To enable this feature, the following items need to be enabled in the <PROFILE>:
BUILD_EMMC_REPART : Enables the feature
BRCM_REPART_MIN_BOOTFS_SIZE_MB : The new required BOOTFS size

To run repartitioning, ONLY the script needs to be directly run:
Usage: repart.sh <index of image that needs repartitioning [1|2]>"

The repartioning algorithm is as follows:
0. During the build, the value of BRCM_REPART_MIN_BOOTFS_SIZE_MB gets filled into repart.sh
1. User calls script with index of image which requires repartitioning ( in reference software this is automatically done during an image upgrade via bcm_imgif_pktb.c )
2. Script calculates delta increase required in bootfs size
3. Script calculates delta decrease in rootfs size as a result of the bootfs size increase
4. Script deletes and recreates bootfsx and rootfsx GPT partitions 
5. Script deletes and recreates the linux /dev partitions

For this script to work, the following preconditions must be true:
- Bootfs and rootfs must be adjacent
- Require bootfs size increase will result in rootfs decrease. New rootfs binary must fit in this reduced rootfs
- The repartitiong target image should be the INACTIVE image. Running repart.sh on ACTIVE image will result in failure

The script returns an error if:
-Any of the sgdisk commands fail
-If the in-memory linux repartitioning fails

It is recommended that a REBOOT be initiated if the script returns an non-zero return value
