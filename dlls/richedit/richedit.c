/*
 * RichEdit32  functions
 *
 * This module is a simple wrap-arround the edit controls.
 * At the point, it is good only for application who use the RICHEDIT control to 
 * display RTF text.
 *
 * Copyright 2000 by Jean-Claude Batista
 * 
 */
 
#include "windows.h"
#include "winbase.h"
#include "heap.h"
#include "debugtools.h"
#include "winerror.h"
#include "riched32.h"
#include "richedit.h"
#include "charlist.h"

#include "rtf.h"
#include "rtf2text.h"

#define ID_EDIT      1

DEFAULT_DEBUG_CHANNEL(richedit)

HANDLE RICHED32_hHeap = (HANDLE)NULL;
DWORD  RICHED32_dwProcessesAttached = 0;
/* LPSTR  RICHED32_aSubclass = (LPSTR)NULL; */
HMODULE RICHED32_hModule = 0;

/***********************************************************************
 * RICHED32_LibMain [Internal] Initializes the internal 'RICHED32.DLL'.
 *
 * PARAMS
 *     hinstDLL    [I] handle to the 'dlls' instance
 *     fdwReason   [I]
 *     lpvReserved [I] reserverd, must be NULL
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
RICHED32_LibMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
        
    switch (fdwReason) {
	    case DLL_PROCESS_ATTACH:

            if (RICHED32_dwProcessesAttached == 0) {

		    /* This will be wrong for any other process attching in this address-space! */
	            RICHED32_hModule = (HMODULE)hinstDLL;

		        /* create private heap */
		        RICHED32_hHeap = HeapCreate (0, 0x10000, 0);

	        }
		
	        /* register the Rich Edit class */
	        RICHED32_Register ();

	        RICHED32_dwProcessesAttached++;
	        break;

	    case DLL_PROCESS_DETACH:
	        RICHED32_dwProcessesAttached--;

	        /* unregister all common control classes */      
	        RICHED32_Unregister ();

	        if (RICHED32_dwProcessesAttached == 0) {
		    HeapDestroy (RICHED32_hHeap);
		    RICHED32_hHeap = (HANDLE)NULL;
                }
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
    static char* rtfBuffer;
    int rtfBufferSize;
    
    switch (uMsg)
    {
 
    case WM_CREATE :           
            
	    /* remove SCROLLBARS from the current window style */
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
            SetFocus (hwndEdit) ;
            return 0 ;
          
    case WM_SIZE :             
            MoveWindow (hwndEdit, 0, 0, LOWORD (lParam), HIWORD (lParam), TRUE) ;
            return 0 ;
          
    case WM_COMMAND :
            if (LOWORD (wParam) == ID_EDIT)
                 if (HIWORD (wParam) == EN_ERRSPACE || 
                           HIWORD (wParam) == EN_MAXTEXT)

                      MessageBoxA (hwnd, "RichEdit control out of space.",
                                  "ERROR", MB_OK | MB_ICONSTOP) ;
            return 0 ;
     
    case EM_STREAMIN:                               
            
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
    }
    /*return SendMessageA( hwndEdit,uMsg,wParam,lParam);*/
    return DefWindowProcA( hwnd,uMsg,wParam,lParam);
}

/***********************************************************************
 * DllGetVersion [COMCTL32.25]
 *
 * Retrieves version information of the 'COMCTL32.DLL'
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
    UnregisterClassA(RICHEDIT_CLASS10A, (HINSTANCE)NULL);
}
