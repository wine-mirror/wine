/*
 * IP Address control
 *
 * Copyright 1999 Chris Morgan<cmorgan@wpi.edu> and
 *                James Abbatiello<abbeyj@wpi.edu>
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 1998 Alex Priem <alexp@sci.kun.nl>
 *
 * NOTES
 *
 * TODO:
 *    -Edit control doesn't support the ES_CENTER style which prevents
 *     this ipaddress control from having centered text look like the
 *     windows ipaddress control
 *    -Check all notifications/behavior.
 *    -Optimization: 
 *        -include lpipsi in IPADDRESS_INFO.
 *	  -CurrentFocus: field that has focus at moment of processing.
 *	  -connect Rect32 rcClient.
 *	  -check ipaddress.cpp for missing features.
 *    -refresh: draw '.' instead of setpixel.
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "winbase.h"
#include "commctrl.h"
#include "ipaddress.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ipaddress);

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
  hbr = CreateSolidBrush (RGB(255,255,255));
  DrawEdge (hdc, &rcClient, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
  FillRect (hdc, &rcClient, hbr);
  DeleteObject (hbr);

  x=rcClient.left;
  fieldsize=(rcClient.right-rcClient.left) / 4;

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

  lpipsi=(LPIP_SUBCLASS_INFO) GetPropA ((HWND)hwnd, IP_SUBCLASS_PROP);
  if (lpipsi == NULL)  {
    lpipsi = (LPIP_SUBCLASS_INFO) COMCTL32_Alloc (sizeof(IP_SUBCLASS_INFO));
    lpipsi->hwnd = hwnd;
    lpipsi->uRefCount++;
    SetPropA ((HWND)hwnd, IP_SUBCLASS_PROP, (HANDLE)lpipsi);
/*		infoPtr->lpipsi= lpipsi; */
  } else 
    WARN("IP-create called twice\n");
	
  for (i=0; i<=3; i++)
  {
    infoPtr->LowerLimit[i]=0;
    infoPtr->UpperLimit[i]=255;
    edit.left=rcClient.left+i*fieldsize+6;
    edit.right=rcClient.left+(i+1)*fieldsize-2;
    lpipsi->hwndIP[i]= CreateWindowA ("edit", NULL, 
        WS_CHILD | WS_VISIBLE | ES_CENTER,
        edit.left, edit.top, edit.right-edit.left, edit.bottom-edit.top,
        hwnd, (HMENU) 1, GetWindowLongA (hwnd, GWL_HINSTANCE), NULL);
    lpipsi->wpOrigProc[i]= (WNDPROC)
        SetWindowLongA (lpipsi->hwndIP[i],GWL_WNDPROC, (LONG)
        IPADDRESS_SubclassProc);
    SetPropA ((HWND)lpipsi->hwndIP[i], IP_SUBCLASS_PROP, (HANDLE)lpipsi);
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

  TRACE("\n");
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

  TRACE("\n");

  hdc = GetDC (hwnd);
  IPADDRESS_Refresh (hwnd, hdc);
  ReleaseDC (hwnd, hdc);

  return 0;
}


static LRESULT
IPADDRESS_Size (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  /* IPADDRESS_INFO *infoPtr = IPADDRESS_GetInfoPtr (hwnd); */
  TRACE("\n");
  return 0;
}


static BOOL
IPADDRESS_SendNotify (HWND hwnd, UINT command)
{
  TRACE("%x\n",command);
  return (BOOL)SendMessageA (GetParent (hwnd), WM_COMMAND,
              MAKEWPARAM (GetWindowLongA (hwnd, GWL_ID),command), (LPARAM)hwnd);
}


static BOOL
IPADDRESS_SendIPAddressNotify (HWND hwnd, UINT field, BYTE newValue)
{
  NMIPADDRESS nmip;

  TRACE("%x %x\n",field,newValue);
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

  TRACE("\n");

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

  TRACE("\n");

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

  TRACE("\n");

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
	
  TRACE("\n");

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

  TRACE("\n");
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
  LPIP_SUBCLASS_INFO lpipsi=(LPIP_SUBCLASS_INFO) GetPropA ((HWND)hwnd,
                                                   IP_SUBCLASS_PROP);
  INT index;

  index=(INT) wParam;
  TRACE(" %d\n", index);
  if ((index<0) || (index>3)) return 0;
	
  SetFocus (lpipsi->hwndIP[index]);
	
  return 1;
}


