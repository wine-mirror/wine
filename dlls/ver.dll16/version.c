/*
 * Implementation of VER.DLL
 *
 * Copyright 1996, 1997 Marcus Meissner
 * Copyright 1997 David Cuthbert
 * Copyright 1999 Ulrich Weigand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "windef.h"
#include "wine/winbase16.h"
#include "winternl.h"
#include "winuser.h"
#include "winver.h"
#include "lzexpand.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ver);

/**********************************************************************
 *  find_entry_by_id
 *
 * Find an entry by id in a resource directory
 * Copied from loader/pe_resource.c
 */
static const IMAGE_RESOURCE_DIRECTORY *find_entry_by_id( const IMAGE_RESOURCE_DIRECTORY *dir,
                                                         WORD id, const void *root )
{
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
    int min, max, pos;

    entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    min = dir->NumberOfNamedEntries;
    max = min + dir->NumberOfIdEntries - 1;
    while (min <= max)
    {
        pos = (min + max) / 2;
        if (entry[pos].Id == id)
            return (const IMAGE_RESOURCE_DIRECTORY *)((const char *)root + entry[pos].OffsetToDirectory);
        if (entry[pos].Id > id) max = pos - 1;
        else min = pos + 1;
    }
    return NULL;
}


/**********************************************************************
 *  find_entry_default
 *
 * Find a default entry in a resource directory
 * Copied from loader/pe_resource.c
 */
static const IMAGE_RESOURCE_DIRECTORY *find_entry_default( const IMAGE_RESOURCE_DIRECTORY *dir,
                                                           const void *root )
{
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;

    entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    return (const IMAGE_RESOURCE_DIRECTORY *)((const char *)root + entry->OffsetToDirectory);
}


/**********************************************************************
 *  find_entry_by_name
 *
 * Find an entry by name in a resource directory
 * Copied from loader/pe_resource.c
 */
static const IMAGE_RESOURCE_DIRECTORY *find_entry_by_name( const IMAGE_RESOURCE_DIRECTORY *dir,
                                                           LPCSTR name, const void *root )
{
    const IMAGE_RESOURCE_DIRECTORY *ret = NULL;
    LPWSTR nameW;
    DWORD namelen;

    if (!HIWORD(name)) return find_entry_by_id( dir, LOWORD(name), root );
    if (name[0] == '#')
    {
        return find_entry_by_id( dir, atoi(name+1), root );
    }

    namelen = MultiByteToWideChar( CP_ACP, 0, name, -1, NULL, 0 );
    if ((nameW = HeapAlloc( GetProcessHeap(), 0, namelen * sizeof(WCHAR) )))
    {
        const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
        const IMAGE_RESOURCE_DIR_STRING_U *str;
        int min, max, res, pos;

        MultiByteToWideChar( CP_ACP, 0, name, -1, nameW, namelen );
        namelen--;  /* remove terminating null */
        entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
        min = 0;
        max = dir->NumberOfNamedEntries - 1;
        while (min <= max)
        {
            pos = (min + max) / 2;
            str = (const IMAGE_RESOURCE_DIR_STRING_U *)((const char *)root + entry[pos].NameOffset);
            res = wcsnicmp( nameW, str->NameString, str->Length );
            if (!res && namelen == str->Length)
            {
                ret = (const IMAGE_RESOURCE_DIRECTORY *)((const char *)root + entry[pos].OffsetToDirectory);
                break;
            }
            if (res < 0) max = pos - 1;
            else min = pos + 1;
        }
        HeapFree( GetProcessHeap(), 0, nameW );
    }
    return ret;
}


/***********************************************************************
 *           read_xx_header         [internal]
 */
