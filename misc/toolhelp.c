/*
 * Misc Toolhelp functions
 *
 * Copyright 1996 Marcus Meissner
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include "windows.h"
#include "win.h"
#include "winerror.h"
#include "tlhelp32.h"
#include "toolhelp.h"
#include "debug.h"
#include "heap.h"
#include "process.h"
#include "k32obj.h"

/*
 * Support for toolhelp's snapshots.  They
 * are supposed to be Kernel32 Objects.
 * Only the Destroy() method is implemented
 */

static void SNAPSHOT_Destroy( K32OBJ *obj );

const K32OBJ_OPS SNAPSHOT_Ops =
{
  NULL,    		/* signaled */
  NULL,   		/* satisfied */
  NULL,     		/* add_wait */
  NULL,  		/* remove_wait */
  NULL,			/* read */
  NULL,			/* write */
  SNAPSHOT_Destroy	/* destroy */
};

/* The K32 snapshot object object */
/* Process snapshot kernel32 object */
typedef struct _Process32Snapshot
{
  K32OBJ		header;

  DWORD			numProcs;
  DWORD			arrayCounter;
  /*
   * Store a reference to the PDB list.  
   * Insuure in the alloc and dealloc routines for this structure that
   * I increment and decrement the pdb->head.refcount, so that the
   * original pdb will stay around for as long as I use it, but it's
   * not locked forver into memory.
   */
  PDB32		**processArray;
}
SNAPSHOT_OBJECT;

/* FIXME: to make this working, we have to callback all these registered 
 * functions from all over the WINE code. Someone with more knowledge than
 * me please do that. -Marcus
 */
static struct notify
{
    HTASK16   htask;
    FARPROC16 lpfnCallback;
    WORD     wFlags;
} *notifys = NULL;

static int nrofnotifys = 0;

static FARPROC16 HookNotify = NULL;

BOOL16 WINAPI NotifyRegister( HTASK16 htask, FARPROC16 lpfnCallback,
                              WORD wFlags )
{
    int	i;

    TRACE(toolhelp, "(%x,%lx,%x) called.\n",
                      htask, (DWORD)lpfnCallback, wFlags );
    if (!htask) htask = GetCurrentTask();
    for (i=0;i<nrofnotifys;i++)
        if (notifys[i].htask==htask)
            break;
    if (i==nrofnotifys) {
        if (notifys==NULL)
            notifys=(struct notify*)HeapAlloc( SystemHeap, 0,
                                               sizeof(struct notify) );
        else
            notifys=(struct notify*)HeapReAlloc( SystemHeap, 0, notifys,
                                        sizeof(struct notify)*(nrofnotifys+1));
        if (!notifys) return FALSE;
        nrofnotifys++;
    }
    notifys[i].htask=htask;
    notifys[i].lpfnCallback=lpfnCallback;
    notifys[i].wFlags=wFlags;
    return TRUE;
}

BOOL16 WINAPI NotifyUnregister( HTASK16 htask )
{
    int	i;
    
    TRACE(toolhelp, "(%x) called.\n", htask );
    if (!htask) htask = GetCurrentTask();
    for (i=nrofnotifys;i--;)
        if (notifys[i].htask==htask)
            break;
    if (i==-1)
        return FALSE;
    memcpy(notifys+i,notifys+(i+1),sizeof(struct notify)*(nrofnotifys-i-1));
    notifys=(struct notify*)HeapReAlloc( SystemHeap, 0, notifys,
                                        (nrofnotifys-1)*sizeof(struct notify));
    nrofnotifys--;
    return TRUE;
}

BOOL16 WINAPI StackTraceCSIPFirst(STACKTRACEENTRY *ste, WORD wSS, WORD wCS, WORD wIP, WORD wBP)
{
    return TRUE;
}

BOOL16 WINAPI StackTraceFirst(STACKTRACEENTRY *ste, HTASK16 Task)
{
    return TRUE;
}

BOOL16 WINAPI StackTraceNext(STACKTRACEENTRY *ste)
{
    return TRUE;
}

/***********************************************************************
 *           ToolHelpHook                             (KERNEL.341)
 *	see "Undocumented Windows"
 */
