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
#include "wine/winestring.h"
#include "winerror.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ver)


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
        dbg_decl_str(ver, 1024);

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

	dsprintf(ver," OS=0x%x.0x%x ",
		HIWORD(vffi->dwFileOS),
		LOWORD(vffi->dwFileOS)
	);
	switch (vffi->dwFileOS&0xFFFF0000) {
	case VOS_DOS:dsprintf(ver,"DOS,");break;
	case VOS_OS216:dsprintf(ver,"OS/2-16,");break;
	case VOS_OS232:dsprintf(ver,"OS/2-32,");break;
	case VOS_NT:dsprintf(ver,"NT,");break;
	case VOS_UNKNOWN:
	default:
		dsprintf(ver,"UNKNOWN(0x%lx),",vffi->dwFileOS&0xFFFF0000);break;
	}
	switch (LOWORD(vffi->dwFileOS)) {
	case VOS__BASE:dsprintf(ver,"BASE");break;
	case VOS__WINDOWS16:dsprintf(ver,"WIN16");break;
	case VOS__WINDOWS32:dsprintf(ver,"WIN32");break;
	case VOS__PM16:dsprintf(ver,"PM16");break;
	case VOS__PM32:dsprintf(ver,"PM32");break;
	default:dsprintf(ver,"UNKNOWN(0x%x)",LOWORD(vffi->dwFileOS));break;
	}
	TRACE("(%s)\n", dbg_str(ver));

	dbg_reset_str(ver);
	switch (vffi->dwFileType) {
	default:
	case VFT_UNKNOWN:
		dsprintf(ver,"filetype=Unknown(0x%lx)",vffi->dwFileType);
		break;
	case VFT_APP:dsprintf(ver,"filetype=APP,");break;
	case VFT_DLL:dsprintf(ver,"filetype=DLL,");break;
	case VFT_DRV:
		dsprintf(ver,"filetype=DRV,");
		switch(vffi->dwFileSubtype) {
		default:
		case VFT2_UNKNOWN:
			dsprintf(ver,"UNKNOWN(0x%lx)",vffi->dwFileSubtype);
			break;
		case VFT2_DRV_PRINTER:
			dsprintf(ver,"PRINTER");
			break;
		case VFT2_DRV_KEYBOARD:
			dsprintf(ver,"KEYBOARD");
			break;
		case VFT2_DRV_LANGUAGE:
			dsprintf(ver,"LANGUAGE");
			break;
		case VFT2_DRV_DISPLAY:
			dsprintf(ver,"DISPLAY");
			break;
		case VFT2_DRV_MOUSE:
			dsprintf(ver,"MOUSE");
			break;
		case VFT2_DRV_NETWORK:
			dsprintf(ver,"NETWORK");
			break;
		case VFT2_DRV_SYSTEM:
			dsprintf(ver,"SYSTEM");
			break;
		case VFT2_DRV_INSTALLABLE:
			dsprintf(ver,"INSTALLABLE");
			break;
		case VFT2_DRV_SOUND:
			dsprintf(ver,"SOUND");
			break;
		case VFT2_DRV_COMM:
			dsprintf(ver,"COMM");
			break;
		case VFT2_DRV_INPUTMETHOD:
			dsprintf(ver,"INPUTMETHOD");
			break;
		}
		break;
	case VFT_FONT:
		dsprintf(ver,"filetype=FONT.");
		switch (vffi->dwFileSubtype) {
		default:
			dsprintf(ver,"UNKNOWN(0x%lx)",vffi->dwFileSubtype);
			break;
		case VFT2_FONT_RASTER:dsprintf(ver,"RASTER");break;
		case VFT2_FONT_VECTOR:dsprintf(ver,"VECTOR");break;
		case VFT2_FONT_TRUETYPE:dsprintf(ver,"TRUETYPE");break;
		}
		break;
	case VFT_VXD:dsprintf(ver,"filetype=VXD");break;
	case VFT_STATIC_LIB:dsprintf(ver,"filetype=STATIC_LIB");break;
	}
	TRACE("%s\n", dbg_str(ver));

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

#define VersionInfo16_Value( ver )  \
    (LPBYTE)( ((DWORD)((ver)->szKey) + (lstrlenA((ver)->szKey)+1) + 3) & ~3 )
#define VersionInfo32_Value( ver )  \
    (LPBYTE)( ((DWORD)((ver)->szKey) + 2*(lstrlenW((ver)->szKey)+1) + 3) & ~3 )

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
    lstrcpyWtoA( info16->szKey, info32->szKey );

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
        info16->wValueLength = lstrlenW( (LPCWSTR)lpValue ) + 1;
        lstrcpyWtoA( VersionInfo16_Value( info16 ), (LPCWSTR)lpValue );

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
 *           GetFileVersionInfoSize32A         [VERSION.2]
 */
