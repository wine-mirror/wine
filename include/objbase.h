#ifndef __WINE_OBJBASE_H
#define __WINE_OBJBASE_H


#include "wine/obj_base.h"

/* the following depend only on obj_base.h */
#include "wine/obj_misc.h"
#include "wine/obj_channel.h"
#include "wine/obj_clientserver.h"
#include "wine/obj_marshal.h"
#include "wine/obj_storage.h"

/* the following depend on obj_storage.h */
#include "wine/obj_moniker.h"
#include "wine/obj_propertystorage.h"

/* the following depend on obj_moniker.h */
#include "wine/obj_dataobject.h"

/* FIXME: the following should be moved to one of the wine/obj_XXX.h headers */

/*****************************************************************************
 * CoXXX API
 */
/* FIXME: more CoXXX functions are missing */
DWORD WINAPI CoBuildVersion(void);

typedef enum tagCOINIT
{
    COINIT_APARTMENTTHREADED  = 0x2, /* Apartment model */
    COINIT_MULTITHREADED      = 0x0, /* OLE calls objects on any thread */
    COINIT_DISABLE_OLE1DDE    = 0x4, /* Don't use DDE for Ole1 support */
    COINIT_SPEED_OVER_MEMORY  = 0x8  /* Trade memory for speed */
} COINIT;

HRESULT WINAPI CoInitialize16(LPVOID lpReserved);
HRESULT WINAPI CoInitialize32(LPVOID lpReserved);
#define CoInitialize WINELIB_NAME(CoInitialize)

HRESULT WINAPI CoInitializeEx32(LPVOID lpReserved, DWORD dwCoInit);
#define CoInitializeEx WINELIB_NAME(CoInitializeEx)

void WINAPI CoUninitialize16(void);
void WINAPI CoUninitialize32(void);
#define CoUninitialize WINELIB_NAME(CoUninitialize)

HRESULT WINAPI CoCreateGuid(GUID *pguid);

/* class registration flags; passed to CoRegisterClassObject */
typedef enum tagREGCLS
{
    REGCLS_SINGLEUSE = 0,
    REGCLS_MULTIPLEUSE = 1,
    REGCLS_MULTI_SEPARATE = 2,
    REGCLS_SUSPENDED = 4
} REGCLS;

HRESULT WINAPI CoRegisterClassObject16(REFCLSID rclsid, LPUNKNOWN pUnk, DWORD dwClsContext, DWORD flags, LPDWORD lpdwRegister);
HRESULT WINAPI CoRegisterClassObject32(REFCLSID rclsid,LPUNKNOWN pUnk,DWORD dwClsContext,DWORD flags,LPDWORD lpdwRegister);
#define CoRegisterClassObject WINELIB_NAME(CoRegisterClassObject)

HRESULT WINAPI CoRevokeClassObject32(DWORD dwRegister);
#define CoRevokeClassObject WINELIB_NAME(CoRevokeClassObject)

HRESULT WINAPI CoGetClassObject(REFCLSID rclsid, DWORD dwClsContext,LPVOID pvReserved, REFIID iid, LPVOID *ppv);


HRESULT WINAPI CoCreateInstance(REFCLSID rclsid,LPUNKNOWN pUnkOuter,DWORD dwClsContext,REFIID iid,LPVOID *ppv);
void WINAPI CoFreeLibrary(HINSTANCE32 hLibrary);
void WINAPI CoFreeAllLibraries(void);
void WINAPI CoFreeUnusedLibraries(void);
HRESULT WINAPI CoFileTimeNow(FILETIME *lpFileTime);
LPVOID WINAPI CoTaskMemAlloc(ULONG size);
void WINAPI CoTaskMemFree(LPVOID ptr);
HINSTANCE32 WINAPI CoLoadLibrary(LPOLESTR16 lpszLibName, BOOL32 bAutoFree);

HRESULT WINAPI CoLockObjectExternal16(LPUNKNOWN pUnk,BOOL16 fLock,BOOL16 fLastUnlockReleases);
HRESULT WINAPI CoLockObjectExternal32(LPUNKNOWN pUnk,BOOL32 fLock,BOOL32 fLastUnlockReleases);
#define CoLockObjectExternal WINELIB_NAME(CoLockObjectExternal)


/* internal Wine stuff */


/*****************************************************************************
 * IClassFactory interface
 */

typedef struct _IClassFactory {
    /* IUnknown fields */
    ICOM_VTABLE(IClassFactory)* lpvtbl;
    DWORD                       ref;
} _IClassFactory;

HRESULT WINE_StringFromCLSID(const CLSID *id, LPSTR);


/*****************************************************************************
 * IMalloc interface
 */
/* private prototypes for the constructors */
LPMALLOC16	IMalloc16_Constructor(void);
LPMALLOC32	IMalloc32_Constructor(void);


/*****************************************************************************
 * IUnknown interface
 */

typedef struct _IUnknown {
    /* IUnknown fields */
    ICOM_VTABLE(IUnknown)* lpvtbl;
    DWORD                  ref;
} _IUnknown;

LPUNKNOWN	IUnknown_Constructor(void);

#endif /* __WINE_OBJBASE_H */
