/***********************************************************************
 *
 *  Copyright (c) 2018  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2018:proprietary:standard

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>

#ifndef DESKTOP_LINUX
#include "bcmutils.h"
#include <wl_common_defs.h>
#include <bcmwifi_channels.h>
#endif

#include "wlsysutil.h"

#define DEV_TYPE_LEN	8
#define IF_NAME_SIZE    16
#define CMD_LENGTH      256
#define BUF_LENGTH      1024

#ifndef  WL_CHANSPEC_BW_320
#define WL_CHANSPEC_BW_320     0x3000u
#if defined(WL11BE) || !defined(DONGLEBUILD)
#define CHSPEC_IS320(chspec)  (((chspec) & WL_CHANSPEC_BW_MASK) == WL_CHANSPEC_BW_320)
#else
#define CHSPEC_IS320(chspec) (FALSE)
#endif
#endif /* WL_CHANSPEC_BW_320 */

#ifndef DESKTOP_LINUX
static int _wl_get_dev_type(char *name, void *buf, int len)
{
   int s;
   int ret = -1;
   struct ifreq ifr;
   struct ethtool_drvinfo info;

   /* open socket to kernel */
   if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
      perror("_wl_get_dev_type: open socket");
      return ret;
   }

   /* get device type */
   memset(&info, 0, sizeof(info));
   info.cmd = ETHTOOL_GDRVINFO;
   ifr.ifr_data = (caddr_t)&info;
   strncpy(ifr.ifr_name, name, IF_NAME_SIZE-1);
   if ((ret = ioctl(s, SIOCETHTOOL, &ifr)) < 0) {

      /* print a good diagnostic if not superuser */
      if (errno == EPERM)
         perror("_wl_get_dev_type: SIOCETHTOOL");

      *(char *)buf = '\0';
   }
   else
   {
      strncpy(buf, info.driver, len);
   }

   close(s);
   return ret;
}
#endif

/*
 * Return the number of wlan adapters running on the system.
 * Dynamically detect it at run time.
 */
int wlgetintfNo(void)
{
#ifdef DESKTOP_LINUX
   return 2;
#else
   char proc_net_dev[] = "/proc/net/dev";
   FILE *fp;
   char buf[BUF_LENGTH], *c, *name;
   char dev_type[DEV_TYPE_LEN];
   int wlif_num=0;

   if (!(fp = fopen(proc_net_dev, "r")))
   {
      printf("Error reading %s!\n", proc_net_dev);
      goto out;
   }

   /* eat first two lines */
   if (!fgets(buf, sizeof(buf), fp) ||
         !fgets(buf, sizeof(buf), fp)) 
   {
      fclose(fp);
      goto out;
   }

   while (fgets(buf, sizeof(buf), fp))
   {
      c = buf;
      while (isspace(*c))
         c++;
      if (!(name = strsep(&c, ":")))
         continue;
      //printf("#### %s:%d  name:%s ####\r\n", __FUNCTION__, __LINE__, name);
      if (_wl_get_dev_type(name, dev_type, DEV_TYPE_LEN) >= 0 &&
            !strncmp(dev_type, "wl", 2))
      {
         //printf("#### %s:%d  it is wireless interface and devtype:%s ####\r\n", __FUNCTION__, __LINE__, dev_type);
         /* filter out virtual interface */
         if (!strstr(name,"."))
            wlif_num++;

         /* limit to the maximum */
         if (wlif_num >= WL_MAX_NUM_RADIO)
            break;
      }
   }
   fclose(fp);

out:
   return wlif_num;
#endif
}

/*
 * Return the number of 'dummy' wlan adapters in the system.
 * These are the DPD powered down adapters
 * Dynamically detect it at run time.
 */
int wlgetdummyintfNo(void)
{
#ifdef DESKTOP_LINUX
   return 0;
#else
   char proc_net_dev[] = "/proc/net/dev";
   FILE *fp;
   char buf[BUF_LENGTH], *c, *name;
   char dev_type[DEV_TYPE_LEN];
   int wlif_num=0;

   if (!(fp = fopen(proc_net_dev, "r")))
   {
      printf("Error reading %s!\n", proc_net_dev);
      goto out;
   }

   /* eat first two lines */
   if (!fgets(buf, sizeof(buf), fp) ||
         !fgets(buf, sizeof(buf), fp)) 
   {
      fclose(fp);
      goto out;
   }

   while (fgets(buf, sizeof(buf), fp))
   {
      c = buf;
      while (isspace(*c))
         c++;

      if (!(name = strsep(&c, ":")))
         continue;

      /* Consider only wl[X] interfaces */
      if (!strncmp(name, "wl", 2))
      {
         /* filter out virtual interface */
         if (!strstr(name,"."))
         {
            /* Only dummy interfaces */
            if (_wl_get_dev_type(name, dev_type, DEV_TYPE_LEN) >= 0 &&
                !strncmp(dev_type, "dummy", 5))
            {
               wlif_num++;
            }

            /* limit to the maximum */
            if (wlif_num >= WL_MAX_NUM_RADIO)
               break;
         }
      }
   }
   fclose(fp);

out:
   return wlif_num;
#endif
}

