/*
 * Copyright (C) 2002 Alexandre Julliard
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
 */

#include <rpc.h>
#include <rpcndr.h>
#ifndef COM_NO_WINDOWS_H
#include <windows.h>
#include <ole2.h>
#endif

#ifndef __mediaobj_h__
#define __mediaobj_h__

typedef struct IDMOQualityControl IDMOQualityControl;
typedef struct IDMOVideoOutputOptimizations IDMOVideoOutputOptimizations;
typedef struct IEnumDMO IEnumDMO;
typedef struct IMediaBuffer IMediaBuffer;
typedef struct IMediaObject IMediaObject;
typedef struct IMediaObjectInPlace IMediaObjectInPlace;

#include <unknwn.h>
#include <objidl.h>

typedef struct _DMOMediaType
{
    GUID majortype;
    GUID subtype;
    BOOL bFixedSizeSamples;
    BOOL bTemporalCompression;
    ULONG lSampleSize;
    GUID formattype;
    IUnknown *pUnk;
    ULONG cbFormat;
    BYTE *pbFormat;
} DMO_MEDIA_TYPE;

#define INTERFACE IEnumDMO
#define IEnumDMO_METHODS \
    IUnknown_METHODS \
    STDMETHOD(Next)(THIS_ DWORD  cItemsToFetch, CLSID * pCLSID, \
        WCHAR ** Names, DWORD *  pcItemsFectched) PURE; \
    STDMETHOD(Skip)(THIS_ DWORD  cItemsToSkip) PURE; \
    STDMETHOD(Reset)(THIS) PURE; \
    STDMETHOD(Clone)(THIS_ IEnumDMO ** ppEnum) PURE;
ICOM_DEFINE(IEnumDMO,IUnknown)
#undef INTERFACE

#endif /* __mediaobj_h__ */
