/*
 * RichEdit32  functions
 *
 * This module is a simple wrapper for the edit controls.
 * At the point, it is good only for application who use the RICHEDIT 
 * control to display RTF text.
 *
 * Copyright 2000 by Jean-Claude Batista
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winerror.h"
#include "riched32.h"
#include "richedit.h"
#include "charlist.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"

#include "rtf.h"
#include "rtf2text.h"
#include "wine/debug.h"

#define ID_EDIT      1

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

HANDLE RICHED32_hHeap = (HANDLE)NULL;
/* LPSTR  RICHED32_aSubclass = (LPSTR)NULL; */

#define DPRINTF_EDIT_MSG32(str) \
        TRACE(\
                     "32 bit : " str ": hwnd=%08x, wParam=%08x, lParam=%08x\n"\
                     , \
                     hwnd, (UINT)wParam, (UINT)lParam)


/***********************************************************************
 * RICHED32_LibMain [Internal] Initializes the internal 'RICHED32.DLL'.
 *
 * PARAMS
 *     hinstDLL    [I] handle to the DLL's instance
 *     fdwReason   [I]
 *     lpvReserved [I] reserved, must be NULL
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
RICHED32_LibMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("\n");
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        /* create private heap */
        RICHED32_hHeap = HeapCreate (0, 0x10000, 0);
        /* register the Rich Edit class */
        RICHED32_Register ();
        break;

    case DLL_PROCESS_DETACH:
        /* unregister all common control classes */
        RICHED32_Unregister ();
        HeapDestroy (RICHED32_hHeap);
        RICHED32_hHeap = (HANDLE)NULL;
        break;
    }
    return TRUE;
}

/*
 *
 * DESCRIPTION:
 * Window procedure of the RichEdit control.
 *
 */
