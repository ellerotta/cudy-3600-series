/***********************************************************************
 *
 *  Copyright (c) 2019  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2019:proprietary:standard

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

#include <gio/gio.h>
#include <glib/gprintf.h>
#include <glib-unix.h>

#include "cms_mdm.h"
#include "cms_obj.h"
#include "cms_phl.h"
#include "cms_lck.h"
#include "cms_util.h"
#include "cms_retcodes.h"
#include "bcm_ulog.h"
#include "bcm_zbus_intf.h"
#include "bcm_zbus_mdm.h"

// this file uses defines associated with the BCM Generic HAL,
// but does not use BCM Generic HAL directly.
#include "bcm_generic_hal_defs.h"
#include "bcm_generic_hal_utils.h"


/*!\file dbus_strproto.c
 * \brief D-Bus handlers for inbound generic string protocol methods.  The
 *        functions in this file just unwraps the D-Bus GVariant stuff off
 *        of the actual parameters and calls the zbus strproto functions in
 *        bcm_zbus_mdm to process them.  If there is return data, these
 *        functions wrap the DBus GVariant stuff back on the data.
 *
 */

/* ---- Private Functions -------------------------------------------------- */

static gboolean
is_path_name_end_with_instance_number_no_dot(const char *fullpath)
{
   gboolean ret = FALSE;
   guint32 len = 0;

   len = strlen(fullpath);
   if (len == 0)
   {
      g_print("NULL or zero length fullpath.\n");
      return ret;
   }

   if (isdigit(fullpath[--len]))
   {
      for (len--;
           len != 0 &&
           fullpath[len] != '.' &&
           isdigit(fullpath[len]);
           len--)
         ;

      if (fullpath[len] == '.')
         ret = TRUE;
   }

   return ret;
}


void dbus_in_setParameterValues(void *input, void **output)
{
    GVariant *parameters = (GVariant *) input;
    GVariant **result = (GVariant **) output;
    GVariantIter *iter = NULL;
    guint32 flags=0;
    GVariantBuilder *builder = NULL;
    gchar *response = NULL;
    BcmGenericParamInfo *paramInfoArray = NULL;
    UINT32 count = 0;
    UINT32 i=0;
    BcmRet ret = BCMRET_SUCCESS;

    bcmuLog_debug("Entered");

    /* retrieve all input parameters */
    g_variant_get (parameters, "(a(sss)u)", &iter, &flags);
    count = (UINT32) g_variant_iter_n_children(iter);

    /* initialize return variant builder */
    builder = g_variant_builder_new (G_VARIANT_TYPE ("a(su)"));


    /* return error when SetParameterValues is called with 0 parameters  */
    if (count == 0)
    {
        const char *errorMsg = "setParameterValues is called with 0 params!";
        bcmuLog_error(errorMsg);
        response = g_strdup_printf ("%s", errorMsg);
        *result = g_variant_new ("(a(su)su)", builder, response, BCMRET_INVALID_ARGUMENTS);
        g_variant_iter_free(iter);
        g_variant_builder_unref(builder);
        g_free(response);
        return;
    }

    /* allocate memory for BcmGenericParamInfo array*/
    paramInfoArray = (BcmGenericParamInfo *) cmsMem_alloc(sizeof(BcmGenericParamInfo)*count, ALLOC_ZEROIZE);
    if (paramInfoArray == NULL)
    {
        response = g_strdup_printf ("Failed to allocate %d BcmGenericParamInfo", count);
        *result = g_variant_new ("(a(su)su)", builder, response, BCMRET_RESOURCE_EXCEEDED);
        g_variant_iter_free (iter);
        g_variant_builder_unref (builder);
        g_free (response);
        return;
    }

    /* fill in BcmGenericParamInfo array based on input data */
    for (i=0; i < count; i++)
    {
        gchar *fullpath = NULL, *strType=NULL, *strValue=NULL;
        if (g_variant_iter_next(iter, "(&s&s&s)", &fullpath, &strType, &strValue))
        {
            paramInfoArray[i].fullpath = cmsMem_strdup(fullpath);
            paramInfoArray[i].type = cmsMem_strdup(strType);
            paramInfoArray[i].value = cmsMem_strdup(strValue);
        }
    }
    g_variant_iter_free (iter);


    /*
     * Call into Z-Bus generic method, and wrap up returned data in D-Bus.
     */
    ret = zbus_in_setParameterValues(paramInfoArray, count, flags);
    if (ret == BCMRET_SUCCESS)
    {
        response = g_strdup_printf ("SetParameterValues success.");
        *result = g_variant_new ("(a(su)su)", builder, response, ret);
    }
    else
    {
        for (i = 0 ; i < count ; i++)
        {
            // Insert (fullpath, errorCode) into builder for any param that
            // had an error.
            if (paramInfoArray[i].errorCode != BCMRET_SUCCESS)
            {
                g_variant_builder_add(builder, "(su)",
                                      paramInfoArray[i].fullpath,
                                      paramInfoArray[i].errorCode);
            }
        }
        response = g_strdup_printf ("SetParameterValues had errors");
        *result = g_variant_new ("(a(su)su)", builder, response, ret);
    }

    /* free paramInfoArray array */
    bcmuLog_debug("done ret=%d, freeing paramValues", ret);
    cmsUtl_freeParamInfoArray(&paramInfoArray, count);

    g_free (response);
    g_variant_builder_unref (builder);
    return;
}


