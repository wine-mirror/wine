/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * DDEML library
 *
 * Copyright 1997 Alexandre Julliard
 * Copyright 1997 Len White
 * Copyright 1999 Keith Matthews
 * Copyright 2000 Corel
 * Copyright 2001 Eric Pouech
 */

#include <string.h>
#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "dde.h"
#include "ddeml.h"
#include "debugtools.h"
#include "dde_private.h"

DEFAULT_DEBUG_CHANNEL(ddeml);

WDML_INSTANCE*		WDML_InstanceList = NULL;
DWORD 			WDML_MaxInstanceID = 0;  /* OK for present, may have to worry about wrap-around later */
static const char	DDEInstanceAccess[] = "DDEMaxInstance";
static const char	DDEHandleAccess[] = "DDEHandleAccess";
HANDLE			handle_mutex = 0;
const char		WDML_szEventClass[] = "DdeEventClass";

/* FIXME
 * currently the msg parameter is not used in the packing functions.
 * it should be used to identify messages which don't actually require the packing operation
 * but would do with the simple DWORD for lParam
 */

static BOOL DDE_RequirePacking(UINT msg)
{
    BOOL	ret;

    switch (msg)
    {
    case WM_DDE_ACK:
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
	ret = TRUE;
	break;
    case WM_DDE_EXECUTE:	/* strange, NT 2000 (at least) really uses packing here... */
    case WM_DDE_INITIATE:
    case WM_DDE_REQUEST:	/* assuming clipboard formats are 16 bit */
    case WM_DDE_TERMINATE:
    case WM_DDE_UNADVISE:	/* assuming clipboard formats are 16 bit */
	ret = FALSE;
	break;
    default:
	TRACE("Unknown message %04x\n", msg);
	ret = FALSE;
	break;
    }
    return ret;
}

/*****************************************************************
 *            PackDDElParam (USER32.@)
 *
 * RETURNS
 *   the packed lParam
 */
LPARAM WINAPI PackDDElParam(UINT msg, UINT uiLo, UINT uiHi)
{
     HGLOBAL	hMem;
     UINT*	params;
     
     if (!DDE_RequirePacking(msg))
	 return MAKELONG(uiLo, uiHi);

     if (!(hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, sizeof(UINT) * 2)))
     {
	  ERR("GlobalAlloc failed\n");
	  return 0;
     }
     
     params = GlobalLock(hMem);
     if (params == NULL)
     {
	  ERR("GlobalLock failed\n");
	  return 0;
     }
     
     params[0] = uiLo;
     params[1] = uiHi;
     
     GlobalUnlock(hMem);

     return (LPARAM)hMem;
}


/*****************************************************************
 *            UnpackDDElParam (USER32.@)
 *
 * RETURNS
 *   success: nonzero
 *   failure: zero
 */
BOOL WINAPI UnpackDDElParam(UINT msg, LPARAM lParam,
			    PUINT uiLo, PUINT uiHi)
{
     HGLOBAL hMem;
     UINT *params;

     if (!DDE_RequirePacking(msg))
     {
	 *uiLo = LOWORD(lParam);
	 *uiHi = HIWORD(lParam);

	 return TRUE;
     }
     
     if (lParam == 0)
     {
	  return FALSE;
     }
     
     hMem = (HGLOBAL)lParam;
     
     params = GlobalLock(hMem);
     if (params == NULL)
     {
	  ERR("GlobalLock failed\n");
	  return FALSE;
     }
     
     *uiLo = params[0];
     *uiHi = params[1];

     GlobalUnlock(hMem);
     
     return TRUE;
}


/*****************************************************************
 *            FreeDDElParam (USER32.@)
 *
 * RETURNS
 *   success: nonzero
 *   failure: zero
 */
BOOL WINAPI FreeDDElParam(UINT msg, LPARAM lParam)
{
     HGLOBAL hMem = (HGLOBAL)lParam;

     if (!DDE_RequirePacking(msg))
	 return TRUE;

     if (lParam == 0)
     {
	  return FALSE;
     }
     return GlobalFree(hMem) == (HGLOBAL)NULL;
}


/*****************************************************************
 *            ReuseDDElParam (USER32.@)
 *
 * RETURNS
 *   the packed lParam
 */
LPARAM WINAPI ReuseDDElParam(LPARAM lParam, UINT msgIn, UINT msgOut,
                             UINT uiLo, UINT uiHi)
{
     HGLOBAL	hMem;
     UINT*	params;
     BOOL	in, out;

     in = DDE_RequirePacking(msgIn);
     out = DDE_RequirePacking(msgOut);

     if (!in)
     {
	 return PackDDElParam(msgOut, uiLo, uiHi);
     }

     if (lParam == 0)
     {
	  return FALSE;
     }

     if (!out)
     {
	 FreeDDElParam(msgIn, lParam);
	 return MAKELONG(uiLo, uiHi);
     }

     hMem = (HGLOBAL)lParam;
     
     params = GlobalLock(hMem);
     if (params == NULL)
     {
	  ERR("GlobalLock failed\n");
	  return 0;
     }
     
     params[0] = uiLo;
     params[1] = uiHi;

     TRACE("Reusing pack %08x %08x\n", uiLo, uiHi);

     GlobalLock(hMem);
     return lParam;
}

/*****************************************************************
 *            ImpersonateDdeClientWindow (USER32.@)
 *
 */
BOOL WINAPI ImpersonateDdeClientWindow(
     HWND hWndClient,  /* [in] handle to DDE client window */
     HWND hWndServer   /* [in] handle to DDE server window */
)
{
     FIXME("(%04x %04x): stub\n", hWndClient, hWndServer);
     return FALSE;
}

/*****************************************************************
 *            DdeSetQualityOfService (USER32.@)
 */

BOOL WINAPI DdeSetQualityOfService(HWND hwndClient, CONST SECURITY_QUALITY_OF_SERVICE *pqosNew,
				   PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
     FIXME("(%04x %p %p): stub\n", hwndClient, pqosNew, pqosPrev);
     return TRUE;
}


/******************************************************************************
 *		IncrementInstanceId
 *
 *	generic routine to increment the max instance Id and allocate a new application instance
 */
static DWORD WDML_IncrementInstanceId(WDML_INSTANCE* thisInstance)
{
    DWORD	id = InterlockedIncrement(&WDML_MaxInstanceID);

    thisInstance->instanceID = id;
    TRACE("New instance id %ld allocated\n", id);
    return DMLERR_NO_ERROR;
}

/******************************************************************************
 *            DdeInitializeA   (USER32.@)
 */
UINT WINAPI DdeInitializeA(LPDWORD pidInst, PFNCALLBACK pfnCallback,
			   DWORD afCmd, DWORD ulRes)
{
    UINT	ret = DdeInitializeW(pidInst, pfnCallback, afCmd, ulRes);
    
    if (ret == DMLERR_NO_ERROR) {
	WDML_INSTANCE*	thisInstance = WDML_FindInstance(*pidInst);
	if (thisInstance)
	    thisInstance->unicode = FALSE;
    }
    return ret;
}

static LRESULT CALLBACK WDML_EventProc(HWND hwndEvent, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WDML_INSTANCE*	thisInstance;
    HDDEDATA		hDdeData;

    switch (uMsg)
    {
    case WM_WDML_REGISTER:
	thisInstance = (WDML_INSTANCE*)GetWindowLongA(hwndEvent, 0);
	
        /* try calling the Callback */
	if (thisInstance->callback != NULL /* && thisInstance->Process_id == GetCurrentProcessId()*/)
	{
	    TRACE("Calling the callback, type=XTYP_REGISTER, CB=0x%lx\n",
		  (DWORD)thisInstance->callback);
		
	    hDdeData = (thisInstance->callback)(XTYP_REGISTER, 0, 0,
						(HSZ)wParam, (HSZ)lParam, 0, 0, 0);
		
	    TRACE("Callback function called - result=%d\n", (INT)hDdeData);
	}
	break;

    case WM_WDML_UNREGISTER:
	thisInstance = (WDML_INSTANCE*)GetWindowLongA(hwndEvent, 0);
	
	if (thisInstance && thisInstance->callback != NULL)
	{
	    if (thisInstance->CBFflags & CBF_SKIP_DISCONNECTS)
	    {
		FIXME("skip callback XTYP_UNREGISTER\n");
	    }
	    else
	    {
		TRACE("calling callback XTYP_UNREGISTER, idInst=%ld\n", 
		      thisInstance->instanceID);
		(thisInstance->callback)(XTYP_UNREGISTER, 0, 0,
					 (HSZ)wParam, (HSZ)lParam, 0, 0, 0);
	    }
	}
    }
    return DefWindowProcA(hwndEvent, uMsg, wParam, lParam);
}

