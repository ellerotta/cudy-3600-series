/***********************************************************************
 *
 *  Copyright (c) 2021  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2021:proprietary:standard

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

#ifndef BCM_WEBPROTO_EVENT_H
#define BCM_WEBPROTO_EVENT_H

#ifdef USE_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#include "cms_log.h"
#include "cms_tmr.h"
#include "cms_strconv.h"  // for UrlProto

// Global vars used by calling apps, but defined in protocol.c
extern UBOOL8 loggingSOAP;


// This code passes a lot of structs into functions as void *.
// Magic IDs to confirm the expected type of struct is passed in.
#define WEBPROTO_MAGICID_WGINT            0xFEEB0119
#define WEBPROTO_MAGICID_WGEXT            0xFEEB0229
#define WEBPROTO_MAGICID_VENDOR_DLSTATE   0xFEEB0339
#define WEBPROTO_MAGICID_VENDOR_ACTIVATE_STATE   0xFEEB0449
#define WEBPROTO_MAGICID_VENDOR_UPLOAD_STATE     0xFEEB0559


/* general system status and configruation retrieval */
/* Returns state of WAN interface to be used by tr69 client */
/* Originally from bcmWrapper.h */
typedef enum
{
   eWAN_INACTIVE,
   eWAN_ACTIVE
} eWanState;


/* status enum for acs connection and msg xfer */
/* originally from tr69cdefs.h */
typedef enum {
   eOK,
   eConnectError,
   eGetError,
   ePostError,
   eAuthError,
   eDownloadDone,
   eUploadDone,
   eAcsDone
}AcsStatus;

/* originally from tr69cdefs.h */
typedef enum {
   eACSNeverContacted=0,
   eACSContacted,
   eACSInformed,
   eACSUpload,
   eACSDownloadReboot,
   eACSSetValueReboot,
   eACSAddObjectReboot,
   eACSDelObjectReboot,
   eACSRPCReboot
} eInformState;

/*
 * Inform Events and their associated sub-code methods.
 * Notes:
 * (1) These values are stored in informEvList and in flash
 *     (GWStateData.informEvList) as single byte char, so the value must not
 *     exceed 255.
 * (2) The text corresponding to these values are defined in the
 *     informEventStrTable in bcmWrapper.c, so any additions to these values
 *     must also be reflected in that table.
 * (3) Values of 0-12 were used up to 4.12 release series.  Since these values
 *     can be written to flash, their values must not change in newer releases;
 *     otherwise, problems will occur when upgrading from 4.12 to 4.14.  Going
 *     forward, whatever values are defined must remain that value for the
 *     same reason.
 */
#define INFORM_EVENT_BOOTSTRAP                0
#define INFORM_EVENT_BOOT                     1
#define INFORM_EVENT_PERIODIC                 2
#define INFORM_EVENT_SCHEDULED                3
#define INFORM_EVENT_VALUE_CHANGE             4
#define INFORM_EVENT_KICKED                   5
#define INFORM_EVENT_CONNECTION_REQUEST       6
#define INFORM_EVENT_TRANSER_COMPLETE         7
#define INFORM_EVENT_DIAGNOSTICS_COMPLETE     8
#define INFORM_EVENT_REBOOT_METHOD            9
#define INFORM_EVENT_SCHEDULE_METHOD          10
#define INFORM_EVENT_DOWNLOAD_METHOD          11
#define INFORM_EVENT_UPLOAD_METHOD            12

#define INFORM_EVENT_REQUEST_DOWNLOAD         109
#define INFORM_EVENT_AUTON_TRANSFER_COMPLETE  110
#define INFORM_EVENT_DU_CHANGE_COMPLETE       111
#define INFORM_EVENT_AUTON_DU_CHANGE_COMPLETE 112

#define INFORM_EVENT_SCHEDULE_DOWNLOAD_METHOD 150
#define INFORM_EVENT_CHANGE_DU_CHANGE_METHOD  151


/**********************************************************************
 * From event.h
 *********************************************************************/

/** tr69c and obuspa is spawned with normal fd 0, 1, and 2 intact,
 * so the first reasonable fd to send to the listener is 3.
 * Max fd is governed by size of fd_set.  I think it is actually 128,
 * but I doubt we will ever need to go up that high, so set the
 * limit to 63.
 */
#define MIN_LISTENER_FD  3
#define MAX_LISTENER_FD  63

/** The amount of time we will sleep in eventLoop when there is nothing to do.
 *  This can be a very high number.
 */
#define EVENTLOOP_MAX_SLEEP_SECS  600


/*----------------------------------------------------------------------
 * type of listener, see select() for more information
 */
