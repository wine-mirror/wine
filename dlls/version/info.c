/* 
 * Implementation of VERSION.DLL - Version Info access
 * 
 * Copyright 1996,1997 Marcus Meissner
 * Copyright 1997 David Cuthbert
 * Copyright 1999 Ulrich Weigand
 */

#include <stdlib.h>
#include <string.h>

#include "winreg.h"
#include "winver.h"
#include "wine/winuser16.h"
#include "wine/unicode.h"
#include "winerror.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ver);


/******************************************************************************
 *
 *   This function will print via dprintf[_]ver to stddeb debug info regarding
 *   the file info structure vffi.
 *      15-Feb-1998 Dimitrie Paun (dimi@cs.toronto.edu)
 *      Added this function to clean up the code.
 *
 *****************************************************************************/
static void print_vffi_debug(VS_FIXEDFILEINFO *vffi)
{
	TRACE(" structversion=%u.%u, fileversion=%u.%u.%u.%u, productversion=%u.%u.%u.%u, flagmask=0x%lx, flags=%s%s%s%s%s%s\n",
		    HIWORD(vffi->dwStrucVersion),LOWORD(vffi->dwStrucVersion),
		    HIWORD(vffi->dwFileVersionMS),LOWORD(vffi->dwFileVersionMS),
		    HIWORD(vffi->dwFileVersionLS),LOWORD(vffi->dwFileVersionLS),
		    HIWORD(vffi->dwProductVersionMS),LOWORD(vffi->dwProductVersionMS),
		    HIWORD(vffi->dwProductVersionLS),LOWORD(vffi->dwProductVersionLS),
		    vffi->dwFileFlagsMask,
		    (vffi->dwFileFlags & VS_FF_DEBUG) ? "DEBUG," : "",
		    (vffi->dwFileFlags & VS_FF_PRERELEASE) ? "PRERELEASE," : "",
		    (vffi->dwFileFlags & VS_FF_PATCHED) ? "PATCHED," : "",
		    (vffi->dwFileFlags & VS_FF_PRIVATEBUILD) ? "PRIVATEBUILD," : "",
		    (vffi->dwFileFlags & VS_FF_INFOINFERRED) ? "INFOINFERRED," : "",
		    (vffi->dwFileFlags & VS_FF_SPECIALBUILD) ? "SPECIALBUILD," : ""
		    );

        TRACE("(");
	DPRINTF(" OS=0x%x.0x%x ",
		HIWORD(vffi->dwFileOS),
		LOWORD(vffi->dwFileOS)
	);
	switch (vffi->dwFileOS&0xFFFF0000) {
	case VOS_DOS:DPRINTF("DOS,");break;
	case VOS_OS216:DPRINTF("OS/2-16,");break;
	case VOS_OS232:DPRINTF("OS/2-32,");break;
	case VOS_NT:DPRINTF("NT,");break;
	case VOS_UNKNOWN:
	default:
		DPRINTF("UNKNOWN(0x%lx),",vffi->dwFileOS&0xFFFF0000);break;
	}
	switch (LOWORD(vffi->dwFileOS)) {
	case VOS__BASE:DPRINTF("BASE");break;
	case VOS__WINDOWS16:DPRINTF("WIN16");break;
	case VOS__WINDOWS32:DPRINTF("WIN32");break;
	case VOS__PM16:DPRINTF("PM16");break;
	case VOS__PM32:DPRINTF("PM32");break;
	default:DPRINTF("UNKNOWN(0x%x)",LOWORD(vffi->dwFileOS));break;
	}
	DPRINTF(")\n");

	switch (vffi->dwFileType) {
	default:
	case VFT_UNKNOWN:
		TRACE("filetype=Unknown(0x%lx)",vffi->dwFileType);
		break;
	case VFT_APP:TRACE("filetype=APP,");break;
	case VFT_DLL:TRACE("filetype=DLL,");break;
	case VFT_DRV:
		TRACE("filetype=DRV,");
		switch(vffi->dwFileSubtype) {
		default:
		case VFT2_UNKNOWN:
			DPRINTF("UNKNOWN(0x%lx)",vffi->dwFileSubtype);
			break;
		case VFT2_DRV_PRINTER:
			DPRINTF("PRINTER");
			break;
		case VFT2_DRV_KEYBOARD:
			DPRINTF("KEYBOARD");
			break;
		case VFT2_DRV_LANGUAGE:
			DPRINTF("LANGUAGE");
			break;
		case VFT2_DRV_DISPLAY:
			DPRINTF("DISPLAY");
			break;
		case VFT2_DRV_MOUSE:
			DPRINTF("MOUSE");
			break;
		case VFT2_DRV_NETWORK:
			DPRINTF("NETWORK");
			break;
		case VFT2_DRV_SYSTEM:
			DPRINTF("SYSTEM");
			break;
		case VFT2_DRV_INSTALLABLE:
			DPRINTF("INSTALLABLE");
			break;
		case VFT2_DRV_SOUND:
			DPRINTF("SOUND");
			break;
		case VFT2_DRV_COMM:
			DPRINTF("COMM");
			break;
		case VFT2_DRV_INPUTMETHOD:
			DPRINTF("INPUTMETHOD");
			break;
		}
		break;
	case VFT_FONT:
                TRACE("filetype=FONT.");
		switch (vffi->dwFileSubtype) {
		default:
			DPRINTF("UNKNOWN(0x%lx)",vffi->dwFileSubtype);
			break;
		case VFT2_FONT_RASTER:DPRINTF("RASTER");break;
		case VFT2_FONT_VECTOR:DPRINTF("VECTOR");break;
		case VFT2_FONT_TRUETYPE:DPRINTF("TRUETYPE");break;
		}
		break;
	case VFT_VXD:TRACE("filetype=VXD");break;
	case VFT_STATIC_LIB:TRACE("filetype=STATIC_LIB");break;
	}
        DPRINTF("\n");
	TRACE("  filedata=0x%lx.0x%lx\n",
		    vffi->dwFileDateMS,vffi->dwFileDateLS);
}


