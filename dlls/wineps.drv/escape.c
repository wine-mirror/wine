/*
 *	PostScript driver Escape function
 *
 *	Copyright 1998  Huw D M Davies
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
#include "wingdi.h"
#include "wine/wingdi16.h"
#include "wownt32.h"
#include "psdrv.h"
#include "wine/debug.h"
#include "winspool.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

static const char psbegindocument[] =
"%%BeginDocument: Wine passthrough\n";

/**********************************************************************
 *           ExtEscape  (WINEPS.@)
 */
INT PSDRV_ExtEscape( PSDRV_PDEVICE *physDev, INT nEscape, INT cbInput, LPCVOID in_data,
                     INT cbOutput, LPVOID out_data )
{
    switch(nEscape)
    {
    case QUERYESCSUPPORT:
        if(cbInput < sizeof(INT))
        {
	    WARN("cbInput < sizeof(INT) (=%d) for QUERYESCSUPPORT\n", cbInput);
	    return 0;
	} else {
	    UINT num = *(const UINT *)in_data;
	    TRACE("QUERYESCSUPPORT for %d\n", num);

	    switch(num) {
	    case NEXTBAND:
	    /*case BANDINFO:*/
	    case SETCOPYCOUNT:
	    case GETTECHNOLOGY:
	    case SETLINECAP:
	    case SETLINEJOIN:
	    case SETMITERLIMIT:
	    case SETCHARSET:
	    case EXT_DEVICE_CAPS:
	    case SET_BOUNDS:
            case EPSPRINTING:
	    case POSTSCRIPT_DATA:
            case PASSTHROUGH:
            case POSTSCRIPT_PASSTHROUGH:
	    case POSTSCRIPT_IGNORE:
	    case BEGIN_PATH:
	    case CLIP_TO_PATH:
	    case END_PATH:
	    /*case DRAWPATTERNRECT:*/
	        return TRUE;

	    default:
		FIXME("QUERYESCSUPPORT(%d) - not supported.\n", num);
	        return FALSE;
	    }
	}

    case MFCOMMENT:
    {
	FIXME("MFCOMMENT(%p, %d)\n", in_data, cbInput);
	return 1;
    }
    case DRAWPATTERNRECT:
    {
	DRAWPATRECT	*dpr = (DRAWPATRECT*)in_data;

	FIXME("DRAWPATTERNRECT(pos (%d,%d), size %dx%d, style %d, pattern %x), stub!\n",
		dpr->ptPosition.x, dpr->ptPosition.y,
		dpr->ptSize.x, dpr->ptSize.y,
		dpr->wStyle, dpr->wPattern
	);
	return 1;
    }
    case BANDINFO:
    {
	BANDINFOSTRUCT	*ibi = (BANDINFOSTRUCT*)in_data;
	BANDINFOSTRUCT	*obi = (BANDINFOSTRUCT*)out_data;

	FIXME("BANDINFO(graphics %d, text %d, rect [%dx%d-%dx%d]), stub!\n",
		ibi->GraphicsFlag,
		ibi->TextFlag,
		ibi->GraphicsRect.top,
		ibi->GraphicsRect.bottom,
		ibi->GraphicsRect.left,
		ibi->GraphicsRect.right
	);
	memcpy (obi, ibi, sizeof(*ibi));
	return 1;
    }
    case NEXTBAND:
    {
        RECT *r = out_data;
	if(!physDev->job.banding) {
	    physDev->job.banding = TRUE;
            r->left   = 0;
            r->top    = 0;
            r->right  = physDev->horzRes;
            r->bottom = physDev->vertRes;
            TRACE("NEXTBAND returning %d,%d - %d,%d\n", r->left, r->top, r->right, r->bottom );
	    return 1;
	}
        r->left   = 0;
        r->top    = 0;
        r->right  = 0;
        r->bottom = 0;
	TRACE("NEXTBAND rect to 0,0 - 0,0\n" );
	physDev->job.banding = FALSE;
        return EndPage( physDev->hdc );
    }

    case SETCOPYCOUNT:
        {
            const INT *NumCopies = in_data;
            INT *ActualCopies = out_data;
            if(cbInput != sizeof(INT)) {
                WARN("cbInput != sizeof(INT) (=%d) for SETCOPYCOUNT\n", cbInput);
		return 0;
	    }
	    TRACE("SETCOPYCOUNT %d\n", *NumCopies);
	    *ActualCopies = 1;
	    return 1;
	}

    case GETTECHNOLOGY:
        {
	    LPSTR p = out_data;
	    strcpy(p, "PostScript");
	    *(p + strlen(p) + 1) = '\0'; /* 2 '\0's at end of string */
	    return 1;
	}

    case SETLINECAP:
        {
            INT newCap = *(const INT *)in_data;
            if(cbInput != sizeof(INT)) {
                WARN("cbInput != sizeof(INT) (=%d) for SETLINECAP\n", cbInput);
		return 0;
            }
	    TRACE("SETLINECAP %d\n", newCap);
	    return 0;
	}

    case SETLINEJOIN:
        {
            INT newJoin = *(const INT *)in_data;
            if(cbInput != sizeof(INT)) {
                WARN("cbInput != sizeof(INT) (=%d) for SETLINEJOIN\n", cbInput);
		return 0;
            }
	    TRACE("SETLINEJOIN %d\n", newJoin);
	    return 0;
	}

    case SETMITERLIMIT:
        {
            INT newLimit = *(const INT *)in_data;
            if(cbInput != sizeof(INT)) {
                WARN("cbInput != sizeof(INT) (=%d) for SETMITERLIMIT\n", cbInput);
		return 0;
            }
	    TRACE("SETMITERLIMIT %d\n", newLimit);
	    return 0;
	}

    case SETCHARSET:
      /* Undocumented escape used by winword6.
	 Switches between ANSI and a special charset.
	 If *lpInData == 1 we require that
	 0x91 is quoteleft
	 0x92 is quoteright
	 0x93 is quotedblleft
	 0x94 is quotedblright
	 0x95 is bullet
	 0x96 is endash
	 0x97 is emdash
	 0xa0 is non break space - yeah right.

	 If *lpInData == 0 we get ANSI.
	 Since there's nothing else there, let's just make these the default
	 anyway and see what happens...
      */
        return 1;

    case EXT_DEVICE_CAPS:
        {
            UINT cap = *(const UINT *)in_data;
            if(cbInput != sizeof(UINT)) {
                WARN("cbInput != sizeof(UINT) (=%d) for EXT_DEVICE_CAPS\n", cbInput);
		return 0;
            }
	    TRACE("EXT_DEVICE_CAPS %d\n", cap);
	    return 0;
	}

    case SET_BOUNDS:
        {
            const RECT *r = in_data;
            if(cbInput != sizeof(RECT)) {
                WARN("cbInput != sizeof(RECT) (=%d) for SET_BOUNDS\n", cbInput);
		return 0;
            }
	    TRACE("SET_BOUNDS (%d,%d) - (%d,%d)\n", r->left, r->top,
		  r->right, r->bottom);
	    return 0;
	}

    case EPSPRINTING:
	{
            UINT epsprint = *(const UINT*)in_data;
	    /* FIXME: In this mode we do not need to send page intros and page
	     * ends according to the doc. But I just ignore that detail
	     * for now.
	     */
	    TRACE("EPS Printing support %sable.\n",epsprint?"en":"dis");
	    return 1;
        }

    case POSTSCRIPT_DATA:
    case PASSTHROUGH:
    case POSTSCRIPT_PASSTHROUGH:
        {
            /* Write directly to spool file, bypassing normal PS driver
             * processing that is done along with writing PostScript code
             * to the spool.
	     * We have a WORD before the data counting the size, but
	     * cbInput is just this +2.
             * However Photoshop 7 has a bug that sets cbInput to 2 less than the
             * length of the string, rather than 2 more.  So we'll use the WORD at
             * in_data[0] instead.
             */
            if(!physDev->job.in_passthrough) {
                WriteSpool16(physDev->job.hJob, (LPSTR)psbegindocument, sizeof(psbegindocument)-1);
                physDev->job.in_passthrough = TRUE;
            }
            return WriteSpool16(physDev->job.hJob,((char*)in_data)+2,*(const WORD*)in_data);
        }

    case POSTSCRIPT_IGNORE:
      {
	BOOL ret = physDev->job.quiet;
        TRACE("POSTSCRIPT_IGNORE %d\n", *(const short*)in_data);
	physDev->job.quiet = *(const short*)in_data;
	return ret;
      }

    case GETSETPRINTORIENT:
	{
	    /* If lpInData is present, it is a 20 byte structure, first 32
	     * bit LONG value is the orientation. if lpInData is NULL, it
	     * returns the current orientation.
	     */
	    FIXME("GETSETPRINTORIENT not implemented (data %p)!\n",in_data);
	    return 1;
	}
    case BEGIN_PATH:
        TRACE("BEGIN_PATH\n");
	if(physDev->pathdepth)
	    FIXME("Nested paths not yet handled\n");
	return ++physDev->pathdepth;

    case END_PATH:
      {
	const struct PATH_INFO *info = (const struct PATH_INFO*)in_data;

	TRACE("END_PATH\n");
        if(!physDev->pathdepth) {
	    ERR("END_PATH called without a BEIGN_PATH\n");
	    return -1;
	}
	TRACE("RenderMode = %d, FillMode = %d, BkMode = %d\n",
	      info->RenderMode, info->FillMode, info->BkMode);
	switch(info->RenderMode) {
	case RENDERMODE_NO_DISPLAY:
	    PSDRV_WriteClosePath(physDev); /* not sure if this is necessary, but it can't hurt */
	    break;
	case RENDERMODE_OPEN:
	case RENDERMODE_CLOSED:
	default:
	    FIXME("END_PATH: RenderMode %d, not yet supported\n", info->RenderMode);
	    break;
	}
	return --physDev->pathdepth;
      }

    case CLIP_TO_PATH:
      {
	WORD mode = *(const WORD*)in_data;

	switch(mode) {
	case CLIP_SAVE:
	    TRACE("CLIP_TO_PATH: CLIP_SAVE\n");
	    PSDRV_WriteGSave(physDev);
	    return 1;
	case CLIP_RESTORE:
	    TRACE("CLIP_TO_PATH: CLIP_RESTORE\n");
	    PSDRV_WriteGRestore(physDev);
	    return 1;
	case CLIP_INCLUSIVE:
	    TRACE("CLIP_TO_PATH: CLIP_INCLUSIVE\n");
	    /* FIXME to clip or eoclip ? (see PATH_INFO.FillMode) */
	    PSDRV_WriteClip(physDev);
            PSDRV_WriteNewPath(physDev);
	    return 1;
	case CLIP_EXCLUSIVE:
	    FIXME("CLIP_EXCLUSIVE: not implemented\n");
	    return 0;
	default:
	    FIXME("Unknown CLIP_TO_PATH mode %d\n", mode);
	    return 0;
	}
      }
    default:
        FIXME("Unimplemented code 0x%x\n", nEscape);
	return 0;
    }
}