/******************************************************************************
 * DdeInitializeW [USER32.@]
 * Registers an application with the DDEML
 *
 * PARAMS
 *    pidInst     [I] Pointer to instance identifier
 *    pfnCallback [I] Pointer to callback function
 *    afCmd       [I] Set of command and filter flags
 *    ulRes       [I] Reserved
 *
 * RETURNS
 *    Success: DMLERR_NO_ERROR
 *    Failure: DMLERR_DLL_USAGE, DMLERR_INVALIDPARAMETER, DMLERR_SYS_ERROR
 */
UINT WINAPI DdeInitializeW(LPDWORD pidInst, PFNCALLBACK pfnCallback,
			   DWORD afCmd, DWORD ulRes)
{
    
/*  probably not really capable of handling multiple processes, but should handle
 *	multiple instances within one process */
    
    SECURITY_ATTRIBUTES		s_attrib;
    DWORD 	       		err_no = 0;
    WDML_INSTANCE*		thisInstance;
    WDML_INSTANCE*		reference_inst;
    UINT			ret;
    WNDCLASSEXA			wndclass;

    if (ulRes)
    {
	ERR("Reserved value not zero?  What does this mean?\n");
	FIXME("(%p,%p,0x%lx,%ld): stub\n", pidInst, pfnCallback,
	      afCmd,ulRes);
	/* trap this and no more until we know more */
	return DMLERR_NO_ERROR;
    }
    if (!pfnCallback) 
    {
	/*  this one may be wrong - MS dll seems to accept the condition, 
	    leave this until we find out more !! */
	
	
	/* can't set up the instance with nothing to act as a callback */
	TRACE("No callback provided\n");
	return DMLERR_INVALIDPARAMETER; /* might be DMLERR_DLL_USAGE */
    }
    
    /* grab enough heap for one control struct - not really necessary for re-initialise
     *	but allows us to use same validation routines */
    thisInstance = (WDML_INSTANCE*)HeapAlloc(GetProcessHeap(), 0, sizeof(WDML_INSTANCE));
    if (thisInstance == NULL)
    {
	/* catastrophe !! warn user & abort */
	ERR("Instance create failed - out of memory\n");
	return DMLERR_SYS_ERROR;
    }
    thisInstance->next = NULL;
    thisInstance->monitor = (afCmd | APPCLASS_MONITOR);
    
    /* messy bit, spec implies that 'Client Only' can be set in 2 different ways, catch 1 here */
    
    thisInstance->clientOnly = afCmd & APPCMD_CLIENTONLY;
    thisInstance->instanceID = *pidInst; /* May need to add calling proc Id */
    thisInstance->callback = *pfnCallback;
    thisInstance->txnCount = 0;
    thisInstance->unicode = TRUE;
    thisInstance->win16 = FALSE;
    thisInstance->nodeList = NULL; /* node will be added later */
    thisInstance->monitorFlags = afCmd & MF_MASK;
    thisInstance->servers = NULL;
    thisInstance->convs[0] = NULL;
    thisInstance->convs[1] = NULL;
    thisInstance->links[0] = NULL;
    thisInstance->links[1] = NULL;

    wndclass.cbSize        = sizeof(wndclass);
    wndclass.style         = 0;
    wndclass.lpfnWndProc   = WDML_EventProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = sizeof(DWORD);
    wndclass.hInstance     = 0;
    wndclass.hIcon         = 0;
    wndclass.hCursor       = 0;
    wndclass.hbrBackground = 0;
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = WDML_szEventClass;
    wndclass.hIconSm       = 0;
	
    RegisterClassExA(&wndclass);
	
    thisInstance->hwndEvent = CreateWindowA(WDML_szEventClass, NULL,
					    WS_POPUP, 0, 0, 0, 0,
					    0, 0, 0, 0);
	
    SetWindowLongA(thisInstance->hwndEvent, 0, (DWORD)thisInstance);

    /* isolate CBF flags in one go, expect this will go the way of all attempts to be clever !! */
    
    thisInstance->CBFflags = afCmd^((afCmd&MF_MASK)|((afCmd&APPCMD_MASK)|(afCmd&APPCLASS_MASK)));
    
    if (!thisInstance->clientOnly)
    {
	
	/* Check for other way of setting Client-only !! */
	
	thisInstance->clientOnly = 
	    (thisInstance->CBFflags & CBF_FAIL_ALLSVRXACTIONS) == CBF_FAIL_ALLSVRXACTIONS;
    }
    
    TRACE("instance created - checking validity \n");
    
    if (*pidInst == 0) 
    {
	/*  Initialisation of new Instance Identifier */
	TRACE("new instance, callback %p flags %lX\n",pfnCallback,afCmd);
	if (WDML_MaxInstanceID == 0)
	{
	    /*  Nothing has been initialised - exit now ! can return TRUE since effect is the same */
	    /*  Need to set up Mutex in case it is not already present */
	    s_attrib.bInheritHandle = TRUE;
	    s_attrib.lpSecurityDescriptor = NULL;
	    s_attrib.nLength = sizeof(s_attrib);
	    handle_mutex = CreateMutexA(&s_attrib,0,DDEHandleAccess);
	    if (!handle_mutex) 
	    {
		ERR("CreateMutex failed - handle list  %li\n",GetLastError());
		HeapFree(GetProcessHeap(), 0, thisInstance);
		return DMLERR_SYS_ERROR;
	    }
	}
	if (!WDML_WaitForMutex(handle_mutex))
	{
	    return DMLERR_SYS_ERROR;
	}
	
	if (WDML_InstanceList == NULL) 
	{
	    /* can't be another instance in this case, assign to the base pointer */
	    WDML_InstanceList = thisInstance;
	    
	    /* since first must force filter of XTYP_CONNECT and XTYP_WILDCONNECT for
	     *		present 
	     *	-------------------------------      NOTE NOTE NOTE    --------------------------
	     *		
	     *	the manual is not clear if this condition
	     *	applies to the first call to DdeInitialize from an application, or the 
	     *	first call for a given callback !!!
	     */
	    
	    thisInstance->CBFflags = thisInstance->CBFflags|APPCMD_FILTERINITS;
	    TRACE("First application instance detected OK\n");
	    /*	allocate new instance ID */
	    if ((err_no = WDML_IncrementInstanceId(thisInstance))) return err_no;
	    
	} 
	else 
	{
	    /* really need to chain the new one in to the latest here, but after checking conditions
	     *	such as trying to start a conversation from an application trying to monitor */
	    reference_inst = WDML_InstanceList;
	    TRACE("Subsequent application instance - starting checks\n");
	    while (reference_inst->next != NULL) 
	    {
		/*
		 *	This set of tests will work if application uses same instance Id
		 *	at application level once allocated - which is what manual implies
		 *	should happen. If someone tries to be 
		 *	clever (lazy ?) it will fail to pick up that later calls are for
		 *	the same application - should we trust them ?
		 */
		if (thisInstance->instanceID == reference_inst->instanceID) 
		{
				/* Check 1 - must be same Client-only state */
		    
		    if (thisInstance->clientOnly != reference_inst->clientOnly)
		    {
			ret = DMLERR_DLL_USAGE;
			goto theError;
		    }
		    
				/* Check 2 - cannot use 'Monitor' with any non-monitor modes */
		    
		    if (thisInstance->monitor != reference_inst->monitor) 
		    {
			ret = DMLERR_INVALIDPARAMETER;
			goto theError;
		    }
		    
				/* Check 3 - must supply different callback address */
		    
		    if (thisInstance->callback == reference_inst->callback)
		    {
			ret = DMLERR_DLL_USAGE;
			goto theError;
		    }
		}
		reference_inst = reference_inst->next;
	    }
	    /*  All cleared, add to chain */
	    
	    TRACE("Application Instance checks finished\n");
	    if ((err_no = WDML_IncrementInstanceId(thisInstance))) return err_no;
	    reference_inst->next = thisInstance;
	}
	if (WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE)) return DMLERR_SYS_ERROR;
	*pidInst = thisInstance->instanceID;
	TRACE("New application instance processing finished OK\n");
    } 
    else 
    {
	/* Reinitialisation situation   --- FIX  */
	TRACE("reinitialisation of (%p,%p,0x%lx,%ld): stub\n",pidInst,pfnCallback,afCmd,ulRes);
	
	if (!WDML_WaitForMutex(handle_mutex))
	{
	    HeapFree(GetProcessHeap(), 0, thisInstance);
	    return DMLERR_SYS_ERROR;
	}
	
	if (WDML_InstanceList == NULL) 
	{
	    ret = DMLERR_DLL_USAGE;
	    goto theError;
	}
	HeapFree(GetProcessHeap(), 0, thisInstance); /* finished - release heap space used as work store */
	/* can't reinitialise if we have initialised nothing !! */
	reference_inst = WDML_InstanceList;
	/* must first check if we have been given a valid instance to re-initialise !!  how do we do that ? */
	/*
	 *	MS allows initialisation without specifying a callback, should we allow addition of the
	 *	callback by a later call to initialise ? - if so this lot will have to change
	 */
	while (reference_inst->next != NULL)
	{
	    if (*pidInst == reference_inst->instanceID && pfnCallback == reference_inst->callback)
	    {
		/* Check 1 - cannot change client-only mode if set via APPCMD_CLIENTONLY */
		
		if (reference_inst->clientOnly)
		{
		    if  ((reference_inst->CBFflags & CBF_FAIL_ALLSVRXACTIONS) != CBF_FAIL_ALLSVRXACTIONS) 
		    {
				/* i.e. Was set to Client-only and through APPCMD_CLIENTONLY */
			
			if (!(afCmd & APPCMD_CLIENTONLY))
			{
			    ret = DMLERR_DLL_USAGE;
			    goto theError;
			}
		    }
		}
		/* Check 2 - cannot change monitor modes */
		
		if (thisInstance->monitor != reference_inst->monitor) 
		{
		    ret = DMLERR_DLL_USAGE;
		    goto theError;
		}
		
		/* Check 3 - trying to set Client-only via APPCMD when not set so previously */
		
		if ((afCmd&APPCMD_CLIENTONLY) && !reference_inst->clientOnly)
		{
		    ret = DMLERR_DLL_USAGE;
		    goto theError;
		}
		break;
	    }
	    reference_inst = reference_inst->next;
	}
	if (reference_inst->next == NULL)
	{
	    /* Crazy situation - trying to re-initialize something that has not beeen initialized !! 
	     *	
	     *	Manual does not say what we do, cannot return DMLERR_NOT_INITIALIZED so what ?
	     */
	    ret = DMLERR_INVALIDPARAMETER;
	    goto theError;
	}
	/*		All checked - change relevant flags */
	
	reference_inst->CBFflags = thisInstance->CBFflags;
	reference_inst->clientOnly = thisInstance->clientOnly;
	reference_inst->monitorFlags = thisInstance->monitorFlags;
	if (WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE))
	{
	    HeapFree(GetProcessHeap(), 0, thisInstance);
	    return DMLERR_SYS_ERROR;
	}
    }
    
    return DMLERR_NO_ERROR;
 theError:
    HeapFree(GetProcessHeap(), 0, thisInstance);
    if (WDML_ReleaseMutex(handle_mutex, "handle_mutex", 0))
	return DMLERR_SYS_ERROR;
    return ret;
}

