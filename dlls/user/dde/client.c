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

static LRESULT CALLBACK WDML_ClientProc(HWND, UINT, WPARAM, LPARAM);	/* only for one client, not conv list */
static const char szClientClassA[] = "DdeClientAnsi";

/******************************************************************************
 * DdeConnectList [USER32.@]  Establishes conversation with DDE servers
 *
 * PARAMS
 *    idInst     [I] Instance identifier
 *    hszService [I] Handle to service name string
 *    hszTopic   [I] Handle to topic name string
 *    hConvList  [I] Handle to conversation list
 *    pCC        [I] Pointer to structure with context data
 *
 * RETURNS
 *    Success: Handle to new conversation list
 *    Failure: 0
 */
HCONVLIST WINAPI DdeConnectList(DWORD idInst, HSZ hszService, HSZ hszTopic,
				HCONVLIST hConvList, LPCONVCONTEXT pCC)
{
    FIXME("(%ld,%d,%d,%d,%p): stub\n", idInst, hszService, hszTopic,
	  hConvList,pCC);
    return (HCONVLIST)1;
}

/*****************************************************************
 * DdeQueryNextServer [USER32.@]
 */
HCONV WINAPI DdeQueryNextServer(HCONVLIST hConvList, HCONV hConvPrev)
{
    FIXME("(%d,%d): stub\n",hConvList,hConvPrev);
    return 0;
}

/******************************************************************************
 * DdeDisconnectList [USER32.@]  Destroys list and terminates conversations
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI DdeDisconnectList(
    HCONVLIST hConvList) /* [in] Handle to conversation list */
{
    FIXME("(%d): stub\n", hConvList);
    return TRUE;
}

/*****************************************************************
 *            DdeConnect   (USER32.@)
 */
HCONV WINAPI DdeConnect(DWORD idInst, HSZ hszService, HSZ hszTopic,
			LPCONVCONTEXT pCC)
{
    HWND		hwndClient;
    LPARAM		lParam = 0;
    UINT		uiLow, uiHi;
    WNDCLASSEXA	wndclass;
    WDML_INSTANCE*	thisInstance;
    WDML_CONV*		pConv;
    
    TRACE("(0x%lx,%d,%d,%p)\n",idInst,hszService,hszTopic,pCC);
    
    thisInstance = WDML_FindInstance(idInst);
    if (!thisInstance)
    {
	return 0;
    }
    
    /* make sure this conv is never created */
    pConv = WDML_FindConv(thisInstance, WDML_CLIENT_SIDE, hszService, hszTopic);
    if (pConv != NULL)
    {
	ERR("This Conv already exists: (0x%lx)\n", (DWORD)pConv);
	return (HCONV)pConv;
    }
    
    /* we need to establish a conversation with
       server, so create a window for it       */
    
    wndclass.cbSize        = sizeof(wndclass);
    wndclass.style         = 0;
    wndclass.lpfnWndProc   = WDML_ClientProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 2 * sizeof(DWORD);
    wndclass.hInstance     = 0;
    wndclass.hIcon         = 0;
    wndclass.hCursor       = 0;
    wndclass.hbrBackground = 0;
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = szClientClassA;
    wndclass.hIconSm       = 0;
    
    RegisterClassExA(&wndclass);
    
    hwndClient = CreateWindowA(szClientClassA, NULL, WS_POPUP, 0, 0, 0, 0, 0, 0, 0, 0);
    
    SetWindowLongA(hwndClient, 0, (DWORD)thisInstance);
    
    SendMessageA(HWND_BROADCAST, WM_DDE_INITIATE, (WPARAM)hwndClient,
		 PackDDElParam(WM_DDE_INITIATE, (UINT)hszService, (UINT)hszTopic));
    
    if (UnpackDDElParam(WM_DDE_INITIATE, lParam, &uiLow, &uiHi))
	FreeDDElParam(WM_DDE_INITIATE, lParam);
    
    TRACE("WM_DDE_INITIATE was processed\n");
    /* At this point, Client WM_DDE_ACK should have saved hwndServer
       for this instance id and hwndClient if server responds.
       So get HCONV and return it. And add it to conv list */
    pConv = (WDML_CONV*)GetWindowLongA(hwndClient, 4);
    if (pConv == NULL || pConv->hwndServer == 0)
    {
	ERR(".. but no Server window available\n");
	return 0;
    }
    /* finish init of pConv */
    if (pCC != NULL)
    {
	pConv->convContext = *pCC;
    }
    
    return (HCONV)pConv;
}

/*****************************************************************
 *            DdeDisconnect   (USER32.@)
 */
