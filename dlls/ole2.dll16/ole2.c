
/*
 *	OLE2 library - 16 bit only interfaces
 *
 *	Copyright 1995	Martin von Loewis
 *      Copyright 1999  Francis Beaudet
 *      Copyright 1999  Noel Borthwick
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wownt32.h"
#include "ole2.h"
#include "winerror.h"

#include "wine/winbase16.h"
#include "wine/wingdi16.h"
#include "wine/winuser16.h"
#include "ifs.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define E_INVALIDARG16 MAKE_SCODE(SEVERITY_ERROR, FACILITY_NULL, 3)

static HICON convert_icon_to_32( HICON16 icon16 )
{
    CURSORICONINFO *info = GlobalLock16( icon16 );
    void *and_bits = info + 1;
    void *xor_bits = (BYTE *)and_bits + info->nHeight * 2 * ((info->nWidth + 15) / 16);
    HICON ret = CreateIcon( 0, info->nWidth, info->nHeight, info->bPlanes, info->bBitsPerPixel,
                            and_bits, xor_bits );
    GlobalUnlock16( icon16 );
    return ret;
}

/******************************************************************************
 *		OleBuildVersion	(OLE2.1)
 */
DWORD WINAPI OleBuildVersion16(void)
{
    return OleBuildVersion();
}

/***********************************************************************
 *           OleInitialize       (OLE2.2)
 */
HRESULT WINAPI OleInitialize16(LPVOID reserved)
{
    return OleInitialize( reserved );
}

/******************************************************************************
 *		OleUninitialize	(OLE2.3)
 */
void WINAPI OleUninitialize16(void)
{
    OleUninitialize();
}

/***********************************************************************
 *           DllGetClassObject (OLE2.4)
 */
HRESULT WINAPI DllGetClassObject16(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
  FIXME("(%s, %s, %p): stub\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);
  return E_NOTIMPL;
}

/******************************************************************************
 *		GetRunningObjectTable (OLE2.30)
 */
HRESULT WINAPI GetRunningObjectTable16(DWORD reserved, LPRUNNINGOBJECTTABLE *pprot)
{
    FIXME("(%ld,%p),stub!\n",reserved,pprot);
    return E_NOTIMPL;
}

/***********************************************************************
 *           RegisterDragDrop (OLE2.35)
 */
HRESULT WINAPI RegisterDragDrop16(
	HWND16 hwnd,
	LPDROPTARGET pDropTarget
) {
	FIXME("(0x%04x,%p),stub!\n",hwnd,pDropTarget);
	return S_OK;
}

/***********************************************************************
 *           RevokeDragDrop (OLE2.36)
 */
HRESULT WINAPI RevokeDragDrop16(
	HWND16 hwnd
) {
	FIXME("(0x%04x),stub!\n",hwnd);
	return S_OK;
}

/******************************************************************************
 * OleMetaFilePictFromIconAndLabel (OLE2.56)
 *
 * Returns a global memory handle to a metafile which contains the icon and
 * label given.
 * I guess the result of that should look somehow like desktop icons.
 * If no hIcon is given, we load the icon via lpszSourceFile and iIconIndex.
 * This code might be wrong at some places.
 */
HGLOBAL16 WINAPI OleMetafilePictFromIconAndLabel16(
	HICON16 icon16,
	LPCOLESTR16 lpszLabel,
	LPCOLESTR16 lpszSourceFile,
	UINT16 iIconIndex
) {
    METAFILEPICT *pict;
    HGLOBAL hmf;
    HGLOBAL16 hmf16;
    LPWSTR label = NULL, source = NULL;
    DWORD len;
    HICON icon = convert_icon_to_32( icon16 );

    if (lpszLabel)
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpszLabel, -1, NULL, 0 );
        label = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpszLabel, -1, label, len );
    }
    if (lpszSourceFile)
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpszSourceFile, -1, NULL, 0 );
        source = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpszSourceFile, -1, source, len );
    }
    hmf = OleMetafilePictFromIconAndLabel( icon, label, source, iIconIndex );
    HeapFree( GetProcessHeap(), 0, label );
    HeapFree( GetProcessHeap(), 0, source );
    DestroyIcon( icon );

    if (!hmf) return 0;
    pict = GlobalLock( hmf );

    hmf16 = GlobalAlloc16(0, sizeof(METAFILEPICT16));
    if (hmf16)
    {
        METAFILEPICT16 *pict16 = GlobalLock16( hmf16 );
        pict16->mm   = pict->mm;
        pict16->xExt = pict->xExt;
        pict16->yExt = pict->yExt;
        len = GetMetaFileBitsEx( pict->hMF, 0, 0 );
        pict16->hMF = GlobalAlloc16( GMEM_MOVEABLE, len );
        GetMetaFileBitsEx( pict->hMF, len, GlobalLock16( pict16->hMF) );
        GlobalUnlock16( pict16->hMF );
        GlobalUnlock16( hmf16 );
    }
    DeleteMetaFile( pict->hMF );
    GlobalUnlock( hmf );
    GlobalFree( hmf );
    return hmf16;
}


