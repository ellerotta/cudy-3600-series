Copyright 2024 Broadcom Corp.  Proprietary and confidential.

------------
Software Version:
impl105, WLAN software package version 17.10.369.61 (23.2.2)

------------
Install:
- Download wifi_src-23.2.2-17.10.369.61.tgz and use sample instructions in INSTALL-src-wifi.txt to combine common core and dependency packages to SDK.

------------
Patch Sources:
- run "./dopatch apply"

------------
Update Profile:

5.04L.04p3

- ./release/maketargets <PROFILE>_WL23D2D2GA_WLMLO

- make PROFILE=<PROFILE>_WL23D2D2GA_WLMLO


------------
MFG/DVT Test build switch (for applicable boot loader):
- Use regular firmware:

    CFE:   WLan Feature                      : 0x00

    uboot: => setenv wlFeature 0; saveenv

   Check wl ver command does NOT return "WLTEST" in the version string


- Use mfgtest firmware:

   CFE: WLan Feature                      : 0x02

   uboot: => setenv wlFeature 2; saveenv

   Check wl ver command should return "WLTEST" in the version string

------------
Additional notes:

Notes about mfgtest firmware:
   **********************************************************************************************************************************************
***Mfgtest firmware is to support capability required by WLAN hardware (MFGc/DVT) testing. For ANY other purposes, regular firmware should be used.***
   ***********************************************************************************************************************************************

-Default image will include both mfgtest and regular firmware, CFE "WLan Feature" selects which firmware to use
-If there is size limitation or else requirement to remove mfgtest firmware:
  -BUILD_BCM_WLAN_NO_MFGBIN option (WLNOMFGBIN.arch) is available to delete mfgtest firmware from target directory
-If there is source code change, mfgtest prebuilt binary should be rebuilt to take effect:
  e.g. #make clean; make PROFILE=<profile>_WLMFGTEST; #this builds and saves mfgtest wlan binaries in the source tree, the image built at this point is mfgtest capable
       #make clean; make PROFILE=<profile>;           #go on with normal builds

------------
Emphasize again because it is VERY important:

Please make sure that non-MFGTEST image is used before starting any WFA certification testing.
For MFGTEST image (not for WFA certification testing), the wl ver shows WLTEST, e.g.,

# wl ver
Broadcom BCA: 17.10 RC121.11
wl0: Feb  6 2020 17:00:26 version 17.10.121.11 (r783116 WLTEST) FWID 01-88eb0b67


For non-MFGTEST image (good for WFA certificaiton testing), the wl ver does not have WLTEST, e.g.,
# wl ver
Broadcom BCA: 17.10 RC121.11
wl0: Feb  6 2020 17:17:05 version 17.10.121.11 (r783116) FWID 01-b12d23ed

-Mfgtest firmware is to support capability required by WLAN hardware (MFGc/DVT) testing. 
 For software/field/certification testing and final production, normal (non-mfgtest)firmware should be used.