BOOL WINAPI DdeDisconnect(HCONV hConv)
{
    WDML_CONV*	pConv = NULL;
    
    TRACE("(%ld)\n", (DWORD)hConv);
    
    if (hConv == 0)
    {
	ERR("DdeDisconnect(): hConv = 0\n");
	return 0;
    }
    
    pConv = WDML_GetConv(hConv);
    if (pConv == NULL)
    {
	return FALSE;
    }
    if (!PostMessageA(pConv->hwndServer, WM_DDE_TERMINATE,
		      (WPARAM)pConv->hwndClient, (LPARAM)hConv))
    {
	ERR("DdeDisconnect(): PostMessage returned 0\n");
	return 0;
    }
    return TRUE;
}


/*****************************************************************
 *            DdeReconnect   (DDEML.37)
 *            DdeReconnect   (USER32.@)
 */
HCONV WINAPI DdeReconnect(HCONV hConv)
{
    FIXME("empty stub\n");
    return 0;
}

typedef enum {
    WDML_QS_ERROR, WDML_QS_HANDLED, WDML_QS_PASS
} WDML_QUEUE_STATE;

/******************************************************************
 *		WDML_QueueAdvise
 *
 * Creates and queue an WM_DDE_ADVISE transaction
 */
static WDML_XACT*	WDML_QueueAdvise(WDML_CONV* pConv, UINT wType, UINT wFmt, HSZ hszItem)
{
    DDEADVISE*		pDdeAdvise;
    WDML_XACT*		pXAct;

    TRACE("XTYP_ADVSTART (with%s data) transaction\n", (wType & XTYPF_NODATA) ? "out" : "");

    pXAct = WDML_AllocTransaction(pConv->thisInstance, WM_DDE_ADVISE);
    if (!pXAct)
	return NULL;

    pXAct->u.advise.wType = wType & ~0x0F;
    pXAct->u.advise.wFmt = wFmt;
    pXAct->u.advise.hszItem = hszItem;
    pXAct->u.advise.hDdeAdvise = GlobalAlloc(GHND | GMEM_DDESHARE, sizeof(DDEADVISE));
    
    /* pack DdeAdvise	*/
    pDdeAdvise = (DDEADVISE*)GlobalLock(pXAct->u.advise.hDdeAdvise);
    pDdeAdvise->fAckReq   = (wType & XTYPF_ACKREQ) ? TRUE : FALSE;
    pDdeAdvise->fDeferUpd = (wType & XTYPF_NODATA) ? TRUE : FALSE;
    pDdeAdvise->cfFormat  = wFmt;
    GlobalUnlock(pXAct->u.advise.hDdeAdvise);

    WDML_QueueTransaction(pConv, pXAct);

    if (!PostMessageA(pConv->hwndServer, WM_DDE_ADVISE, (WPARAM)pConv->hwndClient,
		      PackDDElParam(WM_DDE_ADVISE, (UINT)pXAct->u.advise.hDdeAdvise, (UINT)hszItem)))
    {
	GlobalFree(pXAct->u.advise.hDdeAdvise);
	WDML_UnQueueTransaction(pConv, pXAct);
	WDML_FreeTransaction(pXAct);
	return NULL;
    }

    return pXAct;
}

/******************************************************************
 *		WDML_HandleAdviseReply
 *
 * handles the reply to an advise request
 */
static WDML_QUEUE_STATE WDML_HandleAdviseReply(WDML_CONV* pConv, MSG* msg, WDML_XACT* pXAct)
{
    DDEACK		ddeAck;
    UINT		uiLo, uiHi;
    WORD		wStatus;
    
    if (msg->message != WM_DDE_ACK || msg->wParam != pConv->hwndServer)
    {
	return WDML_QS_PASS;
    }

    UnpackDDElParam(WM_DDE_ACK, msg->lParam, &uiLo, &uiHi);

    if (DdeCmpStringHandles(uiHi, pXAct->u.advise.hszItem) != 0)
	return WDML_QS_PASS;

    GlobalDeleteAtom(uiHi);

    wStatus = uiLo;
    ddeAck = *((DDEACK*)&wStatus);
	    
    if (ddeAck.fAck)
    {
	WDML_LINK*	pLink;
	
	/* billx: first to see if the link is already created. */
	pLink = WDML_FindLink(pConv->thisInstance, (HCONV)pConv, WDML_CLIENT_SIDE, 
			      pXAct->u.advise.hszItem, pXAct->u.advise.wFmt);
	if (pLink != NULL)	
	{	
	    /* we found a link, and only need to modify it in case it changes */
	    pLink->transactionType = pXAct->u.advise.wType;
	}
	else
	{
	    TRACE("Adding Link with hConv = 0x%lx\n", (DWORD)pConv);
	    WDML_AddLink(pConv->thisInstance, (HCONV)pConv, WDML_CLIENT_SIDE, 
			 pXAct->u.advise.wType, pXAct->u.advise.hszItem, 
			 pXAct->u.advise.wFmt);
	}
    }
    else
    {
	TRACE("Returning TRUE on XTYP_ADVSTART - fAck was FALSE\n");
	GlobalFree(pXAct->u.advise.hDdeAdvise);
    }
    pXAct->hDdeData = (HDDEDATA)1;
    return WDML_QS_HANDLED;
}