static LRESULT WINAPI RICHED32_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam)
{
    int RTFToBuffer(char* pBuffer, int nBufferSize);
    LONG newstyle = 0;
    LONG style = 0;  

    static HWND hwndEdit;
    static HWND hwndParent;
    static char* rtfBuffer;
    int rtfBufferSize;

    TRACE("\n"); 
   
    switch (uMsg)
    {
 
    case WM_CREATE :           
	    DPRINTF_EDIT_MSG32("WM_CREATE");
	             
	    /* remove SCROLLBARS from the current window style */
	    hwndParent = ((LPCREATESTRUCTA) lParam)->hwndParent;

	    newstyle = style = ((LPCREATESTRUCTA) lParam)->style;
            newstyle &= ~WS_HSCROLL;
            newstyle &= ~WS_VSCROLL;
            newstyle &= ~ES_AUTOHSCROLL;
            newstyle &= ~ES_AUTOVSCROLL;
                                  
            hwndEdit = CreateWindowA ("edit", ((LPCREATESTRUCTA) lParam)->lpszName,
                                   style, 0, 0, 0, 0,
                                   hwnd, (HMENU) ID_EDIT,
                                   ((LPCREATESTRUCTA) lParam)->hInstance, NULL) ;
	
	    SetWindowLongA(hwnd,GWL_STYLE, newstyle); 		   
            return 0 ;
          
    case WM_SETFOCUS :
	    DPRINTF_EDIT_MSG32("WM_SETFOCUS");            
            SetFocus (hwndEdit) ;
            return 0 ;

    case WM_SIZE :             
            DPRINTF_EDIT_MSG32("WM_SIZE");
            MoveWindow (hwndEdit, 0, 0, LOWORD (lParam), HIWORD (lParam), TRUE) ;
            return 0 ;
          
    case WM_COMMAND :
        DPRINTF_EDIT_MSG32("WM_COMMAND");
	switch(HIWORD(wParam)) {
		case EN_CHANGE:
		case EN_HSCROLL:
		case EN_KILLFOCUS:
		case EN_SETFOCUS:
		case EN_UPDATE:
		case EN_VSCROLL:
			return SendMessageA(hwndParent, WM_COMMAND, 
				wParam, (LPARAM)(hwnd));
		
		case EN_ERRSPACE:
		case EN_MAXTEXT:
			MessageBoxA (hwnd, "RichEdit control out of space.",
                                  "ERROR", MB_OK | MB_ICONSTOP) ;
			return 0 ;
		}
     
    case EM_STREAMIN:                           
            DPRINTF_EDIT_MSG32("EM_STREAMIN");
            
	    /* setup the RTF parser */
	    RTFSetEditStream(( EDITSTREAM*)lParam);
	    WriterInit();
	    RTFInit ();
	    BeginFile();	    

	    /* do the parsing */
	    RTFRead ();
            
	    rtfBufferSize = RTFToBuffer(NULL, 0);
	    rtfBuffer = HeapAlloc(RICHED32_hHeap, 0,rtfBufferSize*sizeof(char));
	    if(rtfBuffer)
	    {
	    	RTFToBuffer(rtfBuffer, rtfBufferSize);
            	SetWindowTextA(hwndEdit,rtfBuffer);
	    	HeapFree(RICHED32_hHeap, 0,rtfBuffer);
	    }
	    else
		WARN("Not enough memory for a allocating rtfBuffer\n");
		
            return 0;   

/* Message specific to Richedit controls */

    case EM_AUTOURLDETECT:
            DPRINTF_EDIT_MSG32("EM_AUTOURLDETECT");
	    return 0;

    case EM_CANPASTE:
            DPRINTF_EDIT_MSG32("EM_CANPASTE");
	    return 0;

    case EM_CANREDO:
            DPRINTF_EDIT_MSG32("EM_CANREDO");
	    return 0;

    case EM_DISPLAYBAND:
            DPRINTF_EDIT_MSG32("EM_DISPLAYBAND");
	    return 0;

    case EM_EXGETSEL:
            DPRINTF_EDIT_MSG32("EM_EXGETSEL");
            return 0;

    case EM_EXLIMITTEXT:
            DPRINTF_EDIT_MSG32("EM_EXLIMITTEXT");
            return 0;

    case EM_EXLINEFROMCHAR:
            DPRINTF_EDIT_MSG32("EM_EXLINEFROMCHAR");
            return 0;

    case EM_EXSETSEL:
            DPRINTF_EDIT_MSG32("EM_EXSETSEL");
            return 0;

    case EM_FINDTEXT:
            DPRINTF_EDIT_MSG32("EM_FINDTEXT");
            return 0;

    case EM_FINDTEXTEX:
            DPRINTF_EDIT_MSG32("EM_FINDTEXTEX");
            return 0;

    case EM_FINDTEXTEXW:
            DPRINTF_EDIT_MSG32("EM_FINDTEXTEXW");
            return 0;

    case EM_FINDTEXTW:
            DPRINTF_EDIT_MSG32("EM_FINDTEXTW");
            return 0;

    case EM_FINDWORDBREAK:
            DPRINTF_EDIT_MSG32("EM_FINDWORDBREAK");
            return 0;

    case EM_FORMATRANGE:
            DPRINTF_EDIT_MSG32("EM_FORMATRANGE");
            return 0;

    case EM_GETAUTOURLDETECT:
            DPRINTF_EDIT_MSG32("EM_GETAUTOURLDETECT");
            return 0;

    case EM_GETBIDIOPTIONS:
            DPRINTF_EDIT_MSG32("EM_GETBIDIOPTIONS");
            return 0;

    case EM_GETCHARFORMAT:
            DPRINTF_EDIT_MSG32("EM_GETCHARFORMAT");
            return 0;

    case EM_GETEDITSTYLE:
            DPRINTF_EDIT_MSG32("EM_GETEDITSTYLE");
            return 0;

    case EM_GETEVENTMASK:
            DPRINTF_EDIT_MSG32("EM_GETEVENTMASK");
            return 0;

    case EM_GETIMECOLOR:
            DPRINTF_EDIT_MSG32("EM_GETIMECOLOR");
            return 0;

    case EM_GETIMECOMPMODE:
            DPRINTF_EDIT_MSG32("EM_GETIMECOMPMODE");
            return 0;

    case EM_GETIMEOPTIONS:
            DPRINTF_EDIT_MSG32("EM_GETIMEOPTIONS");
            return 0;

    case EM_GETLANGOPTIONS:
            DPRINTF_EDIT_MSG32("EM_GETLANGOPTIONS");
            return 0;

    case EM_GETOLEINTERFACE:
            DPRINTF_EDIT_MSG32("EM_GETOLEINTERFACE");
            return 0;

    case EM_GETOPTIONS:
            DPRINTF_EDIT_MSG32("EM_GETOPTIONS");
            return 0;

    case EM_GETPARAFORMAT:
            DPRINTF_EDIT_MSG32("EM_GETPARAFORMAT");
            return 0;

    case EM_GETPUNCTUATION:
            DPRINTF_EDIT_MSG32("EM_GETPUNCTUATION");
            return 0;

    case EM_GETREDONAME:
            DPRINTF_EDIT_MSG32("EM_GETREDONAME");
            return 0;

    case EM_GETSCROLLPOS:
            DPRINTF_EDIT_MSG32("EM_GETSCROLLPOS");
            return 0;

    case EM_GETSELTEXT:
            DPRINTF_EDIT_MSG32("EM_GETSELTEXT");
            return 0;

    case EM_GETTEXTEX:
            DPRINTF_EDIT_MSG32("EM_GETTEXTEX");
            return 0;

    case EM_GETTEXTLENGTHEX:
            DPRINTF_EDIT_MSG32("EM_GETTEXTLENGTHEX");
            return 0;

    case EM_GETTEXTMODE:
            DPRINTF_EDIT_MSG32("EM_GETTEXTMODE");
            return 0;

    case EM_GETTEXTRANGE:
            DPRINTF_EDIT_MSG32("EM_GETTEXTRANGE");
            return 0;

    case EM_GETTYPOGRAPHYOPTIONS:
            DPRINTF_EDIT_MSG32("EM_GETTYPOGRAPHYOPTIONS");
            return 0;

    case EM_GETUNDONAME:
            DPRINTF_EDIT_MSG32("EM_GETUNDONAME");
            return 0;

    case EM_GETWORDBREAKPROCEX:
            DPRINTF_EDIT_MSG32("EM_GETWORDBREAKPROCEX");
            return 0;

    case EM_GETWORDWRAPMODE:
            DPRINTF_EDIT_MSG32("EM_GETWORDWRAPMODE");
            return 0;

    case EM_GETZOOM:
            DPRINTF_EDIT_MSG32("EM_GETZOOM");
            return 0;

    case EM_HIDESELECTION:
            DPRINTF_EDIT_MSG32("EM_HIDESELECTION");
            return 0;

    case EM_PASTESPECIAL:
            DPRINTF_EDIT_MSG32("EM_PASTESPECIAL");
            return 0;

    case EM_RECONVERSION:
            DPRINTF_EDIT_MSG32("EM_RECONVERSION");
            return 0;

    case EM_REDO:
            DPRINTF_EDIT_MSG32("EM_REDO");
            return 0;

    case EM_REQUESTRESIZE:
            DPRINTF_EDIT_MSG32("EM_REQUESTRESIZE");
            return 0;

    case EM_SELECTIONTYPE:
            DPRINTF_EDIT_MSG32("EM_SELECTIONTYPE");
            return 0;

    case EM_SETBIDIOPTIONS:
            DPRINTF_EDIT_MSG32("EM_SETBIDIOPTIONS");
            return 0;

    case EM_SETBKGNDCOLOR:
            DPRINTF_EDIT_MSG32("EM_SETBKGNDCOLOR");
            return 0;

    case EM_SETCHARFORMAT:
            DPRINTF_EDIT_MSG32("EM_SETCHARFORMAT");
            return 0;

    case EM_SETEDITSTYLE:
            DPRINTF_EDIT_MSG32("EM_SETEDITSTYLE");
            return 0;

    case EM_SETEVENTMASK:
            DPRINTF_EDIT_MSG32("EM_SETEVENTMASK");
            return 0;

    case EM_SETFONTSIZE:
            DPRINTF_EDIT_MSG32("EM_SETFONTSIZE");
            return 0;

    case EM_SETIMECOLOR:
            DPRINTF_EDIT_MSG32("EM_SETIMECOLO");
            return 0;

    case EM_SETIMEOPTIONS:
            DPRINTF_EDIT_MSG32("EM_SETIMEOPTIONS");
            return 0;

    case EM_SETLANGOPTIONS:
            DPRINTF_EDIT_MSG32("EM_SETLANGOPTIONS");
            return 0;

    case EM_SETOLECALLBACK:
            DPRINTF_EDIT_MSG32("EM_SETOLECALLBACK");
            return 0;

    case EM_SETOPTIONS:
            DPRINTF_EDIT_MSG32("EM_SETOPTIONS");
            return 0;

    case EM_SETPALETTE:
            DPRINTF_EDIT_MSG32("EM_SETPALETTE");
            return 0;

    case EM_SETPARAFORMAT:
            DPRINTF_EDIT_MSG32("EM_SETPARAFORMAT");
            return 0;

    case EM_SETPUNCTUATION:
            DPRINTF_EDIT_MSG32("EM_SETPUNCTUATION");
            return 0;

    case EM_SETSCROLLPOS:
            DPRINTF_EDIT_MSG32("EM_SETSCROLLPOS");
            return 0;

    case EM_SETTARGETDEVICE:
            DPRINTF_EDIT_MSG32("EM_SETTARGETDEVICE");
            return 0;

    case EM_SETTEXTEX:
            DPRINTF_EDIT_MSG32("EM_SETTEXTEX");
            return 0;

    case EM_SETTEXTMODE:
            DPRINTF_EDIT_MSG32("EM_SETTEXTMODE");
            return 0;

    case EM_SETTYPOGRAPHYOPTIONS:
            DPRINTF_EDIT_MSG32("EM_SETTYPOGRAPHYOPTIONS");
            return 0;

    case EM_SETUNDOLIMIT:
            DPRINTF_EDIT_MSG32("EM_SETUNDOLIMIT");
            return 0;

    case EM_SETWORDBREAKPROCEX:
            DPRINTF_EDIT_MSG32("EM_SETWORDBREAKPROCEX");
            return 0;

    case EM_SETWORDWRAPMODE:
            DPRINTF_EDIT_MSG32("EM_SETWORDWRAPMODE");
            return 0;

    case EM_SETZOOM:
            DPRINTF_EDIT_MSG32("EM_SETZOOM");
            return 0;

    case EM_SHOWSCROLLBAR:
            DPRINTF_EDIT_MSG32("EM_SHOWSCROLLBAR");
            return 0;

    case EM_STOPGROUPTYPING:
            DPRINTF_EDIT_MSG32("EM_STOPGROUPTYPING");
            return 0;

    case EM_STREAMOUT:
            DPRINTF_EDIT_MSG32("EM_STREAMOUT");
            return 0;

/* Messaged dispatched to the edit control */
    case EM_CANUNDO:
    case EM_CHARFROMPOS:
    case EM_EMPTYUNDOBUFFER:
    case EM_FMTLINES:
    case EM_GETFIRSTVISIBLELINE:
    case EM_GETHANDLE:
/*    case EM_GETIMESTATUS:*/
    case EM_GETLIMITTEXT:
    case EM_GETLINE:
    case EM_GETLINECOUNT:
    case EM_GETMARGINS:
    case EM_GETMODIFY:
    case EM_GETPASSWORDCHAR:
    case EM_GETRECT:
    case EM_GETSEL:
    case EM_GETTHUMB:
    case EM_GETWORDBREAKPROC:
    case EM_LINEFROMCHAR:
    case EM_LINEINDEX:
    case EM_LINELENGTH:
    case EM_LINESCROLL:
    case EM_POSFROMCHAR:
    case EM_REPLACESEL:
    case EM_SCROLL:
    case EM_SCROLLCARET:
    case EM_SETHANDLE:
/*    case EM_SETIMESTATUS:*/
    case EM_SETLIMITTEXT:
    case EM_SETMARGINS:
    case EM_SETMODIFY:
    case EM_SETPASSWORDCHAR:
    case EM_SETREADONLY:
    case EM_SETRECT:
    case EM_SETRECTNP:
    case EM_SETSEL:
    case EM_SETTABSTOPS:
    case EM_SETWORDBREAKPROC:
    case EM_UNDO:

    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
    case WM_NCCALCSIZE:
    case WM_GETTEXT:
    case WM_GETTEXTLENGTH:
    case WM_SETTEXT:
	    return SendMessageA( hwndEdit, uMsg, wParam, lParam);

    /* Messages known , but ignored. */ 
    case WM_NCPAINT:
        DPRINTF_EDIT_MSG32("WM_NCPAINT");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_PAINT:
        DPRINTF_EDIT_MSG32("WM_PAINT");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_ERASEBKGND:
        DPRINTF_EDIT_MSG32("WM_ERASEBKGND");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_KILLFOCUS:
        DPRINTF_EDIT_MSG32("WM_KILLFOCUS");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_DESTROY:
        DPRINTF_EDIT_MSG32("WM_DESTROY");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_CHILDACTIVATE:	       
	DPRINTF_EDIT_MSG32("WM_CHILDACTIVATE");
	return DefWindowProcA( hwnd,uMsg,wParam,lParam);

    case WM_WINDOWPOSCHANGING:
        DPRINTF_EDIT_MSG32("WM_WINDOWPOSCHANGING");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_WINDOWPOSCHANGED:
        DPRINTF_EDIT_MSG32("WM_WINDOWPOSCHANGED");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
/*    case WM_INITIALUPDATE:
        DPRINTF_EDIT_MSG32("WM_INITIALUPDATE");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam); */
    case WM_CTLCOLOREDIT:
        DPRINTF_EDIT_MSG32("WM_CTLCOLOREDIT");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_SETCURSOR:
        DPRINTF_EDIT_MSG32("WM_SETCURSOR");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_MOVE:
        DPRINTF_EDIT_MSG32("WM_MOVE");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_SHOWWINDOW:
        DPRINTF_EDIT_MSG32("WM_SHOWWINDOW");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);

    }


    FIXME("Unknown message 0x%04x\n", uMsg);
    return DefWindowProcA( hwnd,uMsg,wParam,lParam);
}