void fillParamInfoIntoBuilder(const BcmGenericParamInfo *paramInfo,
                              GVariantBuilder    *builder)
{
    g_variant_builder_add (builder, "(ssssbb)",
                           paramInfo->fullpath,
                           paramInfo->type,
                           paramInfo->value,
                           paramInfo->profile,
                           paramInfo->writable,
                           paramInfo->isPassword);
    return;
}

void dbus_in_getParameterValues(void *input, void **output)
{
    GVariant *parameters = (GVariant *) input;
    GVariant **result = (GVariant **) output;
    guint32 flags=0;
    gboolean nextLevelOnly = FALSE;
    gchar *response = NULL;
    GVariantIter *iter = NULL;
    GVariantBuilder *builder = NULL;
    const gchar *fullpath = NULL;
    char       **fullpathArray=NULL;
    BcmGenericParamInfo *paramInfoArray=NULL;
    UINT32              numParamInfos = 0;
    UINT32              count = 0;
    UINT32              i = 0;
    BcmRet ret = BCMRET_SUCCESS;

#ifdef TIMESTAMP
    gint64 time1, time2;

    time1 = g_get_real_time();
#endif

   bcmuLog_debug("Entered");

    /* retrieve all input params */
    g_variant_get (parameters, "(asbu)", &iter, &nextLevelOnly, &flags);
    count = (UINT32) g_variant_iter_n_children(iter);

    /* initialize return variant builder */
    builder = g_variant_builder_new (G_VARIANT_TYPE ("a(ssssbb)"));

    /* return error when GetParameterValues is called with 0 parameters  */
    if (count == 0)
    {
        const char *errorMsg = "getParameterValues is called with 0 params!";
        bcmuLog_error(errorMsg);
        response = g_strdup_printf ("%s", errorMsg);
        *result = g_variant_new ("(a(ssssbb)su)", builder, response, BCMRET_INVALID_ARGUMENTS);
        g_variant_iter_free (iter);
        g_variant_builder_unref (builder);
        g_free (response);
        return;
    }

    // Allocate count fullpaths
    fullpathArray = cmsMem_alloc(count * sizeof(char *), ALLOC_ZEROIZE);
    if (fullpathArray == NULL)
    {
        response = g_strdup_printf ("Alloc of %d char * failed", count);
        *result = g_variant_new ("(a(ssssbb)su)", builder, response, BCMRET_INVALID_ARGUMENTS);
        g_variant_iter_free (iter);
        g_variant_builder_unref (builder);
        g_free (response);
        return;
    }

    /* loop through list of fullpaths and copy into array */
    while (g_variant_iter_loop (iter, "&s", &fullpath))
    {
        /*
         * object name of object instance should have
         * Dot (.) at the end, otherwise this function
         * should return CMSRET_INVALID_PARAM_NAME.
         * Since cmsMdm_fullPathToPathDescriptor accepts
         * full path that does not have Dot (.) at the end,
         * isPathNameEndWithInstanceNumberNoDot() is used
         * to validate this format.
         * TODO: this check should be done at the TR69 level or inside the MDM.
         * This layer should only repackage data from DBus to ZBus.
         */
        if (is_path_name_end_with_instance_number_no_dot(fullpath) == TRUE)
        {
            response = g_strdup_printf ("invalid param name %s", fullpath);
            *result = g_variant_new ("(a(ssssbb)su)", builder, response, BCMRET_INVALID_PARAM_NAME);
            g_variant_iter_free (iter);
            g_variant_builder_unref (builder);
            g_free (response);
            cmsUtl_freeArrayOfStrings(&fullpathArray, count);
            return;
        }

        fullpathArray[i] = cmsMem_strdup(fullpath);
        i++;
    }
    g_variant_iter_free (iter);


#ifdef TIMESTAMP
    time2 = g_get_real_time();
#endif

    /*
     * Call Z-Bus generic method.
     */
    ret = zbus_in_getParameterValues((const char **) fullpathArray, count,
                                     nextLevelOnly, flags,
                                     &paramInfoArray, &numParamInfos);

#ifdef TIMESTAMP
    time3 = g_get_real_time();
#endif

    cmsUtl_freeArrayOfStrings(&fullpathArray, count);

    if (ret == BCMRET_SUCCESS)
    {
       for (i = 0; i < numParamInfos; i++)
       {
           fillParamInfoIntoBuilder(&(paramInfoArray[i]), builder);
       }
       response = g_strdup_printf ("GetParameterValues returning %d params.",
                                    numParamInfos);
    }
    else
    {
        response = g_strdup_printf ("Error from zbus_in_getParameterValues, ret=%d", ret);
    }

   cmsUtl_freeParamInfoArray(&paramInfoArray, numParamInfos);

#ifdef TIMESTAMP
    time4 = g_get_real_time();
#endif

    *result = g_variant_new ("(a(ssssbb)su)", builder, response, ret);
    g_free (response);
    g_variant_builder_unref (builder);

#ifdef TIMESTAMP
    time5 = g_get_real_time();
    g_print("%s start - [%lld us][0]\n", __func__, time1);
    g_print("%s cal phl - [%lld us][%lld us]\n", __func__, time2, time2-time1);
    g_print("%s phl done - [%lld us][%lld us]\n", __func__, time3, time3-time2);
    g_print("%s return - [%lld us]\n", __func__, time4);
    g_print("%s finish return- [%lld us][%lld us]\n", __func__, time5, time5-time4);
#endif
    return;
}