/******************************************************************
 *		WDML_QueueUnadvise
 *
 * queues an unadvise transaction
 */
static WDML_XACT*	WDML_QueueUnadvise(WDML_CONV* pConv, UINT wFmt, HSZ hszItem)
{
    WDML_XACT*	pXAct;
    
    TRACE("XTYP_ADVSTOP transaction\n");

    pXAct = WDML_AllocTransaction(pConv->thisInstance, WM_DDE_UNADVISE);
    if (!pXAct)
	return NULL;

    pXAct->u.unadvise.wFmt = wFmt;
    pXAct->u.unadvise.hszItem = hszItem;

    WDML_QueueTransaction(pConv, pXAct);

   /* end advise loop: post WM_DDE_UNADVISE to server to terminate link
       on the specified item. */
	    
    if (!PostMessageA(pConv->hwndServer, WM_DDE_UNADVISE, (WPARAM)pConv->hwndClient,
		      PackDDElParam(WM_DDE_UNADVISE, wFmt, (UINT)hszItem)))
    {
	WDML_UnQueueTransaction(pConv, pXAct);
	WDML_FreeTransaction(pXAct);
	return NULL;
    }
    return pXAct;
}
    
/******************************************************************
 *		WDML_HandleUnadviseReply
 *
 *
 */
static WDML_QUEUE_STATE WDML_HandleUnadviseReply(WDML_CONV* pConv, MSG* msg, WDML_XACT* pXAct)
{
    DDEACK	ddeAck;
    UINT	uiLo, uiHi;
    WORD	wStatus;

    if (msg->message != WM_DDE_ACK || msg->wParam != pConv->hwndServer)
    {
	return WDML_QS_PASS;
    }

    UnpackDDElParam(WM_DDE_ACK, msg->lParam, &uiLo, &uiHi);

    if (DdeCmpStringHandles(uiHi, pXAct->u.unadvise.hszItem) != 0)
	return WDML_QS_PASS;

    GlobalDeleteAtom(uiHi);
		    
    wStatus = uiLo;
    ddeAck = *((DDEACK*)&wStatus);
		    
    TRACE("WM_DDE_ACK received while waiting for a timeout\n");
	    
    if (!ddeAck.fAck)
    {
	TRACE("Returning TRUE on XTYP_ADVSTOP - fAck was FALSE\n");
    }
    else
    {
	/* billx: remove the link */
	WDML_RemoveLink(pConv->thisInstance, (HCONV)pConv, WDML_CLIENT_SIDE, 
			pXAct->u.unadvise.hszItem, pXAct->u.unadvise.wFmt);
    }
    pXAct->hDdeData = (HDDEDATA)1;
    return WDML_QS_HANDLED;
}

/******************************************************************
 *		WDML_QueueRequest
 *
 *
 */
static WDML_XACT*	WDML_QueueRequest(WDML_CONV* pConv, UINT wFmt, HSZ hszItem)
{
    WDML_XACT*	pXAct;
    
    TRACE("XTYP_REQUEST transaction\n");

    pXAct = WDML_AllocTransaction(pConv->thisInstance, WM_DDE_REQUEST);
    if (!pXAct)
	return NULL;

    pXAct->u.request.hszItem = hszItem;

    WDML_QueueTransaction(pConv, pXAct);

   /* end advise loop: post WM_DDE_UNADVISE to server to terminate link
    * on the specified item. 
    */
	    
    if (!PostMessageA(pConv->hwndServer, WM_DDE_REQUEST, (WPARAM)pConv->hwndClient,
		      PackDDElParam(WM_DDE_REQUEST, wFmt, (UINT)hszItem)))
    {
	WDML_UnQueueTransaction(pConv, pXAct);
	WDML_FreeTransaction(pXAct);
	return NULL;
    }
    return pXAct;
}

/******************************************************************
 *		WDML_HandleRequestReply
 *
 *
 */
