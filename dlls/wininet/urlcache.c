/*
 * Wininet - Url Cache functions
 *
 * Copyright 2001,2002 CodeWeavers
 * Copyright 2003 Robert Shearman
 *
 * Eric Kohl
 * Aric Stewart
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define COM_NO_WINDOWS_H
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wininet.h"
#include "winerror.h"
#include "internet.h"
#include "winreg.h"
#include "shlwapi.h"
#include "shlobj.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

#define ENTRY_START_OFFSET  0x4000
#define DIR_LENGTH          8
#define BLOCKSIZE           128
#define CONTENT_DIRECTORY   "\\Content.IE5\\"
#define HASHTABLE_SIZE      448
#define HASHTABLE_BLOCKSIZE 7
#define ALLOCATION_TABLE_OFFSET 0x250
#define ALLOCATION_TABLE_SIZE   (0x1000 - ALLOCATION_TABLE_OFFSET)
#define HASHTABLE_NUM_ENTRIES   (HASHTABLE_SIZE / HASHTABLE_BLOCKSIZE)

#define DWORD_SIG(a,b,c,d)  (a | (b << 8) | (c << 16) | (d << 24))
#define URL_SIGNATURE   DWORD_SIG('U','R','L',' ')
#define REDR_SIGNATURE  DWORD_SIG('R','E','D','R')
#define LEAK_SIGNATURE  DWORD_SIG('L','E','A','K')
#define HASH_SIGNATURE  DWORD_SIG('H','A','S','H')

#define DWORD_ALIGN(x) ( (DWORD)(((DWORD)(x) + 3) >> 2) << 2)

typedef struct _CACHEFILE_ENTRY
{
/*  union
    {*/
        DWORD dwSignature; /* e.g. "URL " */
/*      CHAR szSignature[4];
    };*/
    DWORD dwBlocksUsed; /* number of 128byte blocks used by this entry */
} CACHEFILE_ENTRY;

typedef struct _URL_CACHEFILE_ENTRY
{
    CACHEFILE_ENTRY CacheFileEntry;
    FILETIME LastModifiedTime;
    FILETIME LastAccessTime;
    WORD wExpiredDate; /* expire date in dos format */
    WORD wExpiredTime; /* expire time in dos format */
    DWORD dwUnknown1; /* usually zero */
    DWORD dwSizeLow; /* see INTERNET_CACHE_ENTRY_INFO::dwSizeLow */
    DWORD dwSizeHigh; /* see INTERNET_CACHE_ENTRY_INFO::dwSizeHigh */
    DWORD dwUnknown2; /* usually zero */
    DWORD dwExemptDelta; /* see INTERNET_CACHE_ENTRY_INFO::dwExemptDelta */
    DWORD dwUnknown3; /* usually 0x60 */
    DWORD dwOffsetUrl; /* usually 0x68 */
    BYTE CacheDir; /* index of cache directory this url is stored in */
    BYTE Unknown4; /* usually zero */
    WORD wUnknown5; /* usually 0x1010 */
    DWORD dwOffsetLocalName; /* offset of start of local filename from start of entry */
    DWORD CacheEntryType; /* see INTERNET_CACHE_ENTRY_INFO::CacheEntryType */
    DWORD dwOffsetHeaderInfo; /* offset of start of header info from start of entry */
    DWORD dwHeaderInfoSize;
    DWORD dwUnknown6; /* usually zero */
    WORD wLastSyncDate; /* last sync date in dos format */
    WORD wLastSyncTime; /* last sync time in dos format */
    DWORD dwHitRate; /* see INTERNET_CACHE_ENTRY_INFO::dwHitRate */
    DWORD dwUseCount; /* see INTERNET_CACHE_ENTRY_INFO::dwUseCount */
    WORD wUnknownDate; /* usually same as wLastSyncDate */
    WORD wUnknownTime; /* usually same as wLastSyncTime */
    DWORD dwUnknown7; /* usually zero */
    DWORD dwUnknown8; /* usually zero */
    CHAR szSourceUrlName[1]; /* start of url */
    /* packing to dword align start of next field */
    /* CHAR szLocalFileName[]; (local file name exluding path) */
    /* packing to dword align start of next field */
    /* CHAR szHeaderInfo[]; (header info) */
} URL_CACHEFILE_ENTRY;

struct _HASH_ENTRY
{
    DWORD dwHashKey;
    DWORD dwOffsetEntry;
};

typedef struct _HASH_CACHEFILE_ENTRY
{
    CACHEFILE_ENTRY CacheFileEntry;
    DWORD dwAddressNext;
    DWORD dwHashTableNumber;
    struct _HASH_ENTRY HashTable[HASHTABLE_SIZE];
} HASH_CACHEFILE_ENTRY;

typedef struct _DIRECTORY_DATA
{
    DWORD dwUnknown;
    char filename[DIR_LENGTH];
} DIRECTORY_DATA;

typedef struct _URLCACHE_HEADER
{
    char szSignature[28];
    DWORD dwFileSize;
    DWORD dwOffsetFirstHashTable;
    DWORD dwIndexCapacityInBlocks;
    DWORD dwBlocksInUse; /* is this right? */
    DWORD dwUnknown1;
    DWORD dwCacheLimitLow; /* disk space limit for cache */
    DWORD dwCacheLimitHigh; /* disk space limit for cache */
    DWORD dwUnknown4; /* current disk space usage for cache? */
    DWORD dwUnknown5; /* current disk space usage for cache? */
    DWORD dwUnknown6; /* possibly a flag? */
    DWORD dwUnknown7;
    BYTE DirectoryCount; /* number of directory_data's */
    BYTE Unknown8[3]; /* just padding? */
    DIRECTORY_DATA directory_data[1]; /* first directory entry */
} URLCACHE_HEADER, *LPURLCACHE_HEADER;
typedef const URLCACHE_HEADER *LPCURLCACHE_HEADER;


typedef struct _STREAM_HANDLE
{
    HANDLE hFile;
    CHAR lpszUrl[1];
} STREAM_HANDLE;

/**** File Global Variables ****/
static HANDLE hCacheIndexMapping = NULL; /* handle to file mapping */
static LPSTR szCacheContentPath = NULL; /* path to content index */
static HANDLE hMutex = NULL;

/***********************************************************************
 *           URLCache_PathToObjectName (Internal)
 *
 *  Converts a path to a name suitable for use as a Win32 object name.
 * Replaces '\\' characters in-place with the specified character
 * (usually '_' or '!')
 *
 * RETURNS
 *    nothing
 *
 */
static void URLCache_PathToObjectName(LPSTR lpszPath, char replace)
{
    char ch;
    for (ch = *lpszPath; (ch = *lpszPath); lpszPath++)
    {
        if (ch == '\\')
            *lpszPath = replace;
    }
}

#ifndef CHAR_BIT
#define CHAR_BIT    (8 * sizeof(CHAR))
#endif

/***********************************************************************
 *           URLCache_Allocation_BlockIsFree (Internal)
 *
 *  Is the specified block number free?
 *
 * RETURNS
 *    zero if free
 *    non-zero otherwise
 *
 */
static inline BYTE URLCache_Allocation_BlockIsFree(BYTE * AllocationTable, DWORD dwBlockNumber)
{
    BYTE mask = 1 << (dwBlockNumber % CHAR_BIT);
    return (AllocationTable[dwBlockNumber / CHAR_BIT] & mask) == 0;
}

/***********************************************************************
 *           URLCache_Allocation_BlockFree (Internal)
 *
 *  Marks the specified block as free
 *
 * RETURNS
 *    nothing
 *
 */
static inline void URLCache_Allocation_BlockFree(BYTE * AllocationTable, DWORD dwBlockNumber)
{
    BYTE mask = ~(1 << (dwBlockNumber % CHAR_BIT));
    AllocationTable[dwBlockNumber / CHAR_BIT] &= mask;
}

