/*
 * Copyright 2018 Nikolay Sivov
 * Copyright 2018 Zhiyi Zhang
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
#include "pathcch.h"
#include "strsafe.h"
#include "shlwapi.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(path);

static char *char_next(const char *ptr)
{
    if (!*ptr) return (LPSTR)ptr;
    if (IsDBCSLeadByte( ptr[0] ) && ptr[1]) return (LPSTR)(ptr + 2);
    return (LPSTR)(ptr + 1);
}

static char * char_prev(const char *start, const char *ptr)
{
    while (*start && (start < ptr))
    {
        const char *next = char_next(start);
        if (next >= ptr) break;
        start = next;
    }
    return (char *)start;
}

static SIZE_T strnlenW(const WCHAR *string, SIZE_T maxlen)
{
    SIZE_T i;

    for (i = 0; i < maxlen; i++)
        if (!string[i]) break;
    return i;
}

static BOOL is_prefixed_unc(const WCHAR *string)
{
    static const WCHAR prefixed_unc[] = {'\\', '\\', '?', '\\', 'U', 'N', 'C', '\\'};
    return !strncmpiW(string, prefixed_unc, ARRAY_SIZE(prefixed_unc));
}

static BOOL is_prefixed_disk(const WCHAR *string)
{
    static const WCHAR prefix[] = {'\\', '\\', '?', '\\'};
    return !strncmpW(string, prefix, ARRAY_SIZE(prefix)) && isalphaW(string[4]) && string[5] == ':';
}

static BOOL is_prefixed_volume(const WCHAR *string)
{
    static const WCHAR prefixed_volume[] = {'\\', '\\', '?', '\\', 'V', 'o', 'l', 'u', 'm', 'e'};
    const WCHAR *guid;
    INT i = 0;

    if (strncmpiW(string, prefixed_volume, ARRAY_SIZE(prefixed_volume))) return FALSE;

    guid = string + ARRAY_SIZE(prefixed_volume);

    while (i <= 37)
    {
        switch (i)
        {
        case 0:
            if (guid[i] != '{') return FALSE;
            break;
        case 9:
        case 14:
        case 19:
        case 24:
            if (guid[i] != '-') return FALSE;
            break;
        case 37:
            if (guid[i] != '}') return FALSE;
            break;
        default:
            if (!isalnumW(guid[i])) return FALSE;
            break;
        }
        i++;
    }

    return TRUE;
}

/* Get the next character beyond end of the segment.
   Return TRUE if the last segment ends with a backslash */
static BOOL get_next_segment(const WCHAR *next, const WCHAR **next_segment)
{
    while (*next && *next != '\\') next++;
    if (*next == '\\')
    {
        *next_segment = next + 1;
        return TRUE;
    }
    else
    {
        *next_segment = next;
        return FALSE;
    }
}

/* Find the last character of the root in a path, if there is one, without any segments */
static const WCHAR *get_root_end(const WCHAR *path)
{
    /* Find path root */
    if (is_prefixed_volume(path))
        return path[48] == '\\' ? path + 48 : path + 47;
    else if (is_prefixed_unc(path))
        return path + 7;
    else if (is_prefixed_disk(path))
        return path[6] == '\\' ? path + 6 : path + 5;
    /* \\ */
    else if (path[0] == '\\' && path[1] == '\\')
        return path + 1;
    /* \ */
    else if (path[0] == '\\')
        return path;
    /* X:\ */
    else if (isalphaW(path[0]) && path[1] == ':')
        return path[2] == '\\' ? path + 2 : path + 1;
    else
        return NULL;
}

HRESULT WINAPI PathAllocCanonicalize(const WCHAR *path_in, DWORD flags, WCHAR **path_out)
{
    WCHAR *buffer, *dst;
    const WCHAR *src;
    const WCHAR *root_end;
    SIZE_T buffer_size, length;

    TRACE("%s %#x %p\n", debugstr_w(path_in), flags, path_out);

    if (!path_in || !path_out
        || ((flags & PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS) && (flags & PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS))
        || (flags & (PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS | PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS)
            && !(flags & PATHCCH_ALLOW_LONG_PATHS))
        || ((flags & PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH) && (flags & PATHCCH_ALLOW_LONG_PATHS)))
    {
        if (path_out) *path_out = NULL;
        return E_INVALIDARG;
    }

    length = strlenW(path_in);
    if ((length + 1 > MAX_PATH && !(flags & (PATHCCH_ALLOW_LONG_PATHS | PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH)))
        || (length + 1 > PATHCCH_MAX_CCH))
    {
        *path_out = NULL;
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    }

    /* PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH implies PATHCCH_DO_NOT_NORMALIZE_SEGMENTS */
    if (flags & PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH) flags |= PATHCCH_DO_NOT_NORMALIZE_SEGMENTS;

    /* path length + possible \\?\ addition + possible \ addition + NUL */
    buffer_size = (length + 6) * sizeof(WCHAR);
    buffer = LocalAlloc(LMEM_ZEROINIT, buffer_size);
    if (!buffer)
    {
        *path_out = NULL;
        return E_OUTOFMEMORY;
    }

    src = path_in;
    dst = buffer;

    root_end = get_root_end(path_in);
    if (root_end) root_end = buffer + (root_end - path_in);

    /* Copy path root */
    if (root_end)
    {
        memcpy(dst, src, (root_end - buffer + 1) * sizeof(WCHAR));
        src += root_end - buffer + 1;
        if(PathCchStripPrefix(dst, length + 6) == S_OK)
        {
            /* Fill in \ in X:\ if the \ is missing */
            if(isalphaW(dst[0]) && dst[1] == ':' && dst[2]!= '\\')
            {
                dst[2] = '\\';
                dst[3] = 0;
            }
            dst = buffer + strlenW(buffer);
            root_end = dst;
        }
        else
            dst += root_end - buffer + 1;
    }

    while (*src)
    {
        if (src[0] == '.')
        {
            if (src[1] == '.')
            {
                /* Keep one . after * */
                if (dst > buffer && dst[-1] == '*')
                {
                    *dst++ = *src++;
                    continue;
                }

                /* Keep the . if one of the following is true:
                 * 1. PATHCCH_DO_NOT_NORMALIZE_SEGMENTS
                 * 2. in form of a..b
                 */
                if (dst > buffer
                    && (((flags & PATHCCH_DO_NOT_NORMALIZE_SEGMENTS) && dst[-1] != '\\')
                        || (dst[-1] != '\\' && src[2] != '\\' && src[2])))
                {
                    *dst++ = *src++;
                    *dst++ = *src++;
                    continue;
                }

                /* Remove the \ before .. if the \ is not part of root */
                if (dst > buffer && dst[-1] == '\\' && (!root_end || dst - 1 > root_end))
                {
                    *--dst = '\0';
                    /* Remove characters until a \ is encountered */
                    while (dst > buffer)
                    {
                        if (dst[-1] == '\\')
                        {
                            *--dst = 0;
                            break;
                        }
                        else
                            *--dst = 0;
                    }
                }
                /* Remove the extra \ after .. if the \ before .. wasn't deleted */
                else if (src[2] == '\\')
                    src++;

                src += 2;
            }
            else
            {
                /* Keep the . if one of the following is true:
                 * 1. PATHCCH_DO_NOT_NORMALIZE_SEGMENTS
                 * 2. in form of a.b, which is used in domain names
                 * 3. *.
                 */
                if (dst > buffer
                    && ((flags & PATHCCH_DO_NOT_NORMALIZE_SEGMENTS && dst[-1] != '\\')
                        || (dst[-1] != '\\' && src[1] != '\\' && src[1]) || (dst[-1] == '*')))
                {
                    *dst++ = *src++;
                    continue;
                }

                /* Remove the \ before . if the \ is not part of root */
                if (dst > buffer && dst[-1] == '\\' && (!root_end || dst - 1 > root_end)) dst--;
                /* Remove the extra \ after . if the \ before . wasn't deleted */
                else if (src[1] == '\\')
                    src++;

                src++;
            }

            /* If X:\ is not complete, then complete it */
            if (isalphaW(buffer[0]) && buffer[1] == ':' && buffer[2] != '\\')
            {
                root_end = buffer + 2;
                dst = buffer + 3;
                buffer[2] = '\\';
                /* If next character is \, use the \ to fill in */
                if (src[0] == '\\') src++;
            }
        }
        /* Copy over */
        else
            *dst++ = *src++;
    }
    /* End the path */
    *dst = 0;

    /* If result path is empty, fill in \ */
    if (!*buffer)
    {
        buffer[0] = '\\';
        buffer[1] = 0;
    }

    /* Extend the path if needed */
    length = strlenW(buffer);
    if (((length + 1 > MAX_PATH && isalphaW(buffer[0]) && buffer[1] == ':')
         || (isalphaW(buffer[0]) && buffer[1] == ':' && flags & PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH))
        && !(flags & PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS))
    {
        memmove(buffer + 4, buffer, (length + 1) * sizeof(WCHAR));
        buffer[0] = '\\';
        buffer[1] = '\\';
        buffer[2] = '?';
        buffer[3] = '\\';
    }

    /* Add a trailing backslash to the path if needed */
    if (flags & PATHCCH_ENSURE_TRAILING_SLASH)
        PathCchAddBackslash(buffer, buffer_size);

    *path_out = buffer;
    return S_OK;
}