/* Return the maximum number of SSID numbers is supported on the system for a given wlan adapter */
#ifdef DESKTOP_LINUX
int wlgetVirtIntfNo( int idx __attribute__((unused)))
{
   return 4;
}
#else
int wlgetVirtIntfNo(int idx)
{
   char buf[BUF_LENGTH], cmd[CMD_LENGTH];
   FILE *fp;
   int num = 0; 

   if (wlgetintfNo() == 0)
       return 0;

   snprintf(cmd, CMD_LENGTH, "wl -i wl%d cap", idx);

   if ((fp = popen(cmd, "r")) == NULL)
   {
      fprintf(stderr, "Error opening pipe!\n");
      goto out;
   }

   if (fgets(buf, BUF_LENGTH, fp) != NULL)
   {
      if (strstr(buf, "1ssid"))
      {
         num= 1;
      } else if (strstr(buf, "mbss4"))
      {
         num= 4;
      } else if (strstr(buf, "mbss8"))
      {
         num= 8;

      } else if (strstr(buf, "mbss16"))
      {
         num= 16;
      }
      else
      {
         num = 0;
      }
   }

   pclose(fp);

out:
   /* limit to the maximum */
   return num > WL_MAX_NUM_SSID ? WL_MAX_NUM_SSID : num;
}
#endif

#ifdef DESKTOP_LINUX
void wlgetVer(const char *ifname __attribute__((unused)), char* version)
{
    if (version != NULL)
        sprintf(version, "17.10");
}
#else
void wlgetVer(const char *ifname, char* version)
{
    char *tag = "version ", *p;
    char buf[BUF_LENGTH], cmd[CMD_LENGTH];
    FILE *fp;

    if (wlgetintfNo() == 0)
    {
        sprintf(version, "N/A");
        return;
    }

    snprintf(cmd, CMD_LENGTH, "wl -i %s  ver", ifname);
    if ((fp = popen(cmd, "r")) == NULL)
    {
        fprintf(stderr, "Error opening pipe!\n");
        sprintf(version, "N/A");
        return;
    }

    if (fgets(buf, BUF_LENGTH, fp) == NULL) // skip the first line of output
    {
        sprintf(version, "N/A");
    } else if (fgets(buf, BUF_LENGTH, fp) != NULL)
    {
        p = strstr(buf, tag);
        if (p != NULL)
        {
            p += strlen(tag);
            sscanf(p, "%s", version);
        } else
            sprintf(version, "N/A");
    } else
    {
        sprintf(version, "N/A");
    }

    pclose(fp);
}
#endif

#ifdef DESKTOP_LINUX
void wlgetChannelList(const char *ifname,
                     char chanList[],
                     int size)
{
   memset(chanList, 0, size);
}
#else
void wlgetChannelList(const char *ifname,
                     char chanList[],
                     int size)
{
   char cmd[CMD_LENGTH];
   int i;
   FILE *fp;

   if (wlgetintfNo() == 0)
      return;

   memset(chanList, 0, size);

   snprintf(cmd, CMD_LENGTH, "wl -i %s channels", ifname);

   if ((fp = popen(cmd, "r")) == NULL)
   {
      fprintf(stderr, "Error opening pipe!\n");
      return;
   }

   fgets(chanList, size-1, fp);
   pclose(fp);

   for ( i = 0; i < size ; i++)
   {
      if (chanList[i] == '\n' || chanList[i] == '\0')
         break;

      if (chanList[i] == ' ')
         chanList[i] = ',';
   }

   if (i > 0  && chanList[i-1] == ',') // eliminate the last ',' 
      chanList[i-1] = '\0';
}
#endif

