/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom Corporation
 *  All Rights Reserved
 *
 * <:label-BRCM:2012:DUAL/GPL:standard
 * 
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed
 * to you under the terms of the GNU General Public License version 2
 * (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
 * with the following added to such license:
 * 
 *    As a special exception, the copyright holders of this software give
 *    you permission to link this software with independent modules, and
 *    to copy and distribute the resulting executable under terms of your
 *    choice, provided that you also meet, for each linked independent
 *    module, the terms and conditions of the license of that module.
 *    An independent module is a module which is not derived from this
 *    software.  The special exception does not apply to any modifications
 *    of the software.
 * 
 * Not withstanding the above, under no circumstances may you combine
 * this software in any way with any other Broadcom software provided
 * under a license other than the GPL, without Broadcom's express prior
 * written consent.
 * 
 * :>
 * 
 ************************************************************************/

#include "cms.h"
#include "cms_util.h"
#include "oal.h"
#include "sysutil_fs.h"

UBOOL8 cmsFil_isFilePresent(const char *filename)
{
   return (sysUtil_isFilePresent(filename));
}

SINT32 cmsFil_getSize(const char *filename)
{
   return (sysUtil_getFileSize(filename));
}

CmsRet cmsFil_copyToBuffer(const char *filename, UINT8 *buf, UINT32 *bufSize)
{
   return ((CmsRet) sysUtil_copyToBuffer(filename, buf, bufSize));
}

CmsRet cmsFil_writeToProc(const char *procFilename, const char *s)
{
   return (oalFil_writeToProc(procFilename, s));
}

CmsRet cmsFil_writeBufferToFile(const char *filename, const UINT8 *buf,
                                UINT32 bufLen)
{
   return ((CmsRet) sysUtil_writeBufferToFile(filename, buf, bufLen));
}

CmsRet cmsFil_removeDir(const char *dirname)
{
   return (oalFil_removeDir(dirname));
}

CmsRet cmsFil_makeDir(const char *dirname)
{
   return (oalFil_makeDir(dirname));
}

CmsRet cmsFil_getOrderedFileList(const char *dirname, DlistNode *dirHead)
{
   return (oalFil_getOrderedFileList(dirname, dirHead));
}


void cmsFil_freeFileList(DlistNode *dirHead)
{
   DlistNode *tmp = dirHead->next;

   while (tmp != dirHead)
   {
      dlist_unlink(tmp);
      cmsMem_free(tmp);
      tmp = dirHead->next;
   }
}


UINT32 cmsFil_scaleSizetoKB(long nblks, long blkSize)
{

	return (nblks * (long long) blkSize + KILOBYTE/2 ) / KILOBYTE;

}

CmsRet cmsFil_renameFile(const char *oldName, const char *newName)
{
   return(oalFil_renameFile(oldName,newName));
}

CmsRet cmsFil_getNumFilesInDir(const char *dirname, UINT32 *num)
{
   return(oalFil_getNumFilesInDir(dirname, num));
}

UBOOL8 cmsFil_isDirPresent(const char *dirname)
{
   return (oalFil_isDirPresent(dirname));
}

CmsRet cmsFil_getNumericalOrderedFileList(const char *dirname, DlistNode *dirHead)
{
   return (oalFil_getNumericalOrderedFileList(dirname, dirHead));
}

CmsRet cmsFil_getIntPrefixFromFileName(char *fileName, UINT32 *pNum)
{
   return(oal_getIntPrefixFromFileName(fileName,pNum));
}

CmsRet cmsFil_readFirstlineFromFile(char *fileName, char *line, UINT32 lineSize)
{
   return (oalFil_readFirstlineFromFile(fileName, line, lineSize));
}

CmsRet cmsFil_readFirstlineFromFileWithBasedir(char *baseDir, char *fileName, char *line, UINT32 lineSize)
{
   char fileNameBuf[BUFLEN_64];

   snprintf(fileNameBuf, sizeof(fileNameBuf), "%s/%s", baseDir, fileName);

   return cmsFil_readFirstlineFromFile(fileNameBuf, line, lineSize);   
}

//  compare two files
UBOOL8 cmsFil_compareFile(char *fn1, char *fn2)
{
   char buf1[BUFLEN_264] = {0};
   char buf2[BUFLEN_264] = {0};
   UBOOL8 ret = TRUE;
    
   FILE *fp1 = fopen(fn1, "r");
   if (fp1 == NULL) 
   {
      cmsLog_debug("no file 1 = %s\n", buf1);
      ret = FALSE;
      return (ret);
   }
   FILE *fp2 = fopen(fn2, "r");
   if (fp2 == NULL) 
   {
      fclose(fp1);
      cmsLog_debug("no file 2 = %s\n", buf1);
      ret = FALSE;
      return (ret);
   }

   for(;!feof(fp1) || !feof(fp2);) 
   {
      int rc1 = fread(buf1, 1, sizeof(buf1), fp1);
      int rc2 = fread(buf2, 1, sizeof(buf2), fp2);
       
      if (rc1 != rc2) 
      {
         ret = FALSE;
         break;
      }
      if (strncmp(buf1, buf2, rc1) != 0) 
      {
         ret = FALSE;
         break;
      }
   }

   fclose(fp1);
   fclose(fp2);
    
   return (ret);
}