/*****************************************************************
 * DdeUninitialize [USER32.@]  Frees DDEML resources
 *
 * PARAMS
 *    idInst [I] Instance identifier
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */

BOOL WINAPI DdeUninitialize(DWORD idInst)
{
    /*  Stage one - check if we have a handle for this instance
     */
    WDML_INSTANCE*		thisInstance;
    WDML_INSTANCE*		reference_inst;

    if (WDML_MaxInstanceID == 0)
    {
	/*  Nothing has been initialised - exit now ! can return TRUE since effect is the same */
	return TRUE;
    }
    
    if (!WDML_WaitForMutex(handle_mutex))
    {
	return DMLERR_SYS_ERROR;
    }
    /*  First check instance 
     */
    thisInstance = WDML_FindInstance(idInst);
    if (thisInstance == NULL)
    {
	if (WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE)) return FALSE;
	/*
	 *	Needs something here to record NOT_INITIALIZED ready for DdeGetLastError
	 */
	return FALSE;
    }
    FIXME("(%ld): partial stub\n", idInst);
    
    /*   FIXME	++++++++++++++++++++++++++++++++++++++++++
     *	Needs to de-register all service names
     *	
     */
    
    /* Free the nodes that were not freed by this instance
     * and remove the nodes from the list of HSZ nodes.
     */
    WDML_FreeAllHSZ(thisInstance);

    DestroyWindow(thisInstance->hwndEvent);
    
    /* OK now delete the instance handle itself */
    
    if (WDML_InstanceList == thisInstance)
    {
	/* special case - the first/only entry
	 */
	WDML_InstanceList = thisInstance->next;
    }
    else
    {
	/* general case
	 */
	reference_inst = WDML_InstanceList;
	while (reference_inst->next != thisInstance)
	{
	    reference_inst = thisInstance->next;
	}
	reference_inst->next = thisInstance->next;
    }
    /* release the mutex and the heap entry
     */
    HeapFree(GetProcessHeap(), 0, thisInstance);
    if (WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE)) 
    {
	/* should record something here, but nothing left to hang it from !!
	 */
	return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 *            RemoveHSZNodes    (INTERNAL)
 *
 * Remove a node from the list of HSZ nodes.
 */
static void WDML_RemoveHSZNode(WDML_INSTANCE* thisInstance, HSZ hsz)
{
    HSZNode* pPrev = NULL;
    HSZNode* pCurrent = NULL;
    
    /* Set the current node at the start of the list.
     */
    pCurrent = thisInstance->nodeList;
    /* While we have more nodes.
     */
    while (pCurrent != NULL)
    {
	/* If we found the node we were looking for.
	 */
	if (pCurrent->hsz == hsz)
	{
	    /* Remove the node.
	     */
	    /* If the first node in the list is to to be removed.
	     * Set the global list pointer to the next node.
	     */
	    if (pCurrent == thisInstance->nodeList)
	    {
		thisInstance->nodeList = pCurrent->next;
	    }
	    /* Just fix the pointers has to skip the current
	     * node so we can delete it.
	     */
	    else
	    {
		pPrev->next = pCurrent->next;
	    }
	    /* Destroy this node.
	     */
	    HeapFree(GetProcessHeap(), 0, pCurrent);
	    break;
	}
	/* Save the previous node pointer.
	 */
	pPrev = pCurrent;
	/* Move on to the next node.
	 */
	pCurrent = pCurrent->next;
    }
}

/******************************************************************************
 *            FreeAndRemoveHSZNodes    (INTERNAL)
 *
 * Frees up all the strings still allocated in the list and
 * remove all the nodes from the list of HSZ nodes.
 */
void WDML_FreeAllHSZ(WDML_INSTANCE* thisInstance)
{
    /* Free any strings created in this instance.
     */
    while (thisInstance->nodeList != NULL)
    {
	DdeFreeStringHandle(thisInstance->instanceID, thisInstance->nodeList->hsz);
    }
}

/******************************************************************************
 *            GetSecondaryHSZValue    (INTERNAL)
 *
 * Insert a node to the head of the list.
 */
static HSZ WDML_GetSecondaryHSZValue(WDML_INSTANCE* thisInstance, HSZ hsz)
{
    HSZ hsz2 = 0;
    
    if (hsz != 0)
    {
	/* Create and set the Secondary handle */
	if (thisInstance->unicode)
	{
	    WCHAR wSecondaryString[MAX_BUFFER_LEN];
	    WCHAR wUniqueNum[MAX_BUFFER_LEN];
    
	    if (DdeQueryStringW(thisInstance->instanceID, hsz,
				wSecondaryString,
				MAX_BUFFER_LEN, CP_WINUNICODE))
	    {
		wsprintfW(wUniqueNum,"(%ld)",
			  (DWORD)thisInstance->instanceID);
		lstrcatW(wSecondaryString, wUniqueNum);
		
		hsz2 = GlobalAddAtomW(wSecondaryString);
	    }
	}
	else
	{
	    CHAR SecondaryString[MAX_BUFFER_LEN];
	    CHAR UniqueNum[MAX_BUFFER_LEN];
    
	    if (DdeQueryStringA(thisInstance->instanceID, hsz,
				SecondaryString,
				MAX_BUFFER_LEN, CP_WINANSI))
	    {
		wsprintfA(UniqueNum,"(%ld)", thisInstance->instanceID);
		lstrcatA(SecondaryString, UniqueNum);
		
		hsz2 = GlobalAddAtomA(SecondaryString);
	    }
	}
    }
    return hsz2;
}

/******************************************************************************
 *            InsertHSZNode    (INTERNAL)
 *
 * Insert a node to the head of the list.
 */
static void WDML_InsertHSZNode(WDML_INSTANCE* thisInstance, HSZ hsz)
{
    if (hsz != 0)
    {
	HSZNode* pNew = NULL;
	/* Create a new node for this HSZ.
	 */
	pNew = (HSZNode*)HeapAlloc(GetProcessHeap(), 0, sizeof(HSZNode));
	if (pNew != NULL)
	{
	    /* Set the handle value.
	     */
	    pNew->hsz = hsz;
	    
	    /* Create and set the Secondary handle */
	    pNew->hsz2 = WDML_GetSecondaryHSZValue(thisInstance, hsz);
	    
	    /* Attach the node to the head of the list. i.e most recently added is first
	     */
	    pNew->next = thisInstance->nodeList;
	    
	    /* The new node is now at the head of the list
	     * so set the global list pointer to it.
	     */
	    thisInstance->nodeList = pNew;
	}
	else
	{
	    ERR("Primary HSZ Node allocation failed - out of memory\n");
	}
    }
}

/*****************************************************************************
 *	Find_Instance_Entry
 *
 *	generic routine to return a pointer to the relevant DDE_HANDLE_ENTRY
 *	for an instance Id, or NULL if the entry does not exist
 *
 *	ASSUMES the mutex protecting the handle entry list is reserved before calling
 */
WDML_INSTANCE*	WDML_FindInstance(DWORD InstId)
{
    WDML_INSTANCE*	thisInstance;
    
    thisInstance = WDML_InstanceList;
    while (thisInstance != NULL)
    {
	if (thisInstance->instanceID == InstId)
	{
	    return thisInstance;
	}
	thisInstance = thisInstance->next;
    }
    TRACE("Instance entry missing\n");
    return NULL;
}

/******************************************************************************
 *	WDML_ReleaseMutex
 *
 *	generic routine to release a reserved mutex
 */
DWORD WDML_ReleaseMutex(HANDLE mutex, LPSTR mutex_name, BOOL release_handle_m)
{
    if (!ReleaseMutex(mutex))
    {
	ERR("ReleaseMutex failed - %s mutex %li\n", mutex_name, GetLastError());
	if (release_handle_m)
	{
	    ReleaseMutex(handle_mutex);
	}
	return DMLERR_SYS_ERROR;
    }
    return DMLERR_NO_ERROR;
}

/******************************************************************************
 *	WDML_WaitForMutex
 *
 *	generic routine to wait for the mutex
 */
BOOL WDML_WaitForMutex(HANDLE mutex)
{
    DWORD result;
    
    result = WaitForSingleObject(mutex, INFINITE);
    
    /* both errors should never occur */
    if (WAIT_TIMEOUT == result)
    {
	ERR("WaitForSingleObject timed out\n");
	return FALSE;
    }
    
    if (WAIT_FAILED == result)
    {
	ERR("WaitForSingleObject failed - error %li\n", GetLastError());
	return FALSE;
    }
   /* TRACE("Handle Mutex created/reserved\n"); */
    
    return TRUE;
}

/******************************************************************************
 *              WDML_ReserveAtom
 *
 *      Routine to make an extra Add on an atom to reserve it a bit longer
 */

void WDML_ReserveAtom(WDML_INSTANCE* thisInstance, HSZ hsz)
{
    if (thisInstance->unicode)
    {
	WCHAR SNameBuffer[MAX_BUFFER_LEN];
	GlobalGetAtomNameW(hsz, SNameBuffer, MAX_BUFFER_LEN);
	GlobalAddAtomW(SNameBuffer);
    } else {
	CHAR SNameBuffer[MAX_BUFFER_LEN];
	GlobalGetAtomNameA(hsz, SNameBuffer, MAX_BUFFER_LEN);
	GlobalAddAtomA(SNameBuffer);
    }
}


/******************************************************************************
 *              WDML_ReleaseAtom
 *
 *      Routine to make a delete on an atom to release it a bit sooner
 */

void WDML_ReleaseAtom(WDML_INSTANCE* thisInstance, HSZ hsz)
{
    GlobalDeleteAtom(hsz);
}


/*****************************************************************
 * DdeQueryStringA [USER32.@]
 */
DWORD WINAPI DdeQueryStringA(DWORD idInst, HSZ hsz, LPSTR psz, DWORD cchMax, INT iCodePage)
{
    DWORD		ret = 0;
    CHAR		pString[MAX_BUFFER_LEN];
    WDML_INSTANCE*	thisInstance;
    
    TRACE("(%ld, 0x%x, %p, %ld, %d): partial stub\n",
	  idInst, hsz, psz, cchMax, iCodePage);
    
    if (WDML_MaxInstanceID == 0)
    {
	/*  Nothing has been initialised - exit now ! */
	/*  needs something for DdeGetLAstError even if the manual doesn't say so */
	return FALSE;
    }
    
    if (!WDML_WaitForMutex(handle_mutex))
    {
	return FALSE;
    }
    
    /*  First check instance 
     */
    thisInstance = WDML_FindInstance(idInst);
    if (thisInstance == NULL)
    {
	if (WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE)) return FALSE;
	/*
	  Needs something here to record NOT_INITIALIZED ready for DdeGetLastError
	*/
	return FALSE;
    }
    
    if (iCodePage == CP_WINANSI)
    {
	/* If psz is null, we have to return only the length
	 * of the string.
	 */
	if (psz == NULL)
	{
	    psz = pString;
	    cchMax = MAX_BUFFER_LEN;
	}
	
	ret = GlobalGetAtomNameA(hsz, (LPSTR)psz, cchMax);
    }
    
    WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
    
    TRACE("returning pointer\n"); 
    return ret;
}

