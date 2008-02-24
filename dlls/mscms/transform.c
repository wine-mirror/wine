/*
 * MSCMS - Color Management System for Wine
 *
 * Copyright 2005, 2006, 2008 Hans Leidekker
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
#include "wine/debug.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"

#include "mscms_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL(mscms);

/******************************************************************************
 * CreateColorTransformA            [MSCMS.@]
 *
 * See CreateColorTransformW.
 */
HTRANSFORM WINAPI CreateColorTransformA( LPLOGCOLORSPACEA space, HPROFILE dest,
    HPROFILE target, DWORD flags )
{
    LOGCOLORSPACEW spaceW;
    DWORD len;

    TRACE( "( %p, %p, %p, 0x%08x )\n", space, dest, target, flags );

    if (!space || !dest) return FALSE;

    memcpy( &spaceW, space, FIELD_OFFSET(LOGCOLORSPACEA, lcsFilename) );
    spaceW.lcsSize = sizeof(LOGCOLORSPACEW);

    len = MultiByteToWideChar( CP_ACP, 0, space->lcsFilename, -1, NULL, 0 );
    MultiByteToWideChar( CP_ACP, 0, space->lcsFilename, -1, spaceW.lcsFilename, len );

    return CreateColorTransformW( &spaceW, dest, target, flags );
}

static DWORD from_profile( HPROFILE profile )
{
    PROFILEHEADER header;

    GetColorProfileHeader( profile, &header );
    TRACE( "color space: 0x%08x %s\n", header.phDataColorSpace, MSCMS_dbgstr_tag( header.phDataColorSpace ) );

    switch (header.phDataColorSpace)
    {
    case 0x434d594b: return TYPE_CMYK_16;  /* 'CMYK' */
    case 0x47524159: return TYPE_GRAY_16;  /* 'GRAY' */
    case 0x4c616220: return TYPE_Lab_16;   /* 'Lab ' */
    case 0x52474220: return TYPE_RGB_16;   /* 'RGB ' */
    case 0x58595a20: return TYPE_XYZ_16;   /* 'XYZ ' */
    default:
    {
        WARN("unhandled format\n");
        return TYPE_RGB_16;
    }
    }
}

/******************************************************************************
 * CreateColorTransformW            [MSCMS.@]
 *
 * Create a color transform.
 *
 * PARAMS
 *  space  [I] Input color space.
 *  dest   [I] Color profile of destination device.
 *  target [I] Color profile of target device.
 *  flags  [I] Flags.
 *
 * RETURNS
 *  Success: Handle to a transform.
 *  Failure: NULL
 */
HTRANSFORM WINAPI CreateColorTransformW( LPLOGCOLORSPACEW space, HPROFILE dest,
    HPROFILE target, DWORD flags )
{
    HTRANSFORM ret = NULL;
#ifdef HAVE_LCMS
    cmsHTRANSFORM cmstransform;
    cmsHPROFILE cmsinput, cmsoutput, cmstarget = NULL;
    DWORD in_format, out_format, proofing = 0;
    int intent;

    TRACE( "( %p, %p, %p, 0x%08x )\n", space, dest, target, flags );

    if (!space || !dest) return FALSE;

    intent = space->lcsIntent > 3 ? INTENT_PERCEPTUAL : space->lcsIntent;

    TRACE( "lcsIntent:   %x\n", space->lcsIntent );
    TRACE( "lcsCSType:   %s\n", MSCMS_dbgstr_tag( space->lcsCSType ) );
    TRACE( "lcsFilename: %s\n", debugstr_w( space->lcsFilename ) );

    in_format  = TYPE_RGB_16;
    out_format = from_profile( dest );

    cmsinput = cmsCreate_sRGBProfile(); /* FIXME: create from supplied color space */
    if (target)
    {
        proofing = cmsFLAGS_SOFTPROOFING;
        cmstarget = MSCMS_hprofile2cmsprofile( target );
    }
    cmsoutput = MSCMS_hprofile2cmsprofile( dest );
    cmstransform = cmsCreateProofingTransform(cmsinput, in_format, cmsoutput, out_format, cmstarget,
                                              intent, INTENT_ABSOLUTE_COLORIMETRIC, proofing);

    ret = MSCMS_create_htransform_handle( cmstransform );

#endif /* HAVE_LCMS */
    return ret;
}

/******************************************************************************
 * CreateMultiProfileTransform      [MSCMS.@]
 *
 * Create a color transform from an array of color profiles.
 *
 * PARAMS
 *  profiles  [I] Array of color profiles.
 *  nprofiles [I] Number of color profiles.
 *  intents   [I] Array of rendering intents.
 *  flags     [I] Flags.
 *  cmm       [I] Profile to take the CMM from.
 *
 * RETURNS
 *  Success: Handle to a transform.
 *  Failure: NULL
 */ 
