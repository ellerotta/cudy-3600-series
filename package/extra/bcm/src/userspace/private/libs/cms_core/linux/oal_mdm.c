/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
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


#include <dlfcn.h>
#include <sys/shm.h>  /* for shmat */
#include <sys/stat.h> /* for stat */
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "../oal.h"
#include "../odl.h"
#include "../mdm_private.h"
#include "cms_util.h"
#include "sysutil.h"
#include "sysutil_proc.h"


// from locks.c (this is not for general use, just oal_mdm.c)
extern void lck_clearDeadThreadByTid(SINT32 tid);

/** This structure is used to track mapping of valid strings arrays from
 * the libmdm.so library to the shared memory region.   It is only used
 * during the copy over from libmdm.so to shared memory region.
 */
struct vsa_entry
{
   const char *libAddr;
   char *shmAddr;
   struct vsa_entry *next;
};


/** This structure is used to track strings that were copied from libmdm.so
 * to the shared memory region.  It is only used
 * during the copy over from libmdm.so to shared memory region.
 */
struct str_entry
{
   char *shmStr;
   struct str_entry *next;
};


/** round a char * pointer to the next word boundary */
#ifdef __LP64__
#define ROUNDUP_WORD(s) ((char *) ((((UINT64) (s)) + 0x7) & ((UINT64) 0xfffffffffffffffc)))
#else
#define ROUNDUP_WORD(s) ((char *) ((((UINT32) (s)) + 0x3) & ((UINT32) 0xfffffffc)))
#endif

/* from mdm.h */
extern MdmSharedMemContext *mdmShmCtx;
void freeObjectMemory(MdmObjectNode *objNode);


static UINT32 copyToSharedMem(const MdmObjectNode *rootObj,
                              const MdmObjectNode *remoteRootObj);
static UINT32 getMdmSize(const MdmObjectNode *objNode);
static UINT32 getMdmObjectCount(const MdmObjectNode *objNode);
static char *copyNode(const MdmObjectNode *objNode,
                      char **nodeCopyAddr,
                      char **strCopyAddr,
                      struct vsa_entry **vsaMap,
                      struct str_entry **strMap,
                      UBOOL8 isRemoteObjPass,
                      const char *shmEnd);

static char *getSharedVsa(struct vsa_entry **vsaMap,
                          struct str_entry **strMap,
                          const char *libAddr,
                          char **strCopyAddr,
                          const char *shmEnd);
static char *copyVsa(struct str_entry **strMap,
                     const char **libArray,
                     char **strCopyAddr,
                     const char *shmEnd);
static void freeVsaMap(struct vsa_entry **vsaMap);

static char *getSharedStr(struct str_entry **strMap,
                          const char *libStr,
                          char **strCopyAddr,
                          const char *shmEnd);
static void freeStrMap(struct str_entry **strMap);

static void detachShm(void *shmAddr);

// in mdm_dataModelHelper.c
SINT32 mdm_initDataModelSelection(void);


static CmsRet getPathToLib(const char *libName, char *buf, UINT32 bufLen)
{
   /* On real system and BEEP, just filename is sufficient. */
   snprintf(buf, bufLen, "%s", libName);
#if defined(DESKTOP_LINUX)
   {
       char tmpBuf[CMS_MAX_FULLPATH_LENGTH]={0};
       CmsRet ret;
       ret = cmsUtl_getRunTimeRootDir(tmpBuf, sizeof(tmpBuf));
       if (ret != CMSRET_SUCCESS)
       {
           cmsLog_error("Could not get RunTime root dir!");
           return ret;
       }
       snprintf(buf, bufLen, "%s/lib/%s",tmpBuf, libName);
   }
#endif /* DESKTOP_LINUX */
   return CMSRET_SUCCESS;
}

static MdmObjectNode *getRootObjNode(const char *libName, void **libHandle)
{
   char mdmPathBuf[CMS_MAX_FULLPATH_LENGTH*2]={0};
   void *handle;
   MdmObjectNode *rootObjNode = NULL;
   CmsRet ret;

   ret = getPathToLib(libName, mdmPathBuf, sizeof(mdmPathBuf));
   if (ret != CMSRET_SUCCESS)
   {
      return NULL;
   }

   // On real device, must have RTLD_LAZY or the dlopen will fail.
   cmsLog_debug("dlopen %s", mdmPathBuf);
   handle = dlopen(mdmPathBuf, RTLD_LAZY);
   if (handle == NULL)
   {
      cmsLog_error("could not open %s, errno=%d", mdmPathBuf, errno);
      return NULL;
   }

   // For historical reasons, we use igdRootObjNode even for TR181
   rootObjNode = (MdmObjectNode *) dlsym(handle, "igdRootObjNode");
   if (rootObjNode == NULL)
   {
      cmsLog_error("could not find symbol igdRootObjNode");
      dlclose(handle);
      return NULL;
   }

   *libHandle = handle;
   return rootObjNode;
}


