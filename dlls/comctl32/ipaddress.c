/* IP Address control
 *
 * Copyright 1998 Eric Kohl
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

#include "windows.h"
#include "win.h"
#include "commctrl.h"
#include "ipaddress.h"
#include "heap.h"
#include "debug.h"


#define IPADDRESS_GetInfoPtr(wndPtr) ((IPADDRESS_INFO *)wndPtr->wExtra[0])


static BOOL32 
IPADDRESS_SendNotify (WND *wndPtr, UINT32 command);
static BOOL32 
IPADDRESS_SendIPAddressNotify (WND *wndPtr, UINT32 field, BYTE newValue);


/* property name of tooltip window handle */
#define IP_SUBCLASS_PROP "CCIP32SubclassInfo"


static LRESULT CALLBACK
IPADDRESS_SubclassProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam);




static VOID
IPADDRESS_Refresh (WND *wndPtr, HDC32 hdc)
{
	RECT32 rcClient;
	HBRUSH32 hbr;
	COLORREF clr=GetSysColor32 (COLOR_3DDKSHADOW);
    int i,x,fieldsize;

    GetClientRect32 (wndPtr->hwndSelf, &rcClient);
	hbr =  CreateSolidBrush32 (RGB(255,255,255));
    DrawEdge32 (hdc, &rcClient, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
	FillRect32 (hdc, &rcClient, hbr);
    DeleteObject32 (hbr);

	x=rcClient.left;
	fieldsize=(rcClient.right-rcClient.left) /4;

	for (i=0; i<3; i++) {		/* Should draw text "." here */
		x+=fieldsize;
		SetPixel32 (hdc, x,   13, clr);
		SetPixel32 (hdc, x,   14, clr);
		SetPixel32 (hdc, x+1, 13, clr);
		SetPixel32 (hdc, x+1, 14, clr);

	}

}





static LRESULT
IPADDRESS_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    IPADDRESS_INFO *infoPtr;
	RECT32 rcClient, edit;
	int i,fieldsize;
	LPIP_SUBCLASS_INFO lpipsi;
	

    infoPtr = (IPADDRESS_INFO *)COMCTL32_Alloc (sizeof(IPADDRESS_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;

	if (infoPtr == NULL) {
    	ERR (ipaddress, "could not allocate info memory!\n");
    	return 0;
    }

    GetClientRect32 (wndPtr->hwndSelf, &rcClient);

	fieldsize=(rcClient.right-rcClient.left) /4;

	edit.top   =rcClient.top+2;
	edit.bottom=rcClient.bottom-2;

	lpipsi=(LPIP_SUBCLASS_INFO)
			GetProp32A ((HWND32)wndPtr->hwndSelf,IP_SUBCLASS_PROP);
	if (lpipsi == NULL)  {
		lpipsi= (LPIP_SUBCLASS_INFO) COMCTL32_Alloc (sizeof(IP_SUBCLASS_INFO));
		lpipsi->wndPtr=wndPtr;
		lpipsi->uRefCount++;
		SetProp32A ((HWND32)wndPtr->hwndSelf, IP_SUBCLASS_PROP,
					(HANDLE32)lpipsi);
/*		infoPtr->lpipsi= lpipsi; */
	} else 
		WARN (ipaddress,"IP-create called twice\n");
	
	for (i=0; i<=3; i++) {
		infoPtr->LowerLimit[i]=0;
		infoPtr->UpperLimit[i]=255;
		edit.left=rcClient.left+i*fieldsize+3;
		edit.right=rcClient.left+(i+1)*fieldsize-2;
		lpipsi->hwndIP[i]= CreateWindow32A ("edit", NULL, 
				WS_CHILD | WS_VISIBLE | ES_LEFT,
				edit.left, edit.top, edit.right-edit.left, edit.bottom-edit.top,
				wndPtr->hwndSelf, (HMENU32) 1, wndPtr->hInstance, NULL);
		lpipsi->wpOrigProc[i]= (WNDPROC32)
					SetWindowLong32A (lpipsi->hwndIP[i],GWL_WNDPROC, (LONG)
					IPADDRESS_SubclassProc);
		SetProp32A ((HWND32)lpipsi->hwndIP[i], IP_SUBCLASS_PROP,
					(HANDLE32)lpipsi);
	}

	lpipsi->infoPtr= infoPtr;

    return 0;
}


static LRESULT
IPADDRESS_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	int i;
    IPADDRESS_INFO *infoPtr = IPADDRESS_GetInfoPtr(wndPtr);
	LPIP_SUBCLASS_INFO lpipsi=(LPIP_SUBCLASS_INFO)
            GetProp32A ((HWND32)wndPtr->hwndSelf,IP_SUBCLASS_PROP);

	for (i=0; i<=3; i++) {
		SetWindowLong32A ((HWND32)lpipsi->hwndIP[i], GWL_WNDPROC,
                  (LONG)lpipsi->wpOrigProc[i]);
	}

    COMCTL32_Free (infoPtr);
    return 0;
}


static LRESULT
IPADDRESS_KillFocus (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HDC32 hdc;

	TRACE (ipaddress,"\n");
    hdc = GetDC32 (wndPtr->hwndSelf);
    IPADDRESS_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

	IPADDRESS_SendIPAddressNotify (wndPtr, 0, 0);  /* FIXME: should use -1 */
	IPADDRESS_SendNotify (wndPtr, EN_KILLFOCUS);       
    InvalidateRect32 (wndPtr->hwndSelf, NULL, TRUE);

    return 0;
}


static LRESULT
IPADDRESS_Paint (WND *wndPtr, WPARAM32 wParam)
{
    HDC32 hdc;
    PAINTSTRUCT32 ps;

    hdc = wParam==0 ? BeginPaint32 (wndPtr->hwndSelf, &ps) : (HDC32)wParam;
    IPADDRESS_Refresh (wndPtr, hdc);
    if(!wParam)
	EndPaint32 (wndPtr->hwndSelf, &ps);
    return 0;
}


static LRESULT
IPADDRESS_SetFocus (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HDC32 hdc;

	TRACE (ipaddress,"\n");

    hdc = GetDC32 (wndPtr->hwndSelf);
    IPADDRESS_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return 0;
}


static LRESULT
IPADDRESS_Size (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    IPADDRESS_INFO *infoPtr = IPADDRESS_GetInfoPtr(wndPtr);
	TRACE (ipaddress,"\n");
    return 0;
}


static BOOL32
IPADDRESS_SendNotify (WND *wndPtr, UINT32 command)

{
    TRACE (ipaddress, "%x\n",command);
    return (BOOL32)SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_COMMAND,
              MAKEWPARAM (wndPtr->wIDmenu,command), (LPARAM) wndPtr->hwndSelf);
}


