/*
 *	IContextMenu
 *
 *	Copyright 1998	Juergen Schmied <juergen.schmied@metronet.de>
 */
#include <string.h>

#include "winerror.h"
#include "debug.h"

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
	IShellFolder*	pSFParent;
	LPITEMIDLIST	*aPidls;
	BOOL		bAllValues;
} IContextMenuImpl;


static struct ICOM_VTABLE(IContextMenu) cmvt;

/**************************************************************************
*  IContextMenu_AllocPidlTable()
*/
BOOL IContextMenu_AllocPidlTable(IContextMenuImpl *This, DWORD dwEntries)
{
	TRACE(shell,"(%p)->(entrys=%lu)\n",This, dwEntries);

	/*add one for NULL terminator */
	dwEntries++;

	This->aPidls = (LPITEMIDLIST*)SHAlloc(dwEntries * sizeof(LPITEMIDLIST));

	if(This->aPidls)
	{ ZeroMemory(This->aPidls, dwEntries * sizeof(LPITEMIDLIST));	/*set all of the entries to NULL*/
	}
	return (This->aPidls != NULL);
}

/**************************************************************************
* IContextMenu_FreePidlTable()
*/
void IContextMenu_FreePidlTable(IContextMenuImpl *This)
{
	int   i;

	TRACE(shell,"(%p)->()\n",This);

	if(This->aPidls)
	{ for(i = 0; This->aPidls[i]; i++)
	  { SHFree(This->aPidls[i]);
	  }

	  SHFree(This->aPidls);
	  This->aPidls = NULL;
	}
}

/**************************************************************************
* IContextMenu_FillPidlTable()
*/
BOOL IContextMenu_FillPidlTable(IContextMenuImpl *This, LPCITEMIDLIST *aPidls, UINT uItemCount)
{
	UINT  i;

	TRACE(shell,"(%p)->(apidl=%p count=%u)\n",This, aPidls, uItemCount);

	if(This->aPidls)
	{ for(i = 0; i < uItemCount; i++)
	  { This->aPidls[i] = ILClone(aPidls[i]);
	  }
	  return TRUE;
	}
	return FALSE;
}

/**************************************************************************
* IContextMenu_CanRenameItems()
*/
BOOL IContextMenu_CanRenameItems(IContextMenuImpl *This)
{	UINT  i;
	DWORD dwAttributes;

	TRACE(shell,"(%p)->()\n",This);

	if(This->aPidls)
	{
	  for(i = 0; This->aPidls[i]; i++){} /*get the number of items assigned to This object*/
	  { if(i > 1)	/*you can't rename more than one item at a time*/
	    { return FALSE;
	    }
	  }
	  dwAttributes = SFGAO_CANRENAME;
	  IShellFolder_GetAttributesOf(This->pSFParent, i, (LPCITEMIDLIST*)This->aPidls, &dwAttributes);

	  return dwAttributes & SFGAO_CANRENAME;
	}
	return FALSE;
}

/**************************************************************************
*   IContextMenu_Constructor()
*/
IContextMenu *IContextMenu_Constructor(LPSHELLFOLDER pSFParent, LPCITEMIDLIST *aPidls, UINT uItemCount)
{	IContextMenuImpl* cm;
	UINT  u;

	cm = (IContextMenuImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IContextMenuImpl));
	cm->lpvtbl=&cmvt;
	cm->ref = 1;

	cm->pSFParent = pSFParent;
	if(pSFParent)
	   IShellFolder_AddRef(pSFParent);

	cm->aPidls = NULL;

	IContextMenu_AllocPidlTable(cm, uItemCount);

	if(cm->aPidls)
	{ IContextMenu_FillPidlTable(cm, aPidls, uItemCount);
	}

	cm->bAllValues = 1;
	for(u = 0; u < uItemCount; u++)
	{ cm->bAllValues &= (_ILIsValue(aPidls[u]) ? 1 : 0);
	}
	TRACE(shell,"(%p)->()\n",cm);
	shell32_ObjCount++;
	return (IContextMenu*)cm;
}

