/*
 *	IContextMenu for items in the shellview
 *
 *	1998, 2000	Juergen Schmied <juergen.schmied@debitel.net>
 */
#include <string.h>

#include "winerror.h"
#include "debugtools.h"

#include "pidl.h"
#include "shlguid.h"
#include "wine/obj_base.h"
#include "wine/obj_contextmenu.h"
#include "wine/obj_shellbrowser.h"
#include "wine/obj_shellextinit.h"
#include "wine/undocshell.h"

#include "shell32_main.h"
#include "shellfolder.h"

DEFAULT_DEBUG_CHANNEL(shell);

/**************************************************************************
*  IContextMenu Implementation
*/
typedef struct 
{	ICOM_VFIELD(IContextMenu);
	DWORD		ref;
	IShellFolder*	pSFParent;
	LPITEMIDLIST	pidl;		/* root pidl */
	LPITEMIDLIST	*apidl;		/* array of child pidls */
	UINT		cidl;
	BOOL		bAllValues;
} ItemCmImpl;


static struct ICOM_VTABLE(IContextMenu) cmvt;

/**************************************************************************
* ISvItemCm_CanRenameItems()
*/
static BOOL ISvItemCm_CanRenameItems(ItemCmImpl *This)
{	UINT  i;
	DWORD dwAttributes;

	TRACE("(%p)->()\n",This);

	if(This->apidl)
	{
	  for(i = 0; i < This->cidl; i++){}
	  if(i > 1) return FALSE;		/* can't rename more than one item at a time*/
	  dwAttributes = SFGAO_CANRENAME;
	  IShellFolder_GetAttributesOf(This->pSFParent, 1, This->apidl, &dwAttributes);
	  return dwAttributes & SFGAO_CANRENAME;
	}
	return FALSE;
}

/**************************************************************************
*   ISvItemCm_Constructor()
*/
IContextMenu *ISvItemCm_Constructor(LPSHELLFOLDER pSFParent, LPCITEMIDLIST pidl, LPCITEMIDLIST *apidl, UINT cidl)
{	ItemCmImpl* cm;
	UINT  u;

	cm = (ItemCmImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ItemCmImpl));
	ICOM_VTBL(cm)=&cmvt;
	cm->ref = 1;
	cm->pidl = ILClone(pidl);
	cm->pSFParent = pSFParent;

	if(pSFParent) IShellFolder_AddRef(pSFParent);

	cm->apidl = _ILCopyaPidl(apidl, cidl);
	cm->cidl = cidl;

	cm->bAllValues = 1;
	for(u = 0; u < cidl; u++)
	{
	  cm->bAllValues &= (_ILIsValue(apidl[u]) ? 1 : 0);
	}

	TRACE("(%p)->()\n",cm);

	shell32_ObjCount++;
	return (IContextMenu*)cm;
}

