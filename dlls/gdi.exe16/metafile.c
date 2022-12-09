/*
 * Metafile functions
 *
 * Copyright  David W. Metcalfe, 1994
 * Copyright  Niels de Carpentier, 1996
 * Copyright  Albrecht Kleine, 1996
 * Copyright  Huw Davies, 1996
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

#include <string.h>
#include <fcntl.h>

#include "wine/winbase16.h"
#include "wine/wingdi16.h"
#include "wownt32.h"
#include "winreg.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(metafile);

#define METAFILE_MEMORY 1
#define METAFILE_DISK   2
#define MFHEADERSIZE (sizeof(METAHEADER))
#define MFVERSION 0x300

/******************************************************************
 *         MF_GetMetaHeader16
 *
 * Returns ptr to METAHEADER associated with HMETAFILE16
 * Should be followed by call to MF_ReleaseMetaHeader16
 */
static METAHEADER *MF_GetMetaHeader16( HMETAFILE16 hmf )
{
    return GlobalLock16(hmf);
}

/******************************************************************
 *         MF_ReleaseMetaHeader16
 *
 * Releases METAHEADER associated with HMETAFILE16
 */
static BOOL16 MF_ReleaseMetaHeader16( HMETAFILE16 hmf )
{
    return GlobalUnlock16( hmf );
}

/******************************************************************
 *         create_metafile16
 *
 * Create a 16-bit metafile from a 32-bit one. The 32-bit one is deleted.
 */
static HMETAFILE16 create_metafile16( HMETAFILE hmf )
{
    UINT size;
    HMETAFILE16 hmf16;

    if (!hmf) return 0;
    size = GetMetaFileBitsEx( hmf, 0, NULL );
    hmf16 = GlobalAlloc16( GMEM_MOVEABLE, size );
    if (hmf16)
    {
        void *buffer = GlobalLock16( hmf16 );
        GetMetaFileBitsEx( hmf, size, buffer );
        GlobalUnlock16( hmf16 );
    }
    DeleteMetaFile( hmf );
    return hmf16;
}

/******************************************************************
 *         create_metafile32
 *
 * Create a 32-bit metafile from a 16-bit one.
 */
static HMETAFILE create_metafile32( HMETAFILE16 hmf16 )
{
    METAHEADER *mh = MF_GetMetaHeader16( hmf16 );
    if (!mh) return 0;
    return SetMetaFileBitsEx( mh->mtSize * 2, (BYTE *)mh );
}

/**********************************************************************
 *	     CreateMetaFile     (GDI.125)
 */
HDC16 WINAPI CreateMetaFile16( LPCSTR filename )
{
    return HDC_16( CreateMetaFileA( filename ) );
}

/******************************************************************
 *	     CloseMetaFile     (GDI.126)
 */
HMETAFILE16 WINAPI CloseMetaFile16(HDC16 hdc)
{
    return create_metafile16( CloseMetaFile( HDC_32(hdc) ));
}

/******************************************************************
 *	     DeleteMetaFile   (GDI.127)
 */
BOOL16 WINAPI DeleteMetaFile16(  HMETAFILE16 hmf )
{
    return !GlobalFree16( hmf );
}

/******************************************************************
 *         GetMetaFile   (GDI.124)
 */
HMETAFILE16 WINAPI GetMetaFile16( LPCSTR lpFilename )
{
    return create_metafile16( GetMetaFileA( lpFilename ));
}

/******************************************************************
 *         CopyMetaFile   (GDI.151)
 */
HMETAFILE16 WINAPI CopyMetaFile16( HMETAFILE16 hSrcMetaFile, LPCSTR lpFilename)
{
    HMETAFILE hmf = create_metafile32( hSrcMetaFile );
    HMETAFILE hmf2 = CopyMetaFileA( hmf, lpFilename );
    DeleteMetaFile( hmf );
    return create_metafile16( hmf2 );
}

/******************************************************************
 *         IsValidMetaFile   (GDI.410)
 *
 *  Attempts to check if a given metafile is correctly formatted.
 *  Currently, the only things verified are several properties of the
 *  header.
 *
 * RETURNS
 *  TRUE if hmf passes some tests for being a valid metafile, FALSE otherwise.
 *
 * BUGS
 *  This is not exactly what windows does, see _Undocumented_Windows_
 *  for details.
 */
BOOL16 WINAPI IsValidMetaFile16(HMETAFILE16 hmf)
{
    BOOL16 res=FALSE;
    METAHEADER *mh = MF_GetMetaHeader16(hmf);
    if (mh) {
        if (mh->mtType == METAFILE_MEMORY || mh->mtType == METAFILE_DISK)
	    if (mh->mtHeaderSize == MFHEADERSIZE/sizeof(INT16))
	        if (mh->mtVersion == MFVERSION)
		    res=TRUE;
	MF_ReleaseMetaHeader16(hmf);
    }
    TRACE("IsValidMetaFile %x => %d\n",hmf,res);
    return res;
}

/******************************************************************
 *         PlayMetaFile   (GDI.123)
 *
 */
