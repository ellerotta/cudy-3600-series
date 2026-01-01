/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom Corporation
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


/* Includes. */
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include "adsl_api_trace.h"
#include "adslctlapi.h"
#include "devctl_adsl.h"
#include "bcm_retcodes.h"

// In the past, BUFLEN_x came from cms.h, but now we want to be independent
// from CMS, so just define it here.
#ifndef BUFLEN_8
#define BUFLEN_8  8
#endif


/* Bert status includes: state, elapse time, test result statistics */
BcmRet BcmAdslCtl_GetBertStatus(adslBertStatusEx *bertStatus)
{
   BcmRet nRet;
   adslMibInfo adslMib;
   long size = sizeof(adslMib);

   nRet = devCtl_adslGetObjectValue(NULL, 0, (char *)&adslMib, &size);
   *bertStatus = adslMib.adslBertStatus;
   return (nRet);
}

// Description  : Get ADSL PHY version.
void cmsAdsl_getPhyVersion(char *version, int len)
{
   adslVersionInfo adslVer;
   int verLen;

   if ( version == NULL ) 
      return;

   memset(version,0,len);

   if (devCtl_adslGetVersion(&adslVer) == BCMRET_SUCCESS)
   {
      verLen = strlen(adslVer.phyVerStr);
      if (len > verLen)
         strcpy(version,adslVer.phyVerStr);
      else
         strncpy(version,adslVer.phyVerStr,len);
   }
}


// Description  : Get ADSL connection information.
BcmRet cmsAdsl_getConnectionInfo( PADSL_CONNECTION_INFO pConnInfo )
{
#ifdef DESKTOP_LINUX
   return BCMRET_SUCCESS;
#else
   return xdslCtl_GetConnectionInfo(0, pConnInfo);
#endif
} // BcmAdslCtl_GetConnectionInfo

//***************************************************************************
// Function Name: parseAdslInfo
// Description  : parse info to get value of the given variable.
// Returns      : none.
//***************************************************************************/
void parseAdslInfo(char *info, char *var, char *val, int len)
{
   char *pChar = NULL;
   int i = 0;

   if ( info == NULL || var == NULL || val == NULL ) return;

   pChar = strstr(info, var);
   if ( pChar == NULL ) return;

   // move pass the variable string in line
   pChar += strlen(var);

   // Remove spaces from beginning of value string
   while ( *pChar != '\0' && isspace((int)*pChar) != 0 )
      pChar++;

   // get data until end of line, or comma, or space char
   for ( i = 0;
         i < len && *pChar != '\0' &&
         *pChar != ',' && isspace((int)*pChar) == 0;
         i++, pChar++ )
      val[i] = *pChar;
   val[i] = '\0';
} // parseAdslInfo

BcmRet cmsAdsl_initialize(adslCfgProfile *pAdslCfg)
{
   cmsAdsl_initializeTrace();
   return(devCtl_adslInitialize(NULL,0,pAdslCfg));
}

BcmRet cmsAdsl_uninitialize(void)
{
   cmsAdsl_uninitializeTrace();
   return(devCtl_adslUninitialize());
}

/* this function is called to configure ADSL and also start it by bring it up */
BcmRet cmsAdsl_start(void)
{
   cmsAdsl_startTrace();
   return(devCtl_adslConnectionStart());
}

BcmRet cmsAdsl_stop(void)
{
   cmsAdsl_stopTrace();
   return(devCtl_adslConnectionStop());
}

BcmRet cmsAdsl_getAdslMib(adslMibInfo *adslMib)
{
   BcmRet nRet;
   long size = sizeof(adslMibInfo);

   nRet = devCtl_adslGetObjectValue(NULL, 0, (char *)adslMib, &size);
   return nRet;
}

BcmRet cmsAdsl_ResetStatCounters(void)
{
   cmsAdsl_ResetStatCountersTrace();
   return(devCtl_adslResetStatCounters());
}

