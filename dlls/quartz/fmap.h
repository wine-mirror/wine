#ifndef	WINE_DSHOW_FMAP_H
#define	WINE_DSHOW_FMAP_H

/*
		implements CLSID_FilterMapper.

	- At least, the following interfaces should be implemented:

	IUnknown
		+ IFilterMapper
 */

#include "iunk.h"


typedef struct FM_IFilterMapperImpl
{
	ICOM_VFIELD(IFilterMapper);
} FM_IFilterMapperImpl;

typedef struct CFilterMapper
{
	QUARTZ_IUnkImpl	unk;
	FM_IFilterMapperImpl	fmap;
} CFilterMapper;

#define	CFilterMapper_THIS(iface,member)		CFilterMapper*	This = ((CFilterMapper*)(((char*)iface)-offsetof(CFilterMapper,member)))

HRESULT QUARTZ_CreateFilterMapper(IUnknown* punkOuter,void** ppobj);


HRESULT CFilterMapper_InitIFilterMapper( CFilterMapper* pfm );
void CFilterMapper_UninitIFilterMapper( CFilterMapper* pfm );


#endif	/* WINE_DSHOW_FMAP_H */