BOOL16 WINAPI PlayMetaFile16( HDC16 hdc, HMETAFILE16 hmf16 )
{
    HMETAFILE hmf = create_metafile32( hmf16 );
    BOOL ret = PlayMetaFile( HDC_32(hdc), hmf );
    DeleteMetaFile( hmf );
    return ret;
}


/******************************************************************
 *            EnumMetaFile   (GDI.175)
 *
 */
BOOL16 WINAPI EnumMetaFile16( HDC16 hdc16, HMETAFILE16 hmf,
			      MFENUMPROC16 lpEnumFunc, LPARAM lpData )
{
    METAHEADER *mh = MF_GetMetaHeader16(hmf);
    METARECORD *mr;
    HANDLETABLE16 *ht;
    HDC hdc = HDC_32(hdc16);
    HGLOBAL16 hHT;
    SEGPTR spht;
    unsigned int offset = 0;
    WORD i, seg;
    HPEN hPen;
    HBRUSH hBrush;
    HFONT hFont;
    WORD args[8];
    BOOL16 result = TRUE;

    TRACE("(%p, %04x, %p, %08Ix)\n", hdc, hmf, lpEnumFunc, lpData);

    if(!mh) return FALSE;

    /* save the current pen, brush and font */
    hPen = GetCurrentObject(hdc, OBJ_PEN);
    hBrush = GetCurrentObject(hdc, OBJ_BRUSH);
    hFont = GetCurrentObject(hdc, OBJ_FONT);

    /* create the handle table */

    hHT = GlobalAlloc16(GMEM_MOVEABLE | GMEM_ZEROINIT,
            FIELD_OFFSET(HANDLETABLE16, objectHandle[mh->mtNoObjects]));
    spht = WOWGlobalLock16(hHT);

    seg = hmf | 7;
    offset = mh->mtHeaderSize * 2;

    /* loop through metafile records */

    args[7] = hdc16;
    args[6] = SELECTOROF(spht);
    args[5] = OFFSETOF(spht);
    args[4] = seg + (HIWORD(offset) << __AHSHIFT);
    args[3] = LOWORD(offset);
    args[2] = mh->mtNoObjects;
    args[1] = HIWORD(lpData);
    args[0] = LOWORD(lpData);

    while (offset < (mh->mtSize * 2))
    {
        DWORD ret;

	mr = (METARECORD *)((char *)mh + offset);

        WOWCallback16Ex( (DWORD)lpEnumFunc, WCB16_PASCAL, sizeof(args), args, &ret );
        if (!LOWORD(ret))
	{
	    result = FALSE;
	    break;
	}

	offset += (mr->rdSize * 2);
        args[4] = seg + (HIWORD(offset) << __AHSHIFT);
        args[3] = LOWORD(offset);
    }

    SelectObject(hdc, hBrush);
    SelectObject(hdc, hPen);
    SelectObject(hdc, hFont);

    ht = GlobalLock16(hHT);

    /* free objects in handle table */
    for(i = 0; i < mh->mtNoObjects; i++)
      if(*(ht->objectHandle + i) != 0)
        DeleteObject( (HGDIOBJ)(ULONG_PTR)(*(ht->objectHandle + i) ));

    /* free handle table */
    GlobalFree16(hHT);
    MF_ReleaseMetaHeader16(hmf);
    return result;
}

/******************************************************************
 *         GetMetaFileBits   (GDI.159)
 *
 * Trade in a metafile object handle for a handle to the metafile memory.
 *
 * PARAMS
 *  hmf [I] metafile handle
 */

HGLOBAL16 WINAPI GetMetaFileBits16( HMETAFILE16 hmf )
{
    TRACE("hMem out: %04x\n", hmf);
    return hmf;
}

/******************************************************************
 *         SetMetaFileBits   (GDI.160)
 *
 * Trade in a metafile memory handle for a handle to a metafile object.
 * The memory region should hold a proper metafile, otherwise
 * problems will occur when it is used. Validity of the memory is not
 * checked. The function is essentially just the identity function.
 *
 * PARAMS
 *  hMem [I] handle to a memory region holding a metafile
 *
 * RETURNS
 *  Handle to a metafile on success, NULL on failure..
 */
HMETAFILE16 WINAPI SetMetaFileBits16( HGLOBAL16 hMem )
{
    TRACE("hmf out: %04x\n", hMem);

    return hMem;
}

/******************************************************************
 *         SetMetaFileBitsBetter   (GDI.196)
 *
 * Trade in a metafile memory handle for a handle to a metafile object,
 * making a cursory check (using IsValidMetaFile()) that the memory
 * handle points to a valid metafile.
 *
 * RETURNS
 *  Handle to a metafile on success, NULL on failure..
 */
HMETAFILE16 WINAPI SetMetaFileBitsBetter16( HMETAFILE16 hMeta )
{
    if( IsValidMetaFile16( hMeta ) )
        return GlobalReAlloc16( hMeta, 0, GMEM_SHARE | GMEM_NODISCARD | GMEM_MODIFY);
    return 0;
}
