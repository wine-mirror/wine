/*
 * Defines the basic types used by COM interfaces.
 */

#ifndef __WINE_WTYPES_H
#define __WINE_WTYPES_H

#include "basetsd.h"
#include "guiddef.h"
#include "rpc.h"

typedef WORD CLIPFORMAT, *LPCLIPFORMAT;

/* FIXME: does not belong here */
typedef CHAR		OLECHAR16;
typedef LPSTR		LPOLESTR16;
typedef LPCSTR		LPCOLESTR16;
typedef OLECHAR16	*BSTR16;
typedef BSTR16		*LPBSTR16;
#define OLESTR16(x) x

typedef WCHAR		OLECHAR;
typedef LPWSTR		LPOLESTR;
typedef LPCWSTR		LPCOLESTR;
typedef OLECHAR	*BSTR;
typedef BSTR		*LPBSTR;

#ifndef _DWORDLONG_
#define _DWORDLONG_
typedef __uint64 DWORDLONG, *PDWORDLONG;
#endif

#ifndef _ULONGLONG_
#define _ULONGLONG_
typedef __int64 LONGLONG, *PLONGLONG;
typedef __uint64 ULONGLONG, *PULONGLONG;
#endif

#define OLESTR(x) L##x

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
	STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE = 4,
	STGC_CONSOLIDATE = 8
} STGC;

typedef enum tagSTGMOVE
{   
	STGMOVE_MOVE = 0,
	STGMOVE_COPY = 1,
	STGMOVE_SHALLOWCOPY = 2
} STGMOVE;


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
    CLSCTX_INPROC_HANDLERX86 = 0x80,
    CLSCTX_ESERVER_HANDLER   = 0x100
} CLSCTX;

#define CLSCTX_INPROC           (CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER)
#define CLSCTX_ALL              (CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER)
#define CLSCTX_SERVER           (CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER)

typedef enum tagMSHLFLAGS
{
    MSHLFLAGS_NORMAL        = 0,
    MSHLFLAGS_TABLESTRONG   = 1,
    MSHLFLAGS_TABLEWEAK     = 2,
    MSHLFLAGS_NOPING        = 4
} MSHLFLAGS;

typedef enum tagMSHCTX
{
    MSHCTX_LOCAL            = 0,
    MSHCTX_NOSHAREDMEM      = 1,
    MSHCTX_DIFFERENTMACHINE = 2,
    MSHCTX_INPROC           = 3
} MSHCTX;

typedef unsigned short VARTYPE;

typedef ULONG PROPID;

#ifndef _tagBLOB_DEFINED
#define _tagBLOB_DEFINED
#define _BLOB_DEFINED
#define _LPBLOB_DEFINED
typedef struct tagBLOB
{
    ULONG cbSize;
    BYTE *pBlobData;
} BLOB, *LPBLOB;
#endif

#ifndef _tagCY_DEFINED
#define _tagCY_DEFINED

typedef union tagCY {
    struct {
#ifdef BIG_ENDIAN
        LONG  Hi;
        LONG  Lo;
#else /* defined(BIG_ENDIAN) */
        ULONG Lo;
        LONG  Hi;
#endif /* defined(BIG_ENDIAN) */
    } DUMMYSTRUCTNAME;
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
typedef WORD SECURITY_DESCRIPTOR_CONTROL, *PSECURITY_DESCRIPTOR_CONTROL;
typedef DWORD ACCESS_MASK, *PACCESS_MASK;

typedef PVOID PGENERIC_MAPPING;

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

#endif /* _SECURITY_DEFINED */

#ifndef _ROTFLAGS_DEFINED
#define _ROTFLAGS_DEFINED
#define ROTFLAGS_REGISTRATIONKEEPSALIVE 0x1
#define ROTFLAGS_ALLOWANYCLIENT 0x2
#endif /* !defined(_ROTFLAGS_DEFINED) */

#endif /* __WINE_WTYPES_H */