/***********************************************************************
 * Version Info Structure
 */

typedef struct
{
    WORD  wLength;
    WORD  wValueLength;
    CHAR  szKey[1];
#if 0   /* variable length structure */
    /* DWORD aligned */
    BYTE  Value[];
    /* DWORD aligned */
    VS_VERSION_INFO_STRUCT16 Children[];
#endif
} VS_VERSION_INFO_STRUCT16;

typedef struct
{
    WORD  wLength;
    WORD  wValueLength;
    WORD  bText;
    WCHAR szKey[1];
#if 0   /* variable length structure */
    /* DWORD aligned */
    BYTE  Value[];
    /* DWORD aligned */
    VS_VERSION_INFO_STRUCT32 Children[];
#endif
} VS_VERSION_INFO_STRUCT32;

#define VersionInfoIs16( ver ) \
    ( ((VS_VERSION_INFO_STRUCT16 *)ver)->szKey[0] >= ' ' )

#define DWORD_ALIGN( base, ptr ) \
    ( (LPBYTE)(base) + ((((LPBYTE)(ptr) - (LPBYTE)(base)) + 3) & ~3) )

#define VersionInfo16_Value( ver )  \
    DWORD_ALIGN( (ver), (ver)->szKey + strlen((ver)->szKey) + 1 )
#define VersionInfo32_Value( ver )  \
    DWORD_ALIGN( (ver), (ver)->szKey + strlenW((ver)->szKey) + 1 )

#define VersionInfo16_Children( ver )  \
    (VS_VERSION_INFO_STRUCT16 *)( VersionInfo16_Value( ver ) + \
                           ( ( (ver)->wValueLength + 3 ) & ~3 ) )
#define VersionInfo32_Children( ver )  \
    (VS_VERSION_INFO_STRUCT32 *)( VersionInfo32_Value( ver ) + \
                           ( ( (ver)->wValueLength * \
                               ((ver)->bText? 2 : 1) + 3 ) & ~3 ) )

