/*
 * Defines the basic types used by COM interfaces.
 */

#ifndef __WINE_WTYPES_H
#define __WINE_WTYPES_H


#include "wintypes.h"


typedef WORD CLIPFORMAT32, *LPCLIPFORMAT32;
DECL_WINELIB_TYPE(CLIPFORMAT)

typedef CHAR		OLECHAR16;
typedef WCHAR		OLECHAR32;
DECL_WINELIB_TYPE(OLECHAR)

typedef LPSTR		LPOLESTR16;
typedef LPWSTR		LPOLESTR32;
DECL_WINELIB_TYPE(LPOLESTR)

typedef LPCSTR		LPCOLESTR16;
typedef LPCWSTR		LPCOLESTR32;
DECL_WINELIB_TYPE(LPCOLESTR)

typedef OLECHAR16	*BSTR16;
typedef OLECHAR32	*BSTR32;
DECL_WINELIB_TYPE(BSTR)

typedef BSTR16		*LPBSTR16;
typedef BSTR32		*LPBSTR32;
DECL_WINELIB_TYPE(LPBSTR)

#ifndef GUID_DEFINED
#define GUID_DEFINED
struct _GUID
{
    DWORD Data1;
    WORD  Data2;
    WORD  Data3;
    BYTE  Data4[8];
};
#endif

typedef struct _GUID	GUID,*LPGUID;
typedef struct _GUID	CLSID,*LPCLSID;
typedef struct _GUID	IID,*LPIID;
typedef struct _GUID	FMTID,*LPFMTID;
#ifdef __cplusplus
#define REFGUID             const GUID &
#define REFCLSID            const CLSID &
#define REFIID              const IID &
#define REFFMTID            const FMTID &
#else // !__cplusplus
#define REFGUID             const GUID* const
#define REFCLSID            const CLSID* const
#define REFIID              const IID* const
#define REFFMTID            const FMTID* const
#endif // !__cplusplus


#define DECLARE_HANDLE(a)  typedef HANDLE16 a##16; typedef HANDLE32 a##32
DECLARE_HANDLE(HMETAFILEPICT);
#undef DECLARE_HANDLE

typedef enum tagCLSCTX
{
    CLSCTX_INPROC_SERVER     = 0x1,
    CLSCTX_INPROC_HANDLER    = 0x2,
    CLSCTX_LOCAL_SERVER      = 0x4,
    CLSCTX_INPROC_SERVER16   = 0x8,
    CLSCTX_REMOTE_SERVER     = 0x10,
    CLSCTX_INPROC_HANDLER16  = 0x20,
    CLSCTX_INPROC_SERVERX86  = 0x40,
    CLSCTX_INPROC_HANDLERX86 = 0x80
} CLSCTX;

#define CLSCTX_INPROC           (CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER)
#define CLSCTX_ALL              (CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER)
#define CLSCTX_SERVER           (CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER)


#endif /* __WINE_WTYPES_H */