static WDML_QUEUE_STATE WDML_HandleRequestReply(WDML_CONV* pConv, MSG* msg, WDML_XACT* pXAct)
{
    DDEACK	ddeAck;
    UINT	uiLo, uiHi;
    WORD	wStatus;

    switch (msg->message)
    {
    case WM_DDE_ACK:
	if (msg->wParam != pConv->hwndServer)
	    return WDML_QS_PASS;
	UnpackDDElParam(WM_DDE_ACK, msg->lParam, &uiLo, &uiHi);
	wStatus = uiLo;
	ddeAck = *((DDEACK*)&wStatus);
	pXAct->hDdeData = 0;
	TRACE("Negative answer...\n");
		
	/* FIXME: billx: we should return 0 and post a negatve WM_DDE_ACK. */
	break;

    case WM_DDE_DATA:
	if (msg->wParam != pConv->hwndServer)
	    return WDML_QS_PASS;
	UnpackDDElParam(WM_DDE_DATA, msg->lParam, &uiLo, &uiHi);
	TRACE("Got the result (%08lx)\n", (DWORD)uiLo);
	if (DdeCmpStringHandles(uiHi, pXAct->u.request.hszItem) != 0)
	    return WDML_QS_PASS;
	/* FIXME: memory clean up ? */
	pXAct->hDdeData = WDML_Global2DataHandle((HGLOBAL)uiLo);
	break;

    default:
	return WDML_QS_PASS;
    }

    return WDML_QS_HANDLED;
}	

/******************************************************************
 *		WDML_QueueExecute
 *
 *
 */
static WDML_XACT*	WDML_QueueExecute(WDML_CONV* pConv, LPCVOID pData, DWORD cbData)
{
    WDML_XACT*	pXAct;

    TRACE("XTYP_EXECUTE transaction\n");
	
    pXAct = WDML_AllocTransaction(pConv->thisInstance, WM_DDE_EXECUTE);
    if (!pXAct)
	return NULL;

    if (cbData == (DWORD)-1)
    {
	HDDEDATA		hDdeData = (HDDEDATA)pData;
	DWORD			dwSize;
    
	pData = DdeAccessData(hDdeData, &dwSize);
	if (pData)
	{
	    pXAct->u.execute.hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, dwSize);
	    if (pXAct->u.execute.hMem)
	    {
		LPBYTE	pDst;
	    
		pDst = GlobalLock(pXAct->u.execute.hMem);
		if (pDst)
		{
		    memcpy(pDst, pData, dwSize);
		    GlobalUnlock(pXAct->u.execute.hMem);
		}
		else
		{
		    GlobalFree(pXAct->u.execute.hMem);
		    pXAct->u.execute.hMem = 0;
		}
	    }
	    DdeUnaccessData(hDdeData);
	}
	else
	{
	    pXAct->u.execute.hMem = 0;
	}
    }
    else
    {
	LPSTR	ptr;

	pXAct->u.execute.hMem = GlobalAlloc(GHND | GMEM_DDESHARE, cbData);
	ptr = GlobalLock(pXAct->u.execute.hMem);
	if (ptr) 
	{
	    memcpy(ptr, pData, cbData);
	    GlobalUnlock(pXAct->u.execute.hMem);
	}
    }

    WDML_QueueTransaction(pConv, pXAct);
	
    if (!PostMessageA(pConv->hwndServer, WM_DDE_EXECUTE, (WPARAM)pConv->hwndClient, 
		      pXAct->u.execute.hMem))
    {
	GlobalFree(pXAct->u.execute.hMem);
	WDML_UnQueueTransaction(pConv, pXAct);
	WDML_FreeTransaction(pXAct);
	TRACE("Returning FALSE on XTYP_EXECUTE - PostMessage returned FALSE\n");
	return NULL;
    }
    return pXAct;
}

/******************************************************************
 *		WDML_HandleExecuteReply
 *
 *
 */
static WDML_QUEUE_STATE WDML_HandleExecuteReply(WDML_CONV* pConv, MSG* msg, WDML_XACT* pXAct)
{
    DDEACK	ddeAck;
    UINT	uiLo, uiHi;
    WORD	wStatus;

    if (msg->message != WM_DDE_ACK || msg->wParam != pConv->hwndServer)
    {
	return WDML_QS_PASS;
    }

    UnpackDDElParam(WM_DDE_ACK, msg->lParam, &uiLo, &uiHi);
    FreeDDElParam(WM_DDE_ACK, msg->lParam);

    if (uiHi != pXAct->u.execute.hMem)
    {
	return WDML_QS_PASS;
    }

    wStatus = uiLo;
    ddeAck = *((DDEACK*)&wStatus);
    if (!ddeAck.fAck)
    {
	GlobalFree(pXAct->u.execute.hMem);
    }
    pXAct->hDdeData = (HDDEDATA)1;
    return WDML_QS_HANDLED;
}

/******************************************************************
 *		WDML_QueuePoke
 *
 *
 */