static LRESULT
IPADDRESS_LButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TRACE("\n");

  SetFocus (hwnd);
  IPADDRESS_SendNotify (hwnd, EN_SETFOCUS);
  IPADDRESS_SetFocusToField (hwnd, 0, 0);

  return TRUE;
}


static INT
IPADDRESS_CheckField (LPIP_SUBCLASS_INFO lpipsi, int currentfield)
{
  int newField,fieldvalue;
  char field[20];
  IPADDRESS_INFO *infoPtr=lpipsi->infoPtr;

  if(currentfield >= 0 && currentfield < 4)
  {
    TRACE("\n");
    GetWindowTextA (lpipsi->hwndIP[currentfield],field,4);
    if (field[0])
    {
      field[3]=0;	
      newField=-1;
      fieldvalue=atoi(field);
    
      if (fieldvalue < infoPtr->LowerLimit[currentfield]) 
        newField=infoPtr->LowerLimit[currentfield];
    
      if (fieldvalue > infoPtr->UpperLimit[currentfield])
        newField=infoPtr->UpperLimit[currentfield];
    
      if (newField >= 0)
      {
        sprintf (field,"%d",newField);
        SetWindowTextA (lpipsi->hwndIP[currentfield], field);
        return TRUE;
      }
    }
  }
  return FALSE;
}


static INT
IPADDRESS_GotoNextField (LPIP_SUBCLASS_INFO lpipsi, int currentfield)
{
  if(currentfield >= -1 && currentfield < 4)
  {
    IPADDRESS_CheckField(lpipsi, currentfield); /* check the fields value */  

    if(currentfield < 3)
    { 
      SetFocus (lpipsi->hwndIP[currentfield+1]);
      return TRUE; 
    }
  }
  return FALSE;
}


/* period: move and select the text in the next field to the right if */
/*         the current field is not empty(l!=0), we are not in the */
/*         left most position, and nothing is selected(startsel==endsel) */

/* spacebar: same behavior as period */

/* alpha characters: completely ignored */

/* digits: accepted when field text length < 2 ignored otherwise. */
/*         when 3 numbers have been entered into the field the value */
/*         of the field is checked, if the field value exceeds the */
/*         maximum value and is changed the field remains the current */
/*         field, otherwise focus moves to the field to the right */

/* tab: change focus from the current ipaddress control to the next */
/*      control in the tab order */

/* right arrow: move to the field on the right to the left most */
/*              position in that field if no text is selected, */
/*              we are in the right most position in the field, */
/*              we are not in the right most field */

/* left arrow: move to the field on the left to the right most */
/*             position in that field if no text is selected, */
/*             we are in the left most position in the current field */
/*             and we are not in the left most field */

/* backspace: delete the character to the left of the cursor position, */
/*            if none are present move to the field on the left if */
/*            we are not in the left most field and delete the right */
/*            most digit in that field while keeping the cursor */
/*            on the right side of the field */