HRESULT WINAPI PathAllocCombine(const WCHAR *path1, const WCHAR *path2, DWORD flags, WCHAR **out)
{
    SIZE_T combined_length, length2;
    WCHAR *combined_path;
    BOOL from_path2 = FALSE;
    HRESULT hr;

    TRACE("%s %s %#x %p\n", wine_dbgstr_w(path1), wine_dbgstr_w(path2), flags, out);

    if ((!path1 && !path2) || !out)
    {
        if (out) *out = NULL;
        return E_INVALIDARG;
    }

    if (!path1 || !path2) return PathAllocCanonicalize(path1 ? path1 : path2, flags, out);

    /* If path2 is fully qualified, use path2 only */
    if ((isalphaW(path2[0]) && path2[1] == ':') || (path2[0] == '\\' && path2[1] == '\\'))
    {
        path1 = path2;
        path2 = NULL;
        from_path2 = TRUE;
    }

    length2 = path2 ? strlenW(path2) : 0;
    /* path1 length + path2 length + possible backslash + NULL */
    combined_length = strlenW(path1) + length2 + 2;

    combined_path = HeapAlloc(GetProcessHeap(), 0, combined_length * sizeof(WCHAR));
    if (!combined_path)
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    lstrcpyW(combined_path, path1);
    PathCchStripPrefix(combined_path, combined_length);
    if (from_path2) PathCchAddBackslashEx(combined_path, combined_length, NULL, NULL);

    if (path2 && path2[0])
    {
        if (path2[0] == '\\' && path2[1] != '\\')
        {
            PathCchStripToRoot(combined_path, combined_length);
            path2++;
        }

        PathCchAddBackslashEx(combined_path, combined_length, NULL, NULL);
        lstrcatW(combined_path, path2);
    }

    hr = PathAllocCanonicalize(combined_path, flags, out);
    HeapFree(GetProcessHeap(), 0, combined_path);
    return hr;
}

HRESULT WINAPI PathCchAddBackslash(WCHAR *path, SIZE_T size)
{
    return PathCchAddBackslashEx(path, size, NULL, NULL);
}

HRESULT WINAPI PathCchAddBackslashEx(WCHAR *path, SIZE_T size, WCHAR **endptr, SIZE_T *remaining)
{
    BOOL needs_termination;
    SIZE_T length;

    TRACE("%s, %lu, %p, %p\n", debugstr_w(path), size, endptr, remaining);

    length = strlenW(path);
    needs_termination = size && length && path[length - 1] != '\\';

    if (length >= (needs_termination ? size - 1 : size))
    {
        if (endptr) *endptr = NULL;
        if (remaining) *remaining = 0;
        return STRSAFE_E_INSUFFICIENT_BUFFER;
    }

    if (!needs_termination)
    {
        if (endptr) *endptr = path + length;
        if (remaining) *remaining = size - length;
        return S_FALSE;
    }

    path[length++] = '\\';
    path[length] = 0;

    if (endptr) *endptr = path + length;
    if (remaining) *remaining = size - length;

    return S_OK;
}

HRESULT WINAPI PathCchAddExtension(WCHAR *path, SIZE_T size, const WCHAR *extension)
{
    const WCHAR *existing_extension, *next;
    SIZE_T path_length, extension_length, dot_length;
    BOOL has_dot;
    HRESULT hr;

    TRACE("%s %lu %s\n", wine_dbgstr_w(path), size, wine_dbgstr_w(extension));

    if (!path || !size || size > PATHCCH_MAX_CCH || !extension) return E_INVALIDARG;

    next = extension;
    while (*next)
    {
        if ((*next == '.' && next > extension) || *next == ' ' || *next == '\\') return E_INVALIDARG;
        next++;
    }

    has_dot = extension[0] == '.';

    hr = PathCchFindExtension(path, size, &existing_extension);
    if (FAILED(hr)) return hr;
    if (*existing_extension) return S_FALSE;

    path_length = strnlenW(path, size);
    dot_length = has_dot ? 0 : 1;
    extension_length = strlenW(extension);

    if (path_length + dot_length + extension_length + 1 > size) return STRSAFE_E_INSUFFICIENT_BUFFER;

    /* If extension is empty or only dot, return S_OK with path unchanged */
    if (!extension[0] || (extension[0] == '.' && !extension[1])) return S_OK;

    if (!has_dot)
    {
        path[path_length] = '.';
        path_length++;
    }

    strcpyW(path + path_length, extension);
    return S_OK;
}

HRESULT WINAPI PathCchAppend(WCHAR *path1, SIZE_T size, const WCHAR *path2)
{
    TRACE("%s %lu %s\n", wine_dbgstr_w(path1), size, wine_dbgstr_w(path2));

    return PathCchAppendEx(path1, size, path2, PATHCCH_NONE);
}

HRESULT WINAPI PathCchAppendEx(WCHAR *path1, SIZE_T size, const WCHAR *path2, DWORD flags)
{
    HRESULT hr;
    WCHAR *result;

    TRACE("%s %lu %s %#x\n", wine_dbgstr_w(path1), size, wine_dbgstr_w(path2), flags);

    if (!path1 || !size) return E_INVALIDARG;

    /* Create a temporary buffer for result because we need to keep path1 unchanged if error occurs.
     * And PathCchCombineEx writes empty result if there is error so we can't just use path1 as output
     * buffer for PathCchCombineEx */
    result = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
    if (!result) return E_OUTOFMEMORY;

    /* Avoid the single backslash behavior with PathCchCombineEx when appending */
    if (path2 && path2[0] == '\\' && path2[1] != '\\') path2++;

    hr = PathCchCombineEx(result, size, path1, path2, flags);
    if (SUCCEEDED(hr)) memcpy(path1, result, size * sizeof(WCHAR));

    HeapFree(GetProcessHeap(), 0, result);
    return hr;
}

HRESULT WINAPI PathCchCanonicalize(WCHAR *out, SIZE_T size, const WCHAR *in)
{
    TRACE("%p %lu %s\n", out, size, wine_dbgstr_w(in));

    /* Not X:\ and path > MAX_PATH - 4, return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE) */
    if (strlenW(in) > MAX_PATH - 4 && !(isalphaW(in[0]) && in[1] == ':' && in[2] == '\\'))
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);

    return PathCchCanonicalizeEx(out, size, in, PATHCCH_NONE);
}

