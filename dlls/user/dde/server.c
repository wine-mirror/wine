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
#include "dde/dde_private.h"

DEFAULT_DEBUG_CHANNEL(ddeml);

static const char szServerNameClassA[] = "DdeServerNameAnsi";
static const char szServerConvClassA[] = "DdeServerConvAnsi";

static LRESULT CALLBACK WDML_ServerNameProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK WDML_ServerConvProc(HWND, UINT, WPARAM, LPARAM);

/******************************************************************************
 * DdePostAdvise [USER32.@]  Send transaction to DDE callback function.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI DdePostAdvise(
    DWORD idInst, /* [in] Instance identifier */
    HSZ hszTopic, /* [in] Handle to topic name string */
    HSZ hszItem)  /* [in] Handle to item name string */
{
    WDML_INSTANCE*	thisInstance = NULL;
    WDML_LINK*		pLink = NULL;
    HDDEDATA		hDdeData = 0, hItemData = 0;
    WDML_CONV*		pConv = NULL;
    CHAR 		pszTopic[MAX_BUFFER_LEN];
    CHAR 		pszItem[MAX_BUFFER_LEN];
    
    
    TRACE("(%ld,%ld,%ld)\n",idInst,(DWORD)hszTopic,(DWORD)hszItem);
    
    if (idInst == 0)
    {
	return FALSE;
    }
    
    thisInstance = WDML_FindInstance(idInst);
    
    if (thisInstance == NULL || thisInstance->links == NULL)
    {
	return FALSE;
    }
    
    GlobalGetAtomNameA(hszTopic, (LPSTR)pszTopic, MAX_BUFFER_LEN);
    GlobalGetAtomNameA(hszItem, (LPSTR)pszItem, MAX_BUFFER_LEN);
    
    for (pLink = thisInstance->links[WDML_SERVER_SIDE]; pLink != NULL; pLink = pLink->next)
    {
	if (DdeCmpStringHandles(hszItem, pLink->hszItem) == 0)
	{
	    hDdeData = 0;
	    if (thisInstance->callback != NULL /* && thisInstance->Process_id == GetCurrentProcessId()*/)
	    {
		
		TRACE("Calling the callback, type=XTYP_ADVREQ, CB=0x%lx, hConv=0x%lx, Topic=%s, Item=%s\n",
		      (DWORD)thisInstance->callback, (DWORD)pLink->hConv, pszTopic, pszItem);
		hDdeData = (thisInstance->callback)(XTYP_ADVREQ,
						    pLink->uFmt,
						    pLink->hConv,
						    hszTopic,
						    hszItem,
						    0, 0, 0);
		TRACE("Callback was called\n");
		
	    }
	    
	    if (hDdeData)
	    {
		if (pLink->transactionType & XTYPF_NODATA)
		{
		    TRACE("no data\n");
		    hItemData = 0;
		}
		else
		{
		    TRACE("with data\n");
		    
		    hItemData = WDML_DataHandle2Global(hDdeData, FALSE, FALSE, FALSE, FALSE);
		}
		
		pConv = WDML_GetConv(pLink->hConv);
		
		if (pConv == NULL ||
		    !PostMessageA(pConv->hwndClient, WM_DDE_DATA, (WPARAM)pConv->hwndServer,
				  PackDDElParam(WM_DDE_DATA, (UINT)hItemData, (DWORD)hszItem)))
		{
		    ERR("post message failed\n");
		    DdeFreeDataHandle(hDdeData);
		    return FALSE;
		}		    
	    }
	}
    }
    
    return TRUE;
}


/******************************************************************************
 * DdeNameService [USER32.@]  {Un}registers service name of DDE server
 *
 * PARAMS
 *    idInst [I] Instance identifier
 *    hsz1   [I] Handle to service name string
 *    hsz2   [I] Reserved
 *    afCmd  [I] Service name flags
 *
 * RETURNS
 *    Success: Non-zero
 *    Failure: 0
 */
