#ifndef	WINE_DSHOW_MONPROP_H
#define	WINE_DSHOW_MONPROP_H

/*
		implements IPropertyBag for accessing registry.

	- At least, the following interfaces should be implemented:

	IUnknown
		+ IPropertyBag
 */

#include "iunk.h"

typedef struct DMON_IPropertyBagImpl
{
	ICOM_VFIELD(IPropertyBag);
} DMON_IPropertyBagImpl;

typedef struct CRegPropertyBag
{
	QUARTZ_IUnkImpl	unk;
	DMON_IPropertyBagImpl	propbag;
	/* IPropertyBag fields */
	HKEY	m_hKey;
} CRegPropertyBag;

#define	CRegPropertyBag_THIS(iface,member)	CRegPropertyBag*	This = (CRegPropertyBag*)(((char*)iface)-offsetof(CRegPropertyBag,member))

HRESULT QUARTZ_CreateRegPropertyBag(
	HKEY hkRoot, LPCWSTR lpKeyPath,
	IPropertyBag** ppPropBag );

#endif	/* WINE_DSHOW_MONPROP_H */