#define VersionInfo16_Next( ver ) \
    (VS_VERSION_INFO_STRUCT16 *)( (LPBYTE)ver + (((ver)->wLength + 3) & ~3) )
#define VersionInfo32_Next( ver ) \
    (VS_VERSION_INFO_STRUCT32 *)( (LPBYTE)ver + (((ver)->wLength + 3) & ~3) )

/***********************************************************************
 *           ConvertVersionInfo32To16        [internal]
 */
void ConvertVersionInfo32To16( VS_VERSION_INFO_STRUCT32 *info32, 
                               VS_VERSION_INFO_STRUCT16 *info16 )
{
    /* Copy data onto local stack to prevent overwrites */
    WORD wLength = info32->wLength;
    WORD wValueLength = info32->wValueLength;
    WORD bText = info32->bText;
    LPBYTE lpValue = VersionInfo32_Value( info32 );
    VS_VERSION_INFO_STRUCT32 *child32 = VersionInfo32_Children( info32 );
    VS_VERSION_INFO_STRUCT16 *child16;

    TRACE("Converting %p to %p\n", info32, info16 );
    TRACE("wLength %d, wValueLength %d, bText %d, value %p, child %p\n",
                wLength, wValueLength, bText, lpValue, child32 );

    /* Convert key */
    WideCharToMultiByte( CP_ACP, 0, info32->szKey, -1, info16->szKey, 0x7fffffff, NULL, NULL );

    TRACE("Copied key from %p to %p: %s\n", info32->szKey, info16->szKey,
                debugstr_a(info16->szKey) );

    /* Convert value */
    if ( wValueLength == 0 )
    {
        info16->wValueLength = 0;
        TRACE("No value present\n" );
    }
    else if ( bText )
    {
        info16->wValueLength = WideCharToMultiByte( CP_ACP, 0, (LPCWSTR)lpValue, -1, NULL, 0, NULL, NULL );
        WideCharToMultiByte( CP_ACP, 0, (LPCWSTR)lpValue, -1,
                             VersionInfo16_Value( info16 ), info16->wValueLength, NULL, NULL );

        TRACE("Copied value from %p to %p: %s\n", lpValue, 
                    VersionInfo16_Value( info16 ), 
                    debugstr_a(VersionInfo16_Value( info16 )) );
    }
    else
    {
        info16->wValueLength = wValueLength;
        memmove( VersionInfo16_Value( info16 ), lpValue, wValueLength );

        TRACE("Copied value from %p to %p: %d bytes\n", lpValue, 
                     VersionInfo16_Value( info16 ), wValueLength );
    }

    /* Convert children */
    child16 = VersionInfo16_Children( info16 );
    while ( (DWORD)child32 < (DWORD)info32 + wLength )
    {
        VS_VERSION_INFO_STRUCT32 *nextChild = VersionInfo32_Next( child32 );

        ConvertVersionInfo32To16( child32, child16 );

        child16 = VersionInfo16_Next( child16 );
        child32 = nextChild;
    }

    /* Fixup length */
    info16->wLength = (DWORD)child16 - (DWORD)info16;

    TRACE("Finished, length is %d (%p - %p)\n", 
                info16->wLength, info16, child16 );
}

/***********************************************************************
 *           VERSION_GetFileVersionInfo_PE             [internal]
 *
 *    NOTE: returns size of the PE VERSION resource or 0xFFFFFFFF
 *    in the case if file exists, but VERSION_INFO not found.
 *    FIXME: handle is not used.
 */