HDDEDATA WINAPI DdeNameService(DWORD idInst, HSZ hsz1, HSZ hsz2, UINT afCmd)
{
    WDML_SERVER*	pServer;
    WDML_SERVER*	pServerTmp;
    WDML_INSTANCE*	thisInstance;
    HDDEDATA 		hDdeData;
    HSZ 		hsz2nd = 0;
    HWND 		hwndServer;
    WNDCLASSEXA  	wndclass;
    
    hDdeData = (HDDEDATA)NULL;
    
    TRACE("(%ld,%d,%d,%d): stub\n",idInst,hsz1,hsz2,afCmd);
    
    if (WDML_MaxInstanceID == 0)
    {
	/*  Nothing has been initialised - exit now ! 
	 *	needs something for DdeGetLastError */
	return 0;
    }
    
    if (!WDML_WaitForMutex(handle_mutex))
    {
	/* FIXME: setError DMLERR_SYS_ERROR; */
	return 0;
    }
    
    /*  First check instance
     */
    thisInstance = WDML_FindInstance(idInst);
    if  (thisInstance == NULL)
    {
	TRACE("Instance not found as initialised\n");
	WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
	/*  Nothing has been initialised - exit now ! can return TRUE since effect is the same */
	return FALSE;
    }
    
    if (hsz2 != 0L)
    {
	/*	Illegal, reserved parameter
	 */
	thisInstance->lastError = DMLERR_INVALIDPARAMETER;
	WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
	FIXME("Reserved parameter no-zero !!\n");
	return FALSE;
    }
    if (hsz1 == 0L)
    {
	/*
	 *	General unregister situation
	 */
	if (afCmd != DNS_UNREGISTER)
	{
	    /*	don't know if we should check this but it makes sense
	     *	why supply REGISTER or filter flags if de-registering all
	     */
	    TRACE("General unregister unexpected flags\n");
	    thisInstance->lastError = DMLERR_DLL_USAGE;
	    WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
	    return FALSE;
	}
	/*	Loop to find all registered service and de-register them
	 */
	if (thisInstance->servers == NULL)
	{
	    /*  None to unregister !!  
	     */
	    TRACE("General de-register - nothing registered\n");
	    thisInstance->lastError = DMLERR_DLL_USAGE;
	    WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
	    return FALSE;
	}
	else
	{
	    pServer = thisInstance->servers;
	    while (pServer != NULL)
	    {
		TRACE("general deregister - iteration\n");
		pServerTmp = pServer;
		pServer = pServer->next;
		WDML_ReleaseAtom(thisInstance, pServerTmp->hszService);
		/* finished - release heap space used as work store */
		HeapFree(GetProcessHeap(), 0, pServerTmp); 
	    }
	    thisInstance->servers = NULL;
	    TRACE("General de-register - finished\n");
	}
	WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
	return (HDDEDATA)TRUE;
    }
    TRACE("Specific name action detected\n");
    if (afCmd & DNS_REGISTER)
    {
	/* Register new service name
	 */
	
	pServer = WDML_FindServer(thisInstance, hsz1, 0);
	if (pServer)
	    ERR("Trying to register already registered service!\n");
	else
	{
	    TRACE("Adding service name\n");
	    
	    WDML_ReserveAtom(thisInstance, hsz1);
	    
	    pServer = WDML_AddServer(thisInstance, hsz1, 0);
	    
	    WDML_BroadcastDDEWindows(WDML_szEventClass, WM_WDML_REGISTER, hsz1, hsz2nd);
	}
	
	wndclass.cbSize        = sizeof(wndclass);
	wndclass.style         = 0;
	wndclass.lpfnWndProc   = WDML_ServerNameProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 2 * sizeof(DWORD);
	wndclass.hInstance     = 0;
	wndclass.hIcon         = 0;
	wndclass.hCursor       = 0;
	wndclass.hbrBackground = 0;
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szServerNameClassA;
	wndclass.hIconSm       = 0;
	
	RegisterClassExA(&wndclass);
	
	hwndServer = CreateWindowA(szServerNameClassA, NULL,
				   WS_POPUP, 0, 0, 0, 0,
				   0, 0, 0, 0);
	
	SetWindowLongA(hwndServer, 0, (DWORD)thisInstance);
	TRACE("Created nameServer=%04x for instance=%08lx\n", hwndServer, idInst);
	
	pServer->hwndServer = hwndServer;
    }
    if (afCmd & DNS_UNREGISTER)
    {
	TRACE("Broadcasting WM_DDE_TERMINATE message\n");
	SendMessageA(HWND_BROADCAST, WM_DDE_TERMINATE, (WPARAM)NULL,
		     PackDDElParam(WM_DDE_TERMINATE, (UINT)hsz1, (UINT)hsz2));
	
	WDML_RemoveServer(thisInstance, hsz1, hsz2);
    }
    if (afCmd & DNS_FILTERON)
    {
	/*	Set filter flags on to hold notifications of connection
	 *
	 *	test coded this way as this is the default setting
	 */
	pServer = WDML_FindServer(thisInstance, hsz1, 0);
	if (!pServer)
	{
	    /*  trying to filter where no service names !!
	     */
	    thisInstance->lastError = DMLERR_DLL_USAGE;
	    WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
	    return FALSE;
	} 
	else 
	{
	    pServer->filterOn = TRUE;
	}
    }
    if (afCmd & DNS_FILTEROFF)
    {
	/*	Set filter flags on to hold notifications of connection
	 */
	pServer = WDML_FindServer(thisInstance, hsz1, 0);
	if (!pServer)
	{
	    /*  trying to filter where no service names !!
	     */
	    thisInstance->lastError = DMLERR_DLL_USAGE;
	    WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
	    return FALSE;
	} 	
	else 
	{
	    pServer->filterOn = FALSE;
	}
    }
    WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
    return (HDDEDATA)TRUE;
}