HRESULT WINAPI PathCchCanonicalizeEx(WCHAR *out, SIZE_T size, const WCHAR *in, DWORD flags)
{
    WCHAR *buffer;
    SIZE_T length;
    HRESULT hr;

    TRACE("%p %lu %s %#x\n", out, size, wine_dbgstr_w(in), flags);

    if (!size) return E_INVALIDARG;

    hr = PathAllocCanonicalize(in, flags, &buffer);
    if (FAILED(hr)) return hr;

    length = strlenW(buffer);
    if (size < length + 1)
    {
        /* No root and path > MAX_PATH - 4, return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE) */
        if (length > MAX_PATH - 4 && !(in[0] == '\\' || (isalphaW(in[0]) && in[1] == ':' && in[2] == '\\')))
            hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
        else
            hr = STRSAFE_E_INSUFFICIENT_BUFFER;
    }

    if (SUCCEEDED(hr))
    {
        memcpy(out, buffer, (length + 1) * sizeof(WCHAR));

        /* Fill a backslash at the end of X: */
        if (isalphaW(out[0]) && out[1] == ':' && !out[2] && size > 3)
        {
            out[2] = '\\';
            out[3] = 0;
        }
    }

    LocalFree(buffer);
    return hr;
}

HRESULT WINAPI PathCchCombine(WCHAR *out, SIZE_T size, const WCHAR *path1, const WCHAR *path2)
{
    TRACE("%p %s %s\n", out, wine_dbgstr_w(path1), wine_dbgstr_w(path2));

    return PathCchCombineEx(out, size, path1, path2, PATHCCH_NONE);
}

HRESULT WINAPI PathCchCombineEx(WCHAR *out, SIZE_T size, const WCHAR *path1, const WCHAR *path2, DWORD flags)
{
    HRESULT hr;
    WCHAR *buffer;
    SIZE_T length;

    TRACE("%p %s %s %#x\n", out, wine_dbgstr_w(path1), wine_dbgstr_w(path2), flags);

    if (!out || !size || size > PATHCCH_MAX_CCH) return E_INVALIDARG;

    hr = PathAllocCombine(path1, path2, flags, &buffer);
    if (FAILED(hr))
    {
        out[0] = 0;
        return hr;
    }

    length = strlenW(buffer);
    if (length + 1 > size)
    {
        out[0] = 0;
        LocalFree(buffer);
        return STRSAFE_E_INSUFFICIENT_BUFFER;
    }
    else
    {
        memcpy(out, buffer, (length + 1) * sizeof(WCHAR));
        LocalFree(buffer);
        return S_OK;
    }
}

HRESULT WINAPI PathCchFindExtension(const WCHAR *path, SIZE_T size, const WCHAR **extension)
{
    const WCHAR *lastpoint = NULL;
    SIZE_T counter = 0;

    TRACE("%s %lu %p\n", wine_dbgstr_w(path), size, extension);

    if (!path || !size || size > PATHCCH_MAX_CCH)
    {
        *extension = NULL;
        return E_INVALIDARG;
    }

    while (*path)
    {
        if (*path == '\\' || *path == ' ')
            lastpoint = NULL;
        else if (*path == '.')
            lastpoint = path;

        path++;
        counter++;
        if (counter == size || counter == PATHCCH_MAX_CCH)
        {
            *extension = NULL;
            return E_INVALIDARG;
        }
    }

    *extension = lastpoint ? lastpoint : path;
    return S_OK;
}

BOOL WINAPI PathCchIsRoot(const WCHAR *path)
{
    const WCHAR *root_end;
    const WCHAR *next;
    BOOL is_unc;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path || !*path) return FALSE;

    root_end = get_root_end(path);
    if (!root_end) return FALSE;

    if ((is_unc = is_prefixed_unc(path)) || (path[0] == '\\' && path[1] == '\\' && path[2] != '?'))
    {
        next = root_end + 1;
        /* No extra segments */
        if ((is_unc && !*next) || (!is_unc && !*next)) return TRUE;

        /* Has first segment with an ending backslash but no remaining characters */
        if (get_next_segment(next, &next) && !*next) return FALSE;
        /* Has first segment with no ending backslash */
        else if (!*next)
            return TRUE;
        /* Has first segment with an ending backslash and has remaining characters*/
        else
        {
            next++;
            /* Second segment must have no backslash and no remaining characters */
            return !get_next_segment(next, &next) && !*next;
        }
    }
    else if (*root_end == '\\' && !root_end[1])
        return TRUE;
    else
        return FALSE;
}

HRESULT WINAPI PathCchRemoveBackslash(WCHAR *path, SIZE_T path_size)
{
    WCHAR *path_end;
    SIZE_T free_size;

    TRACE("%s %lu\n", debugstr_w(path), path_size);

    return PathCchRemoveBackslashEx(path, path_size, &path_end, &free_size);
}

HRESULT WINAPI PathCchRemoveBackslashEx(WCHAR *path, SIZE_T path_size, WCHAR **path_end, SIZE_T *free_size)
{
    const WCHAR *root_end;
    SIZE_T path_length;

    TRACE("%s %lu %p %p\n", debugstr_w(path), path_size, path_end, free_size);

    if (!path_size || !path_end || !free_size)
    {
        if (path_end) *path_end = NULL;
        if (free_size) *free_size = 0;
        return E_INVALIDARG;
    }

    path_length = strnlenW(path, path_size);
    if (path_length == path_size && !path[path_length]) return E_INVALIDARG;

    root_end = get_root_end(path);
    if (path_length > 0 && path[path_length - 1] == '\\')
    {
        *path_end = path + path_length - 1;
        *free_size = path_size - path_length + 1;
        /* If the last character is beyond end of root */
        if (!root_end || path + path_length - 1 > root_end)
        {
            path[path_length - 1] = 0;
            return S_OK;
        }
        else
            return S_FALSE;
    }
    else
    {
        *path_end = path + path_length;
        *free_size = path_size - path_length;
        return S_FALSE;
    }
}

HRESULT WINAPI PathCchRemoveExtension(WCHAR *path, SIZE_T size)
{
    const WCHAR *extension;
    WCHAR *next;
    HRESULT hr;

    TRACE("%s %lu\n", wine_dbgstr_w(path), size);

    if (!path || !size || size > PATHCCH_MAX_CCH) return E_INVALIDARG;

    hr = PathCchFindExtension(path, size, &extension);
    if (FAILED(hr)) return hr;

    next = path + (extension - path);
    while (next - path < size && *next) *next++ = 0;

    return next == extension ? S_FALSE : S_OK;
}

HRESULT WINAPI PathCchRemoveFileSpec(WCHAR *path, SIZE_T size)
{
    const WCHAR *root_end = NULL;
    SIZE_T length;
    WCHAR *last;

    TRACE("%s %lu\n", wine_dbgstr_w(path), size);

    if (!path || !size || size > PATHCCH_MAX_CCH) return E_INVALIDARG;

    if (PathCchIsRoot(path)) return S_FALSE;

    PathCchSkipRoot(path, &root_end);

    /* The backslash at the end of UNC and \\* are not considered part of root in this case */
    if (root_end && root_end > path && root_end[-1] == '\\'
        && (is_prefixed_unc(path) || (path[0] == '\\' && path[1] == '\\' && path[2] != '?')))
        root_end--;

    length = strlenW(path);
    last = path + length - 1;
    while (last >= path && (!root_end || last >= root_end))
    {
        if (last - path >= size) return E_INVALIDARG;

        if (*last == '\\')
        {
            *last-- = 0;
            break;
        }

        *last-- = 0;
    }

    return last != path + length - 1 ? S_OK : S_FALSE;
}