/**************************************************************************
*  ISvItemCm_fnQueryInterface
*/
static HRESULT WINAPI ISvItemCm_fnQueryInterface(IContextMenu *iface, REFIID riid, LPVOID *ppvObj)
{
	ICOM_THIS(ItemCmImpl, iface);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

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
*  ISvItemCm_fnAddRef
*/
static ULONG WINAPI ISvItemCm_fnAddRef(IContextMenu *iface)
{
	ICOM_THIS(ItemCmImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This, This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}

/**************************************************************************
*  ISvItemCm_fnRelease
*/
static ULONG WINAPI ISvItemCm_fnRelease(IContextMenu *iface)
{
	ICOM_THIS(ItemCmImpl, iface);

	TRACE("(%p)->()\n",This);

	shell32_ObjCount--;

	if (!--(This->ref)) 
	{
	  TRACE(" destroying IContextMenu(%p)\n",This);

	  if(This->pSFParent)
	    IShellFolder_Release(This->pSFParent);

	  if(This->pidl)
	    SHFree(This->pidl);
	  
	  /*make sure the pidl is freed*/
	  _ILFreeaPidl(This->apidl, This->cidl);

	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

/**************************************************************************
*  ICM_InsertItem()
*/ 
void WINAPI _InsertMenuItem (
	HMENU hmenu,
	UINT indexMenu,
	BOOL fByPosition,
	UINT wID,
	UINT fType,
	LPSTR dwTypeData,
	UINT fState)
{
	MENUITEMINFOA	mii;

	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	if (fType == MFT_SEPARATOR)
	{
	  mii.fMask = MIIM_ID | MIIM_TYPE;
	}
	else
	{
	  mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
	  mii.dwTypeData = dwTypeData;
	  mii.fState = fState;
	}
	mii.wID = wID;
	mii.fType = fType;
	InsertMenuItemA( hmenu, indexMenu, fByPosition, &mii);
}
/**************************************************************************
* ISvItemCm_fnQueryContextMenu()
*/

static HRESULT WINAPI ISvItemCm_fnQueryContextMenu(
	IContextMenu *iface,
	HMENU hmenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
	ICOM_THIS(ItemCmImpl, iface);

	TRACE("(%p)->(hmenu=%x indexmenu=%x cmdfirst=%x cmdlast=%x flags=%x )\n",This, hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

	if(!(CMF_DEFAULTONLY & uFlags))
	{
	  if(uFlags & CMF_EXPLORE)
	  {
	    if(This->bAllValues)
	    {
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_OPEN, MFT_STRING, "&Open", MFS_ENABLED);
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_EXPLORE, MFT_STRING, "&Explore", MFS_ENABLED|MFS_DEFAULT);
	    }
	    else
	    {
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_EXPLORE, MFT_STRING, "&Explore", MFS_ENABLED|MFS_DEFAULT);
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_OPEN, MFT_STRING, "&Open", MFS_ENABLED);
	    }
	  }
	  else
	  {
	    _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_OPEN, MFT_STRING, "&Select", MFS_ENABLED|MFS_DEFAULT);
	  }
	  _InsertMenuItem(hmenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
	  _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_COPY, MFT_STRING, "&Copy", MFS_ENABLED);
	  _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_CUT, MFT_STRING, "&Cut", MFS_ENABLED);

	  _InsertMenuItem(hmenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
	  _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_DELETE, MFT_STRING, "&Delete", MFS_ENABLED);

	  if(uFlags & CMF_CANRENAME)
	    _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_RENAME, MFT_STRING, "&Rename", ISvItemCm_CanRenameItems(This) ? MFS_ENABLED : MFS_DISABLED);

	  return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (FCIDM_SHVIEWLAST));
	}
	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
}

/**************************************************************************
* DoOpenExplore
*
*  for folders only
*/

static void DoOpenExplore(
	IContextMenu *iface,
	HWND hwnd,
	LPCSTR verb)
{
	ICOM_THIS(ItemCmImpl, iface);

	int i, bFolderFound = FALSE;
	LPITEMIDLIST	pidlFQ;
	SHELLEXECUTEINFOA	sei;

	/* Find the first item in the list that is not a value. These commands 
	    should never be invoked if there isn't at least one folder item in the list.*/

	for(i = 0; i<This->cidl; i++)
	{
	  if(!_ILIsValue(This->apidl[i]))
	  {
	    bFolderFound = TRUE;
	    break;
	  }
	}

	if (!bFolderFound) return;

	pidlFQ = ILCombine(This->pidl, This->apidl[i]);

	ZeroMemory(&sei, sizeof(sei));
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_IDLIST | SEE_MASK_CLASSNAME;
	sei.lpIDList = pidlFQ;
	sei.lpClass = "folder";
	sei.hwnd = hwnd;
	sei.nShow = SW_SHOWNORMAL;
	sei.lpVerb = verb;
	ShellExecuteExA(&sei);
	SHFree(pidlFQ);
}