/******************************************************************
 *		WDML_CreateServerConv
 *
 *
 */
static BOOL WDML_CreateServerConv(WDML_INSTANCE* thisInstance, HWND hwndClient, HWND hwndServerName,
				  HSZ hszApp, HSZ hszTopic)
{
    WNDCLASSEXA	wndclass;
    HWND	hwndServerConv;
    WDML_CONV*	pConv;
    
    wndclass.cbSize        = sizeof(wndclass);
    wndclass.style         = 0;
    wndclass.lpfnWndProc   = WDML_ServerConvProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 2 * sizeof(DWORD);
    wndclass.hInstance     = 0;
    wndclass.hIcon         = 0;
    wndclass.hCursor       = 0;
    wndclass.hbrBackground = 0;
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = szServerConvClassA;
    wndclass.hIconSm       = 0;
    
    RegisterClassExA(&wndclass);
    
    hwndServerConv = CreateWindowA(szServerConvClassA, 0,
				   WS_CHILD, 0, 0, 0, 0,
				   hwndServerName, 0, 0, 0);
    TRACE("Created convServer=%04x (nameServer=%04x) for instance=%08lx\n", 
	  hwndServerConv, hwndServerName, thisInstance->instanceID);
    
    pConv = WDML_AddConv(thisInstance, WDML_SERVER_SIDE, hszApp, hszTopic, hwndClient, hwndServerConv);
    
    SetWindowLongA(hwndServerConv, 0, (DWORD)thisInstance);
    SetWindowLongA(hwndServerConv, 4, (DWORD)pConv);
    
    /* this should be the only place using SendMessage for WM_DDE_ACK */
    SendMessageA(hwndClient, WM_DDE_ACK, (WPARAM)hwndServerConv,
		 PackDDElParam(WM_DDE_ACK, (UINT)hszApp, (UINT)hszTopic));

#if 0
    if (thisInstance && thisInstance->callback != NULL /*&& thisInstance->Process_id == GetCurrentProcessId()*/)
    {
	/* confirm connection...
	 * FIXME: a better way would be to check for any incoming message if the conversation
	 * exists (and/or) has been confirmed...
	 * Anyway, always pretend we use a connection from a different instance...
	 */
	(thisInstance->callback)(XTYP_CONNECT_CONFIRM, 0, (HCONV)pConv, hszApp, hszTopic, 0, 0, 0);
    }
#endif

    return TRUE;
}

/******************************************************************
 *		WDML_ServerNameProc
 *
 *
 */