/*****************************************************************
 * DdeQueryStringW [USER32.@]
 */

DWORD WINAPI DdeQueryStringW(DWORD idInst, HSZ hsz, LPWSTR psz, DWORD cchMax, INT iCodePage)
{
    DWORD	ret = 0;
    WCHAR	pString[MAX_BUFFER_LEN];
    int		factor = 1;
    
    TRACE("(%ld, 0x%x, %p, %ld, %d): partial-stub\n",
	  idInst, hsz, psz, cchMax, iCodePage);
    
    if (iCodePage == CP_WINUNICODE)
    {
	/* If psz is null, we have to return only the length
	 * of the string.
	 */
	if (psz == NULL)
	{
	    psz = pString;
	    cchMax = MAX_BUFFER_LEN;
	    /* Note: According to documentation if the psz parameter
	     * was NULL this API must return the length of the string in bytes.
	     */
	    factor = (int)sizeof(WCHAR)/sizeof(BYTE);
	}
	ret = GlobalGetAtomNameW(hsz, (LPWSTR)psz, cchMax) * factor;
    }
    return ret;
}

/*****************************************************************
 * DdeCreateStringHandleA [USER32.@]
 *
 * RETURNS
 *    Success: String handle
 *    Failure: 0
 */
HSZ WINAPI DdeCreateStringHandleA(DWORD idInst, LPCSTR psz, INT codepage)
{
    HSZ			hsz = 0;
    WDML_INSTANCE*	thisInstance;
    
    TRACE("(%ld,%s,%d): partial stub\n",idInst,debugstr_a(psz),codepage);
    
    if (WDML_MaxInstanceID == 0)
    {
	/*  Nothing has been initialised - exit now ! can return FALSE since effect is the same */
	return FALSE;
    }
    
    if (!WDML_WaitForMutex(handle_mutex))
    {
	return DMLERR_SYS_ERROR;
    }
    
    
    /*  First check instance 
     */
    thisInstance = WDML_FindInstance(idInst);
    if (thisInstance == NULL)
    {
	if (WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE)) return 0;
	/*
	  Needs something here to record NOT_INITIALIZED ready for DdeGetLastError
	*/
	return 0;
    }
    
    if (codepage == CP_WINANSI)
    {
	hsz = GlobalAddAtomA(psz);
	/* Save the handle so we know to clean it when
	 * uninitialize is called.
	 */
	TRACE("added atom %s with HSZ 0x%x, \n",debugstr_a(psz),hsz);
	WDML_InsertHSZNode(thisInstance, hsz);
	if (WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE)) 
	{
	    thisInstance->lastError = DMLERR_SYS_ERROR;
	    return 0;
	}
	TRACE("Returning pointer\n");
	return hsz;
    } 
    else 
    {
	WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
    }
    TRACE("Returning error\n");
    return 0;  
}