/***********************************************************************
 *           URLCache_Allocation_BlockAllocate (Internal)
 *
 *  Marks the specified block as allocated
 *
 * RETURNS
 *    nothing
 *
 */
static inline void URLCache_Allocation_BlockAllocate(BYTE * AllocationTable, DWORD dwBlockNumber)
{
    BYTE mask = 1 << (dwBlockNumber % CHAR_BIT);
    AllocationTable[dwBlockNumber / CHAR_BIT] |= mask;
}

/***********************************************************************
 *           URLCache_FindEntry (Internal)
 *
 *  Finds an entry without using the hash tables
 *
 * RETURNS
 *    TRUE if it found the specified entry
 *    FALSE otherwise
 *
 */
static BOOL URLCache_FindEntry(LPCURLCACHE_HEADER pHeader, LPCSTR szUrl, CACHEFILE_ENTRY ** ppEntry)
{
    CACHEFILE_ENTRY * pCurrentEntry;
    DWORD dwBlockNumber;

    BYTE * AllocationTable = (LPBYTE)pHeader + ALLOCATION_TABLE_OFFSET;

    for (pCurrentEntry = (CACHEFILE_ENTRY *)((LPBYTE)pHeader + ENTRY_START_OFFSET);
         (DWORD)((LPBYTE)pCurrentEntry - (LPBYTE)pHeader) < pHeader->dwFileSize;
         pCurrentEntry = (CACHEFILE_ENTRY *)((LPBYTE)pCurrentEntry + pCurrentEntry->dwBlocksUsed * BLOCKSIZE))
    {
        dwBlockNumber = (DWORD)((LPBYTE)pCurrentEntry - (LPBYTE)pHeader - ENTRY_START_OFFSET) / BLOCKSIZE;
        while (URLCache_Allocation_BlockIsFree(AllocationTable, dwBlockNumber))
        {
            if (dwBlockNumber >= pHeader->dwIndexCapacityInBlocks)
                return FALSE;

            pCurrentEntry = (CACHEFILE_ENTRY *)((LPBYTE)pCurrentEntry + BLOCKSIZE);
            dwBlockNumber = (DWORD)((LPBYTE)pCurrentEntry - (LPBYTE)pHeader - ENTRY_START_OFFSET) / BLOCKSIZE;
        }

        switch (pCurrentEntry->dwSignature)
        {
        case URL_SIGNATURE: /* "URL " */
        case LEAK_SIGNATURE: /* "LEAK" */
            {
                URL_CACHEFILE_ENTRY * pUrlEntry = (URL_CACHEFILE_ENTRY *)pCurrentEntry;
                if (!strcmp(szUrl, pUrlEntry->szSourceUrlName))
                {
                    *ppEntry = pCurrentEntry;
                    /* FIXME: should we update the LastAccessTime here? */
                    return TRUE;
                }
            }
            break;
        case HASH_SIGNATURE: /* HASH entries parsed in FindEntryInHash */
        case 0xDEADBEEF: /* this is always at offset 0x4000 in URL cache for some reason */
            break;
        default:
            FIXME("Unknown entry %.4s ignored\n", (LPCSTR)&pCurrentEntry->dwSignature);
        }

    }
    return FALSE;
}

/***********************************************************************
 *           URLCache_OpenIndex (Internal)
 *
 *  Opens the index file and saves mapping handle in hCacheIndexMapping
 *
 * RETURNS
 *    TRUE if succeeded
 *    FALSE if failed
 *
 */