static BOOL32
IPADDRESS_SendIPAddressNotify (WND *wndPtr, UINT32 field, BYTE newValue)

{
	NMIPADDRESS nmip;

    TRACE (ipaddress, "%x %x\n",field,newValue);
    nmip.hdr.hwndFrom = wndPtr->hwndSelf;
    nmip.hdr.idFrom   = wndPtr->wIDmenu;
    nmip.hdr.code     = IPN_FIELDCHANGED;

	nmip.iField=field;
	nmip.iValue=newValue;

    return (BOOL32)SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
                                   (WPARAM32)wndPtr->wIDmenu, (LPARAM)&nmip);
}




static LRESULT
IPADDRESS_ClearAddress (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	int i;
	HDC32 hdc;
	char buf[1];
	LPIP_SUBCLASS_INFO lpipsi=(LPIP_SUBCLASS_INFO)
            GetProp32A ((HWND32)wndPtr->hwndSelf,IP_SUBCLASS_PROP);

	TRACE (ipaddress,"\n");

	buf[0]=0;
	for (i=0; i<=3; i++) 
		SetWindowText32A (lpipsi->hwndIP[i],buf);
	
  	hdc = GetDC32 (wndPtr->hwndSelf);
    IPADDRESS_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);
	return 0;
}

static LRESULT
IPADDRESS_IsBlank (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
 int i;
 char buf[20];
 LPIP_SUBCLASS_INFO lpipsi=(LPIP_SUBCLASS_INFO)
            GetProp32A ((HWND32)wndPtr->hwndSelf,IP_SUBCLASS_PROP);

 TRACE (ipaddress,"\n");

 for (i=0; i<=3; i++) {
		GetWindowText32A (lpipsi->hwndIP[i],buf,5);
		if (buf[0]) return 0;
	}

 return 1;
}