#ifdef DESKTOP_LINUX
void wlgetChanspec(const char* ifname __attribute__((unused)), 
      unsigned int *channel, 
      unsigned int *bandwidth);
{
   if (channel != NULL)
      *channel = 7;

   if (bandwidth != NULL)
      *bandwidth = 20;
}
#else
void wlgetChanspec(const char *ifname, unsigned int *channel, unsigned int *bandwidth)
{
   char cmd[CMD_LENGTH];
   char buf[BUF_LENGTH] = {0}, *ptr = NULL;
   FILE *fp;

   if (wlgetintfNo() == 0)
      return;

   snprintf(cmd, CMD_LENGTH, "wl -i %s chanspec", ifname);

   if ((fp = popen(cmd, "r")) == NULL)
   {
      fprintf(stderr, "Error opening pipe!\n");
      return;
   }

   ptr = fgets(buf, BUF_LENGTH-1, fp);
   pclose(fp);

   if (ptr)
   {
       if (strstr(buf, "6g") != NULL)
           ptr += 2; // skip "6g"
       if (sscanf(ptr, "%d", channel) != 1)
           return;

       if ((ptr = strstr(buf, "0x")) != NULL)
       {
           UINT32 chanspec = 0;

           if (sscanf(ptr, "0x%04x", &chanspec) != 1)
               return;
           if (CHSPEC_IS320(chanspec))
               *bandwidth = 320;
           else if (CHSPEC_IS160(chanspec))
               *bandwidth = 160;
           else if (CHSPEC_IS80(chanspec))
               *bandwidth = 80;
           else if (CHSPEC_IS40(chanspec))
               *bandwidth = 40;
           else
               *bandwidth = 20;
       }
   }
}
#endif

#ifdef DESKTOP_LINUX
int wlgetStationStats(const char* ifname __attribute__((unused)),
                      const char* sta_addr __attribute__((unused)),
                      Station_Stats *stats)
{
    if (!stats)
        return -1;

    bzero(stats, sizeof(Station_Stats));
    return 0;
}


int wlgetStationInfo(const char* ifname __attribute__((unused)),
                     const char* sta_addr __attribute__((unused)),
                     Station_Info *staInfo)
{
    if (!staInfo)
        return -1;

    bzero(staInfo, sizeof(Station_Info));
    return 0;
}


unsigned int wlgetRate(const char* ifname __attribute__((unused)))
{
   return 0;
}

int wlgetStationBSData(const char* ifname __attribute__((unused)),
                       int *data_buf, int size)
{
    memset(data_buf, 0, sizeof(int)*size);
    return 0;
}


void wlgetCapability(const char *ifname __attribute__((unused)),
                     char* cap, int size)
{
    if (cap != NULL)
        memset(cap, 0, sizeof(int)*size);
}


void wlgetBssStatus(const char* ifname __attribute__((unused)),
                    char* bssStatus, int size)
{
    if (bssStatus != NULL)
        memset(bssStatus, 0, sizeof(int)*size);
}

char* wlgetCurrentRateset(const char* ifname __attribute__((unused)),
                          char* rateset, int size)
{
   memset(rateset, 0, sizeof(int)*size);
   return NULL;
}

char* wlgetBasicRateset(const char* ifname __attribute__((unused)),
                        char* rateset, int size)
{
   memset(rateset, 0, sizeof(int)*size);
   return NULL;
}

char* wlgetSupportRateset(const char* ifname __attribute__((unused)),
                          char* rateset, int size)
{
   memset(rateset, 0, sizeof(int)*size);
   return NULL;
}

#else

int wlgetStationStats(const char* ifname, const char* sta_addr, Station_Stats *stats)
{
    char buf[BUF_LENGTH], cmd[CMD_LENGTH], *p;
    FILE *fp;

    if (!stats || wlgetintfNo() == 0)
        return -1;

    bzero(stats, sizeof(Station_Stats));
    if (ifname)
        snprintf(cmd, CMD_LENGTH, "wl -i %s sta_info %s", ifname, sta_addr);
    else
        snprintf(cmd, CMD_LENGTH, "wl sta_info %s", sta_addr);

    if ((fp = popen(cmd, "r")) == NULL)
    {
        fprintf(stderr, "Error opening pipe!\n");
        return -1;
    }

    while (fgets(buf, BUF_LENGTH, fp) != NULL)
    {
        // make sure each item would only be process once
        if (!stats->packetsSent && (p=strstr(buf, "tx total pkts: ")) != NULL)
            sscanf(p+15, "%llu", &stats->packetsSent);
        else if (!stats->bytesSent && (p=strstr(buf, "tx total bytes: ")) != NULL)
            sscanf(p+16, "%llu", &stats->bytesSent);
        else if (!stats->errorsSent &&  (p=strstr(buf, "tx failures: ")) != NULL)
            sscanf(p+13, "%u", &stats->errorsSent);
        else if (!stats->packetsReceived && (p=strstr(buf, "rx data pkts: ")) != NULL)
            sscanf(p+14, "%llu", &stats->packetsReceived);
        else if (!stats->bytesReceived && (p=strstr(buf, "rx data bytes: ")) != NULL)
            sscanf(p+15, "%llu", &stats->bytesReceived);
        else if (!stats->retransCount && (p=strstr(buf, "tx data pkts retried: ")) != NULL)
            sscanf(p+22, "%u", &stats->retransCount);
        else if (!stats->failedRetransCount && (p=strstr(buf, "tx pkts retry exhausted: ")) != NULL)
            sscanf(p+25, "%u", &stats->failedRetransCount);
        else if (!stats->retryCount &&  (p=strstr(buf, "tx pkts retries: ")) != NULL)
            sscanf(p+17, "%u", &stats->retryCount);
    }

    pclose(fp);
    return 0;
}


