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
#include <errno.h>
#include "windows.h"
#include "dialog.h"
#include "drive.h"
#include "heap.h"
#include "win.h"
#include "ldt.h"
#include "user.h"
#include "winproc.h"
#include "message.h"
#include "sysmetrics.h"
#include "debug.h"


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
    UINT32     id;
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
    BOOL32     italic;
    LPCSTR     faceName;
    BOOL32     dialogEx;
} DLG_TEMPLATE;

  /* Dialog base units */
static WORD xBaseUnit = 0, yBaseUnit = 0;


/***********************************************************************
 *           DIALOG_Init
 *
 * Initialisation of the dialog manager.
 */
BOOL32 DIALOG_Init(void)
{
    TEXTMETRIC16 tm;
    HDC16 hdc;
    
      /* Calculate the dialog base units */

    if (!(hdc = CreateDC16( "DISPLAY", NULL, NULL, NULL ))) return FALSE;
    GetTextMetrics16( hdc, &tm );
    DeleteDC32( hdc );
    xBaseUnit = tm.tmAveCharWidth;
    yBaseUnit = tm.tmHeight;

      /* Dialog units are based on a proportional system font */
      /* so we adjust them a bit for a fixed font. */
    if (!(tm.tmPitchAndFamily & TMPF_FIXED_PITCH))
        xBaseUnit = xBaseUnit * 5 / 4;

    TRACE(dialog, "base units = %d,%d\n",
                    xBaseUnit, yBaseUnit );
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
	info->windowName = (LPCSTR)(UINT32)GET_WORD(p+1);
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
      TRACE(dialog,"   %s %04x %d, %d, %d, %d, %d, %08lx, %08lx\n", 
		      info->className,  LOWORD(info->windowName),
		      info->id, info->x, info->y, info->cx, info->cy,
		      info->style, (DWORD)info->data);
    else
      TRACE(dialog,"   %s '%s' %d, %d, %d, %d, %d, %08lx, %08lx\n", 
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
                                        BOOL32 dialogEx )
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
            ERR( dialog, "Unknown built-in class id %04x\n", id );
        }
        p += 2;
    }
    else
    {
        info->className = (LPCSTR)p;
        p += lstrlen32W( (LPCWSTR)p ) + 1;
    }

    if (GET_WORD(p) == 0xffff)  /* Is it an integer id? */
    {
	info->windowName = (LPCSTR)(UINT32)GET_WORD(p + 1);
	p += 2;
    }
    else
    {
	info->windowName = (LPCSTR)p;
        p += lstrlen32W( (LPCWSTR)p ) + 1;
    }

    TRACE(dialog,"    %s %s %d, %d, %d, %d, %d, %08lx, %08lx, %08lx\n", 
          debugstr_w( (LPCWSTR)info->className ),
          debugres_w( (LPCWSTR)info->windowName ),
          info->id, info->x, info->y, info->cx, info->cy,
          info->style, info->exStyle, info->helpId );

    if (GET_WORD(p))
    {
        if (TRACE_ON(dialog))
        {
            WORD i, count = GET_WORD(p) / sizeof(WORD);
            TRACE(dialog, "  BEGIN\n");
            TRACE(dialog, "    ");
            for (i = 0; i < count; i++) DUMP( "%04x,", GET_WORD(p+i+1) );
            DUMP("\n");
            TRACE(dialog, "  END\n" );
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
static BOOL32 DIALOG_CreateControls( WND *pWnd, LPCSTR template,
                                     const DLG_TEMPLATE *dlgTemplate,
                                     HINSTANCE32 hInst, BOOL32 win32 )
{
    DIALOGINFO *dlgInfo = (DIALOGINFO *)pWnd->wExtra;
    DLG_CONTROL_INFO info;
    HWND32 hwndCtrl, hwndDefButton = 0;
    INT32 items = dlgTemplate->nbItems;

    TRACE(dialog, " BEGIN\n" );
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
                        fprintf( stderr, "CreateDialogIndirectParam: Insufficient memory to create heap for edit control\n" );
                        continue;
                    }
                    LocalInit(dlgInfo->hDialogHeap, 0, 0xffff);
                }
                instance = dlgInfo->hDialogHeap;
            }
            else instance = (HINSTANCE16)hInst;

            hwndCtrl = CreateWindowEx16( info.exStyle | WS_EX_NOPARENTNOTIFY,
                                         info.className, info.windowName,
                                         info.style | WS_CHILD,
                                         info.x * dlgInfo->xBaseUnit / 4,
                                         info.y * dlgInfo->yBaseUnit / 8,
                                         info.cx * dlgInfo->xBaseUnit / 4,
                                         info.cy * dlgInfo->yBaseUnit / 8,
                                         pWnd->hwndSelf, (HMENU16)info.id,
                                         instance, info.data );
        }
        else
        {
            template = (LPCSTR)DIALOG_GetControl32( (WORD *)template, &info,
                                                    dlgTemplate->dialogEx );
            hwndCtrl = CreateWindowEx32W( info.exStyle | WS_EX_NOPARENTNOTIFY,
                                          (LPCWSTR)info.className,
                                          (LPCWSTR)info.windowName,
                                          info.style | WS_CHILD,
                                          info.x * dlgInfo->xBaseUnit / 4,
                                          info.y * dlgInfo->yBaseUnit / 8,
                                          info.cx * dlgInfo->xBaseUnit / 4,
                                          info.cy * dlgInfo->yBaseUnit / 8,
                                          pWnd->hwndSelf, (HMENU32)info.id,
                                          hInst, info.data );
        }
        if (!hwndCtrl) return FALSE;

            /* Send initialisation messages to the control */
        if (dlgInfo->hUserFont) SendMessage32A( hwndCtrl, WM_SETFONT,
                                             (WPARAM32)dlgInfo->hUserFont, 0 );
        if (SendMessage32A(hwndCtrl, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON)
        {
              /* If there's already a default push-button, set it back */
              /* to normal and use this one instead. */
            if (hwndDefButton)
                SendMessage32A( hwndDefButton, BM_SETSTYLE32,
                                BS_PUSHBUTTON,FALSE );
            hwndDefButton = hwndCtrl;
            dlgInfo->idResult = GetWindowWord32( hwndCtrl, GWW_ID );
        }
    }    
    TRACE(dialog, " END\n" );
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
    result->nbItems = *p++;
    result->x       = GET_WORD(p);  p += sizeof(WORD);
    result->y       = GET_WORD(p);  p += sizeof(WORD);
    result->cx      = GET_WORD(p);  p += sizeof(WORD);
    result->cy      = GET_WORD(p);  p += sizeof(WORD);
    TRACE(dialog, "DIALOG %d, %d, %d, %d\n",
                    result->x, result->y, result->cx, result->cy );
    TRACE(dialog, " STYLE %08lx\n", result->style );

    /* Get the menu name */

    switch( (BYTE)*p )
    {
    case 0:
        result->menuName = 0;
        p++;
        break;
    case 0xff:
        result->menuName = (LPCSTR)(UINT32)GET_WORD( p + 1 );
        p += 3;
	TRACE(dialog, " MENU %04x\n", LOWORD(result->menuName) );
        break;
    default:
        result->menuName = p;
        TRACE(dialog, " MENU '%s'\n", p );
        p += strlen(p) + 1;
        break;
    }

    /* Get the class name */

    if (*p)
    {
        result->className = p;
        TRACE(dialog, " CLASS '%s'\n", result->className );
    }
    else result->className = DIALOG_CLASS_ATOM;
    p += strlen(p) + 1;

    /* Get the window caption */

    result->caption = p;
    p += strlen(p) + 1;
    TRACE(dialog, " CAPTION '%s'\n", result->caption );

    /* Get the font name */

    if (result->style & DS_SETFONT)
    {
	result->pointSize = GET_WORD(p);
        p += sizeof(WORD);
	result->faceName = p;
        p += strlen(p) + 1;
	TRACE(dialog, " FONT %d,'%s'\n",
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
    TRACE( dialog, "DIALOG%s %d, %d, %d, %d, %ld\n",
           result->dialogEx ? "EX" : "", result->x, result->y,
           result->cx, result->cy, result->helpId );
    TRACE( dialog, " STYLE 0x%08lx\n", result->style );
    TRACE( dialog, " EXSTYLE 0x%08lx\n", result->exStyle );

    /* Get the menu name */

    switch(GET_WORD(p))
    {
    case 0x0000:
        result->menuName = NULL;
        p++;
        break;
    case 0xffff:
        result->menuName = (LPCSTR)(UINT32)GET_WORD( p + 1 );
        p += 2;
	TRACE(dialog, " MENU %04x\n", LOWORD(result->menuName) );
        break;
    default:
        result->menuName = (LPCSTR)p;
        TRACE(dialog, " MENU %s\n", debugstr_w( (LPCWSTR)p ));
        p += lstrlen32W( (LPCWSTR)p ) + 1;
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
        result->className = (LPCSTR)(UINT32)GET_WORD( p + 1 );
        p += 2;
	TRACE(dialog, " CLASS %04x\n", LOWORD(result->className) );
        break;
    default:
        result->className = (LPCSTR)p;
        TRACE(dialog, " CLASS %s\n", debugstr_w( (LPCWSTR)p ));
        p += lstrlen32W( (LPCWSTR)p ) + 1;
        break;
    }

    /* Get the window caption */

    result->caption = (LPCSTR)p;
    p += lstrlen32W( (LPCWSTR)p ) + 1;
    TRACE(dialog, " CAPTION %s\n", debugstr_w( (LPCWSTR)result->caption ) );

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
        p += lstrlen32W( (LPCWSTR)p ) + 1;
	TRACE(dialog, " FONT %d, %s, %d, %s\n",
              result->pointSize, debugstr_w( (LPCWSTR)result->faceName ),
              result->weight, result->italic ? "TRUE" : "FALSE" );
    }

    /* First control is on dword boundary */
    return (LPCSTR)((((int)p) + 3) & ~3);
}