static LRESULT CALLBACK WDML_ServerNameProc(HWND hwndServer, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    HWND		hwndClient;
    HSZ			hszApp, hszTop;
    HDDEDATA		hDdeData = 0;
    WDML_INSTANCE*	thisInstance;
    UINT		uiLow, uiHi;
    
    switch (iMsg)
    {
    case WM_DDE_INITIATE:
	
	/* wParam         -- sending window handle
	   LOWORD(lParam) -- application atom
	   HIWORD(lParam) -- topic atom */
	
	TRACE("WM_DDE_INITIATE message received in the Server Proc!\n");
	hwndClient = (HWND)wParam;
	
	/* don't free DDEParams, since this is a broadcast */
	UnpackDDElParam(WM_DDE_INITIATE, lParam, &uiLow, &uiHi);	
	
	hszApp = (HSZ)uiLow;
	hszTop = (HSZ)uiHi;
	
	thisInstance = (WDML_INSTANCE*)GetWindowLongA(hwndServer, 0);
	TRACE("idInst=%ld, ProcessID=0x%lx\n", thisInstance->instanceID, GetCurrentProcessId());
	
	if (hszApp && hszTop) 
	{
	    /* pass on to the callback  */
	    if (thisInstance && thisInstance->callback != NULL /*&& thisInstance->Process_id == GetCurrentProcessId()*/)
	    {
		
		TRACE("calling the Callback, type = XTYP_CONNECT, CB=0x%lx\n",
		      (DWORD)thisInstance->callback);
		hDdeData = (thisInstance->callback)(XTYP_CONNECT,
						    0, 0,
						    hszTop,
						    hszApp,
						    0, 0, 0);
		if ((UINT)hDdeData)
		{
		    WDML_CreateServerConv(thisInstance, hwndClient, hwndServer, hszApp, hszTop);
		}
	    }
	}
	else
	{
	    /* pass on to the callback  */
	    if (thisInstance && thisInstance->callback != NULL /*&& thisInstance->Process_id == GetCurrentProcessId()*/)
	    {
		TRACE("calling the Callback, type=XTYP_WILDCONNECT, CB=0x%lx\n",
		      (DWORD)thisInstance->callback);
		hDdeData = (thisInstance->callback)(XTYP_WILDCONNECT,
						    0, 0,
						    hszTop,
						    hszApp,
						    0, 0, 0);
		if ((UINT)hDdeData)
		{
		    HSZPAIR*	hszp;
		    
		    hszp = (HSZPAIR*)DdeAccessData(hDdeData, NULL);
		    if (hszp)
		    {
			int	i;
			for (i = 0; hszp[i].hszSvc && hszp[i].hszTopic; i++)
			{
			    WDML_CreateServerConv(thisInstance, hwndClient, hwndServer, 
						  hszp[i].hszSvc, hszp[i].hszTopic);
			}
			DdeUnaccessData(hDdeData);
		    }
		}
	    }
	}
	
	/*
	  billx: make a conv and add it to the server list - 
	  this can be delayed when link is created for the conv. NO NEED !!!
	*/
	
	return 0;
	
	
    case WM_DDE_REQUEST:
	FIXME("WM_DDE_REQUEST message received!\n");
	return 0;
    case WM_DDE_ADVISE:
	FIXME("WM_DDE_ADVISE message received!\n");
	return 0;
    case WM_DDE_UNADVISE:
	FIXME("WM_DDE_UNADVISE message received!\n");
	return 0;
    case WM_DDE_EXECUTE:
	FIXME("WM_DDE_EXECUTE message received!\n");
	return 0;
    case WM_DDE_POKE:
	FIXME("WM_DDE_POKE message received!\n");
	return 0;
    case WM_DDE_TERMINATE:
	FIXME("WM_DDE_TERMINATE message received!\n");
	return 0;

    }
    
    return DefWindowProcA(hwndServer, iMsg, wParam, lParam);
}

/******************************************************************
 *		WDML_ServerHandleRequest
 *
 *
 */