static BOOL URLCache_OpenIndex()
{
    HANDLE hFile;
    CHAR szFullPath[MAX_PATH];
    CHAR szFileMappingName[MAX_PATH+10];
    CHAR szMutexName[MAX_PATH+1];
    DWORD dwFileSize;
    
    if (!szCacheContentPath)
    {
        szCacheContentPath = (LPSTR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(CHAR));
        *szCacheContentPath = '\0';
    }

    if (*szCacheContentPath == '\0')
    {
        if (FAILED(SHGetSpecialFolderPathA(NULL, szCacheContentPath, CSIDL_INTERNET_CACHE, TRUE)))
            return FALSE;
        strcat(szCacheContentPath, CONTENT_DIRECTORY);
    }

    strcpy(szFullPath, szCacheContentPath);
    strcat(szFullPath, "index.dat");

    if (hCacheIndexMapping)
        return TRUE;

    hFile = CreateFileA(szFullPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        FIXME("need to create cache index file\n");
        return FALSE;
    }

    dwFileSize = GetFileSize(hFile, NULL);
    if (dwFileSize == INVALID_FILE_SIZE)
        return FALSE;

    if (dwFileSize == 0)
    {
        FIXME("need to create cache index file\n");
        return FALSE;
    }

    strcpy(szFileMappingName, szFullPath);
    sprintf(szFileMappingName + strlen(szFileMappingName), "\\%lu", dwFileSize);
    URLCache_PathToObjectName(szFileMappingName, '_');
    hCacheIndexMapping = OpenFileMappingA(FILE_MAP_WRITE, FALSE, szFileMappingName);
    if (!hCacheIndexMapping)
        hCacheIndexMapping = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, 0, szFileMappingName);
    CloseHandle(hFile);
    if (!hCacheIndexMapping)
    {
        ERR("Couldn't create file mapping (error is %ld)\n", GetLastError());
        return FALSE;
    }

    strcpy(szMutexName, szFullPath);
    CharLowerA(szMutexName);
    URLCache_PathToObjectName(szMutexName, '!');
    strcat(szMutexName, "!");

    if ((hMutex = CreateMutexA(NULL, FALSE, szMutexName)) == NULL)
    {
        ERR("couldn't create mutex (error is %ld)\n", GetLastError());
        CloseHandle(hCacheIndexMapping);
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *           URLCache_CloseIndex (Internal)
 *
 *  Closes the index
 *
 * RETURNS
 *    nothing
 *
 */
#if 0 /* not used at the moment */
static BOOL URLCache_CloseIndex()
{
    return CloseHandle(hCacheIndexMapping);
}
#endif

/***********************************************************************
 *           URLCache_FindFirstFreeEntry (Internal)
 *
 *  Finds and allocates the first block of free space big enough and
 * sets ppEntry to point to it.
 *
 * RETURNS
 *    TRUE if it had enough space
 *    FALSE if it couldn't find enough space
 *
 */
static BOOL URLCache_FindFirstFreeEntry(URLCACHE_HEADER * pHeader, DWORD dwBlocksNeeded, CACHEFILE_ENTRY ** ppEntry)
{
    LPBYTE AllocationTable = (LPBYTE)pHeader + ALLOCATION_TABLE_OFFSET;
    DWORD dwBlockNumber;
    DWORD dwFreeCounter;
    for (dwBlockNumber = 0; dwBlockNumber < pHeader->dwIndexCapacityInBlocks; dwBlockNumber++)
    {
        for (dwFreeCounter = 0; 
            dwFreeCounter < dwBlocksNeeded &&
              dwFreeCounter + dwBlockNumber < pHeader->dwIndexCapacityInBlocks &&
              URLCache_Allocation_BlockIsFree(AllocationTable, dwBlockNumber + dwFreeCounter);
            dwFreeCounter++)
                TRACE("Found free block at no. %ld (0x%lx)\n", dwBlockNumber, ENTRY_START_OFFSET + dwBlockNumber * BLOCKSIZE);

        if (dwFreeCounter == dwBlocksNeeded)
        {
            DWORD index;
            TRACE("Found free blocks starting at no. %ld (0x%lx)\n", dwBlockNumber, ENTRY_START_OFFSET + dwBlockNumber * BLOCKSIZE);
            for (index = 0; index < dwBlocksNeeded; index++)
                URLCache_Allocation_BlockAllocate(AllocationTable, dwBlockNumber + index);
            *ppEntry = (CACHEFILE_ENTRY *)((LPBYTE)pHeader + ENTRY_START_OFFSET + dwBlockNumber * BLOCKSIZE);
            (*ppEntry)->dwBlocksUsed = dwBlocksNeeded;
            return TRUE;
        }
    }
    return FALSE;
}

/***********************************************************************
 *           URLCache_DeleteEntry (Internal)
 *
 *  Deletes the specified entry and frees the space allocated to it
 *
 * RETURNS
 *    TRUE if it succeeded
 *    FALSE if it failed
 *
 */
static BOOL URLCache_DeleteEntry(CACHEFILE_ENTRY * pEntry)
{
    ZeroMemory(pEntry, pEntry->dwBlocksUsed * BLOCKSIZE);
    return TRUE;
}

/***********************************************************************
 *           URLCache_LockIndex (Internal)
 *
 */
static LPURLCACHE_HEADER URLCache_LockIndex()
{
    BYTE index;
    LPVOID pIndexData = MapViewOfFile(hCacheIndexMapping, FILE_MAP_WRITE, 0, 0, 0);
    URLCACHE_HEADER * pHeader = (URLCACHE_HEADER *)pIndexData;
    if (!pIndexData)
        return FALSE;

    TRACE("Signature: %s, file size: %ld bytes\n", pHeader->szSignature, pHeader->dwFileSize);

    for (index = 0; index < pHeader->DirectoryCount; index++)
    {
        TRACE("Directory[%d] = \"%.8s\"\n", index, pHeader->directory_data[index].filename);
    }
    
    /* acquire mutex */
    WaitForSingleObject(hMutex, INFINITE);

    return pHeader;
}

/***********************************************************************
 *           URLCache_UnlockIndex (Internal)
 *
 */
static BOOL URLCache_UnlockIndex(LPURLCACHE_HEADER pHeader)
{
    /* release mutex */
    ReleaseMutex(hMutex);
    return UnmapViewOfFile(pHeader);
}

/***********************************************************************
 *           URLCache_LocalFileNameToPath (Internal)
 *
 *  Copies the full path to the specified buffer given the local file
 * name and the index of the directory it is in. Always sets value in
 * lpBufferSize to the required buffer size.
 *
 * RETURNS
 *    TRUE if the buffer was big enough
 *    FALSE if the buffer was too small
 *
 */
static BOOL URLCache_LocalFileNameToPath(LPCURLCACHE_HEADER pHeader, LPCSTR szLocalFileName, BYTE Directory, LPSTR szPath, LPLONG lpBufferSize)
{
    LONG nRequired;
    if (Directory >= pHeader->DirectoryCount)
    {
        *lpBufferSize = 0;
        return FALSE;
    }

    nRequired = (strlen(szCacheContentPath) + DIR_LENGTH + strlen(szLocalFileName) + 1) * sizeof(CHAR);
    if (nRequired < *lpBufferSize)
    {
        strcpy(szPath, szCacheContentPath);
        strncat(szPath, pHeader->directory_data[Directory].filename, DIR_LENGTH);
        strcat(szPath, "\\");
        strcat(szPath, szLocalFileName);
        *lpBufferSize = nRequired;
        return TRUE;
    }
    *lpBufferSize = nRequired;
    return FALSE;
}

/***********************************************************************
 *           URLCache_CopyEntry (Internal)
 *
 *  Copies an entry from the cache index file to the Win32 structure
 *
 * RETURNS
 *    TRUE if the buffer was big enough
 *    FALSE if the buffer was too small
 *
 */
static BOOL URLCache_CopyEntry(LPCURLCACHE_HEADER pHeader, LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo, LPDWORD lpdwBufferSize, URL_CACHEFILE_ENTRY * pUrlEntry)
{
    int lenUrl = strlen(pUrlEntry->szSourceUrlName);
    DWORD dwRequiredSize = sizeof(*lpCacheEntryInfo);
    LONG nLocalFilePathSize;
    LPSTR lpszLocalFileName;

    if (*lpdwBufferSize >= dwRequiredSize)
    {
        lpCacheEntryInfo->lpHeaderInfo = NULL;
        lpCacheEntryInfo->lpszFileExtension = NULL;
        lpCacheEntryInfo->lpszLocalFileName = NULL;
        lpCacheEntryInfo->lpszSourceUrlName = NULL;
        lpCacheEntryInfo->CacheEntryType = pUrlEntry->CacheEntryType;
        lpCacheEntryInfo->u.dwExemptDelta = pUrlEntry->dwExemptDelta;
        lpCacheEntryInfo->dwHeaderInfoSize = pUrlEntry->dwHeaderInfoSize;
        lpCacheEntryInfo->dwHitRate = pUrlEntry->dwHitRate;
        lpCacheEntryInfo->dwSizeHigh = pUrlEntry->dwSizeHigh;
        lpCacheEntryInfo->dwSizeLow = pUrlEntry->dwSizeLow;
        lpCacheEntryInfo->dwStructSize = sizeof(*lpCacheEntryInfo);
        lpCacheEntryInfo->dwUseCount = pUrlEntry->dwUseCount;
        DosDateTimeToFileTime(pUrlEntry->wExpiredDate, pUrlEntry->wExpiredTime, &lpCacheEntryInfo->ExpireTime);
        lpCacheEntryInfo->LastAccessTime.dwHighDateTime = pUrlEntry->LastAccessTime.dwHighDateTime;
        lpCacheEntryInfo->LastAccessTime.dwLowDateTime = pUrlEntry->LastAccessTime.dwLowDateTime;
        lpCacheEntryInfo->LastModifiedTime.dwHighDateTime = pUrlEntry->LastModifiedTime.dwHighDateTime;
        lpCacheEntryInfo->LastModifiedTime.dwLowDateTime = pUrlEntry->LastModifiedTime.dwLowDateTime;
        DosDateTimeToFileTime(pUrlEntry->wLastSyncDate, pUrlEntry->wLastSyncTime, &lpCacheEntryInfo->LastSyncTime);
    }

    if ((dwRequiredSize % 4) && (dwRequiredSize < *lpdwBufferSize))
        ZeroMemory((LPBYTE)lpCacheEntryInfo + dwRequiredSize, 4 - (dwRequiredSize % 4));
    dwRequiredSize = DWORD_ALIGN(dwRequiredSize);
    dwRequiredSize += lenUrl + 1;
    
    if (*lpdwBufferSize >= dwRequiredSize)
    {
        lpCacheEntryInfo->lpszSourceUrlName = (LPSTR)lpCacheEntryInfo + dwRequiredSize - lenUrl - 1;
        strcpy(lpCacheEntryInfo->lpszSourceUrlName, pUrlEntry->szSourceUrlName);
    }

    if ((dwRequiredSize % 4) && (dwRequiredSize < *lpdwBufferSize))
        ZeroMemory((LPBYTE)lpCacheEntryInfo + dwRequiredSize, 4 - (dwRequiredSize % 4));
    dwRequiredSize = DWORD_ALIGN(dwRequiredSize);

    lpszLocalFileName = (LPSTR)lpCacheEntryInfo + dwRequiredSize;
    nLocalFilePathSize = *lpdwBufferSize - dwRequiredSize;
    if (URLCache_LocalFileNameToPath(pHeader, (LPSTR)pUrlEntry + pUrlEntry->dwOffsetLocalName, pUrlEntry->CacheDir, lpszLocalFileName, &nLocalFilePathSize))
    {
        lpCacheEntryInfo->lpszLocalFileName = lpszLocalFileName;
    }
    dwRequiredSize += nLocalFilePathSize;

    if ((dwRequiredSize % 4) && (dwRequiredSize < *lpdwBufferSize))
        ZeroMemory((LPBYTE)lpCacheEntryInfo + dwRequiredSize, 4 - (dwRequiredSize % 4));
    dwRequiredSize = DWORD_ALIGN(dwRequiredSize);
    dwRequiredSize += pUrlEntry->dwHeaderInfoSize + 1;

    if (*lpdwBufferSize >= dwRequiredSize)
    {
        lpCacheEntryInfo->lpHeaderInfo = (LPSTR)lpCacheEntryInfo + dwRequiredSize - pUrlEntry->dwHeaderInfoSize - 1;
        memcpy(lpCacheEntryInfo->lpHeaderInfo, (LPSTR)pUrlEntry + pUrlEntry->dwOffsetHeaderInfo, pUrlEntry->dwHeaderInfoSize);
        ((LPBYTE)lpCacheEntryInfo)[dwRequiredSize - 1] = '\0';
    }
    if ((dwRequiredSize % 4) && (dwRequiredSize < *lpdwBufferSize))
        ZeroMemory((LPBYTE)lpCacheEntryInfo + dwRequiredSize, 4 - (dwRequiredSize % 4));
    dwRequiredSize = DWORD_ALIGN(dwRequiredSize);

    if (dwRequiredSize > *lpdwBufferSize)
    {
        *lpdwBufferSize = dwRequiredSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    *lpdwBufferSize = dwRequiredSize;
    return TRUE;
}

/***********************************************************************
 *           URLCache_HashKey (Internal)
 *
 *  Returns the hash key for a given string
 *
 * RETURNS
 *    hash key for the string
 *
 */
static DWORD URLCache_HashKey(LPCSTR lpszKey)
{
    /* NOTE: this uses the same lookup table as SHLWAPI.UrlHash{A,W}
     * but the algorithm and result are not the same!
     */
    static const unsigned char lookupTable[256] = 
    {
        0x01, 0x0E, 0x6E, 0x19, 0x61, 0xAE, 0x84, 0x77,
        0x8A, 0xAA, 0x7D, 0x76, 0x1B, 0xE9, 0x8C, 0x33,
        0x57, 0xC5, 0xB1, 0x6B, 0xEA, 0xA9, 0x38, 0x44,
        0x1E, 0x07, 0xAD, 0x49, 0xBC, 0x28, 0x24, 0x41,
        0x31, 0xD5, 0x68, 0xBE, 0x39, 0xD3, 0x94, 0xDF,
        0x30, 0x73, 0x0F, 0x02, 0x43, 0xBA, 0xD2, 0x1C,
        0x0C, 0xB5, 0x67, 0x46, 0x16, 0x3A, 0x4B, 0x4E,
        0xB7, 0xA7, 0xEE, 0x9D, 0x7C, 0x93, 0xAC, 0x90,
        0xB0, 0xA1, 0x8D, 0x56, 0x3C, 0x42, 0x80, 0x53,
        0x9C, 0xF1, 0x4F, 0x2E, 0xA8, 0xC6, 0x29, 0xFE,
        0xB2, 0x55, 0xFD, 0xED, 0xFA, 0x9A, 0x85, 0x58,
        0x23, 0xCE, 0x5F, 0x74, 0xFC, 0xC0, 0x36, 0xDD,
        0x66, 0xDA, 0xFF, 0xF0, 0x52, 0x6A, 0x9E, 0xC9,
        0x3D, 0x03, 0x59, 0x09, 0x2A, 0x9B, 0x9F, 0x5D,
        0xA6, 0x50, 0x32, 0x22, 0xAF, 0xC3, 0x64, 0x63,
        0x1A, 0x96, 0x10, 0x91, 0x04, 0x21, 0x08, 0xBD,
        0x79, 0x40, 0x4D, 0x48, 0xD0, 0xF5, 0x82, 0x7A,
        0x8F, 0x37, 0x69, 0x86, 0x1D, 0xA4, 0xB9, 0xC2,
        0xC1, 0xEF, 0x65, 0xF2, 0x05, 0xAB, 0x7E, 0x0B,
        0x4A, 0x3B, 0x89, 0xE4, 0x6C, 0xBF, 0xE8, 0x8B,
        0x06, 0x18, 0x51, 0x14, 0x7F, 0x11, 0x5B, 0x5C,
        0xFB, 0x97, 0xE1, 0xCF, 0x15, 0x62, 0x71, 0x70,
        0x54, 0xE2, 0x12, 0xD6, 0xC7, 0xBB, 0x0D, 0x20,
        0x5E, 0xDC, 0xE0, 0xD4, 0xF7, 0xCC, 0xC4, 0x2B,
        0xF9, 0xEC, 0x2D, 0xF4, 0x6F, 0xB6, 0x99, 0x88,
        0x81, 0x5A, 0xD9, 0xCA, 0x13, 0xA5, 0xE7, 0x47,
        0xE6, 0x8E, 0x60, 0xE3, 0x3E, 0xB3, 0xF6, 0x72,
        0xA2, 0x35, 0xA0, 0xD7, 0xCD, 0xB4, 0x2F, 0x6D,
        0x2C, 0x26, 0x1F, 0x95, 0x87, 0x00, 0xD8, 0x34,
        0x3F, 0x17, 0x25, 0x45, 0x27, 0x75, 0x92, 0xB8,
        0xA3, 0xC8, 0xDE, 0xEB, 0xF8, 0xF3, 0xDB, 0x0A,
        0x98, 0x83, 0x7B, 0xE5, 0xCB, 0x4C, 0x78, 0xD1
    };
    BYTE key[4];
    int i;
    int subscript[sizeof(key) / sizeof(key[0])];

    subscript[0] = *lpszKey;
    subscript[1] = (char)(*lpszKey + 1);
    subscript[2] = (char)(*lpszKey + 2);
    subscript[3] = (char)(*lpszKey + 3);

    for (i = 0; i < sizeof(key) / sizeof(key[0]); i++)
        key[i] = lookupTable[i];

    for (lpszKey++; *lpszKey && ((lpszKey[0] != '/') || (lpszKey[1] != 0)); lpszKey++)
    {
        for (i = 0; i < sizeof(key) / sizeof(key[0]); i++)
            key[i] = lookupTable[*lpszKey ^ key[i]];
    }

    return *(DWORD *)key;
}

static inline HASH_CACHEFILE_ENTRY * URLCache_HashEntryFromOffset(LPCURLCACHE_HEADER pHeader, DWORD dwOffset)
{
    return (HASH_CACHEFILE_ENTRY *)((LPBYTE)pHeader + dwOffset);
}

/***********************************************************************
 *           URLCache_FindEntryInHash (Internal)
 *
 *  Searches all the hash tables in the index for the given URL and
 * returns the entry, if it was found, in ppEntry
 *
 * RETURNS
 *    TRUE if the entry was found
 *    FALSE if the entry could not be found
 *
 */
static BOOL URLCache_FindEntryInHash(LPCURLCACHE_HEADER pHeader, LPCSTR lpszUrl, CACHEFILE_ENTRY ** ppEntry)
{
    /* structure of hash table:
     *  448 entries divided into 64 blocks
     *  each block therefore contains a chain of 7 key/offset pairs
     * how position in table is calculated:
     *  1. the url is hashed in helper function
     *  2. the key % 64 * 8 is the offset
     *  3. the key in the hash table is the hash key aligned to 64
     *
     * note:
     *  there can be multiple hash tables in the file and the offset to
     *  the next one is stored in the header of the hash table
     */
    DWORD key = URLCache_HashKey(lpszUrl);
    DWORD offset = (key % HASHTABLE_NUM_ENTRIES) * sizeof(struct _HASH_ENTRY);
    HASH_CACHEFILE_ENTRY * pHashEntry;
    DWORD dwHashTableNumber = 0;

    key = (DWORD)(key / HASHTABLE_NUM_ENTRIES) * HASHTABLE_NUM_ENTRIES;

    for (pHashEntry = URLCache_HashEntryFromOffset(pHeader, pHeader->dwOffsetFirstHashTable);
         ((DWORD)((LPBYTE)pHashEntry - (LPBYTE)pHeader) >= ENTRY_START_OFFSET) && ((DWORD)((LPBYTE)pHashEntry - (LPBYTE)pHeader) < pHeader->dwFileSize);
         pHashEntry = URLCache_HashEntryFromOffset(pHeader, pHashEntry->dwAddressNext))
    {
        int i;
        if (pHashEntry->dwHashTableNumber != dwHashTableNumber++)
        {
            ERR("Error: not right hash table number (%ld) expected %ld\n", pHashEntry->dwHashTableNumber, dwHashTableNumber);
            continue;
        }
        /* make sure that it is in fact a hash entry */
        if (pHashEntry->CacheFileEntry.dwSignature != HASH_SIGNATURE)
        {
            ERR("Error: not right signature (\"%.4s\") - expected \"HASH\"\n", (LPCSTR)&pHashEntry->CacheFileEntry.dwSignature);
            continue;
        }

        for (i = 0; i < HASHTABLE_BLOCKSIZE; i++)
        {
            struct _HASH_ENTRY * pHashElement = &pHashEntry->HashTable[offset + i];
            if (key == (DWORD)(pHashElement->dwHashKey / HASHTABLE_NUM_ENTRIES) * HASHTABLE_NUM_ENTRIES)
            {
                *ppEntry = (CACHEFILE_ENTRY *)((LPBYTE)pHeader + pHashElement->dwOffsetEntry);
                return TRUE;
            }
        }
    }
    return FALSE;
}

/***********************************************************************
 *           URLCache_HashEntrySetUse (Internal)
 *
 *  Searches all the hash tables in the index for the given URL and
 * sets the use count (stored or'ed with key)
 *
 * RETURNS
 *    TRUE if the entry was found
 *    FALSE if the entry could not be found
 *
 */
static BOOL URLCache_HashEntrySetUse(LPCURLCACHE_HEADER pHeader, LPCSTR lpszUrl, DWORD dwUseCount)
{
    /* see URLCache_FindEntryInHash for structure of hash tables */

    DWORD key = URLCache_HashKey(lpszUrl);
    DWORD offset = (key % HASHTABLE_NUM_ENTRIES) * sizeof(struct _HASH_ENTRY);
    HASH_CACHEFILE_ENTRY * pHashEntry;
    DWORD dwHashTableNumber = 0;

    if (dwUseCount >= HASHTABLE_NUM_ENTRIES)
    {
        ERR("don't know what to do when use count exceeds %d, guessing\n", HASHTABLE_NUM_ENTRIES);
        dwUseCount = HASHTABLE_NUM_ENTRIES - 1;
    }

    key = (DWORD)(key / HASHTABLE_NUM_ENTRIES) * HASHTABLE_BLOCKSIZE;

    for (pHashEntry = URLCache_HashEntryFromOffset(pHeader, pHeader->dwOffsetFirstHashTable);
         ((DWORD)((LPBYTE)pHashEntry - (LPBYTE)pHeader) >= ENTRY_START_OFFSET) && ((DWORD)((LPBYTE)pHashEntry - (LPBYTE)pHeader) < pHeader->dwFileSize);
         pHashEntry = URLCache_HashEntryFromOffset(pHeader, pHashEntry->dwAddressNext))
    {
        int i;
        if (pHashEntry->dwHashTableNumber != dwHashTableNumber++)
        {
            ERR("not right hash table number (%ld) expected %ld\n", pHashEntry->dwHashTableNumber, dwHashTableNumber);
            continue;
        }
        /* make sure that it is in fact a hash entry */
        if (pHashEntry->CacheFileEntry.dwSignature != HASH_SIGNATURE)
        {
            ERR("not right signature (\"%.4s\") - expected \"HASH\"\n", (LPCSTR)&pHashEntry->CacheFileEntry.dwSignature);
            continue;
        }

        for (i = 0; i < 7; i++)
        {
            struct _HASH_ENTRY * pHashElement = &pHashEntry->HashTable[offset + i];
            if (key == (DWORD)(pHashElement->dwHashKey / HASHTABLE_NUM_ENTRIES) * HASHTABLE_NUM_ENTRIES)
            {
                pHashElement->dwOffsetEntry = dwUseCount | (DWORD)(pHashElement->dwHashKey / HASHTABLE_NUM_ENTRIES) * HASHTABLE_NUM_ENTRIES;
                return TRUE;
            }
        }
    }
    return FALSE;
}

/***********************************************************************
 *           URLCache_AddEntryToHash (Internal)
 *
 *  Searches all the hash tables for a free slot based on the offset
 * generated from the hash key. If a free slot is found, the offset and
 * key are entered into the hash table.
 *
 * RETURNS
 *    TRUE if the entry was added
 *    FALSE if the entry could not be added
 *
 */
static BOOL URLCache_AddEntryToHash(LPCURLCACHE_HEADER pHeader, LPCSTR lpszUrl, DWORD dwOffsetEntry)
{
    /* see URLCache_FindEntryInHash for structure of hash tables */

    DWORD key = URLCache_HashKey(lpszUrl);
    DWORD offset = (key % HASHTABLE_NUM_ENTRIES) * sizeof(struct _HASH_ENTRY);
    HASH_CACHEFILE_ENTRY * pHashEntry;
    DWORD dwHashTableNumber = 0;

    key = (DWORD)(key / HASHTABLE_NUM_ENTRIES) * HASHTABLE_NUM_ENTRIES;

    for (pHashEntry = URLCache_HashEntryFromOffset(pHeader, pHeader->dwOffsetFirstHashTable);
         ((DWORD)((LPBYTE)pHashEntry - (LPBYTE)pHeader) >= ENTRY_START_OFFSET) && ((DWORD)((LPBYTE)pHashEntry - (LPBYTE)pHeader) < pHeader->dwFileSize);
         pHashEntry = URLCache_HashEntryFromOffset(pHeader, pHashEntry->dwAddressNext))
    {
        int i;
        if (pHashEntry->dwHashTableNumber != dwHashTableNumber++)
        {
            ERR("not right hash table number (%ld) expected %ld\n", pHashEntry->dwHashTableNumber, dwHashTableNumber);
            break;
        }
        /* make sure that it is in fact a hash entry */
        if (pHashEntry->CacheFileEntry.dwSignature != HASH_SIGNATURE)
        {
            ERR("not right signature (\"%.4s\") - expected \"HASH\"\n", (LPCSTR)&pHashEntry->CacheFileEntry.dwSignature);
            break;
        }

        for (i = 0; i < 7; i++)
        {
            struct _HASH_ENTRY * pHashElement = &pHashEntry->HashTable[offset + i];
            if (pHashElement->dwHashKey == 3 /* FIXME: just 3? */) /* if the slot is free */
            {
                pHashElement->dwHashKey = key;
                pHashElement->dwOffsetEntry = dwOffsetEntry;
                return TRUE;
            }
        }
    }
    FIXME("need to create another hash table\n");
    return FALSE;
}

/***********************************************************************
 *           GetUrlCacheEntryInfoExA (WININET.@)
 *
 */
BOOL WINAPI GetUrlCacheEntryInfoExA(
    LPCSTR lpszUrl,
    LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    LPDWORD lpdwCacheEntryInfoBufSize,
    LPSTR lpszReserved,
    LPDWORD lpdwReserved,
    LPVOID lpReserved,
    DWORD dwFlags)
{
    TRACE("(%s, %p, %p, %p, %p, %p, %lx)\n",
        debugstr_a(lpszUrl), 
        lpCacheEntryInfo,
        lpdwCacheEntryInfoBufSize,
        lpszReserved,
        lpdwReserved,
        lpReserved,
        dwFlags);

    if ((lpszReserved != NULL) ||
        (lpdwReserved != NULL) ||
        (lpReserved != NULL))
    {
        ERR("Reserved value was not 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (dwFlags != 0)
        FIXME("Undocumented flag(s): %lx\n", dwFlags);
    return GetUrlCacheEntryInfoA(lpszUrl, lpCacheEntryInfo, lpdwCacheEntryInfoBufSize);
}

/***********************************************************************
 *           GetUrlCacheEntryInfoA (WININET.@)
 *
 */
BOOL WINAPI GetUrlCacheEntryInfoA(
    IN LPCSTR lpszUrlName,
    IN LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize
)
{
    LPURLCACHE_HEADER pHeader;
    CACHEFILE_ENTRY * pEntry;
    URL_CACHEFILE_ENTRY * pUrlEntry;

    if (!URLCache_OpenIndex())
        return FALSE;

    if (!(pHeader = URLCache_LockIndex()))
        return FALSE;

    if (!URLCache_FindEntryInHash(pHeader, lpszUrlName, &pEntry))
    {
        if (!URLCache_FindEntry(pHeader, lpszUrlName, &pEntry))
        {
            URLCache_UnlockIndex(pHeader);
            WARN("entry %s not found!\n", debugstr_a(lpszUrlName));
            SetLastError(ERROR_FILE_NOT_FOUND);
            return FALSE;
        }
    }

    /* FIXME: check signature */

    pUrlEntry = (URL_CACHEFILE_ENTRY *)pEntry;
    TRACE("Found URL: %s\n", pUrlEntry->szSourceUrlName);
    TRACE("Header info: %s\n", (LPBYTE)pUrlEntry + pUrlEntry->dwOffsetHeaderInfo);

    if (!URLCache_CopyEntry(pHeader, lpCacheEntryInfo, lpdwCacheEntryInfoBufferSize, pUrlEntry))
    {
        URLCache_UnlockIndex(pHeader);
        return FALSE;
    }
    TRACE("Local File Name: %s\n", lpCacheEntryInfo->lpszLocalFileName);

    URLCache_UnlockIndex(pHeader);

    return TRUE;
}

BOOL WINAPI RetrieveUrlCacheEntryFileA(
    IN LPCSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo, 
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN DWORD dwReserved
    )
{
    LPURLCACHE_HEADER pHeader;
    CACHEFILE_ENTRY * pEntry;
    URL_CACHEFILE_ENTRY * pUrlEntry;

    if (!URLCache_OpenIndex())
        return FALSE;

    if (!(pHeader = URLCache_LockIndex()))
        return FALSE;

    if (!URLCache_FindEntryInHash(pHeader, lpszUrlName, &pEntry))
    {
        if (!URLCache_FindEntry(pHeader, lpszUrlName, &pEntry))
        {
            URLCache_UnlockIndex(pHeader);
            TRACE("entry %s not found!\n", lpszUrlName);
            SetLastError(ERROR_FILE_NOT_FOUND);
            return FALSE;
        }
    }

    /* FIXME: check signature */

    pUrlEntry = (URL_CACHEFILE_ENTRY *)pEntry;
    TRACE("Found URL: %s\n", pUrlEntry->szSourceUrlName);
    TRACE("Header info: %s\n", (LPBYTE)pUrlEntry + pUrlEntry->dwOffsetHeaderInfo);

    pUrlEntry->dwHitRate++;
    pUrlEntry->dwUseCount++;
    URLCache_HashEntrySetUse(pHeader, lpszUrlName, pUrlEntry->dwUseCount);

    if (!URLCache_CopyEntry(pHeader, lpCacheEntryInfo, lpdwCacheEntryInfoBufferSize, pUrlEntry))
    {
        URLCache_UnlockIndex(pHeader);
        return FALSE;
    }
    TRACE("Local File Name: %s\n", lpCacheEntryInfo->lpszLocalFileName);

    URLCache_UnlockIndex(pHeader);

    return TRUE;
}

BOOL WINAPI UnlockUrlCacheEntryFileA(
    IN LPCSTR lpszUrlName, 
    IN DWORD dwReserved
    )
{
    LPURLCACHE_HEADER pHeader;
    CACHEFILE_ENTRY * pEntry;
    URL_CACHEFILE_ENTRY * pUrlEntry;

    if (dwReserved)
    {
        ERR("dwReserved != 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!URLCache_OpenIndex())
        return FALSE;

    if (!(pHeader = URLCache_LockIndex()))
        return FALSE;

    if (!URLCache_FindEntryInHash(pHeader, lpszUrlName, &pEntry))
    {
        if (!URLCache_FindEntry(pHeader, lpszUrlName, &pEntry))
        {
            URLCache_UnlockIndex(pHeader);
            TRACE("entry %s not found!\n", lpszUrlName);
            SetLastError(ERROR_FILE_NOT_FOUND);
            return FALSE;
        }
    }

    /* FIXME: check signature */

    pUrlEntry = (URL_CACHEFILE_ENTRY *)pEntry;

    if (pUrlEntry->dwUseCount == 0)
    {
        URLCache_UnlockIndex(pHeader);
        return FALSE;
    }
    pUrlEntry->dwUseCount--;
    URLCache_HashEntrySetUse(pHeader, lpszUrlName, pUrlEntry->dwUseCount);

    URLCache_UnlockIndex(pHeader);

    return TRUE;
}

/***********************************************************************
 *           CreateUrlCacheEntryA (WININET.@)
 *
 */
BOOL WINAPI CreateUrlCacheEntryA(
    IN LPCSTR lpszUrlName,
    IN DWORD dwExpectedFileSize,
    IN LPCSTR lpszFileExtension,
    OUT LPSTR lpszFileName,
    IN DWORD dwReserved
)
{
    LPURLCACHE_HEADER pHeader;
    CHAR szFile[MAX_PATH];
    CHAR szExtension[MAX_PATH];
    LPCSTR lpszUrlPart;
    LPCSTR lpszUrlEnd;
    LPCSTR lpszFileNameExtension;
    LPSTR lpszFileNameNoPath;
    int i;
    int countnoextension;
    BYTE CacheDir;
    LONG lBufferSize = MAX_PATH * sizeof(CHAR);
    BOOL bFound = FALSE;
    int count;

    if (dwReserved)
    {
        ERR("dwReserved != 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for (lpszUrlEnd = lpszUrlName; *lpszUrlEnd; lpszUrlEnd++)
        ;
    
    if (((lpszUrlEnd - lpszUrlName) > 1) && (*(lpszUrlEnd - 1) == '/'))
        lpszUrlEnd--;

    for (lpszUrlPart = lpszUrlEnd; 
        (lpszUrlPart >= lpszUrlName); 
        lpszUrlPart--)
    {
        if ((*lpszUrlPart == '/') && ((lpszUrlEnd - lpszUrlPart) > 1))
        {
            bFound = TRUE;
            lpszUrlPart++;
            break;
        }
    }
    if (!strcmp(lpszUrlPart, "www"))
    {
        lpszUrlPart += strlen("www");
    }

    count = lpszUrlEnd - lpszUrlPart;

    if (bFound && (count < MAX_PATH))
    {
        memcpy(szFile, lpszUrlPart, count * sizeof(CHAR));
        szFile[count] = '\0';
        /* FIXME: get rid of illegal characters like \, / and : */
    }
    else
    {
        FIXME("need to generate a random filename");
    }

    TRACE("File name: %s\n", szFile);

    if (!URLCache_OpenIndex())
        return FALSE;

    if (!(pHeader = URLCache_LockIndex()))
        return FALSE;

    CacheDir = (BYTE)(rand() % pHeader->DirectoryCount);

    URLCache_LocalFileNameToPath(pHeader, szFile, CacheDir, lpszFileName, &lBufferSize);

    URLCache_UnlockIndex(pHeader);

    lpszFileNameNoPath = lpszFileName + strlen(szCacheContentPath) + DIR_LENGTH + 1;

    countnoextension = strlen(lpszFileNameNoPath);
    lpszFileNameExtension = PathFindExtensionA(lpszFileNameNoPath);
    if (lpszFileNameExtension)
        countnoextension -= strlen(lpszFileNameExtension);
    *szExtension = '\0';

    if (lpszFileExtension)
    {
        szExtension[0] = '.';
        strcpy(szExtension+1, lpszFileExtension);
    }

    for (i = 0; i < 255; i++)
    {
        HANDLE hFile;
        strncpy(lpszFileNameNoPath, szFile, countnoextension);
        sprintf(lpszFileNameNoPath + countnoextension, "[%u]%s", i, szExtension);
        TRACE("Trying: %s\n", lpszFileName);
        hFile = CreateFileA(lpszFileName, GENERIC_READ, 0, NULL, CREATE_NEW, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
            return TRUE;
        }
    }

    return FALSE;
}

/***********************************************************************
 *           CommitUrlCacheEntryA (WININET.@)
 *
 */
BOOL WINAPI CommitUrlCacheEntryA(
    IN LPCSTR lpszUrlName,
    IN LPCSTR lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPBYTE lpHeaderInfo,
    IN DWORD dwHeaderSize,
    IN LPCSTR lpszFileExtension,
    IN LPCSTR dwReserved
    )
{
    LPURLCACHE_HEADER pHeader;
    CACHEFILE_ENTRY * pEntry;
    URL_CACHEFILE_ENTRY * pUrlEntry;
    DWORD dwBytesNeeded = sizeof(*pUrlEntry) - sizeof(pUrlEntry->szSourceUrlName);
    BYTE cDirectory;
    BOOL bFound = FALSE;
    DWORD dwOffsetLocalFileName;
    DWORD dwOffsetHeader;
    DWORD dwFileSizeLow;
    DWORD dwFileSizeHigh;
    HANDLE hFile;

    TRACE("(%s, %s, ..., ..., %lx, %p, %ld, %s, %p)\n",
        debugstr_a(lpszUrlName),
        debugstr_a(lpszLocalFileName),
        CacheEntryType,
        lpHeaderInfo,
        dwHeaderSize,
        lpszFileExtension,
        dwReserved);

    if (dwReserved)
    {
        ERR("dwReserved != 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (lpHeaderInfo == NULL)
    {
        FIXME("lpHeaderInfo == NULL - will crash at the moment\n");
    }

    hFile = CreateFileA(lpszLocalFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ERR("couldn't open file (error is %ld)\n", GetLastError());
        return FALSE;
    }
    
    /* Get file size */
    dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHigh);
    if ((dwFileSizeLow == -1) && (GetLastError() != NO_ERROR))
    {
        ERR("couldn't get file size (error is %ld)\n", GetLastError());
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);

    if (!URLCache_OpenIndex())
        return FALSE;

    if (!(pHeader = URLCache_LockIndex()))
        return FALSE;

    if (URLCache_FindEntryInHash(pHeader, lpszUrlName, &pEntry) || URLCache_FindEntry(pHeader, lpszUrlName, &pEntry))
    {
        URLCache_UnlockIndex(pHeader);
        FIXME("entry already in cache - don't know what to do!\n");
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    if (memcmp(lpszLocalFileName, szCacheContentPath, strlen(szCacheContentPath)))
    {
        URLCache_UnlockIndex(pHeader);
        ERR("path must begin with cache content path\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    lpszLocalFileName += strlen(szCacheContentPath);

    for (cDirectory = 0; cDirectory < pHeader->DirectoryCount; cDirectory++)
    {
        if (!strncmp(pHeader->directory_data[cDirectory].filename, lpszLocalFileName, DIR_LENGTH))
        {
            bFound = TRUE;
            break;
        }
    }

    if (!bFound)
    {
        URLCache_UnlockIndex(pHeader);
        ERR("cache directory not found in path %s\n", lpszLocalFileName);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    lpszLocalFileName += (DIR_LENGTH + 1); /* "1234WXYZ\" */

    dwOffsetLocalFileName = DWORD_ALIGN(dwBytesNeeded + strlen(lpszUrlName) + 1); /* + 1 for NULL terminating char */
    dwOffsetHeader = DWORD_ALIGN(dwOffsetLocalFileName + strlen(lpszLocalFileName) + 1);
    dwBytesNeeded = DWORD_ALIGN(dwBytesNeeded + dwHeaderSize);

    if (dwBytesNeeded % BLOCKSIZE)
    {
        dwBytesNeeded -= dwBytesNeeded % BLOCKSIZE;
        dwBytesNeeded += BLOCKSIZE;
    }

    if (!URLCache_FindFirstFreeEntry(pHeader, dwBytesNeeded / BLOCKSIZE, &pEntry))
    {
        /* we should grow the index file here */
        URLCache_UnlockIndex(pHeader);
        FIXME("no free entries\n");
        return FALSE;
    }

    /* FindFirstFreeEntry fills in blocks used */
    pUrlEntry = (URL_CACHEFILE_ENTRY *)pEntry;
    pUrlEntry->CacheFileEntry.dwSignature = URL_SIGNATURE;
    pUrlEntry->CacheDir = cDirectory;
    pUrlEntry->CacheEntryType = CacheEntryType;
    pUrlEntry->dwHeaderInfoSize = dwHeaderSize;
    pUrlEntry->dwExemptDelta = 0;
    pUrlEntry->dwHitRate = 0;
    pUrlEntry->dwOffsetHeaderInfo = dwOffsetHeader;
    pUrlEntry->dwOffsetLocalName = dwOffsetLocalFileName;
    pUrlEntry->dwOffsetUrl = sizeof(*pUrlEntry) - sizeof(pUrlEntry->szSourceUrlName);
    pUrlEntry->dwSizeHigh = 0;
    pUrlEntry->dwSizeLow = dwFileSizeLow;
    pUrlEntry->dwSizeHigh = dwFileSizeHigh;
    pUrlEntry->dwUseCount = 0;
    GetSystemTimeAsFileTime(&pUrlEntry->LastAccessTime);
    pUrlEntry->LastModifiedTime = LastModifiedTime;
    FileTimeToDosDateTime(&pUrlEntry->LastAccessTime, &pUrlEntry->wLastSyncDate, &pUrlEntry->wLastSyncTime);
    FileTimeToDosDateTime(&ExpireTime, &pUrlEntry->wExpiredDate, &pUrlEntry->wExpiredTime);
    pUrlEntry->wUnknownDate = pUrlEntry->wLastSyncDate;
    pUrlEntry->wUnknownTime = pUrlEntry->wLastSyncTime;

    /*** Unknowns ***/
    pUrlEntry->dwUnknown1 = 0;
    pUrlEntry->dwUnknown2 = 0;
    pUrlEntry->dwUnknown3 = 0x60;
    pUrlEntry->Unknown4 = 0;
    pUrlEntry->wUnknown5 = 0x1010;
    pUrlEntry->dwUnknown6 = 0;
    pUrlEntry->dwUnknown7 = 0;
    pUrlEntry->dwUnknown8 = 0;

    strcpy(pUrlEntry->szSourceUrlName, lpszUrlName);
    strcpy((LPSTR)((LPBYTE)pUrlEntry + dwOffsetLocalFileName), lpszLocalFileName);
    memcpy((LPSTR)((LPBYTE)pUrlEntry + dwOffsetHeader), lpHeaderInfo, dwHeaderSize);

    if (!URLCache_AddEntryToHash(pHeader, lpszUrlName, (DWORD)((LPBYTE)pUrlEntry - (LPBYTE)pHeader)))
    {
        URLCache_UnlockIndex(pHeader);
        return FALSE;
    }

    URLCache_UnlockIndex(pHeader);

    return TRUE;
}

BOOL WINAPI ReadUrlCacheEntryStream(
    IN HANDLE hUrlCacheStream,
    IN  DWORD dwLocation,
    IN OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwLen,
    IN DWORD dwReserved
    )
{
    /* Get handle to file from 'stream' */
    STREAM_HANDLE * pStream = (STREAM_HANDLE *)hUrlCacheStream;

    if (dwReserved != 0)
    {
        ERR("dwReserved != 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (IsBadReadPtr(pStream, sizeof(*pStream)) || IsBadStringPtrA(pStream->lpszUrl, INTERNET_MAX_URL_LENGTH))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (SetFilePointer(pStream->hFile, dwLocation, NULL, FILE_CURRENT) == -1)
        return FALSE;
    return ReadFile(pStream->hFile, lpBuffer, *lpdwLen, lpdwLen, NULL);
}

HANDLE WINAPI RetrieveUrlCacheEntryStreamA(
    IN LPCSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN BOOL fRandomRead,
    IN DWORD dwReserved
    )
{
    /* NOTE: this is not the same as the way that the native
     * version allocates 'stream' handles. I did it this way
     * as it is much easier and no applications should depend
     * on this behaviour. (Native version appears to allocate
     * indices into a table)
     */
    STREAM_HANDLE * pStream;
    HANDLE hFile;

    if (!RetrieveUrlCacheEntryFileA(lpszUrlName, 
        lpCacheEntryInfo, 
        lpdwCacheEntryInfoBufferSize,
        dwReserved))
    {
        return NULL;
    }
    
    hFile = CreateFileA(lpCacheEntryInfo->lpszLocalFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        fRandomRead ? FILE_FLAG_RANDOM_ACCESS : 0,
        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;
    
    /* allocate handle storage space */
    pStream = (STREAM_HANDLE *)HeapAlloc(GetProcessHeap(), 0, sizeof(STREAM_HANDLE) + strlen(lpszUrlName) * sizeof(CHAR));
    if (!pStream)
    {
        CloseHandle(hFile);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    pStream->hFile = hFile;
    strcpy(pStream->lpszUrl, lpszUrlName);
    return (HANDLE)pStream;
}

BOOL WINAPI UnlockUrlCacheEntryStream(
    IN HANDLE hUrlCacheStream,
    IN DWORD dwReserved
)
{
    STREAM_HANDLE * pStream = (STREAM_HANDLE *)hUrlCacheStream;

    if (dwReserved != 0)
    {
        ERR("dwReserved != 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (IsBadReadPtr(pStream, sizeof(*pStream)) || IsBadStringPtrA(pStream->lpszUrl, INTERNET_MAX_URL_LENGTH))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!UnlockUrlCacheEntryFileA(pStream->lpszUrl, 0))
        return FALSE;

    /* close file handle */
    CloseHandle(pStream->hFile);

    /* free allocated space */
    HeapFree(GetProcessHeap(), 0, pStream);

    return TRUE;
}

BOOL WINAPI DeleteUrlCacheEntryA(LPCSTR lpszUrlName)
{
    LPURLCACHE_HEADER pHeader;
    CACHEFILE_ENTRY * pEntry;
    DWORD dwStartBlock;
    DWORD dwBlock;
    BYTE * AllocationTable;

    if (!URLCache_OpenIndex())
        return FALSE;

    if (!(pHeader = URLCache_LockIndex()))
        return FALSE;

    if (!URLCache_FindEntryInHash(pHeader, lpszUrlName, &pEntry))
    {
        if (!URLCache_FindEntry(pHeader, lpszUrlName, &pEntry))
        {
            URLCache_UnlockIndex(pHeader);
            TRACE("entry %s not found!\n", lpszUrlName);
            SetLastError(ERROR_FILE_NOT_FOUND);
            return FALSE;
        }
    }

    AllocationTable = (LPBYTE)pHeader + ALLOCATION_TABLE_OFFSET;

    /* update allocation table */
    dwStartBlock = ((DWORD)pEntry - (DWORD)pHeader) / BLOCKSIZE;
    for (dwBlock = dwStartBlock; dwBlock < dwStartBlock + pEntry->dwBlocksUsed; dwBlock++)
        URLCache_Allocation_BlockFree(AllocationTable, dwBlock);

    URLCache_DeleteEntry(pEntry);

    /* FIXME: update hash table */

    URLCache_UnlockIndex(pHeader);

    return TRUE;
}

INTERNETAPI GROUPID WINAPI CreateUrlCacheGroup(DWORD dwFlags, LPVOID 
lpReserved)
{
  FIXME("(%lx, %p): stub\n", dwFlags, lpReserved);
  return FALSE;
}

INTERNETAPI HANDLE WINAPI FindFirstUrlCacheEntryA(LPCSTR lpszUrlSearchPattern,
 LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo, LPDWORD lpdwFirstCacheEntryInfoBufferSize)
{
  FIXME("(%s, %p, %p): stub\n", debugstr_a(lpszUrlSearchPattern), lpFirstCacheEntryInfo, lpdwFirstCacheEntryInfoBufferSize);
  return 0;
}

INTERNETAPI HANDLE WINAPI FindFirstUrlCacheEntryW(LPCWSTR lpszUrlSearchPattern,
 LPINTERNET_CACHE_ENTRY_INFOW lpFirstCacheEntryInfo, LPDWORD lpdwFirstCacheEntryInfoBufferSize)
{
  FIXME("(%s, %p, %p): stub\n", debugstr_w(lpszUrlSearchPattern), lpFirstCacheEntryInfo, lpdwFirstCacheEntryInfoBufferSize);
  return 0;
}

BOOL WINAPI DeleteUrlCacheGroup(GROUPID GroupId, DWORD dwFlags, LPVOID lpReserved)
{
    FIXME("STUB\n");
    return FALSE;
}

BOOL WINAPI SetUrlCacheEntryGroup(LPCSTR lpszUrlName, DWORD dwFlags,
  GROUPID GroupId, LPBYTE pbGroupAttributes, DWORD cbGroupAttributes,
  LPVOID lpReserved)
{
    FIXME("STUB\n");
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

/***********************************************************************
 *           GetUrlCacheEntryInfoW (WININET.@)
 *
 */
BOOL WINAPI GetUrlCacheEntryInfoW(LPCWSTR lpszUrl,
  LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntry,
  LPDWORD lpCacheEntrySize)
{
    FIXME("(%s) stub\n",debugstr_w(lpszUrl));
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

/***********************************************************************
 *           GetUrlCacheEntryInfoExW (WININET.@)
 *
 */
BOOL WINAPI GetUrlCacheEntryInfoExW(
    LPCWSTR lpszUrl,
    LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    LPDWORD lpdwCacheEntryInfoBufSize,
    LPWSTR lpszReserved,
    LPDWORD lpdwReserved,
    LPVOID lpReserved,
    DWORD dwFlags)
{
    FIXME(" url=%s, flags=%ld\n",debugstr_w(lpszUrl),dwFlags);
    INTERNET_SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

/***********************************************************************
 *           GetUrlCacheConfigInfoA (WININET.@)
 *
 * CacheInfo is some CACHE_CONFIG_INFO structure, with no MS info found by google
 */
BOOL WINAPI GetUrlCacheConfigInfoA(LPDWORD CacheInfo, LPDWORD size, DWORD bitmask)
{
    FIXME("(%p, %p, %lx)\n", CacheInfo, size, bitmask);
    INTERNET_SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

/***********************************************************************
 *           GetUrlCacheConfigInfoW (WININET.@)
 */
BOOL WINAPI GetUrlCacheConfigInfoW(LPDWORD CacheInfo, LPDWORD size, DWORD bitmask)
{
    FIXME("(%p, %p, %lx)\n", CacheInfo, size, bitmask);
    INTERNET_SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}


/***********************************************************************
 *           SetUrlCacheEntryInfoA (WININET.@)
 */
BOOL WINAPI SetUrlCacheEntryInfoA(LPCSTR lpszUrlName, LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo, DWORD dwFieldControl)
{
    FIXME("stub\n");
    return FALSE;
}

/***********************************************************************
 *           SetUrlCacheEntryInfoW (WININET.@)
 */
BOOL WINAPI SetUrlCacheEntryInfoW(LPCWSTR lpszUrlName, LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo, DWORD dwFieldControl)
{
    FIXME("stub\n");
    return FALSE;
}
