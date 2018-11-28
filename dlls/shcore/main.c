/*
 * Copyright 2016 Sebastian Lackner
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

#include "config.h"
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "initguid.h"
#include "ocidl.h"
#include "shellscalingapi.h"
#include "shlwapi.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(shcore);

static DWORD shcore_tls;
static IUnknown *process_ref;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    TRACE("(%p, %u, %p)\n", instance, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;  /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(instance);
            shcore_tls = TlsAlloc();
            break;
        case DLL_PROCESS_DETACH:
            if (reserved) break;
            if (shcore_tls != TLS_OUT_OF_INDEXES)
                TlsFree(shcore_tls);
            break;
    }

    return TRUE;
}

HRESULT WINAPI GetProcessDpiAwareness(HANDLE process, PROCESS_DPI_AWARENESS *value)
{
    if (GetProcessDpiAwarenessInternal( process, (DPI_AWARENESS *)value )) return S_OK;
    return HRESULT_FROM_WIN32( GetLastError() );
}

HRESULT WINAPI SetProcessDpiAwareness(PROCESS_DPI_AWARENESS value)
{
    if (SetProcessDpiAwarenessInternal( value )) return S_OK;
    return HRESULT_FROM_WIN32( GetLastError() );
}

HRESULT WINAPI GetDpiForMonitor(HMONITOR monitor, MONITOR_DPI_TYPE type, UINT *x, UINT *y)
{
    if (GetDpiForMonitorInternal( monitor, type, x, y )) return S_OK;
    return HRESULT_FROM_WIN32( GetLastError() );
}

HRESULT WINAPI _IStream_Read(IStream *stream, void *dest, ULONG size)
{
    ULONG read;
    HRESULT hr;

    TRACE("(%p, %p, %u)\n", stream, dest, size);

    hr = IStream_Read(stream, dest, size, &read);
    if (SUCCEEDED(hr) && read != size)
        hr = E_FAIL;
    return hr;
}

HRESULT WINAPI IStream_Reset(IStream *stream)
{
    static const LARGE_INTEGER zero;

    TRACE("(%p)\n", stream);

    return IStream_Seek(stream, zero, 0, NULL);
}

HRESULT WINAPI IStream_Size(IStream *stream, ULARGE_INTEGER *size)
{
    STATSTG statstg;
    HRESULT hr;

    TRACE("(%p, %p)\n", stream, size);

    memset(&statstg, 0, sizeof(statstg));

    hr = IStream_Stat(stream, &statstg, STATFLAG_NONAME);

    if (SUCCEEDED(hr) && size)
        *size = statstg.cbSize;
    return hr;
}

HRESULT WINAPI _IStream_Write(IStream *stream, const void *src, ULONG size)
{
    ULONG written;
    HRESULT hr;

    TRACE("(%p, %p, %u)\n", stream, src, size);

    hr = IStream_Write(stream, src, size, &written);
    if (SUCCEEDED(hr) && written != size)
        hr = E_FAIL;

    return hr;
}

void WINAPI IUnknown_AtomicRelease(IUnknown **obj)
{
    TRACE("(%p)\n", obj);

    if (!obj || !*obj)
        return;

    IUnknown_Release(*obj);
    *obj = NULL;
}

HRESULT WINAPI IUnknown_GetSite(IUnknown *unk, REFIID iid, void **site)
{
    IObjectWithSite *obj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p, %s, %p)\n", unk, debugstr_guid(iid), site);

    if (unk && iid && site)
    {
        hr = IUnknown_QueryInterface(unk, &IID_IObjectWithSite, (void **)&obj);
        if (SUCCEEDED(hr) && obj)
        {
            hr = IObjectWithSite_GetSite(obj, iid, site);
            IObjectWithSite_Release(obj);
        }
    }

    return hr;
}

HRESULT WINAPI IUnknown_QueryService(IUnknown *obj, REFGUID sid, REFIID iid, void **out)
{
    IServiceProvider *provider = NULL;
    HRESULT hr;

    if (!out)
        return E_FAIL;

    *out = NULL;

    if (!obj)
        return E_FAIL;

    hr = IUnknown_QueryInterface(obj, &IID_IServiceProvider, (void **)&provider);
    if (hr == S_OK && provider)
    {
        TRACE("Using provider %p.\n", provider);

        hr = IServiceProvider_QueryService(provider, sid, iid, out);

        TRACE("Provider %p returned %p.\n", provider, *out);

        IServiceProvider_Release(provider);
    }

    return hr;
}

void WINAPI IUnknown_Set(IUnknown **dest, IUnknown *src)
{
    TRACE("(%p, %p)\n", dest, src);

    IUnknown_AtomicRelease(dest);

    if (src)
    {
        IUnknown_AddRef(src);
        *dest = src;
    }
}

HRESULT WINAPI IUnknown_SetSite(IUnknown *obj, IUnknown *site)
{
    IInternetSecurityManager *sec_manager;
    IObjectWithSite *objwithsite;
    HRESULT hr;

    if (!obj)
        return E_FAIL;

    hr = IUnknown_QueryInterface(obj, &IID_IObjectWithSite, (void **)&objwithsite);
    TRACE("ObjectWithSite %p, hr %#x.\n", objwithsite, hr);
    if (SUCCEEDED(hr))
    {
        hr = IObjectWithSite_SetSite(objwithsite, site);
        TRACE("SetSite() hr %#x.\n", hr);
        IObjectWithSite_Release(objwithsite);
    }
    else
    {
        hr = IUnknown_QueryInterface(obj, &IID_IInternetSecurityManager, (void **)&sec_manager);
        TRACE("InternetSecurityManager %p, hr %#x.\n", sec_manager, hr);
        if (FAILED(hr))
            return hr;

        hr = IInternetSecurityManager_SetSecuritySite(sec_manager, (IInternetSecurityMgrSite *)site);
        TRACE("SetSecuritySite() hr %#x.\n", hr);
        IInternetSecurityManager_Release(sec_manager);
    }

    return hr;
}

HRESULT WINAPI SetCurrentProcessExplicitAppUserModelID(const WCHAR *appid)
{
    FIXME("%s: stub\n", debugstr_w(appid));
    return E_NOTIMPL;
}

HRESULT WINAPI GetCurrentProcessExplicitAppUserModelID(const WCHAR **appid)
{
    FIXME("%p: stub\n", appid);
    *appid = NULL;
    return E_NOTIMPL;
}

/*************************************************************************
 * CommandLineToArgvW            [SHCORE.@]
 *
 * We must interpret the quotes in the command line to rebuild the argv
 * array correctly:
 * - arguments are separated by spaces or tabs
 * - quotes serve as optional argument delimiters
 *   '"a b"'   -> 'a b'
 * - escaped quotes must be converted back to '"'
 *   '\"'      -> '"'
 * - consecutive backslashes preceding a quote see their number halved with
 *   the remainder escaping the quote:
 *   2n   backslashes + quote -> n backslashes + quote as an argument delimiter
 *   2n+1 backslashes + quote -> n backslashes + literal quote
 * - backslashes that are not followed by a quote are copied literally:
 *   'a\b'     -> 'a\b'
 *   'a\\b'    -> 'a\\b'
 * - in quoted strings, consecutive quotes see their number divided by three
 *   with the remainder modulo 3 deciding whether to close the string or not.
 *   Note that the opening quote must be counted in the consecutive quotes,
 *   that's the (1+) below:
 *   (1+) 3n   quotes -> n quotes
 *   (1+) 3n+1 quotes -> n quotes plus closes the quoted string
 *   (1+) 3n+2 quotes -> n+1 quotes plus closes the quoted string
 * - in unquoted strings, the first quote opens the quoted string and the
 *   remaining consecutive quotes follow the above rule.
 */