/************************************************************************
 *           PSDRV_StartPage
 */
INT PSDRV_StartPage( PSDRV_PDEVICE *physDev )
{
    if(!physDev->job.OutOfPage) {
        FIXME("Already started a page?\n");
	return 1;
    }

    if(physDev->job.PageNo++ == 0) {
        if(!PSDRV_WriteHeader( physDev, physDev->job.DocName ))
            return 0;
    }

    if(!PSDRV_WriteNewPage( physDev ))
        return 0;
    physDev->job.OutOfPage = FALSE;
    return 1;
}


/************************************************************************
 *           PSDRV_EndPage
 */
INT PSDRV_EndPage( PSDRV_PDEVICE *physDev )
{
    if(physDev->job.OutOfPage) {
        FIXME("Already ended a page?\n");
	return 1;
    }
    if(!PSDRV_WriteEndPage( physDev ))
        return 0;
    PSDRV_EmptyDownloadList(physDev, FALSE);
    physDev->job.OutOfPage = TRUE;
    return 1;
}


/************************************************************************
 *           PSDRV_StartDocA
 */
static INT PSDRV_StartDocA( PSDRV_PDEVICE *physDev, const DOCINFOA *doc )
{
    LPCSTR output = "LPT1:";
    BYTE buf[300];
    HANDLE hprn = INVALID_HANDLE_VALUE;
    PRINTER_INFO_5A *pi5 = (PRINTER_INFO_5A*)buf;
    DWORD needed;

    if(physDev->job.hJob) {
        FIXME("hJob != 0. Now what?\n");
	return 0;
    }

    if(doc->lpszOutput)
        output = doc->lpszOutput;
    else if(physDev->job.output)
        output = physDev->job.output;
    else {
        if(OpenPrinterA(physDev->pi->FriendlyName, &hprn, NULL) &&
           GetPrinterA(hprn, 5, buf, sizeof(buf), &needed)) {
            output = pi5->pPortName;
        }
        if(hprn != INVALID_HANDLE_VALUE)
            ClosePrinter(hprn);
    }

    physDev->job.hJob = OpenJob16(output,  doc->lpszDocName, HDC_16(physDev->hdc) );
    if(!physDev->job.hJob) {
        WARN("OpenJob failed\n");
	return 0;
    }
    physDev->job.banding = FALSE;
    physDev->job.OutOfPage = TRUE;
    physDev->job.PageNo = 0;
    physDev->job.quiet = FALSE;
    physDev->job.in_passthrough = FALSE;
    physDev->job.had_passthrough_rect = FALSE;
    if(doc->lpszDocName) {
        physDev->job.DocName = HeapAlloc(GetProcessHeap(), 0, strlen(doc->lpszDocName)+1);
        strcpy(physDev->job.DocName, doc->lpszDocName);
    } else
        physDev->job.DocName = NULL;

    return physDev->job.hJob;
}

