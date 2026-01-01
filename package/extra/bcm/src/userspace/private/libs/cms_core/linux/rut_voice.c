/***********************************************************************
 *
 *  Copyright (c) 2006 - 2009  Broadcom Corporation
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

/* ---- Include Files ---------------------------------------------------- */
#ifdef BRCM_VOICE_SUPPORT

#include <fcntl.h>
#include <unistd.h>

#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rut_util.h"
#include "rut_voice.h"
#include "mdm.h"

#include <bcm_country.h>         /* For VRG_COUNTRY_CFG defines */
#include <bcm_dsphal.h>

/* ---- Constants and Types ---------------------------------------------- */
#define DSPHAL_DRV_PATH    "/dev/" DSPHAL_DEV_NAME
#define VOICE_MAX_PREF_CODECS 6 // Arbitrary max preferred codec limit

#define ARRAY_SIZE(x)      (sizeof(x)/sizeof(*(x)))
#define TMP_BUFF_SIZE      32
/* ---- Private Variables ------------------------------------------------ */

static RUT_VRG_CM_TR104_LOCALE_MAP rutLocaleMap[] =
{  /*  VRG Locale define           CM Locale define (Alpha-3)   TR104 Region (Alpha-2)  Name   */
#if VRG_COUNTRY_CFG_AUSTRALIA
   { VRG_COUNTRY_AUSTRALIA             , "AUS", "AU", "AUSTRALIA"},
#endif
#if VRG_COUNTRY_CFG_BELGIUM
   { VRG_COUNTRY_BELGIUM               , "BEL", "BE", "BELGIUM"},
#endif
#if VRG_COUNTRY_CFG_BRAZIL
   { VRG_COUNTRY_BRAZIL                , "BRA", "BR", "BRAZIL"},
#endif
#if VRG_COUNTRY_CFG_CHILE
   { VRG_COUNTRY_CHILE                 , "CHL", "CL", "CHILE"},
#endif
#if VRG_COUNTRY_CFG_CHINA
   { VRG_COUNTRY_CHINA                 , "CHN", "CN", "CHINA"},
#endif
#if VRG_COUNTRY_CFG_CYPRUS
   { VRG_COUNTRY_CYPRUS                , "CYP", "CY", "CYPRUS"},
#endif
#if VRG_COUNTRY_CFG_CZECH
   { VRG_COUNTRY_CZECH                 , "CZH", "CZ", "CZECH"},
#endif
#if VRG_COUNTRY_CFG_DENMARK
   { VRG_COUNTRY_DENMARK               , "DNK", "DK", "DENMARK"},
#endif
#if VRG_COUNTRY_CFG_ETSI
   { VRG_COUNTRY_ETSI                  , "ETS", "XE", "ETSI"},
#endif
#if VRG_COUNTRY_CFG_FINLAND
   { VRG_COUNTRY_FINLAND               , "FIN", "FI", "FINLAND"},
#endif
#if VRG_COUNTRY_CFG_FRANCE
   { VRG_COUNTRY_FRANCE                , "FRA", "FR", "FRANCE"},
#endif
#if VRG_COUNTRY_CFG_GERMANY
   { VRG_COUNTRY_GERMANY               , "DEU", "DE", "GERMANY"},
#endif
#if VRG_COUNTRY_CFG_HUNGARY
   { VRG_COUNTRY_HUNGARY               , "HUN", "HU", "HUNGARY"},
#endif
#if VRG_COUNTRY_CFG_INDIA
   { VRG_COUNTRY_INDIA                 , "IND", "IN", "INDIA"},
#endif
#if VRG_COUNTRY_CFG_ITALY
   { VRG_COUNTRY_ITALY                 , "ITA", "IT", "ITALY"},
#endif
#if VRG_COUNTRY_CFG_JAPAN
   { VRG_COUNTRY_JAPAN                 , "JPN", "JP", "JAPAN"},
#endif
#if VRG_COUNTRY_CFG_MEXICO
   { VRG_COUNTRY_MEXICO                , "MEX", "MX", "MEXICO"},
#endif
#if VRG_COUNTRY_CFG_NETHERLANDS
   { VRG_COUNTRY_NETHERLANDS           , "NLD", "NL", "NETHERLANDS"},
#endif
#if VRG_COUNTRY_CFG_NEW_ZEALAND
   { VRG_COUNTRY_NEW_ZEALAND           , "NZL", "NZ", "NEWZEALAND"},
#endif
#if VRG_COUNTRY_CFG_NORTH_AMERICA
   { VRG_COUNTRY_NORTH_AMERICA         , "USA", "US", "NORTHAMERICA"},
#endif
#if VRG_COUNTRY_CFG_NORWAY
   { VRG_COUNTRY_NORWAY                , "NOR", "NO", "NORWAY"},
#endif
#if VRG_COUNTRY_CFG_POLAND
   { VRG_COUNTRY_POLAND                , "POL", "PL", "POLAND"},
#endif
#if VRG_COUNTRY_CFG_SPAIN
   { VRG_COUNTRY_SPAIN                 , "ESP", "ES", "SPAIN"},
#endif
#if VRG_COUNTRY_CFG_SWEDEN
   { VRG_COUNTRY_SWEDEN                , "SWE", "SE", "SWEDEN"},
#endif
#if VRG_COUNTRY_CFG_SWITZERLAND
   { VRG_COUNTRY_SWITZERLAND           , "CHE", "CH", "SWITZERLAND"},
#endif
#if VRG_COUNTRY_CFG_TAIWAN
   { VRG_COUNTRY_TAIWAN                , "TWN", "TW", "TAIWAN"},
#endif
#if VRG_COUNTRY_CFG_TR57
   { VRG_COUNTRY_TR57                  , "T57", "XT", "TR57"},
#endif
#if VRG_COUNTRY_CFG_UK
   { VRG_COUNTRY_UK                    , "GBR", "GB", "UK"},
#endif
#if VRG_COUNTRY_CFG_UNITED_ARAB_EMIRATES
   { VRG_COUNTRY_UNITED_ARAB_EMIRATES  , "UAE", "AE", "UNITEDARABEMIRATES"},
#endif
};

#define CODEC_CACHE_MAX_CODECS  100 // Max size of codec list - loosely based on CODEC_MAX_TYPES
#define CODEC_CACHE_MAX_STR_LEN 50  // Max string length of each codec name
typedef struct {
   int maxCodecs;   // number of codecs in the codecList
   char codecList[CODEC_CACHE_MAX_CODECS][CODEC_CACHE_MAX_STR_LEN]; // List of codec names
} codecCacheType;
// Cache of codec list in order to optimize codec validation operations
static codecCacheType codecCache = { 0 };

/*****************************************************************************
**  FUNCTION:       rutVoice_updateCache
**
**  PURPOSE:        Updates the codec list cache if not yet done.
**
**  INPUT PARMS:    None;
**
**  OUTPUT PARMS:   None
**
**  RETURNS:        None
**
*****************************************************************************/
static void rutVoice_updateCache()
{
   if (!codecCache.maxCodecs)
   {
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      VoiceCapCodecsObject *pCodec = NULL;

      while (cmsObj_getNextFlags(MDMOID_VOICE_CAP_CODECS, &iidStack,
                                 OGF_NO_VALUE_UPDATE, (void **)&pCodec)
                == CMSRET_SUCCESS)
      {
         if(pCodec->codec)
         {
            strncpy(codecCache.codecList[codecCache.maxCodecs], pCodec->codec,
                  CODEC_CACHE_MAX_STR_LEN - 1);
         }
         codecCache.maxCodecs++;

         cmsObj_free((void **)&pCodec);

         if (codecCache.maxCodecs >= CODEC_CACHE_MAX_CODECS)
         {
            cmsLog_error("%s: Max cache size (%d) reached!", __FUNCTION__,
                         codecCache.maxCodecs);
            break;
         }
      }
   }
}

