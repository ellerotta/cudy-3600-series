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
#ifdef DMP_VOICE_SERVICE_2

#include <fcntl.h>
#include <unistd.h>

#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rut_util.h"
#include "rut2_voice.h"
#include "mdm.h"

#include <bcm_country.h>         /* For VRG_COUNTRY_CFG defines */
#include <bcm_dsphal.h>

/* ---- Constants and Types ---------------------------------------------- */
#define DSPHAL_DRV_PATH    "/dev/" DSPHAL_DEV_NAME

#define ARRAY_SIZE(x)      (sizeof(x)/sizeof(*(x)))
#define TMP_BUFF_SIZE      128
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
         strncpy(codecCache.codecList[codecCache.maxCodecs], pCodec->codec,
                 CODEC_CACHE_MAX_STR_LEN - 1);
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

/*****************************************************************************
**  FUNCTION:       rutVoice_getMaxPhysEndpt
**
**  PURPOSE:        Gets total number of physical endpts
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
   int epts;
   int fp = open(DSPHAL_DRV_PATH, O_RDWR);

   epts = ioctl(fp, DSPHAL_CMD_GETNUMEPT);
   close(fp);

   *maxPhysEndpt = epts;
   return CMSRET_SUCCESS;
}


/*****************************************************************************
**  FUNCTION:       rutVoice_getNumFxsEndpt
**
**  PURPOSE:        Gets total number of FXO endpts
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
**  PURPOSE:        Gets total number of FXO endpts
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
**  PURPOSE:        Gets total number of DECT endpts
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
**  FUNCTION:       rutVoice_getDspHalDebugStats
**
**  PURPOSE:        Gets debug-related statistics from the DSP HAL
**
**  INPUT PARMS:    pointer to debug struct to be populated
**
**  OUTPUT PARMS:   debugStats - debug stats of interest
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_getDspHalDebugStats(RUT_VOICE_DEBUG_INFO *debugInfo)
{
   struct dsphal_debuginfo dsp_debug_info;

   if (debugInfo == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   int fp = open(DSPHAL_DRV_PATH, O_RDWR);

   ioctl(fp, DSPHAL_CMD_GETDSPDEBUGINFO, &dsp_debug_info);
   close(fp);

   debugInfo->dspHalUnderrunCount = dsp_debug_info.dsp_hal_underruns;
   debugInfo->peakDspHalUnderrunCount = dsp_debug_info.dsp_hal_underruns_peak;
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_mapFxoInterfaceIDToInst
**
**  PURPOSE:        Maps fxo interface id number TR104 PSTN instance number.
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

   return ret;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_mapL2ObjInstToNum
**
**  PURPOSE:        Maps any L2 Object instance id to number id ( 0 based )
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
CmsRet rutVoice_mapL2ObjInstToNum( MdmObjectId oid, InstanceIdStack *iidStack, int *Num )
{
    int  i = -1, L2_Inst = -1;
    CmsRet ret  = CMSRET_SUCCESS;
    InstanceIdStack  local_iidStack = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack  searchIidStack = EMPTY_INSTANCE_ID_STACK;
    void  *obj = NULL;

    local_iidStack = *(iidStack);
    L2_Inst = POP_INSTANCE_ID(&local_iidStack);
    ret = cmsObj_getNextInSubTreeFlags(oid, &local_iidStack, &searchIidStack, OGF_NO_VALUE_UPDATE, (void **)&obj);
    while( ret == CMSRET_SUCCESS ){
        cmsObj_free((void **)&obj );
        i++;
        if( L2_Inst == (PEEK_INSTANCE_ID(&searchIidStack))){
            break;
        }
        ret = cmsObj_getNextInSubTreeFlags(oid, &local_iidStack, &searchIidStack, OGF_NO_VALUE_UPDATE, (void **)&obj);
    }

    if( ret == CMSRET_SUCCESS )
    {
        *Num = i;
        cmsLog_debug("%s() map L2 object (%d) instance (%d) to number (%d)", __FUNCTION__, oid, L2_Inst, *Num );
    }
    else
    {
        cmsLog_error("%s() could not find L2 object (%d) instance (%d)", __FUNCTION__, oid, L2_Inst);
    }
    return ret;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_mapAcntNumToLineInst
**
**  PURPOSE:        Maps account number and vpInst to TR104 Line instance number.
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
CmsRet rutVoice_mapAcntNumToClientInst( int vpInst, int acntNum, int * lineInst )
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
**  PURPOSE:        Maps the voice profile instance and line instance to the
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
   return CMSRET_SUCCESS;
}


/*****************************************************************************
**  FUNCTION:       rutVoice_mapCmLineToVpInstLineInst
**
**  PURPOSE:        Maps CM account number to vpInst and line instance number.
**                  These objects are meaningless in TR104v2 but kept here to
**                  keep the API consistent.
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
   *lineInst = 0;
   *vpInst = 0;

   cmsLog_debug("cmNum:%d mapped to lineInst:%d, vpInst:%d", cmNum, *lineInst, *vpInst);

   return CMSRET_SUCCESS;
}


/*****************************************************************************
**  FUNCTION:       rutVoice_assignSpNumToVpInst
**
**  PURPOSE:        Assigns a service provider number to the vp instance
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
**  PURPOSE:        Updates CM account number for line instances upon deletion
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
   return CMSRET_SUCCESS;
}


/*****************************************************************************
**  FUNCTION:       rutVoice_updateSpNum
**
**  PURPOSE:        Updates service provider numbers for voice profiles upon deletion
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

   return CMSRET_SUCCESS;
}


/*****************************************************************************
**  FUNCTION:       rutVoice_assignCMAcnt
**
**  PURPOSE:        Assigns CM account numbers to line instances.  The CM account
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
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_getSupportedLocales
**
**  PURPOSE:        Obtains a list of supported locales
**
**
**  INPUT PARMS:    length - max length of the output string
**
**  OUTPUT PARMS:   localeList - list of supported locales
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
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

/*****************************************************************************
**  FUNCTION:       rutVoice_getDefaultAlpha3Locale
**
**  PURPOSE:        Obtains the default Alpla-3 (3-letter) locale
**
**
**  INPUT PARMS:    length - max length of the output string
**
**  OUTPUT PARMS:   locale - default locale
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
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

/*****************************************************************************
**  FUNCTION:       rutVoice_getSupportedAlpha2Locales
**
**  PURPOSE:        Obtains a list of supported 2-letter (TR-104) locales
**
**
**  INPUT PARMS:    length - max length of the output string
**
**  OUTPUT PARMS:   localeList - list of supported locales
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
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

/*****************************************************************************
**  FUNCTION:       rutVoice_getSupportedAlpha3Locales
**
**  PURPOSE:        Obtains a list of supported 3-letter locales
**
**
**  INPUT PARMS:    length - max length of the output string
**
**  OUTPUT PARMS:   localeList - list of supported locales
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
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

/*****************************************************************************
**  FUNCTION:       rutVoice_getSupportedTransports
**
**  PURPOSE:        Obtains a list of supported transport options
**
**
**  INPUT PARMS:    length - max length of the output string
**
**  OUTPUT PARMS:   transports - list of supported transports
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
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

/*****************************************************************************
**  FUNCTION:       rutVoice_getSupportedSrtpOptions
**
**  PURPOSE:        Obtains a list of supported SRTP options
**
**
**  INPUT PARMS:    length - max length of the output string
**
**  OUTPUT PARMS:   options - list of supported SRTP options
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_getSupportedSrtpOptions ( char * options, unsigned int length )
{
   if (options == NULL || length == 0)
      return CMSRET_SUCCESS;

#ifdef SIPLOAD
   /* Currently, SRTP is only supported under SIP */
