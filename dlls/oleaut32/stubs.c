/*
 * OlePro32 Stubs
 *
 * Copyright 1999 Corel Corporation
 *
 * Sean Langley
 */

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "debugtools.h"
#include "ole2.h"
#include "olectl.h"

DEFAULT_DEBUG_CHANNEL(ole);

/***********************************************************************
 * OleIconToCursor (OLEAUT32.415)
 */
HCURSOR WINAPI OleIconToCursor( HINSTANCE hinstExe, HICON hicon)
{
	FIXME("(%x,%x), not implemented (olepro32.dll)\n",hinstExe,hicon);
	return S_OK;
}

/***********************************************************************
 * OleCreatePropertyFrameIndirect (OLEAUT32.416)
 */
HRESULT WINAPI OleCreatePropertyFrameIndirect( LPOCPFIPARAMS lpParams)
{
	FIXME("(%p), not implemented (olepro32.dll)\n",lpParams);
	return S_OK;
}
 
/***********************************************************************
 * OleCreatePropertyFrame (OLEAUT32.417)
 */
HRESULT WINAPI OleCreatePropertyFrame(
    HWND hwndOwner, UINT x, UINT y, LPCOLESTR lpszCaption,ULONG cObjects,
    LPUNKNOWN* ppUnk, ULONG cPages, LPCLSID pPageClsID, LCID lcid, 
    DWORD dwReserved, LPVOID pvReserved )
{
	FIXME("(%x,%d,%d,%s,%ld,%p,%ld,%p,%x,%ld,%p), not implemented (olepro32.dll)\n",
		hwndOwner,x,y,debugstr_w(lpszCaption),cObjects,ppUnk,cPages,
		pPageClsID, (int)lcid,dwReserved,pvReserved);
	return S_OK;
}
 
