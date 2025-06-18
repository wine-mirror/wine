/*
 * Setupapi miscellaneous functions
 *
 * Copyright 2005 Eric Kohl
 * Copyright 2007 Hans Leidekker
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "wincrypt.h"
#include "setupapi.h"
#include "lzexpand.h"
#include "mscat.h"
#include "shlobj.h"

#include "wine/debug.h"

#include "setupapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/* Handles and critical sections for the SetupLog API */
static HANDLE setupact = INVALID_HANDLE_VALUE;
static HANDLE setuperr = INVALID_HANDLE_VALUE;
static CRITICAL_SECTION setupapi_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &setupapi_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": setupapi_cs") }
};
static CRITICAL_SECTION setupapi_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

/**************************************************************************
 * MyFree [SETUPAPI.@]
 *
 * Frees an allocated memory block from the process heap.
 *
 * PARAMS
 *     lpMem [I] pointer to memory block which will be freed
 *
 * RETURNS
 *     None
 */
VOID WINAPI MyFree(LPVOID lpMem)
{
    free(lpMem);
}


/**************************************************************************
 * MyMalloc [SETUPAPI.@]
 *
 * Allocates memory block from the process heap.
 *
 * PARAMS
 *     dwSize [I] size of the allocated memory block
 *
 * RETURNS
 *     Success: pointer to allocated memory block
 *     Failure: NULL
 */
LPVOID WINAPI MyMalloc(DWORD dwSize)
{
    return malloc(dwSize);
}


/**************************************************************************
 * MyRealloc [SETUPAPI.@]
 *
 * Changes the size of an allocated memory block or allocates a memory
 * block from the process heap.
 *
 * PARAMS
 *     lpSrc  [I] pointer to memory block which will be resized
 *     dwSize [I] new size of the memory block
 *
 * RETURNS
 *     Success: pointer to the resized memory block
 *     Failure: NULL
 *
 * NOTES
 *     If lpSrc is a NULL-pointer, then MyRealloc allocates a memory
 *     block like MyMalloc.
 */
LPVOID WINAPI MyRealloc(LPVOID lpSrc, DWORD dwSize)
{
    return realloc(lpSrc, dwSize);
}


/**************************************************************************
 * DuplicateString [SETUPAPI.@]
 *
 * Duplicates a unicode string.
 *
 * PARAMS
 *     lpSrc  [I] pointer to the unicode string that will be duplicated
 *
 * RETURNS
 *     Success: pointer to the duplicated unicode string
 *     Failure: NULL
 *
 * NOTES
 *     Call MyFree() to release the duplicated string.
 */
LPWSTR WINAPI DuplicateString(LPCWSTR lpSrc)
{
    LPWSTR lpDst;

    lpDst = MyMalloc((lstrlenW(lpSrc) + 1) * sizeof(WCHAR));
    if (lpDst == NULL)
        return NULL;

    lstrcpyW(lpDst, lpSrc);

    return lpDst;
}


/**************************************************************************
 * QueryRegistryValue [SETUPAPI.@]
 *
 * Retrieves value data from the registry and allocates memory for the
 * value data.
 *
 * PARAMS
 *     hKey        [I] Handle of the key to query
 *     lpValueName [I] Name of value under hkey to query
 *     lpData      [O] Destination for the values contents,
 *     lpType      [O] Destination for the value type
 *     lpcbData    [O] Destination for the size of data
 *
 * RETURNS
 *     Success: ERROR_SUCCESS
 *     Failure: Otherwise
 *
 * NOTES
 *     Use MyFree to release the lpData buffer.
 */
