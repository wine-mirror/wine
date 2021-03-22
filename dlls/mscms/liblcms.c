/*
 * Unix interface for liblcms2
 *
 * Copyright 2020 Alexandre Julliard
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

#ifdef HAVE_LCMS2

#include <stdarg.h>
#include <lcms2.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"
#include "mscms_priv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mscms);

static DWORD from_bmformat( BMFORMAT format )
{
    static BOOL quietfixme = FALSE;
    DWORD ret;

    switch (format)
    {
    case BM_RGBTRIPLETS: ret = TYPE_RGB_8; break;
    case BM_BGRTRIPLETS: ret = TYPE_BGR_8; break;
    case BM_GRAY:        ret = TYPE_GRAY_8; break;
    case BM_xRGBQUADS:   ret = TYPE_ARGB_8; break;
    case BM_xBGRQUADS:   ret = TYPE_ABGR_8; break;
    case BM_KYMCQUADS:   ret = TYPE_KYMC_8; break;
    default:
        if (!quietfixme)
        {
            FIXME( "unhandled bitmap format %08x\n", format );
            quietfixme = TRUE;
        }
        ret = TYPE_RGB_8;
        break;
    }
    TRACE( "color space: %08x -> %08x\n", format, ret );
    return ret;
}

static DWORD from_type( COLORTYPE type )
{
    DWORD ret;

    switch (type)
    {
    case COLOR_GRAY: ret = TYPE_GRAY_16; break;
    case COLOR_RGB:  ret = TYPE_RGB_16; break;
    case COLOR_XYZ:  ret = TYPE_XYZ_16; break;
    case COLOR_Yxy:  ret = TYPE_Yxy_16; break;
    case COLOR_Lab:  ret = TYPE_Lab_16; break;
    case COLOR_CMYK: ret = TYPE_CMYK_16; break;
    default:
        FIXME( "unhandled color type %08x\n", type );
        ret = TYPE_RGB_16;
        break;
    }

    TRACE( "color type: %08x -> %08x\n", type, ret );
    return ret;
}

static void * CDECL lcms_open_profile( void *data, DWORD size )
{
    return cmsOpenProfileFromMem( data, size );
}

static void CDECL lcms_close_profile( void *profile )
{
    cmsCloseProfile( profile );
}

static void * CDECL lcms_create_transform( void *output, void *target, DWORD intent )
{
    DWORD proofing = 0;
    cmsHPROFILE input = cmsCreate_sRGBProfile(); /* FIXME: create from supplied color space */

    if (target) proofing = cmsFLAGS_SOFTPROOFING;
    return cmsCreateProofingTransform( input, 0, output, 0, target,
                                       intent, INTENT_ABSOLUTE_COLORIMETRIC, proofing );
}

static void * CDECL lcms_create_multi_transform( void *profiles, DWORD count, DWORD intent )
{
    return cmsCreateMultiprofileTransform( profiles, count, 0, 0, intent, 0 );
}

static BOOL CDECL lcms_translate_bits( void *transform, void *srcbits, BMFORMAT input,
                                       void *dstbits, BMFORMAT output, DWORD size )
{
    if (!cmsChangeBuffersFormat( transform, from_bmformat(input), from_bmformat(output) )) return FALSE;
    cmsDoTransform( transform, srcbits, dstbits, size );
    return TRUE;
}

static BOOL CDECL lcms_translate_colors( void *transform, COLOR *in, DWORD count, COLORTYPE input_type,
                                         COLOR *out, COLORTYPE output_type )
{
    unsigned int i;

    if (!cmsChangeBuffersFormat( transform, from_type(input_type), from_type(output_type) )) return FALSE;

    for (i = 0; i < count; i++) cmsDoTransform( transform, &in[i], &out[i], 1 );
    return TRUE;
}

static void CDECL lcms_close_transform( void *transform )
{
    cmsDeleteTransform( transform );
}

static const struct lcms_funcs funcs =
{
    lcms_open_profile,
    lcms_close_profile,
    lcms_create_transform,
    lcms_create_multi_transform,
    lcms_translate_bits,
    lcms_translate_colors,
    lcms_close_transform
};

static void lcms_error_handler(cmsContext ctx, cmsUInt32Number error, const char *text)
{
    TRACE("%u %s\n", error, debugstr_a(text));
}

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;
    cmsSetLogErrorHandler( lcms_error_handler );
    *(const struct lcms_funcs **)ptr_out = &funcs;
    return STATUS_SUCCESS;
}

#endif /* HAVE_LCMS2 */
