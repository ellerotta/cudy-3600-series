/*
   Copyright (c) 2015 Broadcom
   All Rights Reserved

    <:label-BRCM:2015:DUAL/GPL:standard

    Unless you and Broadcom execute a separate written software license
    agreement governing use of this software, this software is licensed
    to you under the terms of the GNU General Public License version 2
    (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
    with the following added to such license:

       As a special exception, the copyright holders of this software give
       you permission to link this software with independent modules, and
       to copy and distribute the resulting executable under terms of your
       choice, provided that you also meet, for each linked independent
       module, the terms and conditions of the license of that module.
       An independent module is a module which is not derived from this
       software.  The special exception does not apply to any modifications
       of the software.

    Not withstanding the above, under no circumstances may you combine
    this software in any way with any other Broadcom software provided
    under a license other than the GPL, without Broadcom's express prior
    written consent.

:>
*/



/* This is an automated file. Do not edit its contents. */


#include "rdd_runner_tasks_auto.h"


char *image_task_names[7][16] = {
	{"IMAGE_0_DHD_TX_COMPLETE_0", "IMAGE_0_DHD_TX_COMPLETE_1", "IMAGE_0_DHD_TX_COMPLETE_2", "IMAGE_0_DHD_RX_COMPLETE_0", "IMAGE_0_DHD_RX_COMPLETE_1", "IMAGE_0_DHD_RX_COMPLETE_2", "", "", "IMAGE_0_PROCESSING0", "IMAGE_0_PROCESSING1", "IMAGE_0_PROCESSING2", "IMAGE_0_PROCESSING3", "IMAGE_0_PROCESSING4", "IMAGE_0_PROCESSING5", "IMAGE_0_PROCESSING6", "IMAGE_0_PROCESSING7"},
	{"IMAGE_1_GENERAL_TIMER", "IMAGE_1_CPU_RX", "IMAGE_1_CPU_RECYCLE", "IMAGE_1_DHD_TIMER", "IMAGE_1_DHD_TX_POST_UPDATE_FIFO", "IMAGE_1_DHD_TX_POST_0", "IMAGE_1_DHD_TX_POST_1", "IMAGE_1_DHD_TX_POST_2", "IMAGE_1_PROCESSING0", "IMAGE_1_PROCESSING1", "IMAGE_1_PROCESSING2", "IMAGE_1_PROCESSING3", "IMAGE_1_CPU_RX_COPY", "", "", ""},
	{"IMAGE_2_GENERAL_TIMER", "IMAGE_2_CPU_RECYCLE", "IMAGE_2_CPU_TX_0", "IMAGE_2_CPU_TX_1", "", "", "", "", "IMAGE_2_PROCESSING0", "IMAGE_2_PROCESSING1", "IMAGE_2_PROCESSING2", "IMAGE_2_PROCESSING3", "IMAGE_2_PROCESSING4", "IMAGE_2_PROCESSING5", "IMAGE_2_PROCESSING6", "IMAGE_2_PROCESSING7"},
	{"IMAGE_3_CPU_FPM_RINGS_AND_BUFMNG_REFILL", "IMAGE_3_GENERAL_TIMER", "IMAGE_3_SERVICE_QUEUES", "", "", "", "", "", "IMAGE_3_PROCESSING0", "IMAGE_3_PROCESSING1", "IMAGE_3_PROCESSING2", "IMAGE_3_PROCESSING3", "IMAGE_3_PROCESSING4", "IMAGE_3_PROCESSING5", "IMAGE_3_PROCESSING6", "IMAGE_3_PROCESSING7"},
	{"DS_TM_GENERAL_TIMER", "DS_TM_BUFFER_CONG_MGT", "DS_TM_REPORTING", "DS_TM_UPDATE_FIFO", "DS_TM_TX_TASK_0", "DS_TM_TX_TASK_1", "DS_TM_TX_TASK_2", "", "", "", "", "", "", "", "", ""},
	{"IMAGE_5_COMMON_REPROCESSING", "", "IMAGE_5_SPDSVC_GEN", "IMAGE_5_TCPSPDTEST_DOWNLOAD", "IMAGE_5_TCPSPDTEST_UPLOAD", "IMAGE_5_TCPSPDTEST_UPLOAD_TIMER", "", "", "IMAGE_5_PROCESSING0", "IMAGE_5_PROCESSING1", "IMAGE_5_PROCESSING2", "IMAGE_5_PROCESSING3", "IMAGE_5_PROCESSING4", "IMAGE_5_PROCESSING5", "IMAGE_5_PROCESSING6", "IMAGE_5_PROCESSING7"},
	{"US_TM_DIRECT_FLOW", "US_TM_GENERAL_TIMER", "US_TM_BUFFER_CONG_MGT", "", "US_TM_UPDATE_FIFO", "US_TM_EPON_UPDATE_FIFO", "US_TM_WAN", "US_TM_WAN_EPON", "US_TM_SPDSVC_ANALYZER", "", "", "", "", "", "", ""}
	};