CmsRet rutVoice_getMaxCodecs( int * maxCodec )
{
   rutVoice_updateCache();
   *maxCodec = codecCache.maxCodecs;

   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_getMaxPhysEndpt
**
**  PUROPOSE:       Gets total number of physical endpts
**
**  INPUT PARMS:    None;
**
**  OUTPUT PARMS:   maxPhysEndpt - max number of physical endpts
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_getMaxPhysEndpt( int * maxPhysEndpt )
{
   int epts = 0;
   int fp = open(DSPHAL_DRV_PATH, O_RDWR);
   if(fp < 0)
   {
      cmsLog_error("Cannot open %s", DSPHAL_DRV_PATH);
      return CMSRET_INVALID_ARGUMENTS;
   } 
   epts = ioctl(fp, DSPHAL_CMD_GETNUMEPT);
   close(fp);

   *maxPhysEndpt = epts;
   return CMSRET_SUCCESS;
}


/*****************************************************************************
**  FUNCTION:       rutVoice_getPhysEndptType
**
**  PUROPOSE:       Gets physical endpts type
**
**  INPUT PARMS:    PhyEndptId - physical endpoint ID
**
**  OUTPUT PARMS:   type - physical endpoint type (enum dh_type)
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_getPhysEndptType( int PhyEndptId, int *type )
{
   struct dsphal_chancfg cfg;

   int fp = open(DSPHAL_DRV_PATH, O_RDWR);

   memset((void *)&cfg, 0, sizeof(cfg));
   cfg.id = PhyEndptId;
   
   if(fp < 0)
   {
      cmsLog_error("Cannot open %s", DSPHAL_DRV_PATH);
      return CMSRET_INVALID_ARGUMENTS;
   } 
   
   ioctl(fp, DSPHAL_CMD_GETCHANCFG, &cfg);
   close(fp);

   *type = cfg.type;

   return CMSRET_SUCCESS;
}



/*****************************************************************************
**  FUNCTION:       rutVoice_getNumFxsEndpt
**
**  PUROPOSE:       Gets total number of FXO endpts
**
**
**  INPUT PARMS:    None;
**
**  OUTPUT PARMS:   numFxoEndpt - number of FXO endpts
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_getNumFxsEndpt( int * numFxsEndpt )
{
   int i, epts, ret;
   int fp = open(DSPHAL_DRV_PATH, O_RDWR);
   if(fp < 0)
   {
      cmsLog_error("Cannot open %s", DSPHAL_DRV_PATH);
      return CMSRET_INVALID_ARGUMENTS;
   } 
   
   epts = ioctl(fp, DSPHAL_CMD_GETNUMEPT);
   ret  = 0;
   for(i = 0; i < epts; i++)
   {
      struct dsphal_chancfg chan;
      chan.id = i;
      ioctl(fp, DSPHAL_CMD_GETCHANCFG, &chan);
      if(chan.type == DSPHAL_TYPE_FXS)
         ret++;
   }
   close(fp);

   *numFxsEndpt = ret;
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_getNumFxoEndpt
**
**  PUROPOSE:       Gets total number of FXO endpts
**
**  INPUT PARMS:    None;
**
**  OUTPUT PARMS:   numFxoEndpt - number of FXO endpts
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_getNumFxoEndpt( int * numFxoEndpt )
{
   int i, epts, ret;
   int fp = open(DSPHAL_DRV_PATH, O_RDWR);
   if(fp < 0)
   {
      cmsLog_error("Cannot open %s", DSPHAL_DRV_PATH);
      return CMSRET_INVALID_ARGUMENTS;
   } 
   epts = ioctl(fp, DSPHAL_CMD_GETNUMEPT);
   ret  = 0;
   for(i = 0; i < epts; i++)
   {
      struct dsphal_chancfg chan;
      chan.id = i;
      ioctl(fp, DSPHAL_CMD_GETCHANCFG, &chan);
      if(chan.type == DSPHAL_TYPE_FXO)
         ret++;
   }
   close(fp);

   *numFxoEndpt = ret;
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_getNumDectEndpt
**
**  PUROPOSE:       Gets total number of DECT endpts
**
**  INPUT PARMS:    None;
**
**  OUTPUT PARMS:   value - number of FXO endpts
**
**  RETURNS:        Nothing
**
*****************************************************************************/
void rutVoice_getNumDectEndpt( int * value )
{
   int i, epts, ret;
   int fp = open(DSPHAL_DRV_PATH, O_RDWR);
   if(fp < 0)
   {
      cmsLog_error("Cannot open %s", DSPHAL_DRV_PATH);
      return;
   } 
   epts = ioctl(fp, DSPHAL_CMD_GETNUMEPT);
   ret  = 0;
   for(i = 0; i < epts; i++)
   {
      struct dsphal_chancfg chan;
      chan.id = i;
      ioctl(fp, DSPHAL_CMD_GETCHANCFG, &chan);
      if(chan.type == DSPHAL_TYPE_DECT)
         ret++;
   }
   close(fp);

   *value = ret;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_getMaxLine
**
**  PUROPOSE:       Gets total number of Line Instances created and enabled by
**                  TR104.
**
**
**  INPUT PARMS:    None;
**
**  OUTPUT PARMS:   maxLine - max number of line instances
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_getMaxLine( int * maxLine )
{
#ifdef SIPLOAD
   *maxLine = getNumberOfLineInstances();
#else /* MGCPLOAD */
   rutVoice_getMaxPhysEndpt(maxLine);
#endif

   return CMSRET_SUCCESS;
}


/*****************************************************************************
**  FUNCTION:       rutVoice_mapFxoInterfaceIDToInst
**
**  PUROPOSE:       Maps fxo interface id number TR104 PSTN instance number.
**
**
**  INPUT PARMS:    vpInst - voice profile instance
**                  acntNum - account number associated to the vp
**
**  OUTPUT PARMS:   lineInst - line instance associated with the given acntNum
**                             and vpInst
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_mapFxoInterfaceIDToInst( int fxoId, int *inst )
{
   CmsRet          ret         = CMSRET_SUCCESS;
   InstanceIdStack iidStack    = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack subIidStack = EMPTY_INSTANCE_ID_STACK;
   void           *obj         = NULL;
   int             i = 0;

   if ( fxoId < 0 || inst == NULL )
   {
      cmsLog_error("Invalid arguments\n");
      return CMSRET_INVALID_ARGUMENTS;
   }

   PUSH_INSTANCE_ID(&iidStack, 1);  /* first voice service instance */
   while ( (ret =  cmsObj_getNextInSubTreeFlags(MDMOID_VOICE_PSTN, &iidStack, &subIidStack, OGF_NO_VALUE_UPDATE, &obj)) == CMSRET_SUCCESS)
   {
      i = (int)((_VoicePstnObject *)obj)->pstnInterfaceID;
      cmsObj_free(&obj);

      if( i == fxoId )
      {
         (*inst) = PEEK_INSTANCE_ID(&subIidStack);
         break;
      }
   }

   cmsLog_debug("fxoId:%d mapped to Inst:%d", *inst, fxoId);

   return ret;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_mapAcntNumToLineInst
**
**  PUROPOSE:       Maps account number and vpInst to TR104 Line instance number.
**
**
**  INPUT PARMS:    vpInst - voice profile instance
**                  acntNum - account number associated to the vp
**
**  OUTPUT PARMS:   lineInst - line instance associated with the given acntNum
**                             and vpInst
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_mapAcntNumToLineInst( int vpInst, int acntNum, int * lineInst )
{
   int spNum = getSpNumFromVpInst( vpInst );
   int numAcc = getNumAccPerSrvProv( spNum );

   if ( acntNum < 0 || acntNum >= numAcc || spNum < 0 )
   {
      cmsLog_error("Invalid arguments\n");
      return CMSRET_INVALID_ARGUMENTS;
   }

   *lineInst = getLineInstFromSpAcntNum( spNum, acntNum );

   cmsLog_debug("vpInst:%d, acntNum:%d mapped to lineInst:%d", vpInst, acntNum, *lineInst);

   return CMSRET_SUCCESS;
}


/*****************************************************************************
**  FUNCTION:       rutVoice_mapVpInstLineInstToCMAcnt
**
**  PUROPOSE:       Maps the voice profile instance and line instance to the
**                  corresponding call manager account number
**
**  INPUT PARMS:    vpInnst - voice profile instance
**                  lineinst - line instance
**
**  OUTPUT PARMS:   cmAcntNum - call manager account number
**
**  RETURNS:        CMSRET_SUCCESS if successful
**
*****************************************************************************/
CmsRet rutVoice_mapVpInstLineInstToCMAcnt( int vpInst, int lineInst, int * cmAcntNum )
{
   CmsRet          ret;
   InstanceIdStack iidStack    = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack subIidStack = EMPTY_INSTANCE_ID_STACK;
   VoiceProfObject *vpObj      = NULL;
   VoiceLineObject *lineObj    = NULL;
   int             inst;

   while ((ret = cmsObj_getNextFlags(MDMOID_VOICE_PROF, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &vpObj)) == CMSRET_SUCCESS)
   {
      inst = INSTANCE_ID_AT_DEPTH(&iidStack, (DEPTH_OF_IIDSTACK(&iidStack) - 1));

      if ( inst == vpInst )
      {
         cmsObj_free((void **) &vpObj);

         while ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_VOICE_LINE, &iidStack, &subIidStack, OGF_NO_VALUE_UPDATE, (void **) &lineObj)) == CMSRET_SUCCESS)
         {
            inst = INSTANCE_ID_AT_DEPTH(&subIidStack, (DEPTH_OF_IIDSTACK(&subIidStack) - 1));

            if ( inst == lineInst )
            {
               *cmAcntNum = lineObj->X_BROADCOM_COM_CMAcntNum;
               cmsObj_free((void **) &lineObj);
               cmsLog_debug("vpInst:%d, lineInst:%d mapped to cmAcntNum:%d", vpInst, lineInst, *cmAcntNum);
               return CMSRET_SUCCESS;
            }
            cmsObj_free((void **) &lineObj);
         }
         break;
      }
      cmsObj_free((void **) &vpObj);
   }

   cmsLog_error("Cannot map voice profile instance and line instance to a call manager number, ret = %d", ret);
   return ret;
}