static LRESULT
IPADDRESS_GetAddress (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
 char field[20];
 int i,valid,fieldvalue;
 DWORD ip_addr;
 IPADDRESS_INFO *infoPtr = IPADDRESS_GetInfoPtr(wndPtr);
 LPIP_SUBCLASS_INFO lpipsi=(LPIP_SUBCLASS_INFO)
            GetProp32A ((HWND32)wndPtr->hwndSelf,IP_SUBCLASS_PROP);

 TRACE (ipaddress,"\n");

 valid=0;
 ip_addr=0;
 for (i=0; i<=3; i++) {
		GetWindowText32A (lpipsi->hwndIP[i],field,4);
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
IPADDRESS_SetRange (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)

{
    IPADDRESS_INFO *infoPtr = IPADDRESS_GetInfoPtr(wndPtr);
	INT32 index;
	
 	TRACE (ipaddress,"\n");

	index=(INT32) wParam;
	if ((index<0) || (index>3)) return 0;

	infoPtr->LowerLimit[index]=lParam & 0xff;
	infoPtr->UpperLimit[index]=(lParam >>8)  & 0xff;
	return 1;
}

static LRESULT
IPADDRESS_SetAddress (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	HDC32 hdc;
    IPADDRESS_INFO *infoPtr = IPADDRESS_GetInfoPtr(wndPtr);
	LPIP_SUBCLASS_INFO lpipsi=(LPIP_SUBCLASS_INFO)
            GetProp32A ((HWND32)wndPtr->hwndSelf,IP_SUBCLASS_PROP);
	int i,ip_address,value;
    char buf[20];

 	TRACE (ipaddress,"\n");
	ip_address=(DWORD) lParam;

	for (i=3; i>=0; i--) {
		value=ip_address & 0xff;
		if ((value>=infoPtr->LowerLimit[i]) && (value<=infoPtr->UpperLimit[i])) 
			{
			 sprintf (buf,"%d",value);
			 SetWindowText32A (lpipsi->hwndIP[i],buf);
			 IPADDRESS_SendNotify (wndPtr, EN_CHANGE);
		}
		ip_address/=256;
	}

	hdc = GetDC32 (wndPtr->hwndSelf);		/* & send notifications */
    IPADDRESS_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

 return TRUE;
}




static LRESULT
IPADDRESS_SetFocusToField (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    IPADDRESS_INFO *infoPtr = IPADDRESS_GetInfoPtr(wndPtr);
	LPIP_SUBCLASS_INFO lpipsi=(LPIP_SUBCLASS_INFO)
            GetProp32A ((HWND32)wndPtr->hwndSelf,IP_SUBCLASS_PROP);
	INT32 index;

	index=(INT32) wParam;
 	TRACE (ipaddress," %d\n", index);
	if ((index<0) || (index>3)) return 0;
	
	SetFocus32 (lpipsi->hwndIP[index]);
	
    return 1;
}


static LRESULT
IPADDRESS_LButtonDown (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACE (ipaddress, "\n");

	SetFocus32 (wndPtr->hwndSelf);
	IPADDRESS_SendNotify (wndPtr, EN_SETFOCUS);
	IPADDRESS_SetFocusToField (wndPtr, 0, 0);

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
 GetWindowText32A (lpipsi->hwndIP[currentfield],field,4);
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
		SetWindowText32A (lpipsi->hwndIP[currentfield], field);
		return 1;
	}
 }

 if (currentfield<3) 
		SetFocus32 (lpipsi->hwndIP[currentfield+1]);
 return 0;
}


