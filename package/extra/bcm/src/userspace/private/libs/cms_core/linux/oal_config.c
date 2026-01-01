/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
 *  All Rights Reserved
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
 ************************************************************************/


#include <sys/stat.h> /* for stat */
#include <fcntl.h>
#include <errno.h>
#include "../oal.h"
#include "cms_util.h"
#include "cms_boardioctl.h"
#include "bcm_flashutil.h"
#include "genutil_crc.h"



/** Return the length field in the compressed config buf header.
 *
 * @param buf (IN) the compressed config header.
 *
 * @return the length field in the compressed config header.
 */
static UINT32 getCompressedConfigFileLength(const char *buf)
{
   char tmpbuf[COMPRESSED_CONFIG_HEADER_LENGTH];
   UINT32 headerLen=strlen(COMPRESSED_CONFIG_HEADER);
   UINT32 i, compressedLen;
   UBOOL8 found=FALSE;


   memcpy(tmpbuf, buf, COMPRESSED_CONFIG_HEADER_LENGTH);

   /* look for end marker and insert a null terminator */
   for (i=0; i < COMPRESSED_CONFIG_HEADER_LENGTH && !found; i++)
   {
      if (tmpbuf[i] == '>')
      {
         tmpbuf[i] = 0;
         found = TRUE;
      }
   }

   compressedLen = atoi(&(tmpbuf[headerLen]));
   cmsLog_debug("compressedLen=%d", compressedLen);

   /* sanity check, compressLen cannot be greater than real len */
   if (compressedLen > cmsImg_getRealConfigFlashSize())
   {
      cmsLog_error("invalid length %d in compressed config file header", compressedLen);
      return -1;
   }

   return compressedLen;
}


/** Return the crc field in the crc config buf header.
 *
 * @param buf (IN) the crc config header.
 *
 * @return the crc field in the crc config header.
 */
static UINT32 getConfigCrc(const char *buf)
{
   char tmpbuf[CRC_CONFIG_HEADER_LENGTH];
   UINT32 headerLen=strlen(CRC_CONFIG_HEADER);
   UINT32 i, crc=0;
   UBOOL8 found=FALSE;
   CmsRet ret;


   memcpy(tmpbuf, buf, CRC_CONFIG_HEADER_LENGTH);

   /* look for end marker and insert a null terminator */
   for (i=0; i < CRC_CONFIG_HEADER_LENGTH && !found; i++)
   {
      if (tmpbuf[i] == '>')
      {
         tmpbuf[i] = 0;
         found = TRUE;
      }
   }

   ret = cmsUtl_strtoul(&(tmpbuf[headerLen]), 0, 0, &crc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not convert crc header ret=%d, header=%s", ret, tmpbuf);
   }
   else
   {
      cmsLog_debug("crc in header 0x%x", crc);
   }

   return crc;
}


/** Get a bunch of information related to the config file headers in one call.
 *
 * Config files may start with a compression header and a CRC header,
 * or just a compression header, or just a CRC header, or no header.
 * Get information from those headers.
 *
 * @param buf (IN) Buffer read from the config flash.
 * @param len (IN) Length of buffer.
 * @param isCompressed  (OUT) TRUE if this is a compressed config file.
 * @param compressedLen (OUT) Length of compressed data from the header.
 * @param isCrc         (OUT) TRUE if there is a CRC header.
 * @param crc           (OUT) crc value from the header.
 *
 * @return number of bytes to start of real data, past all the headers.
 */
