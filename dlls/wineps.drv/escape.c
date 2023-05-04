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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/wingdi16.h"
#include "winuser.h"
#include "winreg.h"
#include "psdrv.h"
#include "wine/debug.h"
#include "winspool.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

DWORD write_spool( PHYSDEV dev, const void *data, DWORD num )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );
    DWORD written;

    if (!WritePrinter(physDev->job.hprinter, (LPBYTE) data, num, &written) || (written != num))
        return SP_OUTOFDISK;

    return num;
}

/**********************************************************************
 *           ExtEscape  (WINEPS.@)
 */
INT CDECL PSDRV_ExtEscape( PHYSDEV dev, INT nEscape, INT cbInput, LPCVOID in_data,
                           INT cbOutput, LPVOID out_data )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );

    TRACE("%p,%d,%d,%p,%d,%p\n",
          dev->hdc, nEscape, cbInput, in_data, cbOutput, out_data);

    switch(nEscape)
    {
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
        passthrough_enter(dev);
        return write_spool(dev, ((char*)in_data) + 2, *(const WORD*)in_data);
    }

    case POSTSCRIPT_IGNORE:
    {
        BOOL ret = physDev->job.quiet;
        TRACE("POSTSCRIPT_IGNORE %d\n", *(const short*)in_data);
        physDev->job.quiet = *(const short*)in_data;
        return ret;
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
            ERR("END_PATH called without a BEGIN_PATH\n");
            return -1;
        }
        TRACE("RenderMode = %d, FillMode = %d, BkMode = %d\n",
                info->RenderMode, info->FillMode, info->BkMode);
        switch(info->RenderMode) {
        case RENDERMODE_NO_DISPLAY:
            PSDRV_WriteClosePath(dev); /* not sure if this is necessary, but it can't hurt */
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
            PSDRV_WriteGSave(dev);
            return 1;
        case CLIP_RESTORE:
            TRACE("CLIP_TO_PATH: CLIP_RESTORE\n");
            PSDRV_WriteGRestore(dev);
            return 1;
        case CLIP_INCLUSIVE:
            TRACE("CLIP_TO_PATH: CLIP_INCLUSIVE\n");
            /* FIXME to clip or eoclip ? (see PATH_INFO.FillMode) */
            PSDRV_WriteClip(dev);
            PSDRV_WriteNewPath(dev);
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
        return 0;
    }
}

/************************************************************************
 *           PSDRV_StartPage
 */
INT CDECL PSDRV_StartPage( PHYSDEV dev )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );

    TRACE("%p\n", dev->hdc);

    if(!physDev->job.OutOfPage) {
        FIXME("Already started a page?\n");
	return 1;
    }

    physDev->job.PageNo++;

    if(!PSDRV_WriteNewPage( dev ))
        return 0;
    physDev->job.OutOfPage = FALSE;
    return 1;
}


/************************************************************************
 *           PSDRV_EndPage
 */
INT CDECL PSDRV_EndPage( PHYSDEV dev )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );

    TRACE("%p\n", dev->hdc);

    if(physDev->job.OutOfPage) {
        FIXME("Already ended a page?\n");
	return 1;
    }

    passthrough_leave(dev);
    if(!PSDRV_WriteEndPage( dev ))
        return 0;
    PSDRV_EmptyDownloadList(dev, FALSE);
    physDev->job.OutOfPage = TRUE;
    return 1;
}


/************************************************************************
 *           PSDRV_StartDoc
 */
INT CDECL PSDRV_StartDoc( PHYSDEV dev, const DOCINFOW *doc )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );
    DOC_INFO_1W di;
    PRINTER_DEFAULTSW prn_def;

    TRACE("(%p, %p) => %s, %s, %s\n", physDev, doc, debugstr_w(doc->lpszDocName),
        debugstr_w(doc->lpszOutput), debugstr_w(doc->lpszDatatype));

    if(physDev->job.id) {
        FIXME("hJob != 0. Now what?\n");
	return 0;
    }

    prn_def.pDatatype = NULL;
    prn_def.pDevMode = &physDev->pi->Devmode->dmPublic;
    prn_def.DesiredAccess = PRINTER_ACCESS_USE;

    if (!OpenPrinterW( physDev->pi->friendly_name, &physDev->job.hprinter, &prn_def ))
    {
        WARN("OpenPrinter(%s, ...) failed: %ld\n",
            debugstr_w(physDev->pi->friendly_name), GetLastError());
        return 0;
    }

    di.pDocName = (LPWSTR) doc->lpszDocName;
    di.pOutputFile = (LPWSTR) doc->lpszOutput;
    di.pDatatype = NULL;

    /* redirection located in HKCU\Software\Wine\Printing\Spooler
       is done during winspool.drv,ScheduleJob */
    physDev->job.id = StartDocPrinterW(physDev->job.hprinter, 1, (LPBYTE) &di);
    if(!physDev->job.id) {
        WARN("StartDocPrinter() failed: %ld\n", GetLastError());
        ClosePrinter(physDev->job.hprinter);
	return 0;
    }

    if (!PSDRV_WriteHeader( dev, doc->lpszDocName )) {
        WARN("Failed to write header\n");
        ClosePrinter(physDev->job.hprinter);
        return 0;
    }

    physDev->job.OutOfPage = TRUE;
    physDev->job.PageNo = 0;
    physDev->job.quiet = FALSE;
    physDev->job.passthrough_state = passthrough_none;
    physDev->job.doc_name = strdupW( doc->lpszDocName );

    return physDev->job.id;
}

/************************************************************************
 *           PSDRV_EndDoc
 */
INT CDECL PSDRV_EndDoc( PHYSDEV dev )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );
    INT ret = 1;

    TRACE("%p\n", dev->hdc);

    if(!physDev->job.id) {
        FIXME("hJob == 0. Now what?\n");
	return 0;
    }

    if(!physDev->job.OutOfPage) {
        WARN("Somebody forgot an EndPage\n");
	PSDRV_EndPage( dev );
    }

    if (physDev->job.PageNo)
        PSDRV_WriteFooter( dev );

    ret = EndDocPrinter(physDev->job.hprinter);
    ClosePrinter(physDev->job.hprinter);
    physDev->job.hprinter = NULL;
    physDev->job.id = 0;
    HeapFree( GetProcessHeap(), 0, physDev->job.doc_name );
    physDev->job.doc_name = NULL;

    return ret;
}