void dbus_in_getParameterNames(void *input, void **output)
{
    GVariant *parameters = (GVariant *) input;
    GVariant **result = (GVariant **) output;
    const gchar *strPath = NULL;
    MdmPathDescriptor strPathDesc = EMPTY_PATH_DESCRIPTOR;
    gboolean nextLevelOnly = FALSE;
    guint32 flags=0;
    GVariantBuilder *builder = NULL;
    gchar *response = NULL;
    BcmGenericParamInfo *info = NULL;
    BcmGenericParamInfo *paramInfoArray = NULL;
    UINT32 numParamInfos=0;
    CmsRet ret = CMSRET_SUCCESS;

#ifdef TIMESTAMP
    GTimeVal   time1, time2, time3, time4;
    g_get_current_time(&time1);
#endif

   bcmuLog_debug("Entered");

    /* retrieve all input params */
    g_variant_get (parameters, "(&sbu)", &strPath, &nextLevelOnly, &flags);

     /* initialize return variant builder.
      * a(sssbb): fullpath_type_profile_writable_isPassword_array
      */
    builder = g_variant_builder_new (G_VARIANT_TYPE ("a(sssbb)"));

#ifdef TIMESTAMP
    g_get_current_time(&time2);
#endif

    ret = cmsMdm_fullPathToPathDescriptor(strPath, &strPathDesc);
    if ( ret != CMSRET_SUCCESS)
    {
        response = g_strdup_printf ("%s is invalid path.", strPath);
        *result = g_variant_new ("(a(sssbb)su)", builder, response, ret);
        g_free (response);
        g_variant_builder_unref (builder);
        return;
    }

    /*
     * Call into Z-Bus and encapsulate returned data into D-Bus builder.
     */
    ret = (CmsRet) zbus_in_getParameterNames(strPath, nextLevelOnly, flags,
                                    &paramInfoArray, &numParamInfos);

    if (ret == CMSRET_SUCCESS)
    {
       UINT32 p;
       for (p=0; p < numParamInfos; p++)
       {
            // old comments: seems to deal with special cases from CDRouter?
            // they should be handled in TR69.  This code just does
            // simple translation between D-Bus and CMS PHL.
            //
            // if nextLevelOnly is true, the first parameter name that matches
            // the given partial path object name should NOT be included
            // in the GetParameterNamesResponse
            //
            /* if ParameterPath is empty, with NextLevel is true, the response
             * should list only "InternetGatewayDevice.".
              */
          info = &(paramInfoArray[p]);
          g_variant_builder_add(builder, "(sssbb)",
                                info->fullpath, info->type, info->profile,
                                info->writable, info->isPassword);
       }
       cmsUtl_freeParamInfoArray(&paramInfoArray, numParamInfos);
    }

#ifdef TIMESTAMP
    g_get_current_time(&time3);
#endif

    if (ret == CMSRET_SUCCESS)
    {
        /* initialize response */
        response = g_strdup_printf ("GetParameterNames success, " \
                                    "returning %d parameters.", numParamInfos);
    }
    else
    {
        response = g_strdup_printf ("getParameterNames failed (%s)", strPath);
    }
    *result = g_variant_new ("(a(sssbb)su)", builder, response, ret);

#ifdef TIMESTAMP
    g_get_current_time(&time4);
    g_print("cwmpd: A - [%ld s, %ld us]\n", time1.tv_sec, time1.tv_usec);
    g_print("cwmpd: B - [%ld s, %ld us]\n", time2.tv_sec, time2.tv_usec);
    g_print("cwmpd: C - [%ld s, %ld us]\n", time3.tv_sec, time3.tv_usec);
    g_print("cwmpd: D - [%ld s, %ld us]\n", time4.tv_sec, time4.tv_usec);
#endif

    g_free (response);
    g_variant_builder_unref (builder);
    return;
}