static UINT32 getConfigHeaderInfo(const char *buf, UINT32 len, UBOOL8 *isCompressed, UINT32 *compressedLen, UBOOL8 *isCrc, UINT32 *crc)
{
   cmsLog_debug("len=%d", len);

   *isCompressed = FALSE;
   *compressedLen = 0;
   *isCrc = FALSE;
   *crc = 0;

   if ((len > COMPRESSED_CONFIG_HEADER_LENGTH) &&
       (!cmsUtl_strncmp(buf, COMPRESSED_CONFIG_HEADER, strlen(COMPRESSED_CONFIG_HEADER))))
   {
      /* compressed header detected, get length of data */
      *isCompressed = TRUE;
      *compressedLen = getCompressedConfigFileLength(buf);
      cmsLog_debug("compressed header detected, len=%d", *compressedLen);

      if ((len > COMPRESSED_CONFIG_HEADER_LENGTH + CRC_CONFIG_HEADER_LENGTH) &&
          (!cmsUtl_strncmp(&(buf[COMPRESSED_CONFIG_HEADER_LENGTH]), CRC_CONFIG_HEADER, strlen(CRC_CONFIG_HEADER))))
      {
         /* both headers present */
         *isCrc = TRUE;
         *crc = getConfigCrc(&(buf[COMPRESSED_CONFIG_HEADER_LENGTH]));
         cmsLog_debug("crc header detected (after compression header), crc=0x%x", *crc);

         return COMPRESSED_CONFIG_HEADER_LENGTH + CRC_CONFIG_HEADER_LENGTH;
      }
      else
      {
         /* only compression header present */
         return COMPRESSED_CONFIG_HEADER_LENGTH;
      }
   }
   else if ((len > CRC_CONFIG_HEADER_LENGTH) &&
            (!cmsUtl_strncmp(buf, CRC_CONFIG_HEADER, strlen(CRC_CONFIG_HEADER))))
   {
      /* only CRC header present */
      *isCrc = TRUE;
      *crc = getConfigCrc(buf);
      cmsLog_debug("crc header detected, crc=0x%x", *crc);

      return CRC_CONFIG_HEADER_LENGTH;
   }

   return 0;
}