LRESULT CALLBACK
IPADDRESS_SubclassProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
 int i,l,index;
 IPADDRESS_INFO *infoPtr;
 LPIP_SUBCLASS_INFO lpipsi=(LPIP_SUBCLASS_INFO)
            GetProp32A ((HWND32)hwnd,IP_SUBCLASS_PROP); 

 infoPtr = lpipsi->infoPtr;
 index=0;             /* FIXME */
 for (i=0; i<=3; i++) 
		if (lpipsi->hwndIP[i]==hwnd) index=i;

 switch (uMsg) {
	case WM_CHAR: break;
	case WM_KEYDOWN: {
			char c=(char) wParam;
			if (c==VK_TAB) {
 				HWND32 pwnd;
				int shift;
				shift = GetKeyState32(VK_SHIFT) & 0x8000;
				if (shift)
					pwnd=GetNextDlgTabItem32 (GetParent32 (hwnd), 0, TRUE);
				else
					pwnd=GetNextDlgTabItem32 (GetParent32 (hwnd), 0, FALSE);
				if (pwnd) SetFocus32 (pwnd);
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
				l=GetWindowTextLength32A (lpipsi->hwndIP[index]);
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

 return CallWindowProc32A (lpipsi->wpOrigProc[index], hwnd, uMsg, wParam, lParam);
}

LRESULT WINAPI
IPADDRESS_WindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
	case IPM_CLEARADDRESS:
		return IPADDRESS_ClearAddress (wndPtr, wParam, lParam);

	case IPM_SETADDRESS:
	    return IPADDRESS_SetAddress (wndPtr, wParam, lParam);

	case IPM_GETADDRESS:
	    return IPADDRESS_GetAddress (wndPtr, wParam, lParam);

	case IPM_SETRANGE:
	    return IPADDRESS_SetRange (wndPtr, wParam, lParam);

	case IPM_SETFOCUS:
	    return IPADDRESS_SetFocusToField (wndPtr, wParam, lParam);

	case IPM_ISBLANK:
		return IPADDRESS_IsBlank (wndPtr, wParam, lParam);

	case WM_CREATE:
	    return IPADDRESS_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return IPADDRESS_Destroy (wndPtr, wParam, lParam);

	case WM_GETDLGCODE:
	    return DLGC_WANTARROWS | DLGC_WANTCHARS;

	case WM_KILLFOCUS:
	    return IPADDRESS_KillFocus (wndPtr, wParam, lParam);

	case WM_LBUTTONDOWN:
        return IPADDRESS_LButtonDown (wndPtr, wParam, lParam);

	case WM_PAINT:
	    return IPADDRESS_Paint (wndPtr, wParam);

	case WM_SETFOCUS:
	    return IPADDRESS_SetFocus (wndPtr, wParam, lParam);

	case WM_SIZE:
	    return IPADDRESS_Size (wndPtr, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR (ipaddress, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void
IPADDRESS_Register (void)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (WC_IPADDRESS32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC32)IPADDRESS_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(IPADDRESS_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = (HBRUSH32)(COLOR_3DFACE + 1);
    wndClass.lpszClassName = WC_IPADDRESS32A;
 
    RegisterClass32A (&wndClass);
}

VOID
IPADDRESS_Unregister (VOID)
{
    if (GlobalFindAtom32A (WC_IPADDRESS32A))
    UnregisterClass32A (WC_IPADDRESS32A, (HINSTANCE32)NULL);
}

