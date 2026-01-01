/******************************************************************************
 *
<:copyright-BRCM:2015:proprietary:standard

   Copyright (c) 2015 Broadcom 
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
 *
******************************************************************************/
#ifndef _VLANCTL_API_TRACE_H_
#define _VLANCTL_API_TRACE_H_

void vlanCtl_initTrace(void);
void vlanCtl_cleanupTrace(void);
void vlanCtl_createVlanInterfaceTrace(const char *realDevName, unsigned int vlanDevId,
                                int isRouted, int isMulticast);
void vlanCtl_createVlanInterfaceExtTrace(const char *realDevName, unsigned int vlanDevId,
                                vlanCtl_createParams_t *createParamsP);
void vlanCtl_createVlanInterfaceByNameTrace(char *realDevName, char *vlanDevName,
                                      int isRouted, int isMulticast);
void vlanCtl_createVlanInterfaceByNameExtTrace(char *realDevName, char *vlanDevName,
                                      vlanCtl_createParams_t *createParamsP);
void vlanCtl_deleteVlanInterfaceTrace(char *vlanDevName);
void vlanCtl_initTagRuleTrace(void);
void vlanCtl_insertTagRuleTrace(const char *realDevName, vlanCtl_direction_t tableDir, unsigned int nbrOfTags,
                          vlanCtl_ruleInsertPosition_t position, unsigned int posTagRuleId);
void vlanCtl_removeTagRuleTrace(char *realDevName, vlanCtl_direction_t tableDir,
                          unsigned int nbrOfTags, unsigned int tagRuleId);
void vlanCtl_removeAllTagRuleTrace(char *vlanDevName);
void vlanCtl_removeTagRuleByFilterTrace(char *realDevName, vlanCtl_direction_t tableDir,
                                  unsigned int nbrOfTags);
void vlanCtl_dumpRuleTableTrace(char *realDevName, vlanCtl_direction_t tableDir, unsigned int nbrOfTags);
void vlanCtl_dumpAllRulesTrace(void);
void vlanCtl_getNbrOfRulesInTableTrace(char *realDevName, vlanCtl_direction_t tableDir, unsigned int nbrOfTags);
void vlanCtl_setDefaultVlanTagTrace(char *realDevName, vlanCtl_direction_t tableDir, unsigned int nbrOfTags,
                              unsigned int defaultTpid, unsigned int defaultPbits, unsigned int defaultCfi,
                              unsigned int defaultVid);
void vlanCtl_setDscpToPbitsTrace(char *realDevName, unsigned int dscp, unsigned int pbits);
void vlanCtl_dumpDscpToPbitsTrace(char *realDevName, unsigned int dscp);
void vlanCtl_setTpidTableTrace(char *realDevName, unsigned int *tpidTable);
void vlanCtl_dumpTpidTableTrace(char *realDevName);
void vlanCtl_dumpLocalStatsTrace(char *realDevName);
void vlanCtl_setIfSuffixTrace(char *ifSuffix);
void vlanCtl_setDefaultActionTrace(const char *realDevName, vlanCtl_direction_t tableDir, unsigned int nbrOfTags,
                             vlanCtl_defaultAction_t defaultAction, char *defaultRxVlanDevName);
void vlanCtl_setRealDevModeTrace(char *realDevName, bcmVlan_realDevMode_t mode);
void vlanCtl_createVlanFlowsTrace(char *rxVlanDevName, char *txVlanDevName);
void vlanCtl_deleteVlanFlowsTrace(char *rxVlanDevName, char *txVlanDevName);
void vlanCtl_filterOnSkbPriorityTrace(unsigned int priority);
void vlanCtl_filterOnSkbMarkFlowIdTrace(unsigned int flowId);
void vlanCtl_filterOnSkbMarkPortTrace(unsigned int port);
void vlanCtl_filterOnEthertypeTrace(unsigned int etherType);
void vlanCtl_filterOnIpProtoTrace(unsigned int ipProto);
void vlanCtl_filterOnDscpTrace(unsigned int dscp);
void vlanCtl_filterOnDscp2PbitsTrace(unsigned int dscp2pbits);
void vlanCtl_filterOnVlanDeviceMacAddrTrace(unsigned int acceptMulticast);
void vlanCtl_filterOnFlagsTrace(unsigned int flags);
void vlanCtl_filterOnTagPbitsTrace(unsigned int pbits, unsigned int tagIndex);
void vlanCtl_filterOnTagCfiTrace(unsigned int cfi, unsigned int tagIndex);
void vlanCtl_filterOnTagVidTrace(unsigned int vid, unsigned int tagIndex);
void vlanCtl_filterOnTagEtherTypeTrace(unsigned int etherType, unsigned int tagIndex);
void vlanCtl_filterOnRxRealDeviceTrace(char *realDevName);
void vlanCtl_filterOnTxVlanDeviceTrace(char *vlanDevName);
void vlanCtl_cmdPopVlanTagTrace(void);
void vlanCtl_cmdPushVlanTagTrace(void);
void vlanCtl_cmdSetEtherTypeTrace(unsigned int etherType);
void vlanCtl_cmdSetTagPbitsTrace(unsigned int pbits, unsigned int tagIndex);
void vlanCtl_cmdSetTagCfiTrace(unsigned int cfi, unsigned int tagIndex);
void vlanCtl_cmdSetTagVidTrace(unsigned int vid, unsigned int tagIndex);
void vlanCtl_cmdSetTagEtherTypeTrace(unsigned int etherType, unsigned int tagIndex);
void vlanCtl_cmdCopyTagPbitsTrace(unsigned int sourceTagIndex, unsigned int targetTagIndex);
void vlanCtl_cmdCopyTagCfiTrace(unsigned int sourceTagIndex, unsigned int targetTagIndex);
void vlanCtl_cmdCopyTagVidTrace(unsigned int sourceTagIndex, unsigned int targetTagIndex);
void vlanCtl_cmdCopyTagEtherTypeTrace(unsigned int sourceTagIndex, unsigned int targetTagIndex);
void vlanCtl_cmdDscpToPbitsTrace(unsigned int tagIndex);
void vlanCtl_cmdSetDscpTrace(unsigned int dscp);
void vlanCtl_cmdDropFrameTrace(void);
void vlanCtl_cmdSetSkbPriorityTrace(unsigned int priority);
void vlanCtl_cmdSetSkbMarkPortTrace(unsigned int port);
void vlanCtl_cmdSetSkbMarkQueueTrace(unsigned int queue);
void vlanCtl_cmdSetSkbMarkQueueByPbitsTrace(void);
void vlanCtl_cmdSetSkbMarkFlowIdTrace(unsigned int flowId);
void vlanCtl_cmdOvrdLearningVidTrace(unsigned int vid);
void vlanCtl_cmdContinueTrace(void);
void vlanCtl_setReceiveVlanDeviceTrace(char *vlanDevName);
void vlanCtl_setVlanRuleTableTypeTrace(unsigned int type);
void vlanCtl_setDropPrecedenceTrace(bcmVlan_flowDir_t dir, bcmVlan_dpCode_t dpCode);
void vlanCtl_setIptvOnlyTrace(void);


#endif /* _VLANCTL_API_TRACE_H_ */
