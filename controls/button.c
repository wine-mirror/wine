/*
 * Interface code to button widgets
 *
 * Copyright  David W. Metcalfe, 1993
 *
 */

static char Copyright[] = "Copyright  David W. Metcalfe, 1993";

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include "WinButton.h"
#include "windows.h"
#include "heap.h"
#include "win.h"

static void BUTTON_WinButtonCallback(Widget w, XtPointer client_data,
				               XtPointer call_data);

void BUTTON_CreateButton(LPSTR className, LPSTR buttonLabel, HWND hwnd)
{
    WND *wndPtr    = WIN_FindWndPtr(hwnd);
    WND *parentPtr = WIN_FindWndPtr(wndPtr->hwndParent);
    DWORD style;
    char widgetName[15];

#ifdef DEBUG_BUTTON
    printf("button: label = %s, x = %d, y = %d\n", buttonLabel,
	   wndPtr->rectClient.left, wndPtr->rectClient.top);
    printf("        width = %d, height = %d\n",
	   wndPtr->rectClient.right - wndPtr->rectClient.left,
	   wndPtr->rectClient.bottom - wndPtr->rectClient.top);
#endif

    if (!wndPtr)
	return;

    style = wndPtr->dwStyle & 0x0000000F;

    switch (style)
    {
    case BS_PUSHBUTTON:
    case BS_DEFPUSHBUTTON:
	sprintf(widgetName, "%s%d", className, wndPtr->wIDmenu);
	wndPtr->winWidget = XtVaCreateManagedWidget(widgetName,
				      winButtonWidgetClass,
				      parentPtr->winWidget,
				      XtNlabel, buttonLabel,
				      XtNx, wndPtr->rectClient.left,
				      XtNy, wndPtr->rectClient.top,
				      XtNwidth, wndPtr->rectClient.right -
					        wndPtr->rectClient.left,
				      XtNheight, wndPtr->rectClient.bottom -
					         wndPtr->rectClient.top,
				      XtVaTypedArg, XtNbackground, XtRString,
					      "grey70", strlen("grey75")+1,
				      NULL);

	XtAddCallback(wndPtr->winWidget, XtNcallback,
		      BUTTON_WinButtonCallback, (XtPointer) hwnd);
	break;

    default:
	printf("CreateButton: Unsupported button style %lX\n", 
	                                         wndPtr->dwStyle);
    }

    GlobalUnlock(hwnd);
    GlobalUnlock(wndPtr->hwndParent);
}

static void BUTTON_WinButtonCallback(Widget w, XtPointer client_data,
				               XtPointer call_data)
{
    HWND hwnd = (HWND) client_data;
    WND  *wndPtr;
    wndPtr = WIN_FindWndPtr(hwnd);

    CallWindowProc(wndPtr->lpfnWndProc, wndPtr->hwndParent, WM_COMMAND,
		   wndPtr->wIDmenu, MAKELPARAM(hwnd, BN_CLICKED));

    GlobalUnlock(hwnd);
}



