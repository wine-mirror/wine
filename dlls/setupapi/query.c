/*
 * setupapi query functions
 *
 * Copyright 2006 James Hawkins
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "setupapi.h"
#include "advpub.h"
#include "winnls.h"
#include "wine/debug.h"
#include "setupapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

#ifdef __i386__
static const WCHAR source_disks_names_platform[] = L"SourceDisksNames.x86";
static const WCHAR source_disks_files_platform[] = L"SourceDisksFiles.x86";
#elif defined(__x86_64__)
static const WCHAR source_disks_names_platform[] = L"SourceDisksNames.amd64";
static const WCHAR source_disks_files_platform[] = L"SourceDisksFiles.amd64";
#elif defined(__arm__)
static const WCHAR source_disks_names_platform[] = L"SourceDisksNames.arm";
static const WCHAR source_disks_files_platform[] = L"SourceDisksFiles.arm";
#elif defined(__aarch64__)
static const WCHAR source_disks_names_platform[] = L"SourceDisksNames.arm64";
static const WCHAR source_disks_files_platform[] = L"SourceDisksFiles.arm64";
#else  /* FIXME: other platforms */
static const WCHAR source_disks_names_platform[] = L"SourceDisksNames";
static const WCHAR source_disks_files_platform[] = L"SourceDisksFiles";
#endif
static const WCHAR source_disks_names[] = L"SourceDisksNames";
static const WCHAR source_disks_files[] = L"SourceDisksFiles";

/* fills the PSP_INF_INFORMATION struct fill_info is TRUE
 * always returns the required size of the information
 */
static BOOL fill_inf_info(HINF inf, PSP_INF_INFORMATION buffer, DWORD size, DWORD *required)
{
    LPCWSTR filename = PARSER_get_inf_filename(inf);
    DWORD total_size = FIELD_OFFSET(SP_INF_INFORMATION, VersionData)
                        + (lstrlenW(filename) + 1) * sizeof(WCHAR);

    if (required) *required = total_size;

    /* FIXME: we need to parse the INF file to find the correct version info */
    if (buffer)
    {
        if (size < total_size)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        buffer->InfStyle = INF_STYLE_WIN4;
        buffer->InfCount = 1;
        /* put the filename in buffer->VersionData */
        lstrcpyW((LPWSTR)&buffer->VersionData[0], filename);
    }
    return TRUE;
}