static DWORD WINAPI VERSION_GetFileVersionInfo_PE( LPCSTR filename, LPDWORD handle,
                                    DWORD datasize, LPVOID data )
{
    VS_FIXEDFILEINFO *vffi;
    DWORD len;
    BYTE *buf;
    HMODULE hModule;
    HRSRC hRsrc;
    HGLOBAL hMem;
    BOOL do_free_library = FALSE;

    TRACE("(%s,%p)\n", debugstr_a(filename), handle );

    hModule = GetModuleHandleA(filename);
    if(!hModule)
    {
	hModule = LoadLibraryExA(filename, 0, LOAD_LIBRARY_AS_DATAFILE);
	do_free_library = TRUE;
    }
    if(!hModule)
    {
	WARN("Could not load %s\n", debugstr_a(filename));
	return 0;
    }
    hRsrc = FindResourceW(hModule,
			  MAKEINTRESOURCEW(VS_VERSION_INFO),
			  MAKEINTRESOURCEW(VS_FILE_INFO));
    if(!hRsrc)
    {
	WARN("Could not find VS_VERSION_INFO in %s\n", debugstr_a(filename));
	if(do_free_library) FreeLibrary(hModule);
	return 0xFFFFFFFF;
    }
    len = SizeofResource(hModule, hRsrc);
    hMem = LoadResource(hModule, hRsrc);
    if(!hMem)
    {
	WARN("Could not load VS_VERSION_INFO from %s\n", debugstr_a(filename));
	if(do_free_library) FreeLibrary(hModule);
	return 0xFFFFFFFF;
    }
    buf = LockResource(hMem);

    vffi = (VS_FIXEDFILEINFO *)VersionInfo32_Value( (VS_VERSION_INFO_STRUCT32 *)buf );

    if ( vffi->dwSignature != VS_FFI_SIGNATURE )
    {
        WARN("vffi->dwSignature is 0x%08lx, but not 0x%08lx!\n",
                   vffi->dwSignature, VS_FFI_SIGNATURE );
	len = 0xFFFFFFFF;
	goto END;
    }

    if ( TRACE_ON(ver) )
        print_vffi_debug( vffi );

    if(data)
    {
	if(datasize < len)
	    len = datasize; /* truncate data */
	if(len)
	    memcpy(data, buf, len);
	else
	    len = 0xFFFFFFFF;
    }
END:
    FreeResource(hMem);
    if(do_free_library) FreeLibrary(hModule);

    return len;
}

/***********************************************************************
 *           VERSION_GetFileVersionInfo_16             [internal]
 *
 *    NOTE: returns size of the 16-bit VERSION resource or 0xFFFFFFFF
 *    in the case if file exists, but VERSION_INFO not found.
 *    FIXME: handle is not used.
 */
static DWORD WINAPI VERSION_GetFileVersionInfo_16( LPCSTR filename, LPDWORD handle,
                                    DWORD datasize, LPVOID data )
{
    VS_FIXEDFILEINFO *vffi;
    DWORD len;
    BYTE *buf;
    HMODULE16 hModule;
    HRSRC16 hRsrc;
    HGLOBAL16 hMem;
    BOOL do_free_library = FALSE;

    TRACE("(%s,%p)\n", debugstr_a(filename), handle );

    hModule = GetModuleHandle16(filename);
    if(hModule < 32)
    {
	hModule = LoadLibrary16(filename);
	do_free_library = TRUE;
    }
    if(hModule < 32)
    {
	WARN("Could not load %s\n", debugstr_a(filename));
	return 0;
    }
    hRsrc = FindResource16(hModule,
			  MAKEINTRESOURCEA(VS_VERSION_INFO),
			  MAKEINTRESOURCEA(VS_FILE_INFO));
    if(!hRsrc)
    {
	WARN("Could not find VS_VERSION_INFO in %s\n", debugstr_a(filename));
	if(do_free_library) FreeLibrary16(hModule);
	return 0xFFFFFFFF;
    }
    len = SizeofResource16(hModule, hRsrc);
    hMem = LoadResource16(hModule, hRsrc);
    if(!hMem)
    {
	WARN("Could not load VS_VERSION_INFO from %s\n", debugstr_a(filename));
	if(do_free_library) FreeLibrary16(hModule);
	return 0xFFFFFFFF;
    }
    buf = LockResource16(hMem);

    if(!VersionInfoIs16(buf))
	goto END;

    vffi = (VS_FIXEDFILEINFO *)VersionInfo16_Value( (VS_VERSION_INFO_STRUCT16 *)buf );

    if ( vffi->dwSignature != VS_FFI_SIGNATURE )
    {
        WARN("vffi->dwSignature is 0x%08lx, but not 0x%08lx!\n",
                   vffi->dwSignature, VS_FFI_SIGNATURE );
	len = 0xFFFFFFFF;
	goto END;
    }

    if ( TRACE_ON(ver) )
        print_vffi_debug( vffi );

    if(data)
    {
	if(datasize < len)
	    len = datasize; /* truncate data */
	if(len)
	    memcpy(data, buf, len);
	else
	    len = 0xFFFFFFFF;
    }
END:
    FreeResource16(hMem);
    if(do_free_library) FreeLibrary16(hModule);

    return len;
}

