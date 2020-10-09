/*
 * Copyright 2020 Esme Povirk
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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#define NONAMELESSUNION

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#ifdef SONAME_LIBPNG

#include <png.h>

static void *libpng_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(png_create_info_struct);
MAKE_FUNCPTR(png_create_read_struct);
MAKE_FUNCPTR(png_destroy_read_struct);
MAKE_FUNCPTR(png_error);
MAKE_FUNCPTR(png_get_error_ptr);
MAKE_FUNCPTR(png_get_io_ptr);
MAKE_FUNCPTR(png_read_info);
MAKE_FUNCPTR(png_set_crc_action);
MAKE_FUNCPTR(png_set_error_fn);
MAKE_FUNCPTR(png_set_read_fn);
#undef MAKE_FUNCPTR

static CRITICAL_SECTION init_png_cs;
static CRITICAL_SECTION_DEBUG init_png_cs_debug =
{
    0, 0, &init_png_cs,
    { &init_png_cs_debug.ProcessLocksList,
      &init_png_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": init_png_cs") }
};
static CRITICAL_SECTION init_png_cs = { &init_png_cs_debug, -1, 0, 0, 0, 0 };

static void *load_libpng(void)
{
    void *result;

    RtlEnterCriticalSection(&init_png_cs);

    if(!libpng_handle && (libpng_handle = dlopen(SONAME_LIBPNG, RTLD_NOW)) != NULL) {

#define LOAD_FUNCPTR(f) \
    if((p##f = dlsym(libpng_handle, #f)) == NULL) { \
        libpng_handle = NULL; \
        RtlLeaveCriticalSection(&init_png_cs); \
        return NULL; \
    }
        LOAD_FUNCPTR(png_create_info_struct);
        LOAD_FUNCPTR(png_create_read_struct);
        LOAD_FUNCPTR(png_destroy_read_struct);
        LOAD_FUNCPTR(png_error);
        LOAD_FUNCPTR(png_get_error_ptr);
        LOAD_FUNCPTR(png_get_io_ptr);
        LOAD_FUNCPTR(png_read_info);
        LOAD_FUNCPTR(png_set_crc_action);
        LOAD_FUNCPTR(png_set_error_fn);
        LOAD_FUNCPTR(png_set_read_fn);

#undef LOAD_FUNCPTR
    }

    result = libpng_handle;

    RtlLeaveCriticalSection(&init_png_cs);

    return result;
}

struct png_decoder
{
    struct decoder decoder;
};

static inline struct png_decoder *impl_from_decoder(struct decoder* iface)
{
    return CONTAINING_RECORD(iface, struct png_decoder, decoder);
}

static void user_error_fn(png_structp png_ptr, png_const_charp error_message)
{
    jmp_buf *pjmpbuf;

    /* This uses setjmp/longjmp just like the default. We can't use the
     * default because there's no way to access the jmp buffer in the png_struct
     * that works in 1.2 and 1.4 and allows us to dynamically load libpng. */
    WARN("PNG error: %s\n", debugstr_a(error_message));
    pjmpbuf = ppng_get_error_ptr(png_ptr);
    longjmp(*pjmpbuf, 1);
}

static void user_warning_fn(png_structp png_ptr, png_const_charp warning_message)
{
    WARN("PNG warning: %s\n", debugstr_a(warning_message));
}

static void user_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    IStream *stream = ppng_get_io_ptr(png_ptr);
    HRESULT hr;
    ULONG bytesread;

    hr = stream_read(stream, data, length, &bytesread);
    if (FAILED(hr) || bytesread != length)
    {
        ppng_error(png_ptr, "failed reading data");
    }
}

HRESULT CDECL png_decoder_initialize(struct decoder *iface, IStream *stream, struct decoder_stat *st)
{
    png_structp png_ptr;
    png_infop info_ptr;
    png_infop end_info;
    jmp_buf jmpbuf;
    HRESULT hr = E_FAIL;

    png_ptr = ppng_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
    {
        return E_FAIL;
    }

    info_ptr = ppng_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        ppng_destroy_read_struct(&png_ptr, NULL, NULL);
        return E_FAIL;
    }

    end_info = ppng_create_info_struct(png_ptr);
    if (!end_info)
    {
        ppng_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return E_FAIL;
    }

    /* set up setjmp/longjmp error handling */
    if (setjmp(jmpbuf))
    {
        hr = WINCODEC_ERR_UNKNOWNIMAGEFORMAT;
        goto end;
    }
    ppng_set_error_fn(png_ptr, jmpbuf, user_error_fn, user_warning_fn);
    ppng_set_crc_action(png_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);

    /* seek to the start of the stream */
    hr = stream_seek(stream, 0, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
    {
        goto end;
    }

    /* set up custom i/o handling */
    ppng_set_read_fn(png_ptr, stream, user_read_data);

    /* read the header */
    ppng_read_info(png_ptr, info_ptr);

    st->flags = WICBitmapDecoderCapabilityCanDecodeAllImages |
                WICBitmapDecoderCapabilityCanDecodeSomeImages |
                WICBitmapDecoderCapabilityCanEnumerateMetadata;
    st->frame_count = 1;
    hr = S_OK;

end:
    ppng_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    return hr;
}

void CDECL png_decoder_destroy(struct decoder* iface)
{
    struct png_decoder *This = impl_from_decoder(iface);

    RtlFreeHeap(GetProcessHeap(), 0, This);
}

static const struct decoder_funcs png_decoder_vtable = {
    png_decoder_initialize,
    png_decoder_destroy
};

HRESULT CDECL png_decoder_create(struct decoder_info *info, struct decoder **result)
{
    struct png_decoder *This;

    if (!load_libpng())
    {
        ERR("Failed reading PNG because unable to find %s\n",SONAME_LIBPNG);
        return E_FAIL;
    }

    This = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(*This));

    if (!This)
    {
        return E_OUTOFMEMORY;
    }

    This->decoder.vtable = &png_decoder_vtable;
    *result = &This->decoder;

    info->container_format = GUID_ContainerFormatPng;
    info->block_format = GUID_ContainerFormatPng;
    info->clsid = CLSID_WICPngDecoder;

    return S_OK;
}

#else

HRESULT CDECL png_decoder_create(struct decoder_info *info, struct decoder **result)
{
    ERR("Trying to load PNG picture, but PNG support is not compiled in.\n");
    return E_FAIL;
}

#endif