/***********************************************************************
 *           DIALOG_CreateIndirect
 */
HWND32 DIALOG_CreateIndirect( HINSTANCE32 hInst, LPCSTR dlgTemplate,
                              BOOL32 win32Template, HWND32 owner,
                              DLGPROC16 dlgProc, LPARAM param,
                              WINDOWPROCTYPE procType )
{
    HMENU16 hMenu = 0;
    HFONT16 hFont = 0;
    HWND32 hwnd;
    RECT32 rect;
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
        else hMenu = LoadMenu32W( hInst, (LPCWSTR)template.menuName );
    }

      /* Create custom font if needed */

    if (template.style & DS_SETFONT)
    {
          /* The font height must be negative as it is a point size */
          /* (see CreateFont() documentation in the Windows SDK).   */
	if (win32Template)
	    hFont = CreateFont32W( -template.pointSize, 0, 0, 0,
                                   template.weight, template.italic, FALSE,
                                   FALSE, DEFAULT_CHARSET, 0, 0, PROOF_QUALITY,
                                   FF_DONTCARE, (LPCWSTR)template.faceName );
	else
	    hFont = CreateFont16( -template.pointSize, 0, 0, 0, FW_DONTCARE,
			    FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0,
			    PROOF_QUALITY, FF_DONTCARE,
                            template.faceName );
	if (hFont)
	{
	    TEXTMETRIC16 tm;
	    HFONT16 oldFont;

	    HDC32 hdc = GetDC32(0);
	    oldFont = SelectObject32( hdc, hFont );
	    GetTextMetrics16( hdc, &tm );
	    SelectObject32( hdc, oldFont );
	    ReleaseDC32( 0, hdc );
	    xUnit = tm.tmAveCharWidth;
	    yUnit = tm.tmHeight;
            if (!(tm.tmPitchAndFamily & TMPF_FIXED_PITCH))
                xBaseUnit = xBaseUnit * 5 / 4;  /* See DIALOG_Init() */
	}
    }
    
    /* Create dialog main window */

    rect.left = rect.top = 0;
    rect.right = template.cx * xUnit / 4;
    rect.bottom = template.cy * yUnit / 8;
    if (template.style & DS_MODALFRAME)
        template.exStyle |= WS_EX_DLGMODALFRAME;
    AdjustWindowRectEx32( &rect, template.style, 
                          hMenu ? TRUE : FALSE , template.exStyle );
    rect.right -= rect.left;
    rect.bottom -= rect.top;

    if ((INT16)template.x == CW_USEDEFAULT16)
    {
        rect.left = rect.top = (procType == WIN_PROC_16) ? CW_USEDEFAULT16
                                                         : CW_USEDEFAULT32;
    }
    else
    {
        rect.left += template.x * xUnit / 4;
        rect.top += template.y * yUnit / 8;
        if ( !(template.style & WS_CHILD) )
	{
            INT16 dX, dY;

            if( !(template.style & DS_ABSALIGN) )
                ClientToScreen32( owner, (POINT32 *)&rect );
	    
            /* try to fit it into the desktop */

            if( (dX = rect.left + rect.right + SYSMETRICS_CXDLGFRAME 
                 - SYSMETRICS_CXSCREEN) > 0 ) rect.left -= dX;
            if( (dY = rect.top + rect.bottom + SYSMETRICS_CYDLGFRAME
                 - SYSMETRICS_CYSCREEN) > 0 ) rect.top -= dY;
            if( rect.left < 0 ) rect.left = 0;
            if( rect.top < 0 ) rect.top = 0;
        }
    }

    if (procType == WIN_PROC_16)
        hwnd = CreateWindowEx16(template.exStyle, template.className,
                                template.caption, template.style & ~WS_VISIBLE,
                                rect.left, rect.top, rect.right, rect.bottom,
                                owner, hMenu, hInst, NULL );
    else
        hwnd = CreateWindowEx32W(template.exStyle, (LPCWSTR)template.className,
                                 (LPCWSTR)template.caption,
                                 template.style & ~WS_VISIBLE,
                                 rect.left, rect.top, rect.right, rect.bottom,
                                 owner, hMenu, hInst, NULL );
	
    if (!hwnd)
    {
	if (hFont) DeleteObject32( hFont );
	if (hMenu) DestroyMenu32( hMenu );
	return 0;
    }
    wndPtr = WIN_FindWndPtr( hwnd );
    wndPtr->flags |= WIN_ISDIALOG;
    wndPtr->helpContext = template.helpId;

      /* Initialise dialog extra data */

    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    WINPROC_SetProc( &dlgInfo->dlgProc, dlgProc, procType, WIN_PROC_WINDOW );
    dlgInfo->hUserFont = hFont;
    dlgInfo->hMenu     = hMenu;
    dlgInfo->xBaseUnit = xUnit;
    dlgInfo->yBaseUnit = yUnit;
    dlgInfo->msgResult = 0;
    dlgInfo->idResult  = 0;
    dlgInfo->flags     = 0;
    dlgInfo->hDialogHeap = 0;

    if (dlgInfo->hUserFont)
        SendMessage32A( hwnd, WM_SETFONT, (WPARAM32)dlgInfo->hUserFont, 0 );

    /* Create controls */

    if (DIALOG_CreateControls( wndPtr, dlgTemplate, &template,
                               hInst, win32Template ))
    {
       /* Send initialisation messages and set focus */

	dlgInfo->hwndFocus = GetNextDlgTabItem32( hwnd, 0, FALSE );

	if (SendMessage32A( hwnd, WM_INITDIALOG, (WPARAM32)dlgInfo->hwndFocus, param ))
            SetFocus32( dlgInfo->hwndFocus );

	if (template.style & WS_VISIBLE && !(wndPtr->dwStyle & WS_VISIBLE)) 
	{
	   ShowWindow32( hwnd, SW_SHOWNORMAL );	/* SW_SHOW doesn't always work */
	   UpdateWindow32( hwnd );
	}
	return hwnd;
    }

    if( IsWindow32(hwnd) ) DestroyWindow32( hwnd );
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

    TRACE(dialog, "%04x,%08lx,%04x,%08lx,%ld\n",
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
 *           CreateDialogParam32A   (USER32.73)
 */
HWND32 WINAPI CreateDialogParam32A( HINSTANCE32 hInst, LPCSTR name,
                                    HWND32 owner, DLGPROC32 dlgProc,
                                    LPARAM param )
{
    if (HIWORD(name))
    {
        LPWSTR str = HEAP_strdupAtoW( GetProcessHeap(), 0, name );
        HWND32 hwnd = CreateDialogParam32W( hInst, str, owner, dlgProc, param);
        HeapFree( GetProcessHeap(), 0, str );
        return hwnd;
    }
    return CreateDialogParam32W( hInst, (LPCWSTR)name, owner, dlgProc, param );
}


/***********************************************************************
 *           CreateDialogParam32W   (USER32.74)
 */
HWND32 WINAPI CreateDialogParam32W( HINSTANCE32 hInst, LPCWSTR name,
                                    HWND32 owner, DLGPROC32 dlgProc,
                                    LPARAM param )
{
    HANDLE32 hrsrc = FindResource32W( hInst, name, RT_DIALOG32W );
    if (!hrsrc) return 0;
    return CreateDialogIndirectParam32W( hInst,
                                         (LPVOID)LoadResource32(hInst, hrsrc),
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
 *           CreateDialogIndirectParam32A   (USER32.69)
 */
HWND32 WINAPI CreateDialogIndirectParam32A( HINSTANCE32 hInst,
                                            LPCVOID dlgTemplate,
                                            HWND32 owner, DLGPROC32 dlgProc,
                                            LPARAM param )
{
    return DIALOG_CreateIndirect( hInst, dlgTemplate, TRUE, owner,
                                  (DLGPROC16)dlgProc, param, WIN_PROC_32A );
}


/***********************************************************************
 *           CreateDialogIndirectParam32W   (USER32.72)
 */
HWND32 WINAPI CreateDialogIndirectParam32W( HINSTANCE32 hInst,
                                            LPCVOID dlgTemplate,
                                            HWND32 owner, DLGPROC32 dlgProc,
                                            LPARAM param )
{
    return DIALOG_CreateIndirect( hInst, dlgTemplate, TRUE, owner,
                                  (DLGPROC16)dlgProc, param, WIN_PROC_32W );
}


/***********************************************************************
 *           DIALOG_DoDialogBox
 */
INT32 DIALOG_DoDialogBox( HWND32 hwnd, HWND32 owner )
{
    WND * wndPtr;
    DIALOGINFO * dlgInfo;
    MSG16 msg;
    INT32 retval;

      /* Owner must be a top-level window */
    owner = WIN_GetTopParent( owner );
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return -1;
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    EnableWindow32( owner, FALSE );
    ShowWindow32( hwnd, SW_SHOW );

    while (MSG_InternalGetMessage(&msg, hwnd, owner, MSGF_DIALOGBOX, PM_REMOVE,
                                  !(wndPtr->dwStyle & DS_NOIDLEMSG) ))
    {
	if (!IsDialogMessage16( hwnd, &msg))
	{
	    TranslateMessage16( &msg );
	    DispatchMessage16( &msg );
	}
	if (dlgInfo->flags & DF_END) break;
    }
    retval = dlgInfo->idResult;
    EnableWindow32( owner, TRUE );
    DestroyWindow32( hwnd );
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
 *           DialogBoxParam32A   (USER32.139)
 */
INT32 WINAPI DialogBoxParam32A( HINSTANCE32 hInst, LPCSTR name,
                                HWND32 owner, DLGPROC32 dlgProc, LPARAM param )
{
    HWND32 hwnd = CreateDialogParam32A( hInst, name, owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxParam32W   (USER32.140)
 */
INT32 WINAPI DialogBoxParam32W( HINSTANCE32 hInst, LPCWSTR name,
                                HWND32 owner, DLGPROC32 dlgProc, LPARAM param )
{
    HWND32 hwnd = CreateDialogParam32W( hInst, name, owner, dlgProc, param );
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
 *           DialogBoxIndirectParam32A   (USER32.136)
 */
INT32 WINAPI DialogBoxIndirectParam32A(HINSTANCE32 hInstance, LPCVOID template,
                                       HWND32 owner, DLGPROC32 dlgProc,
                                       LPARAM param )
{
    HWND32 hwnd = CreateDialogIndirectParam32A( hInstance, template,
                                                owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxIndirectParam32W   (USER32.138)
 */
INT32 WINAPI DialogBoxIndirectParam32W(HINSTANCE32 hInstance, LPCVOID template,
                                       HWND32 owner, DLGPROC32 dlgProc,
                                       LPARAM param )
{
    HWND32 hwnd = CreateDialogIndirectParam32W( hInstance, template,
                                                owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           EndDialog16   (USER32.173)
 */
BOOL16 WINAPI EndDialog16( HWND16 hwnd, INT16 retval )
{
    return EndDialog32( hwnd, retval );
}


/***********************************************************************
 *           EndDialog32   (USER.88)
 */
BOOL32 WINAPI EndDialog32( HWND32 hwnd, INT32 retval )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    DIALOGINFO * dlgInfo = (DIALOGINFO *)wndPtr->wExtra;

    TRACE(dialog, "%04x %d\n", hwnd, retval );

    if( dlgInfo )
    {
        dlgInfo->idResult = retval;
        dlgInfo->flags |= DF_END;
    }
    return TRUE;
}


/***********************************************************************
 *           DIALOG_IsDialogMessage
 */
static BOOL32 DIALOG_IsDialogMessage( HWND32 hwnd, HWND32 hwndDlg,
                                      UINT32 message, WPARAM32 wParam,
                                      LPARAM lParam, BOOL32 *translate,
                                      BOOL32 *dispatch )
{
    INT32 dlgCode;

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

    dlgCode = SendMessage32A( hwnd, WM_GETDLGCODE, 0, 0 );
    if (dlgCode & DLGC_WANTMESSAGE)
    {
        *translate = *dispatch = TRUE;
        return TRUE;
    }

    switch(message)
    {
    case WM_KEYDOWN:
        if (dlgCode & DLGC_WANTALLKEYS) break;
        switch(wParam)
        {
        case VK_TAB:
            if (!(dlgCode & DLGC_WANTTAB))
            {
                SendMessage32A( hwndDlg, WM_NEXTDLGCTL,
                                (GetKeyState32(VK_SHIFT) & 0x8000), 0 );
                return TRUE;
            }
            break;
            
        case VK_RIGHT:
        case VK_DOWN:
            if (!(dlgCode & DLGC_WANTARROWS))
            {
                SetFocus32( GetNextDlgGroupItem32( hwndDlg, GetFocus32(),
                                                   FALSE ) );
                return TRUE;
            }
            break;

        case VK_LEFT:
        case VK_UP:
            if (!(dlgCode & DLGC_WANTARROWS))
            {
                SetFocus32( GetNextDlgGroupItem32( hwndDlg, GetFocus32(),
                                                   TRUE ) );
                return TRUE;
            }
            break;

        case VK_ESCAPE:
            SendMessage32A( hwndDlg, WM_COMMAND, IDCANCEL,
                            (LPARAM)GetDlgItem32( hwndDlg, IDCANCEL ) );
            break;

        case VK_RETURN:
            {
                DWORD dw = SendMessage16( hwndDlg, DM_GETDEFID, 0, 0 );
                if (HIWORD(dw) == DC_HASDEFID)
                    SendMessage32A( hwndDlg, WM_COMMAND, 
                                    MAKEWPARAM( LOWORD(dw), BN_CLICKED ),
                                    (LPARAM)GetDlgItem32(hwndDlg, LOWORD(dw)));
                else
                    SendMessage32A( hwndDlg, WM_COMMAND, IDOK,
                                    (LPARAM)GetDlgItem32( hwndDlg, IDOK ) );
            }
            break;

	default:
            *translate = TRUE;
            break;
        }
        break;  /* case WM_KEYDOWN */

        
    case WM_CHAR:
        if (dlgCode & (DLGC_WANTALLKEYS | DLGC_WANTCHARS)) break;
        break;

    case WM_SYSCHAR:
        if (dlgCode & DLGC_WANTALLKEYS) break;
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
BOOL16 WINAPI IsDialogMessage16( HWND16 hwndDlg, LPMSG16 msg )
{
    BOOL32 ret, translate, dispatch;

    if ((hwndDlg != msg->hwnd) && !IsChild16( hwndDlg, msg->hwnd ))
        return FALSE;

    ret = DIALOG_IsDialogMessage( msg->hwnd, hwndDlg, msg->message,
                                  msg->wParam, msg->lParam,
                                  &translate, &dispatch );
    if (translate) TranslateMessage16( msg );
    if (dispatch) DispatchMessage16( msg );
    return ret;
}


/***********************************************************************
 *           IsDialogMessage32A   (USER32.342)
 */
BOOL32 WINAPI IsDialogMessage32A( HWND32 hwndDlg, LPMSG32 msg )
{
    BOOL32 ret, translate, dispatch;

    if ((hwndDlg != msg->hwnd) && !IsChild32( hwndDlg, msg->hwnd ))
        return FALSE;

    ret = DIALOG_IsDialogMessage( msg->hwnd, hwndDlg, msg->message,
                                  msg->wParam, msg->lParam,
                                  &translate, &dispatch );
    if (translate) TranslateMessage32( msg );
    if (dispatch) DispatchMessage32A( msg );
    return ret;
}


/***********************************************************************
 *           IsDialogMessage32W   (USER32.343)
 */
BOOL32 WINAPI IsDialogMessage32W( HWND32 hwndDlg, LPMSG32 msg )
{
    BOOL32 ret, translate, dispatch;

    if ((hwndDlg != msg->hwnd) && !IsChild32( hwndDlg, msg->hwnd ))
        return FALSE;

    ret = DIALOG_IsDialogMessage( msg->hwnd, hwndDlg, msg->message,
                                  msg->wParam, msg->lParam,
                                  &translate, &dispatch );
    if (translate) TranslateMessage32( msg );
    if (dispatch) DispatchMessage32W( msg );
    return ret;
}


/****************************************************************
 *         GetDlgCtrlID16   (USER.277)
 */
INT16 WINAPI GetDlgCtrlID16( HWND16 hwnd )
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr) return wndPtr->wIDmenu;
    else return 0;
}
 

/****************************************************************
 *         GetDlgCtrlID32   (USER32.234)
 */
INT32 WINAPI GetDlgCtrlID32( HWND32 hwnd )
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr) return wndPtr->wIDmenu;
    else return 0;
}
 

/***********************************************************************
 *           GetDlgItem16   (USER.91)
 */
HWND16 WINAPI GetDlgItem16( HWND16 hwndDlg, INT16 id )
{
    WND *pWnd;

    if (!(pWnd = WIN_FindWndPtr( hwndDlg ))) return 0;
    for (pWnd = pWnd->child; pWnd; pWnd = pWnd->next)
        if (pWnd->wIDmenu == (UINT16)id) return pWnd->hwndSelf;
    return 0;
}


/***********************************************************************
 *           GetDlgItem32   (USER32.235)
 */
HWND32 WINAPI GetDlgItem32( HWND32 hwndDlg, INT32 id )
{
    WND *pWnd;

    if (!(pWnd = WIN_FindWndPtr( hwndDlg ))) return 0;
    for (pWnd = pWnd->child; pWnd; pWnd = pWnd->next)
        if (pWnd->wIDmenu == (UINT16)id) return pWnd->hwndSelf;
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
 *           SendDlgItemMessage32A   (USER32.452)
 */
LRESULT WINAPI SendDlgItemMessage32A( HWND32 hwnd, INT32 id, UINT32 msg,
                                      WPARAM32 wParam, LPARAM lParam )
{
    HWND32 hwndCtrl = GetDlgItem32( hwnd, id );
    if (hwndCtrl) return SendMessage32A( hwndCtrl, msg, wParam, lParam );
    else return 0;
}


/*******************************************************************
 *           SendDlgItemMessage32W   (USER32.453)
 */
LRESULT WINAPI SendDlgItemMessage32W( HWND32 hwnd, INT32 id, UINT32 msg,
                                      WPARAM32 wParam, LPARAM lParam )
{
    HWND32 hwndCtrl = GetDlgItem32( hwnd, id );
    if (hwndCtrl) return SendMessage32W( hwndCtrl, msg, wParam, lParam );
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
 *           SetDlgItemText32A   (USER32.478)
 */
void WINAPI SetDlgItemText32A( HWND32 hwnd, INT32 id, LPCSTR lpString )
{
    SendDlgItemMessage32A( hwnd, id, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *           SetDlgItemText32W   (USER32.479)
 */
void WINAPI SetDlgItemText32W( HWND32 hwnd, INT32 id, LPCWSTR lpString )
{
    SendDlgItemMessage32W( hwnd, id, WM_SETTEXT, 0, (LPARAM)lpString );
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
 *           GetDlgItemText32A   (USER32.237)
 */
INT32 WINAPI GetDlgItemText32A( HWND32 hwnd, INT32 id, LPSTR str, UINT32 len )
{
    return (INT32)SendDlgItemMessage32A( hwnd, id, WM_GETTEXT,
                                         len, (LPARAM)str );
}


/***********************************************************************
 *           GetDlgItemText32W   (USER32.238)
 */
INT32 WINAPI GetDlgItemText32W( HWND32 hwnd, INT32 id, LPWSTR str, UINT32 len )
{
    return (INT32)SendDlgItemMessage32W( hwnd, id, WM_GETTEXT,
                                         len, (LPARAM)str );
}


/*******************************************************************
 *           SetDlgItemInt16   (USER.94)
 */
void WINAPI SetDlgItemInt16( HWND16 hwnd, INT16 id, UINT16 value, BOOL16 fSigned )
{
    return SetDlgItemInt32( hwnd, (UINT32)(UINT16)id, value, fSigned );
}


/*******************************************************************
 *           SetDlgItemInt32   (USER32.477)
 */
void WINAPI SetDlgItemInt32( HWND32 hwnd, INT32 id, UINT32 value,
                             BOOL32 fSigned )
{
    char str[20];

    if (fSigned) sprintf( str, "%d", (INT32)value );
    else sprintf( str, "%u", value );
    SendDlgItemMessage32A( hwnd, id, WM_SETTEXT, 0, (LPARAM)str );
}


/***********************************************************************
 *           GetDlgItemInt16   (USER.95)
 */
UINT16 WINAPI GetDlgItemInt16( HWND16 hwnd, INT16 id, BOOL16 *translated,
                               BOOL16 fSigned )
{
    UINT32 result;
    BOOL32 ok;

    if (translated) *translated = FALSE;
    result = GetDlgItemInt32( hwnd, (UINT32)(UINT16)id, &ok, fSigned );
    if (!ok) return 0;
    if (fSigned)
    {
        if (((INT32)result < -32767) || ((INT32)result > 32767)) return 0;
    }
    else
    {
        if (result > 65535) return 0;
    }
    if (translated) *translated = TRUE;
    return (UINT16)result;
}


/***********************************************************************
 *           GetDlgItemInt32   (USER32.236)
 */
UINT32 WINAPI GetDlgItemInt32( HWND32 hwnd, INT32 id, BOOL32 *translated,
                               BOOL32 fSigned )
{
    char str[30];
    char * endptr;
    long result = 0;
    
    if (translated) *translated = FALSE;
    if (!SendDlgItemMessage32A(hwnd, id, WM_GETTEXT, sizeof(str), (LPARAM)str))
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
    return (UINT32)result;
}


/***********************************************************************
 *           CheckDlgButton16   (USER.97)
 */
BOOL16 WINAPI CheckDlgButton16( HWND16 hwnd, INT16 id, UINT16 check )
{
    SendDlgItemMessage32A( hwnd, id, BM_SETCHECK32, check, 0 );
    return TRUE;
}


/***********************************************************************
 *           CheckDlgButton32   (USER32.45)
 */
BOOL32 WINAPI CheckDlgButton32( HWND32 hwnd, INT32 id, UINT32 check )
{
    SendDlgItemMessage32A( hwnd, id, BM_SETCHECK32, check, 0 );
    return TRUE;
}


/***********************************************************************
 *           IsDlgButtonChecked16   (USER.98)
 */
UINT16 WINAPI IsDlgButtonChecked16( HWND16 hwnd, UINT16 id )
{
    return (UINT16)SendDlgItemMessage32A( hwnd, id, BM_GETCHECK32, 0, 0 );
}


/***********************************************************************
 *           IsDlgButtonChecked32   (USER32.344)
 */
UINT32 WINAPI IsDlgButtonChecked32( HWND32 hwnd, UINT32 id )
{
    return (UINT32)SendDlgItemMessage32A( hwnd, id, BM_GETCHECK32, 0, 0 );
}


/***********************************************************************
 *           CheckRadioButton16   (USER.96)
 */
BOOL16 WINAPI CheckRadioButton16( HWND16 hwndDlg, UINT16 firstID,
                                  UINT16 lastID, UINT16 checkID )
{
    return CheckRadioButton32( hwndDlg, firstID, lastID, checkID );
}


/***********************************************************************
 *           CheckRadioButton32   (USER32.48)
 */
BOOL32 WINAPI CheckRadioButton32( HWND32 hwndDlg, UINT32 firstID,
                                  UINT32 lastID, UINT32 checkID )
{
    WND *pWnd = WIN_FindWndPtr( hwndDlg );
    if (!pWnd) return FALSE;

    for (pWnd = pWnd->child; pWnd; pWnd = pWnd->next)
        if ((pWnd->wIDmenu == firstID) || (pWnd->wIDmenu == lastID)) break;
    if (!pWnd) return FALSE;

    if (pWnd->wIDmenu == lastID)
        lastID = firstID;  /* Buttons are in reverse order */
    while (pWnd)
    {
	SendMessage32A( pWnd->hwndSelf, BM_SETCHECK32,
                        (pWnd->wIDmenu == checkID), 0 );
        if (pWnd->wIDmenu == lastID) break;
	pWnd = pWnd->next;
    }
    return TRUE;
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
    rect->left   = (rect->left * dlgInfo->xBaseUnit) / 4;
    rect->right  = (rect->right * dlgInfo->xBaseUnit) / 4;
    rect->top    = (rect->top * dlgInfo->yBaseUnit) / 8;
    rect->bottom = (rect->bottom * dlgInfo->yBaseUnit) / 8;
}


/***********************************************************************
 *           MapDialogRect32   (USER32.382)
 */
void WINAPI MapDialogRect32( HWND32 hwnd, LPRECT32 rect )
{
    DIALOGINFO * dlgInfo;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return;
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    rect->left   = (rect->left * dlgInfo->xBaseUnit) / 4;
    rect->right  = (rect->right * dlgInfo->xBaseUnit) / 4;
    rect->top    = (rect->top * dlgInfo->yBaseUnit) / 8;
    rect->bottom = (rect->bottom * dlgInfo->yBaseUnit) / 8;
}


/***********************************************************************
 *           GetNextDlgGroupItem16   (USER.227)
 */
HWND16 WINAPI GetNextDlgGroupItem16( HWND16 hwndDlg, HWND16 hwndCtrl,
                                     BOOL16 fPrevious )
{
    return (HWND16)GetNextDlgGroupItem32( hwndDlg, hwndCtrl, fPrevious );
}


/***********************************************************************
 *           GetNextDlgGroupItem32   (USER32.275)
 */
HWND32 WINAPI GetNextDlgGroupItem32( HWND32 hwndDlg, HWND32 hwndCtrl,
                                     BOOL32 fPrevious )
{
    WND *pWnd, *pWndLast, *pWndCtrl, *pWndDlg;

    if (!(pWndDlg = WIN_FindWndPtr( hwndDlg ))) return 0;
    if (hwndCtrl)
    {
        if (!(pWndCtrl = WIN_FindWndPtr( hwndCtrl ))) return 0;
        /* Make sure hwndCtrl is a top-level child */
        while ((pWndCtrl->dwStyle & WS_CHILD) && (pWndCtrl->parent != pWndDlg))
            pWndCtrl = pWndCtrl->parent;
        if (pWndCtrl->parent != pWndDlg) return 0;
    }
    else
    {
        /* No ctrl specified -> start from the beginning */
        if (!(pWndCtrl = pWndDlg->child)) return 0;
        if (fPrevious) while (pWndCtrl->next) pWndCtrl = pWndCtrl->next;
    }

    pWndLast = pWndCtrl;
    pWnd = pWndCtrl->next;
    while (1)
    {
        if (!pWnd || (pWnd->dwStyle & WS_GROUP))
        {
            /* Wrap-around to the beginning of the group */
            WND *pWndStart = pWndDlg->child;
            for (pWnd = pWndStart; pWnd; pWnd = pWnd->next)
            {
                if (pWnd->dwStyle & WS_GROUP) pWndStart = pWnd;
                if (pWnd == pWndCtrl) break;
            }
            pWnd = pWndStart;
        }
        if (pWnd == pWndCtrl) break;
	if ((pWnd->dwStyle & WS_VISIBLE) && !(pWnd->dwStyle & WS_DISABLED))
	{
            pWndLast = pWnd;
	    if (!fPrevious) break;
	}
        pWnd = pWnd->next;
    }
    return pWndLast->hwndSelf;
}


/***********************************************************************
 *           GetNextDlgTabItem16   (USER.228)
 */
HWND16 WINAPI GetNextDlgTabItem16( HWND16 hwndDlg, HWND16 hwndCtrl,
                                   BOOL16 fPrevious )
{
    return (HWND16)GetNextDlgTabItem32( hwndDlg, hwndCtrl, fPrevious );
}


/***********************************************************************
 *           GetNextDlgTabItem32   (USER32.276)
 */
HWND32 WINAPI GetNextDlgTabItem32( HWND32 hwndDlg, HWND32 hwndCtrl,
                                   BOOL32 fPrevious )
{
    WND *pWnd, *pWndLast, *pWndCtrl, *pWndDlg;

    if (!(pWndDlg = WIN_FindWndPtr( hwndDlg ))) return 0;
    if (hwndCtrl)
    {
        if (!(pWndCtrl = WIN_FindWndPtr( hwndCtrl ))) return 0;
        /* Make sure hwndCtrl is a top-level child */
        while ((pWndCtrl->dwStyle & WS_CHILD) && (pWndCtrl->parent != pWndDlg))
            pWndCtrl = pWndCtrl->parent;
        if (pWndCtrl->parent != pWndDlg) return 0;
    }
    else
    {
        /* No ctrl specified -> start from the beginning */
        if (!(pWndCtrl = pWndDlg->child)) return 0;
        if (fPrevious) while (pWndCtrl->next) pWndCtrl = pWndCtrl->next;
    }

    pWndLast = pWndCtrl;
    pWnd = pWndCtrl->next;
    while (1)
    {
        if (!pWnd) pWnd = pWndDlg->child;
        if (pWnd == pWndCtrl) break;
	if ((pWnd->dwStyle & WS_TABSTOP) && (pWnd->dwStyle & WS_VISIBLE) &&
            !(pWnd->dwStyle & WS_DISABLED))
	{
            pWndLast = pWnd;
	    if (!fPrevious) break;
	}
        pWnd = pWnd->next;
    }
    return pWndLast->hwndSelf;
}


/**********************************************************************
 *           DIALOG_DlgDirSelect
 *
 * Helper function for DlgDirSelect*
 */
static BOOL32 DIALOG_DlgDirSelect( HWND32 hwnd, LPSTR str, INT32 len,
                                   INT32 id, BOOL32 win32, BOOL32 unicode,
                                   BOOL32 combo )
{
    char *buffer, *ptr;
    INT32 item, size;
    BOOL32 ret;
    HWND32 listbox = GetDlgItem32( hwnd, id );

    TRACE(dialog, "%04x '%s' %d\n", hwnd, str, id );
    if (!listbox) return FALSE;
    if (win32)
    {
        item = SendMessage32A(listbox, combo ? CB_GETCURSEL32
                                             : LB_GETCURSEL32, 0, 0 );
        if (item == LB_ERR) return FALSE;
        size = SendMessage32A(listbox, combo ? CB_GETLBTEXTLEN32
                                             : LB_GETTEXTLEN32, 0, 0 );
        if (size == LB_ERR) return FALSE;
    }
    else
    {
        item = SendMessage32A(listbox, combo ? CB_GETCURSEL16
                                             : LB_GETCURSEL16, 0, 0 );
        if (item == LB_ERR) return FALSE;
        size = SendMessage32A(listbox, combo ? CB_GETLBTEXTLEN16
                                             : LB_GETTEXTLEN16, 0, 0 );
        if (size == LB_ERR) return FALSE;
    }

    if (!(buffer = SEGPTR_ALLOC( size+1 ))) return FALSE;

    if (win32)
        SendMessage32A( listbox, combo ? CB_GETLBTEXT32 : LB_GETTEXT32,
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
    else lstrcpyn32A( str, ptr, len );
    SEGPTR_FREE( buffer );
    TRACE(dialog, "Returning %d '%s'\n", ret, str );
    return ret;
}


/**********************************************************************
 *	    DIALOG_DlgDirList
 *
 * Helper function for DlgDirList*
 */
static INT32 DIALOG_DlgDirList( HWND32 hDlg, LPSTR spec, INT32 idLBox,
                                INT32 idStatic, UINT32 attrib, BOOL32 combo )
{
    int drive;
    HWND32 hwnd;
    LPSTR orig_spec = spec;

#define SENDMSG(msg,wparam,lparam) \
    ((attrib & DDL_POSTMSGS) ? PostMessage32A( hwnd, msg, wparam, lparam ) \
                             : SendMessage32A( hwnd, msg, wparam, lparam ))

    TRACE(dialog, "%04x '%s' %d %d %04x\n",
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

    TRACE(dialog, "path=%c:\\%s mask=%s\n",
                    'A' + drive, DRIVE_GetDosCwd(drive), spec );

    if (idLBox && ((hwnd = GetDlgItem32( hDlg, idLBox )) != 0))
    {
        SENDMSG( combo ? CB_RESETCONTENT32 : LB_RESETCONTENT32, 0, 0 );
        if (attrib & DDL_DIRECTORY)
        {
            if (!(attrib & DDL_EXCLUSIVE))
            {
                if (SENDMSG( combo ? CB_DIR32 : LB_DIR32,
                             attrib & ~(DDL_DIRECTORY | DDL_DRIVES),
                             (LPARAM)spec ) == LB_ERR)
                    return FALSE;
            }
            if (SENDMSG( combo ? CB_DIR32 : LB_DIR32,
                       (attrib & (DDL_DIRECTORY | DDL_DRIVES)) | DDL_EXCLUSIVE,
                         (LPARAM)"*.*" ) == LB_ERR)
                return FALSE;
        }
        else
        {
            if (SENDMSG( combo ? CB_DIR32 : LB_DIR32, attrib,
                         (LPARAM)spec ) == LB_ERR)
                return FALSE;
        }
    }

    if (idStatic && ((hwnd = GetDlgItem32( hDlg, idStatic )) != 0))
    {
        char temp[512];
        int drive = DRIVE_GetCurrentDrive();
        strcpy( temp, "A:\\" );
        temp[0] += drive;
        lstrcpyn32A( temp + 3, DRIVE_GetDosCwd(drive), sizeof(temp)-3 );
        CharLower32A( temp );
        /* Can't use PostMessage() here, because the string is on the stack */
        SetDlgItemText32A( hDlg, idStatic, temp );
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
 * Helper function for DlgDirList*32W
 */
static INT32 DIALOG_DlgDirListW( HWND32 hDlg, LPWSTR spec, INT32 idLBox,
                                 INT32 idStatic, UINT32 attrib, BOOL32 combo )
{
    if (spec)
    {
        LPSTR specA = HEAP_strdupWtoA( GetProcessHeap(), 0, spec );
        INT32 ret = DIALOG_DlgDirList( hDlg, specA, idLBox, idStatic,
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
BOOL16 WINAPI DlgDirSelect( HWND16 hwnd, LPSTR str, INT16 id )
{
    return DlgDirSelectEx16( hwnd, str, 128, id );
}


/**********************************************************************
 *	    DlgDirSelectComboBox    (USER.194)
 */
BOOL16 WINAPI DlgDirSelectComboBox( HWND16 hwnd, LPSTR str, INT16 id )
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
 *           DlgDirSelectEx32A    (USER32.149)
 */
BOOL32 WINAPI DlgDirSelectEx32A( HWND32 hwnd, LPSTR str, INT32 len, INT32 id )
{
    return DIALOG_DlgDirSelect( hwnd, str, len, id, TRUE, FALSE, FALSE );
}


/**********************************************************************
 *           DlgDirSelectEx32W    (USER32.150)
 */
BOOL32 WINAPI DlgDirSelectEx32W( HWND32 hwnd, LPWSTR str, INT32 len, INT32 id )
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
 *           DlgDirSelectComboBoxEx32A    (USER32.147)
 */
BOOL32 WINAPI DlgDirSelectComboBoxEx32A( HWND32 hwnd, LPSTR str, INT32 len,
                                         INT32 id )
{
    return DIALOG_DlgDirSelect( hwnd, str, len, id, TRUE, FALSE, TRUE );
}


/**********************************************************************
 *           DlgDirSelectComboBoxEx32W    (USER32.148)
 */
BOOL32 WINAPI DlgDirSelectComboBoxEx32W( HWND32 hwnd, LPWSTR str, INT32 len,
                                         INT32 id)
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
 *	    DlgDirList32A    (USER32.143)
 */
INT32 WINAPI DlgDirList32A( HWND32 hDlg, LPSTR spec, INT32 idLBox,
                            INT32 idStatic, UINT32 attrib )
{
    return DIALOG_DlgDirList( hDlg, spec, idLBox, idStatic, attrib, FALSE );
}


/**********************************************************************
 *	    DlgDirList32W    (USER32.146)
 */
INT32 WINAPI DlgDirList32W( HWND32 hDlg, LPWSTR spec, INT32 idLBox,
                            INT32 idStatic, UINT32 attrib )
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
 *	    DlgDirListComboBox32A    (USER32.144)
 */
INT32 WINAPI DlgDirListComboBox32A( HWND32 hDlg, LPSTR spec, INT32 idCBox,
                                    INT32 idStatic, UINT32 attrib )
{
    return DIALOG_DlgDirList( hDlg, spec, idCBox, idStatic, attrib, TRUE );
}


/**********************************************************************
 *	    DlgDirListComboBox32W    (USER32.145)
 */
INT32 WINAPI DlgDirListComboBox32W( HWND32 hDlg, LPWSTR spec, INT32 idCBox,
                                    INT32 idStatic, UINT32 attrib )
{
    return DIALOG_DlgDirListW( hDlg, spec, idCBox, idStatic, attrib, TRUE );
}