LRESULT CALLBACK
IPADDRESS_SubclassProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  IPADDRESS_INFO *infoPtr;
  LPIP_SUBCLASS_INFO lpipsi = (LPIP_SUBCLASS_INFO) GetPropA
	((HWND)hwnd,IP_SUBCLASS_PROP); 
  CHAR c = (CHAR)wParam;
  INT i, l, index, startsel, endsel;

  infoPtr = lpipsi->infoPtr;
  index=0;             /* FIXME */
  for (i=0; i<=3; i++) 
    if (lpipsi->hwndIP[i]==hwnd) index=i;

  switch (uMsg) {
  case WM_CHAR: 
    if(isdigit(c)) /* process all digits */
    {
      int return_val;
      SendMessageA(lpipsi->hwndIP[index], EM_GETSEL, (WPARAM)&startsel, (LPARAM)&endsel); 
      l = GetWindowTextLengthA (lpipsi->hwndIP[index]);
      if(l==2 && startsel==endsel) /* field is full after this key is processed */
      { 
        /* process the digit press before we check the field */
        return_val = CallWindowProcA (lpipsi->wpOrigProc[index], hwnd, uMsg, wParam, lParam);
          
        /* if the field value was changed stay at the current field */
        if(!IPADDRESS_CheckField(lpipsi, index))
          IPADDRESS_GotoNextField (lpipsi,index);

        return return_val;
      }
      
      if(l > 2) /* field is full, stop key messages */
      {
        lParam = 0;
        wParam = 0;
      }
      break;
    }	
    
    if(c == '.') /* VK_PERIOD */
    { 
      l = GetWindowTextLengthA(lpipsi->hwndIP[index]);
      SendMessageA(lpipsi->hwndIP[index], EM_GETSEL, (WPARAM)&startsel, (LPARAM)&endsel);
      if(l != 0 && startsel==endsel && startsel != 0)
      {
        IPADDRESS_GotoNextField(lpipsi, index); 
        SendMessageA(lpipsi->hwndIP[index+1], EM_SETSEL, (WPARAM)0, (LPARAM)3);
      }
    }
        
    /* stop all other characters */
    wParam = 0;
    lParam = 0;
    break;

  case WM_KEYDOWN:
    if(c == VK_SPACE)
    { 
      l = GetWindowTextLengthA(lpipsi->hwndIP[index]);
      SendMessageA(lpipsi->hwndIP[index], EM_GETSEL, (WPARAM)&startsel, (LPARAM)&endsel);
      if(l != 0 && startsel==endsel && startsel != 0)
      {
        IPADDRESS_GotoNextField(lpipsi, index); 
        SendMessageA(lpipsi->hwndIP[index+1], EM_SETSEL, (WPARAM)0, (LPARAM)3);
      }
    }

    if(c == VK_RIGHT)
    {
      SendMessageA(lpipsi->hwndIP[index], EM_GETSEL, (WPARAM)&startsel, (LPARAM)&endsel);
      l = GetWindowTextLengthA (lpipsi->hwndIP[index]);
    
      if(startsel==endsel && startsel==l)
      {
        IPADDRESS_GotoNextField(lpipsi, index);
        SendMessageA(lpipsi->hwndIP[index+1], EM_SETSEL, (WPARAM)0,(LPARAM)0);
      }
    }

    if(c == VK_LEFT)
    { 
      SendMessageA(lpipsi->hwndIP[index], EM_GETSEL, (WPARAM)&startsel, (LPARAM)&endsel);
      if(startsel==0 && startsel==endsel && index > 0)
      {        
        IPADDRESS_GotoNextField(lpipsi, index - 2);
        l = GetWindowTextLengthA(lpipsi->hwndIP[index-1]);
        SendMessageA(lpipsi->hwndIP[index-1], EM_SETSEL, (WPARAM)l,(LPARAM)l);
      }     
    }
   
    if(c == VK_BACK) /* VK_BACKSPACE */
    {
      CHAR buf[4];

      SendMessageA(lpipsi->hwndIP[index], EM_GETSEL, (WPARAM)&startsel, (LPARAM)&endsel); 
      l = GetWindowTextLengthA (lpipsi->hwndIP[index]);
      if(startsel==endsel && startsel==0 && index > 0)
      {
        l = GetWindowTextLengthA(lpipsi->hwndIP[index-1]);
        if(l!=0)
        {
          GetWindowTextA (lpipsi->hwndIP[index-1],buf,4);
          buf[l-1] = '\0'; 
          SetWindowTextA(lpipsi->hwndIP[index-1], buf);
          SendMessageA(lpipsi->hwndIP[index-1], EM_SETSEL, (WPARAM)l,(LPARAM)l);
        } 
        IPADDRESS_GotoNextField(lpipsi, index - 2);
      }
    }
    break;

  default:
    break;
  }
  return CallWindowProcA (lpipsi->wpOrigProc[index], hwnd, uMsg, wParam, lParam);
}


static LRESULT WINAPI
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
        ERR("unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
      return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void IPADDRESS_Register (void)
{
  WNDCLASSA wndClass;

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


VOID IPADDRESS_Unregister (void)
{
  UnregisterClassA (WC_IPADDRESSA, (HINSTANCE)NULL);
}