static WDML_XACT*	WDML_QueuePoke(WDML_CONV* pConv, LPCVOID pData, DWORD cbData, 
				       UINT wFmt, HSZ hszItem)
{
    WDML_XACT*	pXAct;

    TRACE("XTYP_POKE transaction\n");
	
    pXAct = WDML_AllocTransaction(pConv->thisInstance, WM_DDE_POKE);
    if (!pXAct)
	return NULL;

    if (cbData == (DWORD)-1)
    {
	pXAct->u.poke.hMem = (HDDEDATA)pData;
    }
    else
    {
	DDEPOKE*	ddePoke;

	pXAct->u.poke.hMem = GlobalAlloc(GHND | GMEM_DDESHARE, sizeof(DDEPOKE) + cbData);
	ddePoke = GlobalLock(pXAct->u.poke.hMem);
	if (ddePoke) 
	{
	    memcpy(ddePoke->Value, pData, cbData);
	    ddePoke->fRelease = FALSE; /* FIXME: app owned ? */
	    ddePoke->cfFormat = wFmt;
	    GlobalUnlock(pXAct->u.poke.hMem);
	}
    }

    pXAct->u.poke.hszItem = hszItem;

    WDML_QueueTransaction(pConv, pXAct);
	
    if (!PostMessageA(pConv->hwndServer, WM_DDE_POKE, (WPARAM)pConv->hwndClient, 
		      PackDDElParam(WM_DDE_POKE, pXAct->u.execute.hMem, hszItem)))
    {
	GlobalFree(pXAct->u.execute.hMem);
	WDML_UnQueueTransaction(pConv, pXAct);
	WDML_FreeTransaction(pXAct);
	TRACE("Returning FALSE on XTYP_POKE - PostMessage returned FALSE\n");
	return NULL;
    }
    return pXAct;
}

/******************************************************************
 *		WDML_HandlePokeReply
 *
 *
 */
static WDML_QUEUE_STATE WDML_HandlePokeReply(WDML_CONV* pConv, MSG* msg, WDML_XACT* pXAct)
{
    DDEACK	ddeAck;
    UINT	uiLo, uiHi;
    WORD	wStatus;

    if (msg->message != WM_DDE_ACK && msg->wParam != pConv->hwndServer)
    {
	return WDML_QS_PASS;
    }

    UnpackDDElParam(WM_DDE_ACK, msg->lParam, &uiLo, &uiHi);
    if (uiHi != pXAct->u.poke.hszItem)
    {
	return WDML_QS_PASS;
    }
    FreeDDElParam(WM_DDE_ACK, msg->lParam);

    wStatus = uiLo;
    ddeAck = *((DDEACK*)&wStatus);
    if (!ddeAck.fAck)
    {
	GlobalFree(pXAct->u.poke.hMem);
    }
    pXAct->hDdeData = (HDDEDATA)TRUE;
    return TRUE;
}

/******************************************************************
 *		WDML_HandleReplyData
 *
 *
 */
static WDML_QUEUE_STATE WDML_HandleReplyData(WDML_CONV* pConv, MSG* msg, HDDEDATA* hdd)
{
    UINT	uiLo, uiHi;
    HDDEDATA	hDdeDataIn, hDdeDataOut;
    WDML_LINK*	pLink;

    TRACE("WM_DDE_DATA message received in the Client Proc!\n");
    /* wParam -- sending window handle	*/
    /* lParam -- hDdeData & item HSZ	*/
	
    UnpackDDElParam(WM_DDE_DATA, msg->lParam, &uiLo, &uiHi);
	
    hDdeDataIn = WDML_Global2DataHandle((HGLOBAL)uiLo);

    /* billx: 
     *  For hot link, data should be passed to its callback with
     * XTYP_ADVDATA and callback should return the proper status.
     */
	
    for (pLink = pConv->thisInstance->links[WDML_CLIENT_SIDE]; pLink != NULL; pLink = pLink->next)
    {
	if (DdeCmpStringHandles((HSZ)uiHi, pLink->hszItem) == 0)
	{
	    BOOL	fRelease = FALSE;
	    BOOL	fAckReq = FALSE;
	    DDEDATA*	pDdeData;

	    /* item in the advise loop */
	    pConv = WDML_GetConv(pLink->hConv);
	    if (pConv == NULL)
	    {
		continue;
	    }
	    if ((pDdeData = GlobalLock(uiLo)) != NULL)
	    {
		fRelease = pDdeData->fRelease;
		fAckReq = pDdeData->fAckReq;
	    }

	    if (hDdeDataIn != 0)
	    {
		if (fAckReq)
		{
		    DDEACK	ddeAck;
		    
		    ddeAck.bAppReturnCode = 0;
		    ddeAck.reserved       = 0;
		    ddeAck.fBusy          = FALSE;
		    ddeAck.fAck           = TRUE;
		    
		    if (msg->lParam) {
			PostMessageA(pConv->hwndServer, WM_DDE_ACK, pConv->hwndClient, 
				     ReuseDDElParam(msg->lParam, WM_DDE_DATA, WM_DDE_ACK, 
						    *(WORD*)&ddeAck, (UINT)pLink->hszItem));
			msg->lParam = 0L;
		    }
		    else 
		    {
			PostMessageA(pConv->hwndServer, WM_DDE_ACK, pConv->hwndClient, 
				     PackDDElParam(WM_DDE_ACK, *(WORD*)&ddeAck, (UINT)pLink->hszItem));
		    }	
		}
	    }
	    hDdeDataOut = 0;
	    if (pConv->thisInstance->callback != NULL /*&& thisInstance->processID == GetCurrentProcessId() */)
	    {
		TRACE("Calling the callback, type = XTYP_ADVDATA, CB = 0x%lx, hConv = 0x%lx\n", 
		      (DWORD)pConv->thisInstance->callback, (DWORD)pLink->hConv);
		hDdeDataOut = (pConv->thisInstance->callback)(XTYP_ADVDATA,
							      pLink->uFmt,
							      pLink->hConv,
							      pConv->hszTopic,
							      pLink->hszItem,
							      hDdeDataIn, 
							      0, 0);
		if (hDdeDataOut == (HDDEDATA)DDE_FACK)
		{
		    pLink->hDdeData = hDdeDataIn;
		}
	    }
#if 0
	    if (fRelease) 
	    {
		DdeFreeDataHandle(hDdeDataIn);
	    }
#endif
	    break;
	}
    }
    
    if (msg->lParam)
	FreeDDElParam(WM_DDE_DATA, msg->lParam);
	
    return WDML_QS_HANDLED;
}