/**************************************************************************
*  IContextMenu_fnQueryInterface
*/
static HRESULT WINAPI IContextMenu_fnQueryInterface(IContextMenu *iface, REFIID riid, LPVOID *ppvObj)
{
	ICOM_THIS(IContextMenuImpl, iface);

	char    xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);

	TRACE(shell,"(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IContextMenu))  /*IContextMenu*/
	{ *ppvObj = This;
	}   
	else if(IsEqualIID(riid, &IID_IShellExtInit))  /*IShellExtInit*/
	{ FIXME (shell,"-- LPSHELLEXTINIT pointer requested\n");
	}

	if(*ppvObj)
	{ 
	  IContextMenu_AddRef((IContextMenu*)*ppvObj);      
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**************************************************************************
*  IContextMenu_fnAddRef
*/
static ULONG WINAPI IContextMenu_fnAddRef(IContextMenu *iface)
{
	ICOM_THIS(IContextMenuImpl, iface);

	TRACE(shell,"(%p)->(count=%lu)\n",This, This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}

/**************************************************************************
*  IContextMenu_fnRelease
*/
static ULONG WINAPI IContextMenu_fnRelease(IContextMenu *iface)
{
	ICOM_THIS(IContextMenuImpl, iface);

	TRACE(shell,"(%p)->()\n",This);

	shell32_ObjCount--;

	if (!--(This->ref)) 
	{ TRACE(shell," destroying IContextMenu(%p)\n",This);

	  if(This->pSFParent)
	    IShellFolder_Release(This->pSFParent);

	  /*make sure the pidl is freed*/
	  if(This->aPidls)
	  { IContextMenu_FreePidlTable(This);
	  }

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
	{ mii.fMask = MIIM_ID | MIIM_TYPE;
	}
	else
	{ mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
	  mii.dwTypeData = dwTypeData;
	  mii.fState = fState;
	}
	mii.wID = wID;
	mii.fType = fType;
	InsertMenuItemA( hmenu, indexMenu, fByPosition, &mii);
}
/**************************************************************************
* IContextMenu_fnQueryContextMenu()
*/

static HRESULT WINAPI IContextMenu_fnQueryContextMenu(
	IContextMenu *iface,
	HMENU hmenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
	ICOM_THIS(IContextMenuImpl, iface);

	BOOL	fExplore ;

	TRACE(shell,"(%p)->(hmenu=%x indexmenu=%x cmdfirst=%x cmdlast=%x flags=%x )\n",This, hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

	if(!(CMF_DEFAULTONLY & uFlags))
	{ if(!This->bAllValues)	
	  { /* folder menu */
	    fExplore = uFlags & CMF_EXPLORE;
	    if(fExplore) 
	    { _InsertMenuItem(hmenu, indexMenu++, TRUE, idCmdFirst+IDM_EXPLORE, MFT_STRING, "&Explore", MFS_ENABLED|MFS_DEFAULT);
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, idCmdFirst+IDM_OPEN, MFT_STRING, "&Open", MFS_ENABLED);
	    }
	    else
	    { _InsertMenuItem(hmenu, indexMenu++, TRUE, idCmdFirst+IDM_OPEN, MFT_STRING, "&Open", MFS_ENABLED|MFS_DEFAULT);
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, idCmdFirst+IDM_EXPLORE, MFT_STRING, "&Explore", MFS_ENABLED);
	    }

	    if(uFlags & CMF_CANRENAME)
	    { _InsertMenuItem(hmenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, idCmdFirst+IDM_RENAME, MFT_STRING, "&Rename", (IContextMenu_CanRenameItems(This) ? MFS_ENABLED : MFS_DISABLED));
	    }
	  }
	  else	/* file menu */
	  { _InsertMenuItem(hmenu, indexMenu++, TRUE, idCmdFirst+IDM_OPEN, MFT_STRING, "&Open", MFS_ENABLED|MFS_DEFAULT);
	    if(uFlags & CMF_CANRENAME)
	    { _InsertMenuItem(hmenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, idCmdFirst+IDM_RENAME, MFT_STRING, "&Rename", (IContextMenu_CanRenameItems(This) ? MFS_ENABLED : MFS_DISABLED));
	    }
	  }
	  return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (IDM_LAST + 1));
	}
	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
}

/**************************************************************************
* IContextMenu_fnInvokeCommand()
*/
static HRESULT WINAPI IContextMenu_fnInvokeCommand(
	IContextMenu *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{
	ICOM_THIS(IContextMenuImpl, iface);

	LPITEMIDLIST	pidlTemp,pidlFQ;
	LPSHELLBROWSER	lpSB;
	LPSHELLVIEW	lpSV;
	HWND	hWndSV;
	SHELLEXECUTEINFOA	sei;
	int   i;

	TRACE(shell,"(%p)->(invcom=%p verb=%p wnd=%x)\n",This,lpcmi,lpcmi->lpVerb, lpcmi->hwnd);    

	if(HIWORD(lpcmi->lpVerb))
	{ /* get the active IShellView */
	  lpSB = (LPSHELLBROWSER)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER,0,0);
	  IShellBrowser_QueryActiveShellView(lpSB, &lpSV);	/* does AddRef() on lpSV */
	  IShellView_GetWindow(lpSV, &hWndSV);
	  
	  /* these verbs are used by the filedialogs*/
	  TRACE(shell,"%s\n",lpcmi->lpVerb);
	  if (! strcmp(lpcmi->lpVerb,CMDSTR_NEWFOLDER))
	  { FIXME(shell,"%s not implemented\n",lpcmi->lpVerb);
	  }
	  else if (! strcmp(lpcmi->lpVerb,CMDSTR_VIEWLIST))
	  { SendMessageA(hWndSV, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_LISTVIEW,0),0 );
	  }
	  else if (! strcmp(lpcmi->lpVerb,CMDSTR_VIEWDETAILS))
	  { SendMessageA(hWndSV, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_REPORTVIEW,0),0 );
	  } 
	  else
	  { FIXME(shell,"please report: unknown verb %s\n",lpcmi->lpVerb);
	  }
	  IShellView_Release(lpSV);
	  return NOERROR;
	}

	if(LOWORD(lpcmi->lpVerb) > IDM_LAST)
	  return E_INVALIDARG;

	switch(LOWORD(lpcmi->lpVerb))
	{ case IDM_EXPLORE:
	  case IDM_OPEN:
	  /* Find the first item in the list that is not a value. These commands 
	    should never be invoked if there isn't at least one folder item in the list.*/

	    for(i = 0; This->aPidls[i]; i++)
	    { if(!_ILIsValue(This->aPidls[i]))
	         break;
	    }

	    pidlTemp = ILCombine(((IGenericSFImpl*)(This->pSFParent))->mpidl, This->aPidls[i]);
	    pidlFQ = ILCombine(((IGenericSFImpl*)(This->pSFParent))->pMyPidl, pidlTemp);
	    SHFree(pidlTemp);
	
	    ZeroMemory(&sei, sizeof(sei));
	    sei.cbSize = sizeof(sei);
	    sei.fMask = SEE_MASK_IDLIST | SEE_MASK_CLASSNAME;
	    sei.lpIDList = pidlFQ;
	    sei.lpClass = "folder";
	    sei.hwnd = lpcmi->hwnd;
	    sei.nShow = SW_SHOWNORMAL;

	    if(LOWORD(lpcmi->lpVerb) == IDM_EXPLORE)
	    { sei.lpVerb = "explore";
	    }
	    else
	    { sei.lpVerb = "open";
	    }
	    ShellExecuteExA(&sei);
	    SHFree(pidlFQ);
	    break;
		
	  case IDM_RENAME:
	    MessageBeep(MB_OK);
	    /*handle rename for the view here*/
	    break;	    
	}
	return NOERROR;
}