/**************************************************************************
* DoRename
*/
static void DoRename(
	IContextMenu *iface,
	HWND hwnd)
{
	ICOM_THIS(ItemCmImpl, iface);

	LPSHELLBROWSER	lpSB;
	LPSHELLVIEW	lpSV;

	TRACE("(%p)->(wnd=%x)\n",This, hwnd);    

	/* get the active IShellView */
	if ((lpSB = (LPSHELLBROWSER)SendMessageA(hwnd, CWM_GETISHELLBROWSER,0,0)))
	{
	  if(SUCCEEDED(IShellBrowser_QueryActiveShellView(lpSB, &lpSV)))
	  {
	    TRACE("(sv=%p)\n",lpSV);    
	    IShellView_SelectItem(lpSV, This->apidl[0],
              SVSI_DESELECTOTHERS|SVSI_EDIT|SVSI_ENSUREVISIBLE|SVSI_FOCUSED|SVSI_SELECT);
	    IShellView_Release(lpSV);
	  }
	}
}

/**************************************************************************
 * DoDelete
 *
 * deletes the currently selected items
 */
static void DoDelete(IContextMenu *iface)
{
	ICOM_THIS(ItemCmImpl, iface);
	ISFHelper * psfhlp;
	
	IShellFolder_QueryInterface(This->pSFParent, &IID_ISFHelper, (LPVOID*)&psfhlp);
	if (psfhlp)
	{
	  ISFHelper_DeleteItems(psfhlp, This->cidl, This->apidl);
	  ISFHelper_Release(psfhlp);
	}
}

/**************************************************************************
 * DoCopyOrCut
 *
 * copys the currently selected items into the clipboard
 */
