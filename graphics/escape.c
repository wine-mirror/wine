/*
 * Escape() function.
 *
 * Copyright 1994  Bob Amstadt
 * Copyright 2001  Alexandre Julliard
 */

#include <string.h>
#include "windef.h"
#include "wingdi.h"
#include "gdi.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(driver);

/***********************************************************************
 *            Escape  [GDI.38]
 */
INT16 WINAPI Escape16( HDC16 hdc, INT16 escape, INT16 in_count,
                       SEGPTR in_data, LPVOID out_data )
{
    INT ret;

    switch(escape)
    {
    /* Escape(hdc,CLIP_TO_PATH,LPINT16,NULL) */
    /* Escape(hdc,DRAFTMODE,LPINT16,NULL) */
    /* Escape(hdc,ENUMPAPERBINS,LPINT16,LPSTR); */
    /* Escape(hdc,EPSPRINTING,LPINT16,NULL) */
    /* Escape(hdc,EXT_DEVICE_CAPS,LPINT16,LPDWORD) */
    /* Escape(hdc,GETCOLORTABLE,LPINT16,LPDWORD) */
    /* Escape(hdc,MOUSETRAILS,LPINT16,NULL) */
    /* Escape(hdc,POSTSCRIPT_IGNORE,LPINT16,NULL) */
    /* Escape(hdc,QUERYESCSUPPORT,LPINT16,NULL) */
    /* Escape(hdc,SET_ARC_DIRECTION,LPINT16,NULL) */
    /* Escape(hdc,SET_POLY_MODE,LPINT16,NULL) */
    /* Escape(hdc,SET_SCREEN_ANGLE,LPINT16,NULL) */
    /* Escape(hdc,SET_SPREAD,LPINT16,NULL) */
    case CLIP_TO_PATH:
    case DRAFTMODE:
    case ENUMPAPERBINS:
    case EPSPRINTING:
    case EXT_DEVICE_CAPS:
    case GETCOLORTABLE:
    case MOUSETRAILS:
    case POSTSCRIPT_IGNORE:
    case QUERYESCSUPPORT:
    case SET_ARC_DIRECTION:
    case SET_POLY_MODE:
    case SET_SCREEN_ANGLE:
    case SET_SPREAD:
    {
        INT16 *ptr = MapSL(in_data);
        INT data = *ptr;
        return Escape( hdc, escape, sizeof(data), (LPCSTR)&data, out_data );
    }

    /* Escape(hdc,ENABLEDUPLEX,LPUINT16,NULL) */
    case ENABLEDUPLEX:
    {
        UINT16 *ptr = MapSL(in_data);
        UINT data = *ptr;
        return Escape( hdc, escape, sizeof(data), (LPCSTR)&data, NULL );
    }

    /* Escape(hdc,GETPHYSPAGESIZE,NULL,LPPOINT16) */
    /* Escape(hdc,GETPRINTINGOFFSET,NULL,LPPOINT16) */
    /* Escape(hdc,GETSCALINGFACTOR,NULL,LPPOINT16) */
    case GETPHYSPAGESIZE:
    case GETPRINTINGOFFSET:
    case GETSCALINGFACTOR:
    {
        POINT16 *ptr = MapSL(in_data);
        POINT pt32;
        ret = Escape( hdc, escape, 0, NULL, &pt32 );
        ptr->x = pt32.x;
        ptr->y = pt32.y;
        return ret;
    }

    /* Escape(hdc,ENABLEPAIRKERNING,LPINT16,LPINT16); */
    /* Escape(hdc,ENABLERELATIVEWIDTHS,LPINT16,LPINT16); */
    /* Escape(hdc,SETCOPYCOUNT,LPINT16,LPINT16) */
    /* Escape(hdc,SETKERNTRACK,LPINT16,LPINT16) */
    /* Escape(hdc,SETLINECAP,LPINT16,LPINT16) */
    /* Escape(hdc,SETLINEJOIN,LPINT16,LPINT16) */
    /* Escape(hdc,SETMITERLIMIT,LPINT16,LPINT16) */
    case ENABLEPAIRKERNING:
    case ENABLERELATIVEWIDTHS:
    case SETCOPYCOUNT:
    case SETKERNTRACK:
    case SETLINECAP:
    case SETLINEJOIN:
    case SETMITERLIMIT:
    {
        INT16 *new = MapSL(in_data);
        INT16 *old = out_data;
        INT out, in = *new;
        ret = Escape( hdc, escape, sizeof(in), (LPCSTR)&in, &out );
        *old = out;
        return ret;
    }

    /* Escape(hdc,SETABORTPROC,ABORTPROC,NULL); */
    case SETABORTPROC:
        return SetAbortProc16( hdc, in_data );

    /* Escape(hdc,STARTDOC,LPSTR,LPDOCINFO16);
     * lpvOutData is actually a pointer to the DocInfo structure and used as
     * a second input parameter */
    case STARTDOC:
        if (out_data)
        {
            ret = StartDoc16( hdc, out_data );
            if (ret > 0) ret = StartPage( hdc );
            return ret;
        }
        return Escape( hdc, escape, in_count, MapSL(in_data), NULL );

    /* Escape(hdc,SET_BOUNDS,LPRECT16,NULL); */
    /* Escape(hdc,SET_CLIP_BOX,LPRECT16,NULL); */
    case SET_BOUNDS:
    case SET_CLIP_BOX:
    {
        RECT16 *rc16 = MapSL(in_data);
        RECT rc;
        rc.left   = rc16->left;
        rc.top    = rc16->top;
        rc.right  = rc16->right;
        rc.bottom = rc16->bottom;
        return Escape( hdc, escape, sizeof(rc), (LPCSTR)&rc, NULL );
    }

    /* Escape(hdc,NEXTBAND,NULL,LPRECT16); */
    case NEXTBAND:
    {
        RECT rc;
        RECT16 *rc16 = MapSL(in_data);
        ret = Escape( hdc, escape, 0, NULL, &rc );
        rc16->left   = rc.left;
        rc16->top    = rc.top;
        rc16->right  = rc.right;
        rc16->bottom = rc.bottom;
        return ret;
    }

    /* Escape(hdc,ABORTDOC,NULL,NULL); */
    /* Escape(hdc,BANDINFO,BANDINFOSTRUCT*,BANDINFOSTRUCT*); */
    /* Escape(hdc,BEGIN_PATH,NULL,NULL); */
    /* Escape(hdc,DRAWPATTERNRECT,PRECT_STRUCT*,NULL); */
    /* Escape(hdc,ENDDOC,NULL,NULL); */
    /* Escape(hdc,END_PATH,PATHINFO,NULL); */
    /* Escape(hdc,EXTTEXTOUT,EXTTEXT_STRUCT*,NULL); */
    /* Escape(hdc,FLUSHOUTPUT,NULL,NULL); */
    /* Escape(hdc,GETFACENAME,NULL,LPSTR); */
    /* Escape(hdc,GETPAIRKERNTABLE,NULL,KERNPAIR*); */
    /* Escape(hdc,GETSETPAPERBINS,BinInfo*,BinInfo*); */
    /* Escape(hdc,GETSETPRINTORIENT,ORIENT*,NULL); */
    /* Escape(hdc,GETSETSCREENPARAMS,SCREENPARAMS*,SCREENPARAMS*); */
    /* Escape(hdc,GETTECHNOLOGY,NULL,LPSTR); */
    /* Escape(hdc,GETTRACKKERNTABLE,NULL,KERNTRACK*); */
    /* Escape(hdc,MFCOMMENT,LPSTR,NULL); */
    /* Escape(hdc,NEWFRAME,NULL,NULL); */
    /* Escape(hdc,PASSTHROUGH,LPSTR,NULL); */
    /* Escape(hdc,RESTORE_CTM,NULL,NULL); */
    /* Escape(hdc,SAVE_CTM,NULL,NULL); */
    /* Escape(hdc,SETALLJUSTVALUES,EXTTEXTDATA*,NULL); */
    /* Escape(hdc,SETCOLORTABLE,COLORTABLE_STRUCT*,LPDWORD); */
    /* Escape(hdc,SET_BACKGROUND_COLOR,LPDWORD,LPDWORD); */
    /* Escape(hdc,TRANSFORM_CTM,LPSTR,NULL); */
    case ABORTDOC:
    case BANDINFO:
    case BEGIN_PATH:
    case DRAWPATTERNRECT:
    case ENDDOC:
    case END_PATH:
    case EXTTEXTOUT:
    case FLUSHOUTPUT:
    case GETFACENAME:
    case GETPAIRKERNTABLE:
    case GETSETPAPERBINS:
    case GETSETPRINTORIENT:
    case GETSETSCREENPARAMS:
    case GETTECHNOLOGY:
    case GETTRACKKERNTABLE:
    case MFCOMMENT:
    case NEWFRAME:
    case PASSTHROUGH:
    case RESTORE_CTM:
    case SAVE_CTM:
    case SETALLJUSTVALUES:
    case SETCOLORTABLE:
    case SET_BACKGROUND_COLOR:
    case TRANSFORM_CTM:
        /* pass it unmodified to the 32-bit function */
        return Escape( hdc, escape, in_count, MapSL(in_data), out_data );

    /* Escape(hdc,ENUMPAPERMETRICS,LPINT16,LPRECT16); */
    /* Escape(hdc,GETEXTENDEDTEXTMETRICS,LPUINT16,EXTTEXTMETRIC*); */
    /* Escape(hdc,GETEXTENTTABLE,LPSTR,LPINT16); */
    /* Escape(hdc,GETSETPAPERMETRICS,LPRECT16,LPRECT16); */
    /* Escape(hdc,GETVECTORBRUSHSIZE,LPLOGBRUSH16,LPPOINT16); */
    /* Escape(hdc,GETVECTORPENSIZE,LPLOGPEN16,LPPOINT16); */
    case ENUMPAPERMETRICS:
    case GETEXTENDEDTEXTMETRICS:
    case GETEXTENTTABLE:
    case GETSETPAPERMETRICS:
    case GETVECTORBRUSHSIZE:
    case GETVECTORPENSIZE:
    default:
        FIXME("unknown/unsupported 16-bit escape %x (%d,%p,%p\n",
              escape, in_count, MapSL(in_data), out_data );
        return Escape( hdc, escape, in_count, MapSL(in_data), out_data );
    }
}


