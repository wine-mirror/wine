#ifndef	WINE_DSHOW_DEVMON_H
#define	WINE_DSHOW_DEVMON_H

/*
		implements CLSID_CDeviceMoniker.

	- At least, the following interfaces should be implemented:

	IUnknown
		+ IPersist - IPersistStream - IMoniker
 */

#include "iunk.h"

typedef struct DMON_IMonikerImpl
{
	ICOM_VFIELD(IMoniker);
} DMON_IMonikerImpl;

typedef struct CDeviceMoniker
{
	QUARTZ_IUnkImpl	unk;
	DMON_IMonikerImpl	moniker;
	/* IMoniker fields */
	HKEY	m_hkRoot;
	WCHAR*	m_pwszPath;
} CDeviceMoniker;

#define	CDeviceMoniker_THIS(iface,member)	CDeviceMoniker*	This = (CDeviceMoniker*)(((char*)iface)-offsetof(CDeviceMoniker,member))

HRESULT QUARTZ_CreateDeviceMoniker(
	HKEY hkRoot, LPCWSTR lpKeyPath,
	IMoniker** ppMoniker );


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

#endif	/* WINE_DSHOW_DEVMON_H */
