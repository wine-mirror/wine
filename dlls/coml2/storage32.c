/*
 * Compound Storage (32 bit version)
 * Storage implementation
 *
 * This file contains the compound file implementation
 * of the storage interface.
 *
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Sylvain St-Germain
 * Copyright 1999 Thuy Nguyen
 * Copyright 2005 Mike McCormack
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
 *
 * NOTES
 *  The compound file implementation of IStorage used for create
 *  and manage substorages and streams within a storage object
 *  residing in a compound file object.
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "wine/debug.h"

#include "ole2.h"      /* For Write/ReadClassStm */

#include "winreg.h"
#include "wine/wingdi16.h"

WINE_DEFAULT_DEBUG_CHANNEL(storage);

/***********************************************************************
 *    ReadClassStg (coml2.@)
 */
HRESULT WINAPI ReadClassStg(IStorage *pstg, CLSID *pclsid)
{
    STATSTG pstatstg;
    HRESULT hRes;

    TRACE("(%p, %p)\n", pstg, pclsid);

    if (!pstg || !pclsid)
        return E_INVALIDARG;

   /*
    * read a STATSTG structure (contains the clsid) from the storage
    */
    hRes = IStorage_Stat(pstg, &pstatstg, STATFLAG_NONAME);

    if (SUCCEEDED(hRes))
        *pclsid = pstatstg.clsid;

    return hRes;
}

/***********************************************************************
 *              WriteClassStm (coml2.@)
 */
HRESULT WINAPI WriteClassStm(IStream *pStm, REFCLSID rclsid)
{
    TRACE("(%p,%p)\n", pStm, rclsid);

    if (!pStm || !rclsid)
        return E_INVALIDARG;

    return IStream_Write(pStm, rclsid, sizeof(CLSID), NULL);
}

/***********************************************************************
 *              ReadClassStm (coml2.@)
 */
HRESULT WINAPI ReadClassStm(IStream *pStm, CLSID *pclsid)
{
    ULONG nbByte;
    HRESULT res;

    TRACE("(%p,%p)\n", pStm, pclsid);

    if (!pStm || !pclsid)
        return E_INVALIDARG;

    /* clear the output args */
    *pclsid = CLSID_NULL;

    res = IStream_Read(pStm, pclsid, sizeof(CLSID), &nbByte);

    if (FAILED(res))
        return res;

    if (nbByte != sizeof(CLSID))
        return STG_E_READFAULT;
    else
        return S_OK;
}

enum stream_1ole_flags {
    OleStream_LinkedObject = 0x00000001,
    OleStream_Convert      = 0x00000004
};

/***********************************************************************
 *              GetConvertStg (coml2.@)
 */
HRESULT WINAPI GetConvertStg(IStorage *stg)
{
    static const DWORD version_magic = 0x02000001;
    DWORD header[2];
    IStream *stream;
    HRESULT hr;

    TRACE("%p\n", stg);

    if (!stg) return E_INVALIDARG;

    hr = IStorage_OpenStream(stg, L"\1Ole", NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stream);
    if (FAILED(hr)) return hr;

    hr = IStream_Read(stream, header, sizeof(header), NULL);
    IStream_Release(stream);
    if (FAILED(hr)) return hr;

    if (header[0] != version_magic)
    {
        ERR("got wrong version magic for 1Ole stream, %#lx.\n", header[0]);
        return E_FAIL;
    }

    return header[1] & OleStream_Convert ? S_OK : S_FALSE;
}