HRESULT WINAPI PathCchRenameExtension(WCHAR *path, SIZE_T size, const WCHAR *extension)
{
    HRESULT hr;

    TRACE("%s %lu %s\n", wine_dbgstr_w(path), size, wine_dbgstr_w(extension));

    hr = PathCchRemoveExtension(path, size);
    if (FAILED(hr)) return hr;

    hr = PathCchAddExtension(path, size, extension);
    return FAILED(hr) ? hr : S_OK;
}

HRESULT WINAPI PathCchSkipRoot(const WCHAR *path, const WCHAR **root_end)
{
    static const WCHAR unc_prefix[] = {'\\', '\\', '?'};

    TRACE("%s %p\n", debugstr_w(path), root_end);

    if (!path || !path[0] || !root_end
        || (!strncmpiW(unc_prefix, path, ARRAY_SIZE(unc_prefix)) && !is_prefixed_volume(path) && !is_prefixed_unc(path)
            && !is_prefixed_disk(path)))
        return E_INVALIDARG;

    *root_end = get_root_end(path);
    if (*root_end)
    {
        (*root_end)++;
        if (is_prefixed_unc(path))
        {
            get_next_segment(*root_end, root_end);
            get_next_segment(*root_end, root_end);
        }
        else if (path[0] == '\\' && path[1] == '\\' && path[2] != '?')
        {
            /* Skip share server */
            get_next_segment(*root_end, root_end);
            /* If mount point is empty, don't skip over mount point */
            if (**root_end != '\\') get_next_segment(*root_end, root_end);
        }
    }

    return *root_end ? S_OK : E_INVALIDARG;
}

HRESULT WINAPI PathCchStripPrefix(WCHAR *path, SIZE_T size)
{
    TRACE("%s %lu\n", wine_dbgstr_w(path), size);

    if (!path || !size || size > PATHCCH_MAX_CCH) return E_INVALIDARG;

    if (is_prefixed_unc(path))
    {
        /* \\?\UNC\a -> \\a */
        if (size < strlenW(path + 8) + 3) return E_INVALIDARG;
        strcpyW(path + 2, path + 8);
        return S_OK;
    }
    else if (is_prefixed_disk(path))
    {
        /* \\?\C:\ -> C:\ */
        if (size < strlenW(path + 4) + 1) return E_INVALIDARG;
        strcpyW(path, path + 4);
        return S_OK;
    }
    else
        return S_FALSE;
}

HRESULT WINAPI PathCchStripToRoot(WCHAR *path, SIZE_T size)
{
    const WCHAR *root_end;
    WCHAR *segment_end;
    BOOL is_unc;

    TRACE("%s %lu\n", wine_dbgstr_w(path), size);

    if (!path || !*path || !size || size > PATHCCH_MAX_CCH) return E_INVALIDARG;

    /* \\\\?\\UNC\\* and \\\\* have to have at least two extra segments to be striped,
     * e.g. \\\\?\\UNC\\a\\b\\c -> \\\\?\\UNC\\a\\b
     *      \\\\a\\b\\c         -> \\\\a\\b         */
    if ((is_unc = is_prefixed_unc(path)) || (path[0] == '\\' && path[1] == '\\' && path[2] != '?'))
    {
        root_end = is_unc ? path + 8 : path + 3;
        if (!get_next_segment(root_end, &root_end)) return S_FALSE;
        if (!get_next_segment(root_end, &root_end)) return S_FALSE;

        if (root_end - path >= size) return E_INVALIDARG;

        segment_end = path + (root_end - path) - 1;
        *segment_end = 0;
        return S_OK;
    }
    else if (PathCchSkipRoot(path, &root_end) == S_OK)
    {
        if (root_end - path >= size) return E_INVALIDARG;

        segment_end = path + (root_end - path);
        if (!*segment_end) return S_FALSE;

        *segment_end = 0;
        return S_OK;
    }
    else
        return E_INVALIDARG;
}

BOOL WINAPI PathIsUNCEx(const WCHAR *path, const WCHAR **server)
{
    const WCHAR *result = NULL;

    TRACE("%s %p\n", wine_dbgstr_w(path), server);

    if (is_prefixed_unc(path))
        result = path + 8;
    else if (path[0] == '\\' && path[1] == '\\' && path[2] != '?')
        result = path + 2;

    if (server) *server = result;
    return !!result;
}

BOOL WINAPI PathIsUNCA(const char *path)
{
    TRACE("%s\n", wine_dbgstr_a(path));

    return path && (path[0] == '\\') && (path[1] == '\\');
}

BOOL WINAPI PathIsUNCW(const WCHAR *path)
{
    TRACE("%s\n", wine_dbgstr_w(path));

    return path && (path[0] == '\\') && (path[1] == '\\');
}

BOOL WINAPI PathIsRelativeA(const char *path)
{
    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path || !*path || IsDBCSLeadByte(*path))
        return TRUE;

    return !(*path == '\\' || (*path && path[1] == ':'));
}

BOOL WINAPI PathIsRelativeW(const WCHAR *path)
{
    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path || !*path)
        return TRUE;

    return !(*path == '\\' || (*path && path[1] == ':'));
}

BOOL WINAPI PathIsUNCServerShareA(const char *path)
{
    BOOL seen_slash = FALSE;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (path && *path++ == '\\' && *path++ == '\\')
    {
        while (*path)
        {
            if (*path == '\\')
            {
                if (seen_slash)
                    return FALSE;
                seen_slash = TRUE;
            }

            path = char_next(path);
        }
    }

    return seen_slash;
}

BOOL WINAPI PathIsUNCServerShareW(const WCHAR *path)
{
    BOOL seen_slash = FALSE;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (path && *path++ == '\\' && *path++ == '\\')
    {
        while (*path)
        {
            if (*path == '\\')
            {
                if (seen_slash)
                    return FALSE;
                seen_slash = TRUE;
            }

            path++;
        }
    }

    return seen_slash;
}

BOOL WINAPI PathIsRootA(const char *path)
{
    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path || !*path)
        return FALSE;

    if (*path == '\\')
    {
        if (!path[1])
            return TRUE; /* \ */
        else if (path[1] == '\\')
        {
            BOOL seen_slash = FALSE;
            path += 2;

            /* Check for UNC root path */
            while (*path)
            {
                if (*path == '\\')
                {
                    if (seen_slash)
                        return FALSE;
                    seen_slash = TRUE;
                }

                path = char_next(path);
            }

            return TRUE;
        }
    }
    else if (path[1] == ':' && path[2] == '\\' && path[3] == '\0')
        return TRUE; /* X:\ */

    return FALSE;
}

BOOL WINAPI PathIsRootW(const WCHAR *path)
{
    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path || !*path)
        return FALSE;

    if (*path == '\\')
    {
        if (!path[1])
            return TRUE; /* \ */
        else if (path[1] == '\\')
        {
            BOOL seen_slash = FALSE;

            path += 2;
            /* Check for UNC root path */
            while (*path)
            {
                if (*path == '\\')
                {
                    if (seen_slash)
                        return FALSE;
                    seen_slash = TRUE;
                }
                path++;
            }

            return TRUE;
        }
    }
    else if (path[1] == ':' && path[2] == '\\' && path[3] == '\0')
        return TRUE; /* X:\ */

    return FALSE;
}

BOOL WINAPI PathRemoveFileSpecA(char *path)
{
    char *filespec = path;
    BOOL modified = FALSE;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path)
        return FALSE;

    /* Skip directory or UNC path */
    if (*path == '\\')
        filespec = ++path;
    if (*path == '\\')
        filespec = ++path;

    while (*path)
    {
        if (*path == '\\')
            filespec = path; /* Skip dir */
        else if (*path == ':')
        {
            filespec = ++path; /* Skip drive */
            if (*path == '\\')
                filespec++;
        }
        if (!(path = char_next(path)))
            break;
    }

    if (*filespec)
    {
        *filespec = '\0';
        modified = TRUE;
    }

    return modified;
}