int wlgetStationInfo(const char* ifname, const char* sta_addr, Station_Info *staInfo)
{
    char buf[BUF_LENGTH], cmd[CMD_LENGTH], *p;
    FILE *fp;
    int signalStrength = 0;

    if (!staInfo || wlgetintfNo() == 0)
        return -1;

    bzero(staInfo, sizeof(Station_Info));
    if (ifname)
        snprintf(cmd, CMD_LENGTH, "wl -i %s sta_info %s", ifname, sta_addr);
    else
        snprintf(cmd, CMD_LENGTH, "wl sta_info %s", sta_addr);


    if ((fp = popen(cmd, "r")) == NULL)
    {
        fprintf(stderr, "Error opening pipe!\n");
        return -1;
    }

    while (fgets(buf, BUF_LENGTH, fp) != NULL)
    {
        // make sure each item would only be process once
        if (!staInfo->connectTime && (p=strstr(buf, "in network ")) != 0)
            sscanf(p+11, "%u", &staInfo->connectTime);
        else if (!staInfo->HTCaps && (p=strstr(buf, "HT caps ")) != 0)
            sscanf(p+8, "%x", &staInfo->HTCaps);
        else if (!staInfo->VHTCaps && (p=strstr(buf, "VHT caps ")) != 0)
            sscanf(p+9, "%x", &staInfo->VHTCaps);
        else if (!staInfo->HECaps && (p=strstr(buf, "HE caps ")) != 0)
            sscanf(p+8, "%x", &staInfo->HECaps);
        else if (!staInfo->txRate && (p=strstr(buf, "rate of last tx pkt: ")) != 0)
            sscanf(p+21, "%u", &staInfo->txRate);
        else if (!staInfo->rxRate && (p=strstr(buf, "rate of last rx pkt: ")) != 0)
            sscanf(p+21, "%u", &staInfo->rxRate);
        else if (!signalStrength && (p=strstr(buf, "smoothed rssi: ")) != 0)
            sscanf(p+15, "%d", &signalStrength);
        else if (!staInfo->max_nss && (p=strstr(buf, "Nss ")) != 0)
            sscanf(p+4, "%c", &staInfo->max_nss);
    }
    staInfo->signalStrength = abs(signalStrength);

    pclose(fp);
    return 0;
}


int wlgetStationBSData(const char* ifname, int *data_buf, int size)
{
    char buf[BUF_LENGTH], cmd[CMD_LENGTH];
    FILE *fp;

    if (!data_buf || wlgetintfNo() == 0)
        return -1;

    memset(data_buf, 0, sizeof(int)*size);
    if (ifname)
        snprintf(cmd, CMD_LENGTH, "wl -i %s bs_data", ifname);
    else
        snprintf(cmd, CMD_LENGTH, "wl bs_data");


    if ((fp = popen(cmd, "r")) == NULL)
    {
        fprintf(stderr, "Error opening pipe!\n");
        return -1;
    }

    //skip first line
    fgets(buf, BUF_LENGTH, fp);
    int i = 0;
    while (fgets(buf, BUF_LENGTH, fp) != NULL && i < size)
    {
        char mac[IF_NAME_SIZE] = {0}, sep;
        float phy, data_mbps, air_use, data_use;

        sscanf(buf, "%s %f %f %f%c %f", mac, &phy, &data_mbps, 
                                        &air_use, &sep, &data_use);
        data_buf[i++] = (int)data_use;
    }

    pclose(fp);
    return 0;
}