/******************************************************************************
 *        CreateItemMoniker	(OLE2.27)
 */
HRESULT WINAPI CreateItemMoniker16(LPCOLESTR16 lpszDelim,LPCOLESTR16 lpszItem,LPMONIKER* ppmk)
{
    FIXME("(%s,%p),stub!\n",lpszDelim,ppmk);
    *ppmk = NULL;
    return E_NOTIMPL;
}


/******************************************************************************
 *        CreateFileMoniker (OLE2.28)
 */
HRESULT WINAPI CreateFileMoniker16(LPCOLESTR16 lpszPathName,LPMONIKER* ppmk)
{
    FIXME("(%s,%p),stub!\n",lpszPathName,ppmk);
    return E_NOTIMPL;
}

/******************************************************************************
 *        OleSetMenuDescriptor (OLE2.41)
 *
 * PARAMS
 *  hOleMenu  FIXME: Should probably be an HOLEMENU16.
 */
HRESULT WINAPI OleSetMenuDescriptor16(
    HOLEMENU               hOleMenu,
    HWND16                 hwndFrame,
    HWND16                 hwndActiveObject,
    LPOLEINPLACEFRAME        lpFrame,
    LPOLEINPLACEACTIVEOBJECT lpActiveObject)
{
    FIXME("(%p, %x, %x, %p, %p), stub!\n", hOleMenu, hwndFrame, hwndActiveObject, lpFrame, lpActiveObject);
    return E_NOTIMPL;
}

/******************************************************************************
 *              OleLoad        [OLE2.12]
 *
 * PARAMS
 *  pStg Segmented LPSTORAGE pointer.
 *  pClientSite Segmented LPOLECLIENTSITE pointer.
 */
HRESULT WINAPI OleLoad16(
    SEGPTR		pStg,
    REFIID            	riid,
    SEGPTR		pClientSite,
    LPVOID*		ppvObj)
{
  FIXME("(%lx,%s,%lx,%p), stub!\n", pStg, debugstr_guid(riid), pClientSite, ppvObj);
  return E_NOTIMPL;
}

/******************************************************************************
 *              OleDoAutoConvert        [OLE2.79]
 */
HRESULT WINAPI OleDoAutoConvert16(LPSTORAGE pStg, LPCLSID pClsidNew)
{
    FIXME("(%p,%p) : stub\n",pStg,pClsidNew);
    return E_NOTIMPL;
}

/***********************************************************************
 *           OleSetClipboard                            [OLE2.49]
 */
HRESULT WINAPI OleSetClipboard16(IDataObject* pDataObj)
{
  FIXME("(%p): stub\n", pDataObj);
  return S_OK;
}

/***********************************************************************
 *           OleGetClipboard                            [OLE2.50]
 */
HRESULT WINAPI OleGetClipboard16(IDataObject** ppDataObj)
{
  FIXME("(%p): stub\n", ppDataObj);
  return E_NOTIMPL;
}

/***********************************************************************
 *           OleFlushClipboard   [OLE2.76]
 */

HRESULT WINAPI OleFlushClipboard16(void)
{
  return OleFlushClipboard();
}

/***********************************************************************
 *           OleIsCurrentClipboard                    [OLE2.77]
 */
HRESULT WINAPI OleIsCurrentClipboard16(IDataObject* pDataObj)
{
  FIXME("(%p): stub\n", pDataObj);
  return S_OK;
}

