/*
 * Implements IEnumMediaTypes and helper functions. (internal)
 *
 * Copyright (C) Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
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

#ifndef WINE_DSHOW_MTYPE_H
#define WINE_DSHOW_MTYPE_H

HRESULT QUARTZ_MediaType_Copy(
	AM_MEDIA_TYPE* pmtDst,
	const AM_MEDIA_TYPE* pmtSrc );
void QUARTZ_MediaType_Free(
	AM_MEDIA_TYPE* pmt );
AM_MEDIA_TYPE* QUARTZ_MediaType_Duplicate(
	const AM_MEDIA_TYPE* pmtSrc );
void QUARTZ_MediaType_Destroy(
	AM_MEDIA_TYPE* pmt );

void QUARTZ_MediaSubType_FromFourCC(
	GUID* psubtype, DWORD dwFourCC );
BOOL QUARTZ_MediaSubType_IsFourCC(
	const GUID* psubtype );

HRESULT QUARTZ_MediaSubType_FromBitmap(
	GUID* psubtype, const BITMAPINFOHEADER* pbi );

void QUARTZ_PatchBitmapInfoHeader( BITMAPINFOHEADER* pbi );
BOOL QUARTZ_BitmapHasFixedSample( const BITMAPINFOHEADER* pbi );


HRESULT QUARTZ_CreateEnumMediaTypes(
	IEnumMediaTypes** ppobj,
	const AM_MEDIA_TYPE* pTypes, ULONG cTypes );


#endif /* WINE_DSHOW_MTYPE_H */
