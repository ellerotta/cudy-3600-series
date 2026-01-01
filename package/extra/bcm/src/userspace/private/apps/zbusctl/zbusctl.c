/***********************************************************************
 *
 *
<:copyright-BRCM:2020:proprietary:standard

   Copyright (c) 2020 Broadcom
   All Rights Reserved

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:>
 *
 ************************************************************************/


/*!\file zbusctl.c
 * \brief A debug app for the BCM Z-Bus (which is a wrapper around D-Bus and
 *        U-Bus).
 *
 * Example usage:
 *
 * getpn on Device.DeviceInfo:
 *    zbusctl -c devinfo getpn Device.DeviceInfo.
 *    zbusctl -c devinfo --nextLevelOnly getpn Device.DeviceInfo.
 *
 * getpv on Device.DeviceInfo:
 *    zbusctl -c devinfo getpv Device.DeviceInfo.
 *    zbusctl -c devinfo -n getpv Device.DeviceInfo.
 *
 * getpv on Device.DeviceInfo with flags=OGF_NO_VALUE_UPDATE (useful when
 *    doing perf measurements on the MDM itself, not the STL read times).
 *    zbusctl --perf -c devinfo -f 0x2 getpv Device.DeviceInfo.
 *
 * When doing perf measurements, a single loop shows the cost of a single cmd,
 * but multiple loops show the speed/efficiency of the software itself, without
 * initial start up costs such as context switching and cache warmup.
 *    zbusctl --perf --loops 10000 -c devinfo -f 0x2 getpv Device.DeviceInfo.
 *
 * setpv on Device.DSL.Line.1.Enable
 *    zbusctl -c dsl setpv Device.DSL.Line.1.Enable boolean true
 *
 * Do a set without calling the the RCL handler function (OSF_NO_RCL_CALLBACK
 * which is defined as 1), useful for perf measurements and mem leak checks:
 *    zbusctl -c dsl -f 1 setpv Device.DSL.Line.1.Enable boolean true
 *
 * Check for memory leaks on the getpn path:
 *    zbusctl --mem --loops 20000 -i 5000 -c devinfo getpn Device.DeviceInfo.
 *
 * Check for memory leaks on the getpv path:
 *    zbusctl --mem --loops 20000 -i 5000 -f 0x2 -c devinfo getpv Device.DeviceInfo.
 *
 * Check for memory leaks on the setpv path:
 *    zbusctl --mem --loops 20000 -i 5000 -f 1 -c dsl setpv Device.DSL.Line.1.Enable boolean true
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>

#include "bdk.h"
#include "bdk_dbus.h"
#include "bcm_retcodes.h"
#include "sysutil_proc.h"
#include "cms_util.h"
#include "bcm_ulog.h"
#include "cms_msg.h"
#include "cms_msg_pubsub.h"
#include "bcm_zbus_intf.h"
#include "bcm_generic_hal_utils.h"   // in libcms_util.h


static void usage()
{
   printf("usage: zbusctl -c compName [optional args] cmd fullpath [depending on cmd, may require additional args]\n");
   printf("       required arg: -c or --compName [dsl|devinfo|sysmgmt|etc]\n");
   printf("       required cmd: [getpn | getpv | setpv] \n");
   printf("       optional args: -v or --verbosity [0, 1, or 2]\n");
   printf("                      -l or --loops (default is 1)\n");
   printf("                      -f or --flags (bitmask of OGF_xxx bits)\n");
   printf("                      -n or --nextLevelOnly (no args, default is 0)\n");
   printf("       zbusctl -c compName [optional args] getpn <fullpath>\n");
   printf("       zbusctl -c compName [optional args] getpv <fullpath>\n");
   printf("       zbusctl -c compName [optional args] setpv <fullpath> <type> <value>\n");
   exit(-1);
}


// Global test config parms
typedef struct {
   int do_help;      // if 1, print usage and exit
   int do_perf;      // if 1, do performance measurements, suppress verbose output.
   int do_mem;       // if 1, do memory leak checking
   int nextLevelOnly; // this is false by default
   int verbosity;    // 0: log error only, 1: notice, 2: debug
   int loops;        // by default, run command once
   int progressInterval; // print results every progressInterval times through the loop.
   int growthDivisor;  // consider 10% growth as sign of mem leak
   int flags;
   char *compName;  // remote compName
} ZbusTestParams;

ZbusTestParams testParams = {0, 0, 0, 0, 0,
            1, 10000, 10,  // loops, progress interval, growthDivisor
            0, NULL};      // flags, compName


// For perf measurement
CmsTimestamp startTms;
CmsTimestamp stopTms;

void dumpPerf()
{
   UINT32 deltaMs;

   cmsTms_get(&stopTms);

   deltaMs = cmsTms_deltaInMilliSeconds(&stopTms, &startTms);
   printf("%d loops in %d ms ==> %d ms/op\n",
          testParams.loops, deltaMs, deltaMs/testParams.loops);
   return;
}


// For mem leak measurements
CmsMemStats startMemStats;
CmsMemStats stopMemStats;
ProcThreadInfo startProcInfo;
ProcThreadInfo stopProcInfo;

void dumpMem(int loop)
{
   int rc = 0;
   UINT32 uThresh=8 * 1024;  // CMS mem reported in bytes, so 8KB.  CMS has observed overhead of 6KB.
   int iThreshKB=8;      // proc mem reported in KB, so 8KB
   int pid = getpid();

   sysUtl_getThreadInfoFromProc(pid, &stopProcInfo);
   cmsMem_getStats(&stopMemStats);

   printf("After %d loops:\n", loop);
   printf("   CMS shared mem (KB) %d -> %d, delta KB %d (allocs=%d)\n",
          startMemStats.shmBytesAllocd/1024, stopMemStats.shmBytesAllocd/1024,
          (stopMemStats.shmBytesAllocd - startMemStats.shmBytesAllocd)/1024,
          stopMemStats.shmNumAllocs);
   printf("   CMS heap mem (KB) %d -> %d, delta KB %d (allocs=%d)\n",
          startMemStats.bytesAllocd/1024, stopMemStats.bytesAllocd/1024,
          (stopMemStats.bytesAllocd - startMemStats.bytesAllocd)/1024,
          stopMemStats.numAllocs);
   printf("   total mem (KB) %d -> %d, delta KB %d\n",
          startProcInfo.totalMemKB, stopProcInfo.totalMemKB,
          (stopProcInfo.totalMemKB - startProcInfo.totalMemKB));

   // check growth in terms of percentage and minimum growth thresh (4KB).
   if (((stopMemStats.shmBytesAllocd - startMemStats.shmBytesAllocd) >
        (startMemStats.shmBytesAllocd / testParams.growthDivisor)) &&
       ((stopMemStats.shmBytesAllocd - startMemStats.shmBytesAllocd) > uThresh))
   {
      printf("memory leak detected in CMS shared mem!\n");
      rc = -1;
   }

   if (((stopMemStats.bytesAllocd - startMemStats.bytesAllocd) >
        (startMemStats.bytesAllocd / testParams.growthDivisor)) &&
       ((stopMemStats.bytesAllocd - startMemStats.bytesAllocd) > uThresh))
   {
      printf("memory leak detected in CMS heap mem!\n");
      rc = -1;
   }

   if (((stopProcInfo.totalMemKB- startProcInfo.totalMemKB) >
        (startProcInfo.totalMemKB / testParams.growthDivisor)) &&
       ((stopProcInfo.totalMemKB- startProcInfo.totalMemKB) > iThreshKB))
   {
      printf("memory leak detected in total app mem!\n");
      rc = -1;
   }

   if (rc < 0)
   {
      //exit(rc);
      return;
   }

   printf("No memory leaks detected.\n");
   return;
}


int processGetpn(const char *fullpath)
{
   const ZbusAddr *destAddr;
   BcmGenericParamInfo *paramInfoArray=NULL;
   UINT32 numParamInfos=0;
   UBOOL8 nextLevel=FALSE;
   BcmRet ret;
   int i;

   if (testParams.do_perf)
      cmsTms_get(&startTms);


   destAddr = zbusIntf_componentNameToZbusAddr(testParams.compName);
   if (destAddr == NULL)
   {
      printf("Unrecognized compName %s\n", testParams.compName);
      return -1;
   }

   // convert int to UBOOL8
   if (testParams.nextLevelOnly)
      nextLevel = TRUE;

   for (i=0; i < testParams.loops; i++)
   {
      if (testParams.do_mem && i == 1)
      {
         // take a snapshot of CMS shared mem, CMS heap mem, and total app mem.
         // Take the snapshot after first operation so any one-time startup
         // memory allocs are not included.
         int pid = getpid();
         sysUtl_getThreadInfoFromProc(pid, &startProcInfo);
         cmsMem_getStats(&startMemStats);
      }

      ret = zbus_out_getParameterNames(destAddr, fullpath,
                                       nextLevel, testParams.flags,
                                       &paramInfoArray, &numParamInfos);
      if (ret != BCMRET_SUCCESS)
      {
         printf("zbus_out_getParameterNames (%s) failed, ret=%d", fullpath, ret);
         return -1;
      }

      if ((testParams.do_perf == 0) &&
          ((i % testParams.progressInterval) == 0))
      {
         UINT32 j;
         printf("[loop=%06d] getpn %s (nextLevel=%d) returned:\n",
                i, fullpath, nextLevel);
         for (j=0; j < numParamInfos; j++)
         {
            printf("  [%02d] profile=%s type=%s %s\n", j,
                   paramInfoArray[j].profile,
                   paramInfoArray[j].type,
                   paramInfoArray[j].fullpath);
         }
         printf("\n\n");

         // do a mem leak check every progressInterval as well
         if (testParams.do_mem && i > 1)
            dumpMem(i);
      }

      cmsUtl_freeParamInfoArray(&paramInfoArray, numParamInfos);
      numParamInfos = 0;
   }

   if (testParams.do_perf)
      dumpPerf();

   if (testParams.do_mem)
      dumpMem(i);

   return 0;
}


int processGetpv(const char *fullpath)
{
   const ZbusAddr *destAddr;
   const char *fullpathArray[] = {fullpath};
   BcmGenericParamInfo *paramInfoArray=NULL;
   UINT32 numParamInfos=0;
   UBOOL8 nextLevel=FALSE;
   BcmRet ret;
   int i;

   if (testParams.do_perf)
      cmsTms_get(&startTms);


   destAddr = zbusIntf_componentNameToZbusAddr(testParams.compName);
   if (destAddr == NULL)
   {
      printf("Unrecognized compName %s\n", testParams.compName);
      return -1;
   }

   // convert int to UBOOL8
   if (testParams.nextLevelOnly)
      nextLevel = TRUE;

   for (i=0; i < testParams.loops; i++)
   {
      if (testParams.do_mem && i == 1)
      {
         // take a snapshot of CMS shared mem, CMS heap mem, and total app mem.
         // Take the snapshot after first operation so any one-time startup
         // memory allocs are not included.
         int pid = getpid();
         sysUtl_getThreadInfoFromProc(pid, &startProcInfo);
         cmsMem_getStats(&startMemStats);
      }

      // This function can handle multiple fullpaths, but this test app only
      // passes in 1.
      ret = zbus_out_getParameterValues(destAddr, fullpathArray, 1,
                                        nextLevel, testParams.flags,
                                        &paramInfoArray, &numParamInfos);
      if (ret != BCMRET_SUCCESS)
      {
         printf("zbus_out_getParameterValues failed, ret=%d", ret);
         return -1;
      }

      if ((testParams.do_perf == 0) &&
          ((i % testParams.progressInterval) == 0))
      {
         UINT32 j;
         printf("[%06d] getpv %s (nextLevel=%d) returned:\n",
                i, fullpath, nextLevel);
         for (j=0; j < numParamInfos; j++)
         {
            printf("[%02d] type=%s %s=%s\n", j,
                   paramInfoArray[j].type,
                   paramInfoArray[j].fullpath,
                   paramInfoArray[j].value);
         }
         printf("\n\n");

         // do a mem leak check every progressInterval as well
         if (testParams.do_mem && i > 1)
            dumpMem(i);
      }

      cmsUtl_freeParamInfoArray(&paramInfoArray, numParamInfos);
      numParamInfos = 0;
   }

   if (testParams.do_perf)
      dumpPerf();

   if (testParams.do_mem)
      dumpMem(i);

   return 0;
}


int processSetpv(const char *fullpath, const char *type, const char *value)
{
   const ZbusAddr *destAddr;
   BcmGenericParamInfo paramInfo;
   BcmRet ret;
   int i;

   memset(&paramInfo, 0, sizeof(paramInfo));
   paramInfo.fullpath = (char *) fullpath;
   paramInfo.type = (char *) type;
   paramInfo.value = (char *) value;
   printf("Setting %s (type=%s) to %s\n",
           paramInfo.fullpath, paramInfo.type, paramInfo.value);


   if (testParams.do_perf)
      cmsTms_get(&startTms);

   destAddr = zbusIntf_componentNameToZbusAddr(testParams.compName);
   if (destAddr == NULL)
   {
      printf("Unrecognized compName %s\n", testParams.compName);
      return -1;
   }

   for (i=0; i < testParams.loops; i++)
   {
      if (testParams.do_mem && i == 1)
      {
         // take a snapshot of CMS shared mem, CMS heap mem, and total app mem.
         // Take the snapshot after first operation so any one-time startup
         // memory allocs are not included.
         int pid = getpid();
         sysUtl_getThreadInfoFromProc(pid, &startProcInfo);
         cmsMem_getStats(&startMemStats);
      }

      // This function can handle multiple fullpaths, but this test app only
      // passes in 1.
      ret = zbus_out_setParameterValues(destAddr, &paramInfo, 1,
                                        testParams.flags);
      if (ret != BCMRET_SUCCESS)
      {
         printf("zbus_out_setParameterValues failed, ret=%d", ret);
         return -1;
      }

      if ((testParams.do_perf == 0) &&
          ((i % testParams.progressInterval) == 0))
      {
         printf("[%06d] setpv %s to %s (type=%s) OK.\n",
                i, fullpath, value, type);

         // do a mem leak check every progressInterval as well
         if (testParams.do_mem && i > 1)
            dumpMem(i);
      }
   }

   if (testParams.do_perf)
      dumpPerf();

   if (testParams.do_mem)
      dumpMem(i);

   return 0;
}


struct option long_options[] =
{
   {"help", no_argument, &(testParams.do_help), 1},  // -h
   {"perf", no_argument, NULL, 'p'},
   {"mem", no_argument, NULL, 'm'},
   {"nextLevelOnly", no_argument, &(testParams.nextLevelOnly), 1},  // -n
   {"verbosity", required_argument, NULL, 'v'},
   {"loops", required_argument, NULL, 'l'},
   {"interval", required_argument, NULL, 'i'},
   {"flags", required_argument, NULL, 'f'},
   {"compName", required_argument, NULL, 'c'},
};

int main(int argc, char **argv)
{
   int i, c;
   int longopt_index = 0;
   int r=0;

   if (argc < 2)
      usage();

   while (1)
   {
      c = getopt_long(argc, argv, "hpmnv:l:i:f:c:", long_options, &longopt_index);
      if (c == -1)
         break;
         
      switch(c)
      {
         case 0:
            // long opt without short opt.
            printf("set longopt %s to %d\n",
                    long_options[longopt_index].name,
                    long_options[longopt_index].val);
            break;

         case 'h':
            testParams.do_help = 1;
            break;

         case 'p':
            testParams.do_perf = 1;
            printf("do performance/timing measurements, verbose output will be suppressed\n");
            break;

         case 'm':
            testParams.do_mem = 1;
            printf("do memory leak checking\n");
            break;

         case 'n':
            testParams.nextLevelOnly = 1;
            printf("nextLevelOnly=1\n");
            break;

         case 'v':
             testParams.verbosity = atoi(optarg);
             printf("verbosity=%d\n", testParams.verbosity);
             if (testParams.verbosity == 1)
                bcmuLog_setLevel(BCMULOG_LEVEL_NOTICE);
             else if (testParams.verbosity > 1)
                bcmuLog_setLevel(BCMULOG_LEVEL_DEBUG);
             break;

         case 'l':
            testParams.loops = atoi(optarg);
            printf("loops=%d\n", testParams.loops);
            break;

         case 'i':
            testParams.progressInterval = atoi(optarg);
            printf("progressInterval=%d\n", testParams.progressInterval);
            break;

         case 'f':
            testParams.flags = (UINT32) strtoul(optarg, NULL, 0);
            printf("flags=0x%x\n", testParams.flags);
            break;

         case 'c':
            testParams.compName = optarg;
            printf("remote compName=%s\n", testParams.compName);
            break;

          default:
             testParams.do_help = 1;
            break;
       }
   }

   if (testParams.do_help)
      usage();

   if (testParams.do_mem && testParams.loops < 2)
   {
      printf("mem leak checking requires at least 2 loops, set loops=2\n");
      testParams.loops = 2;
   }

   // printf("argc=%d optind=%d\n", argc, optind);
   printf("top level command argv[%d] = %s\n", optind, argv[optind]);
   for (i=optind+1; i < argc; i++)
       printf("argv[%d]=%s\n", i, argv[i]);


   if (!strcasecmp(argv[optind], "getpn"))
   {
      if (optind + 2 != argc)
      {
         printf("getpn must have exactly 1 arg: fullpath\n");
         usage();
      }
      r = processGetpn(argv[optind+1]);
   }
   else if (!strcasecmp(argv[optind], "getpv"))
   {
      if (optind + 2 != argc)
      {
         printf("getpv must have exactly 1 arg: fullpath\n");
         usage();
      }
      r = processGetpv(argv[optind+1]);
   }
   else if (!strcasecmp(argv[optind], "setpv"))
   {
      if (optind + 4 != argc)
      {
         printf("Must have 3 args: fullpath type value\n");
         usage();
      }
      r = processSetpv(argv[optind+1], argv[optind+2], argv[optind+3]);
   }
   else
   {
      printf("Unsupported cmd %s\n", argv[optind]);
      usage();
   }

   return r;
}