HTRANSFORM WINAPI CreateMultiProfileTransform( PHPROFILE profiles, DWORD nprofiles,
    PDWORD intents, DWORD nintents, DWORD flags, DWORD cmm )
{
    HTRANSFORM ret = NULL;
#ifdef HAVE_LCMS
    cmsHPROFILE *cmsprofiles, cmsconvert = NULL;
    cmsHTRANSFORM cmstransform;
    DWORD in_format, out_format;

    TRACE( "( %p, 0x%08x, %p, 0x%08x, 0x%08x, 0x%08x )\n",
           profiles, nprofiles, intents, nintents, flags, cmm );

    if (!profiles || !nprofiles || !intents) return NULL;

    if (nprofiles > 2)
    {
        FIXME("more than 2 profiles not supported\n");
        return NULL;
    }

    in_format  = from_profile( profiles[0] );
    out_format = from_profile( profiles[nprofiles - 1] );

    if (in_format != out_format)
    {
        /* insert a conversion profile for pairings that lcms doesn't handle */
        if (out_format == TYPE_RGB_16) cmsconvert = cmsCreate_sRGBProfile();
        if (out_format == TYPE_Lab_16) cmsconvert = cmsCreateLabProfile( NULL );
    }

    cmsprofiles = HeapAlloc( GetProcessHeap(), 0, (nprofiles + 1) * sizeof(cmsHPROFILE *) );
    if (cmsprofiles)
    {
        cmsprofiles[0] = MSCMS_hprofile2cmsprofile( profiles[0] );
        if (cmsconvert)
        {
            cmsprofiles[1] = cmsconvert;
            cmsprofiles[2] = MSCMS_hprofile2cmsprofile( profiles[1] );
            nprofiles++;
        }
        else
        {
            cmsprofiles[1] = MSCMS_hprofile2cmsprofile( profiles[1] );
        }
        cmstransform = cmsCreateMultiprofileTransform( cmsprofiles, nprofiles, in_format, out_format, *intents, 0 );

        HeapFree( GetProcessHeap(), 0, cmsprofiles );
        ret = MSCMS_create_htransform_handle( cmstransform );
    }

#endif /* HAVE_LCMS */
    return ret;
}

/******************************************************************************
 * DeleteColorTransform             [MSCMS.@]
 *
 * Delete a color transform.
 *
 * PARAMS
 *  transform [I] Handle to a color transform.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */ 
BOOL WINAPI DeleteColorTransform( HTRANSFORM transform )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS
    cmsHTRANSFORM cmstransform;

    TRACE( "( %p )\n", transform );

    cmstransform = MSCMS_htransform2cmstransform( transform );
    cmsDeleteTransform( cmstransform );

    MSCMS_destroy_htransform_handle( transform );
    ret = TRUE;

#endif /* HAVE_LCMS */
    return ret;
}

static DWORD from_bmformat( BMFORMAT format )
{
    TRACE( "bitmap format: 0x%08x\n", format );

    switch (format)
    {
    case BM_RGBTRIPLETS: return TYPE_RGB_8;
    case BM_BGRTRIPLETS: return TYPE_BGR_8;
    case BM_GRAY:        return TYPE_GRAY_8;
    default:
    {
        FIXME("unhandled bitmap format\n");
        return TYPE_RGB_8;
    }
    }
}

/******************************************************************************
 * TranslateBitmapBits              [MSCMS.@]
 *
 * Perform color translation.
 *
 * PARAMS
 *  transform    [I] Handle to a color transform.
 *  srcbits      [I] Source bitmap.
 *  input        [I] Format of the source bitmap.
 *  width        [I] Width of the source bitmap.
 *  height       [I] Height of the source bitmap.
 *  inputstride  [I] Number of bytes in one scanline.
 *  destbits     [I] Destination bitmap.
 *  output       [I] Format of the destination bitmap.
 *  outputstride [I] Number of bytes in one scanline. 
 *  callback     [I] Callback function.
 *  data         [I] Callback data. 
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI TranslateBitmapBits( HTRANSFORM transform, PVOID srcbits, BMFORMAT input,
    DWORD width, DWORD height, DWORD inputstride, PVOID destbits, BMFORMAT output,
    DWORD outputstride, PBMCALLBACKFN callback, ULONG data )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS
    cmsHTRANSFORM cmstransform;

    TRACE( "( %p, %p, 0x%08x, 0x%08x, 0x%08x, 0x%08x, %p, 0x%08x, 0x%08x, %p, 0x%08x )\n",
           transform, srcbits, input, width, height, inputstride, destbits, output,
           outputstride, callback, data );

    cmstransform = MSCMS_htransform2cmstransform( transform );
    cmsChangeBuffersFormat( cmstransform, from_bmformat(input), from_bmformat(output) );

    cmsDoTransform( cmstransform, srcbits, destbits, width * height );
    ret = TRUE;

#endif /* HAVE_LCMS */
    return ret;
}

