/*
 * IP Address control
 *
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 1998 Alex Priem <alexp@sci.kun.nl>
 *
 * NOTES

 *
 * TODO:
 *    -Check ranges when changing field-focus.
 * 	  -Check all notifications/behavior.
 *    -Optimization: include lpipsi in IPADDRESS_INFO.
 *	  -CurrentFocus: field that has focus at moment of processing.
 *	  -connect Rect32 rcClient.
 *	  -handle right and left arrows correctly. Boring.
 *	  -split GotoNextField in CheckField and GotoNextField.
 *	  -check ipaddress.cpp for missing features.
 *    -refresh: draw '.' instead of setpixel.
 *	  -handle VK_ESCAPE.
 */

#include <ctype.h>
#include <stdlib.h>

#include "winbase.h"
#include "commctrl.h"
#include "ipaddress.h"
#include "heap.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(ipaddress)


#define IPADDRESS_GetInfoPtr(hwnd) ((IPADDRESS_INFO *)GetWindowLongA (hwnd, 0))


static BOOL 
IPADDRESS_SendNotify (HWND hwnd, UINT command);
static BOOL 
IPADDRESS_SendIPAddressNotify (HWND hwnd, UINT field, BYTE newValue);


/* property name of tooltip window handle */
#define IP_SUBCLASS_PROP "CCIP32SubclassInfo"


static LRESULT CALLBACK
IPADDRESS_SubclassProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);




static VOID
IPADDRESS_Refresh (HWND hwnd, HDC hdc)
{
	RECT rcClient;
	HBRUSH hbr;
	COLORREF clr=GetSysColor (COLOR_3DDKSHADOW);
    int i,x,fieldsize;

    GetClientRect (hwnd, &rcClient);
	hbr =  CreateSolidBrush (RGB(255,255,255));
    DrawEdge (hdc, &rcClient, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
	FillRect (hdc, &rcClient, hbr);
    DeleteObject (hbr);

	x=rcClient.left;
	fieldsize=(rcClient.right-rcClient.left) /4;

	for (i=0; i<3; i++) {		/* Should draw text "." here */
		x+=fieldsize;
		SetPixel (hdc, x,   13, clr);
		SetPixel (hdc, x,   14, clr);
		SetPixel (hdc, x+1, 13, clr);
		SetPixel (hdc, x+1, 14, clr);

	}

}





static LRESULT
IPADDRESS_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    IPADDRESS_INFO *infoPtr;
	RECT rcClient, edit;
	int i,fieldsize;
	LPIP_SUBCLASS_INFO lpipsi;
	

    infoPtr = (IPADDRESS_INFO *)COMCTL32_Alloc (sizeof(IPADDRESS_INFO));
    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);

    GetClientRect (hwnd, &rcClient);

	fieldsize=(rcClient.right-rcClient.left) /4;

	edit.top   =rcClient.top+2;
	edit.bottom=rcClient.bottom-2;

	lpipsi=(LPIP_SUBCLASS_INFO)
			GetPropA ((HWND)hwnd, IP_SUBCLASS_PROP);
	if (lpipsi == NULL)  {
		lpipsi= (LPIP_SUBCLASS_INFO) COMCTL32_Alloc (sizeof(IP_SUBCLASS_INFO));
		lpipsi->hwnd = hwnd;
		lpipsi->uRefCount++;
		SetPropA ((HWND)hwnd, IP_SUBCLASS_PROP,
					(HANDLE)lpipsi);
/*		infoPtr->lpipsi= lpipsi; */
	} else 
		WARN (ipaddress,"IP-create called twice\n");
	
	for (i=0; i<=3; i++) {
		infoPtr->LowerLimit[i]=0;
		infoPtr->UpperLimit[i]=255;
		edit.left=rcClient.left+i*fieldsize+3;
		edit.right=rcClient.left+(i+1)*fieldsize-2;
		lpipsi->hwndIP[i]= CreateWindowA ("edit", NULL, 
				WS_CHILD | WS_VISIBLE | ES_LEFT,
				edit.left, edit.top, edit.right-edit.left, edit.bottom-edit.top,
				hwnd, (HMENU) 1, GetWindowLongA (hwnd, GWL_HINSTANCE), NULL);
		lpipsi->wpOrigProc[i]= (WNDPROC)
					SetWindowLongA (lpipsi->hwndIP[i],GWL_WNDPROC, (LONG)
					IPADDRESS_SubclassProc);
		SetPropA ((HWND)lpipsi->hwndIP[i], IP_SUBCLASS_PROP,
					(HANDLE)lpipsi);
	}

	lpipsi->infoPtr= infoPtr;

    return 0;
}