static BOOL DoCopyOrCut(
	IContextMenu *iface,
	HWND hwnd,
	BOOL bCut)
{
	ICOM_THIS(ItemCmImpl, iface);

	LPSHELLBROWSER	lpSB;
	LPSHELLVIEW	lpSV;
	LPDATAOBJECT    lpDo;
	
	TRACE("(%p)->(wnd=0x%04x,bCut=0x%08x)\n",This, hwnd, bCut);

	if(GetShellOle())
	{
	  /* get the active IShellView */
	  if ((lpSB = (LPSHELLBROWSER)SendMessageA(hwnd, CWM_GETISHELLBROWSER,0,0)))
	  {
	    if (SUCCEEDED(IShellBrowser_QueryActiveShellView(lpSB, &lpSV)))
	    {
	      if (SUCCEEDED(IShellView_GetItemObject(lpSV, SVGIO_SELECTION, &IID_IDataObject, (LPVOID*)&lpDo)))
	      {
	        pOleSetClipboard(lpDo);
	        IDataObject_Release(lpDo);
	      }
	      IShellView_Release(lpSV);
	    }
	  }
	}
	return TRUE;
#if 0
/*
  the following code does the copy operation witout ole32.dll
  we might need this possibility too (js)
*/
	BOOL bSuccess = FALSE;

	TRACE("(%p)\n", iface);
	
	if(OpenClipboard(NULL))
	{
	  if(EmptyClipboard())
	  {
	    IPersistFolder2 * ppf2;
	    IShellFolder_QueryInterface(This->pSFParent, &IID_IPersistFolder2, (LPVOID*)&ppf2);
	    if (ppf2)
	    {
	      LPITEMIDLIST pidl;
	      IPersistFolder2_GetCurFolder(ppf2, &pidl);
	      if(pidl)
	      {
	        HGLOBAL hMem;

		hMem = RenderHDROP(pidl, This->apidl, This->cidl);
		
		if(SetClipboardData(CF_HDROP, hMem))
		{
		  bSuccess = TRUE;
		}
	        SHFree(pidl);
	      }
	      IPersistFolder2_Release(ppf2);
	    }
	    
	  }
	  CloseClipboard();
	}
 	return bSuccess;
#endif
}
/**************************************************************************
* ISvItemCm_fnInvokeCommand()
*/
static HRESULT WINAPI ISvItemCm_fnInvokeCommand(
	IContextMenu *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{
	ICOM_THIS(ItemCmImpl, iface);

	TRACE("(%p)->(invcom=%p verb=%p wnd=%x)\n",This,lpcmi,lpcmi->lpVerb, lpcmi->hwnd);    

	if(LOWORD(lpcmi->lpVerb) > FCIDM_SHVIEWLAST)  return E_INVALIDARG;

	switch(LOWORD(lpcmi->lpVerb))
	{
	  case FCIDM_SHVIEW_EXPLORE:
	    DoOpenExplore(iface, lpcmi->hwnd, "explore");
	    break;
	  case FCIDM_SHVIEW_OPEN:
	    DoOpenExplore(iface, lpcmi->hwnd, "open");
	    break;
	  case FCIDM_SHVIEW_RENAME:
	    DoRename(iface, lpcmi->hwnd);
	    break;	    
	  case FCIDM_SHVIEW_DELETE:
	    DoDelete(iface);
	    break;	    
	  case FCIDM_SHVIEW_COPY:
	    DoCopyOrCut(iface, lpcmi->hwnd, FALSE);
	    break;
	  case FCIDM_SHVIEW_CUT:
	    DoCopyOrCut(iface, lpcmi->hwnd, TRUE);
	    break;
	}
	return NOERROR;
}

/**************************************************************************
*  ISvItemCm_fnGetCommandString()
*/
static HRESULT WINAPI ISvItemCm_fnGetCommandString(
	IContextMenu *iface,
	UINT idCommand,
	UINT uFlags,
	LPUINT lpReserved,
	LPSTR lpszName,
	UINT uMaxNameLen)
{	
	ICOM_THIS(ItemCmImpl, iface);

	HRESULT  hr = E_INVALIDARG;

	TRACE("(%p)->(idcom=%x flags=%x %p name=%p len=%x)\n",This, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

	switch(uFlags)
	{
	  case GCS_HELPTEXT:
	    hr = E_NOTIMPL;
	    break;

	  case GCS_VERBA:
	    switch(idCommand)
	    {
	      case FCIDM_SHVIEW_RENAME:
	        strcpy((LPSTR)lpszName, "rename");
	        hr = NOERROR;
	        break;
	    }
	    break;

	     /* NT 4.0 with IE 3.0x or no IE will always call This with GCS_VERBW. In This 
	     case, you need to do the lstrcpyW to the pointer passed.*/
	  case GCS_VERBW:
	    switch(idCommand)
	    { case FCIDM_SHVIEW_RENAME:
                MultiByteToWideChar( CP_ACP, 0, "rename", -1, (LPWSTR)lpszName, uMaxNameLen );
	        hr = NOERROR;
	        break;
	    }
	    break;

	  case GCS_VALIDATE:
	    hr = NOERROR;
	    break;
	}
	TRACE("-- (%p)->(name=%s)\n",This, lpszName);
	return hr;
}

/**************************************************************************
* ISvItemCm_fnHandleMenuMsg()
* NOTES
*  should be only in IContextMenu2 and IContextMenu3
*  is nevertheless called from word95
*/
static HRESULT WINAPI ISvItemCm_fnHandleMenuMsg(
	IContextMenu *iface,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	ICOM_THIS(ItemCmImpl, iface);

	TRACE("(%p)->(msg=%x wp=%x lp=%lx)\n",This, uMsg, wParam, lParam);

	return E_NOTIMPL;
}

static struct ICOM_VTABLE(IContextMenu) cmvt = 
{	
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ISvItemCm_fnQueryInterface,
	ISvItemCm_fnAddRef,
	ISvItemCm_fnRelease,
	ISvItemCm_fnQueryContextMenu,
	ISvItemCm_fnInvokeCommand,
	ISvItemCm_fnGetCommandString,
	ISvItemCm_fnHandleMenuMsg,
	(void *) 0xdeadbabe	/* just paranoia */
};
