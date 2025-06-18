/*
 * PatchAPI
 *
 * Copyright 2011 David Hedberg for CodeWeavers
 * Copyright 2019 Conor McCarthy (implementations)
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
 *
 * TODO
 *  - Special processing of 32-bit executables is not supported, so this
 *    version cannot patch 32-bit .exe and .dll files. See pa19.c for details.
 *  - Implement interleaved decoding when PATCH_OPTION_INTERLEAVE_FILES was
 *    used or the old file exceeds the lzxd window size.
 *  - APPLY_OPTION_FAIL_IF_CLOSE is ignored. Normalization of 32-bit PE files
 *    is required for checking this.
 *  - GetFilePatchSignature* and NormalizeFileForPatchSignature require a
 *    solution to the above 32-bit exe problem.
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "patchapi.h"
#include "wine/debug.h"

#include "pa19.h"

WINE_DEFAULT_DEBUG_CHANNEL(mspatcha);


static WCHAR *strdupAW(const char *src)
{
    WCHAR *dst = NULL;
    if (src)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
        if ((dst = malloc(len * sizeof(WCHAR))))
            MultiByteToWideChar(CP_ACP, 0, src, -1, dst, len);
    }
    return dst;
}

/*****************************************************
 *    TestApplyPatchToFileA (MSPATCHA.@)
 */
BOOL WINAPI TestApplyPatchToFileA(LPCSTR patch_file, LPCSTR old_file, ULONG apply_flags)
{
    BOOL ret;
    WCHAR *patch_fileW, *old_fileW = NULL;

    if (!(patch_fileW = strdupAW(patch_file))) return FALSE;
    if (old_file && !(old_fileW = strdupAW(old_file)))
    {
        free(patch_fileW);
        return FALSE;
    }
    ret = apply_patch_to_file(patch_fileW, old_fileW, NULL, apply_flags, NULL, NULL, TRUE);
    free(patch_fileW);
    free(old_fileW);
    return ret;
}

BOOL WINAPI TestApplyPatchToFileW(LPCWSTR patch_file_name, LPCWSTR old_file_name, ULONG apply_option_flags)
{
    return apply_patch_to_file(patch_file_name, old_file_name, NULL, apply_option_flags, NULL, NULL, TRUE);
}

BOOL WINAPI TestApplyPatchToFileByHandles(HANDLE patch_file_hndl, HANDLE old_file_hndl, ULONG apply_option_flags)
{
    return apply_patch_to_file_by_handles(patch_file_hndl, old_file_hndl, NULL,
        apply_option_flags, NULL, NULL, TRUE);
}

BOOL WINAPI TestApplyPatchToFileByBuffers(BYTE *patch_file_buf, ULONG patch_file_size,
    BYTE *old_file_buf, ULONG old_file_size,
    ULONG* new_file_size,
    ULONG  apply_option_flags)
{
    /* NOTE: windows preserves last error on success for this function, but no apps are known to depend on it */

    DWORD err = apply_patch_to_file_by_buffers(patch_file_buf, patch_file_size,
        old_file_buf, old_file_size,
        NULL, 0, new_file_size, NULL,
        apply_option_flags,
        NULL, NULL,
        TRUE);

    SetLastError(err);

    return err == ERROR_SUCCESS;
}

/*****************************************************
 *    ApplyPatchToFileExA (MSPATCHA.@)
 */
BOOL WINAPI ApplyPatchToFileExA(LPCSTR patch_file, LPCSTR old_file, LPCSTR new_file, ULONG apply_flags,
    PPATCH_PROGRESS_CALLBACK progress_fn, PVOID progress_ctx)
{
    BOOL ret = FALSE;
    WCHAR *patch_fileW, *new_fileW, *old_fileW = NULL;

    if (!(patch_fileW = strdupAW(patch_file))) return FALSE;

    if (old_file && !(old_fileW = strdupAW(old_file)))
        goto free_wstrs;

    if (!(new_fileW = strdupAW(new_file)))
        goto free_wstrs;

    ret = apply_patch_to_file(patch_fileW, old_fileW, new_fileW, apply_flags, progress_fn, progress_ctx, FALSE);

    free(new_fileW);
free_wstrs:
    free(patch_fileW);
    free(old_fileW);
    return ret;
}

/*****************************************************
 *    ApplyPatchToFileA (MSPATCHA.@)
 */
BOOL WINAPI ApplyPatchToFileA(LPCSTR patch_file, LPCSTR old_file, LPCSTR new_file, ULONG apply_flags)
{
    return ApplyPatchToFileExA(patch_file, old_file, new_file, apply_flags, NULL, NULL);
}

/*****************************************************
 *    ApplyPatchToFileW (MSPATCHA.@)
 */
BOOL WINAPI ApplyPatchToFileW(LPCWSTR patch_file_name, LPCWSTR old_file_name, LPCWSTR new_file_name,
    ULONG apply_option_flags)
{
    return apply_patch_to_file(patch_file_name, old_file_name, new_file_name, apply_option_flags,
        NULL, NULL, FALSE);
}

/*****************************************************
 *    ApplyPatchToFileByHandles (MSPATCHA.@)
 */
BOOL WINAPI ApplyPatchToFileByHandles(HANDLE patch_file_hndl, HANDLE old_file_hndl, HANDLE new_file_hndl,
    ULONG apply_option_flags)
{
    return apply_patch_to_file_by_handles(patch_file_hndl, old_file_hndl, new_file_hndl,
        apply_option_flags, NULL, NULL, FALSE);
}

/*****************************************************
 *    ApplyPatchToFileExW (MSPATCHA.@)
 */