WCHAR** WINAPI CommandLineToArgvW(const WCHAR *cmdline, int *numargs)
{
    int qcount, bcount;
    const WCHAR *s;
    WCHAR **argv;
    DWORD argc;
    WCHAR *d;

    if (!numargs)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (*cmdline == 0)
    {
        /* Return the path to the executable */
        DWORD len, deslen = MAX_PATH, size;

        size = sizeof(WCHAR *) * 2 + deslen * sizeof(WCHAR);
        for (;;)
        {
            if (!(argv = LocalAlloc(LMEM_FIXED, size))) return NULL;
            len = GetModuleFileNameW(0, (WCHAR *)(argv + 2), deslen);
            if (!len)
            {
                LocalFree(argv);
                return NULL;
            }
            if (len < deslen) break;
            deslen *= 2;
            size = sizeof(WCHAR *) * 2 + deslen * sizeof(WCHAR);
            LocalFree(argv);
        }
        argv[0] = (WCHAR *)(argv + 2);
        argv[1] = NULL;
        *numargs = 1;

        return argv;
    }

    /* --- First count the arguments */
    argc = 1;
    s = cmdline;
    /* The first argument, the executable path, follows special rules */
    if (*s == '"')
    {
        /* The executable path ends at the next quote, no matter what */
        s++;
        while (*s)
            if (*s++ == '"')
                break;
    }
    else
    {
        /* The executable path ends at the next space, no matter what */
        while (*s && *s != ' ' && *s != '\t')
            s++;
    }
    /* skip to the first argument, if any */
    while (*s == ' ' || *s == '\t')
        s++;
    if (*s)
        argc++;

    /* Analyze the remaining arguments */
    qcount = bcount = 0;
    while (*s)
    {
        if ((*s == ' ' || *s == '\t') && qcount == 0)
        {
            /* skip to the next argument and count it if any */
            while (*s == ' ' || *s == '\t')
                s++;
            if (*s)
                argc++;
            bcount = 0;
        }
        else if (*s == '\\')
        {
            /* '\', count them */
            bcount++;
            s++;
        }
        else if (*s == '"')
        {
            /* '"' */
            if ((bcount & 1) == 0)
                qcount++; /* unescaped '"' */
            s++;
            bcount = 0;
            /* consecutive quotes, see comment in copying code below */
            while (*s == '"')
            {
                qcount++;
                s++;
            }
            qcount = qcount % 3;
            if (qcount == 2)
                qcount = 0;
        }
        else
        {
            /* a regular character */
            bcount = 0;
            s++;
        }
    }

    /* Allocate in a single lump, the string array, and the strings that go
     * with it. This way the caller can make a single LocalFree() call to free
     * both, as per MSDN.
     */
    argv = LocalAlloc(LMEM_FIXED, (argc + 1) * sizeof(WCHAR *) + (strlenW(cmdline) + 1) * sizeof(WCHAR));
    if (!argv)
        return NULL;

    /* --- Then split and copy the arguments */
    argv[0] = d = strcpyW((WCHAR *)(argv + argc + 1), cmdline);
    argc = 1;
    /* The first argument, the executable path, follows special rules */
    if (*d == '"')
    {
        /* The executable path ends at the next quote, no matter what */
        s = d + 1;
        while (*s)
        {
            if (*s == '"')
            {
                s++;
                break;
            }
            *d++ = *s++;
        }
    }
    else
    {
        /* The executable path ends at the next space, no matter what */
        while (*d && *d != ' ' && *d != '\t')
            d++;
        s = d;
        if (*s)
            s++;
    }
    /* close the executable path */
    *d++ = 0;
    /* skip to the first argument and initialize it if any */
    while (*s == ' ' || *s == '\t')
        s++;
    if (!*s)
    {
        /* There are no parameters so we are all done */
        argv[argc] = NULL;
        *numargs = argc;
        return argv;
    }

    /* Split and copy the remaining arguments */
    argv[argc++] = d;
    qcount = bcount = 0;
    while (*s)
    {
        if ((*s == ' ' || *s == '\t') && qcount == 0)
        {
            /* close the argument */
            *d++ = 0;
            bcount = 0;

            /* skip to the next one and initialize it if any */
            do {
                s++;
            } while (*s == ' ' || *s == '\t');
            if (*s)
                argv[argc++] = d;
        }
        else if (*s=='\\')
        {
            *d++ = *s++;
            bcount++;
        }
        else if (*s == '"')
        {
            if ((bcount & 1) == 0)
            {
                /* Preceded by an even number of '\', this is half that
                 * number of '\', plus a quote which we erase.
                 */
                d -= bcount / 2;
                qcount++;
            }
            else
            {
                /* Preceded by an odd number of '\', this is half that
                 * number of '\' followed by a '"'
                 */
                d = d - bcount / 2 - 1;
                *d++ = '"';
            }
            s++;
            bcount = 0;
            /* Now count the number of consecutive quotes. Note that qcount
             * already takes into account the opening quote if any, as well as
             * the quote that lead us here.
             */
            while (*s == '"')
            {
                if (++qcount == 3)
                {
                    *d++ = '"';
                    qcount = 0;
                }
                s++;
            }
            if (qcount == 2)
                qcount = 0;
        }
        else
        {
            /* a regular character */
            *d++ = *s++;
            bcount = 0;
        }
    }
    *d = '\0';
    argv[argc] = NULL;
    *numargs = argc;

    return argv;
}