BOOL WINAPI PathRemoveFileSpecW(WCHAR *path)
{
    WCHAR *filespec = path;
    BOOL modified = FALSE;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path)
        return FALSE;

    /* Skip directory or UNC path */
    if (*path == '\\')
        filespec = ++path;
    if (*path == '\\')
        filespec = ++path;

    while (*path)
    {
        if (*path == '\\')
            filespec = path; /* Skip dir */
        else if (*path == ':')
        {
            filespec = ++path; /* Skip drive */
            if (*path == '\\')
                filespec++;
        }

        path++;
    }

    if (*filespec)
    {
        *filespec = '\0';
        modified = TRUE;
    }

    return modified;
}

BOOL WINAPI PathStripToRootA(char *path)
{
    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path)
        return FALSE;

    while (!PathIsRootA(path))
        if (!PathRemoveFileSpecA(path))
            return FALSE;

    return TRUE;
}

BOOL WINAPI PathStripToRootW(WCHAR *path)
{
    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path)
        return FALSE;

    while (!PathIsRootW(path))
        if (!PathRemoveFileSpecW(path))
            return FALSE;

    return TRUE;
}

LPSTR WINAPI PathAddBackslashA(char *path)
{
    unsigned int len;
    char *prev = path;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path || (len = strlen(path)) >= MAX_PATH)
        return NULL;

    if (len)
    {
        do
        {
            path = char_next(prev);
            if (*path)
            prev = path;
        } while (*path);

        if (*prev != '\\')
        {
            *path++ = '\\';
            *path = '\0';
        }
    }

    return path;
}

LPWSTR WINAPI PathAddBackslashW(WCHAR *path)
{
    unsigned int len;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path || (len = strlenW(path)) >= MAX_PATH)
        return NULL;

    if (len)
    {
        path += len;
        if (path[-1] != '\\')
        {
            *path++ = '\\';
            *path = '\0';
        }
    }

    return path;
}

LPSTR WINAPI PathFindExtensionA(const char *path)
{
    const char *lastpoint = NULL;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (path)
    {
        while (*path)
        {
            if (*path == '\\' || *path == ' ')
                lastpoint = NULL;
            else if (*path == '.')
                lastpoint = path;
            path = char_next(path);
        }
    }

    return (LPSTR)(lastpoint ? lastpoint : path);
}

LPWSTR WINAPI PathFindExtensionW(const WCHAR *path)
{
    const WCHAR *lastpoint = NULL;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (path)
    {
        while (*path)
        {
            if (*path == '\\' || *path == ' ')
                lastpoint = NULL;
            else if (*path == '.')
                lastpoint = path;
            path++;
        }
    }

    return (LPWSTR)(lastpoint ? lastpoint : path);
}

BOOL WINAPI PathAddExtensionA(char *path, const char *ext)
{
    unsigned int len;

    TRACE("%s, %s\n", wine_dbgstr_a(path), wine_dbgstr_a(ext));

    if (!path || !ext || *(PathFindExtensionA(path)))
        return FALSE;

    len = strlen(path);
    if (len + strlen(ext) >= MAX_PATH)
        return FALSE;

    strcpy(path + len, ext);
    return TRUE;
}

BOOL WINAPI PathAddExtensionW(WCHAR *path, const WCHAR *ext)
{
    unsigned int len;

    TRACE("%s, %s\n", wine_dbgstr_w(path), wine_dbgstr_w(ext));

    if (!path || !ext || *(PathFindExtensionW(path)))
        return FALSE;

    len = strlenW(path);
    if (len + strlenW(ext) >= MAX_PATH)
        return FALSE;

    strcpyW(path + len, ext);
    return TRUE;
}

BOOL WINAPI PathCanonicalizeW(WCHAR *buffer, const WCHAR *path)
{
    const WCHAR *src = path;
    WCHAR *dst = buffer;

    TRACE("%p, %s\n", buffer, wine_dbgstr_w(path));

    if (dst)
        *dst = '\0';

    if (!dst || !path)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!*path)
    {
        *buffer++ = '\\';
        *buffer = '\0';
        return TRUE;
    }

    /* Copy path root */
    if (*src == '\\')
    {
        *dst++ = *src++;
    }
    else if (*src && src[1] == ':')
    {
        /* X:\ */
        *dst++ = *src++;
        *dst++ = *src++;
        if (*src == '\\')
            *dst++ = *src++;
    }

    /* Canonicalize the rest of the path */
    while (*src)
    {
        if (*src == '.')
        {
            if (src[1] == '\\' && (src == path || src[-1] == '\\' || src[-1] == ':'))
            {
                src += 2; /* Skip .\ */
            }
            else if (src[1] == '.' && (dst == buffer || dst[-1] == '\\'))
            {
                /* \.. backs up a directory, over the root if it has no \ following X:.
                 * .. is ignored if it would remove a UNC server name or initial \\
                 */
                if (dst != buffer)
                {
                    *dst = '\0'; /* Allow PathIsUNCServerShareA test on lpszBuf */
                    if (dst > buffer + 1 && dst[-1] == '\\' && (dst[-2] != '\\' || dst > buffer + 2))
                    {
                        if (dst[-2] == ':' && (dst > buffer + 3 || dst[-3] == ':'))
                        {
                            dst -= 2;
                            while (dst > buffer && *dst != '\\')
                                dst--;
                            if (*dst == '\\')
                                dst++; /* Reset to last '\' */
                            else
                                dst = buffer; /* Start path again from new root */
                        }
                        else if (dst[-2] != ':' && !PathIsUNCServerShareW(buffer))
                            dst -= 2;
                    }
                    while (dst > buffer && *dst != '\\')
                        dst--;
                    if (dst == buffer)
                    {
                        *dst++ = '\\';
                        src++;
                    }
                }
                src += 2; /* Skip .. in src path */
            }
            else
                *dst++ = *src++;
        }
        else
            *dst++ = *src++;
    }

    /* Append \ to naked drive specs */
    if (dst - buffer == 2 && dst[-1] == ':')
        *dst++ = '\\';
    *dst++ = '\0';
    return TRUE;
}

BOOL WINAPI PathCanonicalizeA(char *buffer, const char *path)
{
    WCHAR pathW[MAX_PATH], bufferW[MAX_PATH];
    BOOL ret;
    int len;

    TRACE("%p, %s\n", buffer, wine_dbgstr_a(path));

    if (buffer)
        *buffer = '\0';

    if (!buffer || !path)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    len = MultiByteToWideChar(CP_ACP, 0, path, -1, pathW, ARRAY_SIZE(pathW));
    if (!len)
        return FALSE;

    ret = PathCanonicalizeW(bufferW, pathW);
    WideCharToMultiByte(CP_ACP, 0, bufferW, -1, buffer, MAX_PATH, 0, 0);

    return ret;
}

WCHAR * WINAPI PathCombineW(WCHAR *dst, const WCHAR *dir, const WCHAR *file)
{
    BOOL use_both = FALSE, strip = FALSE;
    WCHAR tmp[MAX_PATH];

    TRACE("%p, %s, %s\n", dst, wine_dbgstr_w(dir), wine_dbgstr_w(file));

    /* Invalid parameters */
    if (!dst)
        return NULL;

    if (!dir && !file)
    {
        dst[0] = 0;
        return NULL;
    }

    if ((!file || !*file) && dir)
    {
        /* Use dir only */
        lstrcpynW(tmp, dir, ARRAY_SIZE(tmp));
    }
    else if (!dir || !*dir || !PathIsRelativeW(file))
    {
        if (!dir || !*dir || *file != '\\' || PathIsUNCW(file))
        {
            /* Use file only */
            lstrcpynW(tmp, file, ARRAY_SIZE(tmp));
        }
        else
        {
            use_both = TRUE;
            strip = TRUE;
        }
    }
    else
        use_both = TRUE;

    if (use_both)
    {
        lstrcpynW(tmp, dir, ARRAY_SIZE(tmp));
        if (strip)
        {
            PathStripToRootW(tmp);
            file++; /* Skip '\' */
        }

        if (!PathAddBackslashW(tmp) || strlenW(tmp) + strlenW(file) >= MAX_PATH)
        {
            dst[0] = 0;
            return NULL;
        }

        strcatW(tmp, file);
    }

    PathCanonicalizeW(dst, tmp);
    return dst;
}