typedef enum {
  iListener_Read,
  iListener_Write,
  iListener_Except,
  iListener_ReadWrite
} tListenerType;

typedef struct Listener
{
   struct Listener   *next;
   int               fd;
   CmsEventHandler   func;
   void              *handle;
   tListenerType     type;
} Listener;

extern void stopListener(int fd);
extern void closeAllListeners(void);
extern void freeAllListeners(void);
extern void setListener (int fd, CmsEventHandler func, void* handle);
extern void setListenerType(int fd, CmsEventHandler func, void* handle, tListenerType type);


// Calling app must allocate this object (or declare it in global) and pass it
// into webproto_setCallbacks.  If app is single threaded or all threads can
// use same callbacks, then register with webproto_setCallbacks.  If app has
// multiple threads (e.g. obuspa with upload and download threads) which want
// to use different state and callback funcs, register and unregister with
// webproto_setCallbacksForThread() and webproto_clearCallbacksForThread().
typedef struct
{
   void     *tmrHandle;  // CMS Timer Handle, caller must initialize and pass in
   Listener *listeners;  // low level fd event handler.  Just a pointer, no need to init.
   const char* (*getAcsUrl)(void);
   const char* (*getRedirectUrl)(void);
   CmsRet     (*getFault)(void);
   void       (*setFault)(CmsRet fault);
   eWanState  (*getWanState)(void);
   UBOOL8     (*isTimeSynchronized)(void);
   int        (*establishConnection)(struct sockaddr_storage host_addr, int *sock_fd);
} WebprotoCB;

extern void webproto_setCallbacks(WebprotoCB *cb);
extern void webproto_setCallbacksForThread(WebprotoCB *cb);
extern void webproto_clearCallbacksForThread(void);

// bcm_webproto code should use this to get the appCB.  May return NULL if
// not set or not found for this thread.
extern WebprotoCB *webproto_getAppCB(void);


/**********************************************************************
 * From appdefs.h
 *********************************************************************/

#define USER_AGENT_NAME    "BCM_TR69_CPE_04_00"

/* Timer values */

#define ACSINFORMDELAY     500      /* initial delay (ms) after pgm start before */
                                    /* sending inform to ACS */
#define ACSRESPONSETIME    (180*1000) /* MAX Time to wait on ACS response */
#define CHECKWANINTERVAL   (60*1000) /* check wan up */

/* ACS Connection Initiation */
#define ACSREALM        "IgdAuthentication"
#define ACSDOMAIN       "/"

#define SHELL           "/bin/sh"

/**********************************************************************
 * From utils.h
 *********************************************************************/

int dns_lookup(const char *name, struct sockaddr_storage *res);
uint32_t getRandomNumber(uint32_t from, uint32_t to);

extern const u_char zeroMac[6];

/* Files */
int  hasChanged(const char* new, const char* old);
int  mkdirs(const char *path);
void rmrf(const char* path);

/* Time */
time_t getCurrentTime(void);
char *getXSIdateTime(time_t *tp);

/* hex */
extern const char *util_StringToHex(const char *s);

/* Addresses */
void readMac(u_char* mac, const char* val);
int  readIp(const char* ip);
int  readProto(const char* val);
int readMask(const char *mask);

char* writeMac(const u_char* mac);
char* writeCanonicalMac(const u_char* mac);
char* writeQMac(const u_char* mac);
void  writeIp_b(int ip, char *buf);
char* writeIp(struct sockaddr * ip);
char* writeNet(int ip, int bits);
char* writeBcast(int ip, int bits);
char* writeMask(int bits);
char* writeRevNet(int ip, int bits);
char* writeRevHost(int ip, int bits);
char* writeProto(int proto);

/* Text handling and formatting */
void  readHash(u_char* hash, const char* val);
char* writeQHash(const u_char* mac);
char* unquoteText(const char* text);
char* quoteText(const char* text);
int streq(const char *s0, const char *s1);
int stricmp( const char *s1, const char *s2 );
const char *itoa(uint32_t i);
int testBoolean(const char *s);


typedef enum
{
   eNone,
   eDigest,
   eBasic
} eAuthentication;

typedef enum
{
   eNoQop,
   eAuth,
   eAuthInt
} eQop;

/* Used for both server/client */
typedef struct SessionAuth
{
   eQop           qopType;
   int            nonceCnt;
   char           *nc;      /* str of nonceCnt */
   char           *nonce;
   char           *orignonce;
   char           *realm;
   char           *domain;
   char           *method;
   char           *cnonce;
   char           *opaque;
   char           *qop;
   char           *user;
   char           *uri;
   char           *algorithm;
   char           *response;
   char           *basic;
   unsigned char  requestDigest[33];
} SessionAuth;

