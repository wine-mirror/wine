/*
 * Date and time picker control
 *
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 1999 Alex Priem <alexp@sci.kun.nl>
 *
 *
 * TODO:
 *   - All messages.
 *   - All notifications.
 *
 */

#include "winbase.h"
#include "commctrl.h"
#include "datetime.h"
#include "monthcal.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(datetime)



#define DATETIME_GetInfoPtr(hwnd) ((DATETIME_INFO *)GetWindowLongA (hwnd, 0))
static BOOL

DATETIME_SendSimpleNotify (HWND hwnd, UINT code);

static const char * const days[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
"Thursday", "Friday", "Saturday", NULL};

static LRESULT
DATETIME_GetSystemTime (HWND hwnd, WPARAM wParam, LPARAM lParam )
{
  DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);
  DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
  SYSTEMTIME *lprgSysTimeArray=(SYSTEMTIME *) lParam;

  if (!lParam) return GDT_NONE;

  if ((dwStyle & DTS_SHOWNONE) && 
       (SendMessageA (infoPtr->hwndCheckbut, BM_GETCHECK, 0, 0)))
        return GDT_NONE;

  MONTHCAL_CopyTime (&infoPtr->date, lprgSysTimeArray);

  return GDT_VALID;
}


static LRESULT
DATETIME_SetSystemTime (HWND hwnd, WPARAM wParam, LPARAM lParam )
{
  DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);
  SYSTEMTIME *lprgSysTimeArray=(SYSTEMTIME *) lParam;

  if (!lParam) return 0;

  if (lParam==GDT_VALID) 
  	MONTHCAL_CopyTime (lprgSysTimeArray, &infoPtr->date);
  if (lParam==GDT_NONE) {
	infoPtr->dateValid=FALSE;
    SendMessageA (infoPtr->hwndCheckbut, BM_SETCHECK, 0, 0);
	}
  return 1;
}


static LRESULT
DATETIME_GetMonthCalColor (HWND hwnd, WPARAM wParam)
{
  DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);

  return SendMessageA (infoPtr->hMonthCal, MCM_GETCOLOR, wParam, 0);
}

static LRESULT
DATETIME_SetMonthCalColor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);

  return SendMessageA (infoPtr->hMonthCal, MCM_SETCOLOR, wParam, lParam);
}


/* FIXME: need to get way to force font into monthcal structure */

static LRESULT
DATETIME_GetMonthCal (HWND hwnd)
{
  DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);

  return infoPtr->hMonthCal;
}



/* FIXME: need to get way to force font into monthcal structure */

static LRESULT
DATETIME_GetMonthCalFont (HWND hwnd)
{

  return 0;
}

static LRESULT
DATETIME_SetMonthCalFont (HWND hwnd, WPARAM wParam, LPARAM lParam)
{

  return 0;
}


static void DATETIME_Refresh (HWND hwnd, HDC hdc)