BcmRet cmsAdsl_BertTestStart(UINT32  duration)
{
   cmsAdsl_BertTestStartTrace();
   return(devCtl_adslBertStartEx(duration));
}

BcmRet cmsAdsl_BertTestStop(void)
{
   cmsAdsl_BertTestStopTrace();
   return(devCtl_adslBertStopEx());
}

BcmRet cmsAdsl_getAdslMibObject(char *oidStr, int oidLen, char *data, long *dataLen)
{
   BcmRet nRet;

   nRet = devCtl_adslGetObjectValue(oidStr, oidLen, data, dataLen);
   return nRet;
}

BcmRet cmsAdsl_setTestDiagMode(void)
{
   cmsAdsl_setTestDiagModeTrace();
   return(devCtl_adslSetTestMode(ADSL_TEST_DIAGMODE));
}

/* utilities functions */
SINT32 Qn2DecI(SINT32 qnVal, int q)
{
   return (qnVal >> q) - (qnVal >> 31);
}

SINT32 Qn2DecF(SINT32 qnVal, int q)
{
   int      sn = qnVal >> 31;
   return (((qnVal ^ sn) - sn) & ((1 << q) - 1)) * (10000 >> q);
}

char* QnToString(SINT32 val, int q)
{
   static   char  str1[32];
   SINT32        iPart;

   if (val < 0) {
      val = -val;
      iPart = -(val >> q);
      if (0 == iPart) {
         sprintf(str1, "-0.%04u", Qn2DecF(val,q));
         return str1;
      }
   }
   else
      iPart = val >> q;
   sprintf( str1, "%d.%04d", iPart, Qn2DecF(val,q));
   return str1;
}

/* to be tested with new driver */

BcmRet cmsAdsl_setAdslMibObject(char *objId, int objIdLen, char *dataBuf, long *dataBufLen)
{
   BcmRet nRet;
   cmsAdsl_setAdslMibObjectTrace();
   nRet = devCtl_adslSetObjectValue(objId, objIdLen, dataBuf, dataBufLen);
   return nRet;
}


/* for debug */
void cmsAdsl_printMibObject(int oid,int oidStrSize, char* data, ulong dataLen)
{
   int i;

   printf("\nMIB object retrieved by TR69c after dslDiag completes\n");
   printf("oid %d, oidStrSize %d, datatLen %ld\n",oid,oidStrSize,dataLen);
   for (i = 0; i < dataLen; i+=8)
      printf("   %d\t%x %x %x %x %x %x %x %x\n", i, data[i],data[i+1],data[i+2],data[i+3],data[i+4],data[i+5],data[i+6],data[i+7]);
   printf("--------------------------------------------------------------------\n");
}

int cmsAdsl_paramNameToOid(char *paramName)
{
	if (strcmp(paramName,"QLNpsds") == 0)
	{
	   return (kOidAdslPrivQuietLineNoiseDsPerToneGroup);
	}
	if (strcmp(paramName,"QLNpsus") == 0)
	{
	   return (kOidAdslPrivQuietLineNoiseUsPerToneGroup);
	}
	if (strcmp(paramName,"SNRpsds") == 0)
	{
	   return (kOidAdslPrivSNRDsPerToneGroup);
	}
	if (strcmp(paramName,"SNRpsus") == 0)
	{
	   return (kOidAdslPrivSNRUsPerToneGroup);
	}
	if (strcmp(paramName,"HLOGpsds") == 0)
	{
	   return (kOidAdslPrivChanCharLogDsPerToneGroup);
	}
	if (strcmp(paramName,"HLOGpsus") == 0)
	{
	   return (kOidAdslPrivChanCharLogUsPerToneGroup);
	}
	if (strcmp(paramName,"BITSpsds") == 0)
	{
	   return (kOidAdslPrivBitAllocDsPerToneGroup);
	}
	return 0;
}