/************************************************************************
 *           PSDRV_StartDoc
 */
INT PSDRV_StartDoc( PSDRV_PDEVICE *physDev, const DOCINFOW *doc )
{
    DOCINFOA docA;
    INT ret, len;
    LPSTR docname = NULL, output = NULL, datatype = NULL;

    docA.cbSize = doc->cbSize;
    if (doc->lpszDocName)
    {
        len = WideCharToMultiByte( CP_ACP, 0, doc->lpszDocName, -1, NULL, 0, NULL, NULL );
        if ((docname = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, doc->lpszDocName, -1, docname, len, NULL, NULL );
    }
    if (doc->lpszOutput)
    {
        len = WideCharToMultiByte( CP_ACP, 0, doc->lpszOutput, -1, NULL, 0, NULL, NULL );
        if ((output = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, doc->lpszOutput, -1, output, len, NULL, NULL );
    }
    if (doc->lpszDatatype)
    {
        len = WideCharToMultiByte( CP_ACP, 0, doc->lpszDatatype, -1, NULL, 0, NULL, NULL );
        if ((datatype = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, doc->lpszDatatype, -1, datatype, len, NULL, NULL );
    }
    docA.lpszDocName = docname;
    docA.lpszOutput = output;
    docA.lpszDatatype = datatype;
    docA.fwType = doc->fwType;

    ret = PSDRV_StartDocA(physDev, &docA);

    HeapFree( GetProcessHeap(), 0, docname );
    HeapFree( GetProcessHeap(), 0, output );
    HeapFree( GetProcessHeap(), 0, datatype );

    return ret;
}

/************************************************************************
 *           PSDRV_EndDoc
 */
INT PSDRV_EndDoc( PSDRV_PDEVICE *physDev )
{
    INT ret = 1;
    if(!physDev->job.hJob) {
        FIXME("hJob == 0. Now what?\n");
	return 0;
    }

    if(!physDev->job.OutOfPage) {
        WARN("Somebody forgot an EndPage\n");
	PSDRV_EndPage( physDev );
    }
    PSDRV_WriteFooter( physDev );

    if( CloseJob16( physDev->job.hJob ) == SP_ERROR ) {
        WARN("CloseJob error\n");
	ret = 0;
    }
    physDev->job.hJob = 0;
    HeapFree(GetProcessHeap(), 0, physDev->job.DocName);
    physDev->job.DocName = NULL;

    return ret;
}