/*****************************************************************************
**  FUNCTION:       rutVoice_mapCmLineToVpInstLineInst
**
**  PUROPOSE:       Maps CM account number to vpInst and line instance number.
**
**
**  INPUT PARMS:    cmNum - CM account number
**
**  OUTPUT PARMS:   lineInst - line instance associated with the given acntNum
**                             and vpInst
**                  vpInst - voice profile instance
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_mapCmLineToVpInstLineInst( int cmNum, int * vpInst, int * lineInst )
{
   int lclLineInst = getLineInstFromCmAcntNum( cmNum );
   int lclVpInst = getVpInstFromCmAcntNum( cmNum );

   if ( lclLineInst == 0 || lclVpInst == 0 )
   {
      cmsLog_error("Invalid cmNum %d\n", cmNum);
      return CMSRET_INVALID_ARGUMENTS;
   }

   *lineInst = lclLineInst;
   *vpInst = lclVpInst;

   cmsLog_debug("cmNum:%d mapped to lineInst:%d, vpInst:%d", cmNum, *lineInst, *vpInst);

   return CMSRET_SUCCESS;
}


/*****************************************************************************
**  FUNCTION:       rutVoice_assignSpNumToVpInst
**
**  PUROPOSE:       Assigns a service provider number to the vp instance
**
**  INPUT PARMS:    vpInst - voice profile instance
**
**  OUTPUT PARMS:   spNum - service provider number
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_assignSpNumToVpInst( int vpInst, int * spNum )
{
   if ( vpInst < 1 )
      return CMSRET_INVALID_ARGUMENTS;

   *spNum = assignSpNumToVpInst( vpInst );

   if ( *spNum == -1 )
   {
      cmsLog_error("Unable to assign a service provider number to the voice profile instance\n");
      return CMSRET_INTERNAL_ERROR;
   }

   return CMSRET_SUCCESS;
}


/*****************************************************************************
**  FUNCTION:       rutVoice_updateCMAcntNum
**
**  PUROPOSE:       Updates CM account number for line instances upon deletion
**                  of line instance lineToDelete.
**
**
**  INPUT PARMS:    vp - voice profile instance of the line instance that is
**                       being deleted
**                  lineToDelete - TR104 line instance number that is being
**                                 deleted.
**
**  OUTPUT PARMS:   None.
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_updateCMAcntNum( int vp, int lineToDelete )
{
   CmsRet          ret         = CMSRET_SUCCESS;
   InstanceIdStack iidStack    = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack subIidStack = EMPTY_INSTANCE_ID_STACK;
#if DMP_X_BROADCOM_COM_DECTENDPOINT_1
   InstanceIdStack subsubIidStack = EMPTY_INSTANCE_ID_STACK;
   VoiceLineDectLineIdObject *lineIdObj = NULL;
#endif /* DMP_X_BROADCOM_COM_DECTENDPOINT_1          */
   VoiceLineObject *lineObj    = NULL;
   VoiceProfObject *vpObj      = NULL;
   int             newAcntNum  = 0;
   int             vpInst;

   while ( (ret = cmsObj_getNextFlags(MDMOID_VOICE_PROF, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &vpObj)) == CMSRET_SUCCESS )
   {
      vpInst = INSTANCE_ID_AT_DEPTH( &iidStack, 1 );

      while ( (ret = cmsObj_getNextInSubTreeFlags(MDMOID_VOICE_LINE, &iidStack, &subIidStack, OGF_NO_VALUE_UPDATE, (void **) &lineObj)) == CMSRET_SUCCESS )
      {
         /* Skip object if it is the line object to delete */
         if (vp != vpInst || INSTANCE_ID_AT_DEPTH(&subIidStack, 2) != lineToDelete)
         {
            lineObj->X_BROADCOM_COM_CMAcntNum = newAcntNum;
            newAcntNum++;

#if DMP_X_BROADCOM_COM_DECTENDPOINT_1
         /* Update line id in DECT linesettings list */
         if( cmsObj_getNextInSubTreeFlags(MDMOID_VOICE_LINE_DECT_LINE_ID, &subIidStack, &subsubIidStack, OGF_NO_VALUE_UPDATE, (void **) &lineIdObj) == CMSRET_SUCCESS )
         {
            /* Dect line ids start from 1 therefore we use incremented CMAcntNum*/
            lineIdObj->element = newAcntNum;
            cmsObj_set(lineIdObj, &subsubIidStack);
            cmsObj_free((void **) &lineIdObj);
         }
#endif /* DMP_X_BROADCOM_COM_DECTENDPOINT_1          */

            /* Set line obj in mdm */
            cmsObj_set(lineObj, &subIidStack);
         }

         cmsObj_free((void **) &lineObj);
      }

      cmsObj_free((void **) &vpObj);
   }

   return CMSRET_SUCCESS;
}