static int read_xx_header( HFILE lzfd )
{
    IMAGE_DOS_HEADER mzh;
    char magic[3];

    LZSeek( lzfd, 0, FILE_BEGIN );
    if ( sizeof(mzh) != LZRead( lzfd, (LPSTR)&mzh, sizeof(mzh) ) )
        return 0;
    if ( mzh.e_magic != IMAGE_DOS_SIGNATURE )
        return 0;

    LZSeek( lzfd, mzh.e_lfanew, FILE_BEGIN );
    if ( 2 != LZRead( lzfd, magic, 2 ) )
        return 0;

    LZSeek( lzfd, mzh.e_lfanew, FILE_BEGIN );

    if ( magic[0] == 'N' && magic[1] == 'E' )
        return IMAGE_OS2_SIGNATURE;
    if ( magic[0] == 'P' && magic[1] == 'E' )
        return IMAGE_NT_SIGNATURE;

    magic[2] = '\0';
    WARN("Can't handle %s files.\n", magic );
    return 0;
}

/***********************************************************************
 *           find_ne_resource         [internal]
 */
static BOOL find_ne_resource( HFILE lzfd, LPCSTR typeid, LPCSTR resid,
                                DWORD *resLen, DWORD *resOff )
{
    IMAGE_OS2_HEADER nehd;
    NE_TYPEINFO *typeInfo;
    NE_NAMEINFO *nameInfo;
    DWORD nehdoffset;
    LPBYTE resTab;
    DWORD resTabSize;
    int count;

    /* Read in NE header */
    nehdoffset = LZSeek( lzfd, 0, FILE_CURRENT );
    if ( sizeof(nehd) != LZRead( lzfd, (LPSTR)&nehd, sizeof(nehd) ) ) return FALSE;

    resTabSize = nehd.ne_restab - nehd.ne_rsrctab;
    if ( !resTabSize )
    {
        TRACE("No resources in NE dll\n" );
        return FALSE;
    }

    /* Read in resource table */
    resTab = HeapAlloc( GetProcessHeap(), 0, resTabSize );
    if ( !resTab ) return FALSE;

    LZSeek( lzfd, nehd.ne_rsrctab + nehdoffset, FILE_BEGIN );
    if ( resTabSize != LZRead( lzfd, (char*)resTab, resTabSize ) )
    {
        HeapFree( GetProcessHeap(), 0, resTab );
        return FALSE;
    }

    /* Find resource */
    typeInfo = (NE_TYPEINFO *)(resTab + 2);

    if (HIWORD(typeid) != 0)  /* named type */
    {
        BYTE len = strlen( typeid );
        while (typeInfo->type_id)
        {
            if (!(typeInfo->type_id & 0x8000))
            {
                BYTE *p = resTab + typeInfo->type_id;
                if ((*p == len) && !_strnicmp( (char*)p+1, typeid, len )) goto found_type;
            }
            typeInfo = (NE_TYPEINFO *)((char *)(typeInfo + 1) +
                                       typeInfo->count * sizeof(NE_NAMEINFO));
        }
    }
    else  /* numeric type id */
    {
        WORD id = LOWORD(typeid) | 0x8000;
        while (typeInfo->type_id)
        {
            if (typeInfo->type_id == id) goto found_type;
            typeInfo = (NE_TYPEINFO *)((char *)(typeInfo + 1) +
                                       typeInfo->count * sizeof(NE_NAMEINFO));
        }
    }
    TRACE("No typeid entry found for %p\n", typeid );
    HeapFree( GetProcessHeap(), 0, resTab );
    return FALSE;

 found_type:
    nameInfo = (NE_NAMEINFO *)(typeInfo + 1);

    if (HIWORD(resid) != 0)  /* named resource */
    {
        BYTE len = strlen( resid );
        for (count = typeInfo->count; count > 0; count--, nameInfo++)
        {
            BYTE *p = resTab + nameInfo->id;
            if (nameInfo->id & 0x8000) continue;
            if ((*p == len) && !_strnicmp( (char*)p+1, resid, len )) goto found_name;
        }
    }
    else  /* numeric resource id */
    {
        WORD id = LOWORD(resid) | 0x8000;
        for (count = typeInfo->count; count > 0; count--, nameInfo++)
            if (nameInfo->id == id) goto found_name;
    }
    TRACE("No resid entry found for %p\n", typeid );
    HeapFree( GetProcessHeap(), 0, resTab );
    return FALSE;

 found_name:
    /* Return resource data */
    if ( resLen ) *resLen = nameInfo->length << *(WORD *)resTab;
    if ( resOff ) *resOff = nameInfo->offset << *(WORD *)resTab;

    HeapFree( GetProcessHeap(), 0, resTab );
    return TRUE;
}