char *generateWWWAuthenticateHdr(SessionAuth *sa, char *realm, char *domain, char* method);

int parseAuthorizationHdr(char *ahdr, SessionAuth *sa, char *username, char *password);

char *generateAuthorizationHdrValue( SessionAuth *sa, char *wwwAuth, char *method,
                             char *uri, char *user, char *pwd);
char *generateNextAuthorizationHdrValue(SessionAuth *, char *user, char *pwd );
char *generateBasicAuthorizationHdrValue(SessionAuth *, char *user, char *pwd );
eAuthentication parseWWWAuthenticate(char *ahdr, SessionAuth *sa);



/**********************************************************************
 * From protocol.h
 *********************************************************************/
typedef enum {
  iZone_Unknown,
  iZone_Lan,
  iZone_Ihz
} tZone;


typedef struct CookieHdr {
	struct CookieHdr *next;
	char	*name;
	char	*value;
} CookieHdr;

/*--------------------*/
typedef struct {
  /* common */
  char *content_type;
  char *protocol;
  char *wwwAuthenticate;
  char *Authorization;
  char *TransferEncoding;
  char *Connection;
  /* request */
  char *method;
  char *path;
  char *host;
  int  port;
  int  content_length;

  /* result */
  int  status_code;
  CookieHdr	*setCookies;
  char *message;
  char *locationHdr;		/* from 3xx status response */

  /* request derived */
//  tIpAddr addr;  /* IP-address of communicating entity */
  tZone zone;    /* zone in which communicating entity is */
  char *filename;
  char *arg;
} tHttpHdrs;

typedef void (*tProtoHandler)(void *, int lth);

typedef enum {
    sslRead,
    sslWrite
} tSSLIO;
/*--------------------*/
typedef enum {
  iUnknown,
  iNormal,
#ifdef USE_SSL
  iSsl,
#endif
  i__Last
} tPostCtxType;

/*--------------------*/
typedef struct {
   tPostCtxType   type;
   int            fd;      /* filedescriptor */

   /* internal use */
#ifdef USE_SSL
   SSL            *ssl;
   int            sslConn;
#endif
   tProtoHandler  cb;
   void           *data;
} tProtoCtx;


/* convenient naming */
#define fdgets   proto_Readline
#define fdprintf proto_Printline

#define PROTO_OK                0
#define PROTO_ERROR            -1
#define PROTO_ERROR_SSL        -2

/** Maximum chunked buffer size.
 *
 * 1MB should be enough to hold the largest possible chunked message
 * that we can receive.  Note this constant is not used for firmware
 * download.
 */
#define MAXWEBBUFSZ     (1024 * 1024)

/*----------------------------------------------------------------------*/
extern void proto_Init(void);

extern tHttpHdrs *proto_NewHttpHdrs(void);
extern void proto_FreeHttpHdrs(tHttpHdrs *p);

extern tProtoCtx *proto_NewCtx(int fd);
#ifdef USE_SSL
extern void proto_SetSslCtx(tProtoCtx *pc, tProtoHandler cb, void *data);
#endif
extern void proto_FreeCtx(tProtoCtx *pc);

extern int  proto_ReadWait(tProtoCtx *pc, char *ptr, int nbytes);
extern int  proto_Readn(tProtoCtx *pc, char *ptr, int nbytes);
extern int  proto_Writen(tProtoCtx *pc, const char *ptr, int nbytes);
extern int  proto_Readline(tProtoCtx *pc, char *ptr, int maxlen);
extern void proto_Printline(tProtoCtx *pc, const char *fmt, ...);
extern int	proto_Skip(tProtoCtx *pc);
extern int  proto_SSL_IO(tSSLIO iofunc, tProtoCtx *pc, char *ptr, int nbytes, tProtoHandler cb, void *data);
extern void proto_SendRequest(tProtoCtx *pc, const char *method, const char *url);
void proto_SendCookie(tProtoCtx *pc, CookieHdr *c);
extern void proto_SendHeader(tProtoCtx *pc,  const char *header, const char *value);
extern void proto_SendEndHeaders(tProtoCtx *pc);
extern void proto_SendRaw(tProtoCtx *pc, const char *arg, int len);
extern void proto_SendHeaders(tProtoCtx *pc, int status, const char* title, const char* extra_header, const char* content_type);

