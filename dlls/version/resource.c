/* 
 * Implementation of VERSION.DLL - Resource Access routines
 * 
 * Copyright 1996,1997 Marcus Meissner
 * Copyright 1997 David Cuthbert
 * Copyright 1999 Ulrich Weigand
 */

#include <stdlib.h>
#include <string.h>

#include "windows.h"
#include "heap.h"
#include "ver.h"
#include "lzexpand.h"
#include "module.h"
#include "neexe.h"
#include "debug.h"


/***********************************************************************
 *           read_xx_header         [internal]
 */
static int read_xx_header( HFILE32 lzfd )
{
    IMAGE_DOS_HEADER mzh;
    char magic[3];

    LZSeek32( lzfd, 0, SEEK_SET );
    if ( sizeof(mzh) != LZRead32( lzfd, &mzh, sizeof(mzh) ) )
        return 0;
    if ( mzh.e_magic != IMAGE_DOS_SIGNATURE )
        return 0;

    LZSeek32( lzfd, mzh.e_lfanew, SEEK_SET );
    if ( 2 != LZRead32( lzfd, magic, 2 ) )
        return 0;

    LZSeek32( lzfd, mzh.e_lfanew, SEEK_SET );

    if ( magic[0] == 'N' && magic[1] == 'E' )
        return IMAGE_OS2_SIGNATURE;
    if ( magic[0] == 'P' && magic[1] == 'E' )
        return IMAGE_NT_SIGNATURE;

    magic[2] = '\0';
    WARN( ver, "Can't handle %s files.\n", magic );
    return 0;
}

/***********************************************************************
 *           load_ne_resource         [internal]
 */
static BOOL32 find_ne_resource( HFILE32 lzfd, LPCSTR typeid, LPCSTR resid,
                                DWORD *resLen, DWORD *resOff )
{
    IMAGE_OS2_HEADER nehd;
    NE_TYPEINFO *typeInfo;
    NE_NAMEINFO *nameInfo;
    DWORD nehdoffset;
    LPBYTE resTab;
    DWORD resTabSize;

    /* Read in NE header */ 
    nehdoffset = LZSeek32( lzfd, 0, SEEK_CUR );
    if ( sizeof(nehd) != LZRead32( lzfd, &nehd, sizeof(nehd) ) ) return 0;

    resTabSize = nehd.rname_tab_offset - nehd.resource_tab_offset; 
    if ( !resTabSize )
    {
        TRACE( ver, "No resources in NE dll\n" );
        return FALSE;
    }

    /* Read in resource table */ 
    resTab = HeapAlloc( GetProcessHeap(), 0, resTabSize );
    if ( !resTab ) return FALSE;

    LZSeek32( lzfd, nehd.resource_tab_offset + nehdoffset, SEEK_SET );
    if ( resTabSize != LZRead32( lzfd, resTab, resTabSize ) )
    {
        HeapFree( GetProcessHeap(), 0, resTab );
        return FALSE;
    }

    /* Find resource */
    typeInfo = (NE_TYPEINFO *)(resTab + 2);
    typeInfo = NE_FindTypeSection( resTab, typeInfo, typeid );
    if ( !typeInfo )
    {
        TRACE( ver, "No typeid entry found for %p\n", typeid );
        HeapFree( GetProcessHeap(), 0, resTab );
        return FALSE;
    }
    nameInfo = NE_FindResourceFromType( resTab, typeInfo, resid );
    if ( !nameInfo )
    {
        TRACE( ver, "No resid entry found for %p\n", typeid );
        HeapFree( GetProcessHeap(), 0, resTab );
        return FALSE;
    }

    /* Return resource data */
    *resLen = nameInfo->length << *(WORD *)resTab;
    *resOff = nameInfo->offset << *(WORD *)resTab;

    HeapFree( GetProcessHeap(), 0, resTab );
    return TRUE;
}

/***********************************************************************
 *           load_pe_resource         [internal]
 */
