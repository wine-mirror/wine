/*
 * Desktop window class.
 *
 * Copyright 1994 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1994";

#include "windows.h"

LONG DesktopWndProc ( HWND hwnd, WORD message, WORD wParam, LONG lParam )
{
      /* Most messages are ignored (we DON'T call DefWindowProc) */

    switch(message)
    {
    }
    
    return 0;
}