/***********************************************************************
 *           GetFileVersionInfoSizeA         [VERSION.2]
 */
DWORD WINAPI GetFileVersionInfoSizeA( LPCSTR filename, LPDWORD handle )
{
    VS_FIXEDFILEINFO *vffi;
    DWORD len, ret, offset;
    BYTE buf[144];

    TRACE("(%s,%p)\n", debugstr_a(filename), handle );

    len = VERSION_GetFileVersionInfo_PE(filename, handle, 0, NULL);
    /* 0xFFFFFFFF means: file exists, but VERSION_INFO not found */
    if(len == 0xFFFFFFFF) return 0;
    if(len) return len;
    len = VERSION_GetFileVersionInfo_16(filename, handle, 0, NULL);
    /* 0xFFFFFFFF means: file exists, but VERSION_INFO not found */
    if(len == 0xFFFFFFFF) return 0;
    if(len) return len;

    len = GetFileResourceSize16( filename,
                                 MAKEINTRESOURCEA(VS_FILE_INFO),
                                 MAKEINTRESOURCEA(VS_VERSION_INFO),
                                 &offset );
    if (!len) return 0;

    ret = GetFileResource16( filename,
                             MAKEINTRESOURCEA(VS_FILE_INFO),
                             MAKEINTRESOURCEA(VS_VERSION_INFO),
                             offset, sizeof( buf ), buf );
    if (!ret) return 0;

    if ( handle ) *handle = offset;
    
    if ( VersionInfoIs16( buf ) )
        vffi = (VS_FIXEDFILEINFO *)VersionInfo16_Value( (VS_VERSION_INFO_STRUCT16 *)buf );
    else
        vffi = (VS_FIXEDFILEINFO *)VersionInfo32_Value( (VS_VERSION_INFO_STRUCT32 *)buf );

    if ( vffi->dwSignature != VS_FFI_SIGNATURE )
    {
        WARN("vffi->dwSignature is 0x%08lx, but not 0x%08lx!\n",
                   vffi->dwSignature, VS_FFI_SIGNATURE );
        return 0;
    }

    if ( ((VS_VERSION_INFO_STRUCT16 *)buf)->wLength < len )
        len = ((VS_VERSION_INFO_STRUCT16 *)buf)->wLength;

    if ( TRACE_ON(ver) )
        print_vffi_debug( vffi );

    return len;
}

/***********************************************************************
 *           GetFileVersionInfoSizeW         [VERSION.3]
 */
DWORD WINAPI GetFileVersionInfoSizeW( LPCWSTR filename, LPDWORD handle )
{
    DWORD ret, len = WideCharToMultiByte( CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL );
    LPSTR fn = HeapAlloc( GetProcessHeap(), 0, len );
    WideCharToMultiByte( CP_ACP, 0, filename, -1, fn, len, NULL, NULL );
    ret = GetFileVersionInfoSizeA( fn, handle );
    HeapFree( GetProcessHeap(), 0, fn );
    return ret;
}

