/*
 * Copyright 2001 TAKESHIMA Hidenori <hidenori@a2.ctktv.ne.jp>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * FIXME - use PropertySheetW.
 * FIXME - not tested.
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"
#include "ole2.h"
#include "olectl.h"
#include "oleauto.h"
#include "commctrl.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

typedef struct CPropertyPageContainerImpl CPropertyPageContainerImpl;

static const struct
{
	DLGTEMPLATE	templ;
	WORD	wMenuName;
	WORD	wClassName;
	WCHAR	wDummyCaption;
	BYTE	padding[4];
} propsite_dlg =
{
	{
        WS_VISIBLE | WS_CHILD, /* style */
        0, /* dwExtendedStyle */
        0, /* cdit */
        0, /* x */
        0, /* y */
        0, /* cx */
        0, /* cy */
	},
	0, 0, 0,
};

typedef struct CPropertyPageSiteImpl
{
	ICOM_VFIELD(IPropertyPageSite);
	/* IUnknown fields */
	ULONG	ref;

	/* IPropertyPageSite fields */
	CPropertyPageContainerImpl*	pContainer;
	IPropertyPage*	pPage;
	HWND	hwnd;
	BYTE	templ[sizeof(propsite_dlg)];
	PROPPAGEINFO	info;
	BOOL	bActivate;
} CPropertyPageSiteImpl;

struct CPropertyPageContainerImpl
{
	ULONG	ref; /* for IUnknown(not used now) */
	LCID	lcid;
	DWORD	m_cSites;
	CPropertyPageSiteImpl**	m_ppSites;
	PROPSHEETPAGEA*	m_pPsp;
	HRESULT m_hr;
};

/* for future use. */
#define CPropertyPageContainerImpl_AddRef(pContainer)	(++((pContainer)->ref))
#define CPropertyPageContainerImpl_Release(pContainer)	(--((pContainer)->ref))


/***************************************************************************/

#define PropSiteDlg_Return(a)	do{SetWindowLongA(hwnd,DWL_MSGRESULT,(LONG)a);return TRUE;}while(1)
static BOOL CALLBACK PropSiteDlgProc(
	HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	CPropertyPageSiteImpl*	This = (CPropertyPageSiteImpl*)GetWindowLongA( hwnd, DWL_USER );
	HRESULT hr;
	RECT	rc;
	NMHDR*	pnmh;

	switch ( msg )
	{
	case WM_INITDIALOG:
		This = (CPropertyPageSiteImpl*)(((PROPSHEETPAGEA*)lParam)->lParam);
		SetWindowLongA( hwnd, DWL_USER, (LONG)This );
		TRACE("WM_INITDIALOG (%p) hwnd = %08x\n", This, hwnd );

		This->hwnd = hwnd;
		ZeroMemory( &rc, sizeof(rc) );
		GetClientRect( hwnd, &rc );
		hr = IPropertyPage_Activate(This->pPage,hwnd,&rc,TRUE);
		if ( SUCCEEDED(hr) )
		{
			This->bActivate = TRUE;
			hr = IPropertyPage_Show(This->pPage,SW_SHOW);
		}
		if ( FAILED(hr) )
			This->pContainer->m_hr = hr;
		break;
	case WM_DESTROY:
		TRACE("WM_DESTROY (%p)\n",This);
		if ( This != NULL )
		{
			if ( This->bActivate )
			{
				IPropertyPage_Show(This->pPage,SW_HIDE);
				IPropertyPage_Deactivate(This->pPage);
				This->bActivate = FALSE;
			}
			This->hwnd = (HWND)NULL;
		}
		SetWindowLongA( hwnd, DWL_USER, (LONG)0 );
		break;
	case WM_NOTIFY:
		pnmh = (NMHDR*)lParam;
		switch ( pnmh->code )
		{
		case PSN_APPLY:
			TRACE("PSN_APPLY (%p)\n",This);
			hr = IPropertyPage_Apply(This->pPage);
			if ( FAILED(hr) )
				PropSiteDlg_Return(PSNRET_INVALID_NOCHANGEPAGE);
			PropSiteDlg_Return(PSNRET_NOERROR);
		case PSN_QUERYCANCEL:
			FIXME("PSN_QUERYCANCEL (%p)\n",This);
			PropSiteDlg_Return(FALSE);
		case PSN_RESET:
			FIXME("PSN_RESET (%p)\n",This);
			PropSiteDlg_Return(0);
		case PSN_SETACTIVE:
			TRACE("PSN_SETACTIVE (%p)\n",This);
			PropSiteDlg_Return(0);
		case PSN_KILLACTIVE:
			TRACE("PSN_KILLACTIVE (%p)\n",This);
			PropSiteDlg_Return(FALSE);
		}
		break;
	}

	return FALSE;
}

