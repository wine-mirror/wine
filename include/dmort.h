/*
 * Copyright (C) 2002 Hidenori Takeshima
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

#ifndef __WINE_DMORT_H_
#define __WINE_DMORT_H_


/* exported APIs */

HRESULT WINAPI MoCopyMediaType( DMO_MEDIA_TYPE* pmtDst, const DMO_MEDIA_TYPE* pmtSrc );
HRESULT WINAPI MoCreateMediaType( DMO_MEDIA_TYPE** ppmt, DWORD cbFormat );
HRESULT WINAPI MoDeleteMediaType( DMO_MEDIA_TYPE* pmt );
HRESULT WINAPI MoDuplicateMediaType( DMO_MEDIA_TYPE** ppmtDest, const DMO_MEDIA_TYPE* pmtSrc );
HRESULT WINAPI MoFreeMediaType( DMO_MEDIA_TYPE* pmt );
HRESULT WINAPI MoInitMediaType( DMO_MEDIA_TYPE* pmt, DWORD cbFormat );


#endif	/* __WINE_DMORT_H_ */