/******************************************************************************
 * DdeCreateStringHandleW [USER32.@]  Creates handle to identify string
 *
 * RETURNS
 *    Success: String handle
 *    Failure: 0
 */
HSZ WINAPI DdeCreateStringHandleW(
    DWORD idInst,   /* [in] Instance identifier */
    LPCWSTR psz,    /* [in] Pointer to string */
    INT codepage)   /* [in] Code page identifier */
{
    WDML_INSTANCE*	thisInstance;
    HSZ			hsz = 0;
    
    TRACE("(%ld,%s,%d): partial stub\n",idInst,debugstr_w(psz),codepage);
    
    
    if (WDML_MaxInstanceID == 0)
    {
	/*  Nothing has been initialised - exit now ! can return FALSE since effect is the same */
	return FALSE;
    }
    
    if (!WDML_WaitForMutex(handle_mutex))
    {
	return DMLERR_SYS_ERROR;
    }
    
    /*  First check instance 
     */
    thisInstance = WDML_FindInstance(idInst);
    if (thisInstance == NULL)
    {
	if (WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE)) return 0;
	/*
	  Needs something here to record NOT_INITIALIZED ready for DdeGetLastError
	*/
	return 0;
    }
    
    FIXME("(%ld,%s,%d): partial stub\n",idInst,debugstr_w(psz),codepage);
    
    if (codepage == CP_WINUNICODE)
    {
	/*
	 *  Should we be checking this against the unicode/ascii nature of the call to DdeInitialize ?
	 */
	hsz = GlobalAddAtomW(psz);
	/* Save the handle so we know to clean it when
	 * uninitialize is called.
	 */
	WDML_InsertHSZNode(thisInstance, hsz);
	if (WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE)) 
	{
	    thisInstance->lastError = DMLERR_SYS_ERROR;
	    return 0;
	}
	TRACE("Returning pointer\n");
	return hsz;
    } 
    else 
    {
	WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
    }
    TRACE("Returning error\n");
    return 0;
}

/*****************************************************************
 *            DdeFreeStringHandle   (USER32.@)
 * RETURNS: success: nonzero
 *          fail:    zero
 */
BOOL WINAPI DdeFreeStringHandle(DWORD idInst, HSZ hsz)
{
    WDML_INSTANCE*	thisInstance;
    HSZ			hsz2;
    
    TRACE("(%ld,%d): \n",idInst,hsz);
    
    if (WDML_MaxInstanceID == 0)
    {
	/*  Nothing has been initialised - exit now ! can return TRUE since effect is the same */
	return TRUE;
    }
    
    if (!WDML_WaitForMutex(handle_mutex))
    {
	return DMLERR_SYS_ERROR;
    }
    
    /*  First check instance 
     */
    thisInstance = WDML_FindInstance(idInst);
    if ((thisInstance == NULL) || (thisInstance->nodeList == NULL))
    {
	if (WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE)) return TRUE;
	/*  Nothing has been initialised - exit now ! can return TRUE since effect is the same */
	return TRUE;
    }
    
    /* Remove the node associated with this HSZ.
     */
    hsz2 = thisInstance->nodeList->hsz2;	/* save this value first */
    
    WDML_RemoveHSZNode(thisInstance, hsz);
    /* Free the string associated with this HSZ.
     */
    WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
    if (hsz2 != 0)
    {
	GlobalDeleteAtom(hsz2);
    }
    return GlobalDeleteAtom(hsz) ? 0 : hsz;
}

/*****************************************************************
 *            DdeKeepStringHandle  (USER32.@)
 *
 * RETURNS: success: nonzero
 *          fail:    zero
 */
BOOL WINAPI DdeKeepStringHandle(DWORD idInst, HSZ hsz)
{
    
    WDML_INSTANCE*	thisInstance;
    
    TRACE("(%ld,%d): \n",idInst,hsz);
    
    if (WDML_MaxInstanceID == 0)
    {
	/*  Nothing has been initialised - exit now ! can return FALSE since effect is the same */
	return FALSE;
    }
    
    
    if (!WDML_WaitForMutex(handle_mutex))
    {
	return FALSE;
    }
    
    /*  First check instance
     */
    thisInstance = WDML_FindInstance(idInst);
    if ((thisInstance == NULL) || (thisInstance->nodeList == NULL))
    {
	if (WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE)) return FALSE;
	/*  Nothing has been initialised - exit now ! can return FALSE since effect is the same */
	return FALSE;
	return FALSE;
    }
    WDML_ReserveAtom(thisInstance, hsz);
    WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
    return TRUE;
}

/*****************************************************************
 *            DdeCreateDataHandle (USER32.@)
 */
HDDEDATA WINAPI DdeCreateDataHandle(DWORD idInst, LPBYTE pSrc, DWORD cb, 
				    DWORD cbOff, HSZ hszItem, UINT wFmt, 
				    UINT afCmd)
{
    /*
      For now, we ignore idInst, hszItem, wFmt, and afCmd.
      The purpose of these arguments still need to be investigated.
    */
    
    HGLOBAL     		hMem;
    LPBYTE      		pByte;
    DDE_DATAHANDLE_HEAD*	pDdh;
    
    TRACE("(%ld,%p,%ld,%ld,0x%lx,%d,%d): semi-stub\n",
	  idInst,pSrc,cb,cbOff,(DWORD)hszItem,wFmt,afCmd);
    
    /* we use the first 4 bytes to store the size */
    if (!(hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cb + sizeof(DDE_DATAHANDLE_HEAD))))
    {
	ERR("GlobalAlloc failed\n");
	return 0;
    }   
    
    pDdh = (DDE_DATAHANDLE_HEAD*)GlobalLock(hMem);
    pDdh->cfFormat = wFmt;
    
    pByte = (LPBYTE)(pDdh + 1);
    if (pSrc)
    {
	memcpy(pByte, pSrc + cbOff, cb);
    }
    GlobalUnlock(hMem);
    
    return (HDDEDATA)hMem;
}

/*****************************************************************
 *
 *            DdeAddData (USER32.@)
 */
HDDEDATA WINAPI DdeAddData(HDDEDATA hData, LPBYTE pSrc, DWORD cb, DWORD cbOff)
{
    DWORD	old_sz, new_sz;
    LPBYTE	pDst; 
    
    pDst = DdeAccessData(hData, &old_sz);
    if (!pDst) return 0;
    
    new_sz = cb + cbOff;
    if (new_sz > old_sz)
    {
	DdeUnaccessData(hData);
	hData = GlobalReAlloc((HGLOBAL)hData, new_sz + sizeof(DDE_DATAHANDLE_HEAD), 
			      GMEM_MOVEABLE | GMEM_DDESHARE);
	pDst = DdeAccessData(hData, &old_sz);
    }
    
    if (!pDst) return 0;
    
    memcpy(pDst + cbOff, pSrc, cb);
    DdeUnaccessData(hData);
    return hData;
}

