/*
 * Dialog functions
 *
 * Copyright 1993, 1994, 1996 Alexandre Julliard
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "windowsx.h"
#include "wine/winuser16.h"
#include "wine/winbase16.h"
#include "dialog.h"
#include "drive.h"
#include "heap.h"
#include "win.h"
#include "ldt.h"
#include "user.h"
#include "winproc.h"
#include "message.h"
#include "queue.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(dialog);


  /* Dialog control information */
typedef struct
{
    DWORD      style;
    DWORD      exStyle;
    DWORD      helpId;
    INT16      x;
    INT16      y;
    INT16      cx;
    INT16      cy;
    UINT     id;
    LPCSTR     className;
    LPCSTR     windowName;
    LPVOID     data;
} DLG_CONTROL_INFO;

  /* Dialog template */
typedef struct
{
    DWORD      style;
    DWORD      exStyle;
    DWORD      helpId;
    UINT16     nbItems;
    INT16      x;
    INT16      y;
    INT16      cx;
    INT16      cy;
    LPCSTR     menuName;
    LPCSTR     className;
    LPCSTR     caption;
    WORD       pointSize;
    WORD       weight;
    BOOL     italic;
    LPCSTR     faceName;
    BOOL     dialogEx;
} DLG_TEMPLATE;

  /* Radio button group */
typedef struct 
{
    UINT firstID;
    UINT lastID;
    UINT checkID;
} RADIOGROUP;

  /* Dialog base units */
static WORD xBaseUnit = 0, yBaseUnit = 0;

/***********************************************************************
 *           DIALOG_GetCharSizeFromDC
 *
 * 
 *  Calculates the *true* average size of English characters in the 
 *  specified font as oppposed to the one returned by GetTextMetrics.
 *
 *  Latest: the X font driver will now compute a proper average width
 *  so this code can be removed
 */
static BOOL DIALOG_GetCharSizeFromDC( HDC hDC, HFONT hFont, SIZE * pSize )
{
    BOOL Success = FALSE;
    HFONT hFontPrev = 0;
    pSize->cx = xBaseUnit;
    pSize->cy = yBaseUnit;
    if ( hDC ) 
    {
        /* select the font */
        TEXTMETRICA tm;
        memset(&tm,0,sizeof(tm));
        if (hFont) hFontPrev = SelectFont(hDC,hFont);
        if (GetTextMetricsA(hDC,&tm))
        {
            pSize->cx = tm.tmAveCharWidth;
            pSize->cy = tm.tmHeight;

            /* if variable width font */
            if (tm.tmPitchAndFamily & TMPF_FIXED_PITCH) 
            {
                SIZE total;
                const char* szAvgChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

                /* Calculate a true average as opposed to the one returned 
                 * by tmAveCharWidth. This works better when dealing with 
                 * proportional spaced fonts and (more important) that's 
                 * how Microsoft's dialog creation code calculates the size 
                 * of the font
                 */
                if (GetTextExtentPointA(hDC,szAvgChars,sizeof(szAvgChars),&total))
                {
                   /* round up */
                    pSize->cx = ((2*total.cx/sizeof(szAvgChars)) + 1)/2;
                    Success = TRUE;
                }
            } 
            else 
            {
                Success = TRUE;
            }
	    /* Use the text metrics */
	    TRACE("Using tm: %ldx%ld (dlg: %dx%d) (%s)\n", tm.tmAveCharWidth, tm.tmHeight, pSize->cx, pSize->cy,
		  tm.tmPitchAndFamily & TMPF_FIXED_PITCH ? "variable" : "fixed");		
	    pSize->cx = tm.tmAveCharWidth;
	    pSize->cy = tm.tmHeight;
        }
        /* select the original font */
        if (hFontPrev) SelectFont(hDC,hFontPrev);
    }
    return (Success);
}

/***********************************************************************
 *           DIALOG_GetCharSize
 *
 *  A convenient variant of DIALOG_GetCharSizeFromDC.
 */
static BOOL DIALOG_GetCharSize( HFONT hFont, SIZE * pSize )
{
    HDC  hDC = GetDC(0);
    BOOL Success = DIALOG_GetCharSizeFromDC( hDC, hFont, pSize );    
    ReleaseDC(0, hDC);
    return Success;
}

/***********************************************************************
 *           DIALOG_Init
 *
 * Initialisation of the dialog manager.
 */
BOOL DIALOG_Init(void)
{
    HDC16 hdc;
    SIZE size;

      /* Calculate the dialog base units */

    if (!(hdc = CreateDC16( "DISPLAY", NULL, NULL, NULL ))) return FALSE;
    if (!DIALOG_GetCharSizeFromDC( hdc, 0, &size )) return FALSE;
    DeleteDC( hdc );
    xBaseUnit = size.cx;
    yBaseUnit = size.cy;

    TRACE("base units = %d,%d\n", xBaseUnit, yBaseUnit );
    return TRUE;
}


/***********************************************************************
 *           DIALOG_GetControl16
 *
 * Return the class and text of the control pointed to by ptr,
 * fill the header structure and return a pointer to the next control.
 */
static LPCSTR DIALOG_GetControl16( LPCSTR p, DLG_CONTROL_INFO *info )
{
    static char buffer[10];
    int int_id;

    info->x       = GET_WORD(p);  p += sizeof(WORD);
    info->y       = GET_WORD(p);  p += sizeof(WORD);
    info->cx      = GET_WORD(p);  p += sizeof(WORD);
    info->cy      = GET_WORD(p);  p += sizeof(WORD);
    info->id      = GET_WORD(p);  p += sizeof(WORD);
    info->style   = GET_DWORD(p); p += sizeof(DWORD);
    info->exStyle = 0;

    if (*p & 0x80)
    {
        switch((BYTE)*p)
        {
            case 0x80: strcpy( buffer, "BUTTON" ); break;
            case 0x81: strcpy( buffer, "EDIT" ); break;
            case 0x82: strcpy( buffer, "STATIC" ); break;
            case 0x83: strcpy( buffer, "LISTBOX" ); break;
            case 0x84: strcpy( buffer, "SCROLLBAR" ); break;
            case 0x85: strcpy( buffer, "COMBOBOX" ); break;
            default:   buffer[0] = '\0'; break;
        }
        info->className = buffer;
        p++;
    }
    else 
    {
	info->className = p;
	p += strlen(p) + 1;
    }

    int_id = ((BYTE)*p == 0xff);
    if (int_id)
    {
	  /* Integer id, not documented (?). Only works for SS_ICON controls */
	info->windowName = (LPCSTR)(UINT)GET_WORD(p+1);
	p += 3;
    }
    else
    {
	info->windowName = p;
	p += strlen(p) + 1;
    }

    info->data = (LPVOID)(*p ? p + 1 : NULL);  /* FIXME: should be a segptr */
    p += *p + 1;

    if(int_id)
      TRACE("   %s %04x %d, %d, %d, %d, %d, %08lx, %08lx\n", 
		      info->className,  LOWORD(info->windowName),
		      info->id, info->x, info->y, info->cx, info->cy,
		      info->style, (DWORD)info->data);
    else
      TRACE("   %s '%s' %d, %d, %d, %d, %d, %08lx, %08lx\n", 
		      info->className,  info->windowName,
		      info->id, info->x, info->y, info->cx, info->cy,
		      info->style, (DWORD)info->data);

    return p;
}


/***********************************************************************
 *           DIALOG_GetControl32
 *
 * Return the class and text of the control pointed to by ptr,
 * fill the header structure and return a pointer to the next control.
 */
static const WORD *DIALOG_GetControl32( const WORD *p, DLG_CONTROL_INFO *info,
                                        BOOL dialogEx )
{
    if (dialogEx)
    {
        info->helpId  = GET_DWORD(p); p += 2;
        info->exStyle = GET_DWORD(p); p += 2;
        info->style   = GET_DWORD(p); p += 2;
    }
    else
    {
        info->helpId  = 0;
        info->style   = GET_DWORD(p); p += 2;
        info->exStyle = GET_DWORD(p); p += 2;
    }
    info->x       = GET_WORD(p); p++;
    info->y       = GET_WORD(p); p++;
    info->cx      = GET_WORD(p); p++;
    info->cy      = GET_WORD(p); p++;

    if (dialogEx)
    {
        /* id is a DWORD for DIALOGEX */
        info->id = GET_DWORD(p);
        p += 2;
    }
    else
    {
        info->id = GET_WORD(p);
        p++;
    }

    if (GET_WORD(p) == 0xffff)
    {
        static const WCHAR class_names[6][10] =
        {
            { 'B','u','t','t','o','n', },             /* 0x80 */
            { 'E','d','i','t', },                     /* 0x81 */
            { 'S','t','a','t','i','c', },             /* 0x82 */
            { 'L','i','s','t','B','o','x', },         /* 0x83 */
            { 'S','c','r','o','l','l','B','a','r', }, /* 0x84 */
            { 'C','o','m','b','o','B','o','x', }      /* 0x85 */
        };
        WORD id = GET_WORD(p+1);
        if ((id >= 0x80) && (id <= 0x85))
            info->className = (LPCSTR)class_names[id - 0x80];
        else
        {
            info->className = NULL;
            ERR("Unknown built-in class id %04x\n", id );
        }
        p += 2;
    }
    else
    {
        info->className = (LPCSTR)p;
        p += lstrlenW( (LPCWSTR)p ) + 1;
    }

    if (GET_WORD(p) == 0xffff)  /* Is it an integer id? */
    {
	info->windowName = (LPCSTR)(UINT)GET_WORD(p + 1);
	p += 2;
    }
    else
    {
	info->windowName = (LPCSTR)p;
        p += lstrlenW( (LPCWSTR)p ) + 1;
    }

    TRACE("    %s %s %d, %d, %d, %d, %d, %08lx, %08lx, %08lx\n", 
          debugstr_w( (LPCWSTR)info->className ),
          debugres_w( (LPCWSTR)info->windowName ),
          info->id, info->x, info->y, info->cx, info->cy,
          info->style, info->exStyle, info->helpId );

    if (GET_WORD(p))
    {
        if (TRACE_ON(dialog))
        {
            WORD i, count = GET_WORD(p) / sizeof(WORD);
            TRACE("  BEGIN\n");
            TRACE("    ");
            for (i = 0; i < count; i++) DPRINTF( "%04x,", GET_WORD(p+i+1) );
            DPRINTF("\n");
            TRACE("  END\n" );
        }
        info->data = (LPVOID)(p + 1);
        p += GET_WORD(p) / sizeof(WORD);
    }
    else info->data = NULL;
    p++;

    /* Next control is on dword boundary */
    return (const WORD *)((((int)p) + 3) & ~3);
}