LPSTR WINAPI PathCombineA(char *dst, const char *dir, const char *file)
{
    WCHAR dstW[MAX_PATH], dirW[MAX_PATH], fileW[MAX_PATH];

    TRACE("%p, %s, %s\n", dst, wine_dbgstr_a(dir), wine_dbgstr_a(file));

    /* Invalid parameters */
    if (!dst)
        return NULL;

    dst[0] = 0;

    if (!dir && !file)
        return NULL;

    if (dir && !MultiByteToWideChar(CP_ACP, 0, dir, -1, dirW, ARRAY_SIZE(dirW)))
        return NULL;

    if (file && !MultiByteToWideChar(CP_ACP, 0, file, -1, fileW, ARRAY_SIZE(fileW)))
        return NULL;

    if (PathCombineW(dstW, dir ? dirW : NULL, file ? fileW : NULL))
        if (WideCharToMultiByte(CP_ACP, 0, dstW, -1, dst, MAX_PATH, 0, 0))
            return dst;

    return NULL;
}

BOOL WINAPI PathAppendA(char *path, const char *append)
{
    TRACE("%s, %s\n", wine_dbgstr_a(path), wine_dbgstr_a(append));

    if (path && append)
    {
        if (!PathIsUNCA(append))
            while (*append == '\\')
                append++;

        if (PathCombineA(path, path, append))
            return TRUE;
    }

    return FALSE;
}

BOOL WINAPI PathAppendW(WCHAR *path, const WCHAR *append)
{
    TRACE("%s, %s\n", wine_dbgstr_w(path), wine_dbgstr_w(append));

    if (path && append)
    {
        if (!PathIsUNCW(append))
            while (*append == '\\')
                append++;

        if (PathCombineW(path, path, append))
            return TRUE;
    }

    return FALSE;
}

int WINAPI PathCommonPrefixA(const char *file1, const char *file2, char *path)
{
    const char *iter1 = file1;
    const char *iter2 = file2;
    unsigned int len = 0;

    TRACE("%s, %s, %p.\n", wine_dbgstr_a(file1), wine_dbgstr_a(file2), path);

    if (path)
        *path = '\0';

    if (!file1 || !file2)
        return 0;

    /* Handle roots first */
    if (PathIsUNCA(file1))
    {
        if (!PathIsUNCA(file2))
            return 0;
        iter1 += 2;
        iter2 += 2;
    }
    else if (PathIsUNCA(file2))
        return 0;

    for (;;)
    {
        /* Update len */
        if ((!*iter1 || *iter1 == '\\') && (!*iter2 || *iter2 == '\\'))
            len = iter1 - file1; /* Common to this point */

        if (!*iter1 || (tolower(*iter1) != tolower(*iter2)))
            break; /* Strings differ at this point */

        iter1++;
        iter2++;
    }

    if (len == 2)
        len++; /* Feature/Bug compatible with Win32 */

    if (len && path)
    {
        memcpy(path, file1, len);
        path[len] = '\0';
    }

    return len;
}

int WINAPI PathCommonPrefixW(const WCHAR *file1, const WCHAR *file2, WCHAR *path)
{
    const WCHAR *iter1 = file1;
    const WCHAR *iter2 = file2;
    unsigned int len = 0;

    TRACE("%s, %s, %p\n", wine_dbgstr_w(file1), wine_dbgstr_w(file2), path);

    if (path)
        *path = '\0';

    if (!file1 || !file2)
        return 0;

    /* Handle roots first */
    if (PathIsUNCW(file1))
    {
        if (!PathIsUNCW(file2))
            return 0;
        iter1 += 2;
        iter2 += 2;
    }
    else if (PathIsUNCW(file2))
      return 0;

    for (;;)
    {
        /* Update len */
        if ((!*iter1 || *iter1 == '\\') && (!*iter2 || *iter2 == '\\'))
            len = iter1 - file1; /* Common to this point */

        if (!*iter1 || (tolowerW(*iter1) != tolowerW(*iter2)))
            break; /* Strings differ at this point */

        iter1++;
        iter2++;
    }

    if (len == 2)
        len++; /* Feature/Bug compatible with Win32 */

    if (len && path)
    {
        memcpy(path, file1, len * sizeof(WCHAR));
        path[len] = '\0';
    }

    return len;
}

BOOL WINAPI PathIsPrefixA(const char *prefix, const char *path)
{
    TRACE("%s, %s\n", wine_dbgstr_a(prefix), wine_dbgstr_a(path));

    return prefix && path && PathCommonPrefixA(path, prefix, NULL) == (int)strlen(prefix);
}

BOOL WINAPI PathIsPrefixW(const WCHAR *prefix, const WCHAR *path)
{
    TRACE("%s, %s\n", wine_dbgstr_w(prefix), wine_dbgstr_w(path));

    return prefix && path && PathCommonPrefixW(path, prefix, NULL) == (int)strlenW(prefix);
}

char * WINAPI PathFindFileNameA(const char *path)
{
    const char *last_slash = path;

    TRACE("%s\n", wine_dbgstr_a(path));

    while (path && *path)
    {
        if ((*path == '\\' || *path == '/' || *path == ':') &&
                path[1] && path[1] != '\\' && path[1] != '/')
            last_slash = path + 1;
        path = char_next(path);
    }

    return (char *)last_slash;
}

WCHAR * WINAPI PathFindFileNameW(const WCHAR *path)
{
    const WCHAR *last_slash = path;

    TRACE("%s\n", wine_dbgstr_w(path));

    while (path && *path)
    {
        if ((*path == '\\' || *path == '/' || *path == ':') &&
                path[1] && path[1] != '\\' && path[1] != '/')
            last_slash = path + 1;
        path++;
    }

    return (WCHAR *)last_slash;
}

char * WINAPI PathGetArgsA(const char *path)
{
    BOOL seen_quote = FALSE;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path)
        return NULL;

    while (*path)
    {
        if (*path == ' ' && !seen_quote)
            return (char *)path + 1;

        if (*path == '"')
            seen_quote = !seen_quote;
        path = char_next(path);
    }

    return (char *)path;
}

WCHAR * WINAPI PathGetArgsW(const WCHAR *path)
{
    BOOL seen_quote = FALSE;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path)
        return NULL;

    while (*path)
    {
        if (*path == ' ' && !seen_quote)
            return (WCHAR *)path + 1;

        if (*path == '"')
            seen_quote = !seen_quote;
        path++;
    }

    return (WCHAR *)path;
}

UINT WINAPI PathGetCharTypeW(WCHAR ch)
{
    UINT flags = 0;

    TRACE("%#x\n", ch);

    if (!ch || ch < ' ' || ch == '<' || ch == '>' || ch == '"' || ch == '|' || ch == '/')
        flags = GCT_INVALID; /* Invalid */
    else if (ch == '*' || ch == '?')
        flags = GCT_WILD; /* Wildchars */
    else if (ch == '\\' || ch == ':')
        return GCT_SEPARATOR; /* Path separators */
    else
    {
        if (ch < 126)
        {
            if (((ch & 0x1) && ch != ';') || !ch || isalnum(ch) || ch == '$' || ch == '&' || ch == '(' ||
                    ch == '.' || ch == '@' || ch == '^' || ch == '\'' || ch == 130 || ch == '`')
            {
                flags |= GCT_SHORTCHAR; /* All these are valid for DOS */
            }
        }
        else
            flags |= GCT_SHORTCHAR; /* Bug compatible with win32 */

        flags |= GCT_LFNCHAR; /* Valid for long file names */
    }

    return flags;
}