/*****************************************************************************
**  FUNCTION:       rutVoice_updateSpNum
**
**  PUROPOSE:       Updates service provider numbers for voice profiles upon deletion
**                  of voice profile vpToDelete
**
**
**  INPUT PARMS:    vpToDelete - voice profile instance that is being deleted
**
**  OUTPUT PARMS:   None.
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_updateSpNum( int vpToDelete )
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   VoiceProfObject *vpObj   = NULL;
   int             newSpNum = 0;
   CmsRet ret;

   while ( (ret = cmsObj_getNextFlags(MDMOID_VOICE_PROF, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &vpObj)) == CMSRET_SUCCESS )
   {
      if ( vpToDelete != INSTANCE_ID_AT_DEPTH(&iidStack, 1) )
      {
         vpObj->X_BROADCOM_COM_SPNum = newSpNum;

         cmsLog_debug("vp %d gets newSpNum %d\n", INSTANCE_ID_AT_DEPTH(&iidStack, 1), newSpNum);
         newSpNum++;
         cmsObj_set(vpObj, &iidStack);
      }

      cmsObj_free((void **) &vpObj);
   }

   return CMSRET_SUCCESS;
}


/*****************************************************************************
**  FUNCTION:       rutVoice_assignCMAcnt
**
**  PUROPOSE:       Assigns CM account numbers to line instances.  The CM account
**                  numbers are assigned so that the smallest voice profile instance
**                  with the smallest line instance gets CM account 0.  This
**                  function is called when a new line instance is created.
**
**
**  INPUT PARMS:    vp - voice profile instance of the line instance that is
**                       being deleted
**                  lineToDelete - TR104 line instance number that is being
**                                 deleted.
**
**  OUTPUT PARMS:   None.
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_assignCMAcnt( void )
{
   InstanceIdStack iidStack    = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack subIidStack = EMPTY_INSTANCE_ID_STACK;
#if DMP_X_BROADCOM_COM_DECTENDPOINT_1
   InstanceIdStack subsubIidStack = EMPTY_INSTANCE_ID_STACK;
   VoiceLineDectLineIdObject *lineIdObj = NULL;
#endif /* DMP_X_BROADCOM_COM_DECTENDPOINT_1          */
   VoiceProfObject *vpObj      = NULL;
   VoiceLineObject *lineObj    = NULL;
   int              newAcntNum = 0;
   CmsRet           ret;


   while ((ret =  cmsObj_getNextFlags(MDMOID_VOICE_PROF, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &vpObj)) == CMSRET_SUCCESS)
   {
      while ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_VOICE_LINE, &iidStack, &subIidStack, OGF_NO_VALUE_UPDATE, (void **) &lineObj)) == CMSRET_SUCCESS )
      {
         lineObj->X_BROADCOM_COM_CMAcntNum = newAcntNum;
         newAcntNum++;

#if DMP_X_BROADCOM_COM_DECTENDPOINT_1
         /* Update line id in DECT linesettings list */
         if( cmsObj_getNextInSubTreeFlags(MDMOID_VOICE_LINE_DECT_LINE_ID, &subIidStack, &subsubIidStack, OGF_NO_VALUE_UPDATE, (void **) &lineIdObj) == CMSRET_SUCCESS )
         {
            /* Dect line ids start from 1 therefore we use incremented CMAcntNum*/
            lineIdObj->element = newAcntNum;
            cmsObj_set(lineIdObj, &subsubIidStack);
            cmsObj_free((void **) &lineIdObj);
         }
#endif /* DMP_X_BROADCOM_COM_DECTENDPOINT_1          */


         /* Set line obj in mdm */
         cmsObj_set(lineObj, &subIidStack);

         cmsObj_free((void **) &lineObj);
      }

      cmsObj_free((void **) &vpObj);
   }

   return CMSRET_SUCCESS;
}


CmsRet rutVoice_getSupportedLocales ( char * localeList, unsigned int length )
{
   int i = 0;
   char tmp[TMP_BUFF_SIZE];

   /* Generate a comma separated list of supported locales in the form
      Alpha3/Alpha2 - Country Name */
   localeList[0] = '\0';
   for(i = 0; i < ARRAY_SIZE(rutLocaleMap); i++)
   {
      snprintf(tmp, sizeof(tmp), "%s/%s - %s", rutLocaleMap[i].cmTxt,
                 rutLocaleMap[i].tr104Txt, rutLocaleMap[i].name);
      if (i)
         strncat(localeList, "\n", length);
      strncat(localeList, tmp, length);
   }
   return CMSRET_SUCCESS;
}

CmsRet rutVoice_getDefaultAlpha3Locale ( char * locale, unsigned int length )
{
   int i = 0;

   /* Generate a comma separated list of supported locales */
   for(i = 0; i < ARRAY_SIZE(rutLocaleMap); i++)
   {
      if (strncmp(rutLocaleMap[i].cmTxt,"USA",length) == 0)
      {
         snprintf(locale, length, "%s", rutLocaleMap[i].cmTxt);
         return CMSRET_SUCCESS;
      }
   }
   snprintf(locale, length, "%s", rutLocaleMap[0].cmTxt);
   return CMSRET_SUCCESS;
}

CmsRet rutVoice_getSupportedAlpha2Locales ( char * localeList, unsigned int length )
{
   int i = 0;
   char tmp[TMP_BUFF_SIZE];

   /* Generate a comma separated list of supported locales */
   localeList[0] = '\0';
   for(i = 0; i < ARRAY_SIZE(rutLocaleMap); i++)
   {
      snprintf(tmp, sizeof(tmp), "%s", rutLocaleMap[i].tr104Txt);
      if(i)
         strncat(localeList, ",", length);
      strncat(localeList, tmp, length);
      }
   return CMSRET_SUCCESS;
}

CmsRet rutVoice_getSupportedAlpha3Locales ( char * localeList, unsigned int length )
{
   int i = 0;
   char tmp[TMP_BUFF_SIZE];

   /* Generate a comma separated list of supported locales */
   localeList[0] = '\0';
   for(i = 0; i < ARRAY_SIZE(rutLocaleMap); i++)
   {
      snprintf(tmp, sizeof(tmp), "%s - %s", rutLocaleMap[i].cmTxt, rutLocaleMap[i].name);
      if(i)
         strncat(localeList, ",", length);
      strncat(localeList, tmp, length);
      }
   return CMSRET_SUCCESS;
}

CmsRet rutVoice_getSupportedTransports ( char * transports, unsigned int length )
{
   if (transports == NULL || length == 0)
      return CMSRET_SUCCESS;

   snprintf(transports, length, "%s", MDMVS_UDP);
#ifdef SIPLOAD
   strcat(transports, " ");
   strcat(transports, MDMVS_TCP);
#ifdef BRCM_SIP_TLS_SUPPORT 
   strcat(transports, " ");
   strcat(transports, MDMVS_TLS);
#endif /* BRCM_SIP_TLS_SUPPORT */
#endif /* SIPLOAD */

   return CMSRET_SUCCESS;
}

CmsRet rutVoice_getSupportedSrtpOptions ( char * options, unsigned int length )
{
   if (options == NULL || length == 0)
      return CMSRET_SUCCESS;

   snprintf(options, length, MDMVS_DISABLED);
#ifdef SIPLOAD
   /* Currently, SRTP is only supported under SIP */
#ifdef BRCM_VOICE_SRTP_SUPPORT
   strcat(options, " ");
   strcat(options, MDMVS_OPTIONAL);
   strcat(options, " ");
   strcat(options, MDMVS_MANDATORY);
#endif /* BRCM_VOICE_SRTP_SUPPORT */
#endif /* SIPLOAD */

   return CMSRET_SUCCESS;
}

