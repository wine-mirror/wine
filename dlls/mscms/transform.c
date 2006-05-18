/*
 * MSCMS - Color Management System for Wine
 *
 * Copyright 2005 Hans Leidekker
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

HTRANSFORM WINAPI CreateColorTransformA( LPLOGCOLORSPACEA space, HPROFILE dest,
    HPROFILE target, DWORD flags )
{
    LOGCOLORSPACEW spaceW;
    DWORD len;

    TRACE( "( %p, %p, %p, 0x%08lx )\n", space, dest, target, flags );

    if (!space || !dest) return FALSE;

    memcpy( &spaceW, space, FIELD_OFFSET(LOGCOLORSPACEA, lcsFilename) );
    spaceW.lcsSize = sizeof(LOGCOLORSPACEW);

    len = MultiByteToWideChar( CP_ACP, 0, space->lcsFilename, -1, NULL, 0 );
    MultiByteToWideChar( CP_ACP, 0, space->lcsFilename, -1, spaceW.lcsFilename, len );

    return CreateColorTransformW( &spaceW, dest, target, flags );
}

HTRANSFORM WINAPI CreateColorTransformW( LPLOGCOLORSPACEW space, HPROFILE dest,
    HPROFILE target, DWORD flags )
{
    HTRANSFORM ret = NULL;
#ifdef HAVE_LCMS
    cmsHTRANSFORM cmstransform;
    cmsHPROFILE cmsprofiles[3];
    int intent;

    TRACE( "( %p, %p, %p, 0x%08lx )\n", space, dest, target, flags );

    if (!space || !dest) return FALSE;

    intent = space->lcsIntent > 3 ? INTENT_PERCEPTUAL : space->lcsIntent;

    cmsprofiles[0] = cmsCreate_sRGBProfile(); /* FIXME: create from supplied color space */
    cmsprofiles[1] = MSCMS_hprofile2cmsprofile( dest ); 

    if (target)
    {
        cmsprofiles[2] = MSCMS_hprofile2cmsprofile( target );
        cmstransform = cmsCreateMultiprofileTransform( cmsprofiles, 3, TYPE_BGR_8,
                                                       TYPE_BGR_8, intent, 0 );
    }
    else
    {
        cmstransform = cmsCreateTransform( cmsprofiles[0], TYPE_BGR_8, cmsprofiles[1],
                                           TYPE_BGR_8, intent, 0 );
    }
    ret = MSCMS_create_htransform_handle( cmstransform );

#endif /* HAVE_LCMS */
    return ret;
}

HTRANSFORM WINAPI CreateMultiProfileTransform( PHPROFILE profiles, DWORD nprofiles,
    PDWORD intents, DWORD nintents, DWORD flags, DWORD cmm )
{
    HTRANSFORM ret = NULL;
#ifdef HAVE_LCMS
    cmsHPROFILE *cmsprofiles;
    cmsHTRANSFORM cmstransform;
    DWORD i;

    TRACE( "( %p, 0x%08lx, %p, 0x%08lx, 0x%08lx, 0x%08lx ) stub\n",
           profiles, nprofiles, intents, nintents, flags, cmm );

    if (!profiles || !intents) return NULL;

    cmsprofiles = HeapAlloc( GetProcessHeap(), 0, nprofiles * sizeof(cmsHPROFILE) );

    if (cmsprofiles)
    {
        for (i = 0; i < nprofiles; i++)
            cmsprofiles[i] = MSCMS_hprofile2cmsprofile( profiles[i] );
    }

    cmstransform = cmsCreateMultiprofileTransform( cmsprofiles, nprofiles, TYPE_BGR_8,
                                                   TYPE_BGR_8, *intents, 0 );
    HeapFree( GetProcessHeap(), 0, cmsprofiles );
    ret = MSCMS_create_htransform_handle( cmstransform );

#endif /* HAVE_LCMS */
    return ret;
}

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

BOOL WINAPI TranslateBitmapBits( HTRANSFORM transform, PVOID srcbits, BMFORMAT input,
    DWORD width, DWORD height, DWORD inputstride, PVOID destbits, BMFORMAT output,
    DWORD outputstride, PBMCALLBACKFN callback, ULONG data )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS
    cmsHTRANSFORM cmstransform;

    TRACE( "( %p, %p, 0x%08x, 0x%08lx, 0x%08lx, 0x%08lx, %p, 0x%08x, 0x%08lx, %p, 0x%08lx )\n",
           transform, srcbits, input, width, height, inputstride, destbits, output,
           outputstride, callback, data );

    cmstransform = MSCMS_htransform2cmstransform( transform );

    cmsDoTransform( cmstransform, srcbits, destbits, width * height );
    ret = TRUE;

#endif /* HAVE_LCMS */
    return ret;
}