/***********************************************************************
 *           find_pe_resource         [internal]
 */
static BOOL find_pe_resource( HFILE lzfd, LPCSTR typeid, LPCSTR resid,
                                DWORD *resLen, DWORD *resOff )
{
    IMAGE_NT_HEADERS pehd;
    DWORD pehdoffset;
    PIMAGE_DATA_DIRECTORY resDataDir;
    PIMAGE_SECTION_HEADER sections;
    LPBYTE resSection;
    DWORD resSectionSize;
    const void *resDir;
    const IMAGE_RESOURCE_DIRECTORY *resPtr;
    const IMAGE_RESOURCE_DATA_ENTRY *resData;
    int i, nSections;
    BOOL ret = FALSE;

    /* Read in PE header */
    pehdoffset = LZSeek( lzfd, 0, FILE_CURRENT );
    if ( sizeof(pehd) != LZRead( lzfd, (LPSTR)&pehd, sizeof(pehd) ) ) return FALSE;

    resDataDir = pehd.OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_RESOURCE;
    if ( !resDataDir->Size )
    {
        TRACE("No resources in PE dll\n" );
        return FALSE;
    }

    /* Read in section table */
    nSections = pehd.FileHeader.NumberOfSections;
    sections = HeapAlloc( GetProcessHeap(), 0,
                          nSections * sizeof(IMAGE_SECTION_HEADER) );
    if ( !sections ) return FALSE;

    LZSeek( lzfd, pehdoffset +
                    sizeof(DWORD) + /* Signature */
                    sizeof(IMAGE_FILE_HEADER) +
                    pehd.FileHeader.SizeOfOptionalHeader, FILE_BEGIN );

    if ( nSections * sizeof(IMAGE_SECTION_HEADER) !=
         LZRead( lzfd, (LPSTR)sections, nSections * sizeof(IMAGE_SECTION_HEADER) ) )
    {
        HeapFree( GetProcessHeap(), 0, sections );
        return FALSE;
    }

    /* Find resource section */
    for ( i = 0; i < nSections; i++ )
        if (    resDataDir->VirtualAddress >= sections[i].VirtualAddress
             && resDataDir->VirtualAddress <  sections[i].VirtualAddress +
                                              sections[i].SizeOfRawData )
            break;

    if ( i == nSections )
    {
        HeapFree( GetProcessHeap(), 0, sections );
        TRACE("Couldn't find resource section\n" );
        return FALSE;
    }

    /* Read in resource section */
    resSectionSize = sections[i].SizeOfRawData;
    resSection = HeapAlloc( GetProcessHeap(), 0, resSectionSize );
    if ( !resSection )
    {
        HeapFree( GetProcessHeap(), 0, sections );
        return FALSE;
    }

    LZSeek( lzfd, sections[i].PointerToRawData, FILE_BEGIN );
    if ( resSectionSize != LZRead( lzfd, (char*)resSection, resSectionSize ) ) goto done;

    /* Find resource */
    resDir = resSection + (resDataDir->VirtualAddress - sections[i].VirtualAddress);

    resPtr = resDir;
    resPtr = find_entry_by_name( resPtr, typeid, resDir );
    if ( !resPtr )
    {
        TRACE("No typeid entry found for %p\n", typeid );
        goto done;
    }
    resPtr = find_entry_by_name( resPtr, resid, resDir );
    if ( !resPtr )
    {
        TRACE("No resid entry found for %p\n", resid );
        goto done;
    }
    resPtr = find_entry_default( resPtr, resDir );
    if ( !resPtr )
    {
        TRACE("No default language entry found for %p\n", resid );
        goto done;
    }

    /* Find resource data section */
    resData = (const IMAGE_RESOURCE_DATA_ENTRY*)resPtr;
    for ( i = 0; i < nSections; i++ )
        if (    resData->OffsetToData >= sections[i].VirtualAddress
             && resData->OffsetToData <  sections[i].VirtualAddress +
                                         sections[i].SizeOfRawData )
            break;

    if ( i == nSections )
    {
        TRACE("Couldn't find resource data section\n" );
        goto done;
    }

    /* Return resource data */
    if ( resLen ) *resLen = resData->Size;
    if ( resOff ) *resOff = resData->OffsetToData - sections[i].VirtualAddress
                            + sections[i].PointerToRawData;
    ret = TRUE;

 done:
    HeapFree( GetProcessHeap(), 0, resSection );
    HeapFree( GetProcessHeap(), 0, sections );
    return ret;
}