void dbus_in_setParameterAttributes(void *input, void **output)
{
    GVariant *parameters = (GVariant *) input;
    GVariant **result = (GVariant **) output;
    GVariantIter *iter = NULL;
    GVariantBuilder *builder = NULL;
    guint32 flags = 0;
    UINT32 count = 0;
    const gchar *fullpath = NULL;
    gboolean setAccess=0, setNotif=0, setAltNotif=0, clearAltValue=0;
    guint16 access=0, notif=0, altNotif=0;
    BcmGenericParamAttr *paramAttrArray = NULL;
    UINT32 i = 0;
    gchar *response = NULL;
    CmsRet ret = CMSRET_SUCCESS;

    bcmuLog_debug("Entered:");

    /* retrieve all input parameters */
    g_variant_get (parameters, "(a(sbqbqbqb)u)", &iter, &flags);

    /* create a return builder for the fullpath_error array.
     * TODO: this array is never fill out, so always empty for now.
     */
    builder = g_variant_builder_new (G_VARIANT_TYPE ("a(su)"));

    /* return error if called with 0 parameters  */
    count = (UINT32) g_variant_iter_n_children(iter);
    if (count == 0)
    {
        const char *errorMsg = "setParameterAttributes is called with 0 fullpaths!";
        bcmuLog_error(errorMsg);
        response = g_strdup_printf ("%s", errorMsg);
        ret = CMSRET_INVALID_ARGUMENTS;
    }
    else
    {
        /* allocate memory for array of set param values */
        paramAttrArray = (BcmGenericParamAttr *) cmsMem_alloc(sizeof(BcmGenericParamAttr)*count, ALLOC_ZEROIZE);
        if (paramAttrArray == NULL)
        {
            response = g_strdup_printf ("Could not allocate %d BcmGenericParamAttr", count);
            ret = CMSRET_RESOURCE_EXCEEDED;
        }
    }

    if (ret != CMSRET_SUCCESS)
    {
        *result = g_variant_new ("(a(su)su)", builder, response, ret);
        g_variant_iter_free (iter);
        g_variant_builder_unref (builder);
        g_free (response);
        cmsUtl_freeParamAttrArray(&paramAttrArray, count);
        return;
    }

    /* transfer args from D-Bus into BcmGenericParamAttr */
    i = 0;
    while (g_variant_iter_loop(iter, "(&sbqbqbqb)",
                               &fullpath, &setAccess, &access,
                               &setNotif, &notif, &setAltNotif, &altNotif,
                               &clearAltValue))
    {
        paramAttrArray[i].fullpath = cmsMem_strdup(fullpath);
        paramAttrArray[i].setAccess = setAccess;
        paramAttrArray[i].access = access;
        paramAttrArray[i].setNotif = setNotif;
        paramAttrArray[i].notif = notif;
        paramAttrArray[i].setAltNotif = setAltNotif;
        paramAttrArray[i].altNotif = altNotif;
        paramAttrArray[i].clearAltNotifValue = clearAltValue;
        i++;
    }
    g_variant_iter_free (iter);

    /*
     * Call into Z-Bus generic method, and wrap up returned data in D-Bus.
     */
    ret = (CmsRet) zbus_in_setParameterAttributes(paramAttrArray, count, flags);
    if (ret == CMSRET_SUCCESS)
    {
        response = g_strdup_printf ("SetParameterAttrs is performed completely.");
    }
    else
    {
        //TODO: fill out the array of table: {"fullpath", "error"}
        response = g_strdup_printf ("SetParameterAttrs error, ret=%d", ret);
    }

    *result = g_variant_new ("(a(su)su)", builder, response, ret);
    g_variant_builder_unref (builder);
    g_free (response);
    cmsUtl_freeParamAttrArray(&paramAttrArray, count);
    bcmuLog_debug("Exit, ret=%d", ret);
    return;
}