/******************************************************************
 *		WDML_HandleReplyTerminate
 *
 *
 */
static WDML_QUEUE_STATE WDML_HandleReplyTerminate(WDML_CONV* pConv, MSG* msg, HDDEDATA* hdd)
{
    if ((LPARAM)pConv != msg->lParam)
	return WDML_QS_PASS;

    /* billx: clean up the conv and associated links */
    WDML_RemoveAllLinks(pConv->thisInstance, (HCONV)pConv, WDML_CLIENT_SIDE);
    WDML_RemoveConv(pConv->thisInstance, WDML_CLIENT_SIDE, (HCONV)pConv);
    DestroyWindow(msg->hwnd);
    return WDML_QS_HANDLED;
}

/******************************************************************
 *		WDML_HandleReply
 *
 * handles any incoming reply, and try to match to an already sent request
 */
static WDML_QUEUE_STATE	WDML_HandleReply(WDML_CONV* pConv, MSG* msg, HDDEDATA* hdd)
{
    WDML_XACT*	pXAct = pConv->transactions;
    WDML_QUEUE_STATE	qs;

    if (pConv->transactions) 
    {
	/* first check message against a pending transaction, if any */
	switch (pXAct->ddeMsg)
	{
	case WM_DDE_ADVISE:
	    qs = WDML_HandleAdviseReply(pConv, msg, pXAct);
	    break;
	case WM_DDE_UNADVISE:
	    qs = WDML_HandleUnadviseReply(pConv, msg, pXAct);
	    break;
	case WM_DDE_EXECUTE:
	    qs = WDML_HandleExecuteReply(pConv, msg, pXAct);
	    break;
	case WM_DDE_REQUEST:
	    qs = WDML_HandleRequestReply(pConv, msg, pXAct);
	    break;
	case WM_DDE_POKE:
	    qs = WDML_HandlePokeReply(pConv, msg, pXAct);
	    break;
	default:
	    qs = WDML_QS_ERROR;
	    FIXME("oooch\n");
	}
    }
    else
    {
	qs = WDML_QS_PASS;
    }

    /* now check the results */
    switch (qs) 
    {
    case WDML_QS_ERROR:
	*hdd = 0;
	break;
    case WDML_QS_HANDLED:
	/* ok, we have resolved a pending transaction
	 * notify callback if asynchronous, and remove it in any case
	 */
	WDML_UnQueueTransaction(pConv, pXAct);
	if (pXAct->dwTimeout == TIMEOUT_ASYNC)
	{
	    if (pConv->thisInstance->callback != NULL /*&& thisInstance->processID == GetCurrentProcessId() */)
	    {
		TRACE("Calling the callback, type = XTYP_XACT_COMPLETE, CB = 0x%lx, hConv = 0x%lx\n",
		      (DWORD)pConv->thisInstance->callback, (DWORD)pConv);
		(pConv->thisInstance->callback)(XTYP_XACT_COMPLETE, 0 /* FIXME */,
						(HCONV)pConv,
						pConv->hszTopic, 0 /* FIXME */,
						pXAct->hDdeData, 
						MAKELONG(0, pXAct->xActID), 
						0 /* FIXME */);
		qs = WDML_QS_PASS;
	    }
	}	
	else
	{
	    *hdd = pXAct->hDdeData;
	}
	WDML_FreeTransaction(pXAct);
	break;
    case WDML_QS_PASS:
	/* no pending transaction found, try a warm link or a termination request */
	switch (msg->message)
	{
	case WM_DDE_DATA:
	    qs = WDML_HandleReplyData(pConv, msg, hdd);
	    break;
	case WM_DDE_TERMINATE:
	    qs = WDML_HandleReplyTerminate(pConv, msg, hdd);
	    break;
	}
	break;
    }

    return qs;
}