{
  DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);
  RECT *daytxt   = &infoPtr->daytxt;
  RECT *daynumtxt= &infoPtr->daynumtxt;
  RECT *rmonthtxt= &infoPtr->rmonthtxt;
  RECT *yeartxt  = &infoPtr->yeartxt;
  RECT *calbutton= &infoPtr->calbutton;
  RECT *checkbox = &infoPtr->checkbox;
  RECT *rect     = &infoPtr->rect;
  DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
  SYSTEMTIME date = infoPtr->date;
  SIZE size;
  BOOL prssed=FALSE;
  COLORREF oldBk = 0;
  

	
  if (infoPtr->dateValid) {
    char txt[80];
	HFONT oldFont;
  	oldFont = SelectObject (hdc, infoPtr->hFont);

	GetClientRect (hwnd, rect);

	sprintf (txt,"%s,",days[date.wDayOfWeek]);
	GetTextExtentPoint32A (hdc, txt, strlen (txt), &size);
	rect->bottom=size.cy+2;

	checkbox->left  = 0;
	checkbox->right = 0;
	checkbox->top   = rect->top;
	checkbox->bottom= rect->bottom;
    if (dwStyle & DTS_SHOWNONE) {    /* FIXME: draw checkbox */
		checkbox->right=18;
		}


	if (infoPtr->select==(DTHT_DAY|DTHT_GOTFOCUS))
		oldBk=SetBkColor (hdc, COLOR_GRAYTEXT);
       daytxt->left  = checkbox->right;
       daytxt->right = checkbox->right+size.cx;
       daytxt->top   = rect->top;
       daytxt->bottom= rect->bottom;
       DrawTextA ( hdc, txt, strlen(txt), daytxt,
                         DT_LEFT | DT_VCENTER | DT_SINGLELINE );
	if (infoPtr->select==(DTHT_DAY|DTHT_GOTFOCUS)) 
		SetBkColor (hdc, oldBk);

	if (infoPtr->select==(DTHT_MONTH|DTHT_GOTFOCUS)) 
		oldBk=SetBkColor (hdc, COLOR_GRAYTEXT);
	strcpy (txt, monthtxt[date.wMonth]);
	GetTextExtentPoint32A (hdc, "September", 9, &size);
       rmonthtxt->left  = daytxt->right;
       rmonthtxt->right = daytxt->right+size.cx;
	rmonthtxt->top   = rect->top;
	rmonthtxt->bottom= rect->bottom;
	DrawTextA ( hdc, txt, strlen(txt), rmonthtxt,
                         DT_CENTER | DT_VCENTER | DT_SINGLELINE );
	if (infoPtr->select==(DTHT_MONTH|DTHT_GOTFOCUS)) 
		SetBkColor (hdc, oldBk);

	if (infoPtr->select==(DTHT_DAYNUM|DTHT_GOTFOCUS)) 
		oldBk=SetBkColor (hdc, COLOR_GRAYTEXT);
	sprintf (txt,"%d,",date.wDay);
	GetTextExtentPoint32A (hdc, "31,", 3, &size);
       daynumtxt->left  = rmonthtxt->right;
       daynumtxt->right = rmonthtxt->right+size.cx;
	daynumtxt->top   = rect->top;
	daynumtxt->bottom= rect->bottom;
	DrawTextA ( hdc, txt, strlen(txt), daynumtxt,
                         DT_RIGHT | DT_VCENTER | DT_SINGLELINE );
	if (infoPtr->select==(DTHT_DAYNUM|DTHT_GOTFOCUS)) 
		SetBkColor (hdc, oldBk);

	if (infoPtr->select==(DTHT_YEAR|DTHT_GOTFOCUS)) 
		oldBk=SetBkColor (hdc, COLOR_GRAYTEXT);
	sprintf (txt,"%d",date.wYear);
	GetTextExtentPoint32A (hdc, "2000", 5, &size);
       yeartxt->left  = daynumtxt->right;
       yeartxt->right = daynumtxt->right+size.cx;
	yeartxt->top   = rect->top;
	yeartxt->bottom= rect->bottom;
	DrawTextA ( hdc, txt, strlen(txt), yeartxt,
                         DT_RIGHT | DT_VCENTER | DT_SINGLELINE );
	if (infoPtr->select==(DTHT_YEAR|DTHT_GOTFOCUS)) 
		SetBkColor (hdc, oldBk);
	
  	SelectObject (hdc, oldFont);
	}

	if (!(dwStyle & DTS_UPDOWN)) {

		calbutton->right = rect->right;
		calbutton->left  = rect->right-15;
		calbutton->top   = rect->top;
		calbutton->bottom= rect->bottom;

    	DrawFrameControl(hdc, calbutton, DFC_SCROLL,
        	DFCS_SCROLLDOWN | (prssed ? DFCS_PUSHED : 0) |
        	(dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0) );
	}

}

static LRESULT
DATETIME_HitTest (HWND hwnd, DATETIME_INFO *infoPtr, POINT pt)
{
  TRACE ("%ld, %ld\n",pt.x,pt.y);

  if (PtInRect (&infoPtr->calbutton, pt)) return DTHT_MCPOPUP;
  if (PtInRect (&infoPtr->yeartxt, pt))   return DTHT_YEAR;
  if (PtInRect (&infoPtr->daynumtxt, pt)) return DTHT_DAYNUM;
  if (PtInRect (&infoPtr->rmonthtxt, pt)) return DTHT_MONTH;
  if (PtInRect (&infoPtr->daytxt, pt))    return DTHT_DAY;
  if (PtInRect (&infoPtr->checkbox, pt))  return DTHT_CHECKBOX;

  return 0;
}