CmsRet oalShm_init(const MdmInitConfig *config, UINT32 shmSize, SINT32 *shmId, void **shmAddr)
{
   void *addr=NULL;
   void *libmdmHandle=NULL;
   void *remoteLibmdmHandle=NULL;
   const char *mdmFilename=NULL;
   SINT32 id;
   UINT32 shmflg, consumed;
   MdmObjectNode *rootObjNode=NULL;
   MdmObjectNode *remoteRootObjNode=NULL;
   CmsRet ret=CMSRET_SUCCESS;

   if ((*shmId) != UNINITIALIZED_SHM_ID)
   {
      cmsLog_notice("attach to existing shmId=%d at %p",
                    *shmId, config->shmAttachAddr);
      addr = shmat(*shmId, config->shmAttachAddr, 0);
      if (addr == (void *) -1)
      {
         cmsLog_error("Could not attach to shmId=%d at %p, error=%s",
                      *shmId, config->shmAttachAddr, strerror(errno));
         *shmAddr = NULL;
         return CMSRET_INTERNAL_ERROR;
      }
      else if (addr != config->shmAttachAddr)
      {
         cmsLog_error("actual (%p) and requested addr (%p) differ!",
                      addr, config->shmAttachAddr);
         *shmAddr = NULL;
         return CMSRET_INTERNAL_ERROR;
      }
      else
      {
         mdmShmCtx = (MdmSharedMemContext *) addr;

         /*
          * Every process needs to call oalLck_init, but oalLck_cleanup
          * only needs to be called once.
          */
         if ((ret = oalLck_init(TRUE, mdmShmCtx->lockKeyOffset)) != CMSRET_SUCCESS)
         {
            cmsLog_error("oalLck_init failed, ret=%d", ret);
            return ret;
         }

         /*
          * When a process first starts and attaches to the existing MDM, 
          * we try to look for this process's info in the lock table and clean it up.
          * Since this process is just starting, it cannot have any locks, 
          * so anything in the lock table must be an old entry that should be cleaned up.
          */
         lck_clearDeadThreadByTid(sysUtl_gettid());

         /*
          * Caller's copy of shared memory allocator just needs to know
          * the address of the allocatable region.  The shared memory
          * allocator has already been fully initialized by the first
          * caller who created the shared memory region.
          */
         cmsMem_initSharedMemPointer(mdmShmCtx->lockKeyOffset,
                          mdmShmCtx->mallocStart,
                          mdmShmCtx->shmEnd - ((char *) mdmShmCtx->mallocStart));

         *shmAddr = addr;
         return CMSRET_SUCCESS;
      }
   }


   /*
    * OK, if we get here, then shmId must be -1, which means we
    * have to create the shared memory region and do all the hard
    * work of copying the MDM data structures into the shared memory region.
    */

   /*
    * Create the shared memory region.  Use shmSize, not config->shmSize.
    */
   cmsLog_notice("creating new shared memory region at %p, size=%d (0x%x)",
                 config->shmAttachAddr, shmSize, shmSize);
   shmflg = 0666; /* allow everyone to read/write */
   shmflg |= (IPC_CREAT | IPC_EXCL);

   if ((id = shmget(0, shmSize, shmflg)) == -1)
   {
      cmsLog_error("Could not create shared memory.");
      return CMSRET_INTERNAL_ERROR;
   }
   else
   {
      cmsLog_debug("created shared memory, shmid=%d", id);
   }

   cmsLog_debug("attaching to shared mem at %p", config->shmAttachAddr);
   if ((addr = shmat(id, config->shmAttachAddr, 0)) == (void *) -1)
   {
      cmsLog_error("could not attach %d at %p", id, config->shmAttachAddr);
      oalShm_cleanup(id, NULL);
      return CMSRET_INTERNAL_ERROR;
   }
   if (addr != config->shmAttachAddr)
   {
      cmsLog_error("actual (%p) and requested addr (%p) differ!",
                   addr, config->shmAttachAddr);
      oalShm_cleanup(id, NULL);
      return CMSRET_INTERNAL_ERROR;
   }

   mdmShmCtx = (MdmSharedMemContext *) addr;
   mdmShmCtx->shmEnd = (char *) (addr + shmSize);
   cmsLog_notice("New shared mem: addr=%p mdmShmCtx=%p end=%p",
                addr, mdmShmCtx, mdmShmCtx->shmEnd);

   mdmShmCtx->flags = config->flags;
   cmsLog_notice("Copied flags 0x%x", mdmShmCtx->flags);

   /*
    * Initialize the MDM xlock earlier because it is easy to undo if we
    * encounter errors later on.
    */
   if ((ret = oalLck_init(FALSE, config->lockKeyOffset)) != CMSRET_SUCCESS)
   {
      cmsLog_error("oalLck_init failed, ret=%d", ret);
      oalShm_cleanup(id, NULL);
      return ret;
   }
   mdmShmCtx->lockKeyOffset = config->lockKeyOffset;

   /* figure out which data model shared lib we need to open and load */
   {
      SINT32 dataModelRetVal;

      dataModelRetVal = mdm_initDataModelSelection();
      if (dataModelRetVal == 0)
      {
         // CMS Classic: monolithic TR98 Hybrid data model
         mdmFilename = "libmdm.so";
         mdmShmCtx->isDataModelDevice2 = 0;
      }
      else if (dataModelRetVal == 1)
      {
         if (config->mdmLibName != NULL)
         {
            // This is probably a Distributed MDM, which implies TR181.
            mdmFilename = config->mdmLibName;
         }
         else
         {
            // CMS Classic: monolithic PURE181 data model
            mdmFilename = "libmdm2.so";
         }
         mdmShmCtx->isDataModelDevice2 = 1;
      }
      else
      {
         cmsLog_error("mdm_initDataModelSelection failed, retval=%d", dataModelRetVal);
         return CMSRET_INTERNAL_ERROR;
      }
   }

   rootObjNode = getRootObjNode(mdmFilename, &libmdmHandle);
   if (rootObjNode == NULL)
   {
      oalLck_cleanup();
      oalShm_cleanup(id, addr);
      return CMSRET_INTERNAL_ERROR;
   }
   cmsLog_debug("got dlsym rootObjNode at %p", rootObjNode);

   if (config->remoteMdmLibName)
   {
      remoteRootObjNode = getRootObjNode(config->remoteMdmLibName, &remoteLibmdmHandle);
      if (remoteRootObjNode == NULL)
      {
         dlclose(libmdmHandle);
         oalLck_cleanup();
         oalShm_cleanup(id, addr);
         return CMSRET_INTERNAL_ERROR;
      }
      cmsLog_debug("got dlsym remoteRootObjNode at %p", remoteRootObjNode);
   }

   /*
    * Copy contents of MDM shared library (libmdm_xxx.so) to shared memory
    * and close the handle after we are done.
    */
   consumed = copyToSharedMem(rootObjNode, remoteRootObjNode);
   dlclose(libmdmHandle);
   libmdmHandle = NULL;
   if (remoteLibmdmHandle)
   {
      dlclose(remoteLibmdmHandle);
      remoteLibmdmHandle = NULL;
   }

   if (consumed == 0)
   {
      oalLck_cleanup();
      oalShm_cleanup(id, addr);
      return CMSRET_RESOURCE_EXCEEDED;
   }



   /*
    * Let the shared memory allocator know where it can allocate shared
    * memory from.  As a heuristic, the shared memory allocator should
    * have at least half the shared memory region available to it.
    * If we become heavily invested in the shared mem approach, the
    * shared memory allocator (bget) does have the ability to accept more
    * memory into its pool, so we could slowly grow it on an as needed basis.
    * This will cause more complexity in the code, of course.
    */
   mdmShmCtx->mallocStart = addr + consumed;

   if (shmSize - consumed < (shmSize/3))
   {
      cmsLog_error("insufficient memory for shared memory allocator, got %d, need %d",
                   shmSize - consumed, (shmSize/3));
      oalLck_cleanup();
      oalShm_cleanup(id, addr);
      return CMSRET_RESOURCE_EXCEEDED;
   }
   else
   {
      cmsLog_debug("init shared mem allocator with %p, size=%d",
                   mdmShmCtx->mallocStart, shmSize - consumed);
      cmsMem_initSharedMem(mdmShmCtx->lockKeyOffset,
                           mdmShmCtx->mallocStart, shmSize - consumed);
   }

   {
      unsigned int sysTotal=0;
      unsigned int sysFree=0;
      char nameBuf[64]={0};
      ProcThreadInfo threadInfo;
      int threadId=0;
      int rv;

      memset(&threadInfo, 0, sizeof(threadInfo));
      threadId = sysUtl_getThreadId();
      rv = sysUtl_getThreadInfoFromProc(threadId, &threadInfo);
      if (rv == 0)
      {
         snprintf(nameBuf, sizeof(nameBuf), "%s", threadInfo.name);
      }
      else
      {
         snprintf(nameBuf, sizeof(nameBuf), "eid %d", config->eid);
      }

      sysUtl_getMemInfo(&sysTotal, &sysFree);
      printf("%s: created shared mem at %p size %dKB (free=%dKB, shmId=%d); [system mem: total=%dKB free=%dKB]\n",
             nameBuf, addr, shmSize/1024, (shmSize-consumed)/1024, id,
             sysTotal, sysFree);
   }

   *shmId = id;
   *shmAddr = addr;

   return ret;
}