struct shstream
{
    IStream IStream_iface;
    LONG refcount;

    union
    {
        struct
        {
            BYTE *buffer;
            DWORD length;
            DWORD position;
        } mem;
        struct
        {
            HANDLE handle;
            DWORD mode;
            WCHAR *path;
        } file;
    } u;
};

static inline struct shstream *impl_from_IStream(IStream *iface)
{
    return CONTAINING_RECORD(iface, struct shstream, IStream_iface);
}

static HRESULT WINAPI shstream_QueryInterface(IStream *iface, REFIID riid, void **out)
{
    struct shstream *stream = impl_from_IStream(iface);

    TRACE("(%p)->(%s, %p)\n", stream, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IStream))
    {
        *out = iface;
        IStream_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI shstream_AddRef(IStream *iface)
{
    struct shstream *stream = impl_from_IStream(iface);
    ULONG refcount = InterlockedIncrement(&stream->refcount);

    TRACE("(%p)->(%u)\n", stream, refcount);

    return refcount;
}

static ULONG WINAPI memstream_Release(IStream *iface)
{
    struct shstream *stream = impl_from_IStream(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);

    TRACE("(%p)->(%u)\n", stream, refcount);

    if (!refcount)
    {
        heap_free(stream->u.mem.buffer);
        heap_free(stream);
    }

    return refcount;
}

static HRESULT WINAPI memstream_Read(IStream *iface, void *buff, ULONG buff_size, ULONG *read_len)
{
    struct shstream *stream = impl_from_IStream(iface);
    DWORD length;

    TRACE("(%p)->(%p, %u, %p)\n", stream, buff, buff_size, read_len);

    if (stream->u.mem.position >= stream->u.mem.length)
        length = 0;
    else
        length = stream->u.mem.length - stream->u.mem.position;

    length = buff_size > length ? length : buff_size;
    if (length != 0) /* not at end of buffer and we want to read something */
    {
        memmove(buff, stream->u.mem.buffer + stream->u.mem.position, length);
        stream->u.mem.position += length; /* adjust pointer */
    }

    if (read_len)
        *read_len = length;

    return S_OK;
}

static HRESULT WINAPI memstream_Write(IStream *iface, const void *buff, ULONG buff_size, ULONG *written)
{
    struct shstream *stream = impl_from_IStream(iface);
    DWORD length = stream->u.mem.position + buff_size;

    TRACE("(%p)->(%p, %u, %p)\n", stream, buff, buff_size, written);

    if (length < stream->u.mem.position) /* overflow */
        return STG_E_INSUFFICIENTMEMORY;

    if (length > stream->u.mem.length)
    {
        BYTE *buffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, stream->u.mem.buffer, length);
        if (!buffer)
            return STG_E_INSUFFICIENTMEMORY;

        stream->u.mem.length = length;
        stream->u.mem.buffer = buffer;
    }
    memmove(stream->u.mem.buffer + stream->u.mem.position, buff, buff_size);
    stream->u.mem.position += buff_size; /* adjust pointer */

    if (written)
        *written = buff_size;

    return S_OK;
}