/******************************************************************
 *		WDML_SyncWaitTransactionReply
 *
 * waits until an answer for a sent request is received
 * time out is also handled. only used for synchronous transactions
 */
static HDDEDATA WDML_SyncWaitTransactionReply(HCONV hConv, DWORD dwTimeout, WDML_XACT* pXAct)
{
    DWORD		dwTime;

    TRACE("Starting wait for a timeout of %ld ms\n", dwTimeout);

    /* FIXME: time 32 bit wrap around */
    dwTimeout += GetCurrentTime();
	    
    while ((dwTime = GetCurrentTime()) < dwTimeout)
    {
	/* we cannot hold the mutex all the time because when client and server run in a
	 * single process they need to share the access to the internal data
	 */
	if (MsgWaitForMultipleObjects(0, NULL, FALSE, 
				      dwTime - dwTimeout, QS_POSTMESSAGE) == WAIT_OBJECT_0 &&
	    WDML_WaitForMutex(handle_mutex))
	{
	    BOOL	ret = FALSE;
	    MSG		msg;
	    WDML_CONV*	pConv;
	    HDDEDATA	hdd;
	    
	    pConv = WDML_GetConv(hConv);
	    if (pConv == NULL)
	    {
		/* conversation no longer available... return failure */
		break;
	    }
	    while (PeekMessageA(&msg, pConv->hwndClient, WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE))
	    {
		/* check that either pXAct has been processed or no more xActions are pending */
		ret = (pConv->transactions == pXAct);
		ret = WDML_HandleReply(pConv, &msg, &hdd) == WDML_QS_HANDLED && 
		    (pConv->transactions == NULL || ret);
		if (ret) break;
	    }
	    WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
	    if (ret)
	    {
		return hdd;
	    }
	}
    }

    TRACE("Timeout !!\n");
    if (WDML_WaitForMutex(handle_mutex))
    {
	DWORD		err;
	WDML_CONV*	pConv;

	pConv = WDML_GetConv(hConv);
	if (pConv == NULL)
	{
	    return 0;
	} 
	switch (pConv->transactions->ddeMsg)
	{
	case WM_DDE_ADVISE:	err = DMLERR_ADVACKTIMEOUT;	break;
	case WM_DDE_REQUEST:	err = DMLERR_DATAACKTIMEOUT; 	break;
	case WM_DDE_EXECUTE:	err = DMLERR_EXECACKTIMEOUT;	break;
	case WM_DDE_POKE:	err = DMLERR_POKEACKTIMEOUT;	break;
	case WM_DDE_UNADVISE:	err = DMLERR_UNADVACKTIMEOUT;	break;
	default:		err = DMLERR_INVALIDPARAMETER;	break;
	}

	pConv->thisInstance->lastError = err;
	WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
    }
    return 0;
}

/*****************************************************************
 *            DdeClientTransaction  (USER32.@)
 */
