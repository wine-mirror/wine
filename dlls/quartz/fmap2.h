#ifndef	WINE_DSHOW_FMAP2_H
#define	WINE_DSHOW_FMAP2_H

/*
		implements CLSID_FilterMapper2.

	- At least, the following interfaces should be implemented:

	IUnknown
		+ IFilterMapper2 - IFilterMapper3
 */

#include "iunk.h"


typedef struct FM2_IFilterMapper3Impl
{
	ICOM_VFIELD(IFilterMapper3);
} FM2_IFilterMapper3Impl;

typedef struct CFilterMapper2
{
	QUARTZ_IUnkImpl	unk;
	FM2_IFilterMapper3Impl	fmap3;
	/* IFilterMapper3 fields */
} CFilterMapper2;

#define	CFilterMapper2_THIS(iface,member)		CFilterMapper2*	This = ((CFilterMapper2*)(((char*)iface)-offsetof(CFilterMapper2,member)))

HRESULT QUARTZ_CreateFilterMapper2(IUnknown* punkOuter,void** ppobj);


HRESULT CFilterMapper2_InitIFilterMapper3( CFilterMapper2* psde );
void CFilterMapper2_UninitIFilterMapper3( CFilterMapper2* psde );


#endif	/* WINE_DSHOW_FMAP2_H */