void oalShm_cleanup(SINT32 shmId, void *shmAddr)
{
   struct shmid_ds shmbuf;

   /*
    * stat the shared memory to see how many processes are attached.
    */
   memset(&shmbuf, 0, sizeof(shmbuf));
   if (shmctl(shmId, IPC_STAT, &shmbuf) < 0)
   {
      cmsLog_error("shmctl IPC_STAT failed");
      return;
   }
   else
   {
      cmsLog_debug("nattached=%d", shmbuf.shm_nattch);
   }



   if (shmbuf.shm_nattch > 1)
   {
      /* other proceeses are still attached, just detach myself and return now. */
      detachShm(shmAddr);
      return;
   }

   /*
    * No other processes attached to memory region, do final cleanup:
    * First terminate remote_objd (if there is one), then
    * free all the dynamically allocated shared memory (this will make the memory leak checkers happy),
    * then detach myself, and then destroy the shared memory region.
    */

   if (mdmShmCtx->isRemoteCapable)
   {
      odl_stopRemoteObjd(mdmShmCtx->compName);
   }

   cmsLog_debug("freeing all MdmObjects");
   freeObjectMemory(mdmShmCtx->rootObjNode);

   cmsMem_cleanup();

   oalLck_cleanup();

   detachShm(shmAddr);

   memset(&shmbuf, 0, sizeof(shmbuf));
   if (shmctl(shmId, IPC_RMID, &shmbuf) < 0)
   {
      cmsLog_error("shm destory of shmId=%d failed.", shmId);
   }
   else
   {
      cmsLog_debug("shared mem (shmId=%d) destroyed.", shmId);
   }

   return;
}


/** Detach the shared memory address. */
void detachShm(void *shmAddr)
{
   if (shmAddr == NULL)
   {
      cmsLog_error("got uninitialized shmAddr, no detach needed");
   }
   else
   {
      if (shmdt(shmAddr) != 0)
      {
         cmsLog_error("shmdt of shmAddr=%p failed", shmAddr);
      }
      else
      {
         cmsLog_debug("detached shmAddr=%p", shmAddr);
      }
   }
}


UINT32 getMdmSize(const MdmObjectNode *objNode)
{
   UINT32 i, s=0;

   if (objNode == NULL)
      return 0;

   /* first count my own size */
   s = sizeof(MdmObjectNode);

   /* get the size of my parameters */
   s += objNode->numParamNodes * sizeof(MdmParamNode);

   /* get the size of my children/sub-trees */
   for (i=0; i < objNode->numChildObjNodes; i++)
   {
      /* recurse */
      s += getMdmSize(&(objNode->childObjNodes[i]));
   }

   return s;
}

UINT32 getMdmObjectCount(const MdmObjectNode *objNode)
{
   UINT32 i, cnt=0;

   if (objNode == NULL)
      return 0;

   /* count myself */
   cnt = 1;

   /* get the size of my children/sub-trees */
   for (i=0; i < objNode->numChildObjNodes; i++)
   {
      /* recurse */
      cnt += getMdmObjectCount(&(objNode->childObjNodes[i]));
   }

   return cnt;
}


/** Copy all MDM objects from dlOpened shared lib to shared memory.
 * 
 *  This function is called only once even if we have to copy over remote
 *  objects.
 *
 * @returns number of bytes used by initial MDM data structures, or 0 on error.
 */