/***********************************************************************
 *           GetFileVersionInfoA             [VERSION.1]
 */
BOOL WINAPI GetFileVersionInfoA( LPCSTR filename, DWORD handle,
                                    DWORD datasize, LPVOID data )
{
    DWORD len;

    TRACE("(%s,%ld,size=%ld,data=%p)\n",
                debugstr_a(filename), handle, datasize, data );

    len = VERSION_GetFileVersionInfo_PE(filename, &handle, datasize, data);
    /* 0xFFFFFFFF means: file exists, but VERSION_INFO not found */
    if(len == 0xFFFFFFFF) return FALSE;
    if(len)
	goto DO_CONVERT;
    len = VERSION_GetFileVersionInfo_16(filename, &handle, datasize, data);
    /* 0xFFFFFFFF means: file exists, but VERSION_INFO not found */
    if(len == 0xFFFFFFFF) return FALSE;
    if(len)
	goto DO_CONVERT;

    if ( !GetFileResource16( filename, MAKEINTRESOURCEA(VS_FILE_INFO),
                                       MAKEINTRESOURCEA(VS_VERSION_INFO),
                                       handle, datasize, data ) )
        return FALSE;
DO_CONVERT:
    if (    datasize >= sizeof(VS_VERSION_INFO_STRUCT16)
         && datasize >= ((VS_VERSION_INFO_STRUCT16 *)data)->wLength  
         && !VersionInfoIs16( data ) )
    {
        /* convert resource from PE format to NE format */
        ConvertVersionInfo32To16( (VS_VERSION_INFO_STRUCT32 *)data, 
                                  (VS_VERSION_INFO_STRUCT16 *)data );
    }

    return TRUE;
}

/***********************************************************************
 *           GetFileVersionInfoW             [VERSION.4]
 */
BOOL WINAPI GetFileVersionInfoW( LPCWSTR filename, DWORD handle,
                                    DWORD datasize, LPVOID data )
{
    DWORD len = WideCharToMultiByte( CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL );
    LPSTR fn = HeapAlloc( GetProcessHeap(), 0, len );
    DWORD retv = TRUE;

    WideCharToMultiByte( CP_ACP, 0, filename, -1, fn, len, NULL, NULL );

    TRACE("(%s,%ld,size=%ld,data=%p)\n",
                debugstr_w(filename), handle, datasize, data );

    if(VERSION_GetFileVersionInfo_PE(fn, &handle, datasize, data))
	goto END;
    if(VERSION_GetFileVersionInfo_16(fn, &handle, datasize, data))
	goto END;

    if ( !GetFileResource16( fn, MAKEINTRESOURCEA(VS_FILE_INFO),
                                 MAKEINTRESOURCEA(VS_VERSION_INFO),
                                 handle, datasize, data ) )
        retv = FALSE;

    else if (    datasize >= sizeof(VS_VERSION_INFO_STRUCT16)
              && datasize >= ((VS_VERSION_INFO_STRUCT16 *)data)->wLength 
              && VersionInfoIs16( data ) )
    {
        ERR("Cannot access NE resource in %s\n", debugstr_a(fn) );
        retv =  FALSE;
    }
END:
    HeapFree( GetProcessHeap(), 0, fn );
    return retv;
}


/***********************************************************************
 *           VersionInfo16_FindChild             [internal]
 */
static VS_VERSION_INFO_STRUCT16 *VersionInfo16_FindChild( VS_VERSION_INFO_STRUCT16 *info, 
                                            LPCSTR szKey, UINT cbKey )
{
    VS_VERSION_INFO_STRUCT16 *child = VersionInfo16_Children( info );

    while ( (DWORD)child < (DWORD)info + info->wLength )
    {
        if ( !strncasecmp( child->szKey, szKey, cbKey ) )
            return child;

	if (!(child->wLength)) return NULL;
        child = VersionInfo16_Next( child );
    }

    return NULL;
}

/***********************************************************************
 *           VersionInfo32_FindChild             [internal]
 */