static LRESULT
DATETIME_LButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);	
	POINT pt;
	int old;

    TRACE ("\n");

	old=infoPtr->select;
    pt.x=(INT)LOWORD(lParam);
    pt.y=(INT)HIWORD(lParam);
    infoPtr->select=DATETIME_HitTest (hwnd, infoPtr, pt);
                           
	if (infoPtr->select!=old) {
		HDC hdc;

		SetFocus (hwnd);
		hdc=GetDC (hwnd);
		DATETIME_Refresh (hwnd,hdc);
        ReleaseDC (hwnd, hdc);
    }
	if (infoPtr->select==DTHT_MCPOPUP) {
			POINT pt;

			pt.x=8;
			pt.y=infoPtr->rect.bottom+5;
			ClientToScreen (hwnd, &pt);
			infoPtr->hMonthCal=CreateWindowExA (0,"SysMonthCal32", 0, 
				WS_POPUP | WS_BORDER,
				pt.x,pt.y,145,150,
				GetParent (hwnd), 
				0,0,0);

			TRACE ("dt:%x mc:%x mc parent:%x, desktop:%x, mcpp:%x\n",
                      hwnd,infoPtr->hMonthCal, 
                      GetParent (infoPtr->hMonthCal),
                      GetDesktopWindow (),
                      GetParent (GetParent (infoPtr->hMonthCal)));

			SetFocus (hwnd);
			DATETIME_SendSimpleNotify (hwnd, DTN_DROPDOWN);
	}
    return 0;
}


static LRESULT
DATETIME_Paint (HWND hwnd, WPARAM wParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = wParam==0 ? BeginPaint (hwnd, &ps) : (HDC)wParam;
    DATETIME_Refresh (hwnd, hdc);
    if(!wParam)
    EndPaint (hwnd, &ps);
    return 0;
}

static LRESULT
DATETIME_ParentNotify (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);	
	LPNMHDR lpnmh=(LPNMHDR) lParam;

	TRACE ("%x,%lx\n",wParam, lParam);
	TRACE ("Got notification %x from %x\n", lpnmh->code, lpnmh->hwndFrom);
	TRACE ("info: %x %x %x\n",hwnd,infoPtr->hMonthCal,infoPtr->hUpdown);
	return 0;
}

static LRESULT
DATETIME_Notify (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
    DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);	
	LPNMHDR lpnmh=(LPNMHDR) lParam;

	TRACE ("%x,%lx\n",wParam, lParam);
	TRACE ("Got notification %x from %x\n", lpnmh->code, lpnmh->hwndFrom);
	TRACE ("info: %x %x %x\n",hwnd,infoPtr->hMonthCal,infoPtr->hUpdown);
	return 0;
}

static LRESULT
DATETIME_KillFocus (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);	
    HDC hdc;

    TRACE ("\n");

	if (infoPtr->select) {
			DATETIME_SendSimpleNotify (hwnd, NM_KILLFOCUS);
			infoPtr->select&= ~DTHT_GOTFOCUS;
	}
    hdc = GetDC (hwnd);
    DATETIME_Refresh (hwnd, hdc);
    ReleaseDC (hwnd, hdc);
    InvalidateRect (hwnd, NULL, TRUE);

    return 0;
}


static LRESULT
DATETIME_SetFocus (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);	

    TRACE ("\n");

	if (infoPtr->select) {
			DATETIME_SendSimpleNotify (hwnd, NM_SETFOCUS);	
			infoPtr->select|=DTHT_GOTFOCUS;
	}
    hdc = GetDC (hwnd);
    DATETIME_Refresh (hwnd, hdc);
    ReleaseDC (hwnd, hdc);

    return 0;
}


static BOOL
DATETIME_SendSimpleNotify (HWND hwnd, UINT code)
{
    NMHDR nmhdr;

    TRACE("%x\n",code);
    nmhdr.hwndFrom = hwnd;
    nmhdr.idFrom   = GetWindowLongA( hwnd, GWL_ID);
    nmhdr.code     = code;

    return (BOOL) SendMessageA (GetParent (hwnd), WM_NOTIFY,
                                   (WPARAM)nmhdr.idFrom, (LPARAM)&nmhdr);
}






