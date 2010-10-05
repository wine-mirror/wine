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

HRESULT WINAPI EnumMediaTypes_Construct(IPin *iface, BasePin_GetMediaType enumFunc, BasePin_GetMediaTypeVersion versionFunc, IEnumMediaTypes ** ppEnum);