static LRESULT
IPADDRESS_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	int i;
    IPADDRESS_INFO *infoPtr = IPADDRESS_GetInfoPtr (hwnd);
	LPIP_SUBCLASS_INFO lpipsi=(LPIP_SUBCLASS_INFO)
            GetPropA ((HWND)hwnd, IP_SUBCLASS_PROP);

	for (i=0; i<=3; i++) {
		SetWindowLongA ((HWND)lpipsi->hwndIP[i], GWL_WNDPROC,
                  (LONG)lpipsi->wpOrigProc[i]);
	}

    COMCTL32_Free (infoPtr);
    return 0;
}


static LRESULT
IPADDRESS_KillFocus (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;

	TRACE (ipaddress,"\n");
    hdc = GetDC (hwnd);
    IPADDRESS_Refresh (hwnd, hdc);
    ReleaseDC (hwnd, hdc);

    IPADDRESS_SendIPAddressNotify (hwnd, 0, 0);  /* FIXME: should use -1 */
    IPADDRESS_SendNotify (hwnd, EN_KILLFOCUS);       
    InvalidateRect (hwnd, NULL, TRUE);

    return 0;
}


static LRESULT
IPADDRESS_Paint (HWND hwnd, WPARAM wParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = wParam==0 ? BeginPaint (hwnd, &ps) : (HDC)wParam;
    IPADDRESS_Refresh (hwnd, hdc);
    if(!wParam)
	EndPaint (hwnd, &ps);
    return 0;
}


static LRESULT
IPADDRESS_SetFocus (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;

	TRACE (ipaddress,"\n");

    hdc = GetDC (hwnd);
    IPADDRESS_Refresh (hwnd, hdc);
    ReleaseDC (hwnd, hdc);

    return 0;
}


static LRESULT
IPADDRESS_Size (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    /* IPADDRESS_INFO *infoPtr = IPADDRESS_GetInfoPtr (hwnd); */
	TRACE (ipaddress,"\n");
    return 0;
}


static BOOL
IPADDRESS_SendNotify (HWND hwnd, UINT command)

{
    TRACE (ipaddress, "%x\n",command);
    return (BOOL)SendMessageA (GetParent (hwnd), WM_COMMAND,
              MAKEWPARAM (GetWindowLongA (hwnd, GWL_ID),command), (LPARAM)hwnd);
}


static BOOL
IPADDRESS_SendIPAddressNotify (HWND hwnd, UINT field, BYTE newValue)
{
	NMIPADDRESS nmip;

    TRACE (ipaddress, "%x %x\n",field,newValue);
    nmip.hdr.hwndFrom = hwnd;
    nmip.hdr.idFrom   = GetWindowLongA (hwnd, GWL_ID);
    nmip.hdr.code     = IPN_FIELDCHANGED;

	nmip.iField=field;
	nmip.iValue=newValue;

    return (BOOL)SendMessageA (GetParent (hwnd), WM_NOTIFY,
                               (WPARAM)nmip.hdr.idFrom, (LPARAM)&nmip);
}




static LRESULT
IPADDRESS_ClearAddress (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	int i;
	HDC hdc;
	char buf[1];
	LPIP_SUBCLASS_INFO lpipsi=(LPIP_SUBCLASS_INFO)
            GetPropA ((HWND)hwnd,IP_SUBCLASS_PROP);

	TRACE (ipaddress,"\n");

	buf[0]=0;
	for (i=0; i<=3; i++) 
		SetWindowTextA (lpipsi->hwndIP[i],buf);
	
    hdc = GetDC (hwnd);
    IPADDRESS_Refresh (hwnd, hdc);
    ReleaseDC (hwnd, hdc);
	
	return 0;
}

static LRESULT
IPADDRESS_IsBlank (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
 int i;
 char buf[20];
 LPIP_SUBCLASS_INFO lpipsi=(LPIP_SUBCLASS_INFO)
            GetPropA ((HWND)hwnd, IP_SUBCLASS_PROP);

 TRACE (ipaddress,"\n");

 for (i=0; i<=3; i++) {
		GetWindowTextA (lpipsi->hwndIP[i],buf,5);
	if (buf[0])
	    return 0;
	}

 return 1;
}

