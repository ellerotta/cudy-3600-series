/***********************************************************************
 * <:copyright-BRCM:2006:proprietary:standard
 *
 *    Copyright (c) 2006 Broadcom
 *    All Rights Reserved
 *
 *  This program is the proprietary software of Broadcom and/or its
 *  licensors, and may only be used, duplicated, modified or distributed pursuant
 *  to the terms and conditions of a separate, written license agreement executed
 *  between you and Broadcom (an "Authorized License").  Except as set forth in
 *  an Authorized License, Broadcom grants no license (express or implied), right
 *  to use, or waiver of any kind with respect to the Software, and Broadcom
 *  expressly reserves all rights in and to the Software and all intellectual
 *  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 *  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 *  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1. This program, including its structure, sequence and organization,
 *     constitutes the valuable trade secrets of Broadcom, and you shall use
 *     all reasonable efforts to protect the confidentiality thereof, and to
 *     use this information only in connection with your use of Broadcom
 *     integrated circuit products.
 *
 *  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 *     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
 *     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
 *     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
 *     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
 *     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
 *     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
 *     PERFORMANCE OF THE SOFTWARE.
 *
 *  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
 *     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 *     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
 *     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
 *     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
 *     LIMITED REMEDY.
 * :>
 ************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "cms.h"
#include "cms_util.h"
#include "cms_obj.h"   // for OGF_LOCAL_MDM_ONLY
#include "cms_mgm.h"
#include "cms_msg.h"
#include "cms_msg_remoteobj.h"
#include "mdm.h"
#include "cms_boardioctl.h"
#include "bcm_flashutil.h"
#include "cms_params_modsw.h"
#include "prctl.h"
#include "genutil_crc.h"
#if defined(SUPPORT_OPENPLAT)
#include "beep_common.h"
#endif

static CmsRet writeValidatedBufToConfigFlash(const char *buf, UINT32 len);
static CmsRet writeValidatedBufToConfigFlashCompName(const char *buf, UINT32 len, const char *compName);

/** Try really hard to get all locks.  Wait a long time if neccessary. */
#define MGM_LOCK_TIMEOUT (CMSLCK_MAX_HOLDTIME * 8)

#ifdef COMPRESSED_CONFIG_FILE

/** Compress the given buffer and add a compression header.
 *
 * @param buf (IN)     Buffer to be compressed.
 * @param len (IN/OUT) On entry into the function, len is the length of the buffer to
 *                     be compressed.  On successful exit, len is the length of the
 *                     compressed buffer that is returned.
 *
 * @return the compressed buffer.  Caller is responsible for freeing this buffer.
 */
static UINT8 *compressBuf(const char *buf, UINT32 *len)
{
   UINT32 uncompressedLen = *len;
   UINT32 outbufLen=(uncompressedLen * 3) / 2 + (COMPRESSED_CONFIG_HEADER_LENGTH);
   UINT8 *outbuf;
   LZWEncoderState *encoder=NULL;
   SINT32 rc;
   CmsRet ret;

   outbuf = cmsMem_alloc(outbufLen, ALLOC_ZEROIZE);
   if (outbuf == NULL)
   {
      cmsLog_error("could not allocate %d bytes for compressed buf", outbufLen);
      *len = 0;
      return NULL;
   }

   if ((ret = cmsLzw_initEncoder(&encoder,
                                 &(outbuf[COMPRESSED_CONFIG_HEADER_LENGTH]),
                                 outbufLen-COMPRESSED_CONFIG_HEADER_LENGTH)) != CMSRET_SUCCESS)
   {
      cmsLog_error("initEncoder failed, ret=%d", ret);
      cmsMem_free(outbuf);
      *len = 0;
      return NULL;
   }

   if ((rc = cmsLzw_encode(encoder, (UINT8 *)buf, *len)) < 0)
   {
      cmsLog_error("encode failed");
      cmsMem_free(outbuf);
      *len = 0;
      cmsLzw_cleanupEncoder(&encoder);
      return NULL;
   }
   else
   {
      *len = rc;
   }

   if ((rc = cmsLzw_flushEncoder(encoder)) < 0)
   {
      cmsLog_error("encode flush failed");
      cmsMem_free(outbuf);
      *len = 0;
      cmsLzw_cleanupEncoder(&encoder);
      return NULL;
   }
   else
   {
      *len += rc;
   }

   cmsLzw_cleanupEncoder(&encoder);

   cmsLog_notice("compressed len=%d uncompressed len=%d\n", *len, uncompressedLen);

   /*
    * now add a header to the config file so we know it is
    * compressed and how many compressed bytes there are.
    */
   sprintf((char *)outbuf, "%s%d>", COMPRESSED_CONFIG_HEADER, *len);

   *len += COMPRESSED_CONFIG_HEADER_LENGTH;

   return outbuf;
}

#endif /* COMPRESSED_CONFIG_FILE */


/** Add a CRC header to the config buffer, which may already contain a compression header.
 *
 * Do not calculate the CRC on the compression header.
 *
 * @param buf (IN)     Buffer containing the config file, but may contain the compress
 *                     header.  Do not calculate crc over the compression header.
 * @param len (IN/OUT) On entry into the function, len is the length of the entire buffer.
 *                     On successful exit, len is the length of the new buffer with
 *                     crc header.
 *
 * @return the compressed buffer.  Caller is responsible for freeing this buffer.
 */