/**************************************************************************
*  IContextMenu_fnGetCommandString()
*/
static HRESULT WINAPI IContextMenu_fnGetCommandString(
	IContextMenu *iface,
	UINT idCommand,
	UINT uFlags,
	LPUINT lpReserved,
	LPSTR lpszName,
	UINT uMaxNameLen)
{	
	ICOM_THIS(IContextMenuImpl, iface);

	HRESULT  hr = E_INVALIDARG;

	TRACE(shell,"(%p)->(idcom=%x flags=%x %p name=%p len=%x)\n",This, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

	switch(uFlags)
	{ case GCS_HELPTEXT:
	    hr = E_NOTIMPL;
	    break;

	  case GCS_VERBA:
	    switch(idCommand)
	    { case IDM_RENAME:
	        strcpy((LPSTR)lpszName, "rename");
	        hr = NOERROR;
	        break;
	    }
	    break;

	     /* NT 4.0 with IE 3.0x or no IE will always call This with GCS_VERBW. In This 
	     case, you need to do the lstrcpyW to the pointer passed.*/
	  case GCS_VERBW:
	    switch(idCommand)
	    { case IDM_RENAME:
	        lstrcpyAtoW((LPWSTR)lpszName, "rename");
	        hr = NOERROR;
	        break;
	    }
	    break;

	  case GCS_VALIDATE:
	    hr = NOERROR;
	    break;
	}
	TRACE(shell,"-- (%p)->(name=%s)\n",This, lpszName);
	return hr;
}

/**************************************************************************
* IContextMenu_fnHandleMenuMsg()
* NOTES
*  should be only in IContextMenu2 and IContextMenu3
*  is nevertheless called from word95
*/
static HRESULT WINAPI IContextMenu_fnHandleMenuMsg(
	IContextMenu *iface,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	ICOM_THIS(IContextMenuImpl, iface);

	TRACE(shell,"(%p)->(msg=%x wp=%x lp=%lx)\n",This, uMsg, wParam, lParam);

	return E_NOTIMPL;
}

/**************************************************************************
* IContextMenu VTable
* 
*/
static struct ICOM_VTABLE(IContextMenu) cmvt = 
{	
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IContextMenu_fnQueryInterface,
	IContextMenu_fnAddRef,
	IContextMenu_fnRelease,
	IContextMenu_fnQueryContextMenu,
	IContextMenu_fnInvokeCommand,
	IContextMenu_fnGetCommandString,
	IContextMenu_fnHandleMenuMsg,
	(void *) 0xdeadbabe	/* just paranoia */
};