/*****************************************************************
 *            DdeSetUserHandle (USER32.@)
 */
BOOL WINAPI DdeSetUserHandle(HCONV hConv, DWORD id, DWORD hUser)
{
    WDML_CONV*	pConv;
    BOOL	ret = TRUE;

    WDML_WaitForMutex(handle_mutex);

    pConv = WDML_GetConv(hConv);
    if (pConv == NULL)
    {
	ret = FALSE;
	goto theError;
    }
    if (id == QID_SYNC) 
    {
	pConv->hUser = hUser;
    }
    else
    {
	WDML_XACT*	pXAct;

	pXAct = WDML_FindTransaction(pConv, id);
	if (pXAct)
	{
	    pXAct->hUser = hUser;
	}
	else
	{
	    pConv->thisInstance->lastError = DMLERR_UNFOUND_QUEUE_ID;
	    ret = FALSE;
	}
    }
 theError:
    WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
    return ret;
}

/******************************************************************************
 * DdeGetData [USER32.@]  Copies data from DDE object to local buffer
 *
 * RETURNS
 *    Size of memory object associated with handle
 */
DWORD WINAPI DdeGetData(
    HDDEDATA hData, /* [in] Handle to DDE object */
    LPBYTE pDst,    /* [in] Pointer to destination buffer */
    DWORD cbMax,    /* [in] Amount of data to copy */
    DWORD cbOff)    /* [in] Offset to beginning of data */
{
    DWORD   dwSize, dwRet;
    LPBYTE  pByte;
    
    TRACE("(%08lx,%p,%ld,%ld)\n",(DWORD)hData,pDst,cbMax,cbOff);
    
    pByte = DdeAccessData(hData, &dwSize);
    
    if (pByte) 
    {
	if (cbOff + cbMax < dwSize)
	{
	    dwRet = cbMax;
	}
	else if (cbOff < dwSize)
	{
	    dwRet = dwSize - cbOff;
	}
	else
	{
	    dwRet = 0;
	}
	if (pDst && dwRet != 0)
	{
	    memcpy(pDst, pByte + cbOff, dwRet);
	}
	DdeUnaccessData(hData);
    }
    else
    {
	dwRet = 0;
    }
    return dwRet;
}

/*****************************************************************
 *            DdeAccessData (USER32.@)
 */
LPBYTE WINAPI DdeAccessData(HDDEDATA hData, LPDWORD pcbDataSize)
{
    HGLOBAL			hMem = (HGLOBAL)hData;
    DDE_DATAHANDLE_HEAD*	pDdh;
    
    TRACE("(%08lx,%p)\n", (DWORD)hData, pcbDataSize);
    
    pDdh = (DDE_DATAHANDLE_HEAD*)GlobalLock(hMem);
    if (pDdh == NULL)
    {
	ERR("Failed on GlobalLock(%04x)\n", hMem);
	return 0;
    }
    
    if (pcbDataSize != NULL)
    {
	*pcbDataSize = GlobalSize(hMem) - sizeof(DDE_DATAHANDLE_HEAD);
    }
    
    return (LPBYTE)(pDdh + 1);
}

/*****************************************************************
 *            DdeUnaccessData (USER32.@)
 */
BOOL WINAPI DdeUnaccessData(HDDEDATA hData)
{
    HGLOBAL hMem = (HGLOBAL)hData;
    
    TRACE("(0x%lx)\n", (DWORD)hData);
    
    GlobalUnlock(hMem);
    
    return TRUE;
}

/*****************************************************************
 *            DdeFreeDataHandle   (USER32.@)
 */
BOOL WINAPI DdeFreeDataHandle(HDDEDATA hData)
{	
    return GlobalFree((HGLOBAL)hData) == 0;
}

/* ================================================================
 *
 *                  Global <=> Data handle management
 *
 * ================================================================ */
 
/* Note: we use a DDEDATA, but layout of DDEDATA, DDEADVISE and DDEPOKE structures is similar:
 *    offset	  size 
 *    (bytes)	 (bits)	comment
 *	0	   16	bit fields for options (release, ackreq, response...)
 *	2	   16	clipboard format
 *	4	   ?	data to be used
 */
HDDEDATA        WDML_Global2DataHandle(HGLOBAL hMem)
{
    DDEDATA*    pDd;
 
    if (hMem)
    {
        pDd = GlobalLock(hMem);
        if (pDd)
        {
            return DdeCreateDataHandle(0, pDd->Value,
                                       GlobalSize(hMem) - (sizeof(DDEDATA) - 1),
                                       0, 0, pDd->cfFormat, 0);
        }
    }
    return 0;
}
 
HGLOBAL WDML_DataHandle2Global(HDDEDATA hDdeData, BOOL fResponse, BOOL fRelease, 
			       BOOL fDeferUpd, BOOL fAckReq)
{
    DDE_DATAHANDLE_HEAD*	pDdh;
    DWORD                       dwSize;
    HGLOBAL                     hMem = 0;
 
    dwSize = GlobalSize(hDdeData) - sizeof(DDE_DATAHANDLE_HEAD);
    pDdh = (DDE_DATAHANDLE_HEAD*)GlobalLock(hDdeData);
    if (dwSize && pDdh)
    {
        hMem = GlobalAlloc(sizeof(DDEDATA) - 1 + dwSize,
                           GMEM_MOVEABLE | GMEM_DDESHARE);
        if (hMem)
        {
            DDEDATA*    ddeData;
 
            ddeData = GlobalLock(hMem);
            if (ddeData)
            {
                ddeData->fResponse = fResponse;
                ddeData->fRelease = fRelease;
		ddeData->reserved /*fDeferUpd*/ = fDeferUpd;
                ddeData->fAckReq = fAckReq;
                ddeData->cfFormat = pDdh->cfFormat;
                memcpy(ddeData->Value, pDdh + 1, dwSize);
                GlobalUnlock(hMem);
            }
        }
        GlobalUnlock(hDdeData);
    }
 
    return hMem;
}                                                                                                                                                                                 

/*****************************************************************
 *            DdeEnableCallback (USER32.@)
 */
BOOL WINAPI DdeEnableCallback(DWORD idInst, HCONV hConv, UINT wCmd)
{
    FIXME("(%ld, 0x%x, %d) stub\n", idInst, hConv, wCmd);
    
    return 0;
}

/******************************************************************************
 * DdeGetLastError [USER32.@]  Gets most recent error code
 *
 * PARAMS
 *    idInst [I] Instance identifier
 *
 * RETURNS
 *    Last error code
 */
UINT WINAPI DdeGetLastError(DWORD idInst)
{
    DWORD		error_code;
    WDML_INSTANCE*	thisInstance;
    
    FIXME("(%ld): error reporting is weakly implemented\n",idInst);
    
    if (WDML_MaxInstanceID == 0)
    {
	/*  Nothing has been initialised - exit now ! */
	return DMLERR_DLL_NOT_INITIALIZED;
    }
    
    if (!WDML_WaitForMutex(handle_mutex))
    {
	return DMLERR_SYS_ERROR;
    }
    
    /*  First check instance
     */
    thisInstance = WDML_FindInstance(idInst);
    if  (thisInstance == NULL) 
    {
	error_code = DMLERR_DLL_NOT_INITIALIZED;
    }
    else
    {
	error_code = thisInstance->lastError;
	thisInstance->lastError = 0;
    }
    
    WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
    return error_code;
}

/*****************************************************************
 *            DdeCmpStringHandles (USER32.@)
 *
 * Compares the value of two string handles.  This comparison is
 * not case sensitive.
 *
 * Returns:
 * -1 The value of hsz1 is zero or less than hsz2
 * 0  The values of hsz 1 and 2 are the same or both zero.
 * 1  The value of hsz2 is zero of less than hsz1
 */