CmsRet rutVoice_getSupportedBackToPrimOptions ( char * options, unsigned int length )
{
   if (options == NULL || length == 0)
   {
      return CMSRET_SUCCESS;
   }

   snprintf(options, length, MDMVS_DISABLED);
#ifdef SIPLOAD
   /* Currently, back-to-primary failover is only supported under SIP */
   strcat(options, " ");
   strcat(options, MDMVS_SILENT);
   strcat(options, " ");
   strcat(options, MDMVS_DEREGISTRATION);
   strcat(options, " ");
   strcat(options, MDMVS_SILENTDEREGISTRATION);
#endif /* SIPLOAD */

   return CMSRET_SUCCESS;
}

CmsRet rutVoice_getSupportedRedOptions ( char * options, unsigned int length )
{
   if (options == NULL || length == 0)
      return CMSRET_SUCCESS;

   snprintf(options, length, MDMVS_DISABLED);
#ifdef SIPLOAD
   /* Currently, RFC2198 is only supported under SIP */
   strcat(options, " 1 2 3");
#endif /* SIPLOAD */

   return CMSRET_SUCCESS;
}

CmsRet rutVoice_getSupportedConfOptions ( char * options, unsigned int length )
{
   if (options == NULL || length == 0)
      return CMSRET_SUCCESS;

   snprintf(options, length, MDMVS_LOCAL);
#ifdef SIPLOAD
   strcat(options, " ");
   strcat(options, MDMVS_REFERSERVER);
   strcat(options, " ");
   strcat(options, MDMVS_REFERPARTICIPANTS);
#endif /* SIPLOAD */

   return CMSRET_SUCCESS;
}

CmsRet rutVoice_getSupportedIpFamilyList ( char * ipFamilies, unsigned int length )
{
   if (ipFamilies == NULL || length == 0)
      return CMSRET_SUCCESS;

   /* prepare list of supported IP families */
   snprintf(ipFamilies, length, "%s ", MDMVS_IPV4);
#if VOICE_IPV6_SUPPORT
   strcat(ipFamilies, MDMVS_IPV6);
#endif

   return CMSRET_SUCCESS;
}

CmsRet rutVoice_getSupportedCodecs ( char * codecList, unsigned int length )
{
   unsigned int remainLen = length - 1; // Leave room for the null character

   /* Update the cache */
   rutVoice_updateCache();

   /* Initialize buffer */
   codecList[0] = '\0';

   /* Generate a comma separated list of supported codecs */
   for (int i = 0; i < codecCache.maxCodecs; i++)
   {
      char *pCodec = codecCache.codecList[i];

      if( 0 != strcmp(pCodec, "X_BROADCOM_COM_T38")
          && 0 != strcmp(pCodec, "X_BROADCOM_COM_NTE") )
      {
         if (remainLen >= strlen(pCodec) + 1)
         {
            if (i)
            {
               /* Can't use strncat here because gcc will throw a warning
                * that will be interpreted as an error. */
               strcat(codecList, ",");
               remainLen -= 1;
            }

            /* NULL character is always appended by strncat */
            strncat(codecList, pCodec, strlen(pCodec));
            remainLen -= strlen(pCodec);
         }
         else
         {
            /* Stop appending to the list */
            break;
         }
      }
   }

   return CMSRET_SUCCESS;
}

CmsRet rutVoice_validateAlpha3Locale ( char * locale, UBOOL8 * found )
{
   int i = 0;
   *found = 0;

   for(i = 0; i < ARRAY_SIZE(rutLocaleMap); i++)
   {
      if ( strcmp(locale, rutLocaleMap[i].cmTxt ) == 0 )
      {
         *found = 1;
         return CMSRET_SUCCESS;
      }
      i++;
   }
   return CMSRET_SUCCESS;
}

CmsRet rutVoice_validateAlpha2Locale ( char * locale, UBOOL8 * found )
{
   int i = 0;
   *found = 0;

   for(i = 0; i < ARRAY_SIZE(rutLocaleMap); i++)
   {
      if ( strcmp(locale, rutLocaleMap[i].tr104Txt) == 0 )
      {
         *found = 1;
         break;
      }
   }
   return CMSRET_SUCCESS;
}

CmsRet rutVoice_mapAlpha3toAlpha2Locale ( char * locale, UBOOL8 * found, unsigned int length )
{
   int i = 0;
   *found = 0;

   for(i = 0; i < ARRAY_SIZE(rutLocaleMap); i++)
   {
      if ( strncmp(locale,rutLocaleMap[i].cmTxt, length) == 0 )
      {
         *found = 1;
         strncpy(locale, rutLocaleMap[i].tr104Txt, length);
         break;
      }
   }
   return CMSRET_SUCCESS;
}

CmsRet rutVoice_mapAlpha2toVrg( const char *locale, int *id, UBOOL8 *found, unsigned int length )
{
   int i = 0;
   CmsRet  ret = CMSRET_INVALID_PARAM_VALUE;

   for(i = 0; i < ARRAY_SIZE(rutLocaleMap); i++)
   {
      if ( strncmp(locale,rutLocaleMap[i].tr104Txt, length) == 0 )
      {
         *found = 1;
         *id = rutLocaleMap[i].vrgId;
         return CMSRET_SUCCESS;
      }
   }
   return (ret);
}

CmsRet rutVoice_mapAlpha2toAlpha3( const char *locale, char *alpha3, UBOOL8 *found, unsigned int length )
{
   int i = 0;
   *found = 0;

   for(i = 0; i < ARRAY_SIZE(rutLocaleMap); i++)
   {
      if ( strncmp(locale,rutLocaleMap[i].tr104Txt, length ) == 0 )
      {
         *found = 1;
         strncpy(alpha3, rutLocaleMap[i].cmTxt, length);
         break;
      }
   }
   return CMSRET_SUCCESS;
}