DWORD WINAPI GetFileVersionInfoSizeA( LPCSTR filename, LPDWORD handle )
{
    VS_FIXEDFILEINFO *vffi;
    DWORD len, ret, offset;
    BYTE buf[144];

    TRACE("(%s,%p)\n", debugstr_a(filename), handle );

    len = GetFileResourceSize( filename,
                                 MAKEINTRESOURCEA(VS_FILE_INFO),
                                 MAKEINTRESOURCEA(VS_VERSION_INFO),
                                 &offset );
    if (!len) return 0;

    ret = GetFileResource( filename,
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
 *           GetFileVersionInfoSize32W         [VERSION.3]
 */
DWORD WINAPI GetFileVersionInfoSizeW( LPCWSTR filename, LPDWORD handle )
{
    LPSTR fn = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    DWORD ret = GetFileVersionInfoSizeA( fn, handle );
    HeapFree( GetProcessHeap(), 0, fn );
    return ret;
}

/***********************************************************************
 *           GetFileVersionInfo32A             [VERSION.1]
 */
DWORD WINAPI GetFileVersionInfoA( LPCSTR filename, DWORD handle,
                                    DWORD datasize, LPVOID data )
{
    TRACE("(%s,%ld,size=%ld,data=%p)\n",
                debugstr_a(filename), handle, datasize, data );

    if ( !GetFileResource( filename, MAKEINTRESOURCEA(VS_FILE_INFO),
                                       MAKEINTRESOURCEA(VS_VERSION_INFO),
                                       handle, datasize, data ) )
        return FALSE;

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
 *           GetFileVersionInfo32W             [VERSION.4]
 */
DWORD WINAPI GetFileVersionInfoW( LPCWSTR filename, DWORD handle,
                                    DWORD datasize, LPVOID data )
{
    LPSTR fn = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    DWORD retv = TRUE;

    TRACE("(%s,%ld,size=%ld,data=%p)\n",
                debugstr_a(fn), handle, datasize, data );

    if ( !GetFileResource( fn, MAKEINTRESOURCEA(VS_FILE_INFO),
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

    HeapFree( GetProcessHeap(), 0, fn );
    return retv;
}


/***********************************************************************
 *           VersionInfo16_FindChild             [internal]
 */
VS_VERSION_INFO_STRUCT16 *VersionInfo16_FindChild( VS_VERSION_INFO_STRUCT16 *info, 
                                            LPCSTR szKey, UINT cbKey )
{
    VS_VERSION_INFO_STRUCT16 *child = VersionInfo16_Children( info );

    while ( (DWORD)child < (DWORD)info + info->wLength )
    {
        if ( !lstrncmpiA( child->szKey, szKey, cbKey ) )
            return child;

	if (!(child->wLength)) return NULL;
        child = VersionInfo16_Next( child );
    }

    return NULL;
}

/***********************************************************************
 *           VersionInfo32_FindChild             [internal]
 */
VS_VERSION_INFO_STRUCT32 *VersionInfo32_FindChild( VS_VERSION_INFO_STRUCT32 *info, 
                                            LPCWSTR szKey, UINT cbKey )
{
    VS_VERSION_INFO_STRUCT32 *child = VersionInfo32_Children( info );

    while ( (DWORD)child < (DWORD)info + info->wLength )
    {
        if ( !lstrncmpiW( child->szKey, szKey, cbKey ) )
            return child;

        child = VersionInfo32_Next( child );
    }

    return NULL;
}

/***********************************************************************
 *           VerQueryValue32A              [VERSION.12]
 */
DWORD WINAPI VerQueryValueA( LPVOID pBlock, LPCSTR lpSubBlock,
                               LPVOID *lplpBuffer, UINT *puLen )
{
    VS_VERSION_INFO_STRUCT16 *info = (VS_VERSION_INFO_STRUCT16 *)pBlock;
    if ( !VersionInfoIs16( info ) )
    {
        ERR("called on PE resource!\n" );
        return FALSE;
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
 *           VerQueryValue32W              [VERSION.13]
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

extern LPCSTR WINE_GetLanguageName( UINT langid );

/***********************************************************************
 *           VerLanguageName32A              [VERSION.9]
 */
DWORD WINAPI VerLanguageNameA( UINT wLang, LPSTR szLang, UINT nSize )
{
    char    buffer[80];
    LPCSTR  name;
    DWORD   result;

    TRACE("(%d,%p,%d)\n", wLang, szLang, nSize );

    /* 
     * First, check \System\CurrentControlSet\control\Nls\Locale\<langid>
     * from the registry.
     */

    sprintf( buffer,
             "\\System\\CurrentControlSet\\control\\Nls\\Locale\\%08x",
             wLang );

    result = RegQueryValueA( HKEY_LOCAL_MACHINE, buffer, szLang, (LPDWORD)&nSize );
    if ( result == ERROR_SUCCESS || result == ERROR_MORE_DATA ) 
        return nSize;

    /* 
     * If that fails, use the internal table 
     * (actually, Windows stores the names in a string table resource ...)
     */

    name = WINE_GetLanguageName( wLang );
    lstrcpynA( szLang, name, nSize );
    return lstrlenA( name );
}

/***********************************************************************
 *           VerLanguageName32W              [VERSION.10]
 */
DWORD WINAPI VerLanguageNameW( UINT wLang, LPWSTR szLang, UINT nSize )
{
    char    buffer[80];
    LPWSTR  keyname;
    LPCSTR  name;
    DWORD   result;

    TRACE("(%d,%p,%d)\n", wLang, szLang, nSize );

    /* 
     * First, check \System\CurrentControlSet\control\Nls\Locale\<langid>
     * from the registry.
     */

    sprintf( buffer,
             "\\System\\CurrentControlSet\\control\\Nls\\Locale\\%08x",
             wLang );

    keyname = HEAP_strdupAtoW( GetProcessHeap(), 0, buffer );
    result = RegQueryValueW( HKEY_LOCAL_MACHINE, keyname, szLang, (LPDWORD)&nSize );
    HeapFree( GetProcessHeap(), 0, keyname );

    if ( result == ERROR_SUCCESS || result == ERROR_MORE_DATA ) 
        return nSize;

    /* 
     * If that fails, use the internal table 
     * (actually, Windows stores the names in a string table resource ...)
     */

    name = WINE_GetLanguageName( wLang );
    lstrcpynAtoW( szLang, name, nSize );
    return lstrlenA( name );
}