static	LRESULT	WDML_ServerHandleRequest(WDML_INSTANCE* thisInstance, WDML_CONV* pConv, 
					 HWND hwndServer, HWND hwndClient, LPARAM lParam)
{
    UINT		uiLow, uiHi;
    HSZ			hszItem;
    HDDEDATA		hDdeData;

    TRACE("(%04x %04x %08lx)!\n", hwndServer, hwndClient, lParam);
	
    UnpackDDElParam(WM_DDE_REQUEST, lParam, &uiLow, &uiHi);

    hszItem = (HSZ)uiHi;

    hDdeData = 0;
    if (thisInstance && thisInstance->callback != NULL /* && thisInstance->Process_id == GetCurrentProcessId() */)
    {
	    
	TRACE("calling the Callback, idInst=%ld, CB=0x%lx, uType=0x%x\n",
	      thisInstance->instanceID, (DWORD)thisInstance->callback, XTYP_REQUEST);
	hDdeData = (thisInstance->callback)(XTYP_REQUEST, uiLow, (HCONV)pConv, 
					    pConv->hszTopic, hszItem, 0, 0, 0);
    }
	
    if (hDdeData)
    {
	HGLOBAL	hMem = WDML_DataHandle2Global(hDdeData, FALSE, FALSE, FALSE, FALSE);
	if (!PostMessageA(hwndClient, WM_DDE_DATA, (WPARAM)hwndServer,
			  ReuseDDElParam(lParam, WM_DDE_REQUEST, WM_DDE_DATA, (UINT)hMem, (UINT)hszItem)))
	{
	    DdeFreeDataHandle(hDdeData);
	    GlobalFree(hMem);
	}
    }
    else 
    {
	DDEACK	ddeAck;

	ddeAck.bAppReturnCode = 0;
	ddeAck.reserved       = 0;
	ddeAck.fBusy          = FALSE;
	ddeAck.fAck           = FALSE;
	
	TRACE("Posting a %s ack\n", ddeAck.fAck ? "positive" : "negative");
	PostMessageA(hwndClient, WM_DDE_ACK, (WPARAM)hwndServer, 
		     ReuseDDElParam(lParam, WM_DDE_REQUEST, WM_DDE_ACK, *((WORD*)&ddeAck), (UINT)hszItem));
    }	

    return 0;
}

/******************************************************************
 *		WDML_ServerHandleAdvise
 *
 *
 */
static	LRESULT	WDML_ServerHandleAdvise(WDML_INSTANCE* thisInstance, WDML_CONV* pConv, 
					HWND hwndServer, HWND hwndClient, LPARAM lParam)
{
    UINT		uiLo, uiHi, uType;
    HGLOBAL		hDdeAdvise;
    HSZ			hszItem;
    WDML_LINK*		pLink;
    DDEADVISE*		pDdeAdvise;
    HDDEDATA		hDdeData;
    DDEACK		ddeAck;

    /* XTYP_ADVSTART transaction: 
       establish link and save link info to InstanceInfoTable */
	
    TRACE("(%04x %04x %08lx)!\n", hwndServer, hwndClient, lParam);

    UnpackDDElParam(WM_DDE_ADVISE, lParam, &uiLo, &uiHi);
	
    hDdeAdvise = (HGLOBAL)uiLo;
    hszItem    = (HSZ)uiHi; /* FIXME: it should be a global atom */

    if (!pConv) 
    {
	ERR("Got an advise on a not known conversation, dropping request\n");
	FreeDDElParam(WM_DDE_ADVISE, lParam);
	return 0;
    }

    pDdeAdvise = (DDEADVISE*)GlobalLock(hDdeAdvise);
    uType = XTYP_ADVSTART | 
	    (pDdeAdvise->fDeferUpd ? XTYPF_NODATA : 0) |
	    (pDdeAdvise->fAckReq ? XTYPF_ACKREQ : 0);
	
    hDdeData = 0;
    if (thisInstance && thisInstance->callback != NULL /* && thisInstance->Process_id == GetCurrentProcessId() */)
    {
	TRACE("calling the Callback, idInst=%ld, CB=0x%lx, uType=0x%x, uFmt=%x\n",
	      thisInstance->instanceID, (DWORD)thisInstance->callback, 
	      uType, pDdeAdvise->cfFormat);
	hDdeData = (thisInstance->callback)(XTYP_ADVSTART, pDdeAdvise->cfFormat, (HCONV)pConv, 
					    pConv->hszTopic, hszItem, 0, 0, 0);
    }
    
    ddeAck.bAppReturnCode = 0;
    ddeAck.reserved       = 0;
    ddeAck.fBusy          = FALSE;

    if ((UINT)hDdeData || TRUE)	/* FIXME (from Corel ?) some apps don't return this value */
    {
	ddeAck.fAck           = TRUE;
	
	/* billx: first to see if the link is already created. */
	pLink = WDML_FindLink(thisInstance, (HCONV)pConv, WDML_SERVER_SIDE, 
			      hszItem, pDdeAdvise->cfFormat);

	if (pLink != NULL)
	{
	    /* we found a link, and only need to modify it in case it changes */
	    pLink->transactionType = uType;
	}
	else
	{
	    TRACE("Adding Link with hConv=0x%lx\n", (DWORD)pConv);
	    
	    WDML_AddLink(thisInstance, (HCONV)pConv, WDML_SERVER_SIDE, 
			 uType, hszItem, pDdeAdvise->cfFormat);
	}
    }
    else
    {
	TRACE("No data returned from the Callback\n");
	
	ddeAck.fAck           = FALSE;
    }
	
    GlobalUnlock(hDdeAdvise);
    if (ddeAck.fAck)
	GlobalFree(hDdeAdvise);
	
    TRACE("Posting a %s ack\n", ddeAck.fAck ? "positive" : "negative");
    PostMessageA(hwndClient, WM_DDE_ACK, (WPARAM)hwndServer, 
		 ReuseDDElParam(lParam, WM_DDE_ADVISE, WM_DDE_ACK, *((WORD*)&ddeAck), (UINT)hszItem));

    return 0L;
}