void dbus_in_getParameterAttributes(void *input, void **output)
{
    GVariant *parameters = (GVariant *) input;
    GVariant **result = (GVariant **) output;
    UINT32 count = 0;
    guint32 flags = 0;
    const gchar *fullpath = NULL;
    char **fullpathArray = NULL;
    gchar *response = NULL;
    GVariantIter *iter = NULL;
    gboolean nextLevelOnly = FALSE;
    GVariantBuilder *builder = NULL;
    BcmGenericParamAttr *paramAttrArray = NULL;
    UINT32 numParamAttrs = 0;
    UINT32 i = 0;
    CmsRet ret = CMSRET_SUCCESS;

    bcmuLog_debug("Entered:");

    /* retrieve all input params */
    g_variant_get (parameters, "(asbu)", &iter, &nextLevelOnly, &flags);
    count = (UINT32) g_variant_iter_n_children(iter);

    /* initialize return variant builder */
    builder = g_variant_builder_new (G_VARIANT_TYPE ("a(sqqqqq)"));

    /* return error when GetParameterAttributes is called with 0 parameters  */
    if (count == 0)
    {
        const char *errorMsg = "getParameterAttributes is called with 0 fullpaths!";
        bcmuLog_error(errorMsg);
        response = g_strdup_printf ("%s", errorMsg);
        ret = CMSRET_INVALID_ARGUMENTS;
    }
    else
    {
        /* allocate memory for array of char ptrs */
        fullpathArray = cmsMem_alloc(count * sizeof(char *), ALLOC_ZEROIZE);
        if (fullpathArray == NULL)
        {
            response = g_strdup_printf ("Alloc of %d char ptrs failed", count);
            ret = CMSRET_RESOURCE_EXCEEDED;
        }
    }

    if (ret != CMSRET_SUCCESS)
    {
        *result = g_variant_new ("(a(sqqqqq)su)", builder, response, ret);
        g_variant_iter_free (iter);
        g_variant_builder_unref (builder);
        g_free (response);
        return;
    }

    /* loop through list of fullpaths and copy into array */
    while (g_variant_iter_loop (iter, "&s", &fullpath))
    {
        if (is_path_name_end_with_instance_number_no_dot(fullpath) == TRUE)
        {
            ret = CMSRET_INVALID_PARAM_NAME;
            bcmuLog_error("bad fullpath (no dot) %s", fullpath);
            response = g_strdup_printf ("%s ends with instance number no dot", fullpath);
        }

        if (ret != CMSRET_SUCCESS)
        {
            *result = g_variant_new ("(a(sqqqqq)su)", builder, response, ret);
            g_variant_iter_free (iter);
            g_variant_builder_unref (builder);
            g_free (response);
            cmsUtl_freeArrayOfStrings(&fullpathArray, count);
            return;
        }

        fullpathArray[i] = cmsMem_strdup(fullpath);
        i++;
    }
    g_variant_iter_free (iter);

   /*
    * Call Z-Bus generic method.
    */
    ret = (CmsRet) zbus_in_getParameterAttributes((const char **) fullpathArray, count,
                                         nextLevelOnly, flags,
                                         &paramAttrArray, &numParamAttrs);

    cmsUtl_freeArrayOfStrings(&fullpathArray, count);

    if (ret == CMSRET_SUCCESS)
    {
        /* copy MDM data to variant builder */
        for (i = 0; i < numParamAttrs; i++)
        {
            g_variant_builder_add(builder, "(sqqqqq)",
                                  paramAttrArray[i].fullpath,
                                  paramAttrArray[i].access,
                                  paramAttrArray[i].notif,
                                  paramAttrArray[i].valueChanged,
                                  paramAttrArray[i].altNotif,
                                  paramAttrArray[i].altNotifValue);
        }
        cmsUtl_freeParamAttrArray(&paramAttrArray, numParamAttrs);
        response = g_strdup_printf ("GetParameterAttributes success, return %d attrs", numParamAttrs);
    }
    else
    {
        response = g_strdup_printf ("Error during call to getParameterAttributes");
    }

    *result = g_variant_new ("(a(sqqqqq)su)", builder, response, ret);

    g_free (response);
    g_variant_builder_unref (builder);
    bcmuLog_debug("Exit, ret=%d", ret);
    return;
}