static char *addCrcHeader(const char *buf, UINT32 *len)
{
   UINT32 crc;
   UINT32 newLen = *len + CRC_CONFIG_HEADER_LENGTH;
   char *newBuf;

   if ((newBuf = cmsMem_alloc(newLen, ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("could not allocate %d bytes for crc header buf", newLen);
      *len = 0;
      return NULL;
   }

   if (!cmsUtl_strncmp(buf, COMPRESSED_CONFIG_HEADER, strlen(COMPRESSED_CONFIG_HEADER)))
   {
      /* there is a compression header already. */

      /* copy over the compression header */
      memcpy(newBuf, buf, COMPRESSED_CONFIG_HEADER_LENGTH);


      /* calculate crc and insert crc header */
      crc = genUtl_getCrc32((UINT8 *) &(buf[COMPRESSED_CONFIG_HEADER_LENGTH]),
                            *len - COMPRESSED_CONFIG_HEADER_LENGTH,
                            CRC_INITIAL_VALUE);


      sprintf(&(newBuf[COMPRESSED_CONFIG_HEADER_LENGTH]), "%s0x%x>", CRC_CONFIG_HEADER, crc);

      /* copy over the compressed buffer */
      memcpy(&(newBuf[COMPRESSED_CONFIG_HEADER_LENGTH + CRC_CONFIG_HEADER_LENGTH]),
             &(buf[COMPRESSED_CONFIG_HEADER_LENGTH]),
             *len - COMPRESSED_CONFIG_HEADER_LENGTH);
   }
   else
   {
      /* there is no compression header */
      /* calculate crc and insert crc header */
      crc = genUtl_getCrc32((UINT8 *) buf, *len, CRC_INITIAL_VALUE);

      sprintf(newBuf, "%s0x%x>", CRC_CONFIG_HEADER, crc);

      /* copy over the real config buffer */
      memcpy(&(newBuf[CRC_CONFIG_HEADER_LENGTH]), buf, *len);
   }

   *len += CRC_CONFIG_HEADER_LENGTH;

   return newBuf;
}


CmsRet cmsMgm_saveConfigToFlash(void)
{
   return (cmsMgm_saveConfigToFlashEx(0));
}

CmsRet cmsMgm_saveConfigToFlashEx(UINT32 flags)
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_notice("Entered: flags=0x%x", flags);

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   // If caller did not tell us OGF_LOCAL_MDM_ONLY, and
   // this component is capable of controlling remote components, then tell
   // all other remote components to save their config.
   if (((flags & OGF_LOCAL_MDM_ONLY) == 0) &&
       (cmsMdm_isRemoteCapable()))
   {
      ret = cmsMgm_saveRemoteConfigToFlash();
   }

   // Do not save local config if remote config file save failed.
   if (ret == CMSRET_SUCCESS)
   {
      ret = cmsMgm_saveLocalConfigToFlash();
   }

   cmsLog_notice("Exit: ret=%d", ret);
   return ret;
}

CmsRet cmsMgm_saveLocalConfigToFlash(void)
{
   CmsRet ret;
   char *buf;
   UINT32 len;
   static UBOOL8 deleteOtherConfigFileDone = FALSE;  // only need to do once.
   UBOOL8 deleteOtherConfigFile            = FALSE;

   if (mdmShmCtx->flags & MDM_SHMCTX_NON_PERSISTENT_CONFIG)
   {
      cmsLog_notice("NON_PERSISTENT_CONFIG flag set, do not write out config file");
      return CMSRET_SUCCESS;
   }

   cmsLog_notice("Entered:");

   if ((len = cmsImg_getConfigFlashSize()) == 0)
   {
      return CMSRET_INTERNAL_ERROR;
   }

   if ((buf = cmsMem_alloc(len, 0)) == NULL)
   {
      cmsLog_error("malloc of %d bytes failed", len);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   if ((ret = lck_autoLockAllZonesWithBackoff(0, MGM_LOCK_TIMEOUT, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(buf);
      cmsLck_dumpInfo();
      return ret;
   }

   if ((ret = mdm_serializeToBuf(buf, &len)) == CMSRET_SUCCESS)
   {
      /*
       * The buffer must contain a valid config because it came straight from the MDM,
       * so just write it out.
       */
      cmsLog_debug("writing serialized buf (len=0x%x) to config flash", len);
      ret = writeValidatedBufToConfigFlash(buf, len);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(buf);

   lck_autoUnlockAllZones(__FUNCTION__);


   // Internal SQA often switches between CMS and BDK images.  To avoid any
   // left over config state (especially involving bootFirmwareImage) which
   // creates unexpected boot state, delete the config file from the "other"
   // mode.  See also similar code when we upload config files in oal_event.c
   // Additional complication when running in BDK mode: SQA also likes to 
   // upload a CMS monolithic config file for use in BDK mode.  So we cannot
   // delete the monolithic config file until we know all the BDK components
   // have started and read the monolithic config file.  If OpenPlat is defined,
   // then OpenPlat is the last component.  Otherwise, sysmgmt is the last
   // component.  mdm_init will always call cmsMgm_saveLocalConfigToFlash
   // when it is done, so this function is a good indicator that the component
   // has finished initializing.
#ifdef SUPPORT_OPENPLAT
   if (!strcmp(mdmShmCtx->compName, BDK_COMP_OPENPLAT))
   {
      deleteOtherConfigFile = TRUE;
   }
#else
   
   if (!strcmp(mdmShmCtx->compName, BDK_COMP_SYSMGMT) ||
       cmsMdm_isCmsClassic())
   {
      deleteOtherConfigFile = TRUE;
   }
#endif

   cmsLog_debug("compName=%s (deleteOther=%d deleteOnce=%d)",
                mdmShmCtx->compName,
                deleteOtherConfigFile, deleteOtherConfigFileDone);
   if (deleteOtherConfigFile && !deleteOtherConfigFileDone)
   {
      deleteOtherConfigFileDone = TRUE;
      if (cmsMdm_isCmsClassic())
      {
         char cmd[BUFLEN_128]={0};
         cmsLog_notice("In CMS mode, delete BDK config files...");
         snprintf(cmd, sizeof(cmd), "rm -f %s_*", PSI_FILE_NAME);  // BDK psi_*
         prctl_runCommandInShellBlocking(cmd);
         snprintf(cmd, sizeof(cmd), "rm -f %s_*", PSI_BACKUP_FILE_NAME);
         prctl_runCommandInShellBlocking(cmd);
      }
      else
      {
         char cmd[BUFLEN_128]={0};
         cmsLog_notice("In BDK mode, delete CMS config files...");
         snprintf(cmd, sizeof(cmd), "rm -f %s", PSI_FILE_NAME);  // CMS psi
         prctl_runCommandInShellBlocking(cmd);
         snprintf(cmd, sizeof(cmd), "rm -f %s", PSI_BACKUP_FILE_NAME);
         prctl_runCommandInShellBlocking(cmd);
      }
   }

   return ret;
}

CmsRet cmsMgm_saveRemoteConfigToFlash(void)
{
    CmsRet ret = CMSRET_SUCCESS;
    CmsMsgHeader request = EMPTY_MSG_HEADER;
    CmsMsgHeader *response = NULL;
    void *msgHandle = cmsMdm_getThreadMsgHandle();

   cmsLog_notice("Entered:");

   if (mdmShmCtx->flags & MDM_SHMCTX_NON_PERSISTENT_CONFIG)
   {
      cmsLog_notice("NON_PERSISTENT_CONFIG flag set, do not write out config file");
      return CMSRET_SUCCESS;
   }

    request.src = cmsMsg_getHandleEid(msgHandle);
    request.dst = EID_REMOTE_OBJD;
    request.type = CMS_MSG_REMOTE_OBJ_SAVE_CONFIG;
    request.dataLength = 0;
    request.flags_request = 1;
    ret = cmsMsg_sendAndGetReplyBufWithTimeout(msgHandle,
                                               &request, &response, 10000);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not send to remote_objd!");
    }
    else
    {
        ret = response->wordData;
    }
    cmsMem_free(response);

    cmsLog_notice("Exit: ret=%d", ret);
    return ret;
}


// Warning: this function only works for monolithic MDM.  It is rarely used,
// so there is no remote MDM capable version yet.
CmsRet cmsMgm_writeConfigToBuf(char *buf, UINT32 *len)
{
   CmsRet ret;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if ((ret = lck_autoLockAllZonesWithBackoff(0, MGM_LOCK_TIMEOUT, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   ret = mdm_serializeToBuf(buf, len);

   lck_autoUnlockAllZones(__FUNCTION__);

   return ret;
}


// Warning: this function only works for monolithic MDM.  New code should use
// cmsMgm_writeMdmToBufEx() which handles both monolithic and distributed MDM.
CmsRet cmsMgm_writeMdmToBuf(char *buf, UINT32 *len)
{
   CmsRet ret;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if (buf == NULL || len == NULL)
   {
      cmsLog_error("NULL input args %p/%p", buf, len);
      return CMSRET_INVALID_ARGUMENTS;
   }

   if ((ret = lck_autoLockAllZonesWithBackoff(0, MGM_LOCK_TIMEOUT, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   mdmLibCtx.dumpAll = TRUE;

   ret = mdm_serializeToBuf(buf, len);

   mdmLibCtx.dumpAll = FALSE;

   lck_autoUnlockAllZones(__FUNCTION__);

   return ret;
}

CmsRet cmsMgm_writeMdmToBufEx(char **buf, UINT32 flags)
{
   MdmInfo mInfo;
   CmsMemStats memStats;
   UINT32 localLen = 0;
   UINT32 localBufLen = 0;
   UINT32 remoteBufLen = 0;
   char *tmpBuf = NULL;
   CmsMsgHeader *respMsg = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_notice("Entered: flags=0x%x", flags);

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if (buf == NULL)
   {
      cmsLog_error("NULL buf ptr");
      return CMSRET_INVALID_ARGUMENTS;
   }

   // First get all the remote MDM's from remote_objd.
   if (((flags & OGF_LOCAL_MDM_ONLY) == 0) &&
       (cmsMdm_isRemoteCapable()))
   {
      ret = cmsMgm_readRemoteMdmToBuf((void**)&respMsg);
      if ((ret == CMSRET_SUCCESS) &&
          (respMsg != NULL) &&
          (respMsg->dataLength > 0))
      {
         // Set remoteBufLen, which will indicate there is remote data to copy
         remoteBufLen = respMsg->dataLength;
      }
      else
      {
         /* may or may not be an error: maybe there is no component mdm */
         cmsLog_notice("cmsMgm_readRemoteMdmToBuf return %d, respMsg %p",
                       ret, respMsg);
      }
   }

   // TODO: Should we detect error while getting remote MDM and return now?

   // Now get the local MDM.
   // Calculate the buffer for the local MDM.  It consists of 2 parts:
   // the MDM structure itself (mallocBegin - shmBegin) + dynamically allocated
   // shared mem (shmBytesAllocd).
   cmsMdm_getInfo(&mInfo);
   cmsMem_getStats(&memStats);
   localBufLen = ((UINT32)(mInfo.mallocBegin - mInfo.shmBegin)) + memStats.shmBytesAllocd;

   // string representation is bigger than binary, so multiply by 3
   localBufLen = localBufLen * 3;

   cmsLog_debug("allocating local %d + remote %d bytes for MDM dump",
                localBufLen, remoteBufLen);
   tmpBuf = cmsMem_alloc(localBufLen + remoteBufLen, ALLOC_ZEROIZE);
   if (tmpBuf == NULL)
   {
      cmsLog_error("failed to allocate %d bytes", localBufLen + remoteBufLen);
      CMSMEM_FREE_BUF_AND_NULL_PTR(respMsg);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   if ((ret = lck_autoLockAllZonesWithBackoff(0, MGM_LOCK_TIMEOUT, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(tmpBuf);
      CMSMEM_FREE_BUF_AND_NULL_PTR(respMsg);
      return ret;
   }

   mdmLibCtx.dumpAll = TRUE;

   localLen = localBufLen;
   ret = mdm_serializeToBuf(tmpBuf, &localLen);

   mdmLibCtx.dumpAll = FALSE;

   lck_autoUnlockAllZones(__FUNCTION__);

   if (ret == CMSRET_SUCCESS)
   {
      // Local MDM is already in tmpBuf[0 - localLen].  Append remote MDMs if
      // there are any.
      if (remoteBufLen > 0)
      {
         // We seem to be trying to handle case where there is no local MDM
         // (localLen == 0), but not sure if that can ever happen.
         // Normally, localLen > 0, so we start the copy at buf+localLen-1.
         memcpy(localLen ? (tmpBuf+localLen-1) : tmpBuf, (char *)(respMsg+1),
                                                         remoteBufLen);
      }

      // Finally, give our buffer to caller.
      *buf = tmpBuf;
   }
   else
   {
      // we did not give our buffer to caller due to error, free it.
      cmsLog_error("mdm_serialzeToBuf failed, ret=%d localBufLen=%d",
                   ret, localBufLen);
      CMSMEM_FREE_BUF_AND_NULL_PTR(tmpBuf);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(respMsg);
   cmsLog_debug("Exit: ret=%d buf=%p", ret, *buf);
   return ret;
}


CmsRet cmsMgm_writeObjectToBuf(const MdmObjectId oid, char *buf, UINT32 *len)
{
   CmsRet ret;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if ((ret = lck_autoLockZone(oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   mdmLibCtx.dumpAll = TRUE;

   ret = mdm_serializeObjectToBuf(oid, buf, len);

   mdmLibCtx.dumpAll = FALSE;

   lck_autoUnlockZone(oid, __FUNCTION__);

   return ret;
}

#if defined(SUPPORT_OPENPLAT)
static void umountBeepDUStorage(void)
{
   FILE *fd;
   char line[2000];
   char dev[BUFLEN_128], mountPoint[BUFLEN_512], fsType[BUFLEN_32];

   /*
    * /proc/mounts is a dynamic file while operating umount. So copy the file to a temporary
    * file so that we can ensure all /local/ directories are umounted
    */
   prctl_runCommandInShellBlocking("cp /proc/mounts /var/mountsinfo");

   fd = fopen("/var/mountsinfo", "r");
   if (fd == NULL)
   {
      cmsLog_error("Failed to Open /var/mountsinfo !!");
      return;
   }

   /* find mount point "/local" */
   while (fgets(line, 2000, fd))
   {
      if (sscanf(line, "%s %s %s", dev, mountPoint, fsType) == 3)
      {
         /* umount all under /local/
          * For examples:
          * /dev/loop0 /local/modsw/tr157du/Broadcom/ExampleEE2-1.0/lxc/rootfs ext4 rw,relatime 0 0
          * tmpfs /local/modsw/tr157du/Broadcom/ExampleEE2-1.0/lxc/rootfs/var tmpfs rw,relatime 0 0
          * tmpfs /local/modsw/tr157du/Broadcom/ExampleEE2-1.0/lxc/rootfs/tmp tmpfs rw,relatime 0 0
          */
         if (strstr(mountPoint, CMS_DATA_STORAGE_DIR"/"))
         {
            sprintf(line, "umount -l %s 2>/dev/null", mountPoint);
            prctl_runCommandInShellBlocking(line);
         }
      }
   }

   fclose(fd);
}
#endif


void cmsMgm_invalidateConfigFlash(void)
{
   cmsMgm_invalidateConfigFlashEx(0);
   return;
}

void cmsMgm_invalidateConfigFlashEx(UINT32 flags)
{
   char cmd[BUFLEN_256]={0};
   CmsRet ret;

   cmsLog_notice("Entered, flags=%d", flags);

   CHECK_MDM_EXTERNAL_CALLER_V(__FUNCTION__);

   if ((ret = lck_autoLockAllZonesWithBackoff(0, MGM_LOCK_TIMEOUT, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      return;
   }

   if (cmsMdm_isCmsClassic() ||
       cmsMdm_isBdkWifi())
   {
      unlink("/data/.kernel_nvram.setting");
   }

   cmsLog_notice("invalidating primary config flash");
   if (cmsMdm_isCmsClassic())
   {
      snprintf(cmd, sizeof(cmd), "rm -f /data/psi");  // CMS monolitic config file (PSI_FILE_NAME)
      prctl_runCommandInShellBlocking(cmd);

      // Even if we are in CMS classic mode, we want to delete any
      // Distributed MDM config files in /data.  That way we start with a
      // clean slate.
      snprintf(cmd, sizeof(cmd), "rm -f /data/psi_*"); // BDK Distributed MDM config files
      prctl_runCommandInShellBlocking(cmd);

      // Remove the lease file managed by udhcpd so that udhcpd can start from scratch
      snprintf(cmd, sizeof(cmd), "rm -f /data/udhcpd.leases");
      prctl_runCommandInShellBlocking(cmd);
   }
   else
   {
      // BDK mode: delete our own config file.
      int rc;
      snprintf(cmd, sizeof(cmd), "/data/psi_%s", mdmShmCtx->compName);
      // Use unlink instead of prctl_runCommandInShellBlocking as workaround for SWBCACPE-51136.
      rc = unlink(cmd);
      if (rc < 0)
      {
         cmsLog_error("unlink of %s failed, errno=%d", cmd, errno);
         // print the error but keep going.
      }
   }

#ifdef SUPPORT_BACKUP_PSI
#ifdef SUPPORT_BACKUP_PSI_MIRROR_MODE
   /*
    * Only invalidate the backup if we are in Mirror Mode.  Otherwise,
    * the backup psi contains the per-device defaults which we want to
    * preserve.  (SUPPORT_BACKUP_PSI_DEVICE_DEFAULT)
    */
   cmsLog_notice("invalidating backup config flash");
   snprintf(cmd, sizeof(cmd), "rm -f /data/psibackup*");  // PSI_BACKUP_FILE_NAME
   prctl_runCommandInShellBlocking(cmd);
#endif
#endif


#if defined(SUPPORT_OPENPLAT)
   if (cmsMdm_isBdkOpenPlat())
   {
      /* umount DU storage */
      umountBeepDUStorage();

      /* delete all busgate config files.
       * Note: busgate config files must be deleted before passwd and group files.
       * Otherwise, dbus daemon may crash.
       */
      snprintf(cmd, sizeof(cmd), "rm -rf %s/dbus-1/system.d/*", CMS_DATA_STORAGE_DIR);
      prctl_runCommandInShellBlocking(cmd);

      /* delete /local/modsw/tmp and /local/modsw/tr157du */
      snprintf(cmd, sizeof(cmd), "rm -rf %s %s", CMS_MODSW_TMP_DIR, CMS_MODSW_DU_DIR);
      prctl_runCommandInShellBlocking(cmd);

      /* delete passwd and group files. */
      snprintf(cmd, sizeof(cmd), "rm -rf %s/busgate/*", CMS_DATA_STORAGE_DIR);
      prctl_runCommandInShellBlocking(cmd);

      /* delete openwrt/merged_rootfs/share */
      snprintf(cmd, sizeof(cmd), "rm -rf %s/openwrt/merged_rootfs/share", CMS_DATA_STORAGE_DIR);
      prctl_runCommandInShellBlocking(cmd);

      /*
       * CMS_DATA_STORAGE_DIR/openwrt/merged_rootfs is already unmounted
       * in umountBeepDUStorage
       */

      /* delete all openwrt files */
      snprintf(cmd, sizeof(cmd), "rm -rf %s/openwrt", CMS_DATA_STORAGE_DIR);
      prctl_runCommandInShellBlocking(cmd);
   }
#endif

#if defined(DMP_X_BROADCOM_COM_BASD_1)
   snprintf(cmd, sizeof(cmd), "rm -rf /data/bas");
   prctl_runCommandInShellBlocking(cmd);
#endif

   lck_autoUnlockAllZones(__FUNCTION__);

   // Do remote operations outside of the lock.
   // If caller did not tell us OGF_LOCAL_MDM_ONLY, and
   // this component is capable of controlling remote components, then tell
   // all other remote components to invalidate their config.
   if (((flags & OGF_LOCAL_MDM_ONLY) == 0) &&
       (cmsMdm_isRemoteCapable()))
   {
      ret = cmsMgm_invalidateRemoteConfigFlash();
      if (ret != CMSRET_SUCCESS)
      {
          cmsLog_error("Failed to reset default config in remote MDs.");
      }
   }

   if (cmsMdm_isBdkSysmgmt())
   {
      // Also delete the CMS classic monolithic config file (to be consistent
      // with the behavior above)
      snprintf(cmd, sizeof(cmd), "rm -f /data/psi");  // CMS monolitic config file (PSI_FILE_NAME)
      prctl_runCommandInShellBlocking(cmd);

      // Remove the lease file managed by udhcpd so that udhcpd can start from scratch
      snprintf(cmd, sizeof(cmd), "rm -f /data/udhcpd.leases");
      prctl_runCommandInShellBlocking(cmd);
   }

   cmsLog_notice("Exit");
   return;
}


CmsRet cmsMgm_invalidateRemoteConfigFlash(void)
{
    CmsRet ret = CMSRET_SUCCESS;
    CmsMsgHeader request = EMPTY_MSG_HEADER;
    CmsMsgHeader *response = NULL;
    void *msgHandle = cmsMdm_getThreadMsgHandle();

    request.src = cmsMsg_getHandleEid(msgHandle);
    request.dst = EID_REMOTE_OBJD;
    request.type = CMS_MSG_REMOTE_OBJ_INVALIDATE_CONFIG;
    request.dataLength = 0;
    request.flags_request = 1;
    ret = cmsMsg_sendAndGetReplyBufWithTimeout(msgHandle,
                                               &request, &response, 10000);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not send to remote_objd!");
    }
    else
    {
        ret = response->wordData;
    }
    cmsMem_free(response);
    return ret;
}


// Warning: this version only handles CMS Monolithic MDM.  New code should
// use cmsMgm_readConfigFlashToBufEx(), which handles both monolithic and
// distribued MDM.
CmsRet cmsMgm_readConfigFlashToBuf(char *buf, UINT32 *len)
{
   UINT32 localLen = 0;
   CmsRet ret;

   if (buf == NULL || len == NULL)
   {
      cmsLog_error("NULL input args %p/%p", buf, len);
      return CMSRET_INVALID_ARGUMENTS;
   }

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if ((ret = lck_autoLockAllZonesWithBackoff(0, MGM_LOCK_TIMEOUT, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   localLen = *len;
   ret = oal_readConfigFlashToBuf(CMS_CONFIG_PRIMARY, buf, &localLen);
   cmsLog_debug("oal_readConfigFlashToBuf primary returned %d len=0x%x", ret, localLen);

#ifdef SUPPORT_BACKUP_PSI
   if ((ret != CMSRET_SUCCESS) || (localLen == 0))
   {
      localLen = *len;
      ret = oal_readConfigFlashToBuf(CMS_CONFIG_BACKUP, buf, &localLen);
      cmsLog_debug("oal_readConfigFlashToBuf backup returned %d len=0x%x", ret, localLen);
   }
#endif

   lck_autoUnlockAllZones(__FUNCTION__);

   if (ret == CMSRET_SUCCESS)
   {
      // report actual length of buffer to caller.
      *len = localLen;
   }

   return ret;
}


CmsRet cmsMgm_readConfigFlashToBufEx(char **buf, UINT32 flags)
{
   UINT32 localLen = 0;
   UINT32 localBufLen = 0;
   UINT32 remoteBufLen = 0;
   char *tmpBuf = NULL;
   CmsMsgHeader *respMsg = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_notice("Entered: flags=0x%x", flags);

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if (buf == NULL)
   {
      cmsLog_error("NULL buf ptr");
      return CMSRET_INVALID_ARGUMENTS;
   }

   // First read the config data from all the remote MDM's.
   if (((flags & OGF_LOCAL_MDM_ONLY) == 0) &&
       (cmsMdm_isRemoteCapable()))
   {
      ret = cmsMgm_readRemoteConfigFlashToBuf((void**)&respMsg);
      if ((ret == CMSRET_SUCCESS) && (respMsg != NULL))
      {
         if (respMsg->dataLength > 0)
         {
            // Set remoteBufLen, indicating there is remote data to copy
            remoteBufLen = respMsg->dataLength;
         }
      }
      else
      {
         cmsLog_error("cmsMgm_readRemoteMdmToBuf returned %d, respMsg %p",
                       ret, respMsg);
         return CMSRET_INTERNAL_ERROR;
      }
   }

   // Now get the config from the local MDM.
   // Figure out the size needed for the local buffer, but allocate a buffer
   // big enough to hold the local and all the remote config bufs.
   localBufLen = cmsImg_getConfigFlashSize();
   if (localBufLen == 0)
   {
      cmsLog_error("cmsImg_getConfigFlashSize failed!");
      CMSMEM_FREE_BUF_AND_NULL_PTR(respMsg);
      return CMSRET_INTERNAL_ERROR;
   }

   cmsLog_debug("allocating local %d + remote %d bytes for config dump",
                localBufLen, remoteBufLen);
   tmpBuf = cmsMem_alloc(localBufLen + remoteBufLen, ALLOC_ZEROIZE);
   if (tmpBuf == NULL)
   {
      cmsLog_error("failed to allocate %d bytes", localBufLen + remoteBufLen);
      CMSMEM_FREE_BUF_AND_NULL_PTR(respMsg);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   // Put the local config buf at the beginning
   localLen = localBufLen;
   ret = cmsMgm_readConfigFlashToBuf(tmpBuf, &localLen);
   if (ret == CMSRET_SUCCESS)
   {
      // Local config is already in tmpBuf[0 - localLen].  Append remote MDMs
      // if there are any.
      if (remoteBufLen > 0)
      {
         // We seem to be trying to handle case where there is no local MDM
         // (localLen == 0), but not sure if that can ever happen.
         // Normally, localLen > 0, so we start the copy at buf+localLen-1.
         cmsLog_debug("actual localLen=%d", localLen);
         memcpy(localLen ? (tmpBuf+localLen-1) : tmpBuf, (char *)(respMsg+1),
                                                         remoteBufLen);
      }

      // Finally, give our buffer to caller.
      *buf = tmpBuf;
   }
   else
   {
      // we did not give our buffer to caller due to error, free it.
      cmsLog_error("mdm_readConfigToFlash failed, ret=%d localBufLen=%d",
                   ret, localBufLen);
      CMSMEM_FREE_BUF_AND_NULL_PTR(tmpBuf);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(respMsg);
   cmsLog_debug("Exit: ret=%d buf=%p", ret, *buf);

   return ret;
}

CmsRet cmsMgm_readRemoteConfigFlashToBuf(void **response)
{
    CmsRet ret = CMSRET_SUCCESS;
    CmsMsgHeader msg = EMPTY_MSG_HEADER;
    void *msgHandle = cmsMdm_getThreadMsgHandle();

    msg.src = cmsMsg_getHandleEid(msgHandle);
    msg.dst = EID_REMOTE_OBJD;
    msg.type = CMS_MSG_REMOTE_OBJ_READ_CONFIG;
    msg.flags_request = 1;
    
    ret = cmsMsg_sendAndGetReplyBufWithTimeout(msgHandle,
                                      &msg, (CmsMsgHeader**)response, 10000);
    if (ret != CMSRET_SUCCESS)
    {
       cmsLog_error("Could not send to remote_objd %d!",ret);
    }

    return ret;
}

CmsRet writeValidatedBufToConfigFlash(const char *buf, UINT32 len)
{
   return (writeValidatedBufToConfigFlashCompName(buf, len, mdmShmCtx->compName));
}

CmsRet writeValidatedBufToConfigFlashCompName(const char *buf, UINT32 len,
                                              const char *compName)
{
   char *buf2=NULL;
   char *configBuf=NULL;
   char *crcBuf=NULL;
   CmsMsgHeader msg = EMPTY_MSG_HEADER;
   void *msgHandle = NULL;
   CmsRet ret = CMSRET_INTERNAL_ERROR;

   /*
    * In the flash, the config file must end with a 0 byte.
    * When I send out the config file, I always include a 0 byte, but when the
    * config file comes back into CMS, it might have gotten the 0 byte
    * stripped off.  So check for that, and append a 0 byte if neccessary.
    */
   if (buf[len-1] != 0)
   {
      cmsLog_debug("final zero byte not detected, adding it.");
      if ((buf2 = cmsMem_alloc(len+1, ALLOC_ZEROIZE)) == NULL)
      {
         cmsLog_error("realloc of %u bytes for config buf failed", len);
         return CMSRET_RESOURCE_EXCEEDED;
      }
      memcpy(buf2, buf, len);
      len++;
      configBuf = buf2;
   }
   else
   {
      cmsLog_debug("final zero byte detected, do nothing.");
      configBuf = (char *) buf;
   }

#ifdef COMPRESSED_CONFIG_FILE
   configBuf = (char *) compressBuf(configBuf, &len);
#endif

   crcBuf = addCrcHeader(configBuf, &len);

   // Write to filesystem instead of calling a board ioctl which then writes
   // to the filesystem.
   {
      char tmpName[1024] = {0};
      char realName[1024] = {0};
      char backupName[1024] = {0};
      int fd, rc;

      // TODO: the path to the config file (/data) should be moved to a
      // centralized place so it can be customized easily.
      if (cmsMdm_isCmsClassic() ||
          IS_EMPTY_STRING(compName))
      {
         cmsLog_debug("write out monolithic psi file");
         snprintf(tmpName, sizeof(tmpName), "/data/psi.tmp");
         snprintf(realName, sizeof(realName), "/data/psi");
#if defined(SUPPORT_BACKUP_PSI) && defined(SUPPORT_BACKUP_PSI_MIRROR_MODE)
         snprintf(backupName, sizeof(backupName), "/data/psibackup");  // PSI_BACKUP_FILE_NAME
#endif
      }
      else
      {
         if (cmsUtl_strcmp(compName, mdmShmCtx->compName))
         {
            // Normally, a component will only write the config file for itself. 
            // However, there is a special case where sysmgmt will write out the wifi config file, 
            // see processWriteConfigFile.
            if (cmsUtl_strcmp(mdmShmCtx->compName, BDK_COMP_SYSMGMT))
            {
               cmsLog_error("mismatch: compName=%s mdmShmCtx->compName=%s",
                         compName, mdmShmCtx->compName);
            }
            else
            {
               cmsLog_notice("mismatch: compName=%s mdmShmCtx->compName=%s",
                         compName, mdmShmCtx->compName);
            }
         }
         snprintf(tmpName, sizeof(tmpName), "/data/psi_%s.tmp", compName);
         snprintf(realName, sizeof(realName), "/data/psi_%s", compName);
#if defined(SUPPORT_BACKUP_PSI) && defined(SUPPORT_BACKUP_PSI_MIRROR_MODE)
         // TODO: we write backup file here, but there is no code which looks
         // for and uses this component level backup file.
         snprintf(backupName, sizeof(backupName), "/data/psibackup_%s", compName);  // PSI_BACKUP_FILE_NAME
#endif
      }

      // First write contents to a tmp file.
      if ((fd = open(tmpName, (O_RDWR|O_CREAT|O_TRUNC), (S_IRUSR|S_IWUSR))) < 0)
      {
         cmsLog_error("could not open %s for write", tmpName);
         ret = CMSRET_INTERNAL_ERROR;
         goto writev_exit;
      }

      rc = write(fd, crcBuf, len);
      if (rc < (SINT32) len)
      {
         cmsLog_error("write to %s failed, rc=%d", tmpName, rc);
         ret = CMSRET_INTERNAL_ERROR;
         close(fd);
         goto writev_exit;
      }

      fsync(fd);
      close(fd);

      // the rename operation is atomic.  If the dest file (realName) already
      // exists, it is atomically replaced by src file (tmpName).
      rc = rename(tmpName, realName);
      if (rc != 0)
      {
         cmsLog_error("rename of %s to %s failed, rc=%d", tmpName, realName, rc);
         ret = CMSRET_INTERNAL_ERROR;
         goto writev_exit;
      }

      ret = CMSRET_SUCCESS;

      /* flush to the file system (this just calls sync() ) */
      oal_flushFsToMedia();

      cmsLog_notice("wrote %s (len=%d)", realName, len);

      // Now that we have written the (primary) config file, see if we need to
      // write out a backup copy.  We don't need to write-to-tmp-then-rename
      // with the backup since the primary copy of the config file has been
      // successfully written.
      if (!IS_EMPTY_STRING(backupName))
      {
         if ((fd = open(backupName, (O_RDWR|O_CREAT|O_TRUNC), (S_IRUSR|S_IWUSR))) < 0)
         {
            // Log errors during write of backup, but do not change main
            // ret code.
            cmsLog_error("could not open %s for write", backupName);
         }
         else
         {
            rc = write(fd, crcBuf, len);
            if (rc < (SINT32) len)
            {
               cmsLog_error("write to %s failed, rc=%d", backupName, rc);
            }
            else
            {
               fsync(fd);
               cmsLog_notice("wrote %s (len=%d)", backupName, len);
            }
            close(fd);
         }
      }
   }

writev_exit:

   CMSMEM_FREE_BUF_AND_NULL_PTR(buf2);
   CMSMEM_FREE_BUF_AND_NULL_PTR(crcBuf);

#ifdef COMPRESSED_CONFIG_FILE
   CMSMEM_FREE_BUF_AND_NULL_PTR(configBuf);
#endif

   msgHandle = cmsMdm_getThreadMsgHandle();
   if ((ret == CMSRET_SUCCESS) && (msgHandle != NULL))
   {
      CmsEntityId myEid = cmsMsg_getHandleEid(msgHandle);
      /*
       * Send out an event msg for any interested app.
       * The event message only goes out when a manangement app saves
       * the current MDM to config, but not when an app uploads a config file
       * via the network.  In the latter scenario, the app sends the whole
       * config file to smd, which validates and writes the config file to flash.
       * But the problem is, smd does not set a msgHandle in mdmLibCtx.msgHandle.
       * So it cannot send a message.  This is probably OK, beacuse right
       * after we upload a config file and write it to flash, we reboot the modem.
       */
      msg.src = myEid;
      msg.dst = EID_SMD;
      msg.type = CMS_MSG_CONFIG_WRITTEN;
      msg.flags_event = 1;
      msg.wordData = myEid;  /* this is who wrote out the config file */
      cmsMsg_send(msgHandle, &msg);
   }

   return ret;
}


#if defined(SUPPORT_OPENPLAT)

#define START_TAG_SW_MODULES    "    <SoftwareModules>"
#define END_TAG_SW_MODULES      "    </SoftwareModules>"

#ifdef SUPPORT_DM_PURE181
#define END_TAG_ROOT_MDM        "  </Device>"
#else
#define END_TAG_ROOT_MDM        "  </InternetGatewayDevice>"
#endif

/* replace <SoftwareModules>...</SoftwareModules> section in cfgIn
 * with <SoftwareModules>..</SoftwareModules> section in cfgFlash
 * and write to cfgOut with this section at the end of cfgOut */
static CmsRet writeBeepValidatedBufToConfigFlash(const char *cfgIn,
                                                 UINT32 lenCfgIn)
{
   CmsRet ret = CMSRET_SUCCESS;
   char *cfgFlash = NULL, *cfgOut = NULL;
   char *startSmCfgIn = NULL, *endSmCfgIn = NULL, *endCfgIn = NULL;
   char *startSmCfgFlash = NULL, *endSmCfgFlash = NULL;
   UINT32 len = 0, lenEndSm = 0, lenCfgFlash = 0;
   UINT32 lenSmCfgFlash = 0, lenSmCfgIn = 0;
   UINT32 lenCfgOut = 0;

   if ((lenCfgFlash = cmsImg_getConfigFlashSize()) == 0)
   {
      cmsLog_error("cmsImg_getConfigFlashSize returned 0!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* malloc a buffer for holding the config file from flash. */
   if ((cfgFlash = cmsMem_alloc(lenCfgFlash, 0)) == NULL)
   {
      cmsLog_error("malloc of %d bytes failed", lenCfgFlash);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   /*
    * load primary config file from flash to buffer.
    */
   if ((ret = oal_readConfigFlashToBuf(CMS_CONFIG_PRIMARY,
                                       cfgFlash,
                                       &lenCfgFlash)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Fail to loadd primary config file from flash to buffer, ret=%d", ret);
      CMSMEM_FREE_BUF_AND_NULL_PTR(cfgFlash);
      return CMSRET_INTERNAL_ERROR;
   }

   lenEndSm = strlen(END_TAG_SW_MODULES);

   /* calculate length of Software Modules in flash. */
   if ((startSmCfgFlash = strstr(cfgFlash, START_TAG_SW_MODULES)) != NULL &&
       (endSmCfgFlash = strstr(cfgFlash, END_TAG_SW_MODULES)) != NULL)
   {
      lenSmCfgFlash = endSmCfgFlash - startSmCfgFlash + lenEndSm;
   }
   else
   {
      cmsLog_notice("Software Modules does not exist in flash");
      CMSMEM_FREE_BUF_AND_NULL_PTR(cfgFlash);
      return writeValidatedBufToConfigFlash(cfgIn, lenCfgIn);
   }

   /* calculate length of Software Modules in configuration. */
   if ((startSmCfgIn = strstr(cfgIn, START_TAG_SW_MODULES)) != NULL &&
       (endSmCfgIn = strstr(cfgIn, END_TAG_SW_MODULES)) != NULL)
   {
      lenSmCfgIn = endSmCfgIn - startSmCfgIn + lenEndSm;
   }

   /* do nothing if Software Modules in configuration file is
    * the same with Software Modules in flash */
   if (lenSmCfgIn == lenSmCfgFlash &&
       memcmp(startSmCfgIn, startSmCfgFlash, lenSmCfgIn) == 0)
   {
      cmsLog_notice("Software Modules in configuration file is the same with Software Modules in flash");
      CMSMEM_FREE_BUF_AND_NULL_PTR(cfgFlash);
      return writeValidatedBufToConfigFlash(cfgIn, lenCfgIn);
   }

   /* calculate length of configuration output. */
   lenCfgOut = lenCfgIn - lenSmCfgIn + lenSmCfgFlash;

   /* malloc a buffer for holding the update config. */
   if ((cfgOut = cmsMem_alloc(lenCfgOut, ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("malloc of %d bytes failed", lenCfgOut);
      CMSMEM_FREE_BUF_AND_NULL_PTR(cfgFlash);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   endCfgIn = strstr(cfgIn, END_TAG_ROOT_MDM);
   if(endCfgIn == NULL)
   {
      cmsLog_error("END_TAG_ROOT_MDM not found in cfgIn");
      CMSMEM_FREE_BUF_AND_NULL_PTR(cfgOut);
      return CMSRET_INTERNAL_ERROR; 
   }
   /* copy cfgIn to cfgOut without SoftwareModules section
    * up to and exclude END_TAG_ROOT_MDM */
   if (startSmCfgIn != NULL && endSmCfgIn != NULL)
   {
      len = startSmCfgIn - &(cfgIn[0]);
      memcpy(cfgOut, cfgIn, len);

      memcpy(&(cfgOut[len]),
             endSmCfgIn + lenEndSm,
             endCfgIn - (endSmCfgIn + lenEndSm));
   }
   else
   {
      len = endCfgIn - &(cfgIn[0]);
      memcpy(cfgOut, cfgIn, len);
   }

   /* copy SoftwareModules section in cfgFlash to cfgOut */
   if (startSmCfgFlash != NULL && endSmCfgFlash != NULL)
   {
      len = strlen(cfgOut);
      if ((len + lenSmCfgFlash) < lenCfgOut)
      {
         memcpy(&(cfgOut[len]), startSmCfgFlash, lenSmCfgFlash);
      }
   }

   /* copy from END_TAG_ROOT_MDM to end of cfgIn to cfgOut*/
   len = strlen(cfgOut);
   if ((len + lenCfgIn - (endCfgIn - &(cfgIn[0]))) <= lenCfgOut)
   {
      memcpy(&(cfgOut[len]), endCfgIn, lenCfgIn - (endCfgIn - &(cfgIn[0])));
   }

   ret = writeValidatedBufToConfigFlash(cfgOut, lenCfgOut);

   CMSMEM_FREE_BUF_AND_NULL_PTR(cfgFlash);

   CMSMEM_FREE_BUF_AND_NULL_PTR(cfgOut);

   return ret;
}

#endif


CmsRet cmsMgm_writeValidatedBufToConfigFlash(const char *buf, UINT32 len)
{
   CmsRet ret = CMSRET_SUCCESS;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if ((ret = lck_autoLockAllZonesWithBackoff(0, MGM_LOCK_TIMEOUT, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      return ret;
   }

#if defined(SUPPORT_OPENPLAT)

   ret = writeBeepValidatedBufToConfigFlash(buf, len);

#else

   ret = writeValidatedBufToConfigFlash(buf, len);

#endif

   lck_autoUnlockAllZones(__FUNCTION__);

   return ret;
}

CmsRet cmsMgm_writeValidatedBufToConfigFlashCompName(const char *buf,
                     UINT32 len, const char *compName)
{
   CmsRet ret = CMSRET_SUCCESS;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if ((ret = lck_autoLockAllZonesWithBackoff(0, MGM_LOCK_TIMEOUT, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   ret = writeValidatedBufToConfigFlashCompName(buf, len, compName);

   lck_autoUnlockAllZones(__FUNCTION__);

   return ret;
}


// TODO: make this BDK multi-component aware
CmsRet cmsMgm_validateConfigBuf(const char *buf, UINT32 len)
{
   char *buf2=NULL;
   UBOOL8 freeBuf2=FALSE;
   CmsRet ret;

   cmsLog_notice("Entered:buf=%p len=%d", buf, len);
      
   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);
   
   if ((ret = lck_autoLockAllZonesWithBackoff(0, MGM_LOCK_TIMEOUT, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   // Validating a config buf can take a while, increase lock hold time
   // warning thresh.
   cmsLck_setHoldTimeWarnThresh(CMSLCK_MAX_HOLDTIME * 2);

   /*
    * mdm_validateConfigBuf expects the config file must end with a 0 byte.
    * This config file might have gotten its last 0 byte stripped off by
    * wordpad or something.  So check for that, and append a 0 byte if neccessary.
    */
   if (buf[len-1] != 0)
   {
      cmsLog_debug("final zero byte not detected, adding it.");
      if ((buf2 = cmsMem_alloc(len+1, ALLOC_ZEROIZE)) == NULL)
      {
         cmsLog_error("realloc of %u bytes for config buf failed", len);
         ret = CMSRET_RESOURCE_EXCEEDED;
      }
      else
      {
         freeBuf2 = TRUE;
         memcpy(buf2, buf, len);
         len++;
      }
   }
   else
   {
      cmsLog_debug("final zero byte detected, do nothing.");
      buf2 = (char *) buf;
   }

   ret = mdm_validateConfigBuf(buf2, len);

   if (freeBuf2)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(buf2);
   }

   lck_autoUnlockAllZones(__FUNCTION__);

   cmsLog_notice("Exit: ret=%d", ret);
   return ret;
}

CmsRet cmsMgm_readRemoteMdmToBuf(void **response)
{
    CmsRet ret = CMSRET_SUCCESS;
    CmsMsgHeader msg = EMPTY_MSG_HEADER;
    void *msgHandle = cmsMdm_getThreadMsgHandle();

    msg.src = cmsMsg_getHandleEid(msgHandle);
    msg.dst = EID_REMOTE_OBJD;
    msg.type = CMS_MSG_REMOTE_OBJ_READ_MDM;
    msg.flags_request = 1;
    
    ret = cmsMsg_sendAndGetReplyBufWithTimeout(msgHandle,
                                     &msg, (CmsMsgHeader**)response, 10000);
    if (ret != CMSRET_SUCCESS)
    {
       cmsLog_error("Could not send to remote_objd %d!",ret);
    }

    return ret;
}