/******************************************************************
 *		WDML_ServerHandleUnadvise
 *
 *
 */
static	LRESULT WDML_ServerHandleUnadvise(WDML_INSTANCE* thisInstance, WDML_CONV* pConv, 
					  HWND hwndServer, HWND hwndClient, LPARAM lParam)
{
    UINT		uiLo, uiHi;
    HSZ			hszItem;
    WDML_LINK*		pLink;
    DDEACK		ddeAck;

    TRACE("(%04x %04x %08lx)!\n", hwndServer, hwndClient, lParam);
	
    /* billx: XTYP_ADVSTOP transaction */
    UnpackDDElParam(WM_DDE_UNADVISE, lParam, &uiLo, &uiHi);
	
    /* uiLow: wFmt */
    hszItem    = (HSZ)uiHi; /* FIXME: it should be a global atom */

    if (hszItem == (HSZ)0 || uiLo == 0)
    {
	ERR("Unsupported yet options (null item or clipboard format\n");
    }

    pLink = WDML_FindLink(thisInstance, (HCONV)pConv, WDML_SERVER_SIDE, hszItem, uiLo);
    if (pLink == NULL)
    {
	ERR("Couln'd find link for %08lx, dropping request\n", (DWORD)hszItem);
	FreeDDElParam(WM_DDE_UNADVISE, lParam);
	return 0;
    }

    /* callback shouldn't be invoked if CBF_FAIL_ADVISES is on. */
    if (thisInstance && thisInstance->callback != NULL &&
	!(thisInstance->CBFflags & CBF_SKIP_DISCONNECTS) /* && thisInstance->Process_id == GetCurrentProcessId() */)
    {
	TRACE("calling the Callback, idInst=%ld, CB=0x%lx, uType=0x%x\n",
	      thisInstance->instanceID, (DWORD)thisInstance->callback, XTYP_ADVSTOP);
	(thisInstance->callback)(XTYP_ADVSTOP, uiLo, (HCONV)pConv, pConv->hszTopic, 
				 hszItem, 0, 0, 0);
    }
	
    WDML_RemoveLink(thisInstance, (HCONV)pConv, WDML_SERVER_SIDE, hszItem, uiLo);
	
    /* send back ack */
    ddeAck.bAppReturnCode = 0;
    ddeAck.reserved       = 0;
    ddeAck.fBusy          = FALSE;
    ddeAck.fAck           = TRUE;
    
    PostMessageA(hwndClient, WM_DDE_ACK, (WPARAM)hwndServer,
		 ReuseDDElParam(lParam, WM_DDE_UNADVISE, WM_DDE_ACK, *((WORD*)&ddeAck), (UINT)hszItem));
	
    return 0;
}

/******************************************************************
 *		WDML_ServerHandleExecute
 *
 *
 */