static LRESULT
IPADDRESS_GetAddress (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
 char field[20];
 int i,valid,fieldvalue;
 DWORD ip_addr;
 IPADDRESS_INFO *infoPtr = IPADDRESS_GetInfoPtr (hwnd);
 LPIP_SUBCLASS_INFO lpipsi=(LPIP_SUBCLASS_INFO)
            GetPropA ((HWND)hwnd, IP_SUBCLASS_PROP);

 TRACE (ipaddress,"\n");

 valid=0;
 ip_addr=0;
 for (i=0; i<=3; i++) {
		GetWindowTextA (lpipsi->hwndIP[i],field,4);
		ip_addr*=256;
		if (field[0]) {
			field[3]=0;
			fieldvalue=atoi(field);
			if (fieldvalue<infoPtr->LowerLimit[i]) 
				fieldvalue=infoPtr->LowerLimit[i];
			if (fieldvalue>infoPtr->UpperLimit[i]) 
				fieldvalue=infoPtr->UpperLimit[i];
			ip_addr+=fieldvalue;
			valid++;
		}
 }

 *(LPDWORD) lParam=ip_addr;

 return valid;
}


static LRESULT
IPADDRESS_SetRange (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    IPADDRESS_INFO *infoPtr = IPADDRESS_GetInfoPtr (hwnd);
	INT index;
	
 	TRACE (ipaddress,"\n");

	index=(INT) wParam;
	if ((index<0) || (index>3)) return 0;

	infoPtr->LowerLimit[index]=lParam & 0xff;
	infoPtr->UpperLimit[index]=(lParam >>8)  & 0xff;
	return 1;
}

static LRESULT
IPADDRESS_SetAddress (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    IPADDRESS_INFO *infoPtr = IPADDRESS_GetInfoPtr (hwnd);
	HDC hdc;
	LPIP_SUBCLASS_INFO lpipsi=(LPIP_SUBCLASS_INFO)
            GetPropA ((HWND)hwnd, IP_SUBCLASS_PROP);
	int i,ip_address,value;
    char buf[20];

 	TRACE (ipaddress,"\n");
	ip_address=(DWORD) lParam;

	for (i=3; i>=0; i--) {
		value=ip_address & 0xff;
		if ((value>=infoPtr->LowerLimit[i]) && (value<=infoPtr->UpperLimit[i])) 
			{
			 sprintf (buf,"%d",value);
			 SetWindowTextA (lpipsi->hwndIP[i],buf);
			 IPADDRESS_SendNotify (hwnd, EN_CHANGE);
		}
		ip_address/=256;
	}

    hdc = GetDC (hwnd);		/* & send notifications */
    IPADDRESS_Refresh (hwnd, hdc);
    ReleaseDC (hwnd, hdc);

 return TRUE;
}




static LRESULT
IPADDRESS_SetFocusToField (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    /* IPADDRESS_INFO *infoPtr = IPADDRESS_GetInfoPtr (hwnd); */
	LPIP_SUBCLASS_INFO lpipsi=(LPIP_SUBCLASS_INFO)
            GetPropA ((HWND)hwnd, IP_SUBCLASS_PROP);
	INT index;

	index=(INT) wParam;
 	TRACE (ipaddress," %d\n", index);
	if ((index<0) || (index>3)) return 0;
	
	SetFocus (lpipsi->hwndIP[index]);
	
    return 1;
}


static LRESULT
IPADDRESS_LButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACE (ipaddress, "\n");

	SetFocus (hwnd);
	IPADDRESS_SendNotify (hwnd, EN_SETFOCUS);
	IPADDRESS_SetFocusToField (hwnd, 0, 0);

 	return TRUE;
}



/* tab/shift-tab: IPN_FIELDCHANGED, lose focus.
   dot, space,right arrow:	set focus to next child edit.
   numerics (0..9), control characters: forward to default edit control 
   other characters: dropped
*/
   



static int
IPADDRESS_GotoNextField (LPIP_SUBCLASS_INFO lpipsi, int currentfield)
{
 int newField,fieldvalue;
 char field[20];
 IPADDRESS_INFO *infoPtr=lpipsi->infoPtr;

 TRACE (ipaddress,"\n");
 GetWindowTextA (lpipsi->hwndIP[currentfield],field,4);
 if (field[0]) {
	field[3]=0;	
	newField=-1;
	fieldvalue=atoi(field);
	if (fieldvalue<infoPtr->LowerLimit[currentfield]) 
		newField=infoPtr->LowerLimit[currentfield];
	if (fieldvalue>infoPtr->UpperLimit[currentfield])
		newField=infoPtr->UpperLimit[currentfield];
	if (newField>=0) {
		sprintf (field,"%d",newField);
		SetWindowTextA (lpipsi->hwndIP[currentfield], field);
		return 1;
	}
 }

 if (currentfield<3) 
		SetFocus (lpipsi->hwndIP[currentfield+1]);
 return 0;
}


