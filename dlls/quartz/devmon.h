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


#endif	/* WINE_DSHOW_DEVMON_H */
