#include <windows.h>            /* required for all Windows applications */ 
#include "globals.h"            /* prototypes specific to this application */


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance, 
                     LPSTR     lpCmdLine, 
                     int       nCmdShow)
{
    MSG msg;
    HANDLE hAccelTable;
    
    /* Other instances of app running? */
    if (!hPrevInstance)
    {
      /* stuff to be done once */
      if (!InitApplication(hInstance))
      {
	return FALSE;              /* exit */ 
      }
    }

    /* stuff to be done every time */
    if (!InitInstance(hInstance, nCmdShow))
    {
      return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, szAppName);

    /* Main loop */
    /* Acquire and dispatch messages until a WM_QUIT message is received */
    while (GetMessage(&msg, NULL, 0, 0))
    {
      /* Add other Translation functions (for modeless dialogs
	 and/or MDI windows) here. */

      if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg); 
        }
    }

    /* Add module specific instance free/delete functions here */

    /* Returns the value from PostQuitMessage */
    return msg.wParam;
}
