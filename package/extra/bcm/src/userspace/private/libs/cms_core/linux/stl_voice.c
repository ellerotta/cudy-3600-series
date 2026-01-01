/****************************************************************************
*
*  Copyright (c) 2007-2012 Broadcom Corporation
*
<:label-BRCM:2012:proprietary:standard

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
****************************************************************************
*
*  Filename: stl_voice.c
*
****************************************************************************
*  Description:
*
*
****************************************************************************/

#include "stl.h"
#include "cms.h"
#include "cms_msg.h"
#include "cms_util.h"
#include "cms_mem.h"
#include "mdm.h"
#include "rut_voice.h"

#ifdef BRCM_VOICE_SUPPORT
#ifndef CMS_LOG3
#define CMS_LOG3
#endif /* CMS_LOG3 */
#include <cms_log.h>

/* -------------- Private defines ---------------- */

#define STL_MSG_RECEIVE_TIMEOUT 2000
#define STL_MSG_RECEIVE_LIMIT   2


/* Note, those #define must match the ones present in vrgEndpt.h as they are
** used for offseting the information passed on the message from VOICE to STL.
**
** Long term, we should have only one #define for those used in all modules.
*/
#ifdef DMP_X_BROADCOM_COM_DECTENDPOINT_1
#   define DECT_DATA_ACCESS_CODE_LEN      4
#   define DECT_DATA_LINK_DATE_LEN        10
#   define DECT_DATA_HANDSET_STATUS_LEN   32
#   define DECT_DATA_IPEI_LEN             5
#endif

/* -------------- Functions ---------------- */