/************************************************************************
 *             Escape  [GDI32.@]
 */
INT WINAPI Escape( HDC hdc, INT escape, INT in_count, LPCSTR in_data, LPVOID out_data )
{
    INT ret;
    POINT *pt;

    switch (escape)
    {
    case ABORTDOC:
        return AbortDoc( hdc );

    case ENDDOC:
        return EndDoc( hdc );

    case GETPHYSPAGESIZE:
        pt = out_data;
        pt->x = GetDeviceCaps( hdc, PHYSICALWIDTH );
        pt->y = GetDeviceCaps( hdc, PHYSICALHEIGHT );
        return 1;

    case GETPRINTINGOFFSET:
        pt = out_data;
        pt->x = GetDeviceCaps( hdc, PHYSICALOFFSETX );
        pt->y = GetDeviceCaps( hdc, PHYSICALOFFSETY );
        return 1;

    case GETSCALINGFACTOR:
        pt = out_data;
        pt->x = GetDeviceCaps( hdc, SCALINGFACTORX );
        pt->y = GetDeviceCaps( hdc, SCALINGFACTORY );
        return 1;

    case NEWFRAME:
        return EndPage( hdc );

    case SETABORTPROC:
        return SetAbortProc( hdc, (ABORTPROC)in_data );

    case STARTDOC:
        {
            DOCINFOA doc;
            char *name = NULL;

            /* in_data may not be 0 terminated so we must copy it */
            if (in_data)
            {
                name = HeapAlloc( GetProcessHeap(), 0, in_count+1 );
                memcpy( name, in_data, in_count );
                name[in_count] = 0;
            }
            /* out_data is actually a pointer to the DocInfo structure and used as
             * a second input parameter */
            if (out_data) doc = *(DOCINFOA *)out_data;
            else
            {
                doc.cbSize = sizeof(doc);
                doc.lpszOutput = NULL;
                doc.lpszDatatype = NULL;
                doc.fwType = 0;
            }
            doc.lpszDocName = name;
            ret = StartDocA( hdc, &doc );
            if (name) HeapFree( GetProcessHeap(), 0, name );
            if (ret > 0) ret = StartPage( hdc );
            return ret;
        }

    case QUERYESCSUPPORT:
        {
            INT *ptr = (INT *)in_data;
            if (in_count < sizeof(INT)) return 0;
            switch(*ptr)
            {
            case ABORTDOC:
            case ENDDOC:
            case GETPHYSPAGESIZE:
            case GETPRINTINGOFFSET:
            case GETSCALINGFACTOR:
            case NEWFRAME:
            case QUERYESCSUPPORT:
            case SETABORTPROC:
            case STARTDOC:
                return TRUE;
            }
            break;
        }
    }

    /* if not handled internally, pass it to the driver */
    return ExtEscape( hdc, escape, in_count, in_data, 0, out_data );
}


/******************************************************************************
 *		ExtEscape	[GDI32.@]
 *
 * PARAMS
 *    hdc         [I] Handle to device context
 *    nEscape     [I] Escape function
 *    cbInput     [I] Number of bytes in input structure
 *    lpszInData  [I] Pointer to input structure
 *    cbOutput    [I] Number of bytes in output structure
 *    lpszOutData [O] Pointer to output structure
 *
 * RETURNS
 *    Success: >0
 *    Not implemented: 0
 *    Failure: <0
 */
INT WINAPI ExtEscape( HDC hdc, INT nEscape, INT cbInput, LPCSTR lpszInData,
                      INT cbOutput, LPSTR lpszOutData )
{
    INT ret = 0;
    DC * dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        if (dc->funcs->pExtEscape)
            ret = dc->funcs->pExtEscape( dc, nEscape, cbInput, lpszInData, cbOutput, lpszOutData );
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/*******************************************************************
 *      DrawEscape [GDI32.@]
 *
 *
 */
INT WINAPI DrawEscape(HDC hdc, INT nEscape, INT cbInput, LPCSTR lpszInData)
{
    FIXME("DrawEscape, stub\n");
    return 0;
}