LONG WINAPI QueryRegistryValue(HKEY hKey,
                               LPCWSTR lpValueName,
                               LPBYTE  *lpData,
                               LPDWORD lpType,
                               LPDWORD lpcbData)
{
    LONG lError;

    TRACE("%p %s %p %p %p\n",
          hKey, debugstr_w(lpValueName), lpData, lpType, lpcbData);

    /* Get required buffer size */
    *lpcbData = 0;
    lError = RegQueryValueExW(hKey, lpValueName, 0, lpType, NULL, lpcbData);
    if (lError != ERROR_SUCCESS)
        return lError;

    /* Allocate buffer */
    *lpData = MyMalloc(*lpcbData);
    if (*lpData == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Query registry value */
    lError = RegQueryValueExW(hKey, lpValueName, 0, lpType, *lpData, lpcbData);
    if (lError != ERROR_SUCCESS)
        MyFree(*lpData);

    return lError;
}


/**************************************************************************
 * IsUserAdmin [SETUPAPI.@]
 *
 * Checks whether the current user is a member of the Administrators group.
 *
 * PARAMS
 *     None
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI IsUserAdmin(VOID)
{
    TRACE("\n");
    return IsUserAnAdmin();
}


/**************************************************************************
 * MultiByteToUnicode [SETUPAPI.@]
 *
 * Converts a multi-byte string to a Unicode string.
 *
 * PARAMS
 *     lpMultiByteStr  [I] Multi-byte string to be converted
 *     uCodePage       [I] Code page
 *
 * RETURNS
 *     Success: pointer to the converted Unicode string
 *     Failure: NULL
 *
 * NOTE
 *     Use MyFree to release the returned Unicode string.
 */
LPWSTR WINAPI MultiByteToUnicode(LPCSTR lpMultiByteStr, UINT uCodePage)
{
    LPWSTR lpUnicodeStr;
    int nLength;

    nLength = MultiByteToWideChar(uCodePage, 0, lpMultiByteStr,
                                  -1, NULL, 0);
    if (nLength == 0)
        return NULL;

    lpUnicodeStr = MyMalloc(nLength * sizeof(WCHAR));
    if (lpUnicodeStr == NULL)
        return NULL;

    if (!MultiByteToWideChar(uCodePage, 0, lpMultiByteStr,
                             nLength, lpUnicodeStr, nLength))
    {
        MyFree(lpUnicodeStr);
        return NULL;
    }

    return lpUnicodeStr;
}


/**************************************************************************
 * UnicodeToMultiByte [SETUPAPI.@]
 *
 * Converts a Unicode string to a multi-byte string.
 *
 * PARAMS
 *     lpUnicodeStr  [I] Unicode string to be converted
 *     uCodePage     [I] Code page
 *
 * RETURNS
 *     Success: pointer to the converted multi-byte string
 *     Failure: NULL
 *
 * NOTE
 *     Use MyFree to release the returned multi-byte string.
 */
LPSTR WINAPI UnicodeToMultiByte(LPCWSTR lpUnicodeStr, UINT uCodePage)
{
    LPSTR lpMultiByteStr;
    int nLength;

    nLength = WideCharToMultiByte(uCodePage, 0, lpUnicodeStr, -1,
                                  NULL, 0, NULL, NULL);
    if (nLength == 0)
        return NULL;

    lpMultiByteStr = MyMalloc(nLength);
    if (lpMultiByteStr == NULL)
        return NULL;

    if (!WideCharToMultiByte(uCodePage, 0, lpUnicodeStr, -1,
                             lpMultiByteStr, nLength, NULL, NULL))
    {
        MyFree(lpMultiByteStr);
        return NULL;
    }

    return lpMultiByteStr;
}


/**************************************************************************
 * DoesUserHavePrivilege [SETUPAPI.@]
 *
 * Check whether the current user has got a given privilege.
 *
 * PARAMS
 *     lpPrivilegeName  [I] Name of the privilege to be checked
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI DoesUserHavePrivilege(LPCWSTR lpPrivilegeName)
{
    HANDLE hToken;
    DWORD dwSize;
    PTOKEN_PRIVILEGES lpPrivileges;
    LUID PrivilegeLuid;
    DWORD i;
    BOOL bResult = FALSE;

    TRACE("%s\n", debugstr_w(lpPrivilegeName));

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return FALSE;

    if (!GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &dwSize))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            CloseHandle(hToken);
            return FALSE;
        }
    }

    lpPrivileges = MyMalloc(dwSize);
    if (lpPrivileges == NULL)
    {
        CloseHandle(hToken);
        return FALSE;
    }

    if (!GetTokenInformation(hToken, TokenPrivileges, lpPrivileges, dwSize, &dwSize))
    {
        MyFree(lpPrivileges);
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);

    if (!LookupPrivilegeValueW(NULL, lpPrivilegeName, &PrivilegeLuid))
    {
        MyFree(lpPrivileges);
        return FALSE;
    }

    for (i = 0; i < lpPrivileges->PrivilegeCount; i++)
    {
        if (lpPrivileges->Privileges[i].Luid.HighPart == PrivilegeLuid.HighPart &&
            lpPrivileges->Privileges[i].Luid.LowPart == PrivilegeLuid.LowPart)
        {
            bResult = TRUE;
        }
    }

    MyFree(lpPrivileges);

    return bResult;
}


/**************************************************************************
 * EnablePrivilege [SETUPAPI.@]
 *
 * Enables or disables one of the current users privileges.
 *
 * PARAMS
 *     lpPrivilegeName  [I] Name of the privilege to be changed
 *     bEnable          [I] TRUE: Enables the privilege
 *                          FALSE: Disables the privilege
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI EnablePrivilege(LPCWSTR lpPrivilegeName, BOOL bEnable)
{
    TOKEN_PRIVILEGES Privileges;
    HANDLE hToken;
    BOOL bResult;

    TRACE("%s %s\n", debugstr_w(lpPrivilegeName), bEnable ? "TRUE" : "FALSE");

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return FALSE;

    Privileges.PrivilegeCount = 1;
    Privileges.Privileges[0].Attributes = (bEnable) ? SE_PRIVILEGE_ENABLED : 0;

    if (!LookupPrivilegeValueW(NULL, lpPrivilegeName,
                               &Privileges.Privileges[0].Luid))
    {
        CloseHandle(hToken);
        return FALSE;
    }

    bResult = AdjustTokenPrivileges(hToken, FALSE, &Privileges, 0, NULL, NULL);

    CloseHandle(hToken);

    return bResult;
}


/**************************************************************************
 * DelayedMove [SETUPAPI.@]
 *
 * Moves a file upon the next reboot.
 *
 * PARAMS
 *     lpExistingFileName  [I] Current file name
 *     lpNewFileName       [I] New file name
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI DelayedMove(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName)
{
    return MoveFileExW(lpExistingFileName, lpNewFileName,
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT);
}


/**************************************************************************
 * FileExists [SETUPAPI.@]
 *
 * Checks whether a file exists.
 *
 * PARAMS
 *     lpFileName     [I] Name of the file to check
 *     lpNewFileName  [O] Optional information about the existing file
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI FileExists(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFileFindData)
{
    WIN32_FIND_DATAW FindData;
    HANDLE hFind;
    UINT uErrorMode;
    DWORD dwError;

    uErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    hFind = FindFirstFileW(lpFileName, &FindData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        dwError = GetLastError();
        SetErrorMode(uErrorMode);
        SetLastError(dwError);
        return FALSE;
    }

    FindClose(hFind);

    if (lpFileFindData)
        *lpFileFindData = FindData;

    SetErrorMode(uErrorMode);

    return TRUE;
}


/**************************************************************************
 * CaptureStringArg [SETUPAPI.@]
 *
 * Captures a UNICODE string.
 *
 * PARAMS
 *     lpSrc  [I] UNICODE string to be captured
 *     lpDst  [O] Pointer to the captured UNICODE string
 *
 * RETURNS
 *     Success: ERROR_SUCCESS
 *     Failure: ERROR_INVALID_PARAMETER
 *
 * NOTE
 *     Call MyFree to release the captured UNICODE string.
 */
DWORD WINAPI CaptureStringArg(LPCWSTR pSrc, LPWSTR *pDst)
{
    if (pDst == NULL)
        return ERROR_INVALID_PARAMETER;

    *pDst = DuplicateString(pSrc);

    return ERROR_SUCCESS;
}


/**************************************************************************
 * CaptureAndConvertAnsiArg [SETUPAPI.@]
 *
 * Captures an ANSI string and converts it to a UNICODE string.
 *
 * PARAMS
 *     lpSrc  [I] ANSI string to be captured
 *     lpDst  [O] Pointer to the captured UNICODE string
 *
 * RETURNS
 *     Success: ERROR_SUCCESS
 *     Failure: ERROR_INVALID_PARAMETER
 *
 * NOTE
 *     Call MyFree to release the captured UNICODE string.
 */
DWORD WINAPI CaptureAndConvertAnsiArg(LPCSTR pSrc, LPWSTR *pDst)
{
    if (pDst == NULL)
        return ERROR_INVALID_PARAMETER;

    *pDst = MultiByteToUnicode(pSrc, CP_ACP);

    return ERROR_SUCCESS;
}


/**************************************************************************
 * OpenAndMapFileForRead [SETUPAPI.@]
 *
 * Open and map a file to a buffer.
 *
 * PARAMS
 *     lpFileName [I] Name of the file to be opened
 *     lpSize     [O] Pointer to the file size
 *     lpFile     [0] Pointer to the file handle
 *     lpMapping  [0] Pointer to the mapping handle
 *     lpBuffer   [0] Pointer to the file buffer
 *
 * RETURNS
 *     Success: ERROR_SUCCESS
 *     Failure: Other
 *
 * NOTE
 *     Call UnmapAndCloseFile to release the file.
 */
DWORD WINAPI OpenAndMapFileForRead(LPCWSTR lpFileName,
                                   LPDWORD lpSize,
                                   LPHANDLE lpFile,
                                   LPHANDLE lpMapping,
                                   LPVOID *lpBuffer)
{
    DWORD dwError;

    TRACE("%s %p %p %p %p\n",
          debugstr_w(lpFileName), lpSize, lpFile, lpMapping, lpBuffer);

    *lpFile = CreateFileW(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                          OPEN_EXISTING, 0, NULL);
    if (*lpFile == INVALID_HANDLE_VALUE)
        return GetLastError();

    *lpSize = GetFileSize(*lpFile, NULL);
    if (*lpSize == INVALID_FILE_SIZE)
    {
        dwError = GetLastError();
        CloseHandle(*lpFile);
        return dwError;
    }

    *lpMapping = CreateFileMappingW(*lpFile, NULL, PAGE_READONLY, 0,
                                    *lpSize, NULL);
    if (*lpMapping == NULL)
    {
        dwError = GetLastError();
        CloseHandle(*lpFile);
        return dwError;
    }

    *lpBuffer = MapViewOfFile(*lpMapping, FILE_MAP_READ, 0, 0, *lpSize);
    if (*lpBuffer == NULL)
    {
        dwError = GetLastError();
        CloseHandle(*lpMapping);
        CloseHandle(*lpFile);
        return dwError;
    }

    return ERROR_SUCCESS;
}


/**************************************************************************
 * UnmapAndCloseFile [SETUPAPI.@]
 *
 * Unmap and close a mapped file.
 *
 * PARAMS
 *     hFile    [I] Handle to the file
 *     hMapping [I] Handle to the file mapping
 *     lpBuffer [I] Pointer to the file buffer
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI UnmapAndCloseFile(HANDLE hFile, HANDLE hMapping, LPVOID lpBuffer)
{
    TRACE("%p %p %p\n",
          hFile, hMapping, lpBuffer);

    if (!UnmapViewOfFile(lpBuffer))
        return FALSE;

    if (!CloseHandle(hMapping))
        return FALSE;

    if (!CloseHandle(hFile))
        return FALSE;

    return TRUE;
}


/**************************************************************************
 * StampFileSecurity [SETUPAPI.@]
 *
 * Assign a new security descriptor to the given file.
 *
 * PARAMS
 *     lpFileName          [I] Name of the file
 *     pSecurityDescriptor [I] New security descriptor
 *
 * RETURNS
 *     Success: ERROR_SUCCESS
 *     Failure: other
 */
DWORD WINAPI StampFileSecurity(LPCWSTR lpFileName, PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    TRACE("%s %p\n", debugstr_w(lpFileName), pSecurityDescriptor);

    if (!SetFileSecurityW(lpFileName, OWNER_SECURITY_INFORMATION |
                          GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                          pSecurityDescriptor))
        return GetLastError();

    return ERROR_SUCCESS;
}


/**************************************************************************
 * TakeOwnershipOfFile [SETUPAPI.@]
 *
 * Takes the ownership of the given file.
 *
 * PARAMS
 *     lpFileName [I] Name of the file
 *
 * RETURNS
 *     Success: ERROR_SUCCESS
 *     Failure: other
 */
DWORD WINAPI TakeOwnershipOfFile(LPCWSTR lpFileName)
{
    SECURITY_DESCRIPTOR SecDesc;
    HANDLE hToken = NULL;
    PTOKEN_OWNER pOwner = NULL;
    DWORD dwError;
    DWORD dwSize;

    TRACE("%s\n", debugstr_w(lpFileName));

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return GetLastError();

    if (!GetTokenInformation(hToken, TokenOwner, NULL, 0, &dwSize))
    {
        goto fail;
    }

    pOwner = MyMalloc(dwSize);
    if (pOwner == NULL)
    {
        CloseHandle(hToken);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (!GetTokenInformation(hToken, TokenOwner, pOwner, dwSize, &dwSize))
    {
        goto fail;
    }

    if (!InitializeSecurityDescriptor(&SecDesc, SECURITY_DESCRIPTOR_REVISION))
    {
        goto fail;
    }

    if (!SetSecurityDescriptorOwner(&SecDesc, pOwner->Owner, FALSE))
    {
        goto fail;
    }

    if (!SetFileSecurityW(lpFileName, OWNER_SECURITY_INFORMATION, &SecDesc))
    {
        goto fail;
    }

    MyFree(pOwner);
    CloseHandle(hToken);

    return ERROR_SUCCESS;

fail:;
    dwError = GetLastError();

    MyFree(pOwner);

    if (hToken != NULL)
        CloseHandle(hToken);

    return dwError;
}


/**************************************************************************
 * RetreiveFileSecurity [SETUPAPI.@]
 *
 * Retrieve the security descriptor that is associated with the given file.
 *
 * PARAMS
 *     lpFileName [I] Name of the file
 *
 * RETURNS
 *     Success: ERROR_SUCCESS
 *     Failure: other
 */
DWORD WINAPI RetreiveFileSecurity(LPCWSTR lpFileName,
                                  PSECURITY_DESCRIPTOR *pSecurityDescriptor)
{
    SECURITY_DESCRIPTOR *SecDesc, *NewSecDesc;
    DWORD dwSize = 0x100;
    DWORD dwError;

    SecDesc = MyMalloc(dwSize);
    if (SecDesc == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    if (GetFileSecurityW(lpFileName, OWNER_SECURITY_INFORMATION |
                         GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                         SecDesc, dwSize, &dwSize))
    {
      *pSecurityDescriptor = SecDesc;
      return ERROR_SUCCESS;
    }

    dwError = GetLastError();
    if (dwError != ERROR_INSUFFICIENT_BUFFER)
    {
        MyFree(SecDesc);
        return dwError;
    }

    NewSecDesc = MyRealloc(SecDesc, dwSize);
    if (NewSecDesc == NULL)
    {
        MyFree(SecDesc);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    SecDesc = NewSecDesc;

    if (GetFileSecurityW(lpFileName, OWNER_SECURITY_INFORMATION |
                         GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                         SecDesc, dwSize, &dwSize))
    {
      *pSecurityDescriptor = SecDesc;
      return ERROR_SUCCESS;
    }

    dwError = GetLastError();
    MyFree(SecDesc);

    return dwError;
}


static DWORD global_flags = 0;  /* FIXME: what should be in here? */

/***********************************************************************
 *		pSetupGetGlobalFlags  (SETUPAPI.@)
 */
DWORD WINAPI pSetupGetGlobalFlags(void)
{
    FIXME( "stub\n" );
    return global_flags;
}


/***********************************************************************
 *		pSetupSetGlobalFlags  (SETUPAPI.@)
 */
void WINAPI pSetupSetGlobalFlags( DWORD flags )
{
    global_flags = flags;
}

/***********************************************************************
 *		CMP_WaitNoPendingInstallEvents  (SETUPAPI.@)
 */
DWORD WINAPI CMP_WaitNoPendingInstallEvents( DWORD dwTimeout )
{
    static BOOL warned = FALSE;

    if (!warned)
    {
        FIXME("%ld\n", dwTimeout);
        warned = TRUE;
    }
    return WAIT_OBJECT_0;
}

/***********************************************************************
 *              AssertFail  (SETUPAPI.@)
 *
 * Shows an assert fail error messagebox
 *
 * PARAMS
 *   lpFile [I]         file where assert failed
 *   uLine [I]          line number in file
 *   lpMessage [I]      assert message
 *
 */
void WINAPI AssertFail(LPCSTR lpFile, UINT uLine, LPCSTR lpMessage)
{
    FIXME("%s %u %s\n", lpFile, uLine, lpMessage);
}

/***********************************************************************
 *      InstallCatalog  (SETUPAPI.@)
 */
DWORD WINAPI InstallCatalog( LPCSTR catalog, LPCSTR basename, LPSTR fullname )
{
    FIXME("%s, %s, %p\n", debugstr_a(catalog), debugstr_a(basename), fullname);
    return 0;
}

/***********************************************************************
 *      pSetupInstallCatalog  (SETUPAPI.@)
 */
DWORD WINAPI pSetupInstallCatalog( LPCWSTR catalog, LPCWSTR basename, LPWSTR fullname )
{
    HCATADMIN admin;
    HCATINFO cat;

    TRACE ("%s, %s, %p\n", debugstr_w(catalog), debugstr_w(basename), fullname);

    if (!CryptCATAdminAcquireContext(&admin,NULL,0))
        return GetLastError();

    if (!(cat = CryptCATAdminAddCatalog( admin, (PWSTR)catalog, (PWSTR)basename, 0 )))
    {
        DWORD rc = GetLastError();
        CryptCATAdminReleaseContext(admin, 0);
        return rc;
    }
    CryptCATAdminReleaseCatalogContext(admin, cat, 0);
    CryptCATAdminReleaseContext(admin,0);

    if (fullname)
        FIXME("not returning full installed catalog path\n");

    return NO_ERROR;
}

static UINT detect_compression_type( LPCWSTR file )
{
    DWORD size;
    HANDLE handle;
    UINT type = FILE_COMPRESSION_NONE;
    static const BYTE LZ_MAGIC[] = { 0x53, 0x5a, 0x44, 0x44, 0x88, 0xf0, 0x27, 0x33 };
    static const BYTE MSZIP_MAGIC[] = { 0x4b, 0x57, 0x41, 0x4a };
    static const BYTE NTCAB_MAGIC[] = { 0x4d, 0x53, 0x43, 0x46 };
    BYTE buffer[8];

    handle = CreateFileW( file, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
    if (handle == INVALID_HANDLE_VALUE)
    {
        ERR("cannot open file %s\n", debugstr_w(file));
        return FILE_COMPRESSION_NONE;
    }
    if (!ReadFile( handle, buffer, sizeof(buffer), &size, NULL ) || size != sizeof(buffer))
    {
        CloseHandle( handle );
        return FILE_COMPRESSION_NONE;
    }
    if (!memcmp( buffer, LZ_MAGIC, sizeof(LZ_MAGIC) )) type = FILE_COMPRESSION_WINLZA;
    else if (!memcmp( buffer, MSZIP_MAGIC, sizeof(MSZIP_MAGIC) )) type = FILE_COMPRESSION_MSZIP;
    else if (!memcmp( buffer, NTCAB_MAGIC, sizeof(NTCAB_MAGIC) )) type = FILE_COMPRESSION_MSZIP; /* not a typo */

    CloseHandle( handle );
    return type;
}

static BOOL get_file_size( LPCWSTR file, DWORD *size )
{
    HANDLE handle;

    handle = CreateFileW( file, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
    if (handle == INVALID_HANDLE_VALUE)
    {
        ERR("cannot open file %s\n", debugstr_w(file));
        return FALSE;
    }
    *size = GetFileSize( handle, NULL );
    CloseHandle( handle );
    return TRUE;
}

static BOOL get_file_sizes_none( LPCWSTR source, DWORD *source_size, DWORD *target_size )
{
    DWORD size;

    if (!get_file_size( source, &size )) return FALSE;
    if (source_size) *source_size = size;
    if (target_size) *target_size = size;
    return TRUE;
}

static BOOL get_file_sizes_lz( LPCWSTR source, DWORD *source_size, DWORD *target_size )
{
    DWORD size;
    BOOL ret = TRUE;

    if (source_size)
    {
        if (!get_file_size( source, &size )) ret = FALSE;
        else *source_size = size;
    }
    if (target_size)
    {
        INT file;
        OFSTRUCT of;

        if ((file = LZOpenFileW( (LPWSTR)source, &of, OF_READ )) < 0)
        {
            ERR("cannot open source file for reading\n");
            return FALSE;
        }
        *target_size = LZSeek( file, 0, 2 );
        LZClose( file );
    }
    return ret;
}

static UINT CALLBACK file_compression_info_callback( PVOID context, UINT notification, UINT_PTR param1, UINT_PTR param2 )
{
    DWORD *size = context;
    FILE_IN_CABINET_INFO_W *info = (FILE_IN_CABINET_INFO_W *)param1;

    switch (notification)
    {
    case SPFILENOTIFY_FILEINCABINET:
    {
        *size = info->FileSize;
        return FILEOP_SKIP;
    }
    default: return NO_ERROR;
    }
}

static BOOL get_file_sizes_cab( LPCWSTR source, DWORD *source_size, DWORD *target_size )
{
    DWORD size;
    BOOL ret = TRUE;

    if (source_size)
    {
        if (!get_file_size( source, &size )) ret = FALSE;
        else *source_size = size;
    }
    if (target_size)
    {
        ret = SetupIterateCabinetW( source, 0, file_compression_info_callback, target_size );
    }
    return ret;
}

/***********************************************************************
 *      SetupGetFileCompressionInfoExA  (SETUPAPI.@)
 *
 * See SetupGetFileCompressionInfoExW.
 */
BOOL WINAPI SetupGetFileCompressionInfoExA( PCSTR source, PSTR name, DWORD len, PDWORD required,
                                            PDWORD source_size, PDWORD target_size, PUINT type )
{
    BOOL ret;
    WCHAR *nameW = NULL, *sourceW = NULL;
    DWORD nb_chars = 0;
    LPSTR nameA;

    TRACE("%s, %p, %ld, %p, %p, %p, %p\n", debugstr_a(source), name, len, required,
          source_size, target_size, type);

    if (!source || !(sourceW = MultiByteToUnicode( source, CP_ACP ))) return FALSE;

    if (name)
    {
        ret = SetupGetFileCompressionInfoExW( sourceW, NULL, 0, &nb_chars, NULL, NULL, NULL );
        if (!(nameW = malloc( nb_chars * sizeof(WCHAR) )))
        {
            MyFree( sourceW );
            return FALSE;
        }
    }
    ret = SetupGetFileCompressionInfoExW( sourceW, nameW, nb_chars, &nb_chars, source_size, target_size, type );
    if (ret)
    {
        if ((nameA = UnicodeToMultiByte( nameW, CP_ACP )))
        {
            if (name && len >= nb_chars) lstrcpyA( name, nameA );
            else
            {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                ret = FALSE;
            }
            MyFree( nameA );
        }
    }
    if (required) *required = nb_chars;
    free( nameW );
    MyFree( sourceW );

    return ret;
}

/***********************************************************************
 *      SetupGetFileCompressionInfoExW  (SETUPAPI.@)
 *
 * Get compression type and compressed/uncompressed sizes of a given file.
 *
 * PARAMS
 *  source      [I] File to examine.
 *  name        [O] Actual filename used.
 *  len         [I] Length in characters of 'name' buffer.
 *  required    [O] Number of characters written to 'name'.
 *  source_size [O] Size of compressed file.
 *  target_size [O] Size of uncompressed file.
 *  type        [O] Compression type.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI SetupGetFileCompressionInfoExW( PCWSTR source, PWSTR name, DWORD len, PDWORD required,
                                            PDWORD source_size, PDWORD target_size, PUINT type )
{
    UINT comp;
    BOOL ret = FALSE;
    DWORD source_len;

    TRACE("%s, %p, %ld, %p, %p, %p, %p\n", debugstr_w(source), name, len, required,
          source_size, target_size, type);

    if (!source) return FALSE;

    source_len = lstrlenW( source ) + 1;
    if (required) *required = source_len;
    if (name && len >= source_len)
    {
        lstrcpyW( name, source );
        ret = TRUE;
    }
    else return FALSE;

    comp = detect_compression_type( source );
    if (type) *type = comp;

    switch (comp)
    {
    case FILE_COMPRESSION_MSZIP:
    case FILE_COMPRESSION_NTCAB:  ret = get_file_sizes_cab( source, source_size, target_size ); break;
    case FILE_COMPRESSION_NONE:   ret = get_file_sizes_none( source, source_size, target_size ); break;
    case FILE_COMPRESSION_WINLZA: ret = get_file_sizes_lz( source, source_size, target_size ); break;
    default: break;
    }
    return ret;
}

/***********************************************************************
 *      SetupGetFileCompressionInfoA  (SETUPAPI.@)
 *
 * See SetupGetFileCompressionInfoW.
 */
DWORD WINAPI SetupGetFileCompressionInfoA( PCSTR source, PSTR *name, PDWORD source_size,
                                           PDWORD target_size, PUINT type )
{
    BOOL ret;
    DWORD error, required;
    LPSTR actual_name;

    TRACE("%s, %p, %p, %p, %p\n", debugstr_a(source), name, source_size, target_size, type);

    if (!source || !name || !source_size || !target_size || !type)
        return ERROR_INVALID_PARAMETER;

    ret = SetupGetFileCompressionInfoExA( source, NULL, 0, &required, NULL, NULL, NULL );
    if (!(actual_name = MyMalloc( required ))) return ERROR_NOT_ENOUGH_MEMORY;

    ret = SetupGetFileCompressionInfoExA( source, actual_name, required, &required,
                                          source_size, target_size, type );
    if (!ret)
    {
        error = GetLastError();
        MyFree( actual_name );
        return error;
    }
    *name = actual_name;
    return ERROR_SUCCESS;
}

/***********************************************************************
 *      SetupGetFileCompressionInfoW  (SETUPAPI.@)
 *
 * Get compression type and compressed/uncompressed sizes of a given file.
 *
 * PARAMS
 *  source      [I] File to examine.
 *  name        [O] Actual filename used.
 *  source_size [O] Size of compressed file.
 *  target_size [O] Size of uncompressed file.
 *  type        [O] Compression type.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: Win32 error code.
 */
DWORD WINAPI SetupGetFileCompressionInfoW( PCWSTR source, PWSTR *name, PDWORD source_size,
                                           PDWORD target_size, PUINT type )
{
    BOOL ret;
    DWORD error, required;
    LPWSTR actual_name;

    TRACE("%s, %p, %p, %p, %p\n", debugstr_w(source), name, source_size, target_size, type);

    if (!source || !name || !source_size || !target_size || !type)
        return ERROR_INVALID_PARAMETER;

    ret = SetupGetFileCompressionInfoExW( source, NULL, 0, &required, NULL, NULL, NULL );
    if (!(actual_name = MyMalloc( required * sizeof(WCHAR) )))
        return ERROR_NOT_ENOUGH_MEMORY;

    ret = SetupGetFileCompressionInfoExW( source, actual_name, required, &required,
                                          source_size, target_size, type );
    if (!ret)
    {
        error = GetLastError();
        MyFree( actual_name );
        return error;
    }
    *name = actual_name;
    return ERROR_SUCCESS;
}

static DWORD decompress_file_lz( LPCWSTR source, LPCWSTR target )
{
    DWORD ret;
    LONG error;
    INT src, dst;
    OFSTRUCT sof, dof;

    if ((src = LZOpenFileW( (LPWSTR)source, &sof, OF_READ )) < 0)
    {
        ERR("cannot open source file for reading\n");
        return ERROR_FILE_NOT_FOUND;
    }
    if ((dst = LZOpenFileW( (LPWSTR)target, &dof, OF_CREATE )) < 0)
    {
        ERR("cannot open target file for writing\n");
        LZClose( src );
        return ERROR_FILE_NOT_FOUND;
    }
    if ((error = LZCopy( src, dst )) >= 0) ret = ERROR_SUCCESS;
    else
    {
        WARN("failed to decompress file %ld\n", error);
        ret = ERROR_INVALID_DATA;
    }

    LZClose( src );
    LZClose( dst );
    return ret;
}

struct callback_context
{
    BOOL has_extracted;
    LPCWSTR target;
};

static UINT CALLBACK decompress_or_copy_callback( PVOID context, UINT notification, UINT_PTR param1, UINT_PTR param2 )
{
    struct callback_context *context_info = context;
    FILE_IN_CABINET_INFO_W *info = (FILE_IN_CABINET_INFO_W *)param1;

    switch (notification)
    {
    case SPFILENOTIFY_FILEINCABINET:
    {
        if (context_info->has_extracted)
            return FILEOP_ABORT;

        TRACE("Requesting extraction of cabinet file %s\n",
              wine_dbgstr_w(info->NameInCabinet));
        lstrcpyW( info->FullTargetName, context_info->target );
        context_info->has_extracted = TRUE;
        return FILEOP_DOIT;
    }
    default: return NO_ERROR;
    }
}

static DWORD decompress_file_cab( LPCWSTR source, LPCWSTR target )
{
    struct callback_context context = {0, target};
    BOOL ret;

    ret = SetupIterateCabinetW( source, 0, decompress_or_copy_callback, &context );

    if (ret) return ERROR_SUCCESS;
    else return GetLastError();
}

/***********************************************************************
 *      SetupDecompressOrCopyFileA  (SETUPAPI.@)
 *
 * See SetupDecompressOrCopyFileW.
 */
DWORD WINAPI SetupDecompressOrCopyFileA( PCSTR source, PCSTR target, PUINT type )
{
    DWORD ret = 0;
    WCHAR *sourceW = NULL, *targetW = NULL;

    if (source && !(sourceW = MultiByteToUnicode( source, CP_ACP ))) return FALSE;
    if (target && !(targetW = MultiByteToUnicode( target, CP_ACP )))
    {
        MyFree( sourceW );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ret = SetupDecompressOrCopyFileW( sourceW, targetW, type );

    MyFree( sourceW );
    MyFree( targetW );

    return ret;
}

/***********************************************************************
 *      SetupDecompressOrCopyFileW  (SETUPAPI.@)
 *
 * Copy a file and decompress it if needed.
 *
 * PARAMS
 *  source [I] File to copy.
 *  target [I] Filename of the copy.
 *  type   [I] Compression type.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: Win32 error code.
 */
DWORD WINAPI SetupDecompressOrCopyFileW( PCWSTR source, PCWSTR target, PUINT type )
{
    UINT comp;
    DWORD ret = ERROR_INVALID_PARAMETER;

    TRACE("(%s, %s, %p)\n", debugstr_w(source), debugstr_w(target), type);

    if (!source || !target) return ERROR_INVALID_PARAMETER;

    if (!type)
    {
        comp = detect_compression_type( source );
        TRACE("Detected compression type %u\n", comp);
    }
    else
    {
        comp = *type;
        TRACE("Using specified compression type %u\n", comp);
    }

    switch (comp)
    {
    case FILE_COMPRESSION_NONE:
        if (CopyFileW( source, target, FALSE )) ret = ERROR_SUCCESS;
        else ret = GetLastError();
        break;
    case FILE_COMPRESSION_WINLZA:
        ret = decompress_file_lz( source, target );
        break;
    case FILE_COMPRESSION_NTCAB:
    case FILE_COMPRESSION_MSZIP:
        ret = decompress_file_cab( source, target );
        break;
    default:
        WARN("unknown compression type %d\n", comp);
        break;
    }

    TRACE("%s -> %s %d\n", debugstr_w(source), debugstr_w(target), comp);
    return ret;
}

static BOOL non_interactive_mode;

/***********************************************************************
 *              SetupGetNonInteractiveMode  (SETUPAPI.@)
 */
BOOL WINAPI SetupGetNonInteractiveMode( void )
{
    FIXME("\n");
    return non_interactive_mode;
}

/***********************************************************************
 *              SetupSetNonInteractiveMode  (SETUPAPI.@)
 */
BOOL WINAPI SetupSetNonInteractiveMode( BOOL flag )
{
    BOOL ret = non_interactive_mode;

    FIXME("%d\n", flag);

    non_interactive_mode = flag;
    return ret;
}

/***********************************************************************
 *      SetupCloseLog(SETUPAPI.@)
 */
void WINAPI SetupCloseLog(void)
{
    EnterCriticalSection(&setupapi_cs);

    CloseHandle(setupact);
    setupact = INVALID_HANDLE_VALUE;

    CloseHandle(setuperr);
    setuperr = INVALID_HANDLE_VALUE;

    LeaveCriticalSection(&setupapi_cs);
}

/***********************************************************************
 *      SetupOpenLog(SETUPAPI.@)
 */
BOOL WINAPI SetupOpenLog(BOOL reserved)
{
    WCHAR path[MAX_PATH];

    EnterCriticalSection(&setupapi_cs);

    if (setupact != INVALID_HANDLE_VALUE && setuperr != INVALID_HANDLE_VALUE)
    {
        LeaveCriticalSection(&setupapi_cs);
        return TRUE;
    }

    GetWindowsDirectoryW(path, MAX_PATH);
    lstrcatW(path, L"\\setupact.log");

    setupact = CreateFileW(path, FILE_GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
                           NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (setupact == INVALID_HANDLE_VALUE)
    {
        LeaveCriticalSection(&setupapi_cs);
        return FALSE;
    }

    SetFilePointer(setupact, 0, NULL, FILE_END);

    GetWindowsDirectoryW(path, MAX_PATH);
    lstrcatW(path, L"\\setuperr.log");

    setuperr = CreateFileW(path, FILE_GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
                           NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (setuperr == INVALID_HANDLE_VALUE)
    {
        CloseHandle(setupact);
        setupact = INVALID_HANDLE_VALUE;
        LeaveCriticalSection(&setupapi_cs);
        return FALSE;
    }

    SetFilePointer(setuperr, 0, NULL, FILE_END);

    LeaveCriticalSection(&setupapi_cs);

    return TRUE;
}

/***********************************************************************
 *      SetupLogErrorA(SETUPAPI.@)
 */
BOOL WINAPI SetupLogErrorA(LPCSTR message, LogSeverity severity)
{
    static const char null[] = "(null)";
    BOOL ret;
    DWORD written;
    DWORD len;

    EnterCriticalSection(&setupapi_cs);

    if (setupact == INVALID_HANDLE_VALUE || setuperr == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_FILE_INVALID);
        ret = FALSE;
        goto done;
    }

    if (message == NULL)
        message = null;

    len = lstrlenA(message);

    ret = WriteFile(setupact, message, len, &written, NULL);
    if (!ret)
        goto done;

    if (severity >= LogSevMaximum)
    {
        ret = FALSE;
        goto done;
    }

    if (severity > LogSevInformation)
        ret = WriteFile(setuperr, message, len, &written, NULL);

done:
    LeaveCriticalSection(&setupapi_cs);
    return ret;
}

/***********************************************************************
 *      SetupLogErrorW(SETUPAPI.@)
 */
BOOL WINAPI SetupLogErrorW(LPCWSTR message, LogSeverity severity)
{
    LPSTR msg = NULL;
    DWORD len;
    BOOL ret;

    if (message)
    {
        len = WideCharToMultiByte(CP_ACP, 0, message, -1, NULL, 0, NULL, NULL);
        msg = malloc(len);
        if (msg == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        WideCharToMultiByte(CP_ACP, 0, message, -1, msg, len, NULL, NULL);
    }

    /* This is the normal way to proceed. The log files are ANSI files
     * and W is to be converted.
     */
    ret = SetupLogErrorA(msg, severity);

    free(msg);
    return ret;
}

/***********************************************************************
 *      CM_Get_Version (SETUPAPI.@)
 */
WORD WINAPI CM_Get_Version(void)
{
    TRACE("()\n");
    return 0x0400;
}