static	LRESULT WDML_ServerHandleExecute(WDML_INSTANCE* thisInstance, WDML_CONV* pConv, 
					 HWND hwndServer, HWND hwndClient, LPARAM lParam)
{
    DDEACK		ddeAck;
    HDDEDATA		hDdeData;

    TRACE("(%04x %04x %08lx)!\n", hwndServer, hwndClient, lParam);
	
    if (hwndClient != pConv->hwndClient)
	WARN("hmmm source window (%04x)\n", hwndClient);

    hDdeData = 0;
    if (thisInstance && thisInstance->callback != NULL /* && thisInstance->Process_id == GetCurrentProcessId() */)
    {
	LPVOID	ptr = GlobalLock((HGLOBAL)lParam);
	
	if (ptr)
	{
	    hDdeData = DdeCreateDataHandle(0, ptr, GlobalSize((HGLOBAL)lParam),
					   0, 0, CF_TEXT, 0);
	    GlobalUnlock((HGLOBAL)lParam);
	}  
	TRACE("calling the Callback, idInst=%ld, CB=0x%lx, uType=0x%x\n",
	      thisInstance->instanceID, (DWORD)thisInstance->callback, XTYP_EXECUTE);
	hDdeData = (thisInstance->callback)(XTYP_EXECUTE, 0, (HCONV)pConv, pConv->hszTopic, 0, 
					    hDdeData, 0L, 0L);
    }
	
    ddeAck.bAppReturnCode = 0;
    ddeAck.reserved       = 0;
    ddeAck.fBusy          = FALSE;
    ddeAck.fAck           = FALSE;
    switch ((UINT)hDdeData)
    {
    case DDE_FACK:	
	ddeAck.fAck = TRUE;	
	break;
    case DDE_FBUSY:	
	ddeAck.fBusy = TRUE;	
	break;
    default:	
	WARN("Bad result code\n");
	/* fall thru */
    case DDE_FNOTPROCESSED:				
	break;
    }	
    PostMessageA(pConv->hwndClient, WM_DDE_ACK, (WPARAM)hwndServer,
		 PackDDElParam(WM_DDE_ACK, *((WORD*)&ddeAck), lParam));
	
    return 0;
}

/******************************************************************
 *		WDML_ServerHandlePoke
 *
 *
 */
static	LRESULT WDML_ServerHandlePoke(WDML_INSTANCE* thisInstance, WDML_CONV* pConv, 
				      HWND hwndServer, HWND hwndClient, LPARAM lParam)
{
    UINT		uiLo, uiHi;
    HSZ			hszItem;
    DDEACK		ddeAck;
    DDEPOKE*		pDdePoke;
    HDDEDATA		hDdeData;

    TRACE("(%04x %04x %08lx)!\n", hwndServer, hwndClient, lParam);
	
    UnpackDDElParam(WM_DDE_UNADVISE, lParam, &uiLo, &uiHi);
    hszItem = (HSZ)uiHi;

    pDdePoke = (DDEPOKE*)GlobalLock((HGLOBAL)uiLo);
    if (!pDdePoke)
    {
	return 0;
    }

    ddeAck.bAppReturnCode = 0;
    ddeAck.reserved       = 0;
    ddeAck.fBusy          = FALSE;
    ddeAck.fAck           = FALSE;
    if (thisInstance && thisInstance->callback != NULL)
    {
	hDdeData = DdeCreateDataHandle(thisInstance->instanceID, pDdePoke->Value, 
				       GlobalSize((HGLOBAL)uiLo) - sizeof(DDEPOKE) + 1, 
				       0, 0, pDdePoke->cfFormat, 0);
	if (hDdeData) 
	{
	    HDDEDATA	hDdeDataOut;
	    
	    TRACE("calling callback XTYP_POKE, idInst=%ld\n", 
		  thisInstance->instanceID);
	    hDdeDataOut = (thisInstance->callback)(XTYP_POKE,
						   pDdePoke->cfFormat, (HCONV)pConv,
						   pConv->hszTopic, (HSZ)uiHi, 
						   hDdeData, 0, 0);
	    switch ((UINT)hDdeDataOut) 
	    {
	    case DDE_FACK:
		ddeAck.fAck = TRUE;	
		break;
	    case DDE_FBUSY:
		ddeAck.fBusy = TRUE;
		break;
	    default:
		FIXME("Unsupported returned value %08lx\n", (DWORD)hDdeDataOut);
		/* fal thru */
	    case DDE_FNOTPROCESSED:				
		break;
	    }
	    DdeFreeDataHandle(hDdeData);
	}
    }
    GlobalUnlock((HGLOBAL)uiLo);
    
    if (!ddeAck.fAck)
	GlobalFree((HGLOBAL)uiHi);
    
    PostMessageA(hwndClient, WM_DDE_ACK, (WPARAM)hwndServer,
		 ReuseDDElParam(lParam, WM_DDE_POKE, WM_DDE_ACK, *((WORD*)&ddeAck), (UINT)hszItem));

    return 0L;
}

