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
#include "heap.h"

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
 
/***********************************************************************
 *		LHashValOfNameSysA
 */
ULONG WINAPI LHashValOfNameSysA( SYSKIND skind, LCID lcid, LPCSTR str)
{
    /* returns a 16 bit hashvalue (depending on skind and lcid) in the
     * lowword and a unique id made from skind and lcid in bits 23-16
     */
    FIXME("(%d,%x,%s), stub, returning 0x424242!\n",skind,(int)lcid,debugstr_a(str));
    return 0x00424242;
}
/***********************************************************************
 *		LHashValOfNameSys
 */
ULONG WINAPI LHashValOfNameSys( SYSKIND skind, LCID lcid, LPCOLESTR str)
{
    LPSTR	strA;
    ULONG	res;
    
    if (!str) return 0;
    strA = HEAP_strdupWtoA(GetProcessHeap(),0,str);
    res = LHashValOfNameSysA(skind,lcid,strA);
    HeapFree(GetProcessHeap(),0,strA);
    return res;
}