BcmRet cmsAdsl_formatHLINString(char **dataStr, SINT32 *data, int maxDataStrLen)
{
   int count, totalCount=0;
   int i;
   char *dataPtr;
   char tmpBuf[50];
   SINT16 *pHlinDataPtrH=NULL;
   SINT16 *pHlinDataPtrL=NULL;

   cmsAdsl_formatHLINStringTrace();

   if ((*dataStr == NULL) || (data == NULL))
   {
      return BCMRET_INTERNAL_ERROR;
   }
   else 
   {
      dataPtr = *dataStr;
      memset(dataPtr,0,maxDataStrLen);
      /* number of tone 512 */
      for (i = 0; i < NUM_TONE_GROUP; i++) 
      {
         pHlinDataPtrH = (SINT16*)(&data[i]);
         pHlinDataPtrL = (SINT16*)(&data[i]) + 1;
         count = sprintf(tmpBuf,"%d,%d", (*pHlinDataPtrH),(*pHlinDataPtrL));
         if ((totalCount + count) > maxDataStrLen)
         {
            /* buffer overflow */
            break;
         }
         else
         {
            //pHlinDataPtr = (SINT16*)(&data[i]);
            count = sprintf(dataPtr,"%d,%d", (*pHlinDataPtrH),(*pHlinDataPtrL));
         }
         dataPtr += count;
         totalCount += count;
         if ((i+1) < NUM_TONE_GROUP)
         {
            count = sprintf(dataPtr,",");
            dataPtr += count;
            totalCount += count;
         }
      } /* loop */
   } /* dataStr ok */
   return (BCMRET_SUCCESS);
}