#define GET_SEGPTR_METHOD_ADDR(ifacename,segptr,methodname) \
    ((SEGPTR)((const ifacename##Vtbl*)MapSL((SEGPTR)((ifacename*)MapSL(segptr))->lpVtbl))->methodname)

/***********************************************************************
 *    ReadClassStg (OLE2.18)
 *
 * This method reads the CLSID previously written to a storage object with
 * the WriteClassStg.
 *
 * PARAMS
 *  pstg    [I] Segmented LPSTORAGE pointer.
 *  pclsid  [O] Pointer to where the CLSID is written
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 */
HRESULT WINAPI ReadClassStg16(SEGPTR pstg, CLSID *pclsid)
{
	STATSTG16 statstg;
	HANDLE16 hstatstg;
	HRESULT	hres;
	DWORD args[3];

	TRACE("(%lx, %p)\n", pstg, pclsid);

	if (!pclsid)
	    return E_INVALIDARG16;

	memset(pclsid, 0, sizeof(*pclsid));

	if (!pstg)
	    return E_INVALIDARG16;

	/*
	 * read a STATSTG structure (contains the clsid) from the storage
	 */
	args[0] = pstg; /* iface */
	args[1] = WOWGlobalAllocLock16( 0, sizeof(STATSTG16), &hstatstg );
	args[2] = STATFLAG_DEFAULT;

	if (!WOWCallback16Ex(
	    GET_SEGPTR_METHOD_ADDR(IStorage16, pstg, Stat),
	    WCB16_PASCAL,
	    3*sizeof(DWORD),
	    args,
	    (LPDWORD)&hres
	)) {
	    WOWGlobalUnlockFree16(args[1]);
            ERR("CallTo16 IStorage16::Stat() failed, hres %lx\n",hres);
	    return hres;
	}
	memcpy(&statstg, MapSL(args[1]), sizeof(STATSTG16));
	WOWGlobalUnlockFree16(args[1]);

	if(SUCCEEDED(hres)) {
		*pclsid=statstg.clsid;
		TRACE("clsid is %s\n", debugstr_guid(&statstg.clsid));
	}
	return hres;
}

/***********************************************************************
 *    ReadClassStm (OLE2.20)
 */
HRESULT WINAPI ReadClassStm16(SEGPTR stream, CLSID *clsid)
{
    HANDLE16 hclsid, hread;
    HRESULT hres;
    DWORD args[4];

    TRACE("(0x%lx, %p)\n", stream, clsid);

    if (!clsid)
        return E_INVALIDARG16;

    memset(clsid, 0, sizeof(*clsid));

    if (!stream)
        return E_INVALIDARG16;

    args[0] = stream; /* iface */
    args[1] = WOWGlobalAllocLock16( 0, sizeof(CLSID), &hclsid );
    args[2] = sizeof(CLSID);
    args[3] = WOWGlobalAllocLock16( 0, sizeof(ULONG), &hread );

    if (WOWCallback16Ex(
        GET_SEGPTR_METHOD_ADDR(IStream16, stream, Read),
        WCB16_PASCAL,
        4*sizeof(DWORD),
        args,
        (DWORD*)&hres))
    {
        ULONG readlen;

        memcpy(&readlen, MapSL(args[3]), sizeof(readlen));
        if (readlen == sizeof(CLSID))
            memcpy(clsid, MapSL(args[1]), sizeof(CLSID));
        else
            hres = STG_E_READFAULT;

        TRACE("clsid is %s\n", debugstr_guid(clsid));
    }
    else
    {
        ERR("CallTo16 IStream16::Read() failed, hres %lx\n", hres);
        hres = E_FAIL;
    }
    WOWGlobalUnlockFree16(args[1]);
    WOWGlobalUnlockFree16(args[3]);

    return hres;
}

/***********************************************************************
 *              GetConvertStg (OLE2.82)
 */
HRESULT WINAPI GetConvertStg16(LPSTORAGE stg)
{
    FIXME("unimplemented stub!\n");
    return E_FAIL;
}

/***********************************************************************
 *              ReleaseStgMedium (OLE2.32)
 */
VOID WINAPI ReleaseStgMedium16(LPSTGMEDIUM medium)
{
    FIXME("%p: unimplemented stub!\n", medium);
}

/***********************************************************************
 *              WriteClassStg16 (OLE2.19)
 */
HRESULT WINAPI WriteClassStg16(IStorage *stg, REFCLSID clsid)
{
    FIXME("stub:%p %s\n", stg, debugstr_guid(clsid));
    return STG_E_MEDIUMFULL;
}
