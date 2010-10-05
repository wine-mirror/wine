/*
 * Header file for Wine's strmbase implementation
 *
 * Copyright 2003 Robert Shearman
 * Copyright 2010 Aric Stewart, CodeWeavers
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

HRESULT WINAPI CopyMediaType(AM_MEDIA_TYPE * pDest, const AM_MEDIA_TYPE *pSrc);
void WINAPI FreeMediaType(AM_MEDIA_TYPE * pMediaType);
AM_MEDIA_TYPE * WINAPI CreateMediaType(AM_MEDIA_TYPE const * pSrc);
void WINAPI DeleteMediaType(AM_MEDIA_TYPE * pMediaType);

typedef HRESULT (WINAPI *BasePin_GetMediaType)(IPin* iface, int iPosition, AM_MEDIA_TYPE *amt);
typedef LONG (WINAPI *BasePin_GetMediaTypeVersion)(IPin* iface);

typedef IPin* (WINAPI *BaseFilter_GetPin)(IBaseFilter* iface, int iPosition);
typedef LONG (WINAPI *BaseFilter_GetPinCount)(IBaseFilter* iface);
typedef LONG (WINAPI *BaseFilter_GetPinVersion)(IBaseFilter* iface);

HRESULT WINAPI EnumMediaTypes_Construct(IPin *iface, BasePin_GetMediaType enumFunc, BasePin_GetMediaTypeVersion versionFunc, IEnumMediaTypes ** ppEnum);

HRESULT WINAPI EnumPins_Construct(IBaseFilter *base,  BaseFilter_GetPin receive_pin, BaseFilter_GetPinCount receive_pincount, BaseFilter_GetPinVersion receive_version, IEnumPins ** ppEnum);

/* Pin functions */

typedef struct BasePin
{
	const struct IPinVtbl * lpVtbl;
	LONG refCount;
	LPCRITICAL_SECTION pCritSec;
	PIN_INFO pinInfo;
	IPin * pConnectedTo;
	AM_MEDIA_TYPE mtCurrent;
} BasePin;

/* Base Pin */
HRESULT WINAPI BasePinImpl_GetMediaType(IPin *iface, int iPosition, AM_MEDIA_TYPE *pmt);
LONG WINAPI BasePinImpl_GetMediaTypeVersion(IPin *iface);
ULONG WINAPI BasePinImpl_AddRef(IPin * iface);
HRESULT WINAPI BasePinImpl_Disconnect(IPin * iface);
HRESULT WINAPI BasePinImpl_ConnectedTo(IPin * iface, IPin ** ppPin);
HRESULT WINAPI BasePinImpl_ConnectionMediaType(IPin * iface, AM_MEDIA_TYPE * pmt);
HRESULT WINAPI BasePinImpl_QueryPinInfo(IPin * iface, PIN_INFO * pInfo);
HRESULT WINAPI BasePinImpl_QueryDirection(IPin * iface, PIN_DIRECTION * pPinDir);
HRESULT WINAPI BasePinImpl_QueryId(IPin * iface, LPWSTR * Id);
HRESULT WINAPI BasePinImpl_QueryAccept(IPin * iface, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI BasePinImpl_EnumMediaTypes(IPin * iface, IEnumMediaTypes ** ppEnum);
HRESULT WINAPI BasePinImpl_QueryInternalConnections(IPin * iface, IPin ** apPin, ULONG * cPin);