UINT32 copyToSharedMem(const MdmObjectNode *rootObjNode,
                       const MdmObjectNode *remoteRootObjNode)
{
   MdmSharedMemContext *shmCtx = mdmShmCtx;
   UINT32 consumed;
   UINT32 mdmSize, s1, s2=0;
   UINT32 localNodeCount, remoteNodeCount;
   char *nodeCopyAddr, *strCopyAddr, *strEnd;
   struct str_entry *strMap=NULL;
   struct vsa_entry *vsaMap=NULL;

   cmsLog_notice("Entered: mdmShmCtx=%p, rootObjNode=%p remoteRootObjNode=%p",
                 mdmShmCtx, rootObjNode, remoteRootObjNode);

   // To be on the safe side, do not assume that the remoteObjectNode is a 
   // superset of the (local) objNodes.  Handle worse case scenario where they
   // are disjoint.
   s1 = getMdmSize(rootObjNode);
   s2 = getMdmSize(remoteRootObjNode);
   mdmSize = s1 + s2;

   localNodeCount = getMdmObjectCount(rootObjNode);
   remoteNodeCount = getMdmObjectCount(remoteRootObjNode);

   cmsLog_notice("mdm size = %d (%d+%d) localNodeCount=%d remoteNodeCount=%d",
                mdmSize, s1, s2, localNodeCount, remoteNodeCount);

   /* set the various pointers in the shmCtx */
   shmCtx->lockMeta = (MdmLockMetaInfo *) (shmCtx + 1);

   shmCtx->localOidNodeTableBegin = (OidNodeEntry *) (shmCtx->lockMeta + 1);
   shmCtx->localOidNodeTableEnd = shmCtx->localOidNodeTableBegin + localNodeCount;
   if (remoteRootObjNode)
   {
      // remoteOidNode table starts after the local oidNodeTable
      shmCtx->remoteOidNodeTableBegin = shmCtx->localOidNodeTableEnd;
      shmCtx->remoteOidNodeTableEnd = shmCtx->remoteOidNodeTableBegin + remoteNodeCount;
      shmCtx->rootObjNode = (MdmObjectNode *) (shmCtx->remoteOidNodeTableEnd);
   }
   else
   {
      shmCtx->remoteOidNodeTableBegin = NULL;
      shmCtx->remoteOidNodeTableEnd = NULL;
      shmCtx->rootObjNode = (MdmObjectNode *) (shmCtx->localOidNodeTableEnd);
   }

   shmCtx->stringsStart = (char *) (((char *)shmCtx->rootObjNode) + mdmSize);
   
   //print the address only, not format error
   cmsLog_debug("ShmCtx (start)       = %p (0x%x)", (char *) shmCtx, sizeof(MdmSharedMemContext));
   cmsLog_debug("LockMetaInfo         = %p (+0x%x)", shmCtx->lockMeta, sizeof(MdmLockMetaInfo));
   cmsLog_debug("localOidNodeTable    = %p (+0x%x)", shmCtx->localOidNodeTableBegin, localNodeCount*sizeof(OidNodeEntry));
   cmsLog_debug("remoteOidNodeTable   = %p (+0x%x)", shmCtx->remoteOidNodeTableBegin, remoteNodeCount*sizeof(OidNodeEntry));
   cmsLog_debug("rootObjNode          = %p (+0x%x)", shmCtx->rootObjNode, mdmSize);
   cmsLog_debug("stringStart          = %p (size TBD)", shmCtx->stringsStart);
   cmsLog_debug("shmEnd               = %p", shmCtx->shmEnd);

   if (shmCtx->stringsStart > shmCtx->shmEnd)
   {
      /* not even enough space to hold the fixed sized data */     
      cmsLog_error("not enough space: start %p > end %p",
                   shmCtx->stringsStart,  shmCtx->shmEnd);
      return 0;
   }

   // Due to various call sequence issues, we need to zero out lockMeta here
   // instead of in the lock init function.
   // I vaguely remember the shared mem is guaranteed to be initialized to 0,
   // but just to be safe, zeroize everthing we are about to use.
   memset(shmCtx->lockMeta, 0, shmCtx->stringsStart - ((char *)shmCtx->lockMeta));

   /*
    * Using a recursive function, I can copy over the MdmObjNode and MdmParamNode
    * structures, update the oidNodeEntryTable, and copy over the validStrings.
    * The first pass is for the local MDM objects.
    */
   nodeCopyAddr = (char *) shmCtx->rootObjNode;
   strCopyAddr = shmCtx->stringsStart;

   strEnd = copyNode((MdmObjectNode *) rootObjNode,
                     &nodeCopyAddr,
                     &strCopyAddr,
                     &vsaMap,
                     &strMap,
                     FALSE, // first pass is for the local objects
                     shmCtx->shmEnd);
   {
      int k=0;
      OidNodeEntry *curr = shmCtx->localOidNodeTableBegin;

      const MdmOidInfoEntry *begin=NULL;
      const MdmOidInfoEntry *end=NULL;
      mdm_getOidInfoPtrs(&begin, &end);

      // The number of entries and the oid's sin the OidInfoArray should
      // match what we have in the localOidNodeTable.
      cmsLog_debug("===== VERIFY LOCAL OID TABLE ====");
      while (curr < shmCtx->localOidNodeTableEnd)
      {
         if (curr->oid == begin[k].oid)
            cmsLog_debug("[%d] matched oid=%d", k, curr->oid);
         else
            cmsLog_error("[%d] MISMATCH oid=%d infoOid=%d", k, curr->oid, begin[k].oid);
         k++; curr++;
      }
      if (k == end+1-begin)  // end points to the last valid entry, so +1 to make math work
      {
         cmsLog_debug("entry counts matched (%d)", k);
      }
      else
      {
         cmsLog_error("Entry count MISMATCH: local table=%d oidInfoArray=%d",
                      k, end+1-begin);
      }
   }

   if (strEnd != NULL && remoteRootObjNode)
   {
      strEnd = copyNode((MdmObjectNode *) remoteRootObjNode,
                     &nodeCopyAddr,
                     &strCopyAddr,
                     &vsaMap,
                     &strMap,
                     TRUE, // second pass is for the remote objects
                     shmCtx->shmEnd);
      // We don't fill the entire remoteOidNodeTable,
      // so update the end ptr to make future searches faster.
      {
         OidNodeEntry *curr = shmCtx->remoteOidNodeTableEnd-1;
         while ((curr->oid == 0) &&
                (curr > shmCtx->remoteOidNodeTableBegin))
         {
            curr--;
         }
         if (curr <= shmCtx->remoteOidNodeTableBegin)
         {
            cmsLog_error("Remote table has 0 entries!!");
         }
         shmCtx->remoteOidNodeTableEnd = curr+1;
         cmsLog_debug("pulled remoteOidNodeTableEnd up to %p (numEntries=%d)",
                      shmCtx->remoteOidNodeTableEnd,
                      shmCtx->remoteOidNodeTableEnd-shmCtx->remoteOidNodeTableBegin);
      }

      // Dump the remote table.  We do not have a remoteOidInfoPtrs, so
      // cannot verify these like we did for the local table.
      {
         int k=0;
         OidNodeEntry *curr = shmCtx->remoteOidNodeTableBegin;
         while (curr < shmCtx->remoteOidNodeTableEnd)
         {
            cmsLog_debug("[%d]%p oid=%d", k, curr, curr->oid);
            curr++; k++;
         }
      }
   }

   freeVsaMap(&vsaMap);
   freeStrMap(&strMap);

   if (strEnd == NULL)
   {
      /* not enough space to copy the strings */
      cmsLog_error("not enough space for strings");
      return 0;
   }

   /* round up strEnd to 4 byte boundary */
   strEnd = ROUNDUP_WORD(strEnd);

   consumed = ((char *) strEnd) - ((char *) mdmShmCtx);
   cmsLog_debug("stringsStart    = %p (+0x%x)",
                                   shmCtx->stringsStart,
                                   strEnd - shmCtx->stringsStart);                                 
   cmsLog_debug("stringEnd      = %p", strEnd); 
   cmsLog_debug("shmEnd         = %p", shmCtx->shmEnd);
   cmsLog_debug("returning consumed=%d", consumed);
   return consumed;
}