FARPROC16 WINAPI ToolHelpHook(FARPROC16 lpfnNotifyHandler)
{
FARPROC16 tmp;
	tmp = HookNotify;
	HookNotify = lpfnNotifyHandler;
	/* just return previously installed notification function */
	return tmp;
}

/***********************************************************************
 *	     SNAPSHOT_Destroy
 *
 * Deallocate K32 snapshot objects
 */
static void SNAPSHOT_Destroy (K32OBJ *obj)
{
  int	i;
  SNAPSHOT_OBJECT *snapshot = (SNAPSHOT_OBJECT *) obj;
  assert (obj->type == K32OBJ_CHANGE);

  if (snapshot->processArray)
    {	
      for (i = 0; snapshot->processArray[i] && i <snapshot->numProcs; i++)
	{
	  K32OBJ_DecCount (&snapshot->processArray[i]->header);
	}
      HeapFree (GetProcessHeap (), 0, snapshot->processArray);
      snapshot->processArray = NULL;      
    }	

  obj->type = K32OBJ_UNKNOWN;
  HeapFree (GetProcessHeap (), 0, snapshot);
}

/***********************************************************************
 *           CreateToolHelp32Snapshot			(KERNEL32.179)
 *	see "Undocumented Windows"
 */
HANDLE32 WINAPI CreateToolhelp32Snapshot(DWORD dwFlags, DWORD
					 th32ProcessID) 
{
  HANDLE32		ssHandle;
  SNAPSHOT_OBJECT	*snapshot;
  int			numProcesses;
  int			i;
  PDB32*		pdb;

  TRACE(toolhelp, "%lx & TH32CS_INHERIT (%x) = %lx %s\n", dwFlags, 
	  TH32CS_INHERIT,
	  dwFlags & TH32CS_INHERIT, 
	  dwFlags & TH32CS_INHERIT ? "TRUE" : "FALSE");  
  TRACE(toolhelp, "%lx & TH32CS_SNAPHEAPLIST (%x) = %lx %s\n", dwFlags, 
	  TH32CS_SNAPHEAPLIST,
	  dwFlags & TH32CS_SNAPHEAPLIST, 
	  dwFlags & TH32CS_SNAPHEAPLIST ? "TRUE" : "FALSE");  
  TRACE(toolhelp, "%lx & TH32CS_SNAPMODULE (%x) = %lx %s\n", dwFlags, 
	  TH32CS_SNAPMODULE,
	  dwFlags & TH32CS_SNAPMODULE, 
	  dwFlags & TH32CS_SNAPMODULE ? "TRUE" : "FALSE");  
  TRACE(toolhelp, "%lx & TH32CS_SNAPPROCESS (%x) = %lx %s\n", dwFlags, 
	  TH32CS_SNAPPROCESS,
	  dwFlags & TH32CS_SNAPPROCESS, 
	  dwFlags & TH32CS_SNAPPROCESS ? "TRUE" : "FALSE");  
  TRACE(toolhelp, "%lx & TH32CS_SNAPTHREAD (%x) = %lx %s\n", dwFlags, 
	  TH32CS_SNAPTHREAD,
	  dwFlags & TH32CS_SNAPTHREAD,
	  dwFlags & TH32CS_SNAPTHREAD ? "TRUE" : "FALSE");  

  /**** FIXME: Not implmented ***/
  if (dwFlags & TH32CS_INHERIT)
    {
      FIXME(toolhelp,"(0x%08lx (TH32CS_INHERIT),0x%08lx), stub!\n",
	    dwFlags,th32ProcessID);

      return INVALID_HANDLE_VALUE32;
    }
  if (dwFlags & TH32CS_SNAPHEAPLIST)
    {
      FIXME(toolhelp,"(0x%08lx (TH32CS_SNAPHEAPLIST),0x%08lx), stub!\n",
	    dwFlags,th32ProcessID);
      return INVALID_HANDLE_VALUE32;
    }
  if (dwFlags & TH32CS_SNAPMODULE)
    {
      FIXME(toolhelp,"(0x%08lx (TH32CS_SNAPMODULE),0x%08lx), stub!\n",
	    dwFlags,th32ProcessID);
      return INVALID_HANDLE_VALUE32;
    }

  if (dwFlags & TH32CS_SNAPPROCESS)
    {
      TRACE (toolhelp, "(0x%08lx (TH32CS_SNAPMODULE),0x%08lx)\n",
	    dwFlags,th32ProcessID);
      snapshot = HeapAlloc (GetProcessHeap (), 0, sizeof
			    (SNAPSHOT_OBJECT)); 
      if (!snapshot)
	{
	  return INVALID_HANDLE_VALUE32;
	}

      snapshot->header.type = K32OBJ_TOOLHELP_SNAPSHOT;
      snapshot->header.refcount = 1;
      snapshot->arrayCounter = 0;
  
      /*
       * Lock here, to prevent processes from being created or
       * destroyed while the snapshot is gathered
       */

      SYSTEM_LOCK ();
      numProcesses = PROCESS_PDBList_Getsize ();

      snapshot->processArray = (PDB32**) 
	HeapAlloc (GetProcessHeap (), 0, sizeof (PDB32*) * numProcesses); 

      if (!snapshot->processArray)
        {
          HeapFree (GetProcessHeap (), 0, snapshot->processArray);
	  SetLastError (INVALID_HANDLE_VALUE32);
	  ERR (toolhelp, "Error allocating %d bytes for snapshot\n", 
	       sizeof (PDB32*) * numProcesses); 	       
          return INVALID_HANDLE_VALUE32;
        }

      snapshot->numProcs = numProcesses;

      pdb = PROCESS_PDBList_Getfirst ();
      for (i = 0; pdb && i < numProcesses; i++)
	{
	  TRACE (toolhelp, "Saving ref to pdb %ld\n", PDB_TO_PROCESS_ID(pdb));
	  snapshot->processArray[i] = pdb;
	  K32OBJ_IncCount (&pdb->header);
	  pdb = PROCESS_PDBList_Getnext (pdb);
	}
      SYSTEM_UNLOCK ();

      ssHandle = HANDLE_Alloc (PROCESS_Current (), &snapshot->header,
			       FILE_ALL_ACCESS, TRUE, -1);
      if (ssHandle == INVALID_HANDLE_VALUE32) 
	{
	  /*  HANDLE_Alloc is supposed to deallocate the 
	   *  heap memory if it fails.  This code doesn't need to.
	   */
	  SetLastError (INVALID_HANDLE_VALUE32);
	  ERR (toolhelp, "Error allocating handle\n");
	  return INVALID_HANDLE_VALUE32;
	}
      
      TRACE (toolhelp, "snapshotted %d processes, expected %d\n",
	    i, numProcesses);
      return ssHandle;
    }

  if (dwFlags & TH32CS_SNAPTHREAD)
    {
      FIXME(toolhelp,"(0x%08lx (TH32CS_SNAPMODULE),0x%08lx), stub!\n",
	    dwFlags,th32ProcessID);
      return INVALID_HANDLE_VALUE32;
    }

  return INVALID_HANDLE_VALUE32;
}

