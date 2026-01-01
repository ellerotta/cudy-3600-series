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
 ************************************************************************/var SPT_PROTOCOL_PLDTYPE_TXT=0,SPT_PROTOCOL_PLDTYPE_BIN=1,Protocol=function(){function w(e,t){var n="";for(var r=t;r<e.length;++r)n+=String.fromCharCode(e[r]);return n}function E(a){var f=0;if(a.length<e)return;h[t]=a[0],h[n]=a[1],h[r]=a[2],h[i]=a[3],h[s]=a[4],h[o]=a[5],f=a[6]<<24,f+=a[7]<<16,f+=a[8]<<8,f+=a[9],h[u]=f,0<h[u]?0==h[o]?p=w(a,e):(d=new Uint8Array(h[u]),d=a.slice(e)):(p=null,d=null)}function S(t,n,r,i,s){var o,u=0,a;r>0&&(i==SPT_PROTOCOL_PLDTYPE_TXT?v=s.getBytes():v=s,u=v.length),o=new Uint8Array(e+u),o[0]=f,o[1]=0,o[2]=t,o[3]=n,o[4]=0,o[5]=i,o[6]=u>>24,o[7]=u>>16,o[8]=u>>8,o[9]=u&255,o[10]=0,o[11]=0,o[12]=0,o[13]=0,o[14]=0,o[15]=0;for(var l=0;l<u;++l)o[l+e]=v[l];return o}function x(e,t){var n=1;e instanceof Uint8Array&&(n=0,t="connected"),g!=null&&g(n,t)}function T(e,t){var n=S(0,0,0,1,null);g=t,spt_server=e,WEBSocket.connect(l,e,n,x)}function N(e,t,i){var u=0,f,l="";E(i),f=h[r];if(f!=m)return;if(e!=1&&y!=null)return y(1,t,null);u=h[n];if(a==u){var c;SPT_PROTOCOL_PLDTYPE_TXT==h[o]?c=p:c=d}else l=p;var v=h[s];if(v==1&&y!=null)y(u,l,c);else if(v==2&&b!=null){var f=h[r];b(f,u,c)}}var e=16,t="ver",n="sts",r="tgt",i="cmd",s="mtp",o="ptp",u="len",a=0,f=1,l="bbcd",c=7682,h={},p=null,d=null,v,m=0,g=null,y=null,b=null;return{getHeaderMember:function(e){return h[e]},parsePacket:function(e){E(e)},getPayload:function(){return 0==h[o]?p:d},composePacket:function(e,t,n,r,i){return S(e,t,n,r,i)},connectServer:function(e,t,n){var r=S(0,0,0,1,null);return g=t,WEBSocket.connect(l,e,r,x,n)},getPort:function(){return c},sendPacket:function(e,t,n,r,i,s){var o;m=e,y=s,o=S(e,t,n,r,i),parent.WEBSocket.send(o,N)},registerTasklet:function(e,t,n,r,i,s,o){var u;m=e,y=s,b=o,u=S(e,t,n,r,i),parent.WEBSocket.send(u,N)}}}();