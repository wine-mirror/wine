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

#ifndef WINE_DSHOW_MEMALLOC_H
#define WINE_DSHOW_MEMALLOC_H

/*
		implements CLSID_MemoryAllocator.

	- At least, the following interfaces should be implemented:

	IUnknown
		+ IMemAllocator

*/

#include "iunk.h"
#include "sample.h"

typedef struct MA_IMemAllocatorImpl
{
	ICOM_VFIELD(IMemAllocator);
} MA_IMemAllocatorImpl;

typedef struct CMemoryAllocator
{
	QUARTZ_IUnkImpl	unk;
	MA_IMemAllocatorImpl	memalloc;

	/* IMemAllocator fields. */
	CRITICAL_SECTION	csMem;
	ALLOCATOR_PROPERTIES	prop;
	HANDLE	hEventSample;
	BYTE*	pData;
	CMemMediaSample**	ppSamples;
} CMemoryAllocator;

#define	CMemoryAllocator_THIS(iface,member)		CMemoryAllocator*	This = ((CMemoryAllocator*)(((char*)iface)-offsetof(CMemoryAllocator,member)))

HRESULT QUARTZ_CreateMemoryAllocator(IUnknown* punkOuter,void** ppobj);

HRESULT CMemoryAllocator_InitIMemAllocator( CMemoryAllocator* pma );
void CMemoryAllocator_UninitIMemAllocator( CMemoryAllocator* pma );


#endif  /* WINE_DSHOW_MEMALLOC_H */