CmsRet rutVoice_validateCodec ( char * codec, UBOOL8 * found )
{
   /* Update the cache */
   rutVoice_updateCache();

   /* Init flag */
   *found = 0;

   for (int i = 0; i < codecCache.maxCodecs; i++)
   {
      if (!strcmp(codecCache.codecList[i], codec))
      {
         *found = 1;
         break;
      }
   }

   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_getNumLines
**
**  PUROPOSE:       Gets number of lines that is configured for given vp.
**
**  INPUT PARMS:    vp. If vp = 0, we get the entire lines in the system
**
**  OUTPUT PARMS:   maxLine - max number of lines
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_getNumLines( int vp, int * numLine )
{
   CmsRet          ret      = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   VoiceProfObject *vpObj   = NULL;
   int             inst     = 0;

   /* Set default value of numLine (will be overwritten if correct voice profile is found */
   *numLine = 0;

   while ( (ret = cmsObj_getNextFlags(MDMOID_VOICE_PROF, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &vpObj)) == CMSRET_SUCCESS )
   {
      /* See if the retrieved voice profile number is the same vp we're looking for */
      inst = INSTANCE_ID_AT_DEPTH(&iidStack, DEPTH_OF_IIDSTACK(&iidStack) - 1);
      if ( inst == vp || vp == (intptr_t)NULL )
      {
         *numLine += (int)(vpObj->numberOfLines);
         cmsObj_free((void **) &vpObj);

         if( vp != (intptr_t)NULL )
         {
            /* Found the correct voice profile, can stop searching */
            break;
         }
      }

      cmsObj_free((void **) &vpObj);
   }


   return (CMSRET_SUCCESS);
}


CmsRet rutVoice_getMaxPrefCodecs( int * maxPrefCodecs )
{
   *maxPrefCodecs = VOICE_MAX_PREF_CODECS;
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_getNumSrvProv
**
**  PUROPOSE:       Gets number of voice profiles that has been configured.
**
**  INPUT PARMS:    None;
**
**  OUTPUT PARMS:   numSp - number of voice profiles
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_getNumSrvProv( int * numSp )
{
   *numSp = getNumberOfSrvProv();
   return CMSRET_SUCCESS;
}


/*****************************************************************************
**  FUNCTION:       rutVoice_getNumAccPerSrvProv
**
**  PUROPOSE:       Gets number of accounts associated with the given service
**                  provider.
**
**  INPUT PARMS:    spNum - service provider number
**
**  OUTPUT PARMS:   numAcnt - number of accounts associated with spNum
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_getNumAccPerSrvProv( int spNum, int * numAcnt )
{
   *numAcnt = getNumAccPerSrvProv( spNum );
   return CMSRET_SUCCESS;
}


/*****************************************************************************
**  FUNCTION:       rutVoice_mapSpNumToVpInst
**
**  PUROPOSE:       Gets the voice profile instance that is associated with the
**                  service provider number.
**
**  INPUT PARMS:    spNum - service provider number
**
**  OUTPUT PARMS:   vpInst - voice profile instance associated with spNum
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_mapSpNumToVpInst( int spNum, int * vpInst )
{
   *vpInst = getVpInstFromSpNum( spNum );
   return CMSRET_SUCCESS;
}


CmsRet rutVoice_getMaxSessPerLine( int * maxSess )
{
   /* Each fxs line is capable of conferenceing so max sess per line is 2 */
   *maxSess = 2;
   return CMSRET_SUCCESS;
}


CmsRet rutVoice_getSigProt( char * sigProt, unsigned int length )
{
#ifdef SIPLOAD
   snprintf(sigProt, length, "SIP");
#else /* MGCPLOAD */
   snprintf(sigProt, length, "MGCP");
#endif
   return CMSRET_SUCCESS;
}

#ifdef SIPLOAD
CmsRet rutVoice_getSipRole( char *role, unsigned int length )
{
   snprintf(role, length, "UserAgent");
   return CMSRET_SUCCESS;
}

CmsRet rutVoice_getSipExtensions( char *extensions, unsigned int length )
{
   snprintf(extensions, length, "INVITE,ACK,OPTIONS,BYE,CANCEL,REGISTER,REFER,INFO,"
         "PRACK,SUSBSCRIBE,NOTIFY,MESSAGE,PING,UPDATE,SERVICE");
   return CMSRET_SUCCESS;
}

CmsRet rutVoice_getSipTransports( char *transports, unsigned int length )
{
   snprintf(transports, length, "UDP");
   return CMSRET_SUCCESS;
}

CmsRet rutVoice_getSipUriSchemes( char *uriSchemes, unsigned int length )
{
   snprintf(uriSchemes, length, "sip");
   return CMSRET_SUCCESS;
}
#endif

UBOOL8 rut_validateFaxT38BitRate(UINT32 bitRate)
{
   UBOOL8 ret = TRUE;

   /* validate bitRate */
   if(bitRate != 2400 && bitRate != 4800 &&
      bitRate != 7200 && bitRate != 9600 &&
      bitRate != 12000 && bitRate != 14400 &&
      bitRate != 33600)
   {
      cmsLog_error("invalid bitRate value %d", bitRate);
      return FALSE;
   }

   return ret;
}

UBOOL8 rut_validateFaxT38HighSpeedPacketRate(UINT32 highSpeedPacketRate)
{
   UBOOL8 ret = TRUE;

   /* validate highspeed packet rate */
   if(highSpeedPacketRate != 10 && highSpeedPacketRate != 20 &&
      highSpeedPacketRate != 30 && highSpeedPacketRate != 40)
   {
      cmsLog_error("invalid highSpeedPacketRate value %d", highSpeedPacketRate);
      return FALSE;
   }

   return ret;
}

/*<START>================================= Common Helper Functions =======================================<START>*/

/*****************************************************************************
**  FUNCTION:       rutVoice_updateIfAddr
**
**  PUROPOSE:       Notify voice of interface IP changes. Updates the bound IP
**                  in the voice object if voice is currently bound to the
**                  given interface.
**
**  INPUT PARMS:    ifName - name of the interface whose address changed
**                  address - the new address string
**
**  OUTPUT PARMS:   none.
**
**  RETURNS:        CMSRET_SUCCESS if valid, error code otherwise
**
*****************************************************************************/
CmsRet rutVoice_updateIfAddr ( const char *ifName, const char *address )
{
   CmsRet ret = CMSRET_SUCCESS;

   /* TODO: This function should be removed and ensure caller sends the
    *       appropriate event to voice app instead. 
    */

   if (!ifName || !address)
      return CMSRET_INVALID_ARGUMENTS;

   cmsLog_debug("%s: ifName=%s, address=%s, ret=%d\n", __FUNCTION__, ifName, address, ret);
   return ret;
}


UBOOL8 rutIsVoipRunning(void)
{
   CmsMsgHeader msgHdr = EMPTY_MSG_HEADER;
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);
   CmsRet ret;

   cmsLog_debug("Entered: myEid=%d", myEid);

   /* voice_md acts as SMD in BDK */
   if (myEid == EID_SMD || myEid == EID_VOICE_MD)
   {
      /* Cannot send message to self, just assume active for now. */
      return TRUE;
   }

   msgHdr.dst = EID_SMD;
   msgHdr.src = cmsMsg_getHandleEid(msgHandle);
   msgHdr.type = CMS_MSG_IS_APP_RUNNING;
   msgHdr.flags_request = 1;
   msgHdr.wordData = EID_VOICE;

   ret = cmsMsg_sendAndGetReply(msgHandle, &msgHdr);
   if ( ret == CMSRET_SUCCESS )
      return TRUE;
   else if ( ret == CMSRET_OBJECT_NOT_FOUND )
      return FALSE;
   else
   {
      cmsLog_error("could not send CMS_MSG_IS_APP_RUNNING msg to smd, ret=%d", ret);
      return FALSE;
   }
}

UBOOL8 rutIsDectRunning(void)
{
   CmsMsgHeader msgHdr = EMPTY_MSG_HEADER;
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);
   CmsRet ret;

   cmsLog_debug("Entered: myEid=%d", myEid);

   /* voice_md acts as SMD in BDK */
   if (myEid == EID_SMD || myEid == EID_VOICE_MD)
   {
      /* Cannot send message to self, just assume active for now. */
      return TRUE;
   }

   msgHdr.dst = EID_SMD;
   msgHdr.src = cmsMsg_getHandleEid(msgHandle);
   msgHdr.type = CMS_MSG_IS_APP_RUNNING;
   msgHdr.flags_request = 1;
   msgHdr.wordData = EID_DECT;

   ret = cmsMsg_sendAndGetReply(msgHandle, &msgHdr);
   if ( ret == CMSRET_SUCCESS )
      return TRUE;
   else if ( ret == CMSRET_OBJECT_NOT_FOUND )
      return FALSE;
   else
   {
      cmsLog_error("could not send CMS_MSG_IS_APP_RUNNING msg to smd, ret=%d", ret);
      return FALSE;
   }
}

UBOOL8 rutIsLineAttachedWithDectHS( int handsetId )
{
   InstanceIdStack iidStack;
   void *obj;

   /* Check for line attached with dect handset */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while ( CMSRET_SUCCESS == cmsObj_getNext(MDMOID_VOICE_LINE_ATTACHED_HANDSET, &iidStack, &obj) )
   {
      if( ((_VoiceLineAttachedHandsetObject *)obj)->element & (1 << handsetId) )
      {
         cmsObj_free(&obj);
         return TRUE;
      }
      cmsObj_free(&obj);
   }
   return FALSE;
}

