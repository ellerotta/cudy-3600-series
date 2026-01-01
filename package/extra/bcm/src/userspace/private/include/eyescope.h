#ifndef _EYESCOPE_H
#define _EYESCOPE_H

/*
  <:copyright-BRCM:2023:proprietary:standard
  
     Copyright (c) 2023 Broadcom 
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
*/

typedef enum {
    EYESCOPE_SCAN_FULL,
    EYESCOPE_SCAN_HORIZONTAL,
    EYESCOPE_SCAN_VERTICAL,
} eyescope_scan_type_t;

typedef struct eyescope_params_s {
    eyescope_scan_type_t scan_type;
    int depth;
    int phase_start;
    int phase_step;
    int phase_end;
    int voffset_start;
    int voffset_step;
    int voffset_end;
    int top;
    int bottom;
    float left;
    float right;
    float osrate;
    int x_size;
    int y_size;
    float **eye;
} eyescope_params_t;

#define ALARM_PERCENT           50
#define SCAN_MIN_INTERVAL_SEC   60
#define SCAN_DEFAULT_INTERVAL_SEC   3600
#define SCAN_INVALID_TIME_SEC   0x7FFFFFFF

#define EYE_EVENT_PATH          "/var/eye_event/"
#define EYE_GOOD_FILE           "goodeye"
#define EYE_BAD_FILE            "badeye"
#define EYE_ALARM_PERCENT_FILE  "alarm_percent"
#define EYE_MASK_PARAM_FILE     "eye_mask_param"
#define EYE_DETECTION_INTERVAL_FILE      "eye_detect_interval"
#define EYE_DETECTION_INTERVAL_FILE_VERIFY      "eye_detect_interval_verify"



#define EYE_GOOD_PATH_FILE              EYE_EVENT_PATH EYE_GOOD_FILE
#define EYE_BAD_PATH_FILE               EYE_EVENT_PATH EYE_BAD_FILE
#define EYE_ALARM_PERCENT_PATH_FILE     EYE_EVENT_PATH EYE_ALARM_PERCENT_FILE
#define EYE_MASK_PARAM_PATH_FILE        EYE_EVENT_PATH EYE_MASK_PARAM_FILE
#define EYE_DETECTION_INTERVAL_PATH_FILE      EYE_EVENT_PATH EYE_DETECTION_INTERVAL_FILE
#define EYE_DETECTION_INTERVAL_PATH_FILE_VERIFY      EYE_EVENT_PATH EYE_DETECTION_INTERVAL_FILE_VERIFY


int eyescope_scan(eyescope_params_t *params);
int eyescope_free(eyescope_params_t *params);
#endif