/***********************************************************************
 *           find_resource         [internal]
 */
static DWORD find_resource( HFILE lzfd, LPCSTR type, LPCSTR id, DWORD *reslen, DWORD *offset )
{
    DWORD magic = read_xx_header( lzfd );

    switch (magic)
    {
    case IMAGE_OS2_SIGNATURE:
        if (!find_ne_resource( lzfd, type, id, reslen, offset )) magic = 0;
        break;
    case IMAGE_NT_SIGNATURE:
        if (!find_pe_resource( lzfd, type, id, reslen, offset )) magic = 0;
        break;
    }
    return magic;
}


/*************************************************************************
 * GetFileResourceSize                     [VER.2]
 */
DWORD WINAPI GetFileResourceSize16( LPCSTR lpszFileName, LPCSTR lpszResType,
                                    LPCSTR lpszResId, LPDWORD lpdwFileOffset )
{
    HFILE lzfd;
    OFSTRUCT ofs;
    DWORD reslen = 0;

    TRACE("(%s,type=%p,id=%p,off=%p)\n",
          debugstr_a(lpszFileName), lpszResType, lpszResId, lpszResId );

    lzfd = LZOpenFileA( (LPSTR)lpszFileName, &ofs, OF_READ );
    if (lzfd >= 0)
    {
        if (!find_resource( lzfd, lpszResType, lpszResId, &reslen, lpdwFileOffset )) reslen = 0;
        LZClose( lzfd );
    }
    return reslen;
}


/*************************************************************************
 * GetFileResource                         [VER.3]
 */
DWORD WINAPI GetFileResource16( LPCSTR lpszFileName, LPCSTR lpszResType,
                                LPCSTR lpszResId, DWORD dwFileOffset,
                                DWORD dwResLen, LPVOID lpvData )
{
    HFILE lzfd;
    OFSTRUCT ofs;
    DWORD reslen = dwResLen;

    TRACE("(%s,type=%p,id=%p,off=%ld,len=%ld,data=%p)\n",
		debugstr_a(lpszFileName), lpszResType, lpszResId,
                dwFileOffset, dwResLen, lpvData );

    lzfd = LZOpenFileA( (LPSTR)lpszFileName, &ofs, OF_READ );
    if ( lzfd < 0 ) return 0;

    if ( !dwFileOffset )
    {
        if (!find_resource( lzfd, lpszResType, lpszResId, &reslen, &dwFileOffset ))
        {
            LZClose( lzfd );
            return 0;
        }
    }

    LZSeek( lzfd, dwFileOffset, FILE_BEGIN );
    reslen = LZRead( lzfd, lpvData, min( reslen, dwResLen ) );
    LZClose( lzfd );

    return reslen;
}

/*************************************************************************
 * GetFileVersionInfoSize                  [VER.6]
 */
DWORD WINAPI GetFileVersionInfoSize16( LPCSTR filename, LPDWORD handle )
{
    DWORD offset;

    TRACE("(%s, %p)\n", debugstr_a(filename), handle );

    return GetFileResourceSize16( filename, (LPCSTR)RT_VERSION, (LPCSTR)VS_VERSION_INFO, &offset );
}