#ifdef BRCM_VOICE_SRTP_SUPPORT
   strcat(options, MDMVS_OPTIONAL);
   strcat(options, " ");
   strcat(options, MDMVS_MANDATORY);
#endif /* BRCM_VOICE_SRTP_SUPPORT */
#endif /* SIPLOAD */

   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_getSupportedBackToPrimOptions
**
**  PURPOSE:        Obtains a list of supported back-to-primary options for failover
**
**
**  INPUT PARMS:    length - max length of the output string
**
**  OUTPUT PARMS:   options - list of supported back-to-primary options
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
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

/*****************************************************************************
**  FUNCTION:       rutVoice_getSupportedRedOptions
**
**  PURPOSE:        Obtains a list of supported Redundancy options
**
**
**  INPUT PARMS:    length - max length of the output string
**
**  OUTPUT PARMS:   options - list of supported Red options
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
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

/*****************************************************************************
**  FUNCTION:       rutVoice_getSupportedConfOptions
**
**  PURPOSE:        Obtains a list of supported conferencing options
**
**
**  INPUT PARMS:    length - max length of the output string
**
**  OUTPUT PARMS:   options - list of supported Conference options
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
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

/*****************************************************************************
**  FUNCTION:       rutVoice_getSupportedIpFamilyList
**
**  PURPOSE:        Obtains a list of supported IP families
**
**
**  INPUT PARMS:    length - max length of the output string
**
**  OUTPUT PARMS:   ipFamilies - list of supported IP address families 
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
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

/*****************************************************************************
**  FUNCTION:       rutVoice_validateAlpha3Locale
**
**  PURPOSE:        Validates a 3-letter locale string
**
**
**  INPUT PARMS:    locale - locale string to validate
**
**  OUTPUT PARMS:   none 
**
**  RETURNS:        CMSRET_SUCCESS if validated successfully
**                  CMSRET_INVALID_PARAM_VALUE otherwise
**
*****************************************************************************/
CmsRet rutVoice_validateAlpha3Locale ( char *locale )
{
   int i;

   for(i = 0; i < ARRAY_SIZE(rutLocaleMap); i++)
   {
      if ( strcmp(locale, rutLocaleMap[i].cmTxt ) == 0 )
      {
         return CMSRET_SUCCESS;
      }
   }
   return CMSRET_INVALID_PARAM_VALUE;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_validateAlpha2Locale
**
**  PURPOSE:        Validates a 2-letter locale string
**
**
**  INPUT PARMS:    locale - locale string to validate
**
**  OUTPUT PARMS:   none 
**
**  RETURNS:        CMSRET_SUCCESS if validated successfully
**                  CMSRET_INVALID_PARAM_VALUE otherwise
**
*****************************************************************************/
CmsRet rutVoice_validateAlpha2Locale ( char *locale )
{
   int i;

   for(i = 0; i < ARRAY_SIZE(rutLocaleMap); i++)
   {
      if ( strcmp(locale, rutLocaleMap[i].tr104Txt) == 0 )
      {
         return CMSRET_SUCCESS;
      }
   }
   return CMSRET_INVALID_PARAM_VALUE;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_mapAlpha3toAlpha2Locale
**
**  PURPOSE:        maps 3-letter locale to 2-letter locale
**
**
**  INPUT PARMS:    locale - locale string to map
**                  length - max length of output string
**
**  OUTPUT PARMS:   alpha2 - mapped 2-letter locale 
**
**  RETURNS:        CMSRET_SUCCESS if mapped successfully
**                  CMSRET_INVALID_PARAM_VALUE otherwise
**
*****************************************************************************/
CmsRet rutVoice_mapAlpha3toAlpha2Locale ( const char *locale, char *alpha2, unsigned int length )
{
   int i = 0;

   for(i = 0; i < ARRAY_SIZE(rutLocaleMap); i++)
   {
      if ( strncmp(locale,rutLocaleMap[i].cmTxt, length) == 0 )
      {
         strncpy(alpha2, rutLocaleMap[i].tr104Txt, length);
         return CMSRET_SUCCESS;
      }
   }
   return CMSRET_INVALID_PARAM_VALUE;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_mapAlpha3toAlpha2Locale
**
**  PURPOSE:        maps 3-letter locale to corresponding VRG enum
**
**
**  INPUT PARMS:    locale - locale string to map
**                  length - max length of input string
**
**  OUTPUT PARMS:   ID - mapped VRG enum
**                  found - TRUE if mapping has been found 
**
**  RETURNS:        CMSRET_SUCCESS if mapped successfully
**                  CMSRET_INVALID_PARAM_VALUE otherwise
**
*****************************************************************************/
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

/*****************************************************************************
**  FUNCTION:       rutVoice_mapAlpha2toAlph32Locale
**
**  PURPOSE:        maps 3-letter locale to 2-letter locale
**
**
**  INPUT PARMS:    locale - locale string to map
**                  length - max length of output string
**
**  OUTPUT PARMS:   alpha3 - mapped 2-letter locale 
**
**  RETURNS:        CMSRET_SUCCESS if mapped successfully
**                  CMSRET_INVALID_PARAM_VALUE otherwise
**
*****************************************************************************/
CmsRet rutVoice_mapAlpha2toAlpha3Locale( const char *locale, char *alpha3, unsigned int length )
{
   int i = 0;

   for(i = 0; i < ARRAY_SIZE(rutLocaleMap); i++)
   {
      if ( strncmp(locale,rutLocaleMap[i].tr104Txt, length ) == 0 )
      {
         strncpy(alpha3, rutLocaleMap[i].cmTxt, length);
         return CMSRET_SUCCESS;
      }
   }
   return CMSRET_INVALID_PARAM_VALUE;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_validateCodec
**
**  PURPOSE:        Validates codec against the list of available codecs in
**                  the capability codec.
**
**  INPUT PARMS:    codec - codec to validate
**
**  OUTPUT PARMS:   found - TRUE if codec has been found 
**
**  RETURNS:        CMSRET_SUCCESS 
**
*****************************************************************************/
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
**  FUNCTION:       rutVoice_getNumSrvProv
**
**  PURPOSE:        Gets number of voice profiles that has been configured.
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
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet    ret = CMSRET_SUCCESS;
    void     *obj = NULL;

    *numSp = 0;

    ret = cmsObj_getNextFlags( MDMOID_DEV2_SERVICES, &iidStack, OGF_NO_VALUE_UPDATE, &obj);
    if ( CMSRET_SUCCESS == ret )
    {
        *numSp = ((Dev2ServicesObject *)obj)->voiceServiceNumberOfEntries;
    }

    cmsObj_free(&obj);

    return ret;
}


/*****************************************************************************
**  FUNCTION:       rutVoice_getNumAccPerSrvProv
**
**  PURPOSE:        Gets number of accounts associated with the given service
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
    CmsRet   ret;
    void *obj = NULL;
    InstanceIdStack  iidStack = EMPTY_INSTANCE_ID_STACK;

    /* clear numAcnt */
    *numAcnt = 0;

    /* Get VoiceService.{i}. Object */
    ret = rutVoice_mapSpNumToSvcObject( spNum, &obj, &iidStack );
    if( CMSRET_SUCCESS == ret )
    {
        cmsObj_free(&obj); /* only need iidStack */
    }
    else
    {
        return ret;
    }

    /* Get VoiceService.{i}.SIP. object */
    ret = cmsObj_get( MDMOID_VOICE_SERVICE_SIP, &iidStack, OGF_NO_VALUE_UPDATE, &obj);
    if( CMSRET_SUCCESS == ret )
    {
        *numAcnt = ((VoiceServiceSipObject *)obj)->clientNumberOfEntries;
        cmsObj_free(&obj);
    }

    return ret;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_mapSpNumToSvcInst
**
**  PURPOSE:        Maps service profile number to instance number
**
**  INPUT PARMS:    spNum - service provider number
**
**  OUTPUT PARMS:   vpInst - mapped voice profile instance
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_mapSpNumToSvcInst( int spNum, int *vpInst)
{
    CmsRet          ret      = CMSRET_SUCCESS;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    VoiceObject *obj   = NULL;
    int   i;

    if (spNum < 0)
    {
        cmsLog_error("invalid spNum\n");
        return (CMSRET_INVALID_ARGUMENTS);
    }

    for( i = spNum; i >= 0 ; i-- )
    {
        ret = cmsObj_getNextFlags(MDMOID_VOICE, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &obj);
        if( ret == CMSRET_SUCCESS )
        {
            cmsLog_debug("%s successfully get voice service obj\n", __FUNCTION__);
            cmsObj_free((void **) &obj);
        }
        else
            break;
    }

    if( ret == CMSRET_SUCCESS )
    {
        *vpInst = PEEK_INSTANCE_ID(&iidStack );
        cmsLog_debug("%s map spNum (%d) to vpInst (%d)\n", __FUNCTION__, spNum, *vpInst);
    }

    return ret;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_mapSpNumToVpInst
**
**  PURPOSE:        Gets the voice service instance that is associated with the
**                  service provider number ( spNum = 0 .... N ).
**
**  INPUT PARMS:    spNum - service provider number
**
**  OUTPUT PARMS:   obj - voice service instance object
**                  iidStack - voice service instance stack
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_mapSpNumToSvcObject( int spNum, void **obj, InstanceIdStack *iidStack )
{
   CmsRet  ret;

   if( obj == NULL || spNum < 0 )
   {
      return  CMSRET_INVALID_ARGUMENTS;
   }

   do
   {
      ret = cmsObj_getNextFlags(MDMOID_VOICE, iidStack, OGF_NO_VALUE_UPDATE, obj);
      if( CMSRET_SUCCESS == ret )
      {
         spNum --;
         if(spNum >= 0 )
         {
            cmsObj_free(obj);
         }
      }
   } while ( ret == CMSRET_SUCCESS && spNum >= 0 );

   return ret;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_getSigProt
**
**  PURPOSE:        Gets signaling protocol (SIP or MGCP)
**
**  INPUT PARMS:    length - max length of output
**
**  OUTPUT PARMS:   sigProt - returned signaling protocol
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
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
/*****************************************************************************
**  FUNCTION:       rutVoice_getSipRole
**
**  PURPOSE:        Gets SIP role (userAgent)
**
**  INPUT PARMS:    length - max length of output
**
**  OUTPUT PARMS:   SIP role requested
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_getSipRole( char *role, unsigned int length )
{
   snprintf(role, length, "UserAgent");
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_getSipExtensions
**
**  PURPOSE:        Gets supported SIP methods
**
**  INPUT PARMS:    length - max length of output
**
**  OUTPUT PARMS:   SIP methods requested
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_getSipExtensions( char *extensions, unsigned int length )
{
   snprintf(extensions, length, "INVITE,ACK,OPTIONS,BYE,CANCEL,REGISTER,REFER,INFO,"
         "PRACK,SUSBSCRIBE,NOTIFY,MESSAGE,PING,UPDATE,SERVICE");
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_getSipTransports
**
**  PURPOSE:        Gets supported SIP transports
**
**  INPUT PARMS:    length - max length of output
**
**  OUTPUT PARMS:   transports - supported SIP transports
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_getSipTransports( char *transports, unsigned int length )
{
   snprintf(transports, length, "UDP");
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rutVoice_getSipUriSchemes
**
**  PURPOSE:        Gets supported SIP URI schemes
**
**  INPUT PARMS:    length - max length of output
**
**  OUTPUT PARMS:   uriSchemes - supported SIP URI schemes 
**
**  RETURNS:        CMSRET_SUCCESS
**
*****************************************************************************/
CmsRet rutVoice_getSipUriSchemes( char *uriSchemes, unsigned int length )
{
   snprintf(uriSchemes, length, "sip");
   return CMSRET_SUCCESS;
}
#endif

/*****************************************************************************
**  FUNCTION:       rut_validateFaxT38BitRate
**
**  PURPOSE:        validates a fax bit rate against supported ones
**
**  INPUT PARMS:    bitRate - bit rate to validate
**
**  OUTPUT PARMS:    
**
**  RETURNS:        True if validated successfully, False otherwise
**
*****************************************************************************/
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

/*****************************************************************************
**  FUNCTION:       rut_validateFaxT38HighSpeedPacketRate
**
**  PURPOSE:        validates a fax packet rate against supported ones
**
**  INPUT PARMS:    bitRate - High speed packet rate to validate
**
**  OUTPUT PARMS:   
**
**  RETURNS:        True if validated successfully, False otherwise
**
*****************************************************************************/
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
**  PURPOSE:        Notify voice of interface IP changes. Updates the bound IP
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

/*****************************************************************************
**  FUNCTION:       rutIsVoipRunning
**
**  PURPOSE:        Checks if the voice application is running
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        True if voice is running, False otherwise
**
*****************************************************************************/
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

/*****************************************************************************
**  FUNCTION:       rutIsDectRunning
**
**  PURPOSE:        Checks if the DECT application is running
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        True if DECT is running, False otherwise
**
*****************************************************************************/
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
   return FALSE;
}

/*****************************************************************************
**  FUNCTION:       getNumAccPerSrvProv
**
**  PURPOSE:        Gets the total number of line instances created by TR104
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
   return ( 0 );
}

/*****************************************************************************
**  FUNCTION:       getSpNumFromVpInst
**
**  PURPOSE:        Gets the service provider number associated with the given
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
   return -1;
}

/*****************************************************************************
**  FUNCTION:       getLineInstFromSpAcntNum
**
**  PURPOSE:        Retrieves the line instance number from MDM that is associated
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

   return -1;
}


/*****************************************************************************
**  FUNCTION:       assignSpNumToVpInst
**
**  PURPOSE:        Assign a service provider number to the voice profile instance
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
**  PURPOSE:        Gets the voice profile instance according to how many times
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
   return -1;
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
   (*value) = 0;

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

#endif /* DMP_VOICE_SERVICE_2 */
#endif /* BRCM_VOICE_SUPPORT */