LRESULT CALLBACK
IPADDRESS_SubclassProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 int i,l,index;
 IPADDRESS_INFO *infoPtr;
 LPIP_SUBCLASS_INFO lpipsi=(LPIP_SUBCLASS_INFO)
            GetPropA ((HWND)hwnd,IP_SUBCLASS_PROP); 

 infoPtr = lpipsi->infoPtr;
 index=0;             /* FIXME */
 for (i=0; i<=3; i++) 
		if (lpipsi->hwndIP[i]==hwnd) index=i;

 switch (uMsg) {
	case WM_CHAR: break;
	case WM_KEYDOWN: {
			char c=(char) wParam;
			if (c==VK_TAB) {
 				HWND pwnd;
				int shift;
				shift = GetKeyState(VK_SHIFT) & 0x8000;
				if (shift)
					pwnd=GetNextDlgTabItem (GetParent (hwnd), 0, TRUE);
				else
					pwnd=GetNextDlgTabItem (GetParent (hwnd), 0, FALSE);
				if (pwnd) SetFocus (pwnd);
				break;
			}
			
			if ((c==' ') || (c=='.') || (c==VK_RIGHT)) {
				IPADDRESS_GotoNextField (lpipsi,index);
				wParam=0;
				lParam=0;
				break;
			}
			if (c==VK_LEFT) {
				
			}
			if (c==VK_RETURN) {
			}
			if (((c>='0') && (c<='9')) || (iscntrl(c))) {
				l=GetWindowTextLengthA (lpipsi->hwndIP[index]);
				if (l==3) 
					if (IPADDRESS_GotoNextField (lpipsi,index)) {
						wParam=0;
						lParam=0;
					}
				break;
			}
	
			wParam=0;
			lParam=0;
			break;
		}
 }

 return CallWindowProcA (lpipsi->wpOrigProc[index], hwnd, uMsg, wParam, lParam);
}

LRESULT WINAPI
IPADDRESS_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
	case IPM_CLEARADDRESS:
		return IPADDRESS_ClearAddress (hwnd, wParam, lParam);

	case IPM_SETADDRESS:
	    return IPADDRESS_SetAddress (hwnd, wParam, lParam);

	case IPM_GETADDRESS:
	    return IPADDRESS_GetAddress (hwnd, wParam, lParam);

	case IPM_SETRANGE:
	    return IPADDRESS_SetRange (hwnd, wParam, lParam);

	case IPM_SETFOCUS:
	    return IPADDRESS_SetFocusToField (hwnd, wParam, lParam);

	case IPM_ISBLANK:
		return IPADDRESS_IsBlank (hwnd, wParam, lParam);

	case WM_CREATE:
	    return IPADDRESS_Create (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return IPADDRESS_Destroy (hwnd, wParam, lParam);

	case WM_GETDLGCODE:
	    return DLGC_WANTARROWS | DLGC_WANTCHARS;

	case WM_KILLFOCUS:
	    return IPADDRESS_KillFocus (hwnd, wParam, lParam);

	case WM_LBUTTONDOWN:
        return IPADDRESS_LButtonDown (hwnd, wParam, lParam);

	case WM_PAINT:
	    return IPADDRESS_Paint (hwnd, wParam);

	case WM_SETFOCUS:
	    return IPADDRESS_SetFocus (hwnd, wParam, lParam);

	case WM_SIZE:
	    return IPADDRESS_Size (hwnd, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR (ipaddress, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void
IPADDRESS_Register (void)
{
    WNDCLASSA wndClass;

    if (GlobalFindAtomA (WC_IPADDRESSA)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC)IPADDRESS_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(IPADDRESS_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wndClass.lpszClassName = WC_IPADDRESSA;
 
    RegisterClassA (&wndClass);
}

VOID
IPADDRESS_Unregister (VOID)
{
    if (GlobalFindAtomA (WC_IPADDRESSA))
    UnregisterClassA (WC_IPADDRESSA, (HINSTANCE)NULL);
}