static void orderedOidInsert(OidNodeEntry *begin, OidNodeEntry *end,
                        MdmObjectId oid, MdmObjectNode *objNode)
{
   OidNodeEntry *curr = begin;
   OidNodeEntry *prev = NULL;

   if ((begin == NULL) || (end == NULL))
   {
      cmsLog_error("NULL begin %p or end %p");
      return;
   }

   cmsLog_debug("Entered: begin=%p end=%p oid=%d", begin, end, oid);

   // Find the insertion point
   while ((curr->oid != 0) &&
          (curr->oid < oid) &&
          (curr < end))
   {
      //cmsLog_debug("skip over %d", curr->oid);
      curr++;
   }

   if (curr >= end)
   {
      cmsLog_error("out of space! begin=%p end=%p oid=%d",
                   begin, end, oid);
      return;
   }

   if (curr->oid == oid)
   {
      cmsLog_error("duplicate oid %d", oid);
      return;
   }

   // Current slot is empty (end of populated portion of array) just insert
   if (curr->oid == 0)
   {
      curr->oid = oid;
      curr->objNode = objNode;
      cmsLog_debug("inserted at end (oid=%d), curr=%p", oid, curr);
      return;
   }

   // Need to shift everything down by 1 to make room for this oid.
   end--;  // end now points to last available slot, which should be empty
   if (end->oid != 0)
   {
      cmsLog_error("last slot is occupied, out of space!");
      return;
   }
   while (end > curr)
   {
      prev = end - 1;
      *end = *prev;
      end--;
   }
   // cmsLog_debug("prev=%p curr=%p shifted down %d for %d", prev, curr, prev->oid, oid);
   curr->oid = oid;
   curr->objNode = objNode;
   cmsLog_debug("inserted oid %d at %p", oid, curr);
   return;
}


#define ALIGN_OFFSET(f, a) (((f) + (a)) / (a) * (a))

/** This is a recursive function which will be called for every MdmObjectNode in the
 *  data model.
 *
 *  For each node:
 *    1. update the oidNodePtrTable with shared memory addr of MdmObjectNode
 *    2. copy the MdmObjNode and all of its MdmParamNodes to shared memory
 *    3. copy over any valid strings array used by the MdmParamNode, taking care not
 *       to go over shmEnd.
 *
 * This function will be called twice:
 *   First pass, for local objects (isRemoteObjPass == FALSE)
 *   Second pass, for remote objects (isRemoteObjPass == TRUE)
 *
 * @return pointer to the next byte after the end of valid strings, or NULL if
 *         valid strings did not fit inside shmend.
 */