/***************************************************************************/

static HRESULT WINAPI
CPropertyPageSiteImpl_fnQueryInterface(IPropertyPageSite* iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(CPropertyPageSiteImpl,iface);

	TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);

	if ( ppobj == NULL )
		return E_POINTER;
	*ppobj = NULL;

	if ( IsEqualGUID(riid,&IID_IUnknown) ||
		 IsEqualGUID(riid,&IID_IPropertyPageSite) )
	{
		*ppobj = (LPVOID)This;
		IUnknown_AddRef((IUnknown*)(*ppobj));
		return S_OK;
	}

	return E_NOINTERFACE;
}

static ULONG WINAPI
CPropertyPageSiteImpl_fnAddRef(IPropertyPageSite* iface)
{
	ICOM_THIS(CPropertyPageSiteImpl,iface);

	TRACE("(%p)->()\n",This);

	return InterlockedExchangeAdd(&(This->ref),1) + 1;
}

static ULONG WINAPI
CPropertyPageSiteImpl_fnRelease(IPropertyPageSite* iface)
{
	ICOM_THIS(CPropertyPageSiteImpl,iface);
	LONG	ref;

	TRACE("(%p)->()\n",This);
	ref = InterlockedExchangeAdd(&(This->ref),-1) - 1;
	if ( ref > 0 )
		return (ULONG)ref;

	if ( This->pContainer != NULL )
		CPropertyPageContainerImpl_Release(This->pContainer);
	if ( This->pPage != NULL )
		IPropertyPage_Release(This->pPage);
	if ( This->info.pszTitle != NULL )
		CoTaskMemFree( This->info.pszTitle );
	if ( This->info.pszDocString != NULL )
		CoTaskMemFree( This->info.pszDocString );
	if ( This->info.pszHelpFile != NULL )
		CoTaskMemFree( This->info.pszHelpFile );

	HeapFree(GetProcessHeap(),0,This);

	return 0;
}

static HRESULT WINAPI
CPropertyPageSiteImpl_fnOnStatusChange(IPropertyPageSite* iface,DWORD dwFlags)
{
	ICOM_THIS(CPropertyPageSiteImpl,iface);

	TRACE("(%p,%08lx)\n",This,dwFlags);

	if ( This->hwnd == (HWND)NULL )
		return E_UNEXPECTED;

	switch ( dwFlags )
	{
	case PROPPAGESTATUS_DIRTY:
		/* dirty */
		SendMessageA(GetParent(This->hwnd),PSM_CHANGED,(WPARAM)(This->hwnd),0);
		break;
	case PROPPAGESTATUS_VALIDATE:
		/* validate */
		SendMessageA(GetParent(This->hwnd),PSM_UNCHANGED,(WPARAM)(This->hwnd),0);
		break;
	default:
		FIXME("(%p,%08lx) unknown flags\n",This,dwFlags);
		return E_INVALIDARG;
	}

	return NOERROR;
}

static HRESULT WINAPI
CPropertyPageSiteImpl_fnGetLocaleID(IPropertyPageSite* iface,LCID* pLocaleID)
{
	ICOM_THIS(CPropertyPageSiteImpl,iface);

	TRACE("(%p,%p)\n",This,pLocaleID);

	if ( pLocaleID == NULL )
		return E_POINTER;

	*pLocaleID = This->pContainer->lcid;

	return NOERROR;
}

