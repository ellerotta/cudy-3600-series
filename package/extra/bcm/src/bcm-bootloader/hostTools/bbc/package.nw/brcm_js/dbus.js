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
 ************************************************************************/var DBus=function(){function n(e,n){return"<span style='color:"+n+"'>"+t[e]+"</span>"}function r(t,n){return"<span style='color:"+n+"'>"+e[t]+"</span>"}function i(e,t){var n=[],r=0,i=t.split("&");if(e.length==i.length)for(var s=0;s<i.length;s++)n.push(i[s]);return n}var e={a:"Array",b:"Bool",d:"Double",g:"Signature",i:"Int32",n:"Int16",o:"Object Path",q:"UInt16",s:"String",t:"UInt64",u:"UInt32",v:"Variant",x:"Int64",y:"Byte",ai:"Int32 Array",as:"String Array",sv:"{String, Variant}","a{sv}":"Dict of {String, Variant}","a(sss)":"Array Struct"},t={readwrite:"rw",read:"ro",write:"w"};return{createService:function(e,t,n){var r=new RegExp("\\.","g"),i="node"+e.replace(r,"_"),s="<div class='panel panel-default'><div class='panel-heading'><h4 class='panel-title'><span class='parent_type'></span><a data-toggle='collapse' service='#"+e+"' href='#"+i+"' data-cmpl='false' onclick='"+n+'($(this),"'+e+'","'+t+'","'+"\")' style='font-size:110%;color:black'>"+e+"</a></h4></div><div id='"+i+"' class='panel-collapse collapse'><div class='panel-body' id='"+i+"_body'></div></div></div>";return s},createInterface:function(e,t,i,s,o){var u=i.getAttribute("name"),a=new RegExp("\\/","g"),f="node"+t.replace(a,"_").replace(/\./g,"-"),l=f+"_intf"+"-"+u.replace(a,"_").replace(/\./g,"-"),c="<div class='panel panel-default'><div class='panel-heading'><h4 class='panel-title'><span class='parent_type'> </span><a data-toggle='collapse' data-parent='#"+f+"' href='#"+l+"'>"+u+"</a></h4></div><div id='"+l+"' class='panel-collapse collapse'><div class='panel-body' id='"+l+"_body'>";for(var h=0;h<i.children.length;h++){var p=i.children[h];if(p.nodeName!="node")if(p.nodeName=="method"){var d=p.children,v="";for(var m=0;m<d.length;m++)m>0&&(v+="--"),v+=d[m].getAttribute("direction")+"-"+d[m].getAttribute("type")+"-"+d[m].getAttribute("name");c+="<div class='panel panel-default'><div class='panel-heading'><h4 class='panel-title'><span class='parent_type'>  </span><a href='#' onclick='"+s+'("'+e+'","'+t+'","'+u+'","'+p.getAttribute("name")+'", "'+v+"\")'><span class='node_child'>"+p.getAttribute("name");var g="(";for(var m=0;m<d.length;m++)d[m].getAttribute("direction")=="in"?(g.lastIndexOf(")")==g.length-1&&(g=g.substr(0,g.length-1)+","),g+=r([d[m].getAttribute("type")],"lawngreen")+" "+d[m].getAttribute("name")+")"):d[m].getAttribute("direction")=="out"&&(g.charAt(g.length-1)!=")"&&(g+=")"),g+=" <span class='glyphicon glyphicon-arrow-right' aria-hidden='true'></span> ("+r(d[m].getAttribute("type"),"lawngreen")+" "+d[m].getAttribute("name")+")");g.charAt(g.length-1)!=")"&&(g+=")"),g+="</span></a></h4></div></div>",g=g.replace(/\(/g,"<span style='color:darkorange'>(</span>"),g=g.replace(/\)/g,"<span style='color:darkorange'>)</span>"),c+=g}else if(p.nodeName=="signal"){c+="<div class='panel panel-default'><div class='panel-heading'><h4 class='panel-title'><span class='parent_type'>signal:</span><span class='node_child'>"+p.getAttribute("name");var g="(",d=p.children;for(var m=0;m<d.length;m++)g.lastIndexOf(")")==g.length-1&&(g=g.substr(0,g.length-2)+","),g+=r([d[m].getAttribute("type")],"lawngreen")+" "+d[m].getAttribute("name")+")";g.charAt(g.length-1)!=")"&&(g+=")"),g+="</span></h4></div></div>",g=g.replace(/\(/g,"<span style='color:darkorange'>(</span>"),g=g.replace(/\)/g,"<span style='color:darkorange'>)</span>"),c+=g}else if(p.nodeName=="property"){var y="propval_"+t+"_"+u+"_"+p.getAttribute("name");y=y.replace(a,"_").replace(/\./g,"-"),c+="<div class='panel panel-default'><div class='panel-heading'><h4 class='panel-title'><span class='parent_type'> </span><a href='#' onclick='"+o+'("'+e+'", "'+t+'", "'+u+'", "'+p.getAttribute("name")+'", "'+p.getAttribute("type")+"\")' value='"+p.getAttribute("name")+"'><span class='node_child'>"+r(p.getAttribute("type"),"lawngreen")+" "+p.getAttribute("name")+"  ["+n(p.getAttribute("access"),"darkorange")+"]<span id="+y+"> </span>"+"</span></a></h4></div></div>"}}return c+="</div></div></div>",c},createNode:function(e,t,n,r){var i="";t.charAt(t.length-1)=="/"?i=t+n:i=t+"/"+n;var s=new RegExp("\\/","g"),o="node"+t.replace(s,"_"),u="node"+i.replace(s,"_"),a="<div class='panel panel-default'><div class='panel-heading'><h4 class='panel-title'><span class='parent_type'></span><a data-toggle='collapse' data-parent='#"+o+"' href='#"+u+"' data-cmpl='false' onclick='"+r+'($(this),"'+e+"\")'>"+i+"</a></h4></div><div id='"+u+"' class='panel-collapse collapse'><div class='panel-body' id='"+u+"_body'></div></div></div>";return a},runMethodPrepare:function(t,n){var r;if(n.length>0){var i=n.split("--");r="<h4 id='"+t+"'>Fill the parameters and press 'Confirm' to execute</h4><form class='form-horizontal'>";for(var s=0;s<i.length;s++)parm=i[s].split("-"),parm[0]=="in"?r+="<div class='form-group'><label for='"+i[s]+"' class='col-md-4 cold-lg-4 col-sm-4 control-label'>"+parm[2]+"&nbsp;<span style='color:magenta'>["+parm[0]+"]</span>"+"</label><div class='col-md-8 col-lg-8 col-sm-8'><input type='text' class='form-control' id='"+i[s]+"' placeholder='"+e[parm[1]]+"'></div></div>":parm[0]=="out"&&(r+="<div class='form-group'><label for='"+i[s]+"' class='col-md-4 cold-lg-4 col-sm-4 control-label'>"+parm[2]+"&nbsp;<span style='color:magenta'>["+parm[0]+"]</span>"+"</label><div class='col-md-8 col-lg-8 col-sm-8'><input type='text' class='form-control' id='"+i[s]+"' placeholder='"+e[parm[1]]+"'></div></div>");r+="</form>"}return r},composeMethodCallProps:function(e,t,n,r,i){var s=1,o='{"srv":"'+e+'","path":"'+t+'","iface":"'+n+'","method":"'+r+'","cmd":"mc","props":"',u="",a="",f="",l=0;for(var c=0;c<i.length;c++){var h=i[c].id.split("-");i[c].nodeName=="INPUT"&&h[0]=="in"?i[c].value==""?(i[c].style.borderColor="orange",s=0):u+=h[0]+"-"+h[1]+"-"+h[2]+"-"+i[c].value+"%":i[c].nodeName=="INPUT"&&h[0]=="out"&&(a+=h[0]+"-"+h[1]+"-"+h[2],c+1<i.length&&(a+="%"))}return a===""&&u!==""&&(u=u.slice(0,u.length-1)),o+=u+a+'"}',[s,o,a]},parseMethodCallOutputProps:function(e,t,n){var r,s=[],o=0,u=0;for(var a=0;a<n.length;a++){var f=n[a].id.split("-");n[a].nodeName=="INPUT"&&f[0]=="out"&&(u+=1)}if(e!=undefined){e.charAt(0)=="("&&(e=e.substring(1)),e.charAt(e.length-1)==")"&&(e=e.slice(0,-1)),s=i(t,e);if(s.length!=u)return!1;for(var a=0;a<n.length;a++){var f=n[a].id.split("-");n[a].nodeName=="INPUT"&&f[0]=="in"?n[a].style.borderColor="#dddddd":n[a].nodeName=="INPUT"&&f[0]=="out"&&(r=n[a],r.value=s[o],o+=1)}}return!0},instrospect:function(e,t){return'{"srv":"'+e+'","path":"'+t+'","cmd":"introspect"}'},compGetPropInfo:function(e,t,n){return"in-s-interface_name-"+e+"%in-s-property_name-"+t+"%out-"+n},getProperty:function(e,t,n,r){return'{"srv":"'+e+'","iface":"'+n+'","path":"'+t+'","cmd":"get","props":"'+r+'"}'}}}();