INT WINAPI DdeCmpStringHandles(HSZ hsz1, HSZ hsz2)
{
    CHAR psz1[MAX_BUFFER_LEN];
    CHAR psz2[MAX_BUFFER_LEN];
    int ret = 0;
    int ret1, ret2;
    
    ret1 = GlobalGetAtomNameA(hsz1, psz1, MAX_BUFFER_LEN);
    ret2 = GlobalGetAtomNameA(hsz2, psz2, MAX_BUFFER_LEN);
    TRACE("(%04lx<%s> %04lx<%s>);\n", (DWORD)hsz1, psz1, (DWORD)hsz2, psz2);
    
    /* Make sure we found both strings.
     */
    if (ret1 == 0 && ret2 == 0)
    {
	/* If both are not found, return both  "zero strings".
	 */
	ret = 0;
    }
    else if (ret1 == 0)
    {
	/* If hsz1 is a not found, return hsz1 is "zero string".
	 */
	ret = -1;
    }
    else if (ret2 == 0)
    {
	/* If hsz2 is a not found, return hsz2 is "zero string".
	 */
	ret = 1;
    }
    else
    {
	/* Compare the two strings we got (case insensitive).
	 */
	ret = strcasecmp(psz1, psz2);
	/* Since strcmp returns any number smaller than
	 * 0 when the first string is found to be less than
	 * the second one we must make sure we are returning
	 * the proper values.
	 */
	if (ret < 0)
	{
	    ret = -1;
	}
	else if (ret > 0)
	{
	    ret = 1;
	}
    }
    
    return ret;
}

/******************************************************************
 *		DdeQueryConvInfo (USER32.@)
 *
 */
UINT WINAPI DdeQueryConvInfo(HCONV hConv, DWORD id, LPCONVINFO lpConvInfo)
{
    UINT	ret = lpConvInfo->cb;
    CONVINFO	ci;
    WDML_CONV*	pConv;

    FIXME("semi-stub.\n");

    WDML_WaitForMutex(handle_mutex);

    pConv = WDML_GetConv(hConv);
    if (pConv == NULL)
    {
	WDML_ReleaseMutex(handle_mutex, "hande_mutex", FALSE);
	return 0;
    }

    ci.hConvPartner = 0; /* FIXME */
    ci.hszSvcPartner = pConv->hszService;
    ci.hszServiceReq = pConv->hszService; /* FIXME: they shouldn't be the same, should they ? */
    ci.hszTopic = pConv->hszTopic;
    ci.wStatus = ST_CLIENT /* FIXME */ | ST_CONNECTED;
    ci.wConvst = 0; /* FIXME */
    ci.wLastError = 0; /* FIXME: note it's not the instance last error */
    ci.hConvList = 0;
    ci.ConvCtxt = pConv->convContext;
    if (ci.wStatus & ST_CLIENT)
    {
	ci.hwnd = pConv->hwndClient;
	ci.hwndPartner = pConv->hwndServer;
    }
    else
    {
	ci.hwnd = pConv->hwndServer;
	ci.hwndPartner = pConv->hwndClient;
    }
    if (id == QID_SYNC)
    {
	ci.hUser = pConv->hUser;
	ci.hszItem = 0;
	ci.wFmt = 0;
	ci.wType = 0;
    }
    else
    {
	WDML_XACT*	pXAct;

	pXAct = WDML_FindTransaction(pConv, id);
	if (pXAct)
	{
	    ci.hUser = pXAct->hUser;
	    ci.hszItem = 0; /* FIXME */
	    ci.wFmt = 0; /* FIXME */
	    ci.wType = 0; /* FIXME */
	}
	else
	{
	    ret = 0;
	    pConv->thisInstance->lastError = DMLERR_UNFOUND_QUEUE_ID;
	}
    }

    WDML_ReleaseMutex(handle_mutex, "hande_mutex", FALSE);
    memcpy(lpConvInfo, &ci, min(lpConvInfo->cb, sizeof(ci)));
    return ret;
}

/* ================================================================
 *
 * 			Server management
 *
 * ================================================================ */

/******************************************************************
 *		WDML_AddServer
 *
 *
 */
WDML_SERVER*	WDML_AddServer(WDML_INSTANCE* thisInstance, HSZ hszService, HSZ hszTopic)
{
    WDML_SERVER* pServer;
    
    pServer = (WDML_SERVER*)HeapAlloc(GetProcessHeap(), 0, sizeof(WDML_SERVER));
    if (pServer == NULL) return NULL;
    
    pServer->hszService = hszService;
    pServer->hszTopic = 0;
    pServer->filterOn = TRUE;
    
    pServer->next = thisInstance->servers;
    thisInstance->servers = pServer;
    return pServer;
}

/******************************************************************
 *		WDML_RemoveServer
 *
 *
 */
void WDML_RemoveServer(WDML_INSTANCE* thisInstance, HSZ hszService, HSZ hszTopic)
{
    WDML_SERVER* pPrev = NULL;
    WDML_SERVER* pCurrent = NULL;
    
    pCurrent = thisInstance->servers;
    
    while (pCurrent != NULL)
    {
	if (DdeCmpStringHandles(pCurrent->hszService, hszService) == 0)
	{
	    if (pCurrent == thisInstance->servers)
	    {
		thisInstance->servers = pCurrent->next;
	    }
	    else
	    {
		pPrev->next = pCurrent->next;
	    }
	    
	    DestroyWindow(pCurrent->hwndServer);
	    
	    HeapFree(GetProcessHeap(), 0, pCurrent);
	    break;
	}
	
	pPrev = pCurrent;
	pCurrent = pCurrent->next;
    }
}

/*****************************************************************************
 *	WDML_FindServer
 *
 *	generic routine to return a pointer to the relevant ServiceNode
 *	for a given service name, or NULL if the entry does not exist
 *
 *	ASSUMES the mutex protecting the handle entry list is reserved before calling
 */
WDML_SERVER*	WDML_FindServer(WDML_INSTANCE* thisInstance, HSZ hszService, HSZ hszTopic)
{
    WDML_SERVER*	pServer;
    
    for (pServer = thisInstance->servers; pServer != NULL; pServer = pServer->next)
    {
	if (hszService == pServer->hszService)
	{
	    return pServer;
	}
    }
    TRACE("Service name missing\n");
    return NULL;
}

/* ================================================================
 *
 * 		Conversation management
 *
 * ================================================================ */

/******************************************************************
 *		WDML_AddConv
 *
 *
 */
WDML_CONV*	WDML_AddConv(WDML_INSTANCE* thisInstance, WDML_SIDE side,
			     HSZ hszService, HSZ hszTopic, HWND hwndClient, HWND hwndServer)
{
    WDML_CONV*	pConv;
    
    /* no converstation yet, add it */
    pConv = HeapAlloc(GetProcessHeap(), 0, sizeof(WDML_CONV));
    if (!pConv) return NULL;
    
    pConv->thisInstance = thisInstance;
    pConv->hszService = hszService;
    pConv->hszTopic = hszTopic;
    pConv->hwndServer = hwndServer;
    pConv->hwndClient = hwndClient;
    pConv->transactions = NULL;
    pConv->hUser = 0;

    pConv->next = thisInstance->convs[side];
    thisInstance->convs[side] = pConv;
    
    return pConv;
}

/******************************************************************
 *		WDML_FindConv
 *
 *
 */
WDML_CONV*	WDML_FindConv(WDML_INSTANCE* thisInstance, WDML_SIDE side, 
			      HSZ hszService, HSZ hszTopic)
{
    WDML_CONV*	pCurrent = NULL;
    
    for (pCurrent = thisInstance->convs[side]; pCurrent != NULL; pCurrent = pCurrent->next)
    {
	if (DdeCmpStringHandles(pCurrent->hszService, hszService) == 0 &&
	    DdeCmpStringHandles(pCurrent->hszTopic, hszTopic) == 0)
	{
	    return pCurrent;
	}
	
    }
    return NULL;
}

/******************************************************************
 *		WDML_RemoveConv
 *
 *
 */
void WDML_RemoveConv(WDML_INSTANCE* thisInstance, WDML_SIDE side, HCONV hConv)
{
    WDML_CONV* pPrev = NULL;
    WDML_CONV* pRef = WDML_GetConv(hConv);
    WDML_CONV* pCurrent = NULL;

    if (!pRef)
	return;
    for (pCurrent = thisInstance->convs[side]; pCurrent != NULL; pCurrent = (pPrev = pCurrent)->next)
    {
	if (pCurrent == pRef)
	{
	    if (pCurrent == thisInstance->convs[side])
	    {
		thisInstance->convs[side] = pCurrent->next;
	    }
	    else
	    {
		pPrev->next = pCurrent->next;
	    }
	    
	    HeapFree(GetProcessHeap(), 0, pCurrent);
	    break;
	}
    }
}

