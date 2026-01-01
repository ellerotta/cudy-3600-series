/***********************************************************************
 *
 *  Copyright (c) 2021  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2021:proprietary:standard

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

#include <string.h>
#include "cms_log.h"
#include "rut_util.h"

typedef struct {
   const char *local;
   const char *iso8601;
} localTimeZoneNameMapping;

static const localTimeZoneNameMapping rutTimezone_Iso8601Mapping[] = {
   {"International_Date_Line_West",  "Etc/GMT-12"},
   {"Midway_Island",                 "Pacific/Midway"},
   {"Hawaii",                        "US/Hawaii"},
   {"Alaska",                        "US/Alaska"},
   {"Pacific_Time",                  "America/Tijuana"},
   {"Arizona",                       "US/Arizona"},
   {"Chihuahua",                     "America/Chihuahua"},
   {"Mountain_Time",                 "US/Mountain"},
   {"Central_Time",                  "US/Central"},
   {"Monterrey",                     "America/Monterrey"},
   {"Saskatchewan",                  "Canada/Saskatchewan"},
   {"Bogota",                        "America/Bogota"},
   {"Eastern_Time",                  "US/Eastern"},
   {"Indiana",                       "America/Indianapolis"},
   {"Atlantic_Time",                 "Canada/Atlantic"},
   {"Caracas",                       "America/Caracas"},
   {"Santiago",                      "America/Santiago"},
   {"Newfoundland",                  "Canada/Newfoundland"},
   {"Brasilia",                      "Brazil/East"},
   {"Buenos_Aires",                  "America/Buenos_Aires"},
   {"Greenland",                     "America/Godthab"},
   {"Noronha",                       "America/Noronha"},
   {"Azores",                        "Atlantic/Azores"},
   {"Cape_Verde_Is.",                "Atlantic/Cape_Verde"},
   {"Casablanca",                    "Africa/Casablanca"},
   {"GMT",                           "GMT"},
   {"Amsterdam",                     "Europe/Amsterdam"},
   {"Belgrade",                      "Europe/Belgrade"},
   {"Brussels",                      "Europe/Brussels"},
   {"Sarajevo",                      "Europe/Sarajevo"},
   {"Kinshasa",                      "Africa/Kinshasa"},
   {"Athens",                        "Europe/Athens"},
   {"Bucharest",                     "Europe/Bucharest"},
   {"Cairo",                         "Africa/Cairo"},
   {"Harare",                        "Africa/Harare"},
   {"Helsinki",                      "Europe/Helsinki"},
   {"Jerusalem",                     "Asia/Jerusalem"},
   {"Baghdad",                       "Asia/Baghdad"},
   {"Kuwait",                        "Asia/Kuwait"},
   {"Moscow",                        "Europe/Moscow"},
   {"Nairobi",                       "Africa/Nairobi"},
   {"Tehran",                        "Asia/Tehran"},
   {"Muscat",                        "Asia/Muscat"},
   {"Baku",                          "Asia/Baku"},
   {"Kabul",                         "Asia/Kabul"},
   {"Karachi",                       "Asia/Karachi"},
   {"Kolkata",                       "Asia/Kolkata"},
   {"Kathmandu",                     "Asia/Kathmandu"},
   {"Almaty",                        "Asia/Almaty"},
   {"Dhaka",                         "Asia/Dhaka"},
   {"Rangoon",                       "Asia/Rangoon"},
   {"Bangkok",                       "Asia/Bangkok"},
   {"Krasnoyarsk",                   "Asia/Krasnoyarsk"},
   {"Beijing",                       "Asia/Shanghai"},
   {"Irkutsk",                       "Asia/Irkutsk"},
   {"Kuala_Lumpur",                  "Asia/Kuala_Lumpur"},
   {"Perth",                         "Australia/Perth"},
   {"Taipei",                        "Asia/Taipei"},
   {"Tokyo",                         "Asia/Tokyo"},
   {"Seoul",                         "Asia/Seoul"},
   {"Yakutsk",                       "Asia/Yakutsk"},
   {"Adelaide",                      "Australia/Adelaide"},
   {"Darwin",                        "Australia/Darwin"},
   {"Brisbane",                      "Australia/Brisbane"},
   {"Canberra",                      "Australia/Canberra"},
   {"Guam",                          "Pacific/Guam"},
   {"Hobart",                        "Australia/Hobart"},
   {"Vladivostok",                   "Asia/Vladivostok"},
   {"Magadan",                       "Asia/Magadan"},
   {"Auckland",                      "Pacific/Auckland"},
   {"Fiji",                          "Pacific/Fiji"},
};

#define NUM_TZMAPPING_ENTRIES (sizeof(rutTimezone_Iso8601Mapping)/sizeof(localTimeZoneNameMapping))

const char *rutTimezone_lookupIso8601(const char* localTimeZoneName)
{
   unsigned int i;

   if (localTimeZoneName == NULL) {
      cmsLog_error("NULL input arg localTimeZoneName");
      return NULL;
   }

   for (i=0; i < NUM_TZMAPPING_ENTRIES; i++) {
      if (!strcmp(rutTimezone_Iso8601Mapping[i].local, localTimeZoneName))
         return rutTimezone_Iso8601Mapping[i].iso8601;
   }

   cmsLog_error("failed to map %s in /etc/zoneinfo/", localTimeZoneName);

   return NULL;
}

void rutTimezone_writeTzFiles(const char *iso8601TimeZoneName)
{
   char cmd[BUFLEN_1024] = {0};

   rut_doSystemAction("rut", "rm -f /var/localtime");

   snprintf(cmd, sizeof(cmd), "ln -s /etc/zoneinfo/%s /var/localtime", iso8601TimeZoneName);
   rut_doSystemAction("rut", cmd);

   memset(cmd, 0, sizeof(cmd));
   snprintf(cmd, sizeof(cmd), "echo %s > /var/timezone", iso8601TimeZoneName);
   rut_doSystemAction("rut", cmd);
}