extern void proto_SendRedirect(tProtoCtx *pc, const char *host, const char* location);
extern void proto_SendRedirectProtoHost(tProtoCtx *pc, const char *protohost, const char* location);
extern void proto_SendRedirectViaRefresh(tProtoCtx *pc, const char *host, const char* location);
extern void proto_SendError(tProtoCtx *pc, int status, const char* title, const char* extra_header, const char* text);

extern int  proto_ParseResponse(tProtoCtx *pc, tHttpHdrs *hdrs);
extern int  proto_ParseRequest(tProtoCtx *pc, tHttpHdrs *hdrs);
extern void proto_ParseHdrs(tProtoCtx *pc, tHttpHdrs *hdrs);
extern void proto_ParsePost(tProtoCtx *pc, tHttpHdrs *hdrs);


/**********************************************************************
 * From wget.h
 *********************************************************************/
typedef enum
{
   iWgetStatus_Ok = 0,
   iWgetStatus_InternalError,
   iWgetStatus_ConnectionError,
   iWgetStatus_Error,
   iWgetStatus_HttpError
} tWgetStatus;

typedef struct
{
   int         magicId;
   tWgetStatus status;
   tProtoCtx   *pc;
   tHttpHdrs   *hdrs;
   const char  *msg;  /* Msg associated with status */
   void        *handle;
} tWget;

typedef enum
{
   eCloseConnection=0,
   eKeepConnectionOpen  /* used by wConnect and wClose */
} tConnState;

typedef enum
{
   eUndefined,
   eConnect,
   ePostData,
   eGetData,
   ePutData,
   eDisconnect
} tRequest;

typedef struct XtraPostHdr
{
   struct XtraPostHdr *next;
   char               *hdr;   /* header string */
   char               *value; /* value string*/
} XtraPostHdr;

typedef struct
{
   int               magicId;
   tConnState        keepConnection;
   int               status;
   tRequest          request;
   int               cbActive; /* set to 1 if callback from report status */
   tProtoCtx         *pc;
   CmsEventHandler   cb;
   void              *handle;
   UrlProto          urlProto;
   char              *host;
   struct sockaddr_storage  host_addr;
   unsigned short    port;
   char              *uri;
   tHttpHdrs         *hdrs;
   CookieHdr         *cookieHdrs;
   XtraPostHdr       *xtraPostHdrs;
   char              *content_type;
   char              *postdata;
   int               datalen;
} tWgetInternal;

/*----------------------------------------------------------------------*
 * returns
 *   0 if sending request succeded
 *  -1 on URL syntax error
 *
 * The argument to the callback is of type (tWget *)
 */
tWgetInternal *wget_Connect(const char *url, CmsEventHandler callback, void *handle);
int wget_GetData(tWgetInternal *wg, CmsEventHandler callback, void *handle);
int wget_PostData(tWgetInternal *,char *data, int datalen, char *contenttype,
                  CmsEventHandler callback, void *handle);
int wget_PostDataClose(tWgetInternal *,char *data, int datalen, char *contenttype,
                       CmsEventHandler callback, void *handle);
int wget_PutData(tWgetInternal *,char *data, int datalen, char *contenttype, CmsEventHandler callback, void *handle);
int wget_Disconnect(tWgetInternal *);
const char *wget_LastErrorMsg(void);
int wget_AddPostHdr(tWgetInternal *wio, char *xhdrname, char *value);
void wget_ClearPostHdrs(tWgetInternal *wio);

// from protocol.c
extern char *readLengthMsg(tWget *wg, int readLth, int *mlth, int doFlushStream);
extern char *readChunkedMsg(tWget *wg, int *mlth, int maxSize);
extern char *readChunkedImage(tWget *wg, int *mlth);
extern char *readResponse( tWget *wg, int *mlth, int maxBufferSize);


/**********************************************************************
 * From httpProto.h
 *********************************************************************/

typedef enum {
   sIdle,
   sAuthenticating,
   sAuthenticated,
   sAuthFailed,
   sShutdown,
   sAuthNotNeeded,
} AuthState;
 
typedef enum {
	eClosed,	/* connection is closed */
	eClose,		/* set Connection: close on next send */
	eStart,		/* connection is connecting */
	eConnected,	/* connection has completed */
} HttpState;

typedef void (*CallBack)(AcsStatus);

typedef struct HttpTask {
   /* current posted msg dope */
   HttpState      eHttpState;
   AuthState      eAuthState;
   tWgetInternal  *wio;
   char           *postMsg;
   int            postLth;
   int            content_len;
   AcsStatus      xfrStatus;
   CallBack       callback;
   char           *authHdr;		
} HttpTask;

#endif  /* BCM_WEBPROTO_EVENT_H */