/******************************************************************
 *		WDML_GetConv
 *
 * 
 */
WDML_CONV*	WDML_GetConv(HCONV hConv)
{
    /* FIXME: should do better checking */
    return (WDML_CONV*)hConv;
}

/* ================================================================
 *
 * 			Link (hot & warm) management
 *
 * ================================================================ */

/******************************************************************
 *		WDML_AddLink
 *
 *
 */
void WDML_AddLink(WDML_INSTANCE* thisInstance, HCONV hConv, WDML_SIDE side, 
		  UINT wType, HSZ hszItem, UINT wFmt)
{
    WDML_LINK*	pLink;
    
    TRACE("AddDdeLink was called...\n");
    
    pLink = HeapAlloc(GetProcessHeap(), 0, sizeof(WDML_LINK));
    if (pLink == NULL)
    {
	ERR("OOM\n");
	return;
    }

    pLink->hConv = hConv;
    pLink->transactionType = wType;
    pLink->hszItem = hszItem;
    pLink->uFmt = wFmt;
    pLink->hDdeData = 0;
    pLink->next = thisInstance->links[side];
    thisInstance->links[side] = pLink;
}

/******************************************************************
 *		WDML_RemoveLink
 *
 *
 */
void WDML_RemoveLink(WDML_INSTANCE* thisInstance, HCONV hConv, WDML_SIDE side, 
		     HSZ hszItem, UINT uFmt)
{
    WDML_LINK* pPrev = NULL;
    WDML_LINK* pCurrent = NULL;
    
    pCurrent = thisInstance->links[side];
    
    while (pCurrent != NULL)
    {
	if (pCurrent->hConv == hConv &&
	    DdeCmpStringHandles(pCurrent->hszItem, hszItem) == 0 &&
	    pCurrent->uFmt == uFmt)
	{
	    if (pCurrent == thisInstance->links[side])
	    {
		thisInstance->links[side] = pCurrent->next;
	    }
	    else
	    {
		pPrev->next = pCurrent->next;
	    }
	    
	    if (pCurrent->hDdeData)
	    {
		DdeFreeDataHandle(pCurrent->hDdeData);
	    }
	    
	    HeapFree(GetProcessHeap(), 0, pCurrent);
	    break;
	}
	
	pPrev = pCurrent;
	pCurrent = pCurrent->next;
    }
}

/* this function is called to remove all links related to the conv.
   It should be called from both client and server when terminating 
   the conversation.
*/
/******************************************************************
 *		WDML_RemoveAllLinks
 *
 *
 */
void WDML_RemoveAllLinks(WDML_INSTANCE* thisInstance, HCONV hConv, WDML_SIDE side)
{
    WDML_LINK* pPrev = NULL;
    WDML_LINK* pCurrent = NULL;
    WDML_LINK* pNext = NULL;
    
    pCurrent = thisInstance->links[side];
    
    while (pCurrent != NULL)
    {
	if (pCurrent->hConv == hConv)
	{
	    if (pCurrent == thisInstance->links[side])
	    {
		thisInstance->links[side] = pCurrent->next;
		pNext = pCurrent->next;
	    }
	    else
	    {
		pPrev->next = pCurrent->next;
		pNext = pCurrent->next;
	    }
	    
	    if (pCurrent->hDdeData)
	    {
		DdeFreeDataHandle(pCurrent->hDdeData);
	    }
	    
	    HeapFree(GetProcessHeap(), 0, pCurrent);
	    pCurrent = NULL;
	}
	
	if (pCurrent)
	{
	    pPrev = pCurrent;
	    pCurrent = pCurrent->next;
	}
	else
	{
	    pCurrent = pNext;
	}
    }
}

/******************************************************************
 *		WDML_FindLink
 *
 *
 */
WDML_LINK* 	WDML_FindLink(WDML_INSTANCE* thisInstance, HCONV hConv, WDML_SIDE side, 
			      HSZ hszItem, UINT uFmt)
{
    WDML_LINK*	pCurrent = NULL;
    
    for (pCurrent = thisInstance->links[side]; pCurrent != NULL; pCurrent = pCurrent->next)
    {
	/* we don't need to check for transaction type as
	   it can be altered */
	
	if (pCurrent->hConv == hConv &&
	    DdeCmpStringHandles(pCurrent->hszItem, hszItem) == 0 &&
	    pCurrent->uFmt == uFmt)
	{
	    break;
	}
	
    }
    
    return pCurrent;
}

/* ================================================================
 *
 * 			Transaction management
 *
 * ================================================================ */

/******************************************************************
 *		WDML_AllocTransaction
 *
 * Alloc a transaction structure for handling the message ddeMsg
 */
WDML_XACT*	WDML_AllocTransaction(WDML_INSTANCE* thisInstance, UINT ddeMsg)
{
    WDML_XACT*		pXAct;
    static WORD		tid = 1;	/* FIXME: wrap around */

    pXAct = HeapAlloc(GetProcessHeap(), 0, sizeof(WDML_XACT));
    if (!pXAct) 
    {
	thisInstance->lastError = DMLERR_MEMORY_ERROR;
	return NULL;
    }

    pXAct->xActID = tid++;
    pXAct->ddeMsg = ddeMsg;
    pXAct->hDdeData = 0;
    pXAct->hUser = 0;
    pXAct->next = NULL;
    
    return pXAct;
}

/******************************************************************
 *		WDML_QueueTransaction
 *
 * Adds a transaction to the list of transaction
 */
void	WDML_QueueTransaction(WDML_CONV* pConv, WDML_XACT* pXAct)
{
    WDML_XACT**	pt;
    
    /* advance to last in queue */
    for (pt = &pConv->transactions; *pt != NULL; pt = &(*pt)->next);
    *pt = pXAct;
}

/******************************************************************
 *		WDML_UnQueueTransaction
 *
 *
 */
BOOL	WDML_UnQueueTransaction(WDML_CONV* pConv, WDML_XACT*  pXAct)
{
    WDML_XACT**	pt;

    for (pt = &pConv->transactions; *pt; pt = &(*pt)->next)
    {
	if (*pt == pXAct)
	{
	    *pt = pXAct->next;
	    return TRUE;
	}
    }
    return FALSE;
}

/******************************************************************
 *		WDML_FreeTransaction
 *
 *
 */
void	WDML_FreeTransaction(WDML_XACT* pXAct)
{
    HeapFree(GetProcessHeap(), 0, pXAct);
}

/******************************************************************
 *		WDML_FindTransaction
 *
 *
 */
WDML_XACT*	WDML_FindTransaction(WDML_CONV* pConv, DWORD tid)
{
    WDML_XACT* pXAct;
    
    tid = HIWORD(tid);
    for (pXAct = pConv->transactions; pXAct; pXAct = pXAct->next)
    {
	if (pXAct->xActID == tid)
	    break;
    }
    return pXAct;
}

struct tagWDML_BroadcastPmt
{
    LPCSTR	clsName;
    UINT	uMsg;
    WPARAM	wParam;
    LPARAM	lParam;
};

/******************************************************************
 *		WDML_BroadcastEnumProc
 *
 *
 */
static	BOOL CALLBACK WDML_BroadcastEnumProc(HWND hWnd, LPARAM lParam)
{
    struct tagWDML_BroadcastPmt*	s = (struct tagWDML_BroadcastPmt*)lParam;
    char				buffer[128];

    if (GetClassNameA(hWnd, buffer, sizeof(buffer)) > 0 &&
	strcmp(buffer, s->clsName) == 0)
    {
	PostMessageA(hWnd, s->uMsg, s->wParam, s->lParam);
    }
    return TRUE;
}

/******************************************************************
 *		WDML_BroadcastDDEWindows
 *
 *
 */
void WDML_BroadcastDDEWindows(const char* clsName, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    struct tagWDML_BroadcastPmt	s;

    s.clsName = clsName;
    s.uMsg    = uMsg;
    s.wParam  = wParam;
    s.lParam  = lParam;
    EnumWindows(WDML_BroadcastEnumProc, (LPARAM)&s);
}
