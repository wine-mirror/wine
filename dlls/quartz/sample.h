/*
 * Copyright (C) Hidenori TAKESHIMA
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

#ifndef	WINE_DSHOW_SAMPLE_H
#define	WINE_DSHOW_SAMPLE_H

/*
		implements CMemMediaSample.

	- At least, the following interfaces should be implemented:

	IUnknown - IMediaSample - IMediaSample2
 */

typedef struct CMemMediaSample
{
	ICOM_VFIELD(IMediaSample2);

	/* IUnknown fields */
	ULONG	ref;
	/* IMediaSample2 fields */
	IMemAllocator*	pOwner; /* not addref-ed. */
	BOOL	fMediaTimeIsValid;
	LONGLONG	llMediaTimeStart;
	LONGLONG	llMediaTimeEnd;
	AM_SAMPLE2_PROPERTIES	prop;
} CMemMediaSample;


HRESULT QUARTZ_CreateMemMediaSample(
	BYTE* pbData, DWORD dwDataLength,
	IMemAllocator* pOwner,
	CMemMediaSample** ppSample );
void QUARTZ_DestroyMemMediaSample(
	CMemMediaSample* pSample );


HRESULT QUARTZ_IMediaSample_GetProperties(
	IMediaSample* pSample,
	AM_SAMPLE2_PROPERTIES* pProp );
HRESULT QUARTZ_IMediaSample_SetProperties(
	IMediaSample* pSample,
	const AM_SAMPLE2_PROPERTIES* pProp );
HRESULT QUARTZ_IMediaSample_Copy(
	IMediaSample* pDstSample,
	IMediaSample* pSrcSample,
	BOOL bCopyData );



#endif	/* WINE_DSHOW_SAMPLE_H */