static HRESULT WINAPI memstream_Seek(IStream *iface, LARGE_INTEGER move, DWORD origin, ULARGE_INTEGER*new_pos)
{
    struct shstream *stream = impl_from_IStream(iface);
    LARGE_INTEGER tmp;

    TRACE("(%p)->(%s, %d, %p)\n", stream, wine_dbgstr_longlong(move.QuadPart), origin, new_pos);

    if (origin == STREAM_SEEK_SET)
        tmp = move;
    else if (origin == STREAM_SEEK_CUR)
        tmp.QuadPart = stream->u.mem.position + move.QuadPart;
    else if (origin == STREAM_SEEK_END)
        tmp.QuadPart = stream->u.mem.length + move.QuadPart;
    else
        return STG_E_INVALIDPARAMETER;

    if (tmp.QuadPart < 0)
        return STG_E_INVALIDFUNCTION;

    /* we cut off the high part here */
    stream->u.mem.position = tmp.u.LowPart;

    if (new_pos)
        new_pos->QuadPart = stream->u.mem.position;
    return S_OK;
}

static HRESULT WINAPI memstream_SetSize(IStream *iface, ULARGE_INTEGER new_size)
{
    struct shstream *stream = impl_from_IStream(iface);
    DWORD length;
    BYTE *buffer;

    TRACE("(%p, %s)\n", stream, wine_dbgstr_longlong(new_size.QuadPart));

    /* we cut off the high part here */
    length = new_size.u.LowPart;
    buffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, stream->u.mem.buffer, length);
    if (!buffer)
        return STG_E_INSUFFICIENTMEMORY;

    stream->u.mem.buffer = buffer;
    stream->u.mem.length = length;

    return S_OK;
}

static HRESULT WINAPI shstream_CopyTo(IStream *iface, IStream *pstm, ULARGE_INTEGER size, ULARGE_INTEGER *read_len, ULARGE_INTEGER *written)
{
    struct shstream *stream = impl_from_IStream(iface);

    TRACE("(%p)\n", stream);

    if (read_len)
        read_len->QuadPart = 0;

    if (written)
        written->QuadPart = 0;

    /* TODO implement */
    return E_NOTIMPL;
}

static HRESULT WINAPI shstream_Commit(IStream *iface, DWORD flags)
{
    struct shstream *stream = impl_from_IStream(iface);

    TRACE("(%p, %#x)\n", stream, flags);

    /* Commit is not supported by this stream */
    return E_NOTIMPL;
}

static HRESULT WINAPI shstream_Revert(IStream *iface)
{
    struct shstream *stream = impl_from_IStream(iface);

    TRACE("(%p)\n", stream);

    /* revert not supported by this stream */
    return E_NOTIMPL;
}

static HRESULT WINAPI shstream_LockRegion(IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER size, DWORD lock_type)
{
    struct shstream *stream = impl_from_IStream(iface);

    TRACE("(%p)\n", stream);

    /* lock/unlock not supported by this stream */
    return E_NOTIMPL;
}