BOOL WINAPI ApplyPatchToFileExW(LPCWSTR patch_file_name, LPCWSTR old_file_name, LPCWSTR new_file_name,
    ULONG apply_option_flags,
    PPATCH_PROGRESS_CALLBACK progress_fn, PVOID progress_ctx)
{
    return apply_patch_to_file(patch_file_name, old_file_name, new_file_name, apply_option_flags,
        progress_fn, progress_ctx, FALSE);
}

/*****************************************************
 *    ApplyPatchToFileByHandlesEx (MSPATCHA.@)
 */
BOOL WINAPI ApplyPatchToFileByHandlesEx(HANDLE patch_file_hndl, HANDLE old_file_hndl, HANDLE new_file_hndl,
    ULONG apply_option_flags,
    PPATCH_PROGRESS_CALLBACK progress_fn,
    PVOID progress_ctx)
{
    return apply_patch_to_file_by_handles(patch_file_hndl, old_file_hndl, new_file_hndl,
        apply_option_flags, progress_fn, progress_ctx, FALSE);
}

/*****************************************************
 *    ApplyPatchToFileByBuffers (MSPATCHA.@)
 */
BOOL WINAPI ApplyPatchToFileByBuffers(PBYTE patch_file_view, ULONG  patch_file_size,
    PBYTE  old_file_view, ULONG  old_file_size,
    PBYTE* new_file_buf, ULONG  new_file_buf_size, ULONG* new_file_size,
    FILETIME* new_file_time,
    ULONG  apply_option_flags,
    PPATCH_PROGRESS_CALLBACK progress_fn, PVOID  progress_ctx)
{
    /* NOTE: windows preserves last error on success for this function, but no apps are known to depend on it */

    DWORD err = apply_patch_to_file_by_buffers(patch_file_view, patch_file_size,
        old_file_view, old_file_size,
        new_file_buf, new_file_buf_size, new_file_size, new_file_time,
        apply_option_flags,
        progress_fn, progress_ctx,
        FALSE);

    SetLastError(err);

    return err == ERROR_SUCCESS;
}

/*****************************************************
 *    GetFilePatchSignatureA (MSPATCHA.@)
 */
BOOL WINAPI GetFilePatchSignatureA(LPCSTR filename, ULONG flags, PVOID data, ULONG ignore_range_count,
    PPATCH_IGNORE_RANGE ignore_range, ULONG retain_range_count,
    PPATCH_RETAIN_RANGE retain_range, ULONG bufsize, LPSTR buffer)
{
    FIXME("stub - %s, %lx, %p, %lu, %p, %lu, %p, %lu, %p\n", debugstr_a(filename), flags, data,
        ignore_range_count, ignore_range, retain_range_count, retain_range, bufsize, buffer);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*****************************************************
 *    GetFilePatchSignatureW (MSPATCHA.@)
 */
BOOL WINAPI GetFilePatchSignatureW(LPCWSTR filename, ULONG flags, PVOID data, ULONG ignore_range_count,
    PPATCH_IGNORE_RANGE ignore_range, ULONG retain_range_count,
    PPATCH_RETAIN_RANGE retain_range, ULONG bufsize, LPWSTR buffer)
{
    FIXME("stub - %s, %lx, %p, %lu, %p, %lu, %p, %lu, %p\n", debugstr_w(filename), flags, data,
        ignore_range_count, ignore_range, retain_range_count, retain_range, bufsize, buffer);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*****************************************************
 *    GetFilePatchSignatureByHandle (MSPATCHA.@)
 */
BOOL WINAPI GetFilePatchSignatureByHandle(HANDLE handle, ULONG flags, PVOID options, ULONG ignore_range_count,
    PPATCH_IGNORE_RANGE ignore_range, ULONG retain_range_count,
    PPATCH_RETAIN_RANGE retain_range, ULONG bufsize, LPSTR buffer)
{
    FIXME("stub - %p, %lx, %p, %lu, %p, %lu, %p, %lu, %p\n", handle, flags, options,
        ignore_range_count, ignore_range, retain_range_count, retain_range, bufsize, buffer);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*****************************************************
 *    GetFilePatchSignatureByBuffer (MSPATCHA.@)
 */
BOOL WINAPI GetFilePatchSignatureByBuffer(PBYTE file_buf, ULONG file_size, ULONG flags, PVOID options,
    ULONG ignore_range_count, PPATCH_IGNORE_RANGE ignore_range,
    ULONG retain_range_count, PPATCH_RETAIN_RANGE retain_range,
    ULONG bufsize, LPSTR buffer)
{
    FIXME("stub - %p, %lu, %lx, %p, %lu, %p, %lu, %p, %lu, %p\n", file_buf, file_size, flags, options,
        ignore_range_count, ignore_range, retain_range_count, retain_range, bufsize, buffer);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*****************************************************
 *    NormalizeFileForPatchSignature (MSPATCHA.@)
 */
INT WINAPI NormalizeFileForPatchSignature(PVOID file_buffer, ULONG file_size, ULONG flags, PATCH_OPTION_DATA *options,
    ULONG new_coff_base, ULONG new_coff_time, ULONG ignore_range_count, PPATCH_IGNORE_RANGE ignore_range,
    ULONG retain_range_count, PPATCH_RETAIN_RANGE retain_range)
{
    FIXME("stub - %p, %lu, %lx, %p, %lu, %lu, %lu, %p, %lu, %p\n", file_buffer, file_size, flags, options, new_coff_base,
        new_coff_time, ignore_range_count, ignore_range, retain_range_count, retain_range);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}