/*************************************************************************
 * GetFileVersionInfo                      [VER.7]
 */
DWORD WINAPI GetFileVersionInfo16( LPCSTR filename, DWORD handle, DWORD datasize, LPVOID data )
{
    TRACE("(%s, %08lx, %ld, %p)\n", debugstr_a(filename), handle, datasize, data );

    return GetFileResource16( filename, (LPCSTR)RT_VERSION, (LPCSTR)VS_VERSION_INFO, 0, datasize, data );
}

/*************************************************************************
 * VerFindFile                             [VER.8]
 */
DWORD WINAPI VerFindFile16( UINT16 flags, LPSTR lpszFilename,
                            LPSTR lpszWinDir, LPSTR lpszAppDir,
                            LPSTR lpszCurDir, UINT16 *lpuCurDirLen,
                            LPSTR lpszDestDir, UINT16 *lpuDestDirLen )
{
    UINT curDirLen, destDirLen;
    UINT *pcurDirLen = NULL, *pdestDirLen = NULL;
    DWORD retv;

    if (lpuCurDirLen) {
        curDirLen = *lpuCurDirLen;
        pcurDirLen = &curDirLen;
    }
    if (lpuDestDirLen) {
        destDirLen = *lpuDestDirLen;
        pdestDirLen = &destDirLen;
    }
    retv = VerFindFileA( flags, lpszFilename, lpszWinDir, lpszAppDir,
                                lpszCurDir, pcurDirLen, lpszDestDir, pdestDirLen );
    if (lpuCurDirLen)
        *lpuCurDirLen = (UINT16)curDirLen;
    if (lpuDestDirLen)
        *lpuDestDirLen = (UINT16)destDirLen;
    return retv;
}

/*************************************************************************
 * VerInstallFile                          [VER.9]
 */
DWORD WINAPI VerInstallFile16( UINT16 flags,
                               LPSTR lpszSrcFilename, LPSTR lpszDestFilename,
                               LPSTR lpszSrcDir, LPSTR lpszDestDir, LPSTR lpszCurDir,
                               LPSTR lpszTmpFile, UINT16 *lpwTmpFileLen )
{
    UINT filelen = *lpwTmpFileLen;

    DWORD retv = VerInstallFileA( flags, lpszSrcFilename, lpszDestFilename,
                                    lpszSrcDir, lpszDestDir, lpszCurDir,
                                    lpszTmpFile, &filelen);

    *lpwTmpFileLen = (UINT16)filelen;
    return retv;
}

/*************************************************************************
 * VerLanguageName                        [VER.10]
 */
DWORD WINAPI VerLanguageName16( UINT16 uLang, LPSTR lpszLang, UINT16 cbLang )
{
    return VerLanguageNameA( uLang, lpszLang, cbLang );
}

/*************************************************************************
 * VerQueryValue                          [VER.11]
 */
DWORD WINAPI VerQueryValue16( SEGPTR spvBlock, LPSTR lpszSubBlock,
                              SEGPTR *lpspBuffer, UINT16 *lpcb )
{
    LPVOID lpvBlock = MapSL( spvBlock );
    LPVOID buffer = lpvBlock;
    UINT buflen;
    DWORD retv;

    TRACE("(%p, %s, %p, %p)\n",
                lpvBlock, debugstr_a(lpszSubBlock), lpspBuffer, lpcb );

    retv = VerQueryValueA( lpvBlock, lpszSubBlock, &buffer, &buflen );
    if ( !retv ) return FALSE;

    if ( OFFSETOF( spvBlock ) + ((char *) buffer - (char *) lpvBlock) >= 0x10000 )
    {
        FIXME("offset %08X too large relative to %04X:%04X\n",
               (char *) buffer - (char *) lpvBlock, SELECTOROF( spvBlock ), OFFSETOF( spvBlock ) );
        return FALSE;
    }

    if (lpcb) *lpcb = buflen;
    *lpspBuffer = (SEGPTR) ((char *) spvBlock + ((char *) buffer - (char *) lpvBlock));

    return retv;
}