/***********************************************************************
 *		Process32First
 * Return info about the first process in a toolhelp32 snapshot
 */
BOOL32 WINAPI Process32First(HANDLE32 hSnapshot, LPPROCESSENTRY32 lppe)
{
  PDB32           *pdb; 
  SNAPSHOT_OBJECT *snapshot;
  int             i;

  TRACE (toolhelp, "(0x%08lx,0x%08lx)\n", (DWORD) hSnapshot,
	 (DWORD) lppe);

  if (lppe->dwSize < sizeof (PROCESSENTRY32))
    {
      SetLastError (ERROR_INSUFFICIENT_BUFFER);
      ERR (toolhelp, "Result buffer too small\n");
      return FALSE;
    }

  SYSTEM_LOCK ();
  snapshot = (SNAPSHOT_OBJECT*) HANDLE_GetObjPtr (PROCESS_Current (),
						  hSnapshot,
						  K32OBJ_UNKNOWN,
						  FILE_ALL_ACCESS,
						  NULL); 
  if (!snapshot)
    {
      SYSTEM_UNLOCK ();
      SetLastError (ERROR_INVALID_HANDLE);
      ERR (toolhelp, "Error retreiving snapshot\n");
      return FALSE;
    }

  snapshot->arrayCounter = i = 0;
  pdb = snapshot->processArray[i];

  if (!pdb)
    {
      SetLastError (ERROR_NO_MORE_FILES);
      ERR (toolhelp, "End of snapshot array\n");
      return FALSE;
    }

  TRACE (toolhelp, "Returning info on process %d, id %ld\n", 
	 i, PDB_TO_PROCESS_ID (pdb));

  lppe->cntUsage = 1;
  lppe->th32ProcessID = PDB_TO_PROCESS_ID (pdb);
  lppe->th32DefaultHeapID = (DWORD) pdb->heap; 
  lppe->cntThreads = pdb->threads; 
  lppe->th32ParentProcessID = PDB_TO_PROCESS_ID (pdb->parent); 
  lppe->pcPriClassBase = 6; /* FIXME: this is a made-up value */
  lppe->dwFlags = -1;       /* FIXME: RESERVED by Microsoft :-) */
  if (pdb->exe_modref) 
    {
      lppe->th32ModuleID = (DWORD) pdb->exe_modref->module;
      strncpy (lppe->szExeFile, pdb->exe_modref->longname, 
	       sizeof (lppe->szExeFile)); 
    }
  else
    {
      lppe->th32ModuleID = (DWORD) 0;
      strcpy (lppe->szExeFile, "");
    }

  SYSTEM_UNLOCK ();

  return TRUE;
}