static HRESULT WINAPI shstream_UnlockRegion(IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER size, DWORD lock_type)
{
    struct shstream *stream = impl_from_IStream(iface);

    TRACE("(%p)\n", stream);

    /* lock/unlock not supported by this stream */
    return E_NOTIMPL;
}

static HRESULT WINAPI memstream_Stat(IStream *iface, STATSTG *statstg, DWORD flags)
{
    struct shstream *stream = impl_from_IStream(iface);

    TRACE("(%p, %p, %#x)\n", stream, statstg, flags);

    memset(statstg, 0, sizeof(*statstg));
    statstg->type = STGTY_STREAM;
    statstg->cbSize.QuadPart = stream->u.mem.length;
    statstg->grfMode = STGM_READWRITE;

    return S_OK;
}

static HRESULT WINAPI shstream_Clone(IStream *iface, IStream **dest)
{
    struct shstream *stream = impl_from_IStream(iface);

    TRACE("(%p, %p)\n", stream, dest);

    *dest = NULL;

    /* clone not supported by this stream */
    return E_NOTIMPL;
}

static const IStreamVtbl memstreamvtbl =
{
    shstream_QueryInterface,
    shstream_AddRef,
    memstream_Release,
    memstream_Read,
    memstream_Write,
    memstream_Seek,
    memstream_SetSize,
    shstream_CopyTo,
    shstream_Commit,
    shstream_Revert,
    shstream_LockRegion,
    shstream_UnlockRegion,
    memstream_Stat,
    shstream_Clone,
};

/*************************************************************************
 * SHCreateMemStream   [SHCORE.@]
 *
 * Create an IStream object on a block of memory.
 *
 * PARAMS
 * data     [I] Memory block to create the IStream object on
 * data_len [I] Length of data block
 *
 * RETURNS
 * Success: A pointer to the IStream object.
 * Failure: NULL, if any parameters are invalid or an error occurs.
 *
 * NOTES
 *  A copy of the memory block is made, it's freed when the stream is released.
 */
IStream * WINAPI SHCreateMemStream(const BYTE *data, UINT data_len)
{
    struct shstream *stream;

    TRACE("(%p, %u)\n", data, data_len);

    if (!data)
        data_len = 0;

    stream = heap_alloc(sizeof(*stream));
    stream->IStream_iface.lpVtbl = &memstreamvtbl;
    stream->refcount = 1;
    stream->u.mem.buffer = heap_alloc(data_len);
    if (!stream->u.mem.buffer)
    {
        heap_free(stream);
        return NULL;
    }
    memcpy(stream->u.mem.buffer, data, data_len);
    stream->u.mem.length = data_len;
    stream->u.mem.position = 0;

    return &stream->IStream_iface;
}

static ULONG WINAPI filestream_Release(IStream *iface)
{
    struct shstream *stream = impl_from_IStream(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);

    TRACE("(%p)->(%u)\n", stream, refcount);

    if (!refcount)
    {
        CloseHandle(stream->u.file.handle);
        heap_free(stream);
    }

    return refcount;
}

static HRESULT WINAPI filestream_Read(IStream *iface, void *buff, ULONG size, ULONG *read_len)
{
    struct shstream *stream = impl_from_IStream(iface);
    DWORD read = 0;

    TRACE("(%p, %p, %u, %p)\n", stream, buff, size, read_len);

    if (!ReadFile(stream->u.file.handle, buff, size, &read, NULL))
    {
        WARN("error %d reading file\n", GetLastError());
        return S_FALSE;
    }

    if (read_len)
        *read_len = read;

    return read == size ? S_OK : S_FALSE;
}

static HRESULT WINAPI filestream_Write(IStream *iface, const void *buff, ULONG size, ULONG *written)
{
    struct shstream *stream = impl_from_IStream(iface);
    DWORD written_len = 0;

    TRACE("(%p, %p, %u, %p)\n", stream, buff, size, written);

    switch (stream->u.file.mode & 0xf)
    {
        case STGM_WRITE:
        case STGM_READWRITE:
            break;
        default:
            return STG_E_ACCESSDENIED;
    }

    if (!WriteFile(stream->u.file.handle, buff, size, &written_len, NULL))
        return HRESULT_FROM_WIN32(GetLastError());

    if (written)
        *written = written_len;

    return S_OK;
}

static HRESULT WINAPI filestream_Seek(IStream *iface, LARGE_INTEGER move, DWORD origin, ULARGE_INTEGER *new_pos)
{
    struct shstream *stream = impl_from_IStream(iface);
    DWORD position;

    TRACE("(%p, %s, %d, %p)\n", stream, wine_dbgstr_longlong(move.QuadPart), origin, new_pos);

    position = SetFilePointer(stream->u.file.handle, move.u.LowPart, NULL, origin);
    if (position == INVALID_SET_FILE_POINTER)
        return HRESULT_FROM_WIN32(GetLastError());

    if (new_pos)
    {
        new_pos->u.HighPart = 0;
        new_pos->u.LowPart = position;
    }

    return S_OK;
}