static LRESULT
DATETIME_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  DATETIME_INFO *infoPtr;
  DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);

    /* allocate memory for info structure */
    infoPtr = (DATETIME_INFO *)COMCTL32_Alloc (sizeof(DATETIME_INFO));
    if (infoPtr == NULL) {
	ERR("could not allocate info memory!\n");
	return 0;
    }

    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);

	if (dwStyle & DTS_SHOWNONE) {
		infoPtr->hwndCheckbut=CreateWindowExA (0,"button", 0, 
				WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 
				2,2,13,13,
				hwnd, 
                0, GetWindowLongA  (hwnd, GWL_HINSTANCE), 0);
		SendMessageA (infoPtr->hwndCheckbut, BM_SETCHECK, 1, 0);
	}

	if (dwStyle & DTS_UPDOWN) {

			infoPtr->hUpdown=CreateUpDownControl (
				WS_CHILD | WS_BORDER | WS_VISIBLE,
				120,1,20,20,
				hwnd,1,0,0,
				UD_MAXVAL, UD_MINVAL, 0);
	}

    /* initialize info structure */
	infoPtr->hMonthCal=0;
	GetSystemTime (&infoPtr->date);
	infoPtr->dateValid = TRUE;
	infoPtr->hFont = GetStockObject(DEFAULT_GUI_FONT);
    return 0;
}


static LRESULT
DATETIME_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);
	
    COMCTL32_Free (infoPtr);
    return 0;
}





static LRESULT WINAPI
DATETIME_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    switch (uMsg)
    {

    case DTM_GETSYSTEMTIME:
		DATETIME_GetSystemTime (hwnd, wParam, lParam);

    case DTM_SETSYSTEMTIME:
		DATETIME_SetSystemTime (hwnd, wParam, lParam);

    case DTM_GETRANGE:
        FIXME("Unimplemented msg DTM_GETRANGE\n");
        return 0;

    case DTM_SETRANGE:
        FIXME("Unimplemented msg DTM_SETRANGE\n");
        return 1;

    case DTM_SETFORMATA:
        FIXME("Unimplemented msg DTM_SETFORMAT32A\n");
        return 1;

    case DTM_SETFORMATW:
        FIXME("Unimplemented msg DTM_SETFORMAT32W\n");
        return 1;

    case DTM_SETMCCOLOR:
        return DATETIME_SetMonthCalColor (hwnd, wParam, lParam);

    case DTM_GETMCCOLOR:
        return DATETIME_GetMonthCalColor (hwnd, wParam);

    case DTM_GETMONTHCAL:
        return DATETIME_GetMonthCal (hwnd);

    case DTM_SETMCFONT:
        return DATETIME_SetMonthCalFont (hwnd, wParam, lParam);

    case DTM_GETMCFONT:
        return DATETIME_GetMonthCalFont (hwnd);

	case WM_PARENTNOTIFY:
		return DATETIME_ParentNotify (hwnd, wParam, lParam);

	case WM_NOTIFY:
		return DATETIME_Notify (hwnd, wParam, lParam);

    case WM_PAINT:
        return DATETIME_Paint (hwnd, wParam);

    case WM_KILLFOCUS:
        return DATETIME_KillFocus (hwnd, wParam, lParam);

    case WM_SETFOCUS:
        return DATETIME_SetFocus (hwnd, wParam, lParam);

    case WM_LBUTTONDOWN:
        return DATETIME_LButtonDown (hwnd, wParam, lParam);

	case WM_CREATE:
	    return DATETIME_Create (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return DATETIME_Destroy (hwnd, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR("unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
DATETIME_Register (void)
{
    WNDCLASSA wndClass;

    if (GlobalFindAtomA (DATETIMEPICK_CLASSA)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC)DATETIME_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(DATETIME_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = DATETIMEPICK_CLASSA;
 
    RegisterClassA (&wndClass);
}


VOID
DATETIME_Unregister (void)
{
    if (GlobalFindAtomA (DATETIMEPICK_CLASSA))
	UnregisterClassA (DATETIMEPICK_CLASSA, (HINSTANCE)NULL);
}