/***********************************************************************
 * DllGetVersion [RICHED32.2]
 *
 * Retrieves version information of the 'RICHED32.DLL'
 *
 * PARAMS
 *     pdvi [O] pointer to version information structure.
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: E_INVALIDARG
 *
 * NOTES
 *     Returns version of a comctl32.dll from IE4.01 SP1.
 */

HRESULT WINAPI
RICHED32_DllGetVersion (DLLVERSIONINFO *pdvi)
{
    TRACE("\n");

    if (pdvi->cbSize != sizeof(DLLVERSIONINFO)) {
 
	return E_INVALIDARG;
    }

    pdvi->dwMajorVersion = 4;
    pdvi->dwMinorVersion = 0;
    pdvi->dwBuildNumber = 0;
    pdvi->dwPlatformID = 0;

    return S_OK;
}

/***
 * DESCRIPTION:
 * Registers the window class.
 * 
 * PARAMETER(S):
 * None
 *
 * RETURN:
 * None
 */
VOID RICHED32_Register(void)
{
    WNDCLASSA wndClass; 

    TRACE("\n");

    ZeroMemory(&wndClass, sizeof(WNDCLASSA));
    wndClass.style = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
    wndClass.lpfnWndProc = (WNDPROC)RICHED32_WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0; /*(sizeof(RICHED32_INFO *);*/
    wndClass.hCursor = LoadCursorA(0, IDC_ARROWA);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = RICHEDIT_CLASS10A;//WC_RICHED32A;

    RegisterClassA (&wndClass);
}

/***
 * DESCRIPTION:
 * Unregisters the window class.
 * 
 * PARAMETER(S):
 * None
 *
 * RETURN:
 * None
 */
VOID RICHED32_Unregister(void)
{
    TRACE("\n");

    UnregisterClassA(RICHEDIT_CLASS10A, (HINSTANCE)NULL);
}