unsigned int wlgetRate(const char* ifname)
{
    char cmd[CMD_LENGTH];
    FILE *fp;
    unsigned int rate = 0;

    if (wlgetintfNo() == 0)
        return 0;

    snprintf(cmd, CMD_LENGTH, "wl -i %s rate", ifname);

    if ((fp = popen(cmd, "r")) == NULL)
    {
        fprintf(stderr, "Error opening pipe!\n");
        return 0;
    }

    if (fscanf(fp, "%u", &rate) != 1)
        rate = 0;

    pclose(fp);
    return rate;
}

void wlgetCapability(const char *ifname, char* cap, int size)
{
    char cmd[CMD_LENGTH];
    FILE *fp;

    if (cap == NULL || wlgetintfNo() == 0)
        return;

    snprintf(cmd, CMD_LENGTH, "wl -i %s cap", ifname);
    if ((fp = popen(cmd, "r")) == NULL)
    {
        fprintf(stderr, "Error opening pipe!\n");
        return;
    }

    fgets(cap, size-1, fp);
    pclose(fp);
}

void wlgetBssStatus(const char* ifname, char* bssStatus, int size)
{
    char cmd[CMD_LENGTH];
    FILE *fp;

    if (bssStatus == NULL || wlgetintfNo() == 0)
        return;

    snprintf(cmd, CMD_LENGTH, "wl -i %s bss", ifname);
    if ((fp = popen(cmd, "r")) == NULL)
    {
        fprintf(stderr, "Error opening pipe!\n");
        return;
    }

    fgets(bssStatus, size-1, fp);
    pclose(fp);
}

char* wlgetCurrentRateset(const char* ifname, char* rateset, int size)
{
    char cmd[CMD_LENGTH], buffer[BUF_LENGTH];
    FILE *fp;
    char *p, *savep, *token, *b;
    int num = 0;

    snprintf(cmd, CMD_LENGTH, "wl -i %s cur_rateset", ifname);
    if ((fp = popen(cmd, "r")) == NULL)
    {
        fprintf(stderr, "Error opening pipe!\n");
        return NULL;
    }

    // the first line is the transmit rate
    p = fgets(buffer, sizeof(buffer), fp);

    for ( ; ; p = NULL)
    {
       token = strtok_r(p, " ", &savep);
       if (token == NULL)
          break;

       if (!isdigit(*token)) // ignore non-digit token "[ ]"
          continue;

       if ((b=strstr(token, "(b)")) != NULL) // basic rate
          *b = '\0';

       if (num)
          strcat(rateset, ",");

       strcat(rateset, token);
       num ++;
    }

    pclose(fp);

    return (num == 0) ? NULL : rateset;
}

char* wlgetBasicRateset(const char* ifname, char* rateset, int size)
{
    char cmd[CMD_LENGTH], buffer[BUF_LENGTH];
    FILE *fp;
    char *p, *savep, *token, *b;
    int num = 0;

    snprintf(cmd, CMD_LENGTH, "wl -i %s rateset", ifname);
    if ((fp = popen(cmd, "r")) == NULL)
    {
        fprintf(stderr, "Error opening pipe!\n");
        return NULL;
    }

    // the first line is the transmit rate
    p = fgets(buffer, sizeof(buffer), fp);

    for ( ; ; p = NULL)
    {
       token = strtok_r(p, " ", &savep);
       if (token == NULL)
          break;

       if (!isdigit(*token)) // ignore non-digit token "[ ]"
          continue;

       if ((b=strstr(token, "(b)")) != NULL) // basic rate
       {
          *b = '\0';

          if (num)
             strcat(rateset, ",");

          strcat(rateset, token);
          num ++;
       }
    }

    pclose(fp);

    return (num == 0) ? NULL : rateset;
}

char* wlgetSupportRateset(const char* ifname, char* rateset, int size)
{
    char cmd[CMD_LENGTH], buffer[BUF_LENGTH];
    FILE *fp;
    char *p, *savep, *token, *b;
    int num = 0;

    snprintf(cmd, CMD_LENGTH, "wl -i %s rateset", ifname);
    if ((fp = popen(cmd, "r")) == NULL)
    {
        fprintf(stderr, "Error opening pipe!\n");
        return NULL;
    }

    // the first line is the transmit rate
    p = fgets(buffer, sizeof(buffer), fp);

    for ( ; ; p = NULL)
    {
       token = strtok_r(p, " ", &savep);
       if (token == NULL)
          break;

       if (!isdigit(*token)) // ignore non-digit token "[ ]"
          continue;

       if ((b=strstr(token, "(b)")) != NULL) // basic rate
          *b = '\0';

       if (num)
          strcat(rateset, ",");

       strcat(rateset, token);
       num ++;
    }

    pclose(fp);

    return (num == 0) ? NULL : rateset;
}
#endif