UINT WINAPI PathGetCharTypeA(UCHAR ch)
{
    return PathGetCharTypeW(ch);
}

int WINAPI PathGetDriveNumberA(const char *path)
{
    TRACE("%s\n", wine_dbgstr_a(path));

    if (path && !IsDBCSLeadByte(*path) && path[1] == ':' && tolower(*path) >= 'a' && tolower(*path) <= 'z')
        return tolower(*path) - 'a';

    return -1;
}

int WINAPI PathGetDriveNumberW(const WCHAR *path)
{
    static const WCHAR nt_prefixW[] = {'\\','\\','?','\\'};
    WCHAR drive;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path)
        return -1;

    if (!strncmpW(path, nt_prefixW, 4))
        path += 4;

    drive = tolowerW(path[0]);
    if (drive < 'a' || drive > 'z' || path[1] != ':')
        return -1;

    return drive - 'a';
}

BOOL WINAPI PathIsFileSpecA(const char *path)
{
    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path)
        return FALSE;

    while (*path)
    {
        if (*path == '\\' || *path == ':')
            return FALSE;
        path = char_next(path);
    }

    return TRUE;
}

BOOL WINAPI PathIsFileSpecW(const WCHAR *path)
{
    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path)
        return FALSE;

    while (*path)
    {
        if (*path == '\\' || *path == ':')
            return FALSE;
        path++;
    }

    return TRUE;
}

BOOL WINAPI PathIsUNCServerA(const char *path)
{
    TRACE("%s\n", wine_dbgstr_a(path));

    if (!(path && path[0] == '\\' && path[1] == '\\'))
        return FALSE;

    while (*path)
    {
        if (*path == '\\')
            return FALSE;
        path = char_next(path);
    }

    return TRUE;
}

BOOL WINAPI PathIsUNCServerW(const WCHAR *path)
{
    TRACE("%s\n", wine_dbgstr_w(path));

    if (!(path && path[0] == '\\' && path[1] == '\\'))
        return FALSE;

    return !strchrW(path + 2, '\\');
}

void WINAPI PathRemoveBlanksA(char *path)
{
    char *start;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path || !*path)
        return;

    start = path;

    while (*path == ' ')
        path = char_next(path);

    while (*path)
        *start++ = *path++;

    if (start != path)
        while (start[-1] == ' ')
            start--;

    *start = '\0';
}

void WINAPI PathRemoveBlanksW(WCHAR *path)
{
    WCHAR *start = path;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path || !*path)
        return;

    while (*path == ' ')
        path++;

    while (*path)
        *start++ = *path++;

    if (start != path)
        while (start[-1] == ' ')
            start--;

    *start = '\0';
}

void WINAPI PathRemoveExtensionA(char *path)
{
    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path)
        return;

    path = PathFindExtensionA(path);
    if (path && !*path)
        *path = '\0';
}

void WINAPI PathRemoveExtensionW(WCHAR *path)
{
    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path)
        return;

    path = PathFindExtensionW(path);
    if (path && !*path)
        *path = '\0';
}

BOOL WINAPI PathRenameExtensionA(char *path, const char *ext)
{
    char *extension;

    TRACE("%s, %s\n", wine_dbgstr_a(path), wine_dbgstr_a(ext));

    extension = PathFindExtensionA(path);

    if (!extension || (extension - path + strlen(ext) >= MAX_PATH))
        return FALSE;

    strcpy(extension, ext);
    return TRUE;
}

BOOL WINAPI PathRenameExtensionW(WCHAR *path, const WCHAR *ext)
{
    WCHAR *extension;

    TRACE("%s, %s\n", wine_dbgstr_w(path), wine_dbgstr_w(ext));

    extension = PathFindExtensionW(path);

    if (!extension || (extension - path + strlenW(ext) >= MAX_PATH))
        return FALSE;

    strcpyW(extension, ext);
    return TRUE;
}

void WINAPI PathUnquoteSpacesA(char *path)
{
    unsigned int len;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path || *path != '"')
        return;

    len = strlen(path) - 1;
    if (path[len] == '"')
    {
        path[len] = '\0';
        for (; *path; path++)
            *path = path[1];
    }
}

void WINAPI PathUnquoteSpacesW(WCHAR *path)
{
    unsigned int len;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path || *path != '"')
        return;

    len = strlenW(path) - 1;
    if (path[len] == '"')
    {
        path[len] = '\0';
        for (; *path; path++)
            *path = path[1];
    }
}

char * WINAPI PathRemoveBackslashA(char *path)
{
    char *ptr;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path)
        return NULL;

    ptr = char_prev(path, path + strlen(path));
    if (!PathIsRootA(path) && *ptr == '\\')
        *ptr = '\0';

    return ptr;
}

WCHAR * WINAPI PathRemoveBackslashW(WCHAR *path)
{
    WCHAR *ptr;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path)
        return NULL;

    ptr = path + strlenW(path);
    if (ptr > path) ptr--;
    if (!PathIsRootW(path) && *ptr == '\\')
      *ptr = '\0';

    return ptr;
}

BOOL WINAPI PathIsLFNFileSpecA(const char *path)
{
    unsigned int name_len = 0, ext_len = 0;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path)
        return FALSE;

    while (*path)
    {
        if (*path == ' ')
            return TRUE; /* DOS names cannot have spaces */
        if (*path == '.')
        {
            if (ext_len)
                return TRUE; /* DOS names have only one dot */
            ext_len = 1;
        }
        else if (ext_len)
        {
            ext_len++;
            if (ext_len > 4)
                return TRUE; /* DOS extensions are <= 3 chars*/
        }
        else
        {
            name_len++;
            if (name_len > 8)
                return TRUE; /* DOS names are <= 8 chars */
        }
        path = char_next(path);
    }

    return FALSE; /* Valid DOS path */
}

BOOL WINAPI PathIsLFNFileSpecW(const WCHAR *path)
{
    unsigned int name_len = 0, ext_len = 0;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path)
        return FALSE;

    while (*path)
    {
        if (*path == ' ')
            return TRUE; /* DOS names cannot have spaces */
        if (*path == '.')
        {
            if (ext_len)
                return TRUE; /* DOS names have only one dot */
            ext_len = 1;
        }
        else if (ext_len)
        {
            ext_len++;
            if (ext_len > 4)
                return TRUE; /* DOS extensions are <= 3 chars*/
        }
        else
        {
            name_len++;
            if (name_len > 8)
                return TRUE; /* DOS names are <= 8 chars */
        }
        path++;
    }

    return FALSE; /* Valid DOS path */
}

#define PATH_CHAR_CLASS_LETTER      0x00000001
#define PATH_CHAR_CLASS_ASTERIX     0x00000002
#define PATH_CHAR_CLASS_DOT         0x00000004
#define PATH_CHAR_CLASS_BACKSLASH   0x00000008
#define PATH_CHAR_CLASS_COLON       0x00000010
#define PATH_CHAR_CLASS_SEMICOLON   0x00000020
#define PATH_CHAR_CLASS_COMMA       0x00000040
#define PATH_CHAR_CLASS_SPACE       0x00000080
#define PATH_CHAR_CLASS_OTHER_VALID 0x00000100
#define PATH_CHAR_CLASS_DOUBLEQUOTE 0x00000200

#define PATH_CHAR_CLASS_INVALID     0x00000000
#define PATH_CHAR_CLASS_ANY         0xffffffff