char *copyNode(const MdmObjectNode *objNode,
               char **nodeCopyAddr,
               char **strCopyAddr,
               struct vsa_entry **vsaMap,
               struct str_entry **strMap,
               UBOOL8 isRemoteObjPass,
               const char *shmEnd)
{
   MdmObjectNode *localOidNode = NULL;
   UINT32 i;
   SINT32 offsetInObject=0;
   MdmObjectNode *shmObjNode=NULL;
   MdmObjectNode *parentShmObjNode=NULL;
   MdmObjectNode *childNodesBegin=NULL;
   UINT16 actualChildNodesCopied=0;
   char *strEnd;
   UBOOL8 isPrevParam64=FALSE;
   UBOOL8 isPrevParamBool=FALSE;
   SINT32 align64=8;
#ifdef DESKTOP_LINUX
#if !defined(__LP64__)
   /* the only time 64 bit types are 4 byte aligned is on Intel -m32 mode */
   align64 = 4;
#endif
#endif

   if ((*nodeCopyAddr == (char *) mdmShmCtx->rootObjNode) ||
       (!strcmp(objNode->genericFullpath, "Device.")))
   {
      cmsLog_notice("[remotePass=%d] Copying rootObjNode %s (oid=%d)",
                    isRemoteObjPass, objNode->genericFullpath, objNode->oid);
      /* this is the root object node */
      /* nodeCopyAddr points to the address in shmem where the root node should go */
      /* This block will execute on the first node of the first and second (remote) pass. */
      shmObjNode = (MdmObjectNode *) (*nodeCopyAddr);

      /* copy the MdmObjectNode to shared mem */
      memcpy((void *) shmObjNode, (void *) objNode, sizeof(MdmObjectNode));

      /* deep copy strings to shared mem */
      if ((shmObjNode->name = getSharedStr(strMap, objNode->name, strCopyAddr, shmEnd)) == NULL)
         return NULL;
      if ((shmObjNode->genericFullpath = getSharedStr(strMap, objNode->genericFullpath, strCopyAddr, shmEnd)) == NULL)
         return NULL;
      if ((shmObjNode->profile = getSharedStr(strMap, objNode->profile, strCopyAddr, shmEnd)) == NULL)
         return NULL;

      /* update oidNodeTable with MdmObjectNode's shared mem addr */
      if (isRemoteObjPass)
      {
         orderedOidInsert(mdmShmCtx->remoteOidNodeTableBegin,
                          mdmShmCtx->remoteOidNodeTableEnd,
                          objNode->oid, shmObjNode);
      }
      else
      {
         orderedOidInsert(mdmShmCtx->localOidNodeTableBegin,
                          mdmShmCtx->localOidNodeTableEnd,
                          objNode->oid, shmObjNode);
      }

      /* advance nodeCopyAddr */
      (*nodeCopyAddr) += sizeof(MdmObjectNode);
   }
   else
   {
      cmsLog_notice("[remotePass=%d] processing %s (oid=%d)",
                    isRemoteObjPass, objNode->genericFullpath, objNode->oid);

      /*
       * For all object nodes other than the root object node, the parent
       * has already copied all the children obj nodes over and updated
       * the oidNodePtrTable.  Just get the new shmObjNode pointer from
       * the oidNodePtrTable.
       */
      if (isRemoteObjPass)
      {
         shmObjNode = mdm_getRemoteObjNode(objNode->oid);
      }
      else
      {
         shmObjNode = mdm_getLocalObjNode(objNode->oid);
      }
      if (shmObjNode == NULL)
      {
         cmsLog_error("[remotePass=%d] Could not find oid %d in OidPtrTable",
                      isRemoteObjPass, objNode->oid);
         return NULL;
      }
   }

   /* Copy the params for this obj */
   /* the params of this objNode will start at nodeCopyAddr */
   shmObjNode->params = (objNode->numParamNodes > 0) ?
                        ((MdmParamNode *) (*nodeCopyAddr)) : NULL;

   /* copy the param nodes, the param nodes will be contiguous */
   for (i=0; i < objNode->numParamNodes; i++)
   {
      const MdmParamNode *paramNode = &(objNode->params[i]);
      MdmParamNode *shmParamNode = (MdmParamNode *) (*nodeCopyAddr);

      /* copy the MdmParamNode to shared mem */
      memcpy((void *) shmParamNode, (void *) paramNode, sizeof(MdmParamNode));

      /*
       * Fill in the offsetInObject field.  This calculation must be done
       * after the preprocessor has compiled out the undefined profiles.
       * Note that all MDM objects start with the following fields:
       * MdmObjectId (UINT16)
       * SequenceNum (UINT16)
       */
      if (oalMdm_isParam8(shmParamNode->type))
      {
         if (isPrevParamBool)
         {
            /* just another bool following a bool, advance 1 byte */
            offsetInObject += 1;
         }
         else if (isPrevParam64)
         {
             offsetInObject += 8;
         }
         else
         {
             /* prevParam must be int.  Advance 4 bytes. */
             offsetInObject += 4;
         }

         isPrevParamBool = TRUE;
         isPrevParam64 = FALSE;
      }
      else if (oalMdm_isParam64(shmParamNode->type))
      {
          if (isPrevParamBool)
          {
              offsetInObject = ALIGN_OFFSET(offsetInObject, align64);
          }
          else if (isPrevParam64)
          {
              offsetInObject += 8;
          }
          else
          {
              /* prevParam must be int.  Advance to next aligned boundary. */
              offsetInObject = ALIGN_OFFSET(offsetInObject, align64);
          }

          isPrevParamBool = FALSE;
          isPrevParam64 = TRUE;
      }
      else
      {
         /* current parameter must be a 32 bit type */
         if (isPrevParamBool)
         {
             /* align to the next 4 byte boundary */
             offsetInObject = ALIGN_OFFSET(offsetInObject, 4);
         }
         else if (isPrevParam64)
         {
             offsetInObject += 8;
         }
         else
         {
             /* prev param was also 32 bits, so just advance 4 */
             offsetInObject += 4;
         }

         isPrevParamBool = FALSE;
         isPrevParam64 = FALSE;
      }

      shmParamNode->offsetInObject = offsetInObject;


      /* copy name to shared mem */
      if ((shmParamNode->name = getSharedStr(strMap, paramNode->name, strCopyAddr, shmEnd)) == NULL)
      {
         return NULL;
      }
/*    cmsLog_debug("paramNode %p name=%s(%p)", shmParamNode, shmParamNode->name, shmParamNode->name); */

      /* copy profile name to shared mem */
      if ((shmParamNode->profile = getSharedStr(strMap, paramNode->profile, strCopyAddr, shmEnd)) == NULL)
      {
         return NULL;
      }

      /* copy default value, if there is one, to shared mem. */
      if (paramNode->defaultValue != NULL)
      {
         if ((shmParamNode->defaultValue = getSharedStr(strMap, paramNode->defaultValue, strCopyAddr, shmEnd)) == NULL)
         {
            return NULL;
         }
      }

      /* check for valid strings */
      if ((paramNode->type == MPT_STRING) && (paramNode->vData.min != NULL))
      {
         shmParamNode->vData.min = getSharedVsa(vsaMap,
                                                strMap,
                                                (const char *) paramNode->vData.min,
                                                strCopyAddr,
                                                shmEnd);

         if (shmParamNode->vData.min == NULL)
         {
            /* out of space */
            return NULL;
         }
      }

      /* check for decimal range */
      if ((paramNode->type == MPT_DECIMAL) && (paramNode->vData.min != NULL))
      {
         if ((shmParamNode->vData.min = getSharedStr(strMap, paramNode->vData.min, strCopyAddr, shmEnd)) == NULL)
         {
            /* out of space */
            return NULL;
         }

         if (paramNode->vData.max != NULL)
         {
            if ((shmParamNode->vData.max = getSharedStr(strMap, paramNode->vData.max, strCopyAddr, shmEnd)) == NULL)
            {
               /* out of space */
               return NULL;
            }
         }
      }

      shmParamNode->parent = shmObjNode;
      (*nodeCopyAddr) += sizeof(MdmParamNode);
   }


   // All child obj nodes of this obj node are copied in a contiguous block.
   // Remember the start address of the child obj nodes.
   // Remember the address of this node (the parent node).
   childNodesBegin = (MdmObjectNode *) (*nodeCopyAddr);
   parentShmObjNode = shmObjNode;

   for (i=0; i < objNode->numChildObjNodes; i++)
   {
      const MdmObjectNode *childNode = &(objNode->childObjNodes[i]);
      shmObjNode = (MdmObjectNode *) (*nodeCopyAddr);

      // Stick a pointer to this MdmObjectNode in the correct table.
      if (isRemoteObjPass)
      {
         // On the second pass, if the child oid is not in the (local)
         // oidNodePtrTable, or it is in the local table but is a
         // MULTI_COMP_OBJ, then copy to remoteOidNodePtrTable.
         localOidNode = mdm_getLocalObjNode(childNode->oid);
         if ((localOidNode == NULL) ||
             (localOidNode->flags & OBN_MULTI_COMP_OBJ))
         {
            cmsLog_debug("copy remote child oid %d (%s)",
                          childNode->oid, childNode->genericFullpath);
            /* copy the MdmObjectNode to shared mem */
            memcpy((void *) shmObjNode, (void *) childNode, sizeof(MdmObjectNode));
            actualChildNodesCopied++;
            /* deep copy strings to shared mem */
            if ((shmObjNode->name = getSharedStr(strMap, childNode->name, strCopyAddr, shmEnd)) == NULL)
               return NULL;
            if ((shmObjNode->genericFullpath = getSharedStr(strMap, childNode->genericFullpath, strCopyAddr, shmEnd)) == NULL)
               return NULL;
            if ((shmObjNode->profile = getSharedStr(strMap, childNode->profile, strCopyAddr, shmEnd)) == NULL)
               return NULL;

            orderedOidInsert(mdmShmCtx->remoteOidNodeTableBegin,
                             mdmShmCtx->remoteOidNodeTableEnd,
                             childNode->oid, shmObjNode);
            (*nodeCopyAddr) += sizeof(MdmObjectNode);
         }
      }
      else
      {
         // This is the first pass.  Always update localOidNodeTable.
         cmsLog_debug("copy local child oid %d (%s)", childNode->oid, childNode->genericFullpath);
         /* copy the MdmObjectNode to shared mem */
         memcpy((void *) shmObjNode, (void *) childNode, sizeof(MdmObjectNode));
         actualChildNodesCopied++;
         /* deep copy strings to shared mem */
         if ((shmObjNode->name = getSharedStr(strMap, childNode->name, strCopyAddr, shmEnd)) == NULL)
            return NULL;
         if ((shmObjNode->genericFullpath = getSharedStr(strMap, childNode->genericFullpath, strCopyAddr, shmEnd)) == NULL)
            return NULL;
         if ((shmObjNode->profile = getSharedStr(strMap, childNode->profile, strCopyAddr, shmEnd)) == NULL)
            return NULL;

         orderedOidInsert(mdmShmCtx->localOidNodeTableBegin,
                          mdmShmCtx->localOidNodeTableEnd,
                          childNode->oid, shmObjNode);
         (*nodeCopyAddr) += sizeof(MdmObjectNode);
      }
   }

   cmsLog_debug("[isRemotePass=%d][%s] numChildNodes=%d actualChildNodes copied=%d",
                isRemoteObjPass, parentShmObjNode->genericFullpath,
                parentShmObjNode->numChildObjNodes, actualChildNodesCopied);
   parentShmObjNode->numChildObjNodes = actualChildNodesCopied;
   if (actualChildNodesCopied > 0)
   {
      parentShmObjNode->childObjNodes = childNodesBegin;
   }

   /* recurse to child objects */
   for (i=0; i < objNode->numChildObjNodes; i++)
   {
      const MdmObjectNode *childNode = &(objNode->childObjNodes[i]);
      localOidNode = mdm_getLocalObjNode(childNode->oid);

      // Always recurse to child objs on first pass,
      // On the second pass, recurse if the child obj is not in local table
      // or if it is a multi-comp obj.
      if ((isRemoteObjPass == 0) ||
          (localOidNode == NULL) ||
          (localOidNode->flags & OBN_MULTI_COMP_OBJ))
      {
         strEnd = copyNode(childNode,
                           nodeCopyAddr,
                           strCopyAddr,
                           vsaMap,
                           strMap,
                           isRemoteObjPass,
                           shmEnd);

         if (strEnd == NULL)
         {
            cmsLog_error("No more space in MDM, curr oid=%d", childNode->oid);
            return NULL;
         }
      }
   }

   return (*strCopyAddr);
}