CmsRet oal_readConfigFlashToBuf(const char *selector, char *buf, UINT32 *len)
{
   CmsRet ret=CMSRET_SUCCESS;
   char *flashBuf;
   UINT32 flashLen;
   UINT32 offset;
   UINT32 ioctlCode;
   UBOOL8 isCompressed=FALSE;
   UBOOL8 isCrc=FALSE;
   UINT32 crc;
   UINT32 cdataLen;


   if (!cmsUtl_strcmp(selector, CMS_CONFIG_PRIMARY))
   {
      ioctlCode = PERSISTENT;
   }
   else if (!cmsUtl_strcmp(selector, CMS_CONFIG_BACKUP))
   {
      ioctlCode = BACKUP_PSI;
   }
   else
   {
      cmsLog_error("unregnized selector %s", selector);
      *len = 0;
      return CMSRET_INTERNAL_ERROR;
   }


   /* always read the contents of the flash into our own buffer first */
   flashLen = cmsImg_getRealConfigFlashSize();
   if ((flashBuf = cmsMem_alloc(flashLen, ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("could not allocate %d bytes for compressed buffer", flashLen);
      return CMSRET_RESOURCE_EXCEEDED;
   }


   ret = devCtl_boardIoctl(BOARD_IOCTL_FLASH_READ, ioctlCode,
                           flashBuf, flashLen, 0, mdmShmCtx->compName);
   if (ret != CMSRET_SUCCESS)
   {
      *len = 0;
      cmsMem_free(flashBuf);
      return ret;
   }

   cmsLog_debug("read %d bytes from flash %s", flashLen, selector);

   {
      char z = (char) 0xff;

      /*
       * When the config flash is erased with the cfe "i" command, we
       * get all 0xff.  Look for that pattern and set valid length to 0.
       */
      if (flashBuf[0] == z && flashBuf[1] == z && flashBuf[2] == z && flashBuf[3] == z &&
          flashBuf[4] == z && flashBuf[5] == z && flashBuf[6] == z && flashBuf[7] == z)
      {
         cmsLog_debug("looks like freshly initialized config area, return valid length=0");

         *len = 0;
         cmsMem_free(flashBuf);
         return ret;
      }
   }


   /*
    * Now check CRC.  If the CRC check fails, we do not return the buffer
    * to the user.
    */
   offset = getConfigHeaderInfo(flashBuf, flashLen, &isCompressed, &cdataLen, &isCrc, &crc);

#ifndef COMPRESSED_CONFIG_FILE
   if (isCompressed)
   {
      cmsLog_error("compressed config file detected, but this image does not support compressed config file.");
      *len = 0;
      cmsMem_free(flashBuf);
      return CMSRET_INVALID_CONFIG_FILE;
   }
#endif

   if (isCrc)
   {
      UINT32 calculatedCrc;
      UINT32 crcLen;

#ifdef COMPRESSED_CONFIG_FILE
      crcLen = cdataLen;
#else
      {
         /*
          * If there is no compression header, I need to figure out the actual
          * length of the data to do the CRC over.
          */
         for (crcLen=offset; (crcLen < flashLen) && (flashBuf[crcLen] != 0); crcLen++);

         /* include the first 0 byte by incrementing crcLen by 1 */
         crcLen++;

         /* subtract off the header offset to get true crcLen */
         if (crcLen >= offset)
         {
            crcLen -= offset;
         }
      }
#endif

      calculatedCrc = genUtl_getCrc32((UINT8 *) &(flashBuf[offset]), crcLen, CRC_INITIAL_VALUE);

      cmsLog_debug("calculated=0x%x headerCrc=0x%x", calculatedCrc, crc);
      if (calculatedCrc != crc)
      {
         cmsLog_error("crc check for config %s failed, calculated=0x%x headerCrc=0x%x",
                      selector, calculatedCrc, crc);
         *len = 0;
         cmsMem_free(flashBuf);
         return CMSRET_INVALID_CONFIG_FILE;
      }
   }


#ifdef COMPRESSED_CONFIG_FILE
   if (isCompressed)
   {
      SINT32 dlen;
      LZWDecoderState *decoder=NULL;
      CmsRet r2;

      cmsLog_debug("offset to config data is %d", offset);

      r2 = cmsLzw_initDecoder(&decoder, (UINT8 *) &(flashBuf[offset]), cdataLen);
      if (r2 != CMSRET_SUCCESS)
      {
         cmsMem_free(flashBuf);
         *len = 0;
         return r2;
      }

      /* decode into the user's buffer */
      dlen = (UINT32) cmsLzw_decode(decoder, (UINT8 *) buf, *len);
      cmsLog_debug("decode returned %d", dlen);

      cmsLzw_cleanupDecoder(&decoder);

      if (dlen < 0)
      {
         *len = 0;
         cmsMem_free(flashBuf);
         return CMSRET_INTERNAL_ERROR;
      }
      else
      {
         *len = (UINT32) dlen;
         CMSMEM_FREE_BUF_AND_NULL_PTR(flashBuf);
         ret = CMSRET_SUCCESS;
      }
   }
   else
#endif
   {
      /*
       * This is not a compressed config file.  Copy the contents
       * of the flash buffer into the user's buffer.
       */

      UINT32 smaller = (flashLen < *len) ? flashLen : *len;
      UINT32 i=0;

      memcpy(buf, &(flashBuf[offset]), smaller);
      CMSMEM_FREE_BUF_AND_NULL_PTR(flashBuf);

      /*
       * Look for the last 0 byte to determine the true length of the config file.
       */
      while (i < smaller)
      {
         if (buf[i] == 0)
         {
            *len = (i == 0) ? 0 : i+1;
            break;
         }
         i++;
      }
   }

   cmsLog_debug("returning %s ret=%d len=%d", selector, ret, *len);

   return ret;
}


/* This is used for reading the default config file, not for the config file
 * written to flash.
 */
CmsRet oal_readConfigFileToBuf(const char *filename, char *buf, UINT32 *len)
{
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("Reading config file from %s", filename);


   if (cmsFil_isFilePresent(filename))
   {
      ret = cmsFil_copyToBuffer(filename, (UINT8 *)buf, len);
      cmsLog_debug("read file %s, len=%d ret=%d", filename, *len, ret);
   }
   else
   {
      cmsLog_debug("file %s not found", filename);
      /* Failed to read the config file and make len  0 to indicate that */
      *len = 0;
   }

   return ret;
}


void oal_flushFsToMedia( void )
{
   sync();
}