static HRESULT WINAPI filestream_SetSize(IStream *iface, ULARGE_INTEGER size)
{
    struct shstream *stream = impl_from_IStream(iface);

    TRACE("(%p, %s)\n", stream, wine_dbgstr_longlong(size.QuadPart));

    if (!SetFilePointer(stream->u.file.handle, size.QuadPart, NULL, FILE_BEGIN))
        return E_FAIL;

    if (!SetEndOfFile(stream->u.file.handle))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI filestream_CopyTo(IStream *iface, IStream *dest, ULARGE_INTEGER size,
        ULARGE_INTEGER *read_len, ULARGE_INTEGER *written)
{
    struct shstream *stream = impl_from_IStream(iface);
    HRESULT hr = S_OK;
    char buff[1024];

    TRACE("(%p, %p, %s, %p, %p)\n", stream, dest, wine_dbgstr_longlong(size.QuadPart), read_len, written);

    if (read_len)
        read_len->QuadPart = 0;
    if (written)
        written->QuadPart = 0;

    if (!dest)
        return S_OK;

    while (size.QuadPart)
    {
        ULONG left, read_chunk, written_chunk;

        left = size.QuadPart > sizeof(buff) ? sizeof(buff) : size.QuadPart;

        /* Read */
        hr = IStream_Read(iface, buff, left, &read_chunk);
        if (FAILED(hr) || read_chunk == 0)
            break;
        if (read_len)
            read_len->QuadPart += read_chunk;

        /* Write */
        hr = IStream_Write(dest, buff, read_chunk, &written_chunk);
        if (written_chunk)
            written->QuadPart += written_chunk;
        if (FAILED(hr) || written_chunk != left)
            break;

        size.QuadPart -= left;
    }

    return hr;
}

static HRESULT WINAPI filestream_Stat(IStream *iface, STATSTG *statstg, DWORD flags)
{
    struct shstream *stream = impl_from_IStream(iface);
    BY_HANDLE_FILE_INFORMATION fi;

    TRACE("(%p, %p, %#x)\n", stream, statstg, flags);

    if (!statstg)
        return STG_E_INVALIDPOINTER;

    memset(&fi, 0, sizeof(fi));
    GetFileInformationByHandle(stream->u.file.handle, &fi);

    if (flags & STATFLAG_NONAME)
        statstg->pwcsName = NULL;
    else
    {
        int len = strlenW(stream->u.file.path);
        if ((statstg->pwcsName = CoTaskMemAlloc((len + 1) * sizeof(WCHAR))))
            memcpy(statstg->pwcsName, stream->u.file.path, (len + 1) * sizeof(WCHAR));
    }
    statstg->type = 0;
    statstg->cbSize.u.LowPart = fi.nFileSizeLow;
    statstg->cbSize.u.HighPart = fi.nFileSizeHigh;
    statstg->mtime = fi.ftLastWriteTime;
    statstg->ctime = fi.ftCreationTime;
    statstg->atime = fi.ftLastAccessTime;
    statstg->grfMode = stream->u.file.mode;
    statstg->grfLocksSupported = 0;
    memcpy(&statstg->clsid, &IID_IStream, sizeof(CLSID));
    statstg->grfStateBits = 0;
    statstg->reserved = 0;

    return S_OK;
}

static const IStreamVtbl filestreamvtbl =
{
    shstream_QueryInterface,
    shstream_AddRef,
    filestream_Release,
    filestream_Read,
    filestream_Write,
    filestream_Seek,
    filestream_SetSize,
    filestream_CopyTo,
    shstream_Commit,
    shstream_Revert,
    shstream_LockRegion,
    shstream_UnlockRegion,
    filestream_Stat,
    shstream_Clone,
};

/*************************************************************************
 * SHCreateStreamOnFileEx   [SHCORE.@]
 */
HRESULT WINAPI SHCreateStreamOnFileEx(const WCHAR *path, DWORD mode, DWORD attributes,
    BOOL create, IStream *template, IStream **ret)
{
    DWORD access, share, creation_disposition, len;
    struct shstream *stream;
    HANDLE hFile;

    TRACE("(%s, %d, 0x%08X, %d, %p, %p)\n", debugstr_w(path), mode, attributes,
        create, template, ret);

    if (!path || !ret || template)
        return E_INVALIDARG;

    *ret = NULL;

    /* Access */
    switch (mode & 0xf)
    {
        case STGM_WRITE:
        case STGM_READWRITE:
            access = GENERIC_READ | GENERIC_WRITE;
            break;
        case STGM_READ:
            access = GENERIC_READ;
            break;
        default:
            return E_INVALIDARG;
    }

    /* Sharing */
    switch (mode & 0xf0)
    {
        case 0:
        case STGM_SHARE_DENY_NONE:
            share = FILE_SHARE_READ | FILE_SHARE_WRITE;
            break;
        case STGM_SHARE_DENY_READ:
            share = FILE_SHARE_WRITE;
            break;
        case STGM_SHARE_DENY_WRITE:
            share = FILE_SHARE_READ;
            break;
        case STGM_SHARE_EXCLUSIVE:
            share = 0;
            break;
        default:
            return E_INVALIDARG;
    }

    switch (mode & 0xf000)
    {
        case STGM_FAILIFTHERE:
            creation_disposition = create ? CREATE_NEW : OPEN_EXISTING;
            break;
        case STGM_CREATE:
            creation_disposition = CREATE_ALWAYS;
            break;
        default:
            return E_INVALIDARG;
    }

    hFile = CreateFileW(path, access, share, NULL, creation_disposition, attributes, 0);
    if (hFile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    stream = heap_alloc(sizeof(*stream));
    stream->IStream_iface.lpVtbl = &filestreamvtbl;
    stream->refcount = 1;
    stream->u.file.handle = hFile;
    stream->u.file.mode = mode;

    len = strlenW(path);
    stream->u.file.path = heap_alloc((len + 1) * sizeof(WCHAR));
    memcpy(stream->u.file.path, path, (len + 1) * sizeof(WCHAR));

    *ret = &stream->IStream_iface;

    return S_OK;
}

/*************************************************************************
 * SHCreateStreamOnFileW   [SHCORE.@]
 */
HRESULT WINAPI SHCreateStreamOnFileW(const WCHAR *path, DWORD mode, IStream **stream)
{
    TRACE("(%s, %#x, %p)\n", debugstr_w(path), mode, stream);

    if (!path || !stream)
        return E_INVALIDARG;

    if ((mode & (STGM_CONVERT | STGM_DELETEONRELEASE | STGM_TRANSACTED)) != 0)
        return E_INVALIDARG;

    return SHCreateStreamOnFileEx(path, mode, 0, FALSE, NULL, stream);
}

/*************************************************************************
 * SHCreateStreamOnFileA   [SHCORE.@]
 */
HRESULT WINAPI SHCreateStreamOnFileA(const char *path, DWORD mode, IStream **stream)
{
    WCHAR *pathW;
    HRESULT hr;
    DWORD len;

    TRACE("(%s, %#x, %p)\n", debugstr_a(path), mode, stream);

    if (!path)
        return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);

    len = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
    pathW = heap_alloc(len * sizeof(WCHAR));
    if (!pathW)
        return E_OUTOFMEMORY;

    MultiByteToWideChar(CP_ACP, 0, path, -1, pathW, len);
    hr = SHCreateStreamOnFileW(pathW, mode, stream);
    heap_free(pathW);

    return hr;
}

struct threadref
{
    IUnknown IUnknown_iface;
    LONG *refcount;
};

static inline struct threadref *threadref_impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct threadref, IUnknown_iface);
}