static BOOL32 find_pe_resource( HFILE32 lzfd, LPCSTR typeid, LPCSTR resid,
                                DWORD *resLen, DWORD *resOff )
{
    IMAGE_NT_HEADERS pehd;
    DWORD pehdoffset;
    PIMAGE_DATA_DIRECTORY resDataDir;
    PIMAGE_SECTION_HEADER sections;
    LPBYTE resSection;
    DWORD resSectionSize;
    DWORD resDir;
    PIMAGE_RESOURCE_DIRECTORY resPtr;
    PIMAGE_RESOURCE_DATA_ENTRY resData;
    int i, nSections;


    /* Read in PE header */
    pehdoffset = LZSeek32( lzfd, 0, SEEK_CUR );
    if ( sizeof(pehd) != LZRead32( lzfd, &pehd, sizeof(pehd) ) ) return 0;

    resDataDir = pehd.OptionalHeader.DataDirectory+IMAGE_FILE_RESOURCE_DIRECTORY;
    if ( !resDataDir->Size )
    {
        TRACE( ver, "No resources in PE dll\n" );
        return FALSE;
    }

    /* Read in section table */
    nSections = pehd.FileHeader.NumberOfSections; 
    sections = HeapAlloc( GetProcessHeap(), 0, 
                          nSections * sizeof(IMAGE_SECTION_HEADER) );
    if ( !sections ) return FALSE;

    LZSeek32( lzfd, pehdoffset +
                    sizeof(DWORD) + /* Signature */
                    sizeof(IMAGE_FILE_HEADER) +
                    pehd.FileHeader.SizeOfOptionalHeader, SEEK_SET );

    if ( nSections * sizeof(IMAGE_SECTION_HEADER) !=
         LZRead32( lzfd, sections, nSections * sizeof(IMAGE_SECTION_HEADER) ) )
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
        TRACE( ver, "Couldn't find resource section\n" );
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

    LZSeek32( lzfd, sections[i].PointerToRawData, SEEK_SET );
    if ( resSectionSize != LZRead32( lzfd, resSection, resSectionSize ) )
    {
        HeapFree( GetProcessHeap(), 0, resSection );
        HeapFree( GetProcessHeap(), 0, sections );
        return FALSE;
    }

    /* Find resource */
    resDir = (DWORD)resSection + 
             (resDataDir->VirtualAddress - sections[i].VirtualAddress);

    resPtr = (PIMAGE_RESOURCE_DIRECTORY)resDir;
    resPtr = GetResDirEntryA( resPtr, typeid, resDir, FALSE );
    if ( !resPtr )
    {
        TRACE( ver, "No typeid entry found for %p\n", typeid );
        HeapFree( GetProcessHeap(), 0, resSection );
        HeapFree( GetProcessHeap(), 0, sections );
        return FALSE;
    }
    resPtr = GetResDirEntryA( resPtr, resid, resDir, FALSE );
    if ( !resPtr )
    {
        TRACE( ver, "No resid entry found for %p\n", resid );
        HeapFree( GetProcessHeap(), 0, resSection );
        HeapFree( GetProcessHeap(), 0, sections );
        return FALSE;
    }
    resPtr = GetResDirEntryA( resPtr, 0, resDir, TRUE );
    if ( !resPtr )
    {
        TRACE( ver, "No default language entry found for %p\n", resid );
        HeapFree( GetProcessHeap(), 0, resSection );
        HeapFree( GetProcessHeap(), 0, sections );
        return FALSE;
    }

    /* Find resource data section */
    resData = (PIMAGE_RESOURCE_DATA_ENTRY)resPtr;
    for ( i = 0; i < nSections; i++ )
        if (    resData->OffsetToData >= sections[i].VirtualAddress
             && resData->OffsetToData <  sections[i].VirtualAddress +
                                         sections[i].SizeOfRawData )
            break;

    if ( i == nSections )
    {
        TRACE( ver, "Couldn't find resource data section\n" );
        HeapFree( GetProcessHeap(), 0, resSection );
        HeapFree( GetProcessHeap(), 0, sections );
        return FALSE;
    }

    /* Return resource data */
    *resLen = resData->Size;
    *resOff = resData->OffsetToData - sections[i].VirtualAddress
              + sections[i].PointerToRawData;

    HeapFree( GetProcessHeap(), 0, resSection );
    HeapFree( GetProcessHeap(), 0, sections );
    return TRUE;
}

/***********************************************************************
 *           GetFileResourceSize32         [internal]
 */
DWORD WINAPI GetFileResourceSize32( LPCSTR lpszFileName,
                                    LPCSTR lpszResType, LPCSTR lpszResId,
                                    LPDWORD lpdwFileOffset )
{
    BOOL32 retv = FALSE;
    HFILE32 lzfd;
    OFSTRUCT ofs;
    DWORD reslen;

    TRACE( ver, "(%s,type=0x%lx,id=0x%lx,off=%p)\n",
                debugstr_a(lpszFileName), (LONG)lpszResType, (LONG)lpszResId, 
                lpszResId );

    lzfd = LZOpenFile32A( lpszFileName, &ofs, OF_READ );
    if ( !lzfd ) return 0;

    switch ( read_xx_header( lzfd ) )
    {
    case IMAGE_OS2_SIGNATURE:
        retv = find_ne_resource( lzfd, lpszResType, lpszResId, 
                                 &reslen, lpdwFileOffset );
        break;

    case IMAGE_NT_SIGNATURE:
        retv = find_pe_resource( lzfd, lpszResType, lpszResId, 
                                 &reslen, lpdwFileOffset );
        break;
    }

    LZClose32( lzfd );
    return retv? reslen : 0;
}

/***********************************************************************
 *           GetFileResource32         [internal]
 */
DWORD WINAPI GetFileResource32( LPCSTR lpszFileName,
                                LPCSTR lpszResType, LPCSTR lpszResId,
                                DWORD dwFileOffset,
                                DWORD dwResLen, LPVOID lpvData )
{
    BOOL32 retv = FALSE;
    HFILE32 lzfd;
    OFSTRUCT ofs;
    DWORD reslen;

    TRACE( ver, "(%s,type=0x%lx,id=0x%lx,off=%ld,len=%ld,data=%p)\n",
		debugstr_a(lpszFileName), (LONG)lpszResType, (LONG)lpszResId, 
                dwFileOffset, dwResLen, lpvData );

    lzfd = LZOpenFile32A( lpszFileName, &ofs, OF_READ );
    if ( lzfd == 0 ) return 0;

    if ( !dwFileOffset )
    {
        switch ( read_xx_header( lzfd ) ) 
        {
        case IMAGE_OS2_SIGNATURE:
            retv = find_ne_resource( lzfd, lpszResType, lpszResId, 
                                     &reslen, &dwFileOffset );
            break;

        case IMAGE_NT_SIGNATURE:
            retv = find_pe_resource( lzfd, lpszResType, lpszResId, 
                                     &reslen, &dwFileOffset );
            break;
        }

        if ( !retv ) 
        {
            LZClose32( lzfd );
            return 0;
        }
    }

    LZSeek32( lzfd, dwFileOffset, SEEK_SET );
    reslen = LZRead32( lzfd, lpvData, min( reslen, dwResLen ) );
    LZClose32( lzfd );

    return reslen;
}