/*****************************************************************************
**  FUNCTION:       getVpInstFromCmAcntNum
**
**  PUROPOSE:       Retrieves voice profile instance from CM account number.
**
**
**  INPUT PARMS:    cmNum - CM account number
**
**  OUTPUT PARMS:   none.
**
**  RETURNS:        inst - voice profile instance; 0 if invalid
**
*****************************************************************************/
int getVpInstFromCmAcntNum( int cmNum )
{
   CmsRet          ret      = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   VoiceLineObject *lineObj = NULL;
   int             inst;

   while ( (ret = cmsObj_getNextFlags(MDMOID_VOICE_LINE, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &lineObj)) == CMSRET_SUCCESS )
   {
      /* If the stored CM account number == given account number, we can look
       * at iidStack to get the line instance number
       */
      if (lineObj->X_BROADCOM_COM_CMAcntNum == cmNum)
      {
         //the below line generates a compile warning (promoted to error now).
         //inst = INSTANCE_ID_AT_DEPTH(&iidStack, (DEPTH_OF_IIDSTACK(&iidStack) - 2));
         //rut_voice.c:1566: error: assuming signed overflow does not occur when assuming that (X - c) <= X is always true
         //changing to more suitable code:

         inst = (&iidStack)->instance[(DEPTH_OF_IIDSTACK(&iidStack) - 2)];
         cmsObj_free((void **) &lineObj);
         return inst;
      }

      cmsObj_free((void **) &lineObj);
   }

   return (0);
}
/*****************************************************************************
**  FUNCTION:       getNumberOfLineInstances
**
**  PUROPOSE:       Gets the total number of line instances created by TR104
**                  from MDM.
**
**  INPUT PARMS:    None
**
**  OUTPUT PARMS:   None
**
**  RETURNS:        count - total number of line instances created by TR104
**
*****************************************************************************/
int getNumberOfLineInstances(void)
{
   CmsRet          ret      = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   VoiceLineObject *lineObj = NULL;
   int             count    = 0;

   while ( (ret = cmsObj_getNextFlags(MDMOID_VOICE_LINE, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &lineObj)) == CMSRET_SUCCESS )
   {
      /* Increments count until cmsObj_getNext returns CMSRET_NO_MORE_INSTANCES */
      count++;
      cmsObj_free((void **) &lineObj);
   }

   return count;
}
/*****************************************************************************
**  FUNCTION:       getNumberOfSrvProv
**
**  PUROPOSE:       Gets the total number of line instances created by TR104
**                  from MDM.
**
**  INPUT PARMS:    None
**
**  OUTPUT PARMS:   None
**
**  RETURNS:        count - total number of line instances created by TR104
**
*****************************************************************************/
int getNumberOfSrvProv(void)
{
   CmsRet          ret      = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   VoiceProfObject *vpObj   = NULL;
   int             count    = 0;

   while ( (ret = cmsObj_getNextFlags(MDMOID_VOICE_PROF, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &vpObj)) == CMSRET_SUCCESS )
   {
      /* Increments count until cmsObj_getNext returns CMSRET_NO_MORE_INSTANCES */
      count++;
      cmsObj_free((void **) &vpObj);
   }

   return count;
}
/*****************************************************************************
**  FUNCTION:       getNumAccPerSrvProv
**
**  PUROPOSE:       Gets the total number of line instances created by TR104
**                  from MDM.
**
**  INPUT PARMS:    spNum
**
**  OUTPUT PARMS:   None
**
**  RETURNS:        profObj->numberOfLines - number of line instances associated with spNum
**
*****************************************************************************/
int getNumAccPerSrvProv( int spNum )
{
   CmsRet          ret      = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   VoiceProfObject *vpObj   = NULL;
   int             inst     = 0;
   int             numLines = 0;

   /* Get voice profile number from service provider number */
   int vp = getVpInstFromSpNum( spNum );

   while ( (ret = cmsObj_getNextFlags(MDMOID_VOICE_PROF, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &vpObj)) == CMSRET_SUCCESS )
   {
      /* See if the retrieved voice profile number is the same vp we're looking for */
      inst = INSTANCE_ID_AT_DEPTH(&iidStack, DEPTH_OF_IIDSTACK(&iidStack) - 1);
      if ( inst == vp )
      {
         numLines = (int)(vpObj->numberOfLines);
         cmsObj_free((void **) &vpObj);
         /* Found the correct voice profile, can stop searching */
         break;
      }

      cmsObj_free((void **) &vpObj);
   }

   return ( numLines );
}
/*****************************************************************************
**  FUNCTION:       getVpInstFromSpNum
**
**  PUROPOSE:       Gets the voice profile instance associated to the given
**                  service provider number
**
**  INPUT PARMS:    spNum
**
**  OUTPUT PARMS:   None
**
**  RETURNS:        inst - voice profile instance associated with spNum
**
*****************************************************************************/
int getVpInstFromSpNum( int spNum )
{
   CmsRet          ret      = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   VoiceProfObject *vpObj   = NULL;
   int             inst;

   if (spNum < 0)
   {
      cmsLog_error("invalid spNum\n");
      return (0);
   }

   while ( (ret = cmsObj_getNextFlags(MDMOID_VOICE_PROF, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &vpObj)) == CMSRET_SUCCESS )
   {
      /* If the stored service provider number == given account number, we can look
       * at iidStack to get the line instance number
       */
      if (vpObj->X_BROADCOM_COM_SPNum == spNum)
      {
         cmsObj_free((void **) &vpObj);
         break;
      }

      cmsObj_free((void **) &vpObj);
   }

   inst = INSTANCE_ID_AT_DEPTH(&iidStack, (DEPTH_OF_IIDSTACK(&iidStack) - 1));
   return inst;
}

/*****************************************************************************
**  FUNCTION:       getSpNumFromVpInst
**
**  PUROPOSE:       Gets the service provider number associated with the given
**                  voice profile instance.
**
**  INPUT PARMS:    vpInst - voice profile instance
**
**  OUTPUT PARMS:   none.
**
**  RETURNS:        spNum - service provider number; -1 if invalid
**
*****************************************************************************/
int getSpNumFromVpInst( int vpInst )
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   VoiceProfObject *vpObj   = NULL;
   int             spNum;
   int             inst;

   while ((ret = cmsObj_getNextFlags(MDMOID_VOICE_PROF, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &vpObj)) == CMSRET_SUCCESS)
   {
      inst = INSTANCE_ID_AT_DEPTH(&iidStack, DEPTH_OF_IIDSTACK(&iidStack) - 1);

      if ( inst == vpInst )
      {
         spNum = vpObj->X_BROADCOM_COM_SPNum;
         cmsObj_free((void **) &vpObj);

         return (spNum);
      }

      cmsObj_free((void **) &vpObj);
   }

   return -1;
}