static HRESULT WINAPI threadref_QueryInterface(IUnknown *iface, REFIID riid, void **out)
{
    struct threadref *threadref = threadref_impl_from_IUnknown(iface);

    TRACE("(%p, %s, %p)\n", threadref, debugstr_guid(riid), out);

    if (out == NULL)
        return E_POINTER;

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        *out = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    WARN("Interface %s not supported.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI threadref_AddRef(IUnknown *iface)
{
    struct threadref *threadref = threadref_impl_from_IUnknown(iface);
    LONG refcount = InterlockedIncrement(threadref->refcount);

    TRACE("(%p, %d)\n", threadref, refcount);

    return refcount;
}

static ULONG WINAPI threadref_Release(IUnknown *iface)
{
    struct threadref *threadref = threadref_impl_from_IUnknown(iface);
    LONG refcount = InterlockedDecrement(threadref->refcount);

    TRACE("(%p, %d)\n", threadref, refcount);

    if (!refcount)
        heap_free(threadref);

    return refcount;
}

static const IUnknownVtbl threadrefvtbl =
{
    threadref_QueryInterface,
    threadref_AddRef,
    threadref_Release,
};

/*************************************************************************
 * SHCreateThreadRef        [SHCORE.@]
 */
HRESULT WINAPI SHCreateThreadRef(LONG *refcount, IUnknown **out)
{
    struct threadref *threadref;

    TRACE("(%p, %p)\n", refcount, out);

    if (!refcount || !out)
        return E_INVALIDARG;

    *out = NULL;

    threadref = heap_alloc(sizeof(*threadref));
    if (!threadref)
        return E_OUTOFMEMORY;
    threadref->IUnknown_iface.lpVtbl = &threadrefvtbl;
    threadref->refcount = refcount;

    *refcount = 1;
    *out = &threadref->IUnknown_iface;

    TRACE("Created %p.\n", threadref);
    return S_OK;
}

/*************************************************************************
 * SHGetThreadRef        [SHCORE.@]
 */
HRESULT WINAPI SHGetThreadRef(IUnknown **out)
{
    TRACE("(%p)\n", out);

    if (shcore_tls == TLS_OUT_OF_INDEXES)
        return E_NOINTERFACE;

    *out = TlsGetValue(shcore_tls);
    if (!*out)
        return E_NOINTERFACE;

    IUnknown_AddRef(*out);
    return S_OK;
}

/*************************************************************************
 * SHSetThreadRef        [SHCORE.@]
 */
HRESULT WINAPI SHSetThreadRef(IUnknown *obj)
{
    TRACE("(%p)\n", obj);

    if (shcore_tls == TLS_OUT_OF_INDEXES)
        return E_NOINTERFACE;

    TlsSetValue(shcore_tls, obj);
    return S_OK;
}

/*************************************************************************
 * SHReleaseThreadRef        [SHCORE.@]
 */
HRESULT WINAPI SHReleaseThreadRef(void)
{
    FIXME("() - stub!\n");
    return S_OK;
}

/*************************************************************************
 * GetProcessReference        [SHCORE.@]
 */
HRESULT WINAPI GetProcessReference(IUnknown **obj)
{
    TRACE("(%p)\n", obj);

    *obj = process_ref;

    if (!process_ref)
        return E_FAIL;

    if (*obj)
        IUnknown_AddRef(*obj);

    return S_OK;
}

/*************************************************************************
 * SetProcessReference        [SHCORE.@]
 */
void WINAPI SetProcessReference(IUnknown *obj)
{
    TRACE("(%p)\n", obj);

    process_ref = obj;
}

struct thread_data
{
    LPTHREAD_START_ROUTINE thread_proc;
    LPTHREAD_START_ROUTINE callback;
    void *data;
    DWORD flags;
    HANDLE hEvent;
    IUnknown *thread_ref;
    IUnknown *process_ref;
};

static DWORD WINAPI shcore_thread_wrapper(void *data)
{
    struct thread_data thread_data;
    HRESULT hr = E_FAIL;
    DWORD retval;

    TRACE("(%p)\n", data);

    /* We are now executing in the context of the newly created thread.
     * So we copy the data passed to us (it is on the stack of the function
     * that called us, which is waiting for us to signal an event before
     * returning). */
    thread_data = *(struct thread_data *)data;

    if (thread_data.flags & CTF_COINIT)
    {
        hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (FAILED(hr))
            hr = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE);
    }

    if (thread_data.callback)
        thread_data.callback(thread_data.data);

    /* Signal the thread that created us; it can return now. */
    SetEvent(thread_data.hEvent);

    /* Execute the callers start code. */
    retval = thread_data.thread_proc(thread_data.data);

    /* Release thread and process references. */
    if (thread_data.thread_ref)
        IUnknown_Release(thread_data.thread_ref);

    if (thread_data.process_ref)
        IUnknown_Release(thread_data.process_ref);

    if (SUCCEEDED(hr))
        CoUninitialize();

    return retval;
}

/*************************************************************************
 *      SHCreateThread        [SHCORE.@]
 */
BOOL WINAPI SHCreateThread(LPTHREAD_START_ROUTINE thread_proc, void *data, DWORD flags, LPTHREAD_START_ROUTINE callback)
{
    struct thread_data thread_data;
    BOOL called = FALSE;

    TRACE("(%p, %p, %#x, %p)\n", thread_proc, data, flags, callback);

    thread_data.thread_proc = thread_proc;
    thread_data.callback = callback;
    thread_data.data = data;
    thread_data.flags = flags;
    thread_data.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    if (flags & CTF_THREAD_REF)
        SHGetThreadRef(&thread_data.thread_ref);
    else
        thread_data.thread_ref = NULL;

    if (flags & CTF_PROCESS_REF)
        GetProcessReference(&thread_data.process_ref);
    else
        thread_data.process_ref = NULL;

    /* Create the thread */
    if (thread_data.hEvent)
    {
        HANDLE hThread;
        DWORD retval;

        hThread = CreateThread(NULL, 0, shcore_thread_wrapper, &thread_data, 0, &retval);
        if (hThread)
        {
            /* Wait for the thread to signal us to continue */
            WaitForSingleObject(thread_data.hEvent, INFINITE);
            CloseHandle(hThread);
            called = TRUE;
        }
        CloseHandle(thread_data.hEvent);
    }

    if (!called)
    {
        if (!thread_data.callback && flags & CTF_INSIST)
        {
            /* Couldn't call, call synchronously */
            thread_data.thread_proc(data);
            called = TRUE;
        }
        else
        {
            if (thread_data.thread_ref)
                IUnknown_Release(thread_data.thread_ref);

            if (thread_data.process_ref)
                IUnknown_Release(thread_data.process_ref);
        }
    }

    return called;
}