void dbus_in_addObject(void *input, void **output)
{
    GVariant *parameters = (GVariant *) input;
    GVariant **result = (GVariant **) output;
    gchar *fullpath = NULL;
    gchar *response = NULL;
    guint32 instance = 0, flags = 0;
    CmsRet ret = CMSRET_INVALID_ARGUMENTS;

    bcmuLog_debug("Entered:");

    /* retrieve all input parameters */
    g_variant_get (parameters, "(&su)", &fullpath, &flags);

    /*
     * Call Z-Bus generic method to do the real work.
     */
    ret = (CmsRet) zbus_in_addObject(fullpath, flags, &instance);

    if (ret == CMSRET_SUCCESS)
    {
        response = g_strdup_printf("Add object of %s OK!", fullpath);
    }
    else if (ret == CMSRET_SUCCESS_REBOOT_REQUIRED)
    {
        response = g_strdup_printf("Add object of %s OK, but reboot required.", fullpath);
    }
    else
    {
        response = g_strdup_printf("Add object of %s failed! ret=%d",
                                   fullpath, ret);
    }

    *result = g_variant_new ("(usu)", instance, response, ret);

    g_free (response);
    bcmuLog_notice("exit: ret=%d", ret);
    return;
}

void dbus_in_deleteObject(void *input, void **output)
{
    GVariant *parameters = (GVariant *) input;
    GVariant **result = (GVariant **) output;
    gchar *response = NULL;
    gchar *fullpath = NULL;
    guint32 flags = 0;
    CmsRet ret = CMSRET_INVALID_ARGUMENTS;

    bcmuLog_debug("Entered:");

    /* retrieve all input parameters */
    g_variant_get (parameters, "(&su)", &fullpath, &flags);

    /*
     * Call Z-Bus generic method to do the real work.
     */
    ret = (CmsRet) zbus_in_deleteObject(fullpath, flags);

    if (ret == CMSRET_SUCCESS)
    {
        response = g_strdup_printf("Delete of %s OK!", fullpath);
    }
    else if (ret == CMSRET_SUCCESS_REBOOT_REQUIRED)
    {
        response = g_strdup_printf("Delete of %s OK, but reboot required.",
                                   fullpath);
    }
    else
    {
        response = g_strdup_printf("Delete of %s failed! ret=%d",
                                   fullpath, ret);
    }

    *result = g_variant_new ("(su)", response, ret);

    g_free (response);
    bcmuLog_notice("exit: ret=%d", ret);
    return;
}