/** Find a vsa_entry entry with the specified libAddr, if there is no such entry, allocate
 *  a new vsa_entry entry and link it into the list.
 *
 * @param vsaMap (IN/OUT) pointer to head of list ov vsa_entries.
 * @param libAddr (IN)    address of valid strings array in the libmdm.so.
 * @return pointer to vsa_entry which could be an existing or new entry.  Or may return
 *         NULL if memory allocation failed.
 */
char *getSharedVsa(struct vsa_entry **vsaMap,
                   struct str_entry **strMap,
                   const char *libAddr,
                   char **strCopyAddr,
                   const char *shmEnd)
{
   struct vsa_entry *tmpEntry = (*vsaMap);


   while ((tmpEntry != NULL) && (tmpEntry->libAddr != libAddr))
   {
      tmpEntry = tmpEntry->next;
   }

   if (tmpEntry != NULL)
   {
      /* we must have found an entry, return it. */
      return tmpEntry->shmAddr;
   }


   /*
    * If we get here, then this is a valid strings array we have not seen before.
    * Allocate a new vsa_entry and link it into the list so we can re-use references
    * to the same valid strings array.  Then we need to copy over the array of
    * pointers.  Then we need to copy over the strings.
    */
   tmpEntry = cmsMem_alloc(sizeof(struct vsa_entry), ALLOC_ZEROIZE);
   if (tmpEntry == NULL)
   {
      cmsLog_error("malloc of %d bytes failed", sizeof(struct vsa_entry));
      return NULL;
   }

   tmpEntry->libAddr = libAddr;
   *strCopyAddr = ROUNDUP_WORD(*strCopyAddr); /* array of pointers must be on word boundary */
   tmpEntry->shmAddr = (*strCopyAddr);
   tmpEntry->next = (*vsaMap);
   (*vsaMap) = tmpEntry;

   if (copyVsa(strMap, (const char **) libAddr, strCopyAddr, shmEnd) == NULL)
   {
      return NULL;
   }

   return tmpEntry->shmAddr;
}