BcmRet cmsAdsl_formatSubCarrierDataString(char **dataStr, void *data, char *paramName, int maxDataStrLen)
{
   int count;
   int totalCount = 0;
   char tmpBuf[50];
   int i, oid;
   char *dataPtr;
   ulong value;
   SINT16 *shortDataPtr;
   SINT8 *byteDataPtr;

   cmsAdsl_formatSubCarrierDataStringTrace();

   if ((*dataStr == NULL) || (data == NULL))
   {
      return BCMRET_INTERNAL_ERROR;
   }
   else 
   {
      dataPtr = *dataStr;
      memset(dataPtr,0,maxDataStrLen);
      oid = cmsAdsl_paramNameToOid(paramName);

      switch (oid)
      {
      case kOidAdslPrivQuietLineNoiseDsPerToneGroup:
      case kOidAdslPrivQuietLineNoiseUsPerToneGroup:
         shortDataPtr = (SINT16*)data;
         for (i = 0; i < NUM_TONE_GROUP; i++)
         {
            if (shortDataPtr[i] == 0)
            {
               value = 255;
            }
            else
            {
               value= (ulong) ((-shortDataPtr[i]-368) >> 3);
               if (value > 254)
               {
                  value = 255;
               }
            }
            count = sprintf(tmpBuf,"%d", (UINT8)value);
            if ((totalCount + count) <= maxDataStrLen)
            {
               count = sprintf(dataPtr,"%d", (UINT8)value);
               totalCount += count;
               dataPtr += count;
               if ((i+1) < NUM_TONE_GROUP)
               {
                  count = sprintf(dataPtr,",");
                  dataPtr += count;
                  totalCount += count;
               }
            }
            else
            {
               break;
            }
         } /* NUM_TONE_GROUP loop */
         break;

      case kOidAdslPrivSNRDsPerToneGroup:
      case kOidAdslPrivSNRUsPerToneGroup:
         shortDataPtr = (SINT16*)data;
         for (i = 0; i < NUM_TONE_GROUP; i++)
         {
#if !defined(PSDATA_IMPL_VERSION) || (PSDATA_IMPL_VERSION < 2)
            if (shortDataPtr[i] == 0)
            {
               value = 255;
            }
            else
#endif
            {
               value= (ulong) ((shortDataPtr[i]+512) >> 3);
               if (value > 254)
                  value = 255;
            }
            count = sprintf(tmpBuf,"%d", (UINT8)value);
            if ((totalCount + count) <= maxDataStrLen)
            {
               count = sprintf(dataPtr,"%d", (UINT8)value);
               totalCount += count;
               dataPtr += count;
               if ((i+1) < NUM_TONE_GROUP)
               {
                  count = sprintf(dataPtr,",");
                  dataPtr += count;
                  totalCount += count;
               }
            }
            else 
            {
               break;
            }
         } /* NUM_TONE_GROUP loop */
         break;

      case kOidAdslPrivChanCharLogDsPerToneGroup:
      case kOidAdslPrivChanCharLogUsPerToneGroup:
         shortDataPtr = (SINT16*)data;
         for (i = 0; i < NUM_TONE_GROUP; i++)
         {
#if !defined(PSDATA_IMPL_VERSION) || (PSDATA_IMPL_VERSION < 2)
            if (shortDataPtr[i] == 0)
            {
               value = 0x3ff;
            }
            else
#endif
            {
               value = (ulong) (((96-shortDataPtr[i])*5) >> 3);
               if (value > 0x3FE) 
               {
                  value=0x3FF;
               }
            }
            count = sprintf(tmpBuf,"%u", (unsigned int)value);
            if ((totalCount + count) <= maxDataStrLen)
            {
               count = sprintf(dataPtr,"%u", (unsigned int)value);
               totalCount += count;
               dataPtr += count;
               if ((i+1) < NUM_TONE_GROUP)
               {
                  count = sprintf(dataPtr,",");
                  dataPtr += count;
                  totalCount += count;
               }
            }
            else 
            {
               break;
            }
         } /* NUM_TONE_GROUP loop */
         break;
            
      case kOidAdslPrivBitAllocDsPerToneGroup:
         byteDataPtr = (SINT8*)data;
         for (i = 0; i < NUM_TONE_GROUP; i++)
         {
            count = sprintf(tmpBuf,"%d", (byteDataPtr[i]));
            if ((totalCount + count) <= maxDataStrLen)
            {
               count = sprintf(dataPtr,"%d", byteDataPtr[i]);
               totalCount += count;
               dataPtr += count;
               if ((i+1) < NUM_TONE_GROUP)
               {
                  count = sprintf(dataPtr,",");
                  dataPtr += count;
                  totalCount += count;
               }
            }
            else 
            {
               break;
            }
         } /* NUM_TONE_GROUP loop */

         if (i < NUM_TONE_GROUP)
         {
            /* len of buffer is too small to hold formatted buffer */
         }
         break;
      }/* switch */
   } /* dataStr */
   return (BCMRET_SUCCESS);
} /* cmsAdsl_formatSubCarrierDataString */

BcmRet cmsAdsl_formatPertoneGroupQ4String(char **dataStr, void *data, int maxDataStrLen)
{
   int count;
   int totalCount = 0;
   char tmpBuf[50];
   int i;
   char *dataPtr;
   /* only for GAINpsds now */
   SINT16 *shortDataPtr = (SINT16*)data;

   cmsAdsl_formatPertoneGroupQ4StringTrace();

   if ((*dataStr == NULL) || (data == NULL))
   {
      return BCMRET_RESOURCE_EXCEEDED;
   }
   else 
   {
      dataPtr = *dataStr;
      memset(dataPtr,0,maxDataStrLen);
      for (i = 0; i < NUM_TONE_GROUP; i++)
      {
         count = sprintf(tmpBuf,"%s", QnToString(shortDataPtr[i],4));
         if ((totalCount+count) <= maxDataStrLen)
         {
            count = sprintf(dataPtr,"%s", QnToString(shortDataPtr[i],4));
            dataPtr += count;
            totalCount += count;
            if ((i+1) < NUM_TONE_GROUP)
            {
               count = sprintf(dataPtr,",");
               dataPtr += count;
               totalCount += count;
            }
         }
         else
         {
            /* formatted buffer is too big for dataStr */
            break;
         }
      } /* num_tone_group */
   }
   return (BCMRET_SUCCESS);
} /* getToneDataStringU */

