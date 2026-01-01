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
 ************************************************************************/var WEBSocket=function(){function o(o,u,a,f){r=f,e=u,t=o;var l="";if("WebSocket"in window)try{n=new WebSocket(e,t)}catch(c){r!=null&&r(1,"Failed to connect to the server")}else"MozWebSocket"in window?n=new MozWebSocket(e,t):l="WebSocket is not supported in your browser.";if(n!=null){n.binaryType="arraybuffer";try{n.onopen=function(){s=1,n.send(a)},n.onmessage=function(e){var t=new Uint8Array(e.data);s==1?(s==2,r!=null&&r(t)):s==2&&i!=null&&i(1,null,t)},n.onclose=function(){s>0&&(s=0,r!=null&&r(1,"Connection Closed"))},n.onerror=function(){s=0,r!=null&&r(1,"Please check IP address, network connection or if the server is started")}}catch(h){r!=null&&r(2,"Error <"+h+"> happens; Please try later")}}}function u(e,t){s=2,i=t,n!=null&&n.send(e)}var e,t,n,r=null,i=null,s=0;return{connect:function(e,t,r,i,s){if(s)return typeof n!="undefined"&&n.close(),o(e,t,r,i);typeof n!="undefined"&&n.close()},send:function(e,t){if(s==0){t(0,"Please connect to the bbc server first",null);return}return u(e,t)}}}();