static DWORD from_type( COLORTYPE type )
{
    TRACE( "color type: 0x%08x\n", type );

    switch (type)
    {
    case COLOR_GRAY:    return TYPE_GRAY_16;
    case COLOR_RGB:     return TYPE_RGB_16;
    case COLOR_XYZ:     return TYPE_XYZ_16;
    case COLOR_Yxy:     return TYPE_Yxy_16;
    case COLOR_Lab:     return TYPE_Lab_16;
    case COLOR_CMYK:    return TYPE_CMYK_16;
    default:
    {
        FIXME("unhandled color type\n");
        return TYPE_RGB_16;
    }
    }
}

/******************************************************************************
 * TranslateColors              [MSCMS.@]
 *
 * Perform color translation.
 *
 * PARAMS
 *  transform    [I] Handle to a color transform.
 *  input        [I] Array of input colors.
 *  number       [I] Number of colors to translate.
 *  input_type   [I] Input color format.
 *  output       [O] Array of output colors.
 *  output_type  [I] Output color format.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI TranslateColors( HTRANSFORM transform, PCOLOR in, DWORD count,
                             COLORTYPE input_type, PCOLOR out, COLORTYPE output_type )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS
    cmsHTRANSFORM xfrm = MSCMS_htransform2cmstransform( transform );
    unsigned int i;

    TRACE( "( %p, %p, %d, %d, %p, %d )\n", transform, in, count, input_type, out, output_type );

    cmsChangeBuffersFormat( xfrm, from_type(input_type), from_type(output_type) );

    switch (input_type)
    {
    case COLOR_RGB:
    {
        switch (output_type)
        {
        case COLOR_RGB:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].rgb, &out[i].rgb, 1 ); return TRUE;
        case COLOR_Lab:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].rgb, &out[i].Lab, 1 ); return TRUE;
        case COLOR_GRAY: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].rgb, &out[i].gray, 1 ); return TRUE;
        case COLOR_CMYK: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].rgb, &out[i].cmyk, 1 ); return TRUE;
        case COLOR_XYZ:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].rgb, &out[i].XYZ, 1 ); return TRUE;
        default:
            FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
            return FALSE;
        }
    }
    case COLOR_Lab:
    {
        switch (output_type)
        {
        case COLOR_RGB:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].Lab, &out[i].rgb, 1 ); return TRUE;
        case COLOR_Lab:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].Lab, &out[i].Lab, 1 ); return TRUE;
        case COLOR_GRAY: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].Lab, &out[i].gray, 1 ); return TRUE;
        case COLOR_CMYK: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].Lab, &out[i].cmyk, 1 ); return TRUE;
        case COLOR_XYZ:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].Lab, &out[i].XYZ, 1 ); return TRUE;
        default:
            FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
            return FALSE;
        }
    }
    case COLOR_GRAY:
    {
        switch (output_type)
        {
        case COLOR_RGB:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].gray, &out[i].rgb, 1 ); return TRUE;
        case COLOR_Lab:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].gray, &out[i].Lab, 1 ); return TRUE;
        case COLOR_GRAY: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].gray, &out[i].gray, 1 ); return TRUE;
        case COLOR_CMYK: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].gray, &out[i].cmyk, 1 ); return TRUE;
        case COLOR_XYZ:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].gray, &out[i].XYZ, 1 ); return TRUE;
        default:
            FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
            return FALSE;
        }
    }
    case COLOR_CMYK:
    {
        switch (output_type)
        {
        case COLOR_RGB:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].cmyk, &out[i].rgb, 1 ); return TRUE;
        case COLOR_Lab:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].cmyk, &out[i].Lab, 1 ); return TRUE;
        case COLOR_GRAY: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].cmyk, &out[i].gray, 1 ); return TRUE;
        case COLOR_CMYK: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].cmyk, &out[i].cmyk, 1 ); return TRUE;
        case COLOR_XYZ:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].cmyk, &out[i].XYZ, 1 ); return TRUE;
        default:
            FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
            return FALSE;
        }
    }
    case COLOR_XYZ:
    {
        switch (output_type)
        {
        case COLOR_RGB:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].XYZ, &out[i].rgb, 1 ); return TRUE;
        case COLOR_Lab:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].XYZ, &out[i].Lab, 1 ); return TRUE;
        case COLOR_GRAY: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].XYZ, &out[i].gray, 1 ); return TRUE;
        case COLOR_CMYK: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].XYZ, &out[i].cmyk, 1 ); return TRUE;
        case COLOR_XYZ:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].XYZ, &out[i].XYZ, 1 ); return TRUE;
        default:
            FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
            return FALSE;
        }
    }
    default:
        FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
        break;
    }

#endif /* HAVE_LCMS */
    return ret;
}