/******************************************************************
 *		WDML_ServerHandleTerminate
 *
 *
 */
static	LRESULT WDML_ServerHandleTerminate(WDML_INSTANCE* thisInstance, WDML_CONV* pConv, 
					   HWND hwndServer, HWND hwndClient, LPARAM lParam)
{
    UINT		uiLo, uiHi;
    HSZ			hszApp, hszTop;

    TRACE("(%04x %04x %08lx)!\n", hwndServer, hwndClient, lParam);
	
    TRACE("WM_DDE_TERMINATE!\n");
    /* XTYP_DISCONNECT transaction */
    /* billx: two things to remove: the conv, and associated links.
       callback shouldn't be called if CBF_SKIP_DISCONNECTS is on.
       Respond with another WM_DDE_TERMINATE iMsg.*/
    
    /* don't free DDEParams, since this is a broadcast */
    UnpackDDElParam(WM_DDE_TERMINATE, lParam, &uiLo, &uiHi);	
	
    hszApp = (HSZ)uiLo;
    hszTop = (HSZ)uiHi;
	
    WDML_BroadcastDDEWindows(WDML_szEventClass, WM_WDML_UNREGISTER, hszApp, hszTop);
    
    /* PostMessageA(hwndClient, WM_DDE_TERMINATE, (WPARAM)hwndServer, (LPARAM)hConv); */
    WDML_RemoveAllLinks(thisInstance, (HCONV)pConv, WDML_SERVER_SIDE);
    WDML_RemoveConv(thisInstance, WDML_SERVER_SIDE, (HCONV)pConv);
    /* DestroyWindow(hwnd); don't destroy it now, we may still need it. */
	
    return 0;
}

/******************************************************************
 *		WDML_ServerConvProc
 *
 *
 */
static LRESULT CALLBACK WDML_ServerConvProc(HWND hwndServer, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    WDML_INSTANCE*	thisInstance;
    WDML_CONV*		pConv;

    if (iMsg < WM_DDE_FIRST || iMsg > WM_DDE_LAST)
    {
	return DefWindowProcA(hwndServer, iMsg, wParam, lParam);
    }

    TRACE("About to wait... \n");

    if (!WDML_WaitForMutex(handle_mutex))
    {
	return 0;
    }

    thisInstance = (WDML_INSTANCE*)GetWindowLongA(hwndServer, 0);
    pConv = (WDML_CONV*)GetWindowLongA(hwndServer, 4);
    
    switch (iMsg)
    {
    case WM_DDE_INITIATE:
	FIXME("WM_DDE_INITIATE message received in the ServerConv Proc!\n");
	break;
	
    case WM_DDE_REQUEST:
	WDML_ServerHandleRequest(thisInstance, pConv, hwndServer, (HWND)wParam, lParam);
	break;
		
    case WM_DDE_ADVISE:
	WDML_ServerHandleAdvise(thisInstance, pConv, hwndServer, (HWND)wParam, lParam);
	break;
	
    case WM_DDE_UNADVISE:
	WDML_ServerHandleUnadvise(thisInstance, pConv, hwndServer, (HWND)wParam, lParam);
	break;
	
    case WM_DDE_EXECUTE:
	WDML_ServerHandleExecute(thisInstance, pConv, hwndServer, (HWND)wParam, lParam);
	break;
	
    case WM_DDE_POKE:
	WDML_ServerHandlePoke(thisInstance, pConv, hwndServer, (HWND)wParam, lParam);
	break;
	
    case WM_DDE_TERMINATE:
	WDML_ServerHandleTerminate(thisInstance, pConv, hwndServer, (HWND)wParam, lParam);
	break;

    case WM_DDE_ACK:
	WARN("Shouldn't receive a ACK message (never requests them). Ignoring it\n");
	break;

    default:
	FIXME("Unsupported message %d\n", iMsg);
    }

    WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
    
    return 0;
}