/***********************************************************************
 *           DIALOG_CreateControls
 *
 * Create the control windows for a dialog.
 */
static BOOL DIALOG_CreateControls( WND *pWnd, LPCSTR template,
                                     const DLG_TEMPLATE *dlgTemplate,
                                     HINSTANCE hInst, BOOL win32 )
{
    DIALOGINFO *dlgInfo = (DIALOGINFO *)pWnd->wExtra;
    DLG_CONTROL_INFO info;
    HWND hwndCtrl, hwndDefButton = 0;
    INT items = dlgTemplate->nbItems;

    TRACE(" BEGIN\n" );
    while (items--)
    {
        if (!win32)
        {
            HINSTANCE16 instance;
            template = DIALOG_GetControl16( template, &info );
            if (HIWORD(info.className) && !strcmp( info.className, "EDIT") &&
                ((pWnd->dwStyle & DS_LOCALEDIT) != DS_LOCALEDIT))
            {
                if (!dlgInfo->hDialogHeap)
                {
                    dlgInfo->hDialogHeap = GlobalAlloc16(GMEM_FIXED, 0x10000);
                    if (!dlgInfo->hDialogHeap)
                    {
                        ERR("Insufficient memory to create heap for edit control\n" );
                        continue;
                    }
                    LocalInit16(dlgInfo->hDialogHeap, 0, 0xffff);
                }
                instance = dlgInfo->hDialogHeap;
            }
            else instance = (HINSTANCE16)hInst;

            hwndCtrl = CreateWindowEx16( info.exStyle | WS_EX_NOPARENTNOTIFY,
                                         info.className, info.windowName,
                                         info.style | WS_CHILD,
                                         MulDiv(info.x, dlgInfo->xBaseUnit, 4),
                                         MulDiv(info.y, dlgInfo->yBaseUnit, 8),
                                         MulDiv(info.cx, dlgInfo->xBaseUnit, 4),
                                         MulDiv(info.cy, dlgInfo->yBaseUnit, 8),
                                         pWnd->hwndSelf, (HMENU16)info.id,
                                         instance, info.data );
        }
        else
        {
            template = (LPCSTR)DIALOG_GetControl32( (WORD *)template, &info,
                                                    dlgTemplate->dialogEx );
            hwndCtrl = CreateWindowExW( info.exStyle | WS_EX_NOPARENTNOTIFY,
                                          (LPCWSTR)info.className,
                                          (LPCWSTR)info.windowName,
                                          info.style | WS_CHILD,
                                          MulDiv(info.x, dlgInfo->xBaseUnit, 4),
                                          MulDiv(info.y, dlgInfo->yBaseUnit, 8),
                                          MulDiv(info.cx, dlgInfo->xBaseUnit, 4),
                                          MulDiv(info.cy, dlgInfo->yBaseUnit, 8),
                                          pWnd->hwndSelf, (HMENU)info.id,
                                          hInst, info.data );
        }
        if (!hwndCtrl) return FALSE;

            /* Send initialisation messages to the control */
        if (dlgInfo->hUserFont) SendMessageA( hwndCtrl, WM_SETFONT,
                                             (WPARAM)dlgInfo->hUserFont, 0 );
        if (SendMessageA(hwndCtrl, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON)
        {
              /* If there's already a default push-button, set it back */
              /* to normal and use this one instead. */
            if (hwndDefButton)
                SendMessageA( hwndDefButton, BM_SETSTYLE,
                                BS_PUSHBUTTON,FALSE );
            hwndDefButton = hwndCtrl;
            dlgInfo->idResult = GetWindowWord( hwndCtrl, GWW_ID );
        }
    }    
    TRACE(" END\n" );
    return TRUE;
}


/***********************************************************************
 *           DIALOG_ParseTemplate16
 *
 * Fill a DLG_TEMPLATE structure from the dialog template, and return
 * a pointer to the first control.
 */
static LPCSTR DIALOG_ParseTemplate16( LPCSTR p, DLG_TEMPLATE * result )
{
    result->style   = GET_DWORD(p); p += sizeof(DWORD);
    result->exStyle = 0;
    result->nbItems = (unsigned char) *p++;
    result->x       = GET_WORD(p);  p += sizeof(WORD);
    result->y       = GET_WORD(p);  p += sizeof(WORD);
    result->cx      = GET_WORD(p);  p += sizeof(WORD);
    result->cy      = GET_WORD(p);  p += sizeof(WORD);
    TRACE("DIALOG %d, %d, %d, %d\n",
                    result->x, result->y, result->cx, result->cy );
    TRACE(" STYLE %08lx\n", result->style );

    /* Get the menu name */

    switch( (BYTE)*p )
    {
    case 0:
        result->menuName = 0;
        p++;
        break;
    case 0xff:
        result->menuName = (LPCSTR)(UINT)GET_WORD( p + 1 );
        p += 3;
	TRACE(" MENU %04x\n", LOWORD(result->menuName) );
        break;
    default:
        result->menuName = p;
        TRACE(" MENU '%s'\n", p );
        p += strlen(p) + 1;
        break;
    }

    /* Get the class name */

    if (*p)
    {
        result->className = p;
        TRACE(" CLASS '%s'\n", result->className );
    }
    else result->className = DIALOG_CLASS_ATOM;
    p += strlen(p) + 1;

    /* Get the window caption */

    result->caption = p;
    p += strlen(p) + 1;
    TRACE(" CAPTION '%s'\n", result->caption );

    /* Get the font name */

    if (result->style & DS_SETFONT)
    {
	result->pointSize = GET_WORD(p);
        p += sizeof(WORD);
	result->faceName = p;
        p += strlen(p) + 1;
	TRACE(" FONT %d,'%s'\n",
                        result->pointSize, result->faceName );
    }
    return p;
}


/***********************************************************************
 *           DIALOG_ParseTemplate32
 *
 * Fill a DLG_TEMPLATE structure from the dialog template, and return
 * a pointer to the first control.
 */
static LPCSTR DIALOG_ParseTemplate32( LPCSTR template, DLG_TEMPLATE * result )
{
    const WORD *p = (const WORD *)template;

    result->style = GET_DWORD(p); p += 2;
    if (result->style == 0xffff0001)  /* DIALOGEX resource */
    {
        result->dialogEx = TRUE;
        result->helpId   = GET_DWORD(p); p += 2;
        result->exStyle  = GET_DWORD(p); p += 2;
        result->style    = GET_DWORD(p); p += 2;
    }
    else
    {
        result->dialogEx = FALSE;
        result->helpId   = 0;
        result->exStyle  = GET_DWORD(p); p += 2;
    }
    result->nbItems = GET_WORD(p); p++;
    result->x       = GET_WORD(p); p++;
    result->y       = GET_WORD(p); p++;
    result->cx      = GET_WORD(p); p++;
    result->cy      = GET_WORD(p); p++;
    TRACE("DIALOG%s %d, %d, %d, %d, %ld\n",
           result->dialogEx ? "EX" : "", result->x, result->y,
           result->cx, result->cy, result->helpId );
    TRACE(" STYLE 0x%08lx\n", result->style );
    TRACE(" EXSTYLE 0x%08lx\n", result->exStyle );

    /* Get the menu name */

    switch(GET_WORD(p))
    {
    case 0x0000:
        result->menuName = NULL;
        p++;
        break;
    case 0xffff:
        result->menuName = (LPCSTR)(UINT)GET_WORD( p + 1 );
        p += 2;
	TRACE(" MENU %04x\n", LOWORD(result->menuName) );
        break;
    default:
        result->menuName = (LPCSTR)p;
        TRACE(" MENU %s\n", debugstr_w( (LPCWSTR)p ));
        p += lstrlenW( (LPCWSTR)p ) + 1;
        break;
    }

    /* Get the class name */

    switch(GET_WORD(p))
    {
    case 0x0000:
        result->className = DIALOG_CLASS_ATOM;
        p++;
        break;
    case 0xffff:
        result->className = (LPCSTR)(UINT)GET_WORD( p + 1 );
        p += 2;
	TRACE(" CLASS %04x\n", LOWORD(result->className) );
        break;
    default:
        result->className = (LPCSTR)p;
        TRACE(" CLASS %s\n", debugstr_w( (LPCWSTR)p ));
        p += lstrlenW( (LPCWSTR)p ) + 1;
        break;
    }

    /* Get the window caption */

    result->caption = (LPCSTR)p;
    p += lstrlenW( (LPCWSTR)p ) + 1;
    TRACE(" CAPTION %s\n", debugstr_w( (LPCWSTR)result->caption ) );

    /* Get the font name */

    if (result->style & DS_SETFONT)
    {
	result->pointSize = GET_WORD(p);
        p++;
        if (result->dialogEx)
        {
            result->weight = GET_WORD(p); p++;
            result->italic = LOBYTE(GET_WORD(p)); p++;
        }
        else
        {
            result->weight = FW_DONTCARE;
            result->italic = FALSE;
        }
	result->faceName = (LPCSTR)p;
        p += lstrlenW( (LPCWSTR)p ) + 1;
	TRACE(" FONT %d, %s, %d, %s\n",
              result->pointSize, debugstr_w( (LPCWSTR)result->faceName ),
              result->weight, result->italic ? "TRUE" : "FALSE" );
    }

    /* First control is on dword boundary */
    return (LPCSTR)((((int)p) + 3) & ~3);
}


/***********************************************************************
 *           DIALOG_CreateIndirect
 */
HWND DIALOG_CreateIndirect( HINSTANCE hInst, LPCSTR dlgTemplate,
                              BOOL win32Template, HWND owner,
                              DLGPROC16 dlgProc, LPARAM param,
                              WINDOWPROCTYPE procType )
{
    HMENU16 hMenu = 0;
    HFONT16 hFont = 0;
    HWND hwnd;
    RECT rect;
    WND * wndPtr;
    DLG_TEMPLATE template;
    DIALOGINFO * dlgInfo;
    WORD xUnit = xBaseUnit;
    WORD yUnit = yBaseUnit;

      /* Parse dialog template */

    if (!dlgTemplate) return 0;
    if (win32Template)
        dlgTemplate = DIALOG_ParseTemplate32( dlgTemplate, &template );
    else
        dlgTemplate = DIALOG_ParseTemplate16( dlgTemplate, &template );

      /* Load menu */

    if (template.menuName)
    {
        if (!win32Template)
        {
            LPSTR str = SEGPTR_STRDUP( template.menuName );
	    hMenu = LoadMenu16( hInst, SEGPTR_GET(str) );
            SEGPTR_FREE( str );
	}
        else hMenu = LoadMenuW( hInst, (LPCWSTR)template.menuName );
    }

      /* Create custom font if needed */

    if (template.style & DS_SETFONT)
    {
	  /* The font height must be negative as it is a point size */
	  /* and must be converted to pixels first */
          /* (see CreateFont() documentation in the Windows SDK).   */
	HDC dc = GetDC(0);
	int pixels = template.pointSize * GetDeviceCaps(dc , LOGPIXELSY)/72;
	ReleaseDC(0, dc);

	if (win32Template)
	    hFont = CreateFontW( -pixels, 0, 0, 0, template.weight,
				 template.italic, FALSE, FALSE, 
				 DEFAULT_CHARSET, 0, 0,
				 PROOF_QUALITY, FF_DONTCARE,
				 (LPCWSTR)template.faceName );
	else
	    hFont = CreateFont16( -pixels, 0, 0, 0, FW_DONTCARE,
				  FALSE, FALSE, FALSE,
				  DEFAULT_CHARSET, 0, 0,
				  PROOF_QUALITY, FF_DONTCARE,
				  template.faceName );
        if (hFont)
        {
            SIZE charSize;
            DIALOG_GetCharSize(hFont,&charSize); 
            xUnit = charSize.cx;
            yUnit = charSize.cy;
        }
	TRACE("units = %d,%d\n", xUnit, yUnit );
    }
    
    /* Create dialog main window */

    rect.left = rect.top = 0;
    rect.right = MulDiv(template.cx, xUnit, 4);
    rect.bottom =  MulDiv(template.cy, yUnit, 8);
    if (template.style & DS_MODALFRAME)
        template.exStyle |= WS_EX_DLGMODALFRAME;
    AdjustWindowRectEx( &rect, template.style, 
                          hMenu ? TRUE : FALSE , template.exStyle );
    rect.right -= rect.left;
    rect.bottom -= rect.top;

    if ((INT16)template.x == CW_USEDEFAULT16)
    {
        rect.left = rect.top = win32Template? CW_USEDEFAULT : CW_USEDEFAULT16;
    }
    else
    {
        if (template.style & DS_CENTER)
        {
            rect.left = (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2;
            rect.top = (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2;
        }
        else
        {
            rect.left += MulDiv(template.x, xUnit, 4);
            rect.top += MulDiv(template.y, yUnit, 8);
        }
        if ( !(template.style & WS_CHILD) )
	{
            INT16 dX, dY;

            if( !(template.style & DS_ABSALIGN) )
                ClientToScreen( owner, (POINT *)&rect );
	    
            /* try to fit it into the desktop */

            if( (dX = rect.left + rect.right + GetSystemMetrics(SM_CXDLGFRAME)
                 - GetSystemMetrics(SM_CXSCREEN)) > 0 ) rect.left -= dX;
            if( (dY = rect.top + rect.bottom + GetSystemMetrics(SM_CYDLGFRAME)
                 - GetSystemMetrics(SM_CYSCREEN)) > 0 ) rect.top -= dY;
            if( rect.left < 0 ) rect.left = 0;
            if( rect.top < 0 ) rect.top = 0;
        }
    }

    if (!win32Template)
        hwnd = CreateWindowEx16(template.exStyle, template.className,
                                template.caption, template.style & ~WS_VISIBLE,
                                rect.left, rect.top, rect.right, rect.bottom,
                                owner, hMenu, hInst, NULL );
    else
        hwnd = CreateWindowExW(template.exStyle, (LPCWSTR)template.className,
                                 (LPCWSTR)template.caption,
                                 template.style & ~WS_VISIBLE,
                                 rect.left, rect.top, rect.right, rect.bottom,
                                 owner, hMenu, hInst, NULL );
	
    if (!hwnd)
    {
	if (hFont) DeleteObject( hFont );
	if (hMenu) DestroyMenu( hMenu );
	return 0;
    }
    wndPtr = WIN_FindWndPtr( hwnd );
    wndPtr->flags |= WIN_ISDIALOG;
    wndPtr->helpContext = template.helpId;

      /* Initialise dialog extra data */

    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    WINPROC_SetProc( &dlgInfo->dlgProc, (WNDPROC16)dlgProc, procType, WIN_PROC_WINDOW );
    dlgInfo->hUserFont = hFont;
    dlgInfo->hMenu     = hMenu;
    dlgInfo->xBaseUnit = xUnit;
    dlgInfo->yBaseUnit = yUnit;
    dlgInfo->msgResult = 0;
    dlgInfo->idResult  = 0;
    dlgInfo->flags     = 0;
    dlgInfo->hDialogHeap = 0;

    if (dlgInfo->hUserFont)
        SendMessageA( hwnd, WM_SETFONT, (WPARAM)dlgInfo->hUserFont, 0 );

    /* Create controls */

    if (DIALOG_CreateControls( wndPtr, dlgTemplate, &template,
                               hInst, win32Template ))
    {
        HWND hwndPreInitFocus;

        /* Send initialisation messages and set focus */

	dlgInfo->hwndFocus = GetNextDlgTabItem( hwnd, 0, FALSE );

	hwndPreInitFocus = GetFocus();
	if (SendMessageA( hwnd, WM_INITDIALOG, (WPARAM)dlgInfo->hwndFocus, param ))
            SetFocus( dlgInfo->hwndFocus );
	else
	{
	  /* If the dlgproc has returned FALSE (indicating handling of keyboard focus)
	     but the focus has not changed, set the focus where we expect it. */
	  if ( GetFocus() == hwndPreInitFocus )
	    SetFocus( dlgInfo->hwndFocus );
	}

	if (template.style & WS_VISIBLE && !(wndPtr->dwStyle & WS_VISIBLE)) 
	{
	   ShowWindow( hwnd, SW_SHOWNORMAL );	/* SW_SHOW doesn't always work */
	   UpdateWindow( hwnd );
	}
        WIN_ReleaseWndPtr(wndPtr);
	return hwnd;
    }
    WIN_ReleaseWndPtr(wndPtr);
    if( IsWindow(hwnd) ) DestroyWindow( hwnd );
    return 0;
}


/***********************************************************************
 *           CreateDialog16   (USER.89)
 */
HWND16 WINAPI CreateDialog16( HINSTANCE16 hInst, SEGPTR dlgTemplate,
                              HWND16 owner, DLGPROC16 dlgProc )
{
    return CreateDialogParam16( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           CreateDialogParam16   (USER.241)
 */
HWND16 WINAPI CreateDialogParam16( HINSTANCE16 hInst, SEGPTR dlgTemplate,
                                   HWND16 owner, DLGPROC16 dlgProc,
                                   LPARAM param )
{
    HWND16 hwnd = 0;
    HRSRC16 hRsrc;
    HGLOBAL16 hmem;
    LPCVOID data;

    TRACE("%04x,%08lx,%04x,%08lx,%ld\n",
                   hInst, (DWORD)dlgTemplate, owner, (DWORD)dlgProc, param );

    if (!(hRsrc = FindResource16( hInst, dlgTemplate, RT_DIALOG16 ))) return 0;
    if (!(hmem = LoadResource16( hInst, hRsrc ))) return 0;
    if (!(data = LockResource16( hmem ))) hwnd = 0;
    else hwnd = CreateDialogIndirectParam16( hInst, data, owner,
                                             dlgProc, param );
    FreeResource16( hmem );
    return hwnd;
}

/***********************************************************************
 *           CreateDialogParamA   (USER32.73)
 */
HWND WINAPI CreateDialogParamA( HINSTANCE hInst, LPCSTR name,
                                    HWND owner, DLGPROC dlgProc,
                                    LPARAM param )
{
    HANDLE hrsrc = FindResourceA( hInst, name, RT_DIALOGA );
    if (!hrsrc) return 0;
    return CreateDialogIndirectParamA( hInst,
                                         (LPVOID)LoadResource(hInst, hrsrc),
                                         owner, dlgProc, param );
}


/***********************************************************************
 *           CreateDialogParamW   (USER32.74)
 */
HWND WINAPI CreateDialogParamW( HINSTANCE hInst, LPCWSTR name,
                                    HWND owner, DLGPROC dlgProc,
                                    LPARAM param )
{
    HANDLE hrsrc = FindResourceW( hInst, name, RT_DIALOGW );
    if (!hrsrc) return 0;
    return CreateDialogIndirectParamW( hInst,
                                         (LPVOID)LoadResource(hInst, hrsrc),
                                         owner, dlgProc, param );
}


/***********************************************************************
 *           CreateDialogIndirect16   (USER.219)
 */
HWND16 WINAPI CreateDialogIndirect16( HINSTANCE16 hInst, LPCVOID dlgTemplate,
                                      HWND16 owner, DLGPROC16 dlgProc )
{
    return CreateDialogIndirectParam16( hInst, dlgTemplate, owner, dlgProc, 0);
}


/***********************************************************************
 *           CreateDialogIndirectParam16   (USER.242)
 */
HWND16 WINAPI CreateDialogIndirectParam16( HINSTANCE16 hInst,
                                           LPCVOID dlgTemplate,
                                           HWND16 owner, DLGPROC16 dlgProc,
                                           LPARAM param )
{
    return DIALOG_CreateIndirect( hInst, dlgTemplate, FALSE, owner,
                                  dlgProc, param, WIN_PROC_16 );
}


/***********************************************************************
 *           CreateDialogIndirectParamA   (USER32.69)
 */
HWND WINAPI CreateDialogIndirectParamA( HINSTANCE hInst,
                                            LPCVOID dlgTemplate,
                                            HWND owner, DLGPROC dlgProc,
                                            LPARAM param )
{
    return DIALOG_CreateIndirect( hInst, dlgTemplate, TRUE, owner,
                                  (DLGPROC16)dlgProc, param, WIN_PROC_32A );
}

/***********************************************************************
 *           CreateDialogIndirectParamAorW   (USER32.71)
 */
HWND WINAPI CreateDialogIndirectParamAorW( HINSTANCE hInst,
                                            LPCVOID dlgTemplate,
                                            HWND owner, DLGPROC dlgProc,
                                            LPARAM param )
{   FIXME("assume WIN_PROC_32W\n");
    return DIALOG_CreateIndirect( hInst, dlgTemplate, TRUE, owner,
                                  (DLGPROC16)dlgProc, param, WIN_PROC_32W );
}

/***********************************************************************
 *           CreateDialogIndirectParamW   (USER32.72)
 */
HWND WINAPI CreateDialogIndirectParamW( HINSTANCE hInst,
                                            LPCVOID dlgTemplate,
                                            HWND owner, DLGPROC dlgProc,
                                            LPARAM param )
{
    return DIALOG_CreateIndirect( hInst, dlgTemplate, TRUE, owner,
                                  (DLGPROC16)dlgProc, param, WIN_PROC_32W );
}


/***********************************************************************
 *           DIALOG_DoDialogBox
 */
INT DIALOG_DoDialogBox( HWND hwnd, HWND owner )
{
    WND * wndPtr;
    DIALOGINFO * dlgInfo;
    MSG msg;
    INT retval;

      /* Owner must be a top-level window */
    owner = WIN_GetTopParent( owner );
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return -1;
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;

    if (!dlgInfo->flags & DF_END) /* was EndDialog called in WM_INITDIALOG ? */
    {
        EnableWindow( owner, FALSE );
        ShowWindow( hwnd, SW_SHOW );
        while (MSG_InternalGetMessage(QMSG_WIN32A, &msg, hwnd, owner, MSGF_DIALOGBOX, 
                                      PM_REMOVE, !(wndPtr->dwStyle & DS_NOIDLEMSG), NULL ))
        {
            if (!IsDialogMessageA( hwnd, &msg))
            {
                TranslateMessage( &msg );
                DispatchMessageA( &msg );
            }
            if (dlgInfo->flags & DF_END) break;
        }
        EnableWindow( owner, TRUE );
    }
    retval = dlgInfo->idResult; 
    WIN_ReleaseWndPtr(wndPtr);
    DestroyWindow( hwnd );
    return retval;
}


/***********************************************************************
 *           DialogBox16   (USER.87)
 */
INT16 WINAPI DialogBox16( HINSTANCE16 hInst, SEGPTR dlgTemplate,
                          HWND16 owner, DLGPROC16 dlgProc )
{
    return DialogBoxParam16( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           DialogBoxParam16   (USER.239)
 */
INT16 WINAPI DialogBoxParam16( HINSTANCE16 hInst, SEGPTR template,
                               HWND16 owner, DLGPROC16 dlgProc, LPARAM param )
{
    HWND16 hwnd = CreateDialogParam16( hInst, template, owner, dlgProc, param);
    if (hwnd) return (INT16)DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxParamA   (USER32.139)
 */
INT WINAPI DialogBoxParamA( HINSTANCE hInst, LPCSTR name,
                                HWND owner, DLGPROC dlgProc, LPARAM param )
{
    HWND hwnd = CreateDialogParamA( hInst, name, owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxParamW   (USER32.140)
 */
INT WINAPI DialogBoxParamW( HINSTANCE hInst, LPCWSTR name,
                                HWND owner, DLGPROC dlgProc, LPARAM param )
{
    HWND hwnd = CreateDialogParamW( hInst, name, owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxIndirect16   (USER.218)
 */
INT16 WINAPI DialogBoxIndirect16( HINSTANCE16 hInst, HANDLE16 dlgTemplate,
                                  HWND16 owner, DLGPROC16 dlgProc )
{
    return DialogBoxIndirectParam16( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           DialogBoxIndirectParam16   (USER.240)
 */
INT16 WINAPI DialogBoxIndirectParam16( HINSTANCE16 hInst, HANDLE16 dlgTemplate,
                                       HWND16 owner, DLGPROC16 dlgProc,
                                       LPARAM param )
{
    HWND16 hwnd;
    LPCVOID ptr;

    if (!(ptr = GlobalLock16( dlgTemplate ))) return -1;
    hwnd = CreateDialogIndirectParam16( hInst, ptr, owner, dlgProc, param );
    GlobalUnlock16( dlgTemplate );
    if (hwnd) return (INT16)DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxIndirectParamA   (USER32.136)
 */
INT WINAPI DialogBoxIndirectParamA(HINSTANCE hInstance, LPCVOID template,
                                       HWND owner, DLGPROC dlgProc,
                                       LPARAM param )
{
    HWND hwnd = CreateDialogIndirectParamA( hInstance, template,
                                                owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxIndirectParamW   (USER32.138)
 */
INT WINAPI DialogBoxIndirectParamW(HINSTANCE hInstance, LPCVOID template,
                                       HWND owner, DLGPROC dlgProc,
                                       LPARAM param )
{
    HWND hwnd = CreateDialogIndirectParamW( hInstance, template,
                                                owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}

/***********************************************************************
 *           DialogBoxIndirectParamAorW   (USER32.138)
 */
INT WINAPI DialogBoxIndirectParamAorW(HINSTANCE hInstance, LPCVOID template,
                                       HWND owner, DLGPROC dlgProc,
                                       LPARAM param, DWORD x )
{
    HWND hwnd;
    FIXME("0x%08x %p 0x%08x %p 0x%08lx 0x%08lx\n",
      hInstance, template, owner, dlgProc, param, x);
    hwnd = CreateDialogIndirectParamW( hInstance, template,
                                                owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}

/***********************************************************************
 *           EndDialog16   (USER.88)
 */
BOOL16 WINAPI EndDialog16( HWND16 hwnd, INT16 retval )
{
    return EndDialog( hwnd, retval );
}


/***********************************************************************
 *           EndDialog   (USER32.173)
 */
BOOL WINAPI EndDialog( HWND hwnd, INT retval )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    DIALOGINFO * dlgInfo;
    HWND hOwner = 0;

    TRACE("%04x %d\n", hwnd, retval );

    if (!wndPtr)
    {
	ERR("got invalid window handle (%04x); buggy app !?\n", hwnd);
	return FALSE;
    }

    if ((dlgInfo = (DIALOGINFO *)wndPtr->wExtra))
    {
        dlgInfo->idResult = retval;
        dlgInfo->flags |= DF_END;
    }

    if(wndPtr->owner)
      hOwner = WIN_GetTopParent( wndPtr->owner->hwndSelf );

    /* Enable the owner first */
    if (hOwner && !IsWindowEnabled(hOwner))
      EnableWindow( hOwner, TRUE );
 
    /* Windows sets the focus to the dialog itself in EndDialog */

    if (IsChild(hwnd, GetFocus()))
       SetFocus(wndPtr->hwndSelf);

    /* Don't have to send a ShowWindow(SW_HIDE), just do
       SetWindowPos with SWP_HIDEWINDOW as done in Windows */

    SetWindowPos(hwnd, (HWND)0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE
                 | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);

    WIN_ReleaseWndPtr(wndPtr);
 
    return TRUE;
}


/***********************************************************************
 *           DIALOG_IsAccelerator
 */
static BOOL DIALOG_IsAccelerator( HWND hwnd, HWND hwndDlg, WPARAM vKey )
{
    HWND hwndControl = hwnd;
    HWND hwndNext;
    WND *wndPtr;
    BOOL RetVal = FALSE;
    INT dlgCode;

        do
        {
            wndPtr = WIN_FindWndPtr( hwndControl );
            if ( (wndPtr != NULL) && 
                 ((wndPtr->dwStyle & (WS_VISIBLE | WS_DISABLED)) == WS_VISIBLE) )
            {
                dlgCode = SendMessageA( hwndControl, WM_GETDLGCODE, 0, 0 );
                if ( (dlgCode & (DLGC_BUTTON | DLGC_STATIC)) && 
                     (wndPtr->text!=NULL))
                {
                    /* find the accelerator key */
                    LPSTR p = wndPtr->text - 2;
                    do
                    {
                        p = strchr( p + 2, '&' );
                    }
                    while (p != NULL && p[1] == '&');

                    /* and check if it's the one we're looking for */
                    if (p != NULL && toupper( p[1] ) == toupper( vKey ) )
                    {
                        if ((dlgCode & DLGC_STATIC) || 
                            (wndPtr->dwStyle & 0x0f) == BS_GROUPBOX )
                        {
                            /* set focus to the control */
                            SendMessageA( hwndDlg, WM_NEXTDLGCTL,
                                    hwndControl, 1);
                            /* and bump it on to next */
                            SendMessageA( hwndDlg, WM_NEXTDLGCTL, 0, 0);
                        }
                        else if (dlgCode & DLGC_BUTTON)
                        {
                            /* send BM_CLICK message to the control */
                            SendMessageA( hwndControl, BM_CLICK, 0, 0 );
                        }

                        RetVal = TRUE;
			WIN_ReleaseWndPtr(wndPtr);
                        break;
                    }
                }
		hwndNext = GetWindow( hwndControl, GW_CHILD );
            }
	    else
	    {
		hwndNext = 0;
	    }
            WIN_ReleaseWndPtr(wndPtr);
	    if (!hwndNext)
	    {
	        hwndNext = GetWindow( hwndControl, GW_HWNDNEXT );
	    }
	    while (!hwndNext && hwndControl)
	    {
		hwndControl = GetParent( hwndControl );
		if (hwndControl == hwndDlg)
		{
		    if(hwnd==hwndDlg){  /* prevent endless loop */
		        hwndNext=hwnd;
			break;
		    }
		    hwndNext = GetWindow( hwndDlg, GW_CHILD );
		}
		else
		{
		    hwndNext = GetWindow( hwndControl, GW_HWNDNEXT );
		}
	    }
            hwndControl = hwndNext;
        }
        while (hwndControl && (hwndControl != hwnd));

    return RetVal;
}
 
/***********************************************************************
 *           DIALOG_FindMsgDestination
 *
 * The messages that IsDialogMessage send may not go to the dialog
 * calling IsDialogMessage if that dialog is a child, and it has the
 * DS_CONTROL style set.
 * We propagate up until we hit a that does not have DS_CONTROL, or
 * whose parent is not a dialog.
 *
 * This is undocumented behaviour.
 */
static HWND DIALOG_FindMsgDestination( HWND hwndDlg )
{
    while (GetWindowLongA(hwndDlg, GWL_STYLE) & DS_CONTROL)
    {
	WND *pParent;
	HWND hParent = GetParent(hwndDlg);
	if (!hParent) break;

	pParent = WIN_FindWndPtr(hParent);
	if (!pParent) break;

	if (!(pParent->flags & WIN_ISDIALOG))
	{
	    WIN_ReleaseWndPtr(pParent);
	    break;
	}
	WIN_ReleaseWndPtr(pParent);

	hwndDlg = hParent;
    }

    return hwndDlg;
}

/***********************************************************************
 *           DIALOG_IsDialogMessage
 */
static BOOL DIALOG_IsDialogMessage( HWND hwnd, HWND hwndDlg,
                                      UINT message, WPARAM wParam,
                                      LPARAM lParam, BOOL *translate,
                                      BOOL *dispatch, INT dlgCode )
{
    *translate = *dispatch = FALSE;

    if (message == WM_PAINT)
    {
        /* Apparently, we have to handle this one as well */
        *dispatch = TRUE;
        return TRUE;
    }

      /* Only the key messages get special processing */
    if ((message != WM_KEYDOWN) &&
        (message != WM_SYSCHAR) &&
	(message != WM_CHAR))
        return FALSE;

    if (dlgCode & DLGC_WANTMESSAGE)
    {
        *translate = *dispatch = TRUE;
        return TRUE;
    }

    hwndDlg = DIALOG_FindMsgDestination(hwndDlg);

    switch(message)
    {
    case WM_KEYDOWN:
        switch(wParam)
        {
        case VK_TAB:
            if (!(dlgCode & DLGC_WANTTAB))
            {
                SendMessageA( hwndDlg, WM_NEXTDLGCTL,
                                (GetKeyState(VK_SHIFT) & 0x8000), 0 );
                return TRUE;
            }
            break;
            
        case VK_RIGHT:
        case VK_DOWN:
        case VK_LEFT:
        case VK_UP:
            if (!(dlgCode & DLGC_WANTARROWS))
            {
                BOOL fPrevious = (wParam == VK_LEFT || wParam == VK_UP);
                HWND hwndNext = 
                    GetNextDlgGroupItem (hwndDlg, GetFocus(), fPrevious );
                SendMessageA( hwndDlg, WM_NEXTDLGCTL, hwndNext, 1 );
                return TRUE;
            }
            break;

        case VK_ESCAPE:
            SendMessageA( hwndDlg, WM_COMMAND, IDCANCEL,
                            (LPARAM)GetDlgItem( hwndDlg, IDCANCEL ) );
            return TRUE;

        case VK_RETURN:
            {
                DWORD dw = SendMessage16( hwndDlg, DM_GETDEFID, 0, 0 );
                if (HIWORD(dw) == DC_HASDEFID)
                {
                    SendMessageA( hwndDlg, WM_COMMAND, 
                                    MAKEWPARAM( LOWORD(dw), BN_CLICKED ),
                                    (LPARAM)GetDlgItem(hwndDlg, LOWORD(dw)));
                }
                else
                {
                    SendMessageA( hwndDlg, WM_COMMAND, IDOK,
                                    (LPARAM)GetDlgItem( hwndDlg, IDOK ) );
    
                }
            }
            return TRUE;
        }
        *translate = TRUE;
        break; /* case WM_KEYDOWN */

    case WM_CHAR:
        if (dlgCode & DLGC_WANTCHARS) break;
        /* drop through */

    case WM_SYSCHAR:
        if (DIALOG_IsAccelerator( hwnd, hwndDlg, wParam ))
        {
            /* don't translate or dispatch */
            return TRUE;
        }
        break;
    }

    /* If we get here, the message has not been treated specially */
    /* and can be sent to its destination window. */
    *dispatch = TRUE;
    return TRUE;
}


/***********************************************************************
 *           IsDialogMessage16   (USER.90)
 */
BOOL16 WINAPI WIN16_IsDialogMessage16( HWND16 hwndDlg, SEGPTR msg16 )
{
    LPMSG16 msg = PTR_SEG_TO_LIN(msg16);
    BOOL ret, translate, dispatch;
    INT dlgCode = 0;

    if ((hwndDlg != msg->hwnd) && !IsChild16( hwndDlg, msg->hwnd ))
        return FALSE;

    if ((msg->message == WM_KEYDOWN) ||
        (msg->message == WM_SYSCHAR) ||
        (msg->message == WM_CHAR))
    {
       dlgCode = SendMessage16( msg->hwnd, WM_GETDLGCODE, 0, (LPARAM)msg16);
    }
    ret = DIALOG_IsDialogMessage( msg->hwnd, hwndDlg, msg->message,
                                  msg->wParam, msg->lParam,
                                  &translate, &dispatch, dlgCode );
    if (translate) TranslateMessage16( msg );
    if (dispatch) DispatchMessage16( msg );
    return ret;
}


BOOL16 WINAPI IsDialogMessage16( HWND16 hwndDlg, LPMSG16 msg )
{
    LPMSG16 msg16 = SEGPTR_NEW(MSG16);
    BOOL ret;

    *msg16 = *msg;
    ret = WIN16_IsDialogMessage16( hwndDlg, SEGPTR_GET(msg16) );
    SEGPTR_FREE(msg16);
    return ret;
}

/***********************************************************************
 *           IsDialogMessageA   (USER32.342)
 */
BOOL WINAPI IsDialogMessageA( HWND hwndDlg, LPMSG msg )
{
    BOOL ret, translate, dispatch;
    INT dlgCode = 0;

    if ((hwndDlg != msg->hwnd) && !IsChild( hwndDlg, msg->hwnd ))
        return FALSE;

    if ((msg->message == WM_KEYDOWN) ||
        (msg->message == WM_SYSCHAR) ||
        (msg->message == WM_CHAR))
    {
        dlgCode = SendMessageA( msg->hwnd, WM_GETDLGCODE, 0, (LPARAM)msg);
    }
    ret = DIALOG_IsDialogMessage( msg->hwnd, hwndDlg, msg->message,
                                  msg->wParam, msg->lParam,
                                  &translate, &dispatch, dlgCode );
    if (translate) TranslateMessage( msg );
    if (dispatch) DispatchMessageA( msg );
    return ret;
}


/***********************************************************************
 *           IsDialogMessageW   (USER32.343)
 */
BOOL WINAPI IsDialogMessageW( HWND hwndDlg, LPMSG msg )
{
    BOOL ret, translate, dispatch;
    INT dlgCode = 0;

    if ((hwndDlg != msg->hwnd) && !IsChild( hwndDlg, msg->hwnd ))
        return FALSE;

    if ((msg->message == WM_KEYDOWN) ||
        (msg->message == WM_SYSCHAR) ||
        (msg->message == WM_CHAR))
    {
        dlgCode = SendMessageW( msg->hwnd, WM_GETDLGCODE, 0, (LPARAM)msg);
    }
    ret = DIALOG_IsDialogMessage( msg->hwnd, hwndDlg, msg->message,
                                  msg->wParam, msg->lParam,
                                  &translate, &dispatch, dlgCode );
    if (translate) TranslateMessage( msg );
    if (dispatch) DispatchMessageW( msg );
    return ret;
}


/****************************************************************
 *         GetDlgCtrlID16   (USER.277)
 */
INT16 WINAPI GetDlgCtrlID16( HWND16 hwnd )
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    INT16 retvalue;
    
    if (!wndPtr) return 0;

    retvalue = wndPtr->wIDmenu;
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}
 

/****************************************************************
 *         GetDlgCtrlID   (USER32.234)
 */
INT WINAPI GetDlgCtrlID( HWND hwnd )
{
    INT retvalue;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    if (!wndPtr) return 0;
    retvalue = wndPtr->wIDmenu;
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}
 

/***********************************************************************
 *           GetDlgItem16   (USER.91)
 */
HWND16 WINAPI GetDlgItem16( HWND16 hwndDlg, INT16 id )
{
    WND *pWnd;

    if (!(pWnd = WIN_FindWndPtr( hwndDlg ))) return 0;
    for (WIN_UpdateWndPtr(&pWnd,pWnd->child); pWnd; WIN_UpdateWndPtr(&pWnd,pWnd->next))
        if (pWnd->wIDmenu == (UINT16)id)
        {
            HWND16 retvalue = pWnd->hwndSelf;
            WIN_ReleaseWndPtr(pWnd);
            return retvalue;
        }
    return 0;
}


/***********************************************************************
 *           GetDlgItem   (USER32.235)
 */
HWND WINAPI GetDlgItem( HWND hwndDlg, INT id )
{
    WND *pWnd;

    if (!(pWnd = WIN_FindWndPtr( hwndDlg ))) return 0;
    for (WIN_UpdateWndPtr(&pWnd,pWnd->child); pWnd;WIN_UpdateWndPtr(&pWnd,pWnd->next))
        if (pWnd->wIDmenu == (UINT16)id)
        {
            HWND retvalue = pWnd->hwndSelf;
            WIN_ReleaseWndPtr(pWnd);
            return retvalue;
        }
    return 0;
}


/*******************************************************************
 *           SendDlgItemMessage16   (USER.101)
 */
LRESULT WINAPI SendDlgItemMessage16( HWND16 hwnd, INT16 id, UINT16 msg,
                                     WPARAM16 wParam, LPARAM lParam )
{
    HWND16 hwndCtrl = GetDlgItem16( hwnd, id );
    if (hwndCtrl) return SendMessage16( hwndCtrl, msg, wParam, lParam );
    else return 0;
}


/*******************************************************************
 *           SendDlgItemMessageA   (USER32.452)
 */
LRESULT WINAPI SendDlgItemMessageA( HWND hwnd, INT id, UINT msg,
                                      WPARAM wParam, LPARAM lParam )
{
    HWND hwndCtrl = GetDlgItem( hwnd, id );
    if (hwndCtrl) return SendMessageA( hwndCtrl, msg, wParam, lParam );
    else return 0;
}


/*******************************************************************
 *           SendDlgItemMessageW   (USER32.453)
 */
LRESULT WINAPI SendDlgItemMessageW( HWND hwnd, INT id, UINT msg,
                                      WPARAM wParam, LPARAM lParam )
{
    HWND hwndCtrl = GetDlgItem( hwnd, id );
    if (hwndCtrl) return SendMessageW( hwndCtrl, msg, wParam, lParam );
    else return 0;
}


/*******************************************************************
 *           SetDlgItemText16   (USER.92)
 */
void WINAPI SetDlgItemText16( HWND16 hwnd, INT16 id, SEGPTR lpString )
{
    SendDlgItemMessage16( hwnd, id, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *           SetDlgItemTextA   (USER32.478)
 */
BOOL WINAPI SetDlgItemTextA( HWND hwnd, INT id, LPCSTR lpString )
{
    return SendDlgItemMessageA( hwnd, id, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *           SetDlgItemTextW   (USER32.479)
 */
BOOL WINAPI SetDlgItemTextW( HWND hwnd, INT id, LPCWSTR lpString )
{
    return SendDlgItemMessageW( hwnd, id, WM_SETTEXT, 0, (LPARAM)lpString );
}


/***********************************************************************
 *           GetDlgItemText16   (USER.93)
 */
INT16 WINAPI GetDlgItemText16( HWND16 hwnd, INT16 id, SEGPTR str, UINT16 len )
{
    return (INT16)SendDlgItemMessage16( hwnd, id, WM_GETTEXT,
                                        len, (LPARAM)str );
}


/***********************************************************************
 *           GetDlgItemTextA   (USER32.237)
 */
INT WINAPI GetDlgItemTextA( HWND hwnd, INT id, LPSTR str, UINT len )
{
    return (INT)SendDlgItemMessageA( hwnd, id, WM_GETTEXT,
                                         len, (LPARAM)str );
}


/***********************************************************************
 *           GetDlgItemTextW   (USER32.238)
 */
INT WINAPI GetDlgItemTextW( HWND hwnd, INT id, LPWSTR str, UINT len )
{
    return (INT)SendDlgItemMessageW( hwnd, id, WM_GETTEXT,
                                         len, (LPARAM)str );
}


/*******************************************************************
 *           SetDlgItemInt16   (USER.94)
 */
void WINAPI SetDlgItemInt16( HWND16 hwnd, INT16 id, UINT16 value, BOOL16 fSigned )
{
    SetDlgItemInt( hwnd, (UINT)(UINT16)id, value, fSigned );
}


/*******************************************************************
 *           SetDlgItemInt   (USER32.477)
 */
BOOL WINAPI SetDlgItemInt( HWND hwnd, INT id, UINT value,
                             BOOL fSigned )
{
    char str[20];

    if (fSigned) sprintf( str, "%d", (INT)value );
    else sprintf( str, "%u", value );
    SendDlgItemMessageA( hwnd, id, WM_SETTEXT, 0, (LPARAM)str );
    return TRUE;
}


/***********************************************************************
 *           GetDlgItemInt16   (USER.95)
 */
UINT16 WINAPI GetDlgItemInt16( HWND16 hwnd, INT16 id, BOOL16 *translated,
                               BOOL16 fSigned )
{
    UINT result;
    BOOL ok;

    if (translated) *translated = FALSE;
    result = GetDlgItemInt( hwnd, (UINT)(UINT16)id, &ok, fSigned );
    if (!ok) return 0;
    if (fSigned)
    {
        if (((INT)result < -32767) || ((INT)result > 32767)) return 0;
    }
    else
    {
        if (result > 65535) return 0;
    }
    if (translated) *translated = TRUE;
    return (UINT16)result;
}


/***********************************************************************
 *           GetDlgItemInt   (USER32.236)
 */
UINT WINAPI GetDlgItemInt( HWND hwnd, INT id, BOOL *translated,
                               BOOL fSigned )
{
    char str[30];
    char * endptr;
    long result = 0;
    
    if (translated) *translated = FALSE;
    if (!SendDlgItemMessageA(hwnd, id, WM_GETTEXT, sizeof(str), (LPARAM)str))
        return 0;
    if (fSigned)
    {
        result = strtol( str, &endptr, 10 );
        if (!endptr || (endptr == str))  /* Conversion was unsuccessful */
            return 0;
        if (((result == LONG_MIN) || (result == LONG_MAX)) && (errno==ERANGE))
            return 0;
    }
    else
    {
        result = strtoul( str, &endptr, 10 );
        if (!endptr || (endptr == str))  /* Conversion was unsuccessful */
            return 0;
        if ((result == ULONG_MAX) && (errno == ERANGE)) return 0;
    }
    if (translated) *translated = TRUE;
    return (UINT)result;
}


/***********************************************************************
 *           CheckDlgButton16   (USER.97)
 */
BOOL16 WINAPI CheckDlgButton16( HWND16 hwnd, INT16 id, UINT16 check )
{
    SendDlgItemMessageA( hwnd, id, BM_SETCHECK, check, 0 );
    return TRUE;
}


/***********************************************************************
 *           CheckDlgButton   (USER32.45)
 */
BOOL WINAPI CheckDlgButton( HWND hwnd, INT id, UINT check )
{
    SendDlgItemMessageA( hwnd, id, BM_SETCHECK, check, 0 );
    return TRUE;
}


/***********************************************************************
 *           IsDlgButtonChecked16   (USER.98)
 */
UINT16 WINAPI IsDlgButtonChecked16( HWND16 hwnd, UINT16 id )
{
    return (UINT16)SendDlgItemMessageA( hwnd, id, BM_GETCHECK, 0, 0 );
}


/***********************************************************************
 *           IsDlgButtonChecked   (USER32.344)
 */
UINT WINAPI IsDlgButtonChecked( HWND hwnd, UINT id )
{
    return (UINT)SendDlgItemMessageA( hwnd, id, BM_GETCHECK, 0, 0 );
}


/***********************************************************************
 *           CheckRadioButton16   (USER.96)
 */
BOOL16 WINAPI CheckRadioButton16( HWND16 hwndDlg, UINT16 firstID,
                                  UINT16 lastID, UINT16 checkID )
{
    return CheckRadioButton( hwndDlg, firstID, lastID, checkID );
}


/***********************************************************************
 *           CheckRB
 * 
 * Callback function used to check/uncheck radio buttons that fall 
 * within a specific range of IDs.
 */
static BOOL CALLBACK CheckRB(HWND hwndChild, LPARAM lParam)
{
    LONG lChildID = GetWindowLongA(hwndChild, GWL_ID);
    RADIOGROUP *lpRadioGroup = (RADIOGROUP *) lParam;

    if ((lChildID >= lpRadioGroup->firstID) && 
        (lChildID <= lpRadioGroup->lastID))
    {
        if (lChildID == lpRadioGroup->checkID)
        {
            SendMessageA(hwndChild, BM_SETCHECK, BST_CHECKED, 0);
        }
        else
        {
            SendMessageA(hwndChild, BM_SETCHECK, BST_UNCHECKED, 0);
        }
    }

    return TRUE;
}


/***********************************************************************
 *           CheckRadioButton   (USER32.48)
 */
BOOL WINAPI CheckRadioButton( HWND hwndDlg, UINT firstID,
                              UINT lastID, UINT checkID )
{
    RADIOGROUP radioGroup;

    /* perform bounds checking for a radio button group */
    radioGroup.firstID = min(min(firstID, lastID), checkID);
    radioGroup.lastID = max(max(firstID, lastID), checkID);
    radioGroup.checkID = checkID;
    
    return EnumChildWindows(hwndDlg, (WNDENUMPROC)CheckRB, 
                            (LPARAM)&radioGroup);
}


/***********************************************************************
 *           GetDialogBaseUnits   (USER.243) (USER32.233)
 */
DWORD WINAPI GetDialogBaseUnits(void)
{
    return MAKELONG( xBaseUnit, yBaseUnit );
}


/***********************************************************************
 *           MapDialogRect16   (USER.103)
 */
void WINAPI MapDialogRect16( HWND16 hwnd, LPRECT16 rect )
{
    DIALOGINFO * dlgInfo;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return;
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    rect->left   = MulDiv(rect->left, dlgInfo->xBaseUnit, 4);
    rect->right  = MulDiv(rect->right, dlgInfo->xBaseUnit, 4);
    rect->top    = MulDiv(rect->top, dlgInfo->yBaseUnit, 8);
    rect->bottom = MulDiv(rect->bottom, dlgInfo->yBaseUnit, 8);
    WIN_ReleaseWndPtr(wndPtr);
}


/***********************************************************************
 *           MapDialogRect   (USER32.382)
 */
BOOL WINAPI MapDialogRect( HWND hwnd, LPRECT rect )
{
    DIALOGINFO * dlgInfo;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    rect->left   = MulDiv(rect->left, dlgInfo->xBaseUnit, 4);
    rect->right  = MulDiv(rect->right, dlgInfo->xBaseUnit, 4);
    rect->top    = MulDiv(rect->top, dlgInfo->yBaseUnit, 8);
    rect->bottom = MulDiv(rect->bottom, dlgInfo->yBaseUnit, 8);
    WIN_ReleaseWndPtr(wndPtr);
    return TRUE;
}


/***********************************************************************
 *           GetNextDlgGroupItem16   (USER.227)
 */
HWND16 WINAPI GetNextDlgGroupItem16( HWND16 hwndDlg, HWND16 hwndCtrl,
                                     BOOL16 fPrevious )
{
    return (HWND16)GetNextDlgGroupItem( hwndDlg, hwndCtrl, fPrevious );
}


/***********************************************************************
 *           GetNextDlgGroupItem   (USER32.275)
 */
HWND WINAPI GetNextDlgGroupItem( HWND hwndDlg, HWND hwndCtrl,
                                     BOOL fPrevious )
{
    WND *pWnd = NULL,
        *pWndLast = NULL,
        *pWndCtrl = NULL,
        *pWndDlg = NULL;
    HWND retvalue;

    if(hwndCtrl)
    {
        /* if the hwndCtrl is the child of the control in the hwndDlg then the hwndDlg has to be the parent of the hwndCtrl */
        if(GetParent(hwndCtrl) != hwndDlg && GetParent(GetParent(hwndCtrl)) == hwndDlg)
            hwndDlg = GetParent(hwndCtrl);
    }

    if (!(pWndDlg = WIN_FindWndPtr( hwndDlg ))) return 0;
    if (hwndCtrl)
    {
        if (!(pWndCtrl = WIN_FindWndPtr( hwndCtrl )))
    {
            retvalue = 0;
            goto END;
        }
        /* Make sure hwndCtrl is a top-level child */
        while ((pWndCtrl->dwStyle & WS_CHILD) && (pWndCtrl->parent != pWndDlg))
            WIN_UpdateWndPtr(&pWndCtrl,pWndCtrl->parent);
        if (pWndCtrl->parent != pWndDlg)
        {
            retvalue = 0;
            goto END;
        }
    }
    else
    {
        /* No ctrl specified -> start from the beginning */
        if (!(pWndCtrl = WIN_LockWndPtr(pWndDlg->child)))
        {
            retvalue = 0;
            goto END;
        }
        if (fPrevious)
            while (pWndCtrl->next) WIN_UpdateWndPtr(&pWndCtrl,pWndCtrl->next);
    }

    pWndLast = WIN_LockWndPtr(pWndCtrl);
    pWnd = WIN_LockWndPtr(pWndCtrl->next);

    while (1)
    {
        if (!pWnd || (pWnd->dwStyle & WS_GROUP))
        {
            /* Wrap-around to the beginning of the group */
            WND *pWndTemp;

            WIN_UpdateWndPtr( &pWnd, pWndDlg->child );
            for ( pWndTemp = WIN_LockWndPtr( pWnd ); 
                  pWndTemp;
                  WIN_UpdateWndPtr( &pWndTemp, pWndTemp->next) )
            {
                if (pWndTemp->dwStyle & WS_GROUP) WIN_UpdateWndPtr( &pWnd, pWndTemp );
                if (pWndTemp == pWndCtrl) break;
            }
            WIN_ReleaseWndPtr( pWndTemp );
        }
        if (pWnd == pWndCtrl) break;
	if ((pWnd->dwStyle & WS_VISIBLE) && !(pWnd->dwStyle & WS_DISABLED))
	{
            WIN_UpdateWndPtr(&pWndLast,pWnd);
	    if (!fPrevious) break;
	}
        WIN_UpdateWndPtr(&pWnd,pWnd->next);
    }
    retvalue = pWndLast->hwndSelf;

    WIN_ReleaseWndPtr(pWndLast);
    WIN_ReleaseWndPtr(pWnd);
END:
    WIN_ReleaseWndPtr(pWndCtrl);
    WIN_ReleaseWndPtr(pWndDlg);

    return retvalue;
}


/***********************************************************************
 *           GetNextDlgTabItem16   (USER.228)
 */
HWND16 WINAPI GetNextDlgTabItem16( HWND16 hwndDlg, HWND16 hwndCtrl,
                                   BOOL16 fPrevious )
{
    return (HWND16)GetNextDlgTabItem( hwndDlg, hwndCtrl, fPrevious );
}

/***********************************************************************
 *           DIALOG_GetNextTabItem
 *
 * Helper for GetNextDlgTabItem
 */
static HWND DIALOG_GetNextTabItem( HWND hwndMain, HWND hwndDlg, HWND hwndCtrl, BOOL fPrevious )
{
    LONG dsStyle;
    LONG exStyle;
    UINT wndSearch = fPrevious ? GW_HWNDPREV : GW_HWNDNEXT;
    HWND retWnd = 0;
    HWND hChildFirst = 0;

    if(!hwndCtrl) 
    {
        hChildFirst = GetWindow(hwndDlg,GW_CHILD);
        if(fPrevious) hChildFirst = GetWindow(hChildFirst,GW_HWNDLAST);
    }
    else
    {
        HWND hParent = GetParent(hwndCtrl);
        BOOL bValid = FALSE;
        while( hParent)
        {
            if(hParent == hwndMain)
            {
                bValid = TRUE;
                break;
            }
            hParent = GetParent(hParent);
        }
        if(bValid)
        {
            hChildFirst = GetWindow(hwndCtrl,wndSearch);
            if(!hChildFirst)
            {
                if(GetParent(hwndCtrl) != hwndMain)
                    hChildFirst = GetWindow(GetParent(hwndCtrl),wndSearch);
                else
                {
                    if(fPrevious)
                        hChildFirst = GetWindow(hwndCtrl,GW_HWNDLAST);
                    else
                        hChildFirst = GetWindow(hwndCtrl,GW_HWNDFIRST);
                }
            }
        }	
    }
    while(hChildFirst)
    {
        BOOL bCtrl = FALSE;
        while(hChildFirst)
        {
            dsStyle = GetWindowLongA(hChildFirst,GWL_STYLE);
            exStyle = GetWindowLongA(hChildFirst,GWL_EXSTYLE);
            if( (dsStyle & DS_CONTROL || exStyle & WS_EX_CONTROLPARENT) && (dsStyle & WS_VISIBLE) && !(dsStyle & WS_DISABLED))
            {
                bCtrl=TRUE;
                break;
            }
            else if( (dsStyle & WS_TABSTOP) && (dsStyle & WS_VISIBLE) && !(dsStyle & WS_DISABLED))
                break;
            hChildFirst = GetWindow(hChildFirst,wndSearch);
        }
        if(hChildFirst)
        {
            if(bCtrl)
                retWnd = DIALOG_GetNextTabItem(hwndMain,hChildFirst,(HWND)NULL,fPrevious );
            else
                retWnd = hChildFirst;
        }
        if(retWnd) break;
        hChildFirst = GetWindow(hChildFirst,wndSearch);
    }
    if(!retWnd && hwndCtrl)
    {
        HWND hParent = GetParent(hwndCtrl);
        while(hParent)
	{
            if(hParent == hwndMain) break;
            retWnd = DIALOG_GetNextTabItem(hwndMain,GetParent(hParent),hParent,fPrevious );
            if(retWnd) break;
            hParent = GetParent(hParent);
	}
        if(!retWnd)
            retWnd = DIALOG_GetNextTabItem(hwndMain,hwndMain,(HWND)NULL,fPrevious );
    }
    return retWnd;
}

/***********************************************************************
 *           GetNextDlgTabItem   (USER32.276)
 */
HWND WINAPI GetNextDlgTabItem( HWND hwndDlg, HWND hwndCtrl,
                                   BOOL fPrevious )
{
    return DIALOG_GetNextTabItem(hwndDlg,hwndDlg,hwndCtrl,fPrevious); 
}

/**********************************************************************
 *           DIALOG_DlgDirSelect
 *
 * Helper function for DlgDirSelect*
 */
static BOOL DIALOG_DlgDirSelect( HWND hwnd, LPSTR str, INT len,
                                   INT id, BOOL win32, BOOL unicode,
                                   BOOL combo )
{
    char *buffer, *ptr;
    INT item, size;
    BOOL ret;
    HWND listbox = GetDlgItem( hwnd, id );

    TRACE("%04x '%s' %d\n", hwnd, str, id );
    if (!listbox) return FALSE;
    if (win32)
    {
        item = SendMessageA(listbox, combo ? CB_GETCURSEL
                                             : LB_GETCURSEL, 0, 0 );
        if (item == LB_ERR) return FALSE;
        size = SendMessageA(listbox, combo ? CB_GETLBTEXTLEN
                                             : LB_GETTEXTLEN, 0, 0 );
        if (size == LB_ERR) return FALSE;
    }
    else
    {
        item = SendMessageA(listbox, combo ? CB_GETCURSEL16
                                             : LB_GETCURSEL16, 0, 0 );
        if (item == LB_ERR) return FALSE;
        size = SendMessageA(listbox, combo ? CB_GETLBTEXTLEN16
                                             : LB_GETTEXTLEN16, 0, 0 );
        if (size == LB_ERR) return FALSE;
    }

    if (!(buffer = SEGPTR_ALLOC( size+1 ))) return FALSE;

    if (win32)
        SendMessageA( listbox, combo ? CB_GETLBTEXT : LB_GETTEXT,
                        item, (LPARAM)buffer );
    else
        SendMessage16( listbox, combo ? CB_GETLBTEXT16 : LB_GETTEXT16,
                       item, (LPARAM)SEGPTR_GET(buffer) );

    if ((ret = (buffer[0] == '[')))  /* drive or directory */
    {
        if (buffer[1] == '-')  /* drive */
        {
            buffer[3] = ':';
            buffer[4] = 0;
            ptr = buffer + 2;
        }
        else
        {
            buffer[strlen(buffer)-1] = '\\';
            ptr = buffer + 1;
        }
    }
    else ptr = buffer;

    if (unicode) lstrcpynAtoW( (LPWSTR)str, ptr, len );
    else lstrcpynA( str, ptr, len );
    SEGPTR_FREE( buffer );
    TRACE("Returning %d '%s'\n", ret, str );
    return ret;
}


/**********************************************************************
 *	    DIALOG_DlgDirList
 *
 * Helper function for DlgDirList*
 */
static INT DIALOG_DlgDirList( HWND hDlg, LPSTR spec, INT idLBox,
                                INT idStatic, UINT attrib, BOOL combo )
{
    int drive;
    HWND hwnd;
    LPSTR orig_spec = spec;

#define SENDMSG(msg,wparam,lparam) \
    ((attrib & DDL_POSTMSGS) ? PostMessageA( hwnd, msg, wparam, lparam ) \
                             : SendMessageA( hwnd, msg, wparam, lparam ))

    TRACE("%04x '%s' %d %d %04x\n",
                    hDlg, spec ? spec : "NULL", idLBox, idStatic, attrib );

    if (spec && spec[0] && (spec[1] == ':'))
    {
        drive = toupper( spec[0] ) - 'A';
        spec += 2;
        if (!DRIVE_SetCurrentDrive( drive )) return FALSE;
    }
    else drive = DRIVE_GetCurrentDrive();

    /* If the path exists and is a directory, chdir to it */
    if (!spec || !spec[0] || DRIVE_Chdir( drive, spec )) spec = "*.*";
    else
    {
        char *p, *p2;
        p = spec;
        if ((p2 = strrchr( p, '\\' ))) p = p2;
        if ((p2 = strrchr( p, '/' ))) p = p2;
        if (p != spec)
        {
            char sep = *p;
            *p = 0;
            if (!DRIVE_Chdir( drive, spec ))
            {
                *p = sep;  /* Restore the original spec */
                return FALSE;
            }
            spec = p + 1;
        }
    }

    TRACE("path=%c:\\%s mask=%s\n",
                    'A' + drive, DRIVE_GetDosCwd(drive), spec );

    if (idLBox && ((hwnd = GetDlgItem( hDlg, idLBox )) != 0))
    {
        SENDMSG( combo ? CB_RESETCONTENT : LB_RESETCONTENT, 0, 0 );
        if (attrib & DDL_DIRECTORY)
        {
            if (!(attrib & DDL_EXCLUSIVE))
            {
                if (SENDMSG( combo ? CB_DIR : LB_DIR,
                             attrib & ~(DDL_DIRECTORY | DDL_DRIVES),
                             (LPARAM)spec ) == LB_ERR)
                    return FALSE;
            }
            if (SENDMSG( combo ? CB_DIR : LB_DIR,
                       (attrib & (DDL_DIRECTORY | DDL_DRIVES)) | DDL_EXCLUSIVE,
                         (LPARAM)"*.*" ) == LB_ERR)
                return FALSE;
        }
        else
        {
            if (SENDMSG( combo ? CB_DIR : LB_DIR, attrib,
                         (LPARAM)spec ) == LB_ERR)
                return FALSE;
        }
    }

    if (idStatic && ((hwnd = GetDlgItem( hDlg, idStatic )) != 0))
    {
        char temp[512];
        int drive = DRIVE_GetCurrentDrive();
        strcpy( temp, "A:\\" );
        temp[0] += drive;
        lstrcpynA( temp + 3, DRIVE_GetDosCwd(drive), sizeof(temp)-3 );
        CharLowerA( temp );
        /* Can't use PostMessage() here, because the string is on the stack */
        SetDlgItemTextA( hDlg, idStatic, temp );
    }

    if (orig_spec && (spec != orig_spec))
    {
        /* Update the original file spec */
        char *p = spec;
        while ((*orig_spec++ = *p++));
    }

    return TRUE;
#undef SENDMSG
}


/**********************************************************************
 *	    DIALOG_DlgDirListW
 *
 * Helper function for DlgDirList*W
 */
static INT DIALOG_DlgDirListW( HWND hDlg, LPWSTR spec, INT idLBox,
                                 INT idStatic, UINT attrib, BOOL combo )
{
    if (spec)
    {
        LPSTR specA = HEAP_strdupWtoA( GetProcessHeap(), 0, spec );
        INT ret = DIALOG_DlgDirList( hDlg, specA, idLBox, idStatic,
                                       attrib, combo );
        lstrcpyAtoW( spec, specA );
        HeapFree( GetProcessHeap(), 0, specA );
        return ret;
    }
    return DIALOG_DlgDirList( hDlg, NULL, idLBox, idStatic, attrib, combo );
}


/**********************************************************************
 *	    DlgDirSelect    (USER.99)
 */
BOOL16 WINAPI DlgDirSelect16( HWND16 hwnd, LPSTR str, INT16 id )
{
    return DlgDirSelectEx16( hwnd, str, 128, id );
}


/**********************************************************************
 *	    DlgDirSelectComboBox    (USER.194)
 */
BOOL16 WINAPI DlgDirSelectComboBox16( HWND16 hwnd, LPSTR str, INT16 id )
{
    return DlgDirSelectComboBoxEx16( hwnd, str, 128, id );
}


/**********************************************************************
 *           DlgDirSelectEx16    (USER.422)
 */
BOOL16 WINAPI DlgDirSelectEx16( HWND16 hwnd, LPSTR str, INT16 len, INT16 id )
{
    return DIALOG_DlgDirSelect( hwnd, str, len, id, FALSE, FALSE, FALSE );
}


/**********************************************************************
 *           DlgDirSelectExA    (USER32.149)
 */
BOOL WINAPI DlgDirSelectExA( HWND hwnd, LPSTR str, INT len, INT id )
{
    return DIALOG_DlgDirSelect( hwnd, str, len, id, TRUE, FALSE, FALSE );
}


/**********************************************************************
 *           DlgDirSelectExW    (USER32.150)
 */
BOOL WINAPI DlgDirSelectExW( HWND hwnd, LPWSTR str, INT len, INT id )
{
    return DIALOG_DlgDirSelect( hwnd, (LPSTR)str, len, id, TRUE, TRUE, FALSE );
}


/**********************************************************************
 *           DlgDirSelectComboBoxEx16    (USER.423)
 */
BOOL16 WINAPI DlgDirSelectComboBoxEx16( HWND16 hwnd, LPSTR str, INT16 len,
                                        INT16 id )
{
    return DIALOG_DlgDirSelect( hwnd, str, len, id, FALSE, FALSE, TRUE );
}


/**********************************************************************
 *           DlgDirSelectComboBoxExA    (USER32.147)
 */
BOOL WINAPI DlgDirSelectComboBoxExA( HWND hwnd, LPSTR str, INT len,
                                         INT id )
{
    return DIALOG_DlgDirSelect( hwnd, str, len, id, TRUE, FALSE, TRUE );
}


/**********************************************************************
 *           DlgDirSelectComboBoxExW    (USER32.148)
 */
BOOL WINAPI DlgDirSelectComboBoxExW( HWND hwnd, LPWSTR str, INT len,
                                         INT id)
{
    return DIALOG_DlgDirSelect( hwnd, (LPSTR)str, len, id, TRUE, TRUE, TRUE );
}


/**********************************************************************
 *	    DlgDirList16    (USER.100)
 */
INT16 WINAPI DlgDirList16( HWND16 hDlg, LPSTR spec, INT16 idLBox,
                           INT16 idStatic, UINT16 attrib )
{
    return DIALOG_DlgDirList( hDlg, spec, idLBox, idStatic, attrib, FALSE );
}


/**********************************************************************
 *	    DlgDirListA    (USER32.143)
 */
INT WINAPI DlgDirListA( HWND hDlg, LPSTR spec, INT idLBox,
                            INT idStatic, UINT attrib )
{
    return DIALOG_DlgDirList( hDlg, spec, idLBox, idStatic, attrib, FALSE );
}


/**********************************************************************
 *	    DlgDirListW    (USER32.146)
 */
INT WINAPI DlgDirListW( HWND hDlg, LPWSTR spec, INT idLBox,
                            INT idStatic, UINT attrib )
{
    return DIALOG_DlgDirListW( hDlg, spec, idLBox, idStatic, attrib, FALSE );
}


/**********************************************************************
 *	    DlgDirListComboBox16    (USER.195)
 */
INT16 WINAPI DlgDirListComboBox16( HWND16 hDlg, LPSTR spec, INT16 idCBox,
                                   INT16 idStatic, UINT16 attrib )
{
    return DIALOG_DlgDirList( hDlg, spec, idCBox, idStatic, attrib, TRUE );
}


/**********************************************************************
 *	    DlgDirListComboBoxA    (USER32.144)
 */
INT WINAPI DlgDirListComboBoxA( HWND hDlg, LPSTR spec, INT idCBox,
                                    INT idStatic, UINT attrib )
{
    return DIALOG_DlgDirList( hDlg, spec, idCBox, idStatic, attrib, TRUE );
}


/**********************************************************************
 *	    DlgDirListComboBoxW    (USER32.145)
 */
INT WINAPI DlgDirListComboBoxW( HWND hDlg, LPWSTR spec, INT idCBox,
                                    INT idStatic, UINT attrib )
{
    return DIALOG_DlgDirListW( hDlg, spec, idCBox, idStatic, attrib, TRUE );
}