/*****************************************************************************
**  FUNCTION:       getLineInstFromSpAcntNum
**
**  PUROPOSE:       Retrieves the line instance number from MDM that is associated
**                  with the given service provider number and account number
**
**
**  INPUT PARMS:    spNum - service provider number
**                  acntNum - account number associated to the spNum
**
**  OUTPUT PARMS:   none.
**
**  RETURNS:        inst - line instance number if successful, 0 otherwise
**
*****************************************************************************/
int getLineInstFromSpAcntNum( int spNum, int acntNum )
{
   CmsRet          ret         = CMSRET_SUCCESS;
   InstanceIdStack iidStack    = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack subIidStack = EMPTY_INSTANCE_ID_STACK;
   VoiceProfObject *vpObj      = NULL;
   VoiceLineObject *lineObj    = NULL;
   int             inst;

   while ( spNum >= 0 && ((ret =  cmsObj_getNextFlags(MDMOID_VOICE_PROF, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &vpObj)) == CMSRET_SUCCESS))
   {
      cmsObj_free((void**) &vpObj);
      spNum--;
   }

   /* If there are no more instances, return 0*/
   if ( ret == CMSRET_NO_MORE_INSTANCES )
   {
      cmsLog_error("error has occurred\n");
      return (0);
   }

   while ( acntNum >= 0 && ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_VOICE_LINE, &iidStack, &subIidStack, OGF_NO_VALUE_UPDATE, (void **) &lineObj)) == CMSRET_SUCCESS))
   {
      cmsObj_free((void**) &lineObj);
      acntNum--;
   }

   /* If there are no more instances, return 0*/
   if ( ret == CMSRET_NO_MORE_INSTANCES )
   {
      /* This is not necessary an error because the account may not be created yet
       * by TR104 but is present in call manager
       */
      cmsLog_debug("no more line instances\n");
      return (0);
   }

   inst = INSTANCE_ID_AT_DEPTH(&subIidStack, (DEPTH_OF_IIDSTACK(&subIidStack) - 1));

   return inst;
}


/*****************************************************************************
**  FUNCTION:       assignSpNumToVpInst
**
**  PUROPOSE:       Assign a service provider number to the voice profile instance
**
**  INPUT PARMS:    vpInst - voice profile instance
**
**  OUTPUT PARMS:   none.
**
**  RETURNS:        i - service provider number if successful, -1 otherwise
**
*****************************************************************************/
int assignSpNumToVpInst ( int vpInst )
{
   int i, vp = 0;

   for(i = 0; vp >= 0; i++)
   {
      vp = getVPInst(i);
      if(vp == vpInst)
         return i;
      }

   return (-1);
}

/*****************************************************************************
**  FUNCTION:       getVPInst
**
**  PUROPOSE:       Gets the voice profile instance according to how many times
**                  the caller wants to loop through the objects in MDM.
**
**  INPUT PARMS:    iteration - number of times the caller wants to loop through
**                              the voice profile objects in MDM
**
**  OUTPUT PARMS:   none.
**
**  RETURNS:        i - service provider number if successful, -1 otherwise
**
*****************************************************************************/
int getVPInst( int iteration )
{
   CmsRet          ret      = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   VoiceProfObject *vpObj   = NULL;
   int             inst;

   while ( iteration >= 0 && ((ret =  cmsObj_getNextFlags(MDMOID_VOICE_PROF, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &vpObj)) == CMSRET_SUCCESS))
   {
      cmsObj_free((void**) &vpObj);
      iteration--;
   }

   /* If there are no more instances, return -1 to fill in the mapping table*/
   if ( ret == CMSRET_NO_MORE_INSTANCES )
      return (-1);

   inst = INSTANCE_ID_AT_DEPTH(&iidStack, (DEPTH_OF_IIDSTACK(&iidStack) - 1));

   return inst;
}


/*****************************************************************************
**  FUNCTION:       getLineInstFromCmAcntNum
**
**  PUROPOSE:       Gets line instance number given a CM account number from
**                  MDM.
**
**
**  INPUT PARMS:    cmNum - CM account number
**
**
**  OUTPUT PARMS:   None
**
**  RETURNS:        inst - associated line instance number; 0 if invalid
**
*****************************************************************************/
int getLineInstFromCmAcntNum( int cmNum )
{
   CmsRet          ret      = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   VoiceLineObject *lineObj = NULL;
   int             inst;

   while ( (ret = cmsObj_getNextFlags(MDMOID_VOICE_LINE, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &lineObj)) == CMSRET_SUCCESS )
   {
      /* If the stored CM account number == given account number, we can look
       * at iidStack to get the line instance number
       */
      if (lineObj->X_BROADCOM_COM_CMAcntNum == cmNum)
      {
         inst = INSTANCE_ID_AT_DEPTH(&iidStack, (DEPTH_OF_IIDSTACK(&iidStack) - 1));
         cmsObj_free((void **) &lineObj);
         return inst;
      }

      cmsObj_free((void **) &lineObj);
   }

   return (0);
}


#if DMP_X_BROADCOM_COM_DECTENDPOINT_1
/** Retrieves the maximum number of DECT handset allowed to be supported
 *  on the software when the DECT interface is enabled.
 *
 * @param value (IN/OUT) the information to be retrieved.
 *
 * @return Nothing.
 */
void rutVoice_getMaxDectHset( int * value )
{
   *value = DECT_MAX_HANDSET_COUNT;
}


/** Retrieves total number of the current registered DECT handset
 *
 * @param value (IN/OUT) the information to be retrieved.
 *
 * @return Nothing.
 */
void rutVoice_getCurrDectHset( int * value )
{
   CmsRet          ret      = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   void           *obj = NULL;

   if(value == NULL)
   {
      cmsLog_error("Invalid arguments");
      return;
   }

   (*value) = 0;
   while ( (ret = cmsObj_getNextFlags(MDMOID_DECT_HANDSET, &iidStack, OGF_NO_VALUE_UPDATE, &obj)) == CMSRET_SUCCESS )
   {
      /* If the stored CM account number == given account number, we can look
       * at iidStack to get the line instance number
       */
      if (((_VoiceDECTHandsetObject *)obj)->X_BROADCOM_COM_ID != 0)
         (*value)++;
      cmsObj_free( &obj);
   }

   return;
}

/** Queues a delayed list update message for any changed DECT list
 *
 * @param delayId (IN) the id of the delay message, from the list in  dectctl.h
 * @param lineId (IN) the line id instance to which this update applies
 *
 * @return Nothing.
 */
void rutVoice_dectListUpdate( unsigned int delayId, int lineId )
{
   CmsMsgHeader *msg = NULL;
   RegisterDelayedMsgBody *body = NULL;
   char buf[sizeof(CmsMsgHeader) + sizeof(RegisterDelayedMsgBody)];
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);
   CmsRet ret = CMSRET_SUCCESS;

   /* If SMD was the one creating/deleting the list, no need to send an update */
   if(myEid == EID_SMD || myEid == EID_SSK)
      return;

   memset(buf, 0, sizeof(buf));

   msg = (CmsMsgHeader *) buf;
   body = (RegisterDelayedMsgBody *) (msg + 1);

   if(myEid == EID_DECT) /* DECT is the one adding it. */
   {

      msg->type = CMS_MSG_REGISTER_DELAYED_MSG;
      msg->src = cmsMsg_getHandleEid(msgHandle);
      msg->dst = EID_SMD;
      msg->flags_request = 1;
      msg->wordData = (delayId << 16) | lineId;

      msg->dataLength = sizeof(RegisterDelayedMsgBody);
      body->delayMs = 100; /* 1second delay */

      ret = cmsMsg_send(msgHandle, msg);
      cmsLog_debug("%s: ret = %d\n",__FUNCTION__, ret);
   }
   else if(myEid == EID_CONSOLED || myEid == EID_HTTPD) /* CLI/GUI is the one adding it. */
   {
      msg->type = CMS_MSG_VOICE_DECT_LIST_UPDATE;
      msg->src = cmsMsg_getHandleEid(msgHandle);
      msg->dst = EID_DECT;
      msg->flags_event = 1;
      msg->wordData = (delayId << 16) | lineId;

      msg->dataLength = 0;

      ret = cmsMsg_send(msgHandle, msg);
   }

   cmsLog_debug("%s: ret = %d\n",__FUNCTION__, ret);
}


#endif /* DMP_X_BROADCOM_COM_DECTENDPOINT_1 */

#endif /* BRCM_VOICE_SUPPORT */

