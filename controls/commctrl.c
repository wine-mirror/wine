/*		
 * Common controls functions
 *
 * Copyright 1997 Dimitrie O. Paun
 *
 */

#include "win.h"
#include "heap.h"
#include "commctrl.h"
#include "progress.h"
#include "status.h"
#include "updown.h"

/* Win32 common controls */

static WNDCLASS32A WIDGETS_CommonControls32[] =
{
    { CS_GLOBALCLASS | CS_VREDRAW | CS_HREDRAW, StatusWindowProc, 0,
      sizeof(STATUSWINDOWINFO), 0, 0, 0, 0, 0, STATUSCLASSNAME32A },
    { CS_GLOBALCLASS | CS_VREDRAW | CS_HREDRAW, UpDownWindowProc, 0,
      sizeof(UPDOWN_INFO), 0, 0, 0, 0, 0, UPDOWN_CLASS32A },
    { CS_GLOBALCLASS | CS_VREDRAW | CS_HREDRAW, ProgressWindowProc, 0,
      sizeof(PROGRESS_INFO), 0, 0, 0, 0, 0, PROGRESS_CLASS32A }
};

#define NB_COMMON_CONTROLS32 \
         (sizeof(WIDGETS_CommonControls32)/sizeof(WIDGETS_CommonControls32[0]))


/***********************************************************************
 *           DrawStatusText32A   (COMCTL32.5)
 */
void WINAPI DrawStatusText32A( HDC32 hdc, LPRECT32 lprc, LPCSTR text,
                               UINT32 style )
{
    RECT32 r = *lprc;
    UINT32 border = BDR_SUNKENOUTER;

    DrawEdge32(hdc, &r, BDR_RAISEDINNER, BF_RECT|BF_ADJUST|BF_FLAT);

    if(style==SBT_POPOUT)
      border = BDR_RAISEDOUTER;
    else if(style==SBT_NOBORDERS)
      border = 0;

    DrawEdge32(hdc, &r, border, BF_RECT|BF_ADJUST|BF_MIDDLE);

    /* now draw text */
    if (text) {
      int oldbkmode = SetBkMode32(hdc, TRANSPARENT);
      r.left += 3;
      DrawText32A(hdc, text, lstrlen32A(text),
		  &r, DT_LEFT|DT_VCENTER|DT_SINGLELINE);  
      if (oldbkmode != TRANSPARENT)
	SetBkMode32(hdc, oldbkmode);
    }
    
}

/***********************************************************************
 *           DrawStatusText32W   (COMCTL32.24)
 */
void WINAPI DrawStatusText32W( HDC32 hdc, LPRECT32 lprc, LPCWSTR text,
                               UINT32 style )
{
  LPSTR p = HEAP_strdupWtoA( GetProcessHeap(), 0, text );
  DrawStatusText32A(hdc, lprc, p, style);
  HeapFree( GetProcessHeap(), 0, p );         
}

/***********************************************************************
 *           DrawStatusText16   (COMCTL32.23)
 */
void WINAPI DrawStatusText16( HDC16 hdc, LPRECT16 lprc, LPCSTR text,
			      UINT16 style )
{
  if(!lprc)
    DrawStatusText32A((HDC32)hdc, 0, text, (UINT32)style);
  else{    
    RECT32 rect32;
    CONV_RECT16TO32( lprc, &rect32 );
    DrawStatusText32A((HDC32)hdc, &rect32, text, (UINT32)style);
  }
}

/***********************************************************************
 *           CreateStatusWindow32A   (COMCTL32.6)
 */
HWND32 WINAPI CreateStatusWindow32A( INT32 style, LPCSTR text, HWND32 parent,
                                     UINT32 wid )
{
    return CreateWindow32A(STATUSCLASSNAME32A, text, style, 
			   CW_USEDEFAULT32, CW_USEDEFAULT32,
			   CW_USEDEFAULT32, CW_USEDEFAULT32, 
			   parent, wid, 0, 0);
}

/***********************************************************************
 *           CreateStatusWindow16   (COMCTL32.18)
 */
HWND16 WINAPI CreateStatusWindow16( INT16 style, LPCSTR text, HWND16 parent,
				    UINT16 wid )
{
    return CreateWindow16(STATUSCLASSNAME16, text, style, 
			   CW_USEDEFAULT16, CW_USEDEFAULT16,
			   CW_USEDEFAULT16, CW_USEDEFAULT16, 
			   parent, wid, 0, 0);
}

/***********************************************************************
 *           CreateStatusWindow32W   (COMCTL32.19)
 */
HWND32 WINAPI CreateStatusWindow32W( INT32 style, LPCWSTR text, HWND32 parent,
                                     UINT32 wid )
{
    return CreateWindow32W((LPCWSTR)STATUSCLASSNAME32W, text, style, 
			   CW_USEDEFAULT32, CW_USEDEFAULT32,
			   CW_USEDEFAULT32, CW_USEDEFAULT32, 
			   parent, wid, 0, 0);
}

/***********************************************************************
 *           CreateUpDownControl  (COMCTL32.16)
 */
HWND32 WINAPI CreateUpDownControl( DWORD style, INT32 x, INT32 y,
                                   INT32 cx, INT32 cy, HWND32 parent,
                                   INT32 id, HINSTANCE32 inst, HWND32 buddy,
                                   INT32 maxVal, INT32 minVal, INT32 curVal )
{
  HWND32 hUD = CreateWindow32A(UPDOWN_CLASS32A, 0, style, x, y, cx, cy,
			       parent, id, inst, 0);
  if(hUD){
    SendMessage32A(hUD, UDM_SETBUDDY, buddy, 0);
    SendMessage32A(hUD, UDM_SETRANGE, 0, MAKELONG(maxVal, minVal));
    SendMessage32A(hUD, UDM_SETPOS, 0, MAKELONG(curVal, 0));     
  }

  return hUD;
}


/***********************************************************************
 *           InitCommonControls   (COMCTL32.17)
 */
void WINAPI InitCommonControls(void)
{
    int i;
    char name[30];
    const char *old_name;
    WNDCLASS32A *class32 = WIDGETS_CommonControls32;

    for (i = 0; i < NB_COMMON_CONTROLS32; i++, class32++)
    {
        /* Just to make sure the string is > 0x10000 */
        old_name = class32->lpszClassName;
        strcpy( name, (char *)class32->lpszClassName );
        class32->lpszClassName = name;
        class32->hCursor = LoadCursor32A( 0, IDC_ARROW32A );
        RegisterClass32A( class32 );
        class32->lpszClassName = old_name;	
    }
}