CmsRet stl_voiceObject(_VoiceObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceMsgReqObject(_VoiceMsgReqObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceSysAccessObject(_VoiceSysAccessObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCapObject(_VoiceCapObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCapSipObject(_VoiceCapSipObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCapMgcpObject(_VoiceCapMgcpObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#if 0
CmsRet stl_voiceCapH323Object(_VoiceCapH323Object *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

CmsRet stl_voiceCapCodecsObject(_VoiceCapCodecsObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceProfObject(_VoiceProfObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceProfProviderObject(_VoiceProfProviderObject *obj , const InstanceIdStack *iidStack )
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceProfSipObject(_VoiceProfSipObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_voiceProfSipSubscribeObject(_VoiceProfSipSubscribeObject *obj , const InstanceIdStack *iidStack )
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#if 0
CmsRet stl_voiceProfSipResponseObject(_VoiceProfSipResponseObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
CmsRet stl_voiceProfMgcpObject(_VoiceProfMgcpObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#if 0
CmsRet stl_voiceProfH323Object(_VoiceProfH323Object *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

CmsRet stl_voiceProfRtpObject(_VoiceProfRtpObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceProfRtpRtcpObject(_VoiceProfRtpRtcpObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceProfRtpSrtpObject(_VoiceProfRtpSrtpObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceProfRtpRedundancyObject(_VoiceProfRtpRedundancyObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#if 0
CmsRet stl_voiceProfLineNumberingPlanObject(_VoiceProfLineNumberingPlanObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceProfNumberingPlanPrefixInfoObject(_VoiceProfNumberingPlanPrefixInfoObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceProfToneObject(_VoiceProfToneObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceProfToneEventObject(_VoiceProfToneEventObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceProfToneDescriptionObject(_VoiceProfToneDescriptionObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceProfTonePatternObject(_VoiceProfTonePatternObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceProfButtonMapObject(_VoiceProfButtonMapObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceProfButtonMapButtonObject(_VoiceProfButtonMapButtonObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

CmsRet stl_voiceProfFaxT38Object(_VoiceProfFaxT38Object *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineSipObject(_VoiceLineSipObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#if 0
CmsRet stl_voiceLineSipEventSubscribeObject(_VoiceLineSipEventSubscribeObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceLineMgcpObject(_VoiceLineMgcpObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceLineH323Object(_VoiceLineH323Object *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceLineRingerObject(_VoiceLineRingerObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceLineRingerEventObject(_VoiceLineRingerEventObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceLineRingerDescriptionObject(_VoiceLineRingerDescriptionObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceLineRingerPatternObject(_VoiceLineRingerPatternObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

CmsRet stl_voiceLineCallingFeaturesObject(_VoiceLineCallingFeaturesObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineProcessingObject(_VoiceLineProcessingObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineCodecObject(_VoiceLineCodecObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineCodecListObject(_VoiceLineCodecListObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#if 0
CmsRet stl_voiceLineSessionObject(_VoiceLineSessionObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

CmsRet stl_voiceLineStatsObject(_VoiceLineStatsObject *obj, const InstanceIdStack *iidStack )
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineObject(_VoiceLineObject *obj, const InstanceIdStack *iidStack )
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voicePhyIntfObject(_VoicePhyIntfObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voicePhyIntfTestsObject(_VoicePhyIntfTestsObject *obj , const InstanceIdStack *iidStack )
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


//#ifdef DMP_X_BROADCOM_COM_PSTNENDPOINT_1
CmsRet stl_voicePstnObject(_VoicePstnObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
//#endif

#if defined( DMP_X_BROADCOM_COM_NTR_1 )
CmsRet stl_voiceNtrObject(_VoiceNtrObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceNtrHistoryObject(_VoiceNtrHistoryObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif /* DMP_X_BROADCOM_COM_NTR_1 */


#ifdef DMP_X_BROADCOM_COM_DECTENDPOINT_1

CmsRet stl_voiceDectSystemSettingObject( _VoiceDectSystemSettingObject *obj, const InstanceIdStack *iidStack )
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceDECTHandsetObject(_VoiceDECTHandsetObject *obj, const InstanceIdStack *iidStack)
{
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsMsgHeader msg = EMPTY_MSG_HEADER;
   CmsMsgHeader *reply = NULL;
   CmsRet ret;
   UBOOL8 receive = TRUE;
   int recvCount = 0;

   /* If stats object is being retreived by VOICE, no need to notify VOICE */
   if ( cmsMsg_getHandleGenericEid(msgHandle) != EID_DECT )
   {
      if ( rutIsDectRunning() )
      {
         msg.type = CMS_MSG_VOICE_DECT_HS_INFO_REQ;
         msg.src = cmsMsg_getHandleEid(msgHandle);
         msg.dst = EID_DECT;
         msg.flags_request = 1;
         msg.flags.bits.bounceIfNotRunning = 1;
         msg.wordData = obj->X_BROADCOM_COM_ID;

         /* Send message to VOICE asking for DECT information */
         if ((ret = cmsMsg_send(msgHandle, &msg)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not send DECT handset info message to Call Manager, ret= %d\n", ret);
            return ret;
         }

         /* Wait for reply containing DECT info block with timeout. If time out occurs, exit. If we
         ** receive other messages, we put them back in queue and increment counter. If we exceed the
         ** counter, we give up.
         */
         while ( receive )
         {
            if ( recvCount < STL_MSG_RECEIVE_LIMIT )
            {
               ret = cmsMsg_receiveWithTimeout(msgHandle, &reply, STL_MSG_RECEIVE_TIMEOUT);
               if (ret == CMSRET_SUCCESS)
               {
                  /* Check to see if this is the expected message from VOICE, else put it back in message queue */
                  if ( ( reply->type == CMS_MSG_VOICE_DECT_HS_INFO_RSP ) &&
                        ( reply->src == EID_DECT ) )
                  {
                     if ( reply->dataLength )
                     {
                        char *pData = (char *) ( reply + 1 );
                        char statusBuf[BUFLEN_32];
                        char *pStatus = NULL;

                        pStatus = (char *)( pData + BUFLEN_4 );

                        memset ( statusBuf, 0, sizeof( statusBuf ) );
                        memcpy(statusBuf, pStatus, BUFLEN_32);

                        /* Cleanup old strings from obj */
                        CMSMEM_REPLACE_STRING_FLAGS( obj->status,
                                                     statusBuf,
                                                     mdmLibCtx.allocFlags );


                        /* Free reply message and exit loop */
                        CMSMEM_FREE_BUF_AND_NULL_PTR(reply);
                        receive = FALSE;
                     }
                     else
                     {
                        /* We received an empty message back. In this case,
                           try to use the default values in MDM, if they exist. */
                        CMSMEM_FREE_BUF_AND_NULL_PTR(reply);
                        return CMSRET_SUCCESS_OBJECT_UNCHANGED;
                     }
                  }
                  else
                  {
                     /* Queue and wait again */
                     cmsMsg_putBack(msgHandle, &reply);
                     recvCount++;
                  }
               }
               else
               {
                  cmsLog_error("Error receiving DECT handset info message, ret= %d\n", ret);
                  return ret;
               }
            }
            else
            {
               ret = CMSRET_INTERNAL_ERROR;
               cmsLog_error("Error receiving DECT handset info message, not waiting anymore, ret= %d\n", ret);
               return ret;
            }
         }
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS( obj->status, "Disabled", mdmLibCtx.allocFlags );

         ret = CMSRET_SUCCESS;
      }
   }
   else
   {
      ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   return ret;
}

CmsRet stl_voiceLineNameObject(_VoiceLineNameObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineSettingObject(_VoiceLineSettingObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineIntrusionCallObject(_VoiceLineIntrusionCallObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineMultiCallModeObject(_VoiceLineMultiCallModeObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineMelodyObject(_VoiceLineMelodyObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineAttachedHandsetObject(_VoiceLineAttachedHandsetObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineDialingPrefixObject(_VoiceLineDialingPrefixObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceDectFirmwareVersionObject(_VoiceDectFirmwareVersionObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceDectHardwareVersionObject(_VoiceDectHardwareVersionObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceDectEEPROMVersionObject(_VoiceDectEEPROMVersionObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineCLIRObject(_VoiceLineCLIRObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceDectResetBaseObject(_VoiceDectResetBaseObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceDectClockMasterObject(_VoiceDectClockMasterObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceDectPinCodeObject(_VoiceDectPinCodeObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineVolumnObject(_VoiceLineVolumnObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineBlockedNumberObject(_VoiceLineBlockedNumberObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineDectLineIdObject(_VoiceLineDectLineIdObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceDectSupportListObject(_VoiceDectSupportListObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_voiceContactNameObject( _VoiceContactNameObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceContactFirstNameObject( _VoiceContactFirstNameObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceContactLineIdObject( _VoiceContactLineIdObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_voiceContactListObject( _VoiceContactListObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceContactMelodyObject( _VoiceContactMelodyObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceContactNumberObject( _VoiceContactNumberObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallTypeObject( _VoiceCallTypeObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallNameObject( _VoiceCallNameObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallNumberObject( _VoiceCallNumberObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallDateTimeObject( _VoiceCallDateTimeObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallLineNameObject( _VoiceCallLineNameObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallLineIdObject( _VoiceCallLineIdObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallNumberOfMissedCallsObject( _VoiceCallNumberOfMissedCallsObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallNewFlagObject( _VoiceCallNewFlagObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallListObject( _VoiceCallListObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#endif /* DMP_X_BROADCOM_COM_DECTENDPOINT_1 */

#endif /* BRCM_VOICE_SUPPORT */
