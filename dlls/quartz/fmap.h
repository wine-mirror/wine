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

#endif	/* WINE_DSHOW_FMAP_H */