static HINF search_for_inf(LPCVOID InfSpec, DWORD SearchControl)
{
    HINF hInf = INVALID_HANDLE_VALUE;
    WCHAR inf_path[MAX_PATH];

    if (SearchControl == INFINFO_REVERSE_DEFAULT_SEARCH)
    {
        GetWindowsDirectoryW(inf_path, MAX_PATH);
        lstrcatW(inf_path, L"\\system32\\");
        lstrcatW(inf_path, InfSpec);

        hInf = SetupOpenInfFileW(inf_path, NULL,
                                 INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
        if (hInf != INVALID_HANDLE_VALUE)
            return hInf;

        GetWindowsDirectoryW(inf_path, MAX_PATH);
        lstrcpyW(inf_path, L"\\inf\\");
        lstrcatW(inf_path, InfSpec);

        return SetupOpenInfFileW(inf_path, NULL,
                                 INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
    }

    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *      SetupGetInfInformationA    (SETUPAPI.@)
 *
 */
BOOL WINAPI SetupGetInfInformationA(LPCVOID InfSpec, DWORD SearchControl,
                                    PSP_INF_INFORMATION ReturnBuffer,
                                    DWORD ReturnBufferSize, PDWORD RequiredSize)
{
    LPWSTR inf = (LPWSTR)InfSpec;
    DWORD len;
    BOOL ret;

    if (InfSpec && SearchControl >= INFINFO_INF_NAME_IS_ABSOLUTE)
    {
        len = MultiByteToWideChar(CP_ACP, 0, InfSpec, -1, NULL, 0);
        inf = malloc(len * sizeof(WCHAR));
        if (!inf)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        MultiByteToWideChar(CP_ACP, 0, InfSpec, -1, inf, len);
    }

    ret = SetupGetInfInformationW(inf, SearchControl, ReturnBuffer,
                                  ReturnBufferSize, RequiredSize);

    if (SearchControl >= INFINFO_INF_NAME_IS_ABSOLUTE)
        free(inf);

    return ret;
}

/***********************************************************************
 *      SetupGetInfInformationW    (SETUPAPI.@)
 * 
 * BUGS
 *   Only handles the case when InfSpec is an INF handle.
 */
BOOL WINAPI SetupGetInfInformationW(LPCVOID InfSpec, DWORD SearchControl,
                                     PSP_INF_INFORMATION ReturnBuffer,
                                     DWORD ReturnBufferSize, PDWORD RequiredSize)
{
    HINF inf;
    BOOL ret;
    DWORD infSize;

    TRACE("(%p, %ld, %p, %ld, %p)\n", InfSpec, SearchControl, ReturnBuffer,
           ReturnBufferSize, RequiredSize);

    if (!InfSpec)
    {
        if (SearchControl == INFINFO_INF_SPEC_IS_HINF)
            SetLastError(ERROR_INVALID_HANDLE);
        else
            SetLastError(ERROR_INVALID_PARAMETER);

        return FALSE;
    }

    switch (SearchControl)
    {
        case INFINFO_INF_SPEC_IS_HINF:
            inf = (HINF)InfSpec;
            break;
        case INFINFO_INF_NAME_IS_ABSOLUTE:
        case INFINFO_DEFAULT_SEARCH:
            inf = SetupOpenInfFileW(InfSpec, NULL,
                                    INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
            break;
        case INFINFO_REVERSE_DEFAULT_SEARCH:
            inf = search_for_inf(InfSpec, SearchControl);
            break;
        case INFINFO_INF_PATH_LIST_SEARCH:
            FIXME("Unhandled search control: %ld\n", SearchControl);

            if (RequiredSize)
                *RequiredSize = 0;

            return FALSE;
        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    if (inf == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    ret = fill_inf_info(inf, ReturnBuffer, ReturnBufferSize, &infSize);
    if (!ReturnBuffer && (ReturnBufferSize >= infSize))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        ret = FALSE;
    }
    if (RequiredSize) *RequiredSize = infSize;

    if (SearchControl >= INFINFO_INF_NAME_IS_ABSOLUTE)
        SetupCloseInfFile(inf);

    return ret;
}

/***********************************************************************
 *      SetupQueryInfFileInformationA    (SETUPAPI.@)
 */
BOOL WINAPI SetupQueryInfFileInformationA(PSP_INF_INFORMATION InfInformation,
                                          UINT InfIndex, PSTR ReturnBuffer,
                                          DWORD ReturnBufferSize, PDWORD RequiredSize)
{
    LPWSTR filenameW;
    DWORD size;
    BOOL ret;

    ret = SetupQueryInfFileInformationW(InfInformation, InfIndex, NULL, 0, &size);
    if (!ret)
        return FALSE;

    filenameW = malloc(size * sizeof(WCHAR));

    ret = SetupQueryInfFileInformationW(InfInformation, InfIndex,
                                        filenameW, size, &size);
    if (!ret)
    {
        free(filenameW);
        return FALSE;
    }

    if (RequiredSize)
        *RequiredSize = size;

    if (!ReturnBuffer)
    {
        free(filenameW);
        if (ReturnBufferSize)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        return TRUE;
    }

    if (size > ReturnBufferSize)
    {
        free(filenameW);
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    WideCharToMultiByte(CP_ACP, 0, filenameW, -1, ReturnBuffer, size, NULL, NULL);
    free(filenameW);

    return ret;
}

/***********************************************************************
 *      SetupQueryInfFileInformationW    (SETUPAPI.@)
 */
BOOL WINAPI SetupQueryInfFileInformationW(PSP_INF_INFORMATION InfInformation,
                                          UINT InfIndex, PWSTR ReturnBuffer,
                                          DWORD ReturnBufferSize, PDWORD RequiredSize) 
{
    DWORD len;
    LPWSTR ptr;

    TRACE("(%p, %u, %p, %ld, %p) Stub!\n", InfInformation, InfIndex,
          ReturnBuffer, ReturnBufferSize, RequiredSize);

    if (!InfInformation)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (InfIndex != 0)
        FIXME("Appended INF files are not handled\n");

    ptr = (LPWSTR)InfInformation->VersionData;
    len = lstrlenW(ptr);

    if (RequiredSize)
        *RequiredSize = len + 1;

    if (!ReturnBuffer)
        return TRUE;

    if (ReturnBufferSize < len)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    lstrcpyW(ReturnBuffer, ptr);
    return TRUE;
}

/***********************************************************************
 *            SetupGetSourceFileLocationA   (SETUPAPI.@)
 */

BOOL WINAPI SetupGetSourceFileLocationA( HINF hinf, PINFCONTEXT context, PCSTR filename,
                                         PUINT source_id, PSTR buffer, DWORD buffer_size,
                                         PDWORD required_size )
{
    BOOL ret = FALSE;
    WCHAR *filenameW = NULL, *bufferW = NULL;
    DWORD required;
    INT size;

    TRACE("%p, %p, %s, %p, %p, 0x%08lx, %p\n", hinf, context, debugstr_a(filename), source_id,
          buffer, buffer_size, required_size);

    if (filename && *filename && !(filenameW = strdupAtoW( filename )))
        return FALSE;

    if (!SetupGetSourceFileLocationW( hinf, context, filenameW, source_id, NULL, 0, &required ))
        goto done;

    if (!(bufferW = malloc( required * sizeof(WCHAR) )))
        goto done;

    if (!SetupGetSourceFileLocationW( hinf, context, filenameW, source_id, bufferW, required, NULL ))
        goto done;

    size = WideCharToMultiByte( CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL );
    if (required_size) *required_size = size;

    if (buffer)
    {
        if (buffer_size >= size)
            WideCharToMultiByte( CP_ACP, 0, bufferW, -1, buffer, buffer_size, NULL, NULL );
        else
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            goto done;
        }
    }
    ret = TRUE;

 done:
    free( filenameW );
    free( bufferW );
    return ret;
}

/***********************************************************************
 *            SetupGetSourceFileLocationW   (SETUPAPI.@)
 */

BOOL WINAPI SetupGetSourceFileLocationW( HINF hinf, PINFCONTEXT context, PCWSTR filename,
                                         PUINT source_id, PWSTR buffer, DWORD buffer_size,
                                         PDWORD required_size )
{
    INFCONTEXT ctx;
    int id;

    TRACE("%p, %p, %s, %p, %p, 0x%08lx, %p\n", hinf, context, debugstr_w(filename), source_id,
          buffer, buffer_size, required_size);

    if (context)
    {
        WCHAR *ctx_filename;
        DWORD filename_size;

        if (!SetupGetStringFieldW( context, 1, NULL, 0, &filename_size ))
            return FALSE;
        if (!(ctx_filename = malloc( filename_size * sizeof(WCHAR) )))
            return FALSE;
        SetupGetStringFieldW( context, 1, ctx_filename, filename_size, NULL );

        if (!SetupFindFirstLineW( hinf, source_disks_files_platform, ctx_filename, &ctx ) &&
            !SetupFindFirstLineW( hinf, source_disks_files, ctx_filename, &ctx ))
        {
            free( ctx_filename );
            return FALSE;
        }

        free( ctx_filename );
    }
    else
    {
        if (!SetupFindFirstLineW( hinf, source_disks_files_platform, filename, &ctx ) &&
            !SetupFindFirstLineW( hinf, source_disks_files, filename, &ctx ))
        {
            return FALSE;
        }
    }

    if (!SetupGetIntField( &ctx, 1, &id ))
        return FALSE;
    *source_id = id;

    if (SetupGetStringFieldW( &ctx, 2, buffer, buffer_size, required_size ))
        return TRUE;

    if (required_size) *required_size = 1;
    if (buffer)
    {
        if (buffer_size >= 1) buffer[0] = 0;
        else
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
    }
    return TRUE;
}

/***********************************************************************
 *            SetupGetSourceInfoA  (SETUPAPI.@)
 */

BOOL WINAPI SetupGetSourceInfoA( HINF hinf, UINT source_id, UINT info,
                                 PSTR buffer, DWORD buffer_size, LPDWORD required_size )
{
    BOOL ret = FALSE;
    WCHAR *bufferW = NULL;
    DWORD required;
    INT size;

    TRACE("%p, %d, %d, %p, %ld, %p\n", hinf, source_id, info, buffer, buffer_size,
          required_size);

    if (!SetupGetSourceInfoW( hinf, source_id, info, NULL, 0, &required ))
        return FALSE;

    if (!(bufferW = malloc( required * sizeof(WCHAR) )))
        return FALSE;

    if (!SetupGetSourceInfoW( hinf, source_id, info, bufferW, required, NULL ))
        goto done;

    size = WideCharToMultiByte( CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL );
    if (required_size) *required_size = size;

    if (buffer)
    {
        if (buffer_size >= size)
            WideCharToMultiByte( CP_ACP, 0, bufferW, -1, buffer, buffer_size, NULL, NULL );
        else
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            goto done;
        }
    }
    ret = TRUE;

 done:
    free( bufferW );
    return ret;
}

/***********************************************************************
 *            SetupGetSourceInfoW  (SETUPAPI.@)
 */

BOOL WINAPI SetupGetSourceInfoW( HINF hinf, UINT source_id, UINT info,
                                 PWSTR buffer, DWORD buffer_size, LPDWORD required_size )
{
    INFCONTEXT ctx;
    WCHAR source_id_str[11];
    DWORD index;

    TRACE("%p, %d, %d, %p, %ld, %p\n", hinf, source_id, info, buffer, buffer_size,
          required_size);

    swprintf( source_id_str, ARRAY_SIZE(source_id_str), L"%d", source_id );

    if (!SetupFindFirstLineW( hinf, source_disks_names_platform, source_id_str, &ctx ) &&
        !SetupFindFirstLineW( hinf, source_disks_names, source_id_str, &ctx ))
        return FALSE;

    switch (info)
    {
    case SRCINFO_PATH:          index = 4; break;
    case SRCINFO_TAGFILE:       index = 2; break;
    case SRCINFO_DESCRIPTION:   index = 1; break;
    default:
        WARN("unknown info level: %d\n", info);
        return FALSE;
    }

    if (SetupGetStringFieldW( &ctx, index, buffer, buffer_size, required_size ))
        return TRUE;

    if (required_size) *required_size = 1;
    if (buffer)
    {
        if (buffer_size >= 1) buffer[0] = 0;
        else
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
    }
    return TRUE;
}

/***********************************************************************
 *            SetupGetTargetPathA   (SETUPAPI.@)
 */

BOOL WINAPI SetupGetTargetPathA( HINF hinf, PINFCONTEXT context, PCSTR section, PSTR buffer,
                                 DWORD buffer_size, PDWORD required_size )
{
    BOOL ret = FALSE;
    WCHAR *sectionW = NULL, *bufferW = NULL;
    DWORD required;
    INT size;

    TRACE("%p, %p, %s, %p, 0x%08lx, %p\n", hinf, context, debugstr_a(section), buffer,
          buffer_size, required_size);

    if (section && !(sectionW = strdupAtoW( section )))
        return FALSE;

    if (!SetupGetTargetPathW( hinf, context, sectionW, NULL, 0, &required ))
        goto done;

    if (!(bufferW = malloc( required * sizeof(WCHAR) )))
        goto done;

    if (!SetupGetTargetPathW( hinf, context, sectionW, bufferW, required, NULL ))
        goto done;

    size = WideCharToMultiByte( CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL );
    if (required_size) *required_size = size;

    if (buffer)
    {
        if (buffer_size >= size)
            WideCharToMultiByte( CP_ACP, 0, bufferW, -1, buffer, buffer_size, NULL, NULL );
        else
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            goto done;
        }
    }
    ret = TRUE;

 done:
    free( sectionW );
    free( bufferW );
    return ret;
}

/***********************************************************************
 *            SetupGetTargetPathW   (SETUPAPI.@)
 */

BOOL WINAPI SetupGetTargetPathW( HINF hinf, PINFCONTEXT context, PCWSTR section, PWSTR buffer,
                                 DWORD buffer_size, PDWORD required_size )
{
    INFCONTEXT ctx;
    WCHAR *dir, systemdir[MAX_PATH];
    unsigned int size;
    BOOL ret = FALSE;

    TRACE("%p, %p, %s, %p, 0x%08lx, %p\n", hinf, context, debugstr_w(section), buffer,
          buffer_size, required_size);

    if (context) ret = SetupFindFirstLineW( hinf, L"DestinationDirs", NULL, context );
    else if (section)
    {
        if (!(ret = SetupFindFirstLineW( hinf, L"DestinationDirs", section, &ctx )))
            ret = SetupFindFirstLineW( hinf, L"DestinationDirs", L"DefaultDestDir", &ctx );
    }
    if (!ret || !(dir = PARSER_get_dest_dir( context ? context : &ctx )))
    {
        GetSystemDirectoryW( systemdir, MAX_PATH );
        dir = systemdir;
    }
    size = lstrlenW( dir ) + 1;
    if (required_size) *required_size = size;

    if (buffer)
    {
        if (buffer_size >= size)
            lstrcpyW( buffer, dir );
        else
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            if (dir != systemdir) free( dir );
            return FALSE;
        }
    }
    if (dir != systemdir) free( dir );
    return TRUE;
}

/***********************************************************************
 *            SetupQueryInfOriginalFileInformationA   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueryInfOriginalFileInformationA(
    PSP_INF_INFORMATION InfInformation, UINT InfIndex,
    PSP_ALTPLATFORM_INFO AlternativePlatformInfo,
    PSP_ORIGINAL_FILE_INFO_A OriginalFileInfo)
{
    BOOL ret;
    SP_ORIGINAL_FILE_INFO_W OriginalFileInfoW;

    TRACE("(%p, %d, %p, %p)\n", InfInformation, InfIndex,
        AlternativePlatformInfo, OriginalFileInfo);

    if (OriginalFileInfo->cbSize != sizeof(*OriginalFileInfo))
    {
        WARN("incorrect OriginalFileInfo->cbSize of %ld\n", OriginalFileInfo->cbSize);
        SetLastError( ERROR_INVALID_USER_BUFFER );
        return FALSE;
    }

    OriginalFileInfoW.cbSize = sizeof(OriginalFileInfoW);
    ret = SetupQueryInfOriginalFileInformationW(InfInformation, InfIndex,
        AlternativePlatformInfo, &OriginalFileInfoW);
    if (ret)
    {
        WideCharToMultiByte(CP_ACP, 0, OriginalFileInfoW.OriginalInfName, -1,
            OriginalFileInfo->OriginalInfName, MAX_PATH, NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, OriginalFileInfoW.OriginalCatalogName, -1,
            OriginalFileInfo->OriginalCatalogName, MAX_PATH, NULL, NULL);
    }

    return ret;
}

/***********************************************************************
 *            SetupQueryInfOriginalFileInformationW   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueryInfOriginalFileInformationW(
    PSP_INF_INFORMATION InfInformation, UINT InfIndex,
    PSP_ALTPLATFORM_INFO AlternativePlatformInfo,
    PSP_ORIGINAL_FILE_INFO_W OriginalFileInfo)
{
    LPCWSTR inf_name;
    LPCWSTR inf_path;
    HINF hinf;

    FIXME("(%p, %d, %p, %p): semi-stub\n", InfInformation, InfIndex,
        AlternativePlatformInfo, OriginalFileInfo);

    if (OriginalFileInfo->cbSize != sizeof(*OriginalFileInfo))
    {
        WARN("incorrect OriginalFileInfo->cbSize of %ld\n", OriginalFileInfo->cbSize);
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    inf_path = (LPWSTR)InfInformation->VersionData;

    /* FIXME: we should get OriginalCatalogName from CatalogFile line in
     * the original inf file and cache it, but that would require building a
     * .pnf file. */
    hinf = SetupOpenInfFileW(inf_path, NULL, INF_STYLE_WIN4, NULL);
    if (hinf == INVALID_HANDLE_VALUE) return FALSE;

    if (!SetupGetLineTextW(NULL, hinf, L"Version", L"CatalogFile",
                           OriginalFileInfo->OriginalCatalogName,
                           ARRAY_SIZE(OriginalFileInfo->OriginalCatalogName), NULL))
    {
        OriginalFileInfo->OriginalCatalogName[0] = '\0';
    }
    SetupCloseInfFile(hinf);

    /* FIXME: not quite correct as we just return the same file name as
     * destination (copied) inf file, not the source (original) inf file.
     * to fix it properly would require building a .pnf file */
    /* file name is stored in VersionData field of InfInformation */
    inf_name = wcsrchr(inf_path, '\\');
    if (inf_name) inf_name++;
    else inf_name = inf_path;

    lstrcpyW(OriginalFileInfo->OriginalInfName, inf_name);

    return TRUE;
}

/***********************************************************************
 *      SetupGetInfDriverStoreLocationW (SETUPAPI.@)
 */
BOOL WINAPI SetupGetInfDriverStoreLocationW(
    PCWSTR FileName, PSP_ALTPLATFORM_INFO AlternativePlatformInfo,
    PCWSTR LocaleName, PWSTR ReturnBuffer, DWORD ReturnBufferSize,
    PDWORD RequiredSize)
{
    FIXME("stub: %s %p %s %p %lu %p\n", debugstr_w(FileName), AlternativePlatformInfo, debugstr_w(LocaleName), ReturnBuffer, ReturnBufferSize, RequiredSize);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetupQueryInfVersionInformationA(SP_INF_INFORMATION *info, UINT index, const char *key, char *buff,
    DWORD size, DWORD *req_size)
{
    FIXME("info %p, index %d, key %s, buff %p, size %ld, req_size %p stub!\n", info, index, debugstr_a(key), buff,
        size, req_size);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetupQueryInfVersionInformationW(SP_INF_INFORMATION *info, UINT index, const WCHAR *key, WCHAR *buff,
    DWORD size, DWORD *req_size)
{
    FIXME("info %p, index %d, key %s, buff %p, size %ld, req_size %p stub!\n", info, index, debugstr_w(key), buff,
        size, req_size);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
