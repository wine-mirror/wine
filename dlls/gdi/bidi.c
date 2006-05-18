
/*
 * GDI BiDirectional handling
 *
 * Copyright 2003 Shachar Shemesh
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
#ifdef HAVE_UNICODE_UBIDI_H
#include <unicode/ubidi.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/debug.h"
#include "gdi.h"
#include "gdi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(bidi);

#ifdef HAVE_ICU
BOOL BidiAvail = TRUE;
#else
BOOL BidiAvail = FALSE;
#endif


/*************************************************************
 *    BIDI_Reorder
 */
BOOL BIDI_Reorder(
                LPCWSTR lpString,       /* [in] The string for which information is to be returned */
                INT uCount,     /* [in] Number of WCHARs in string. */
                DWORD dwFlags,  /* [in] GetCharacterPlacement compatible flags specifying how to process the string */
                DWORD dwWineGCP_Flags,       /* [in] Wine internal flags - Force paragraph direction */
                LPWSTR lpOutString, /* [out] Reordered string */
                INT uCountOut,  /* [in] Size of output buffer */
                UINT *lpOrder /* [out] Logical -> Visual order map */
    )
{
#ifdef HAVE_ICU
    TRACE("%s, %d, 0x%08lx lpOutString=%p, lpOrder=%p\n",
          debugstr_wn(lpString, uCount), uCount, dwFlags,
          lpOutString, lpOrder);

    if ((dwFlags & GCP_REORDER) != 0) {
        UBiDi *bidi;
        UErrorCode err=0;
        UBiDiLevel level=0;

        bidi=ubidi_open();
        if( bidi==NULL ) {
            WARN("Failed to allocate structure\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        switch( dwWineGCP_Flags&WINE_GCPW_DIR_MASK )
        {
        case WINE_GCPW_FORCE_LTR:
            level=0;
            break;
        case WINE_GCPW_FORCE_RTL:
            level=1;
            break;
        case WINE_GCPW_LOOSE_LTR:
            level=UBIDI_DEFAULT_LTR;
            break;
        case WINE_GCPW_LOOSE_RTL:
            level=UBIDI_DEFAULT_RTL;
            break;
        }

        ubidi_setPara( bidi, lpString, uCount, level, NULL, &err );
        if( lpOutString!=NULL ) {
            ubidi_writeReordered( bidi, lpOutString, uCount,
                    (dwFlags&GCP_SYMSWAPOFF)?0:UBIDI_DO_MIRRORING, &err );
        }

        if( lpOrder!=NULL ) {
            ubidi_getLogicalMap( bidi, lpOrder, &err );
        }

        ubidi_close( bidi );

        if( U_FAILURE(err) ) {
            FIXME("ICU Library return error code %d.\n", err );
            FIXME("Please report this error to wine-devel@winehq.org so we can place "
                    "descriptive Windows error codes here\n");
            SetLastError(ERROR_INVALID_LEVEL); /* This error is cryptic enough not to mean anything, I hope */

            return FALSE;
        }
    }
    return TRUE;
#else  /* HAVE_ICU */
    return FALSE;
#endif  /* HAVE_ICU */
}