HDDEDATA WINAPI DdeClientTransaction(LPBYTE pData, DWORD cbData, HCONV hConv, HSZ hszItem, UINT wFmt,
				     UINT wType, DWORD dwTimeout, LPDWORD pdwResult)
{
    WDML_CONV*		pConv;
    WDML_XACT*		pXAct;
    HDDEDATA		hDdeData = 0;
    
    TRACE("(0x%lx,%ld,0x%lx,0x%lx,%d,%d,%ld,0x%lx)\n",
	  (ULONG)pData,cbData,(DWORD)hConv,(DWORD)hszItem,wFmt,wType,
	  dwTimeout,(ULONG)pdwResult);
    
    if (hConv == 0)
    {
	ERR("Invalid conversation handle\n");
	return 0;
    }
    
    if (!WDML_WaitForMutex(handle_mutex))
    {
	return FALSE;
    }

    pConv = WDML_GetConv(hConv);
    if (pConv == NULL)
    {
	/* cannot set error... cannot get back to DDE instance */
	goto theError;
    }

    switch (wType)
    {
    case XTYP_EXECUTE:
	if (hszItem != 0 || wFmt != 0)
	{
	    pConv->thisInstance->lastError = DMLERR_INVALIDPARAMETER;
	    goto theError;
	}
	pXAct = WDML_QueueExecute(pConv, pData, cbData);
	break;
    case XTYP_POKE:
	pXAct = WDML_QueuePoke(pConv, pData, cbData, wFmt, hszItem);
	break;
    case XTYP_ADVSTART|XTYPF_NODATA:
    case XTYP_ADVSTART|XTYPF_NODATA|XTYPF_ACKREQ:
    case XTYP_ADVSTART:
    case XTYP_ADVSTART|XTYPF_ACKREQ:
	if (pData)
	{
	    pConv->thisInstance->lastError = DMLERR_INVALIDPARAMETER;
	    goto theError;
	}
	pXAct = WDML_QueueAdvise(pConv, wType, wFmt, hszItem);
	break;
    case XTYP_ADVSTOP:
	if (pData)
	{
	    pConv->thisInstance->lastError = DMLERR_INVALIDPARAMETER;
	    goto theError;
	}
	pXAct = WDML_QueueUnadvise(pConv, wFmt, hszItem);
	break;
    case XTYP_REQUEST:
	if (pData)
	{
	    pConv->thisInstance->lastError = DMLERR_INVALIDPARAMETER;
	    goto theError;
	}
	pXAct = WDML_QueueRequest(pConv, wFmt, hszItem);
	break;
    default:
	FIXME("Unknown transation\n");
	/* unknown transaction type */
	pConv->thisInstance->lastError = DMLERR_INVALIDPARAMETER;
	goto theError;
    }

    pXAct->dwTimeout = dwTimeout;
    /* FIXME: should set the app bits on *pdwResult */
    
    if (dwTimeout == TIMEOUT_ASYNC)
    {
	if (pdwResult)
	{
	    *pdwResult = MAKELONG(0, pXAct->xActID);
	}
	hDdeData = (HDDEDATA)1;
    } 

    WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);

    if (dwTimeout != TIMEOUT_ASYNC)
    {
	DWORD	count = 0;

	if (pdwResult)
	{
	    *pdwResult = 0L;
	}
	while (ReleaseMutex(handle_mutex))
	    count++;
	hDdeData = WDML_SyncWaitTransactionReply((HCONV)pConv, dwTimeout, pXAct);
	while (count-- != 0)
	    WDML_WaitForMutex(handle_mutex);
    }
    return hDdeData;
 theError:
    WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
    return 0;
}

/******************************************************************
 *		WDML_ClientProc
 *
 * Window Proc created on client side for each conversation
 */
static LRESULT CALLBACK WDML_ClientProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    UINT		uiLow, uiHi;
    
    if (iMsg == WM_DDE_ACK &&
	/* In response to WM_DDE_INITIATE, save server window  */
	UnpackDDElParam(WM_DDE_ACK, lParam, &uiLow, &uiHi) &&
	(WDML_CONV*)GetWindowLongA(hwnd, 4) == NULL)
    {
	WDML_INSTANCE*	thisInstance = NULL;
	WDML_CONV*	pConv = NULL;

	FreeDDElParam(WM_DDE_ACK, lParam);
	/* no converstation yet, add it */
	thisInstance = (WDML_INSTANCE*)GetWindowLongA(hwnd, 0);
	pConv = WDML_AddConv(thisInstance, WDML_CLIENT_SIDE, (HSZ)uiLow, (HSZ)uiHi, 
			     hwnd, (HWND)wParam);
	SetWindowLongA(hwnd, 4, (DWORD)pConv);
	/* FIXME: so far we only use the first window in the list... */
	return 0;
    }
	
    if ((iMsg >= WM_DDE_FIRST && iMsg <= WM_DDE_LAST) && WDML_WaitForMutex(handle_mutex))
    {
	WDML_CONV*	pConv = (WDML_CONV*)GetWindowLongA(hwnd, 4);

	if (pConv) 
	{
	    MSG		msg;
	    HDDEDATA	hdd;

	    msg.hwnd = hwnd;
	    msg.message = iMsg;
	    msg.wParam = wParam;
	    msg.lParam = lParam;

	    WDML_HandleReply(pConv, &msg, &hdd);
	}

	WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
	return 0;
    }
	
    return DefWindowProcA(hwnd, iMsg, wParam, lParam);
}


/*****************************************************************
 *            DdeAbandonTransaction (USER32.@)
 */
BOOL WINAPI DdeAbandonTransaction(DWORD idInst, HCONV hConv, DWORD idTransaction)
{
    FIXME("empty stub\n");
    return TRUE;
}


/*****************************************************************
 *            DdeImpersonateClient (USER32.@)
 */
BOOL WINAPI DdeImpersonateClient(HCONV hConv)
{
    WDML_CONV*	pConv;
    BOOL	ret = FALSE;
    
    if (!WDML_WaitForMutex(handle_mutex))
    {
	return FALSE;
    }
    pConv = WDML_GetConv(hConv);
    if (pConv)
    {
	ret = ImpersonateDdeClientWindow(pConv->hwndClient, pConv->hwndServer);
    }
    WDML_ReleaseMutex(handle_mutex, "handle_mutex", FALSE);
    return ret;
}