static const DWORD path_charclass[] =
{
    /* 0x00 */  PATH_CHAR_CLASS_INVALID,      /* 0x01 */  PATH_CHAR_CLASS_INVALID,
    /* 0x02 */  PATH_CHAR_CLASS_INVALID,      /* 0x03 */  PATH_CHAR_CLASS_INVALID,
    /* 0x04 */  PATH_CHAR_CLASS_INVALID,      /* 0x05 */  PATH_CHAR_CLASS_INVALID,
    /* 0x06 */  PATH_CHAR_CLASS_INVALID,      /* 0x07 */  PATH_CHAR_CLASS_INVALID,
    /* 0x08 */  PATH_CHAR_CLASS_INVALID,      /* 0x09 */  PATH_CHAR_CLASS_INVALID,
    /* 0x0a */  PATH_CHAR_CLASS_INVALID,      /* 0x0b */  PATH_CHAR_CLASS_INVALID,
    /* 0x0c */  PATH_CHAR_CLASS_INVALID,      /* 0x0d */  PATH_CHAR_CLASS_INVALID,
    /* 0x0e */  PATH_CHAR_CLASS_INVALID,      /* 0x0f */  PATH_CHAR_CLASS_INVALID,
    /* 0x10 */  PATH_CHAR_CLASS_INVALID,      /* 0x11 */  PATH_CHAR_CLASS_INVALID,
    /* 0x12 */  PATH_CHAR_CLASS_INVALID,      /* 0x13 */  PATH_CHAR_CLASS_INVALID,
    /* 0x14 */  PATH_CHAR_CLASS_INVALID,      /* 0x15 */  PATH_CHAR_CLASS_INVALID,
    /* 0x16 */  PATH_CHAR_CLASS_INVALID,      /* 0x17 */  PATH_CHAR_CLASS_INVALID,
    /* 0x18 */  PATH_CHAR_CLASS_INVALID,      /* 0x19 */  PATH_CHAR_CLASS_INVALID,
    /* 0x1a */  PATH_CHAR_CLASS_INVALID,      /* 0x1b */  PATH_CHAR_CLASS_INVALID,
    /* 0x1c */  PATH_CHAR_CLASS_INVALID,      /* 0x1d */  PATH_CHAR_CLASS_INVALID,
    /* 0x1e */  PATH_CHAR_CLASS_INVALID,      /* 0x1f */  PATH_CHAR_CLASS_INVALID,
    /* ' '  */  PATH_CHAR_CLASS_SPACE,        /* '!'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '"'  */  PATH_CHAR_CLASS_DOUBLEQUOTE,  /* '#'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '$'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* '%'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '&'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* '\'' */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '('  */  PATH_CHAR_CLASS_OTHER_VALID,  /* ')'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '*'  */  PATH_CHAR_CLASS_ASTERIX,      /* '+'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* ','  */  PATH_CHAR_CLASS_COMMA,        /* '-'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '.'  */  PATH_CHAR_CLASS_DOT,          /* '/'  */  PATH_CHAR_CLASS_INVALID,
    /* '0'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* '1'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '2'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* '3'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '4'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* '5'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '6'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* '7'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '8'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* '9'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* ':'  */  PATH_CHAR_CLASS_COLON,        /* ';'  */  PATH_CHAR_CLASS_SEMICOLON,
    /* '<'  */  PATH_CHAR_CLASS_INVALID,      /* '='  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '>'  */  PATH_CHAR_CLASS_INVALID,      /* '?'  */  PATH_CHAR_CLASS_LETTER,
    /* '@'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* 'A'  */  PATH_CHAR_CLASS_ANY,
    /* 'B'  */  PATH_CHAR_CLASS_ANY,          /* 'C'  */  PATH_CHAR_CLASS_ANY,
    /* 'D'  */  PATH_CHAR_CLASS_ANY,          /* 'E'  */  PATH_CHAR_CLASS_ANY,
    /* 'F'  */  PATH_CHAR_CLASS_ANY,          /* 'G'  */  PATH_CHAR_CLASS_ANY,
    /* 'H'  */  PATH_CHAR_CLASS_ANY,          /* 'I'  */  PATH_CHAR_CLASS_ANY,
    /* 'J'  */  PATH_CHAR_CLASS_ANY,          /* 'K'  */  PATH_CHAR_CLASS_ANY,
    /* 'L'  */  PATH_CHAR_CLASS_ANY,          /* 'M'  */  PATH_CHAR_CLASS_ANY,
    /* 'N'  */  PATH_CHAR_CLASS_ANY,          /* 'O'  */  PATH_CHAR_CLASS_ANY,
    /* 'P'  */  PATH_CHAR_CLASS_ANY,          /* 'Q'  */  PATH_CHAR_CLASS_ANY,
    /* 'R'  */  PATH_CHAR_CLASS_ANY,          /* 'S'  */  PATH_CHAR_CLASS_ANY,
    /* 'T'  */  PATH_CHAR_CLASS_ANY,          /* 'U'  */  PATH_CHAR_CLASS_ANY,
    /* 'V'  */  PATH_CHAR_CLASS_ANY,          /* 'W'  */  PATH_CHAR_CLASS_ANY,
    /* 'X'  */  PATH_CHAR_CLASS_ANY,          /* 'Y'  */  PATH_CHAR_CLASS_ANY,
    /* 'Z'  */  PATH_CHAR_CLASS_ANY,          /* '['  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '\\' */  PATH_CHAR_CLASS_BACKSLASH,    /* ']'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '^'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* '_'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '`'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* 'a'  */  PATH_CHAR_CLASS_ANY,
    /* 'b'  */  PATH_CHAR_CLASS_ANY,          /* 'c'  */  PATH_CHAR_CLASS_ANY,
    /* 'd'  */  PATH_CHAR_CLASS_ANY,          /* 'e'  */  PATH_CHAR_CLASS_ANY,
    /* 'f'  */  PATH_CHAR_CLASS_ANY,          /* 'g'  */  PATH_CHAR_CLASS_ANY,
    /* 'h'  */  PATH_CHAR_CLASS_ANY,          /* 'i'  */  PATH_CHAR_CLASS_ANY,
    /* 'j'  */  PATH_CHAR_CLASS_ANY,          /* 'k'  */  PATH_CHAR_CLASS_ANY,
    /* 'l'  */  PATH_CHAR_CLASS_ANY,          /* 'm'  */  PATH_CHAR_CLASS_ANY,
    /* 'n'  */  PATH_CHAR_CLASS_ANY,          /* 'o'  */  PATH_CHAR_CLASS_ANY,
    /* 'p'  */  PATH_CHAR_CLASS_ANY,          /* 'q'  */  PATH_CHAR_CLASS_ANY,
    /* 'r'  */  PATH_CHAR_CLASS_ANY,          /* 's'  */  PATH_CHAR_CLASS_ANY,
    /* 't'  */  PATH_CHAR_CLASS_ANY,          /* 'u'  */  PATH_CHAR_CLASS_ANY,
    /* 'v'  */  PATH_CHAR_CLASS_ANY,          /* 'w'  */  PATH_CHAR_CLASS_ANY,
    /* 'x'  */  PATH_CHAR_CLASS_ANY,          /* 'y'  */  PATH_CHAR_CLASS_ANY,
    /* 'z'  */  PATH_CHAR_CLASS_ANY,          /* '{'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '|'  */  PATH_CHAR_CLASS_INVALID,      /* '}'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '~'  */  PATH_CHAR_CLASS_OTHER_VALID
};

BOOL WINAPI PathIsValidCharA(char c, DWORD class)
{
    if ((unsigned)c > 0x7e)
        return class & PATH_CHAR_CLASS_OTHER_VALID;

    return class & path_charclass[(unsigned)c];
}

BOOL WINAPI PathIsValidCharW(WCHAR c, DWORD class)
{
    if (c > 0x7e)
        return class & PATH_CHAR_CLASS_OTHER_VALID;

    return class & path_charclass[c];
}
