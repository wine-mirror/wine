/* Definitions for the VERsion infolibrary (VER.DLL)
 * 
 * Copyright 1996 Marcus Meissner
 */
#ifndef __WINE_VER_H
#define __WINE_VER_H

#include "windows.h"

/* resource ids for different version infos */
#define	VS_FILE_INFO	MAKEINTRESOURCE(16)
#define	VS_VERSION_INFO	MAKEINTRESOURCE(1)
#define	VS_USER_INFO	MAKEINTRESOURCE(100)

#define	VS_FFI_SIGNATURE	0xfeef04bdL	/* FileInfo Magic */
#define	VS_FFI_STRUCVERSION	0x00010000L	/* struc version 1.0 */
#define	VS_FFI_FILEFLAGSMASK	0x0000003fL	/* valid flags */

/* VS_VERSION.dwFileFlags */
#define	VS_FF_DEBUG		0x01L
#define	VS_FF_PRERELEASE	0x02L
#define	VS_FF_PATCHED		0x04L
#define	VS_FF_PRIVATEBUILD	0x08L
#define	VS_FF_INFOINFERRED	0x10L
#define	VS_FF_SPECIALBUILD	0x20L

/* VS_VERSION.dwFileOS */

/* major os version */
#define	VOS_UNKNOWN		0x00000000L
#define	VOS_DOS			0x00010000L
#define	VOS_OS216		0x00020000L
#define	VOS_OS232		0x00030000L
#define	VOS_NT			0x00040000L

/* minor os version */
#define	VOS__BASE		0x00000000L
#define	VOS__WINDOWS16		0x00000001L
#define	VOS__PM16		0x00000002L
#define	VOS__PM32		0x00000003L
#define	VOS__WINDOWS32		0x00000004L

/* possible versions */
#define	VOS_DOS_WINDOWS16	(VOS_DOS|VOS__WINDOWS16)
#define	VOS_DOS_WINDOWS32	(VOS_DOS|VOS__WINDOWS32)
#define	VOS_OS216_PM16		(VOS_OS216|VOS__PM16)
#define	VOS_OS232_PM32		(VOS_OS232|VOS__PM32)
#define	VOS_NT_WINDOWS32	(VOS_NT|VOS__WINDOWS32)

/* VS_VERSION.dwFileType */
#define	VFT_UNKNOWN		0x00000000L
#define	VFT_APP			0x00000001L
#define	VFT_DLL			0x00000002L
#define	VFT_DRV			0x00000003L
#define	VFT_FONT		0x00000004L
#define	VFT_VXD			0x00000005L
/* ??one type missing??		0x00000006L -Marcus */
#define	VFT_STATIC_LIB		0x00000007L

/* VS_VERSION.dwFileSubtype for VFT_DRV */
#define	VFT2_UNKNOWN		0x00000000L
#define	VFT2_DRV_PRINTER	0x00000001L
#define	VFT2_DRV_KEYBOARD	0x00000002L
#define	VFT2_DRV_LANGUAGE	0x00000003L
#define	VFT2_DRV_DISPLAY	0x00000004L
#define	VFT2_DRV_MOUSE		0x00000005L
#define	VFT2_DRV_NETWORK	0x00000006L
#define	VFT2_DRV_SYSTEM		0x00000007L
#define	VFT2_DRV_INSTALLABLE	0x00000008L
#define	VFT2_DRV_SOUND		0x00000009L
#define	VFT2_DRV_COMM		0x0000000aL
#define	VFT2_DRV_INPUTMETHOD	0x0000000bL

/* VS_VERSION.dwFileSubtype for VFT_FONT */
#define	VFT2_FONT_RASTER	0x00000001L
#define	VFT2_FONT_VECTOR	0x00000002L
#define	VFT2_FONT_TRUETYPE	0x00000003L

/* VerFindFile Flags */
	/* input */
#define	VFFF_ISSHAREDFILE	0x0001

	/* output (returned) */
#define	VFF_CURNEDEST		0x0001
#define	VFF_FILEINUSE		0x0002
#define	VFF_BUFFTOSMALL		0x0003

/* VerInstallFile Flags */
	/* input */
#define	VIFF_FORCEINSTALL	0x0001
#define	VIFF_DONTDELETEOLD	0x0002

	/* output (return) */
#define	VIF_TEMPFILE		0x00000001L
#define	VIF_MISMATCH		0x00000002L
#define	VIF_SRCOLD		0x00000004L
#define	VIF_DIFFLANG		0x00000008L
#define	VIF_DIFFCODEPG		0x00000010L
#define	VIF_DIFFTYPE		0x00000020L
#define	VIF_WRITEPROT		0x00000040L
#define	VIF_FILEINUSE		0x00000080L
#define	VIF_OUTOFSPACE		0x00000100L
#define	VIF_ACCESSVIOLATION	0x00000200L
#define	VIF_SHARINGVIOLATION	0x00000400L
#define	VIF_CANNOTCREATE	0x00000800L
#define	VIF_CANNOTDELETE	0x00001000L
#define	VIF_CANNOTRENAME	0x00002000L
#define	VIF_CANNOTDELETECUR	0x00004000L
#define	VIF_OUTOFMEMORY		0x00008000L
#define	VIF_CANNOTREADSRC	0x00010000L
#define	VIF_CANNOTREADDST	0x00020000L
#define	VIF_BUFTOSMALL		0x00040000L