static HRESULT WINAPI
CPropertyPageSiteImpl_fnGetPageContainer(IPropertyPageSite* iface,IUnknown** ppUnk)
{
	ICOM_THIS(CPropertyPageSiteImpl,iface);

	FIXME("(%p,%p) - Win95 returns E_NOTIMPL\n",This,ppUnk);

	if ( ppUnk == NULL )
		return E_POINTER;

	*ppUnk = NULL;

	return E_NOTIMPL;
}

static HRESULT WINAPI
CPropertyPageSiteImpl_fnTranslateAccelerator(IPropertyPageSite* iface,MSG* pMsg)
{
	ICOM_THIS(CPropertyPageSiteImpl,iface);

	FIXME("(%p,%p) - Win95 returns E_NOTIMPL\n",This,pMsg);

	if ( pMsg == NULL )
		return E_POINTER;

	return E_NOTIMPL;
}

static ICOM_VTABLE(IPropertyPageSite) iproppagesite =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	CPropertyPageSiteImpl_fnQueryInterface,
	CPropertyPageSiteImpl_fnAddRef,
	CPropertyPageSiteImpl_fnRelease,
	/* IPropertyPageSite fields */
	CPropertyPageSiteImpl_fnOnStatusChange,
	CPropertyPageSiteImpl_fnGetLocaleID,
	CPropertyPageSiteImpl_fnGetPageContainer,
	CPropertyPageSiteImpl_fnTranslateAccelerator,
};

/***************************************************************************/

