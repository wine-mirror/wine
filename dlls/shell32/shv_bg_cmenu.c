/*
 *	IContextMenu
 *	ShellView Background Context Menu (shv_bg_cm)
 *
 *	Copyright 1999	Juergen Schmied <juergen.schmied@metronet.de>
 */
#include <string.h>

#include "winerror.h"
#include "debugtools.h"

#include "pidl.h"
#include "wine/obj_base.h"
#include "wine/obj_contextmenu.h"
#include "wine/obj_shellbrowser.h"
#include "wine/obj_shellextinit.h"

#include "shell32_main.h"

DEFAULT_DEBUG_CHANNEL(shell)

/**************************************************************************
*  IContextMenu Implementation
*/
typedef struct 
{	ICOM_VTABLE(IContextMenu)* lpvtbl;
	DWORD		ref;
} BgCmImpl;


static struct ICOM_VTABLE(IContextMenu) cmvt;

/**************************************************************************
*   ISVBgCm_Constructor()
*/
IContextMenu *ISvBgCm_Constructor(void)
{
	BgCmImpl* cm;

	cm = (BgCmImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(BgCmImpl));
	cm->lpvtbl=&cmvt;
	cm->ref = 1;

	TRACE("(%p)->()\n",cm);
	shell32_ObjCount++;
	return (IContextMenu*)cm;
}

/**************************************************************************
*  ISVBgCm_fnQueryInterface
*/
static HRESULT WINAPI ISVBgCm_fnQueryInterface(IContextMenu *iface, REFIID riid, LPVOID *ppvObj)
{
	ICOM_THIS(BgCmImpl, iface);

	char    xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{
	  *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IContextMenu))  /*IContextMenu*/
	{
	  *ppvObj = This;
	}   
	else if(IsEqualIID(riid, &IID_IShellExtInit))  /*IShellExtInit*/
	{
	  FIXME("-- LPSHELLEXTINIT pointer requested\n");
	}

	if(*ppvObj)
	{ 
	  IUnknown_AddRef((IUnknown*)*ppvObj);      
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**************************************************************************
*  ISVBgCm_fnAddRef
*/
static ULONG WINAPI ISVBgCm_fnAddRef(IContextMenu *iface)
{
	ICOM_THIS(BgCmImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This, This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}

/**************************************************************************
*  ISVBgCm_fnRelease
*/
static ULONG WINAPI ISVBgCm_fnRelease(IContextMenu *iface)
{
	ICOM_THIS(BgCmImpl, iface);

	TRACE("(%p)->()\n",This);

	shell32_ObjCount--;

	if (!--(This->ref)) 
	{ TRACE(" destroying IContextMenu(%p)\n",This);

	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

/**************************************************************************
* ISVBgCm_fnQueryContextMenu()
*/

static HRESULT WINAPI ISVBgCm_fnQueryContextMenu(
	IContextMenu *iface,
	HMENU hMenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
	HMENU	hMyMenu;
	UINT	idMax;
	
	ICOM_THIS(BgCmImpl, iface);

	TRACE("(%p)->(hmenu=%x indexmenu=%x cmdfirst=%x cmdlast=%x flags=%x )\n",This, hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

	hMyMenu = LoadMenuA(shell32_hInstance, "MENU_002");

	idMax = Shell_MergeMenus (hMenu, GetSubMenu(hMyMenu,0), indexMenu, idCmdFirst, idCmdLast, MM_SUBMENUSHAVEIDS);

	DestroyMenu(hMyMenu);

	return ResultFromShort(idMax - idCmdFirst);
}

/**************************************************************************
* ISVBgCm_fnInvokeCommand()
*/
static HRESULT WINAPI ISVBgCm_fnInvokeCommand(
	IContextMenu *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{
	ICOM_THIS(BgCmImpl, iface);

	LPSHELLBROWSER	lpSB;
	LPSHELLVIEW	lpSV;
	HWND	hWndSV;

	TRACE("(%p)->(invcom=%p verb=%p wnd=%x)\n",This,lpcmi,lpcmi->lpVerb, lpcmi->hwnd);    

	/* get the active IShellView */
	lpSB = (LPSHELLBROWSER)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER,0,0);
	IShellBrowser_QueryActiveShellView(lpSB, &lpSV);
	IShellView_GetWindow(lpSV, &hWndSV);

	if(HIWORD(lpcmi->lpVerb))
	{
	  TRACE("%s\n",lpcmi->lpVerb);

	  if (! strcmp(lpcmi->lpVerb,CMDSTR_NEWFOLDERA))
	  {
	    FIXME("%s not implemented\n",lpcmi->lpVerb);
	  }
	  else if (! strcmp(lpcmi->lpVerb,CMDSTR_VIEWLISTA))
	  {
	    SendMessageA(hWndSV, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_LISTVIEW,0),0 );
	  }
	  else if (! strcmp(lpcmi->lpVerb,CMDSTR_VIEWDETAILSA))
	  {
	    SendMessageA(hWndSV, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_REPORTVIEW,0),0 );
	  } 
	  else
	  {
	    FIXME("please report: unknown verb %s\n",lpcmi->lpVerb);
	  }
	}
	else
	{
	  switch(LOWORD(lpcmi->lpVerb))
	  {
	    case FCIDM_SHVIEW_BIGICON:
	      SendMessageA(hWndSV, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_BIGICON,0),0 );
	      break;

	    case FCIDM_SHVIEW_SMALLICON:
	      SendMessageA(hWndSV, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_SMALLICON,0),0 );

	    case FCIDM_SHVIEW_LISTVIEW:
	      SendMessageA(hWndSV, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_LISTVIEW,0),0 );
	      break;

	    case FCIDM_SHVIEW_REPORTVIEW:
	      SendMessageA(hWndSV, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_REPORTVIEW,0),0 );
	      break;

	    default:
	      SendMessageA(hWndSV, WM_COMMAND, MAKEWPARAM(LOWORD(lpcmi->lpVerb), 0),0 );
	      break;
	  }
	}
	
	IShellView_Release(lpSV);	/* QueryActiveShellView does AddRef*/
	return NOERROR;
}