BcmRet cmsAdsl_formatSnrmUsString(const bandPlanDescriptor32 *usNegBandPlanDiscPresentation,
                                  const short *data,
                                  char *dataStr, int maxDataStrLen)
{
   int n;
   char substr[BUFLEN_8]={0};
   char *substrPtr;
   int subStrLen, dStrLen;

   if ((usNegBandPlanDiscPresentation == NULL) ||
       (data == NULL) || (dataStr == NULL))
   {
      return (BCMRET_INTERNAL_ERROR);
   }

   memset(dataStr, 0, maxDataStrLen);

   for (n=0; n<=4; n++)
   {
      if (n < usNegBandPlanDiscPresentation->noOfToneGroups)
      {
         substrPtr=&substr[0];
         if (n!=0)
         {
            substr[0]=',';
            substrPtr++;
         }
         if (data[n] < -511 || data[n] > 511)
         {
            sprintf(substrPtr,"-512");
         }
         else
         {
            sprintf(substrPtr, "%d", data[n]);
         }
      }
      subStrLen = strlen(substr);
      dStrLen = strlen(dataStr);
      if ((dStrLen+subStrLen) < maxDataStrLen)
      {
         strcat(dataStr,substr);
      }
      else
      {
         /* just stop printing more data */
         break;
      }
   }

   return (BCMRET_SUCCESS);
}

BcmRet cmsAdsl_formatSnrmDsString(const bandPlanDescriptor32 *dsNegBandPlanDiscPresentation,
                                  const short *data,
                                  char *dataStr, int maxDataStrLen)
{
   int n, numDs;
   char substr[BUFLEN_8]={0};
   char *substrPtr;
   int subStrLen, dStrLen;

   if ((dsNegBandPlanDiscPresentation == NULL) ||
       (data == NULL) || (dataStr == NULL))
   {
      return (BCMRET_INTERNAL_ERROR);
   }

   memset(dataStr, 0, maxDataStrLen);

   numDs = (dsNegBandPlanDiscPresentation->noOfToneGroups == 4) ? 4 : 3;
   for (n=0; n<numDs; n++)
   {
      if (n < dsNegBandPlanDiscPresentation->noOfToneGroups)
      {
         substrPtr = &substr[0];
         if (n != 0)
         {
            substr[0] = ',';
            substrPtr++;
         }
         if (data[n] < -511 || data[n] > 511)
         {
            sprintf(substrPtr,"-512");
         }
         else
         {
            sprintf(substrPtr, "%d", data[n]);
         }
      }
      subStrLen = strlen(substr);
      dStrLen = strlen(dataStr);
      if ((dStrLen+subStrLen) < maxDataStrLen)
      {
         strcat(dataStr, substr);
      }
      else
      {
         /* just stop printing more data */
         break;
      }
   }

   return (BCMRET_SUCCESS);
}

int f2DecI(int val, int q)
{
   return (val/q);
}

int f2DecF(int val, int q)
{
   int      sn = val >> 31;
   return (((val ^ sn) - sn) % q);
}

int GetAdsl2Sq(adsl2DataConnectionInfo *p2, int q)
{
   return (0 == p2->L) ? 0 : (((p2->B+1)*p2->M + p2->R)*8*q)/p2->L;
}

int GetRcvRate(adslMibInfo *pMib, int pathId)
{
    return pMib->xdslInfo.dirInfo[0].lpInfo[pathId].dataRate;
}

int GetXmtRate(adslMibInfo *pMib, int pathId)
{
    return pMib->xdslInfo.dirInfo[1].lpInfo[pathId].dataRate;
}