static
HRESULT OLEPRO32_CreatePropertyPageSite(
	CPropertyPageContainerImpl* pContainer,
	IPropertyPage* pPage,
	CPropertyPageSiteImpl** ppSite,
	PROPSHEETPAGEA* pPspReturned )
{
	CPropertyPageSiteImpl*	This = NULL;
	HRESULT hr;
	DLGTEMPLATE*	ptempl;

	*ppSite = NULL;
	ZeroMemory( pPspReturned, sizeof(PROPSHEETPAGEA) );

	This = (CPropertyPageSiteImpl*)HeapAlloc( GetProcessHeap(), 0,
		sizeof(CPropertyPageSiteImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;
	ZeroMemory( This, sizeof(CPropertyPageSiteImpl) );

	ICOM_VTBL(This) = &iproppagesite;
	This->ref = 1;
	This->pContainer = pContainer; CPropertyPageContainerImpl_AddRef(pContainer);
	This->pPage = pPage; IPropertyPage_AddRef(pPage);
	This->hwnd = (HWND)NULL;
	memcpy( &This->templ, &propsite_dlg, sizeof(propsite_dlg) );
	This->info.cb = sizeof(PROPPAGEINFO);
	This->bActivate = FALSE;
	ptempl = (DLGTEMPLATE*)&This->templ;

	/* construct */
	hr = IPropertyPage_SetPageSite(pPage,(IPropertyPageSite*)This);
	if ( FAILED(hr) )
		goto end;

	hr = IPropertyPage_GetPageInfo(pPage,&This->info);
	if ( FAILED(hr) )
		goto end;

	ptempl->cx = This->info.size.cx;
	ptempl->cy = This->info.size.cy;

	pPspReturned->dwSize = sizeof(PROPSHEETPAGEA);
	pPspReturned->dwFlags = PSP_DLGINDIRECT;
	pPspReturned->u.pResource = ptempl;
	if ( This->info.pszTitle != NULL );
	{
		pPspReturned->dwFlags |= PSP_USETITLE;
		pPspReturned->pszTitle = "Title";/*FIXME - This->info.pszTitle;*/
	}
	pPspReturned->pfnDlgProc = PropSiteDlgProc;
	pPspReturned->lParam = (LONG)This;

end:
	if ( FAILED(hr) )
	{
		IUnknown_Release((IUnknown*)This);
		return hr;
	}

	*ppSite = This;
	return NOERROR;
}

/***************************************************************************/

static
void OLEPRO32_DestroyPropertyPageContainer(
	CPropertyPageContainerImpl* This )
{
	DWORD	nIndex;

	if ( This->m_ppSites != NULL )
	{
		for ( nIndex = 0; nIndex < This->m_cSites; nIndex++ )
		{
			if ( This->m_ppSites[nIndex] != NULL )
			{
				IPropertyPage_SetPageSite(This->m_ppSites[nIndex]->pPage,NULL);
				IUnknown_Release((IUnknown*)This->m_ppSites[nIndex]);
			}
		}
		HeapFree( GetProcessHeap(), 0, This->m_ppSites );
		This->m_ppSites = NULL;
	}
	if ( This->m_pPsp != NULL )
	{
		HeapFree( GetProcessHeap(), 0, This->m_pPsp );
		This->m_pPsp = NULL;
	}
	HeapFree( GetProcessHeap(), 0, This );
}

static
HRESULT OLEPRO32_CreatePropertyPageContainer(
	CPropertyPageContainerImpl** ppContainer,
	ULONG cPages, const CLSID* pclsidPages,
	LCID lcid )
{
	CPropertyPageContainerImpl*	This = NULL;
	DWORD	nIndex;
	IPropertyPage*	pPage;
	HRESULT hr;

	This = (CPropertyPageContainerImpl*)HeapAlloc( GetProcessHeap(), 0,
		sizeof(CPropertyPageContainerImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;
	ZeroMemory( This, sizeof(CPropertyPageContainerImpl) );
	This->ref = 1;
	This->lcid = lcid;
	This->m_cSites = cPages;
	This->m_ppSites = NULL;
	This->m_pPsp = NULL;
	This->m_hr = S_OK;

	This->m_ppSites = (CPropertyPageSiteImpl**)HeapAlloc( GetProcessHeap(), 0, sizeof(CPropertyPageSiteImpl*) * cPages );
	if ( This->m_ppSites == NULL )
	{
		hr = E_OUTOFMEMORY;
		goto end;
	}
	ZeroMemory( This->m_ppSites, sizeof(CPropertyPageSiteImpl*) * cPages );

	This->m_pPsp = (PROPSHEETPAGEA*)HeapAlloc( GetProcessHeap(), 0, sizeof(PROPSHEETPAGEA) * cPages );
	if ( This->m_pPsp == NULL )
	{
		hr = E_OUTOFMEMORY;
		goto end;
	}
	ZeroMemory( This->m_pPsp, sizeof(PROPSHEETPAGEA) * cPages );

	for ( nIndex = 0; nIndex < cPages; nIndex ++ )
	{
		hr = CoCreateInstance(
			&pclsidPages[nIndex], NULL, CLSCTX_SERVER,
			&IID_IPropertyPage, (void**)&pPage );
		if ( FAILED(hr) )
			goto end;
		hr = OLEPRO32_CreatePropertyPageSite(
			This, pPage, &This->m_ppSites[nIndex], &This->m_pPsp[nIndex] );
		IPropertyPage_Release(pPage);
		if ( FAILED(hr) )
			goto end;
	}

	hr = NOERROR;
end:
	if ( FAILED(hr) )
	{
		OLEPRO32_DestroyPropertyPageContainer( This );
		return hr;
	}

	*ppContainer = This;
	return NOERROR;
}

static
HRESULT OLEPRO32_SetObjectsToPropertyPages(
	CPropertyPageContainerImpl* This,
	ULONG cObjects, IUnknown** ppunk )
{
	DWORD	nIndex;
	HRESULT hr;

	for ( nIndex = 0; nIndex < This->m_cSites; nIndex ++ )
	{
		if ( This->m_ppSites[nIndex] == NULL )
			return E_UNEXPECTED;
		hr = IPropertyPage_SetObjects(This->m_ppSites[nIndex]->pPage,cObjects,ppunk);
		if ( FAILED(hr) )
			return hr;
	}

	return NOERROR;
}


/***********************************************************************
 *
 * OleCreatePropertyFrameIndirect (OLEAUT32.416)(OLEPRO32.249)
 *
 */

HRESULT WINAPI OleCreatePropertyFrameIndirect( LPOCPFIPARAMS lpParams )
{
	CPropertyPageContainerImpl*	pContainer = NULL;
	HRESULT hr;
	PROPSHEETHEADERA	psh;
	int ret;

	TRACE("(%p)\n",lpParams);

	if ( lpParams == NULL )
		return E_POINTER;
	if ( lpParams->cbStructSize != sizeof(OCPFIPARAMS) )
	{
		FIXME("lpParams->cbStructSize(%lu) != sizeof(OCPFIPARAMS)(%lu)\n",lpParams->cbStructSize,(unsigned long)sizeof(OCPFIPARAMS));
		return E_INVALIDARG;
	}

	hr = OLEPRO32_CreatePropertyPageContainer(
		&pContainer,
		lpParams->cPages, lpParams->lpPages,
		lpParams->lcid );
	if ( FAILED(hr) )
	{
		TRACE( "OLEPRO32_CreatePropertyPageContainer returns %08lx\n",hr);
		return hr;
	}

	hr = OLEPRO32_SetObjectsToPropertyPages(
		pContainer,
		lpParams->cObjects, lpParams->lplpUnk );
	if ( FAILED(hr) )
	{
		TRACE("OLEPRO32_SetObjectsToPropertyPages returns %08lx\n",hr);
		OLEPRO32_DestroyPropertyPageContainer(pContainer);
		return hr;
	}

	/* FIXME - use lpParams.x / lpParams.y */

	ZeroMemory( &psh, sizeof(psh) );
	psh.dwSize = sizeof(PROPSHEETHEADERA);
	psh.dwFlags = PSH_PROPSHEETPAGE;
	psh.hwndParent = lpParams->hWndOwner;
	psh.pszCaption = "Caption"; /* FIXME - lpParams->lpszCaption; */
	psh.nPages = pContainer->m_cSites;
	psh.u2.nStartPage = lpParams->dispidInitialProperty;
	psh.u3.ppsp = pContainer->m_pPsp;

	ret = PropertySheetA( &psh );
	OLEPRO32_DestroyPropertyPageContainer(pContainer);

	if ( ret == -1 )
		return E_FAIL;

	return S_OK;
}


/***********************************************************************
 *
 * OleCreatePropertyFrame (OLEAUT32.417)(OLEPRO32.250)
 *
 */

HRESULT WINAPI OleCreatePropertyFrame(
    HWND hwndOwner, UINT x, UINT y, LPCOLESTR lpszCaption,ULONG cObjects,
    LPUNKNOWN* lplpUnk, ULONG cPages, LPCLSID pPageClsID, LCID lcid, 
    DWORD dwReserved, LPVOID pvReserved )
{
	OCPFIPARAMS	params;

	TRACE("(%x,%d,%d,%s,%ld,%p,%ld,%p,%x,%ld,%p)\n",
		hwndOwner,x,y,debugstr_w(lpszCaption),cObjects,lplpUnk,cPages,
		pPageClsID, (int)lcid,dwReserved,pvReserved);

	if ( dwReserved != 0 || pvReserved != NULL )
	{
		FIXME("(%x,%d,%d,%s,%ld,%p,%ld,%p,%x,%ld,%p) - E_INVALIDARG\n",
		hwndOwner,x,y,debugstr_w(lpszCaption),cObjects,lplpUnk,cPages,
		pPageClsID, (int)lcid,dwReserved,pvReserved);
		return E_INVALIDARG;
	}

	ZeroMemory( &params, sizeof(params) );
	params.cbStructSize = sizeof(OCPFIPARAMS);
	params.hWndOwner = hwndOwner;
	params.x = x;
	params.y = y;
	params.lpszCaption = lpszCaption;
	params.cObjects = cObjects;
	params.lplpUnk = lplpUnk;
	params.cPages = cPages;
	params.lpPages = pPageClsID;
	params.lcid = lcid;
	params.dispidInitialProperty = 0;

	return OleCreatePropertyFrameIndirect( &params );
}

