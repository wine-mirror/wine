/*
 * Defines the basic types used by COM interfaces.
 */

#ifndef __WINE_WTYPES_H
#define __WINE_WTYPES_H


#include "windef.h"

/* FIXME: this line should be in rpcndr.h */
typedef unsigned char byte;

/* FIXME: and the following group should be in rpcdce.h */
typedef void* RPC_AUTH_IDENTITY_HANDLE;
typedef void* RPC_AUTHZ_HANDLE;


typedef WORD CLIPFORMAT, *LPCLIPFORMAT;

typedef CHAR		OLECHAR16;
typedef WCHAR		OLECHAR;

typedef LPSTR		LPOLESTR16;
typedef LPWSTR		LPOLESTR;

typedef LPCSTR		LPCOLESTR16;
typedef LPCWSTR		LPCOLESTR;

typedef OLECHAR16	*BSTR16;
typedef OLECHAR	*BSTR;

typedef BSTR16		*LPBSTR16;
typedef BSTR		*LPBSTR;

#define OLESTR16(x) x
#define OLESTR(x) L##x

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
    DWORD Data1;
    WORD  Data2;
    WORD  Data3;
    BYTE  Data4[8];
} GUID;
#endif

typedef GUID  *LPGUID;
typedef GUID  CLSID,*LPCLSID;
typedef GUID	IID,*LPIID;
typedef GUID	FMTID,*LPFMTID;
#ifdef __cplusplus
#define REFGUID             const GUID &
#define REFCLSID            const CLSID &
#define REFIID              const IID &
#define REFFMTID            const FMTID &
#else /* !defined(__cplusplus) */
#define REFGUID             const GUID* const
#define REFCLSID            const CLSID* const
#define REFIID              const IID* const
#define REFFMTID            const FMTID* const
#endif /* !defined(__cplusplus) */

extern const IID GUID_NULL;
#define CLSID_NULL GUID_NULL
   
typedef enum tagDVASPECT
{ 
       DVASPECT_CONTENT   = 1,
       DVASPECT_THUMBNAIL = 2,
       DVASPECT_ICON      = 4,   
       DVASPECT_DOCPRINT  = 8
} DVASPECT;

typedef enum tagSTGC
{
	STGC_DEFAULT = 0,
	STGC_OVERWRITE = 1,
	STGC_ONLYIFCURRENT = 2,
	STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE = 4
} STGC;

typedef struct _COAUTHIDENTITY
{
    USHORT* User;
    ULONG UserLength;
    USHORT* Domain;
    ULONG DomainLength;
    USHORT* Password;
    ULONG PasswordLength;
    ULONG Flags;
} COAUTHIDENTITY;

typedef struct _COAUTHINFO
{
    DWORD dwAuthnSvc;
    DWORD dwAuthzSvc;
    LPWSTR pwszServerPrincName;
    DWORD dwAuthnLevel;
    DWORD dwImpersonationLevel;
    COAUTHIDENTITY* pAuthIdentityData;
    DWORD dwCapabilities;
} COAUTHINFO;

typedef struct _COSERVERINFO
{
    DWORD dwReserved1;
    LPWSTR pwszName;
    COAUTHINFO* pAuthInfo;
    DWORD dwReserved2;
} COSERVERINFO;

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

typedef unsigned short VARTYPE;

typedef ULONG PROPID;

typedef struct tagBLOB
{
    ULONG cbSize;
    BYTE *pBlobData;
} BLOB;

#ifndef _tagCY_DEFINED
#define _tagCY_DEFINED
typedef union tagCY
{
    struct {
#ifdef BIG_ENDIAN
        long Hi;
        long Lo;
#else
        unsigned long Lo;
        long Hi;
#endif
    } u;
    LONGLONG int64;
} CY;
#endif /* _tagCY_DEFINED */

/*
 * 0 == FALSE and -1 == TRUE
 */
#define VARIANT_TRUE     ((VARIANT_BOOL)0xFFFF)
#define VARIANT_FALSE    ((VARIANT_BOOL)0x0000)
typedef short VARIANT_BOOL,_VARIANT_BOOL;

typedef struct tagCLIPDATA
{
    ULONG cbSize;
    long ulClipFmt;
    BYTE *pClipData;
} CLIPDATA;

/* Macro to calculate the size of the above pClipData */
#define CBPCLIPDATA(clipdata)    ( (clipdata).cbSize - sizeof((clipdata).ulClipFmt) )

typedef LONG SCODE;

#ifndef _FILETIME_
#define _FILETIME_
/* 64 bit number of 100 nanoseconds intervals since January 1, 1601 */
typedef struct
{
  DWORD  dwLowDateTime;
  DWORD  dwHighDateTime;
} FILETIME, *LPFILETIME;
#endif /* _FILETIME_ */

#ifndef _SECURITY_DEFINED
#define _SECURITY_DEFINED

typedef struct {
    BYTE Value[6];
} SID_IDENTIFIER_AUTHORITY,*PSID_IDENTIFIER_AUTHORITY;

typedef struct _SID {
    BYTE Revision;
    BYTE SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    DWORD SubAuthority[1];
} SID,*PSID;

/*
 * ACL 
 */

typedef struct _ACL {
    BYTE AclRevision;
    BYTE Sbz1;
    WORD AclSize;
    WORD AceCount;
    WORD Sbz2;
} ACL, *PACL;

typedef DWORD SECURITY_INFORMATION;
typedef WORD SECURITY_DESCRIPTOR_CONTROL;

/* The security descriptor structure */
typedef struct {
    BYTE Revision;
    BYTE Sbz1;
    SECURITY_DESCRIPTOR_CONTROL Control;
    PSID Owner;
    PSID Group;
    PACL Sacl;
    PACL Dacl;
} SECURITY_DESCRIPTOR, *PSECURITY_DESCRIPTOR;

#ifndef _ROTFLAGS_DEFINED
#define _ROTFLAGS_DEFINED
#define ROTFLAGS_REGISTRATIONKEEPSALIVE 0x1
#define ROTFLAGS_ALLOWANYCLIENT 0x2
#endif /* !defined(_ROTFLAGS_DEFINED) */

#endif /* _SECURITY_DEFINED */


#endif /* __WINE_WTYPES_H */
