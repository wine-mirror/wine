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