/** Copy a valid string array (array of pointers and strings) to shared memory addr.
 */
char *copyVsa(struct str_entry **strMap, const char **libArray, char **strCopyAddr, const char *shmEnd)
{
   UINT32 j=0;
   UINT32 numEntries=1; /* also count last NULL as an entry */
   char **shmArray;

   while (libArray[j] != NULL)
   {
      numEntries++;
      j++;
   }

   if ((*strCopyAddr) + (numEntries * sizeof(char *)) > shmEnd)
   {
      return NULL;
   }

   shmArray = (char **) (*strCopyAddr);
   memcpy(shmArray, libArray, numEntries * sizeof(char *));
   (*strCopyAddr) += numEntries * sizeof(char *);

/*  cmsLog_debug("vsa %p, numEntries=%d", shmArray, numEntries); */

   /* go through the vsArray again, this time copying the strings over */
   j=0;
   while (libArray[j] != NULL)
   {
      shmArray[j] = getSharedStr(strMap, libArray[j], strCopyAddr, shmEnd);
      if (shmArray[j] == NULL)
      {
         return NULL;
      }

      j++;
   }


   return (char *) shmArray;
}



/** Find a the specified string in the shared memory; if not in shared memory, copy
 *  the specified string to shared memory.
 *
 * @param strMap (IN/OUT) pointer to head of list of str__entries.
 * @param libStr (IN)     pointer to string in the libmdm.so.
 * @return pointer to string in shared memory, or NULL if memory allocation failed
 *         or if there is not enough shared memory to hold the string.
 */
char *getSharedStr(struct str_entry **strMap, const char *libStr, char **strCopyAddr, const char *shmEnd)
{
   UINT32 len;
   struct str_entry *tmpEntry = (*strMap);

   while ((tmpEntry != NULL) && strcmp(tmpEntry->shmStr, libStr))
   {
      tmpEntry = tmpEntry->next;
   }

   if (tmpEntry != NULL)
   {
      /* we must have found an entry, return it. */
      return tmpEntry->shmStr;
   }


   /*
    * If we get here, we need to copy the string from libmdm.so to shared mem.
    * First create a str_entry so we can re-use references to the same string.
    */
   tmpEntry = cmsMem_alloc(sizeof(struct str_entry), ALLOC_ZEROIZE);
   if (tmpEntry == NULL)
   {
      cmsLog_error("malloc of %d bytes failed", sizeof(struct str_entry));
      return NULL;
   }

   tmpEntry->next = (*strMap);
   (*strMap) = tmpEntry;


   len = strlen(libStr) + 1;
   if ((*strCopyAddr) + len > shmEnd)
   {
      /* not enough space to copy over string */
      return NULL;
   }


   tmpEntry->shmStr = (*strCopyAddr);
   memcpy(tmpEntry->shmStr, libStr, len);
   (*strCopyAddr) += len;

/* cmsLog_debug("new string (%p) %s", tmpEntry->shmStr, libStr); */

   return tmpEntry->shmStr;
}


/** Free all entries in the vsaMap list.
 */
void freeVsaMap(struct vsa_entry **vsaMap)
{
   struct vsa_entry *tmpEntry;

   while ((*vsaMap) != NULL)
   {
      tmpEntry = (*vsaMap)->next;
      (*vsaMap)->next = NULL;
      cmsMem_free(*vsaMap);
      (*vsaMap) = tmpEntry;
   }

   return;
}


/** Free all entries in the strMap list.
 */
void freeStrMap(struct str_entry **strMap)
{
   struct str_entry *tmpEntry;

   while ((*strMap) != NULL)
   {
      tmpEntry = (*strMap)->next;
      (*strMap)->next = NULL;
      cmsMem_free(*strMap);
      (*strMap) = tmpEntry;
   }

   return;
}


UBOOL8 oalMdm_isParam8(MdmParamTypes type)
{
   /* there is only one 8-bit type */
   return (type == MPT_BOOLEAN);
}

UBOOL8 oalMdm_isParam64(MdmParamTypes type)
{
   UBOOL8 ret=0;

   switch(type)
   {
      case MPT_LONG64:
      case MPT_UNSIGNED_LONG64:
#ifdef __LP64__
      case MPT_STRING:
      case MPT_DATE_TIME:
      case MPT_BASE64:
      case MPT_HEX_BINARY:
      case MPT_UUID:
      case MPT_IP_ADDR:
      case MPT_MAC_ADDR:
      case MPT_DECIMAL:
#endif
         {
            ret=1;
            break;
         }
      default:
         break;
   }
   return ret;
}