/**************************************************************************
 *  ISVBgCm_fnGetCommandString()
 *
 */
static HRESULT WINAPI ISVBgCm_fnGetCommandString(
	IContextMenu *iface,
	UINT idCommand,
	UINT uFlags,
	LPUINT lpReserved,
	LPSTR lpszName,
	UINT uMaxNameLen)
{	
	ICOM_THIS(BgCmImpl, iface);

	TRACE("(%p)->(idcom=%x flags=%x %p name=%p len=%x)\n",This, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

	/* test the existance of the menu items, the file dialog enables 
	   the buttons according to this */
	if (uFlags == GCS_VALIDATEA)
	{
	  if(HIWORD(idCommand))
	  {
	    if (!strcmp((LPSTR)idCommand, CMDSTR_VIEWLISTA) ||
	        !strcmp((LPSTR)idCommand, CMDSTR_VIEWDETAILSA) ||
	        !strcmp((LPSTR)idCommand, CMDSTR_NEWFOLDERA))
	    {	
	      return NOERROR;
	    }
	  }
	}

	FIXME("unknown command string\n");
	return E_FAIL;
}

/**************************************************************************
* ISVBgCm_fnHandleMenuMsg()
*/
static HRESULT WINAPI ISVBgCm_fnHandleMenuMsg(
	IContextMenu *iface,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	ICOM_THIS(BgCmImpl, iface);

	FIXME("(%p)->(msg=%x wp=%x lp=%lx)\n",This, uMsg, wParam, lParam);

	return E_NOTIMPL;
}

/**************************************************************************
* IContextMenu VTable
* 
*/
static struct ICOM_VTABLE(IContextMenu) cmvt = 
{	
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ISVBgCm_fnQueryInterface,
	ISVBgCm_fnAddRef,
	ISVBgCm_fnRelease,
	ISVBgCm_fnQueryContextMenu,
	ISVBgCm_fnInvokeCommand,
	ISVBgCm_fnGetCommandString,
	ISVBgCm_fnHandleMenuMsg,
	(void *) 0xdeadbabe	/* just paranoia (IContextMenu3) */
};

