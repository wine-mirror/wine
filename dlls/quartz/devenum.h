#ifndef	WINE_DSHOW_DEVENUM_H
#define	WINE_DSHOW_DEVENUM_H

/*
		implements CLSID_SystemDeviceEnum.

	- At least, the following interfaces should be implemented:

	IUnknown
		+ ICreateDevEnum
 */

#include "iunk.h"

typedef struct SDE_ICreateDevEnumImpl
{
	ICOM_VFIELD(ICreateDevEnum);
} SDE_ICreateDevEnumImpl;

typedef struct CSysDevEnum
{
	QUARTZ_IUnkImpl	unk;
	SDE_ICreateDevEnumImpl	createdevenum;
} CSysDevEnum;

#define	CSysDevEnum_THIS(iface,member)		CSysDevEnum*	This = ((CSysDevEnum*)(((char*)iface)-offsetof(CSysDevEnum,member)))

HRESULT QUARTZ_CreateSystemDeviceEnum(IUnknown* punkOuter,void** ppobj);


HRESULT CSysDevEnum_InitICreateDevEnum( CSysDevEnum* psde );
void CSysDevEnum_UninitICreateDevEnum( CSysDevEnum* psde );


#endif	/* WINE_DSHOW_DEVENUM_H */