void dbus_in_getNextObject(void *input, void **output)
{
    GVariant *parameters = (GVariant *) input;
    GVariant **result = (GVariant **) output;
    gchar *fullpath, *response = NULL;
    gchar *limitSubtree = NULL;
    gchar *nextFullpath = NULL;
    GVariantBuilder *paramsBuilder = NULL;
    guint32 flags = 0;
    void *mdmObj = NULL;
    MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
    MdmPathDescriptor parentPathDesc = EMPTY_PATH_DESCRIPTOR;
    BcmRet ret = BCMRET_SUCCESS;
    CmsRet cret;

    bcmuLog_debug("Entered");

    /* retrieve all input params */
    g_variant_get(parameters, "(ssu)", &fullpath, &limitSubtree, &flags);

    /* initialize return variant builder */
    paramsBuilder = g_variant_builder_new(G_VARIANT_TYPE ("a(ssssbb)"));

    bcmuLog_notice("fullpath=%s (limit=%s) flags=0x%x",
                   fullpath, limitSubtree, flags);

    // TODO: push all this direct MDM accesses down into BCM Generic HAL
    // On the first call to getNextObject, fullpath may be a generic path
    cret = cmsMdm_genericPathToOid(fullpath, &(pathDesc.oid));
    if (cret == CMSRET_SUCCESS)
    {
        // This is a generic path, leave pathDesc iidStack empty.  Do nothing here.
    }
    else
    {
        cret = cmsMdm_fullPathToPathDescriptor(fullpath, &pathDesc);
        if (cret != CMSRET_SUCCESS)
        {
            ret = BCMRET_INVALID_ARGUMENTS;
            nextFullpath = g_strdup_printf(" ");
            response = g_strdup_printf("Invalid fullpath %s", fullpath);
            bcmuLog_error("Invalid fullpath %s, ret=%d", fullpath, cret);
            goto exit;
        }
    }

    // ParentIidStack limits the subtree where the getNext is allowed to walk.
    cmsMdm_fullPathToPathDescriptor(limitSubtree, &parentPathDesc);

    bcmuLog_debug("oid=%d", pathDesc.oid);
    bcmuLog_debug("parentIidStack:%s", cmsMdm_dumpIidStack(&parentPathDesc.iidStack));
    bcmuLog_debug("iidStack: %s", cmsMdm_dumpIidStack(&(pathDesc.iidStack)));

    cret = cmsObj_getNextInSubTreeFlags(pathDesc.oid,
                                        &(parentPathDesc.iidStack),
                                        &(pathDesc.iidStack),
                                        flags, &mdmObj);
    if (cret == CMSRET_SUCCESS)
    {
        char *nFullpath = NULL;
        char *fullpathArray[1];
        BcmGenericParamInfo *paramInfoArray = NULL;
        UINT32 numParamInfos = 0;
        UINT32 i;

        cmsMdm_pathDescriptorToFullPath(&pathDesc, &nFullpath);
        fullpathArray[0] = nFullpath;
        bcmuLog_debug("got next fullpath %s", nFullpath);
        nextFullpath = g_strdup(nFullpath);

        /*
         * Call generic Z-Bus function to fill in values for this object.
         */
        ret = zbus_in_getParameterValues((const char **) fullpathArray, 1,
                                         TRUE, flags,
                                         &paramInfoArray, &numParamInfos);

        CMSMEM_FREE_BUF_AND_NULL_PTR(nFullpath);

        if (ret == BCMRET_SUCCESS)
        {
            for (i = 0; i < numParamInfos; i++)
            {
                fillParamInfoIntoBuilder(&(paramInfoArray[i]), paramsBuilder);
            }
            response = g_strdup_printf("getNextObject returned %d params",
                                        numParamInfos);
            cmsUtl_freeParamInfoArray(&paramInfoArray, numParamInfos);
        }
        else
        {
            bcmuLog_error("Error from zbus_in_getParameterValues, ret=%d", ret);
            response = g_strdup_printf("Error from zbus_in_getParameterValues, ret=%d", ret);
        }

        // Delay the freeing of the mdmObj to this point.  This forces MDM
        // auto-lock to hold the lock for us while we are calling
        // zbus_in_getParameterValues on this object.
        cmsObj_free(&mdmObj);
    }
    else
    {
        // Don't complain loudly about error because it could be NO_MORE_INSTANCES(?)
        bcmuLog_notice("Error from cmsObj_getNextInSubTreeFlags, ret=%d", cret);
        ret = (BcmRet) cret;
        response = g_strdup_printf("Error from cmsObj_getNextInSubTreeFlags, ret=%d", cret);
        nextFullpath = g_strdup_printf(" ");
    }

exit:
    *result = g_variant_new("(sa(ssssbb)su)",
                            nextFullpath, paramsBuilder, response, ret);
    g_free(fullpath);
    g_free(limitSubtree);
    g_free(nextFullpath);
    g_free(response);
    g_variant_builder_unref(paramsBuilder);
    return;
}

