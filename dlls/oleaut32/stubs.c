/*
 * OlePro32 Stubs
 *
 * Copyright 1999 Corel Corporation
 *
 * Sean Langley
 */

#include "windef.h"
#include "debugtools.h"
#include "olectl.h"

DEFAULT_DEBUG_CHANNEL(ole);

/***********************************************************************
 * OleIconToCursor
 */
HCURSOR WINAPI OleIconToCursor( HINSTANCE hinstExe, HICON hicon)
{
	FIXME("(%x,%x), not implemented (olepro32.dll)\n",hinstExe,hicon);
	return S_OK;
}

/***********************************************************************
 * OleCreatePropertyFrameIndirect
 */
HRESULT WINAPI OleCreatePropertyFrameIndirect( LPOCPFIPARAMS lpParams)
{
	FIXME("(%p), not implemented (olepro32.dll)\n",lpParams);
	return S_OK;
}
 
/***********************************************************************
 * OleCreatePropertyFrame
 */
HRESULT WINAPI OleCreatePropertyFrame( HWND hwndOwner, UINT x, UINT y,
			LPCOLESTR lpszCaption,ULONG cObjects, LPUNKNOWN* ppUnk,
			ULONG cPages, LPCLSID pPageClsID, LCID lcid, 
			DWORD dwReserved, LPVOID pvReserved )
{
	FIXME("(%x,%d,%d,%p,%ld,%p,%ld,%p,%x,%ld,%p), not implemented (olepro32.dll)\n",
		hwndOwner,x,y,lpszCaption,cObjects,ppUnk,cPages,pPageClsID,
		(int)lcid,dwReserved,pvReserved);
	return S_OK;
}
 
/***********************************************************************
 * OleLoadPicture
 */
HRESULT WINAPI OleLoadPicture( LPSTREAM lpstream, LONG lSize, BOOL fRunmode,
		            REFIID reed, LPVOID *lplpvObj )
{
	FIXME("(%p,%ld,%d,%p,%p), not implemented (olepro32.dll) \n",
		lpstream,lSize,fRunmode,reed,lplpvObj
	);
	return S_OK;
}
 
/***********************************************************************
 * OleCreatePictureIndirect
 */
HRESULT WINAPI OleCreatePictureIndirect(LPPICTDESC lpPictDesc, REFIID riid,
		            BOOL fOwn, LPVOID * lplpvObj )
{
	FIXME("(%p,%p,%d,%p), not implemented (olepro32.dll)\n",
		lpPictDesc,riid,fOwn,lplpvObj
	);
	return S_OK;
}