typedef struct tagVS_FIXEDFILEINFO {
	DWORD   dwSignature;
	DWORD   dwStrucVersion;
	DWORD   dwFileVersionMS;
	DWORD   dwFileVersionLS;
	DWORD   dwProductVersionMS;
	DWORD   dwProductVersionLS;
	DWORD   dwFileFlagsMask;
	DWORD   dwFileFlags;
	DWORD   dwFileOS;
	DWORD   dwFileType;
	DWORD   dwFileSubtype;
	DWORD   dwFileDateMS;
	DWORD   dwFileDateLS;
} VS_FIXEDFILEINFO;

/* following two aren't in version.dll */
DWORD
GetFileResourceSize(LPCSTR filename,SEGPTR restype,SEGPTR resid,LPDWORD off);

DWORD
GetFileResource(LPCSTR filename,SEGPTR restype,SEGPTR resid,
		DWORD off,DWORD reslen,LPVOID data
);

DWORD GetFileVersionInfoSize16(LPCSTR filename,LPDWORD handle);
DWORD GetFileVersionInfoSize32A(LPCSTR filename,LPDWORD handle);
DWORD GetFileVersionInfoSize32W(LPCWSTR filename,LPDWORD handle);
#define GetFileVersionInfoSize WINELIB_NAME_AW(GetFileVersionInfoSize)

DWORD GetFileVersionInfo16(LPCSTR filename,DWORD handle,DWORD datasize,LPVOID data);
DWORD GetFileVersionInfo32A(LPCSTR filename,DWORD handle,DWORD datasize,LPVOID data);
DWORD GetFileVersionInfo32W(LPCWSTR filename,DWORD handle,DWORD datasize,LPVOID data);
#define GetFileVersionInfo WINELIB_NAME_AW(GetFileVersionInfo)

DWORD
VerFindFile16(
	UINT16 flags,LPCSTR filename,LPCSTR windir,LPCSTR appdir,
	LPSTR curdir,UINT16 *curdirlen,LPSTR destdir,UINT16 *destdirlen
);
DWORD
VerFindFile32A(
	UINT32 flags,LPCSTR filename,LPCSTR windir,LPCSTR appdir,
	LPSTR curdir,UINT32 *curdirlen,LPSTR destdir,UINT32 *destdirlen
);
DWORD
VerFindFile32W(
	UINT32 flags,LPCWSTR filename,LPCWSTR windir,LPCWSTR appdir,
	LPWSTR curdir,UINT32 *curdirlen,LPWSTR destdir,UINT32 *destdirlen
);
#define VerFindFile WINELIB_NAME_AW(VerFindFile)

DWORD
VerInstallFile16(
	UINT16 flags,LPCSTR srcfilename,LPCSTR destfilename,LPCSTR srcdir,
	LPCSTR destdir,LPSTR tmpfile,UINT16 *tmpfilelen
);
DWORD
VerInstallFile32A(
	UINT32 flags,LPCSTR srcfilename,LPCSTR destfilename,LPCSTR srcdir,
	LPCSTR destdir,LPSTR tmpfile,UINT32 *tmpfilelen
);
DWORD
VerInstallFile32W(
	UINT32 flags,LPCWSTR srcfilename,LPCWSTR destfilename,LPCWSTR srcdir,
	LPCWSTR destdir,LPWSTR tmpfile,UINT32 *tmpfilelen
);
#define VerInstallFile WINELIB_NAME_AW(VerInstallFile)

DWORD VerLanguageName16(UINT16 lang,LPSTR langname,UINT16 langnamelen);
DWORD VerLanguageName32A(UINT32 lang,LPSTR langname,UINT32 langnamelen);
DWORD VerLanguageName32W(UINT32 lang,LPWSTR langname,UINT32 langnamelen);
#define VerLanguageName WINELIB_NAME_AW(VerLanguageName)

DWORD VerQueryValue16(SEGPTR block,LPCSTR subblock,SEGPTR *buffer,UINT16 *buflen);
DWORD VerQueryValue32A(LPVOID block,LPCSTR subblock,LPVOID *buffer,UINT32 *buflen);
DWORD VerQueryValue32W(LPVOID block,LPCWSTR subblock,LPVOID *buffer,UINT32 *buflen);
#define VerQueryValue WINELIB_NAME_AW(VerQueryValue)

/* 20 GETFILEVERSIONINFORAW */

#endif	/* __WINE_VER_H */