static VS_VERSION_INFO_STRUCT32 *VersionInfo32_FindChild( VS_VERSION_INFO_STRUCT32 *info, 
                                            LPCWSTR szKey, UINT cbKey )
{
    VS_VERSION_INFO_STRUCT32 *child = VersionInfo32_Children( info );

    while ( (DWORD)child < (DWORD)info + info->wLength )
    {
        if ( !strncmpiW( child->szKey, szKey, cbKey ) )
            return child;

        child = VersionInfo32_Next( child );
    }

    return NULL;
}

/***********************************************************************
 *           VerQueryValueA              [VERSION.12]
 */
DWORD WINAPI VerQueryValueA( LPVOID pBlock, LPCSTR lpSubBlock,
                               LPVOID *lplpBuffer, UINT *puLen )
{
    VS_VERSION_INFO_STRUCT16 *info = (VS_VERSION_INFO_STRUCT16 *)pBlock;
    if ( !VersionInfoIs16( info ) )
    {
        INT len;
        LPWSTR wide_str;
        DWORD give;

        /* <lawson_whitney@juno.com> Feb 2001 */
        /* AOL 5.0 does this, expecting to get this: */
        len = MultiByteToWideChar(CP_ACP, 0, lpSubBlock, -1, NULL, 0);
        wide_str = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpSubBlock, -1, wide_str, len);

        give = VerQueryValueW(pBlock, wide_str, lplpBuffer, puLen);
        HeapFree(GetProcessHeap(), 0, wide_str);
        return give;
    }

    TRACE("(%p,%s,%p,%p)\n",
                pBlock, debugstr_a(lpSubBlock), lplpBuffer, puLen );

    while ( *lpSubBlock )
    {
        /* Find next path component */
        LPCSTR lpNextSlash;
        for ( lpNextSlash = lpSubBlock; *lpNextSlash; lpNextSlash++ )
            if ( *lpNextSlash == '\\' )
                break;

        /* Skip empty components */
        if ( lpNextSlash == lpSubBlock )
        {
            lpSubBlock++;
            continue;
        }

        /* We have a non-empty component: search info for key */
        info = VersionInfo16_FindChild( info, lpSubBlock, lpNextSlash-lpSubBlock );
        if ( !info ) return FALSE;

        /* Skip path component */
        lpSubBlock = lpNextSlash;
    }

    /* Return value */
    *lplpBuffer = VersionInfo16_Value( info );
    *puLen = info->wValueLength;

    return TRUE;
}

/***********************************************************************
 *           VerQueryValueW              [VERSION.13]
 */
DWORD WINAPI VerQueryValueW( LPVOID pBlock, LPCWSTR lpSubBlock,
                               LPVOID *lplpBuffer, UINT *puLen )
{
    VS_VERSION_INFO_STRUCT32 *info = (VS_VERSION_INFO_STRUCT32 *)pBlock;
    if ( VersionInfoIs16( info ) )
    {
        ERR("called on NE resource!\n" );
        return FALSE;
    }

    TRACE("(%p,%s,%p,%p)\n",
                pBlock, debugstr_w(lpSubBlock), lplpBuffer, puLen );

    while ( *lpSubBlock )
    {
        /* Find next path component */
        LPCWSTR lpNextSlash;
        for ( lpNextSlash = lpSubBlock; *lpNextSlash; lpNextSlash++ )
            if ( *lpNextSlash == '\\' )
                break;

        /* Skip empty components */
        if ( lpNextSlash == lpSubBlock )
        {
            lpSubBlock++;
            continue;
        }

        /* We have a non-empty component: search info for key */
        info = VersionInfo32_FindChild( info, lpSubBlock, lpNextSlash-lpSubBlock );
        if ( !info ) return FALSE;

        /* Skip path component */
        lpSubBlock = lpNextSlash;
    }

    /* Return value */
    *lplpBuffer = VersionInfo32_Value( info );
    *puLen = info->wValueLength;

    return TRUE;
}