/***********************************************************************
 *		Process32Next
 * Return info about the "next" process in a toolhelp32 snapshot
 */
BOOL32 WINAPI Process32Next(HANDLE32 hSnapshot, LPPROCESSENTRY32 lppe)
{
  PDB32           *pdb; 
  SNAPSHOT_OBJECT *snapshot;
  int             i;

  TRACE (toolhelp, "(0x%08lx,0x%08lx)\n", (DWORD) hSnapshot,
	 (DWORD) lppe);

  if (lppe->dwSize < sizeof (PROCESSENTRY32))
    {
      SetLastError (ERROR_INSUFFICIENT_BUFFER);
      ERR (toolhelp, "Result buffer too small\n");
      return FALSE;
    }

  SYSTEM_LOCK ();
  snapshot = (SNAPSHOT_OBJECT*) HANDLE_GetObjPtr (PROCESS_Current (),
						  hSnapshot,
						  K32OBJ_UNKNOWN,
						  FILE_ALL_ACCESS,
						  NULL); 
  if (!snapshot)
    {
      SYSTEM_UNLOCK ();
      SetLastError (ERROR_INVALID_HANDLE);
      ERR (toolhelp, "Error retreiving snapshot\n");
      return FALSE;
    }

  snapshot->arrayCounter ++;
  i = snapshot->arrayCounter;
  pdb = snapshot->processArray[i];

  if (!pdb || snapshot->arrayCounter >= snapshot->numProcs)
    {
      SetLastError (ERROR_NO_MORE_FILES);
      ERR (toolhelp, "End of snapshot array\n");
      return FALSE;
    }

  TRACE (toolhelp, "Returning info on process %d, id %ld\n", 
	 i, PDB_TO_PROCESS_ID (pdb));

  lppe->cntUsage = 1;
  lppe->th32ProcessID = PDB_TO_PROCESS_ID (pdb);
  lppe->th32DefaultHeapID = (DWORD) pdb->heap; 
  lppe->cntThreads = pdb->threads; 
  lppe->th32ParentProcessID = PDB_TO_PROCESS_ID (pdb->parent); 
  lppe->pcPriClassBase = 6; /* FIXME: this is a made-up value */
  lppe->dwFlags = -1;       /* FIXME: RESERVED by Microsoft :-) */
  if (pdb->exe_modref) 
    {
      lppe->th32ModuleID = (DWORD) pdb->exe_modref->module;
      strncpy (lppe->szExeFile, pdb->exe_modref->longname, 
	       sizeof (lppe->szExeFile)); 
    }
  else
    {
      lppe->th32ModuleID = (DWORD) 0;
      strcpy (lppe->szExeFile, "");
    }

  SYSTEM_UNLOCK ();

  return TRUE;
}
