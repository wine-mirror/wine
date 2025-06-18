/*
 * COMMDLG - Color Dialog
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 * Copyright 2019 Vijay Kiran Kamuju
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* BUGS : still seems to not refresh correctly
   sometimes, especially when 2 instances of the
   dialog are loaded at the same time */

#include <stdarg.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "colordlg.h"
#include "commdlg.h"
#include "dlgs.h"
#include "cderr.h"
#include "cdlg.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

static INT_PTR CALLBACK ColorDlgProc( HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam );

#define CONV_LPARAMTOPOINT(lp,p) do { (p)->x = (short)LOWORD(lp); (p)->y = (short)HIWORD(lp); } while(0)

static const COLORREF predefcolors[6][8]=
{
 { 0x008080FFL, 0x0080FFFFL, 0x0080FF80L, 0x0080FF00L,
   0x00FFFF80L, 0x00FF8000L, 0x00C080FFL, 0x00FF80FFL },
 { 0x000000FFL, 0x0000FFFFL, 0x0000FF80L, 0x0040FF00L,
   0x00FFFF00L, 0x00C08000L, 0x00C08080L, 0x00FF00FFL },

 { 0x00404080L, 0x004080FFL, 0x0000FF00L, 0x00808000L,
   0x00804000L, 0x00FF8080L, 0x00400080L, 0x008000FFL },
 { 0x00000080L, 0x000080FFL, 0x00008000L, 0x00408000L,
   0x00FF0000L, 0x00A00000L, 0x00800080L, 0x00FF0080L },

 { 0x00000040L, 0x00004080L, 0x00004000L, 0x00404000L,
   0x00800000L, 0x00400000L, 0x00400040L, 0x00800040L },
 { 0x00000000L, 0x00008080L, 0x00408080L, 0x00808080L,
   0x00808040L, 0x00C0C0C0L, 0x00400040L, 0x00FFFFFFL },
};

/* Chose Color PRIVATE Structure:
 *
 * This structure is duplicated in the 16 bit code with
 * an extra member
 */

typedef struct CCPRIVATE
{
    LPCHOOSECOLORW lpcc; /* points to public known data structure */
    HWND hwndSelf;       /* dialog window */
    int nextuserdef;     /* next free place in user defined color array */
    HDC hdcMem;          /* color graph used for BitBlt() */
    HBITMAP hbmMem;      /* color graph bitmap */
    RECT fullsize;       /* original dialog window size */
    UINT msetrgb;        /* # of SETRGBSTRING message (today not used)  */
    RECT old3angle;      /* last position of l-marker */
    BOOL updating;       /* to prevent recursive WM_COMMAND/EN_UPDATE processing */
    int h;
    int s;
    int l;               /* for temporary storing of hue,sat,lum */
    int capturedGraph;   /* control mouse captured */
    RECT focusRect;      /* rectangle last focused item */
    HWND hwndFocus;      /* handle last focused item */
} CCPRIV;

static int hsl_to_x(int hue, int sat, int lum)
{
 int res = 0, maxrgb;

 /* l below 120 */
 maxrgb = (256 * min(120,lum)) / 120;  /* 0 .. 256 */
 if (hue < 80)
  res = 0;
 else
  if (hue < 120)
  {
   res = (hue - 80) * maxrgb;           /* 0...10240 */
   res /= 40;                        /* 0...256 */
  }
  else
   if (hue < 200)
    res = maxrgb;
   else
    {
     res= (240 - hue) * maxrgb;
     res /= 40;
    }
 res = res - maxrgb / 2;                 /* -128...128 */

 /* saturation */
 res = maxrgb / 2 + (sat * res) / 240;    /* 0..256 */

 /* lum above 120 */
 if (lum > 120 && res < 256)
  res += ((lum - 120) * (256 - res)) / 120;

 return min(res, 255);
}

/***********************************************************************
 *                             CC_HSLtoRGB                    [internal]
 */
static COLORREF CC_HSLtoRGB(int hue, int sat, int lum)
{
 int h, r, g, b;

 /* r */
 h = hue > 80 ? hue-80 : hue+160;
 r = hsl_to_x(h, sat, lum);
 /* g */
 h = hue > 160 ? hue-160 : hue+80;
 g = hsl_to_x(h, sat, lum);
 /* b */
 b = hsl_to_x(hue, sat, lum);

 return RGB(r, g, b);
}

/***********************************************************************
 *                             CC_RGBtoHSL                    [internal]
 */
static int CC_RGBtoHSL(char c, COLORREF rgb)
{
 WORD maxi, mini, mmsum, mmdif, result = 0;
 int iresult = 0, r, g, b;

 r = GetRValue(rgb);
 g = GetGValue(rgb);
 b = GetBValue(rgb);

 maxi = max(r, b);
 maxi = max(maxi, g);
 mini = min(r, b);
 mini = min(mini, g);

 mmsum = maxi + mini;
 mmdif = maxi - mini;

 switch(c)
 {
  /* lum */
  case 'L': mmsum *= 120;              /* 0...61200=(255+255)*120 */
	   result = mmsum / 255;        /* 0...240 */
	   break;
  /* saturation */
  case 'S': if (!mmsum)
	    result = 0;
	   else
	    if (!mini || maxi == 255)
	     result = 240;
	   else
	   {
	    result = mmdif * 240;       /* 0...61200=255*240 */
	    result /= (mmsum > 255 ? 510 - mmsum : mmsum); /* 0..255 */
	   }
	   break;
  /* hue */
  case 'H': if (!mmdif)
	    result = 160;
	   else
	   {
	    if (maxi == r)
	    {
	     iresult = 40 * (g - b);       /* -10200 ... 10200 */
	     iresult /= (int) mmdif;    /* -40 .. 40 */
	     if (iresult < 0)
	      iresult += 240;          /* 0..40 and 200..240 */
	    }
	    else
	     if (maxi == g)
	     {
	      iresult = 40 * (b - r);
	      iresult /= (int) mmdif;
	      iresult += 80;           /* 40 .. 120 */
	     }
	     else
	      if (maxi == b)
	      {
	       iresult = 40 * (r - g);
	       iresult /= (int) mmdif;
	       iresult += 160;         /* 120 .. 200 */
	      }
	    result = iresult;
	   }
	   break;
 }
 return result;    /* is this integer arithmetic precise enough ? */
}


/***********************************************************************
 *                  CC_DrawCurrentFocusRect                       [internal]
 */
static void CC_DrawCurrentFocusRect( const CCPRIV *lpp )
{
  if (lpp->hwndFocus)
  {
    HDC hdc = GetDC(lpp->hwndFocus);
    DrawFocusRect(hdc, &lpp->focusRect);
    ReleaseDC(lpp->hwndFocus, hdc);
  }
}

/***********************************************************************
 *                  CC_DrawFocusRect                       [internal]
 */
static void CC_DrawFocusRect(CCPRIV *lpp, HWND hwnd, int x, int y, int rows, int cols)
{
  RECT rect;
  int dx, dy;
  HDC hdc;

  CC_DrawCurrentFocusRect(lpp); /* remove current focus rect */
  /* calculate new rect */
  GetClientRect(hwnd, &rect);
  dx = (rect.right - rect.left) / cols;
  dy = (rect.bottom - rect.top) / rows;
  rect.left += (x * dx) - 2;
  rect.top += (y * dy) - 2;
  rect.right = rect.left + dx;
  rect.bottom = rect.top + dy;
  /* draw it */
  hdc = GetDC(hwnd);
  DrawFocusRect(hdc, &rect);
  lpp->focusRect = rect;
  lpp->hwndFocus = hwnd;
  ReleaseDC(hwnd, hdc);
}

#define DISTANCE 4

/***********************************************************************
 *                CC_MouseCheckPredefColorArray               [internal]
 *                returns TRUE if one of the predefined colors is clicked
 */
static BOOL CC_MouseCheckPredefColorArray(CCPRIV *lpp, int rows, int cols, LPARAM lParam)
{
 HWND hwnd;
 POINT point;
 RECT rect;
 int dx, dy, x, y;

 CONV_LPARAMTOPOINT(lParam, &point);
 ClientToScreen(lpp->hwndSelf, &point);
 hwnd = GetDlgItem(lpp->hwndSelf, COLOR_BOX1);
 GetWindowRect(hwnd, &rect);
 if (PtInRect(&rect, point))
 {
  dx = (rect.right - rect.left) / cols;
  dy = (rect.bottom - rect.top) / rows;
  ScreenToClient(hwnd, &point);

  if (point.x % dx < ( dx - DISTANCE) && point.y % dy < ( dy - DISTANCE))
  {
   x = point.x / dx;
   y = point.y / dy;
   lpp->lpcc->rgbResult = predefcolors[y][x];
   CC_DrawFocusRect(lpp, hwnd, x, y, rows, cols);
   return TRUE;
  }
 }
 return FALSE;
}

/***********************************************************************
 *                  CC_MouseCheckUserColorArray               [internal]
 *                  return TRUE if the user clicked a color
 */
static BOOL CC_MouseCheckUserColorArray(CCPRIV *lpp, int rows, int cols, LPARAM lParam)
{
 HWND hwnd;
 POINT point;
 RECT rect;
 int dx, dy, x, y;
 COLORREF *crarr = lpp->lpcc->lpCustColors;

 CONV_LPARAMTOPOINT(lParam, &point);
 ClientToScreen(lpp->hwndSelf, &point);
 hwnd = GetDlgItem(lpp->hwndSelf, COLOR_CUSTOM1);
 GetWindowRect(hwnd, &rect);
 if (PtInRect(&rect, point))
 {
  dx = (rect.right - rect.left) / cols;
  dy = (rect.bottom - rect.top) / rows;
  ScreenToClient(hwnd, &point);

  if (point.x % dx < (dx - DISTANCE) && point.y % dy < (dy - DISTANCE))
  {
   x = point.x / dx;
   y = point.y / dy;
   lpp->lpcc->rgbResult = crarr[x + (cols * y) ];
   CC_DrawFocusRect(lpp, hwnd, x, y, rows, cols);
   return TRUE;
  }
 }
 return FALSE;
}

#define MAXVERT  240
#define MAXHORI  239

/*  240  ^......        ^^ 240
	 |     .        ||
    SAT  |     .        || LUM
	 |     .        ||
	 +-----> 239   ----
	   HUE
*/
/***********************************************************************
 *                  CC_MouseCheckColorGraph                   [internal]
 */
static BOOL CC_MouseCheckColorGraph( HWND hDlg, int dlgitem, int *hori, int *vert, LPARAM lParam )
{
 HWND hwnd;
 POINT point;
 RECT rect;
 int x,y;

 CONV_LPARAMTOPOINT(lParam, &point);
 ClientToScreen(hDlg, &point);
 hwnd = GetDlgItem( hDlg, dlgitem );
 GetWindowRect(hwnd, &rect);

 if (!PtInRect(&rect, point))
  return FALSE;

 GetClientRect(hwnd, &rect);
 ScreenToClient(hwnd, &point);

 x = (point.x * MAXHORI) / rect.right;
 y = ((rect.bottom - point.y) * MAXVERT) / rect.bottom;

 if (x < 0) x = 0;
 if (y < 0) y = 0;
 if (x > MAXHORI) x = MAXHORI;
 if (y > MAXVERT) y = MAXVERT;

 if (hori)
  *hori = x;
 if (vert)
  *vert = y;

 return TRUE;
}
/***********************************************************************
 *                  CC_MouseCheckResultWindow                 [internal]
 *                  test if double click one of the result colors
 */
static BOOL CC_MouseCheckResultWindow( HWND hDlg, LPARAM lParam )
{
 HWND hwnd;
 POINT point;
 RECT rect;

 CONV_LPARAMTOPOINT(lParam, &point);
 ClientToScreen(hDlg, &point);
 hwnd = GetDlgItem(hDlg, COLOR_CURRENT);
 GetWindowRect(hwnd, &rect);
 if (PtInRect(&rect, point))
 {
  PostMessageA(hDlg, WM_COMMAND, COLOR_SOLID, 0);
  return TRUE;
 }
 return FALSE;
}

/***********************************************************************
 *                       CC_CheckDigitsInEdit                 [internal]
 */
static int CC_CheckDigitsInEdit( CCPRIV *infoPtr, HWND hwnd, int maxval )
{
 int i, k, m, result, value;
 char buffer[30];

 GetWindowTextA(hwnd, buffer, ARRAY_SIZE(buffer));
 m = strlen(buffer);
 result = 0;

 for (i = 0 ; i < m ; i++)
  if (buffer[i] < '0' || buffer[i] > '9')
  {
   for (k = i + 1; k <= m; k++)  /* delete bad character */
   {
    buffer[i] = buffer[k];
    m--;
   }
   buffer[m] = 0;
   result = 1;
  }

 value = atoi(buffer);
 if (value > maxval)       /* build a new string */
 {
  value = maxval;
  sprintf(buffer, "%d", maxval);
  result = 2;
 }
 if (result)
 {
  LRESULT editpos = SendMessageA(hwnd, EM_GETSEL, 0, 0);
  infoPtr->updating = TRUE;
  SetWindowTextA(hwnd, buffer );
  SendMessageA(hwnd, EM_SETSEL, 0, editpos);
  infoPtr->updating = FALSE;
 }
 return value;
}



/***********************************************************************
 *                    CC_PaintSelectedColor                   [internal]
 */
static void CC_PaintSelectedColor(const CCPRIV *infoPtr)
{
 if (IsWindowVisible( GetDlgItem(infoPtr->hwndSelf, COLOR_RAINBOW) ))   /* if full size */
 {
  RECT rect;
  HDC  hdc;
  HBRUSH hBrush;
  HWND hwnd = GetDlgItem(infoPtr->hwndSelf, COLOR_CURRENT);

  hdc = GetDC(hwnd);
  GetClientRect(hwnd, &rect) ;
  hBrush = CreateSolidBrush(infoPtr->lpcc->rgbResult);
  if (hBrush)
  {
   FillRect(hdc, &rect, hBrush);
   DrawEdge(hdc, &rect, BDR_SUNKENOUTER, BF_RECT);
   DeleteObject(hBrush);
  }
  ReleaseDC(hwnd, hdc);
 }
}

/***********************************************************************
 *                    CC_PaintTriangle                        [internal]
 */
static void CC_PaintTriangle(CCPRIV *infoPtr)
{
 HDC hDC;
 int temp;
 int w = LOWORD(GetDialogBaseUnits()) / 2;
 POINT points[3];
 int height;
 int oben;
 RECT rect;
 HBRUSH hbr;
 HWND hwnd = GetDlgItem(infoPtr->hwndSelf, COLOR_LUMSCROLL);

 if (IsWindowVisible( GetDlgItem(infoPtr->hwndSelf, COLOR_RAINBOW)))   /* if full size */
 {
   GetClientRect(hwnd, &rect);
   height = rect.bottom;
   hDC = GetDC(infoPtr->hwndSelf);
   points[0].y = rect.top;
   points[0].x = rect.right;                  /*  |  /|  */
   ClientToScreen(hwnd, points);              /*  | / |  */
   ScreenToClient(infoPtr->hwndSelf, points); /*  |<  |  */
   oben = points[0].y;                        /*  | \ |  */
                                              /*  |  \|  */
   temp = height * infoPtr->l;
   points[0].x += 1;
   points[0].y = oben + height - temp / MAXVERT;
   points[1].y = points[0].y + w;
   points[2].y = points[0].y - w;
   points[2].x = points[1].x = points[0].x + w;

   hbr = (HBRUSH)GetClassLongPtrW( hwnd, GCLP_HBRBACKGROUND);
   if (!hbr) hbr = GetSysColorBrush(COLOR_BTNFACE);
   FillRect(hDC, &infoPtr->old3angle, hbr);
   infoPtr->old3angle.left  = points[0].x;
   infoPtr->old3angle.right = points[1].x + 1;
   infoPtr->old3angle.top   = points[2].y - 1;
   infoPtr->old3angle.bottom= points[1].y + 1;

   hbr = SelectObject(hDC, GetStockObject(BLACK_BRUSH));
   Polygon(hDC, points, 3);
   SelectObject(hDC, hbr);

   ReleaseDC(infoPtr->hwndSelf, hDC);
 }
}


/***********************************************************************
 *                    CC_PaintCross                           [internal]
 */
static void CC_PaintCross(CCPRIV *infoPtr)
{
 HWND hwnd = GetDlgItem(infoPtr->hwndSelf, COLOR_RAINBOW);

 if (IsWindowVisible(hwnd))   /* if full size */
 {
   int w, wc, width;
   HDC hDC;
   RECT rect;
   POINT point;
   HRGN region;

   rect.left = 6;
   rect.right = 2;
   rect.top = 1;
   rect.bottom = 0;
   MapDialogRect(infoPtr->hwndSelf, &rect);
   w = rect.left;
   wc = rect.right;
   width = rect.top;

   GetClientRect(hwnd, &rect);
   hDC = GetDC(hwnd);
   region = CreateRectRgnIndirect(&rect);
   SelectClipRgn(hDC, region);
   DeleteObject(region);

   point.x = (rect.right * infoPtr->h) / MAXHORI;
   point.y = rect.bottom - (rect.bottom * infoPtr->s) / MAXVERT;

   rect.left = point.x - w;
   rect.right = point.x - wc;
   rect.top = point.y - width;
   rect.bottom = point.y + width;
   FillRect(hDC, &rect, GetStockObject(BLACK_BRUSH));

   rect.left = point.x + wc;
   rect.right = point.x + w;
   FillRect(hDC, &rect, GetStockObject(BLACK_BRUSH));

   rect.left = point.x - width;
   rect.right = point.x + width;
   rect.top = point.y - w;
   rect.bottom = point.y - wc;
   FillRect(hDC, &rect, GetStockObject(BLACK_BRUSH));

   rect.top = point.y + wc;
   rect.bottom = point.y + w;
   FillRect(hDC, &rect, GetStockObject(BLACK_BRUSH));

   ReleaseDC(hwnd, hDC);
 }
}


#define XSTEPS 48
#define YSTEPS 24


/***********************************************************************
 *                    CC_PrepareColorGraph                    [internal]
 */
static void CC_PrepareColorGraph(CCPRIV *infoPtr)
{
 int sdif, hdif, xdif, ydif, hue, sat;
 HWND hwnd = GetDlgItem(infoPtr->hwndSelf, COLOR_RAINBOW);
 HBRUSH hbrush;
 HDC hdc ;
 RECT rect, client;
 HCURSOR hcursor = SetCursor( LoadCursorW(0, (LPCWSTR)IDC_WAIT) );

 GetClientRect(hwnd, &client);
 hdc = GetDC(hwnd);
 infoPtr->hdcMem = CreateCompatibleDC(hdc);
 infoPtr->hbmMem = CreateCompatibleBitmap(hdc, client.right, client.bottom);
 SelectObject(infoPtr->hdcMem, infoPtr->hbmMem);

 xdif = client.right / XSTEPS;
 ydif = client.bottom / YSTEPS+1;
 hdif = 239 / XSTEPS;
 sdif = 240 / YSTEPS;
 for (rect.left = hue = 0; hue < 239 + hdif; hue += hdif)
 {
  rect.right = rect.left + xdif;
  rect.bottom = client.bottom;
  for(sat = 0; sat < 240 + sdif; sat += sdif)
  {
   rect.top = rect.bottom - ydif;
   hbrush = CreateSolidBrush(CC_HSLtoRGB(hue, sat, 120));
   FillRect(infoPtr->hdcMem, &rect, hbrush);
   DeleteObject(hbrush);
   rect.bottom = rect.top;
  }
  rect.left = rect.right;
 }
 ReleaseDC(hwnd, hdc);
 SetCursor(hcursor);
}

/***********************************************************************
 *                          CC_PaintColorGraph                [internal]
 */
static void CC_PaintColorGraph(CCPRIV *infoPtr)
{
 HWND hwnd = GetDlgItem( infoPtr->hwndSelf, COLOR_RAINBOW );
 HDC  hDC;
 RECT rect;

 if (IsWindowVisible(hwnd))   /* if full size */
 {
  if (!infoPtr->hdcMem)
   CC_PrepareColorGraph(infoPtr);   /* should not be necessary */

  hDC = GetDC(hwnd);
  GetClientRect(hwnd, &rect);
  if (infoPtr->hdcMem)
      BitBlt(hDC, 0, 0, rect.right, rect.bottom, infoPtr->hdcMem, 0, 0, SRCCOPY);
  else
      WARN("choose color: hdcMem is not defined\n");
  ReleaseDC(hwnd, hDC);
 }
}

/***********************************************************************
 *                           CC_PaintLumBar                   [internal]
 */
static void CC_PaintLumBar(const CCPRIV *infoPtr)
{
 HWND hwnd = GetDlgItem(infoPtr->hwndSelf, COLOR_LUMSCROLL);
 RECT rect, client;
 int lum, ldif, ydif;
 HBRUSH hbrush;
 HDC hDC;

 if (IsWindowVisible(hwnd))
 {
  hDC = GetDC(hwnd);
  GetClientRect(hwnd, &client);
  rect = client;

  ldif = 240 / YSTEPS;
  ydif = client.bottom / YSTEPS+1;
  for (lum = 0; lum < 240 + ldif; lum += ldif)
  {
   rect.top = max(0, rect.bottom - ydif);
   hbrush = CreateSolidBrush(CC_HSLtoRGB(infoPtr->h, infoPtr->s, lum));
   FillRect(hDC, &rect, hbrush);
   DeleteObject(hbrush);
   rect.bottom = rect.top;
  }
  GetClientRect(hwnd, &rect);
  DrawEdge(hDC, &rect, BDR_SUNKENOUTER, BF_RECT);
  ReleaseDC(hwnd, hDC);
 }
}

/***********************************************************************
 *                             CC_EditSetRGB                  [internal]
 */
static void CC_EditSetRGB( CCPRIV *infoPtr )
{
 if (IsWindowVisible( GetDlgItem(infoPtr->hwndSelf, COLOR_RAINBOW) ))   /* if full size */
 {
   COLORREF cr = infoPtr->lpcc->rgbResult;
   int r = GetRValue(cr);
   int g = GetGValue(cr);
   int b = GetBValue(cr);

   infoPtr->updating = TRUE;
   SetDlgItemInt(infoPtr->hwndSelf, COLOR_RED, r, TRUE);
   SetDlgItemInt(infoPtr->hwndSelf, COLOR_GREEN, g, TRUE);
   SetDlgItemInt(infoPtr->hwndSelf, COLOR_BLUE, b, TRUE);
   infoPtr->updating = FALSE;
 }
}

/***********************************************************************
 *                             CC_EditSetHSL                  [internal]
 */
static void CC_EditSetHSL( CCPRIV *infoPtr )
{
 if (IsWindowVisible( GetDlgItem(infoPtr->hwndSelf, COLOR_RAINBOW) ))   /* if full size */
 {
   infoPtr->updating = TRUE;
   SetDlgItemInt(infoPtr->hwndSelf, COLOR_HUE, infoPtr->h, TRUE);
   SetDlgItemInt(infoPtr->hwndSelf, COLOR_SAT, infoPtr->s, TRUE);
   SetDlgItemInt(infoPtr->hwndSelf, COLOR_LUM, infoPtr->l, TRUE);
   infoPtr->updating = FALSE;
 }
 CC_PaintLumBar(infoPtr);
}

/***********************************************************************
 *                       CC_SwitchToFullSize                  [internal]
 */
static void CC_SwitchToFullSize( CCPRIV *infoPtr, const RECT *lprect )
{
 int i;

 EnableWindow( GetDlgItem(infoPtr->hwndSelf, COLOR_MIX), FALSE);
 CC_PrepareColorGraph(infoPtr);
 for (i = COLOR_HUE; i <= COLOR_BLUE; i++)
   ShowWindow( GetDlgItem(infoPtr->hwndSelf, i), SW_SHOW);
 for (i = COLOR_HUEACCEL; i <= COLOR_BLUEACCEL; i++)
   ShowWindow( GetDlgItem(infoPtr->hwndSelf, i), SW_SHOW);
 ShowWindow( GetDlgItem(infoPtr->hwndSelf, COLOR_SOLID), SW_SHOW);
 ShowWindow( GetDlgItem(infoPtr->hwndSelf, COLOR_ADD), SW_SHOW);
 ShowWindow( GetDlgItem(infoPtr->hwndSelf, 1090), SW_SHOW);

 if (lprect)
  SetWindowPos(infoPtr->hwndSelf, 0, 0, 0, lprect->right-lprect->left,
   lprect->bottom-lprect->top, SWP_NOMOVE|SWP_NOZORDER);

 ShowWindow( GetDlgItem(infoPtr->hwndSelf, COLOR_LUMSCROLL), SW_SHOW);
 ShowWindow( GetDlgItem(infoPtr->hwndSelf, COLOR_CURRENT), SW_SHOW);

 CC_EditSetRGB(infoPtr);
 CC_EditSetHSL(infoPtr);
 ShowWindow( GetDlgItem( infoPtr->hwndSelf, COLOR_RAINBOW), SW_SHOW);
 UpdateWindow( GetDlgItem(infoPtr->hwndSelf, COLOR_RAINBOW) );
}

/***********************************************************************
 *                           CC_PaintPredefColorArray         [internal]
 *                Paints the default standard 48 colors
 */
static void CC_PaintPredefColorArray(const CCPRIV *infoPtr, int rows, int cols)
{
 HWND hwnd = GetDlgItem(infoPtr->hwndSelf, COLOR_BOX1);
 RECT rect, blockrect;
 HDC  hdc;
 HBRUSH hBrush;
 int dx, dy, i, j, k;

 GetClientRect(hwnd, &rect);
 dx = rect.right / cols;
 dy = rect.bottom / rows;
 k = rect.left;

 hdc = GetDC(hwnd);
 GetClientRect(hwnd, &rect);
 hBrush = (HBRUSH)GetClassLongPtrW( hwnd, GCLP_HBRBACKGROUND);
 if (!hBrush) hBrush = GetSysColorBrush(COLOR_BTNFACE);
 FillRect(hdc, &rect, hBrush);
 for ( j = 0; j < rows; j++ )
 {
  for ( i = 0; i < cols; i++ )
  {
   hBrush = CreateSolidBrush(predefcolors[j][i]);
   if (hBrush)
   {
    blockrect.left = rect.left;
    blockrect.top = rect.top;
    blockrect.right = rect.left + dx - DISTANCE;
    blockrect.bottom = rect.top + dy - DISTANCE;
    FillRect(hdc, &blockrect, hBrush);
    DrawEdge(hdc, &blockrect, BDR_SUNKEN, BF_RECT);
    DeleteObject(hBrush);
   }
   rect.left += dx;
  }
  rect.top += dy;
  rect.left = k;
 }
 ReleaseDC(hwnd, hdc);
 if (infoPtr->hwndFocus == hwnd)
   CC_DrawCurrentFocusRect(infoPtr);
}
/***********************************************************************
 *                             CC_PaintUserColorArray         [internal]
 *               Paint the 16 user-selected colors
 */
static void CC_PaintUserColorArray(const CCPRIV *infoPtr, int rows, int cols)
{
 HWND hwnd = GetDlgItem(infoPtr->hwndSelf, COLOR_CUSTOM1);
 RECT rect, blockrect;
 HDC  hdc;
 HBRUSH hBrush;
 int dx, dy, i, j, k;

 GetClientRect(hwnd, &rect);

 dx = rect.right / cols;
 dy = rect.bottom / rows;
 k = rect.left;

 hdc = GetDC(hwnd);
 if (hdc)
 {
  hBrush = (HBRUSH)GetClassLongPtrW( hwnd, GCLP_HBRBACKGROUND);
  if (!hBrush) hBrush = GetSysColorBrush(COLOR_BTNFACE);
  FillRect( hdc, &rect, hBrush );
  for (j = 0; j < rows; j++)
  {
   for (i = 0; i < cols; i++)
   {
    hBrush = CreateSolidBrush(infoPtr->lpcc->lpCustColors[i+j*cols]);
    if (hBrush)
    {
     blockrect.left = rect.left;
     blockrect.top = rect.top;
     blockrect.right = rect.left + dx - DISTANCE;
     blockrect.bottom = rect.top + dy - DISTANCE;
     FillRect(hdc, &blockrect, hBrush);
     DrawEdge(hdc, &blockrect, BDR_SUNKEN, BF_RECT);
     DeleteObject(hBrush);
    }
    rect.left += dx;
   }
   rect.top += dy;
   rect.left = k;
  }
  ReleaseDC(hwnd, hdc);
 }
 if (infoPtr->hwndFocus == hwnd)
   CC_DrawCurrentFocusRect(infoPtr);
}


/***********************************************************************
 *                             CC_HookCallChk                 [internal]
 */
static BOOL CC_HookCallChk( const CHOOSECOLORW *lpcc )
{
 if (lpcc)
  if(lpcc->Flags & CC_ENABLEHOOK)
   if (lpcc->lpfnHook)
    return TRUE;
 return FALSE;
}

/***********************************************************************
 *                              CC_WMInitDialog                  [internal]
 */
static LRESULT CC_WMInitDialog( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   CHOOSECOLORW *cc = (CHOOSECOLORW*)lParam;
   int i, res;
   int r, g, b;
   HWND hwnd;
   RECT rect;
   POINT point;
   CCPRIV *lpp;

   TRACE("WM_INITDIALOG lParam=%08IX\n", lParam);

   if (cc->lStructSize != sizeof(CHOOSECOLORW))
   {
       EndDialog(hDlg, 0);
       return FALSE;
   }

   lpp = heap_alloc_zero(sizeof(*lpp));
   lpp->lpcc = cc;
   lpp->hwndSelf = hDlg;

   SetPropW( hDlg, L"colourdialogprop", lpp );

   if (!(lpp->lpcc->Flags & CC_SHOWHELP))
      ShowWindow(GetDlgItem(hDlg, pshHelp), SW_HIDE);
   lpp->msetrgb = RegisterWindowMessageA(SETRGBSTRINGA);

#if 0
   cpos = MAKELONG(5,7); /* init */
   if (lpp->lpcc->Flags & CC_RGBINIT)
   {
     for (i = 0; i < 6; i++)
       for (j = 0; j < 8; j++)
        if (predefcolors[i][j] == lpp->lpcc->rgbResult)
        {
          cpos = MAKELONG(i,j);
          goto found;
        }
   }
   found:
   /* FIXME: Draw_a_focus_rect & set_init_values */
#endif

   GetWindowRect(hDlg, &lpp->fullsize);
   if (lpp->lpcc->Flags & CC_FULLOPEN || lpp->lpcc->Flags & CC_PREVENTFULLOPEN)
   {
      hwnd = GetDlgItem(hDlg, COLOR_MIX);
      EnableWindow(hwnd, FALSE);
   }
   if (!(lpp->lpcc->Flags & CC_FULLOPEN ) || lpp->lpcc->Flags & CC_PREVENTFULLOPEN)
   {
      rect = lpp->fullsize;
      res = rect.bottom - rect.top;
      hwnd = GetDlgItem(hDlg, COLOR_RAINBOW); /* cut at left border */
      point.x = point.y = 0;
      ClientToScreen(hwnd, &point);
      ScreenToClient(hDlg,&point);
      GetClientRect(hDlg, &rect);
      point.x += GetSystemMetrics(SM_CXDLGFRAME);
      SetWindowPos(hDlg, 0, 0, 0, point.x, res, SWP_NOMOVE|SWP_NOZORDER);

      for (i = COLOR_HUE; i <= COLOR_BLUE; i++)
         ShowWindow( GetDlgItem(hDlg, i), SW_HIDE);
      for (i = COLOR_HUEACCEL; i <= COLOR_BLUEACCEL; i++)
         ShowWindow( GetDlgItem(hDlg, i), SW_HIDE);
      ShowWindow( GetDlgItem(hDlg, COLOR_SOLID), SW_HIDE);
      ShowWindow( GetDlgItem(hDlg, COLOR_ADD), SW_HIDE);
      ShowWindow( GetDlgItem(hDlg, COLOR_RAINBOW), SW_HIDE);
      ShowWindow( GetDlgItem(hDlg, COLOR_CURRENT), SW_HIDE);
      ShowWindow( GetDlgItem(hDlg, 1090 ), SW_HIDE);
   }
   else
      CC_SwitchToFullSize(lpp, NULL);
   res = TRUE;
   for (i = COLOR_HUE; i <= COLOR_BLUE; i++)
     SendMessageA( GetDlgItem(hDlg, i), EM_LIMITTEXT, 3, 0);  /* max 3 digits:  xyz  */
   if (CC_HookCallChk(lpp->lpcc))
   {
          res = CallWindowProcA( (WNDPROC)lpp->lpcc->lpfnHook, hDlg, WM_INITDIALOG, wParam, lParam);
   }

   /* Set the initial values of the color chooser dialog */
   r = GetRValue(lpp->lpcc->rgbResult);
   g = GetGValue(lpp->lpcc->rgbResult);
   b = GetBValue(lpp->lpcc->rgbResult);

   CC_PaintSelectedColor(lpp);
   lpp->h = CC_RGBtoHSL('H', lpp->lpcc->rgbResult);
   lpp->s = CC_RGBtoHSL('S', lpp->lpcc->rgbResult);
   lpp->l = CC_RGBtoHSL('L', lpp->lpcc->rgbResult);

   /* Doing it the long way because CC_EditSetRGB/HSL doesn't seem to work */
   SetDlgItemInt(hDlg, COLOR_HUE, lpp->h, TRUE);
   SetDlgItemInt(hDlg, COLOR_SAT, lpp->s, TRUE);
   SetDlgItemInt(hDlg, COLOR_LUM, lpp->l, TRUE);
   SetDlgItemInt(hDlg, COLOR_RED, r, TRUE);
   SetDlgItemInt(hDlg, COLOR_GREEN, g, TRUE);
   SetDlgItemInt(hDlg, COLOR_BLUE, b, TRUE);

   CC_PaintCross(lpp);
   CC_PaintTriangle(lpp);

   return res;
}


/***********************************************************************
 *                              CC_WMCommand                  [internal]
 */
static LRESULT CC_WMCommand(CCPRIV *lpp, WPARAM wParam, LPARAM lParam, WORD notifyCode, HWND hwndCtl)
{
    int  r, g, b, i, xx;
    UINT cokmsg;
    HDC hdc;
    COLORREF *cr;

    TRACE("CC_WMCommand wParam=%Ix lParam=%Ix\n", wParam, lParam);
    switch (LOWORD(wParam))
    {
        case COLOR_RED:  /* edit notify RGB */
        case COLOR_GREEN:
        case COLOR_BLUE:
	       if (notifyCode == EN_UPDATE && !lpp->updating)
			 {
			   i = CC_CheckDigitsInEdit(lpp, hwndCtl, 255);
			   r = GetRValue(lpp->lpcc->rgbResult);
			   g = GetGValue(lpp->lpcc->rgbResult);
			   b = GetBValue(lpp->lpcc->rgbResult);
			   xx = 0;
			   switch (LOWORD(wParam))
			   {
			    case COLOR_RED: if ((xx = (i != r))) r = i; break;
			    case COLOR_GREEN: if ((xx = (i != g))) g = i; break;
			    case COLOR_BLUE: if ((xx = (i != b))) b = i; break;
			   }
			   if (xx) /* something has changed */
			   {
			    lpp->lpcc->rgbResult = RGB(r, g, b);
			    CC_PaintSelectedColor(lpp);
			    lpp->h = CC_RGBtoHSL('H', lpp->lpcc->rgbResult);
			    lpp->s = CC_RGBtoHSL('S', lpp->lpcc->rgbResult);
			    lpp->l = CC_RGBtoHSL('L', lpp->lpcc->rgbResult);
			    CC_EditSetHSL(lpp);
			    CC_PaintCross(lpp);
			    CC_PaintTriangle(lpp);
			   }
			 }
		 break;

        case COLOR_HUE:  /* edit notify HSL */
        case COLOR_SAT:
        case COLOR_LUM:
	       if (notifyCode == EN_UPDATE && !lpp->updating)
			 {
			   i = CC_CheckDigitsInEdit(lpp, hwndCtl, LOWORD(wParam) == COLOR_HUE ? 239 : 240);
			   xx = 0;
			   switch (LOWORD(wParam))
			   {
			    case COLOR_HUE: if ((xx = ( i != lpp->h))) lpp->h = i; break;
			    case COLOR_SAT: if ((xx = ( i != lpp->s))) lpp->s = i; break;
			    case COLOR_LUM: if ((xx = ( i != lpp->l))) lpp->l = i; break;
			   }
			   if (xx) /* something has changed */
			   {
			    lpp->lpcc->rgbResult = CC_HSLtoRGB(lpp->h, lpp->s, lpp->l);
			    CC_PaintSelectedColor(lpp);
			    CC_EditSetRGB(lpp);
			    CC_PaintCross(lpp);
			    CC_PaintTriangle(lpp);
			    CC_PaintLumBar(lpp);
			   }
			 }
	       break;

        case COLOR_MIX:
               CC_SwitchToFullSize(lpp, &lpp->fullsize);
	       SetFocus( GetDlgItem(lpp->hwndSelf, COLOR_HUE));
	       break;

        case COLOR_ADD:    /* add colors ... column by column */
               cr = lpp->lpcc->lpCustColors;
               cr[(lpp->nextuserdef % 2) * 8 + lpp->nextuserdef / 2] = lpp->lpcc->rgbResult;
               if (++lpp->nextuserdef == 16)
		   lpp->nextuserdef = 0;
	       CC_PaintUserColorArray(lpp, 2, 8);
	       break;

        case COLOR_SOLID:              /* resulting color */
	       hdc = GetDC(lpp->hwndSelf);
	       lpp->lpcc->rgbResult = GetNearestColor(hdc, lpp->lpcc->rgbResult);
	       ReleaseDC(lpp->hwndSelf, hdc);
	       CC_EditSetRGB(lpp);
	       CC_PaintSelectedColor(lpp);
	       lpp->h = CC_RGBtoHSL('H', lpp->lpcc->rgbResult);
	       lpp->s = CC_RGBtoHSL('S', lpp->lpcc->rgbResult);
	       lpp->l = CC_RGBtoHSL('L', lpp->lpcc->rgbResult);
	       CC_EditSetHSL(lpp);
	       CC_PaintCross(lpp);
	       CC_PaintTriangle(lpp);
	       break;

	  case pshHelp:           /* Help! */ /* The Beatles, 1965  ;-) */
	       i = RegisterWindowMessageA(HELPMSGSTRINGA);
                   if (lpp->lpcc->hwndOwner)
		       SendMessageA(lpp->lpcc->hwndOwner, i, 0, (LPARAM)lpp->lpcc);
                   if ( CC_HookCallChk(lpp->lpcc))
		       CallWindowProcA( (WNDPROC) lpp->lpcc->lpfnHook, lpp->hwndSelf,
		          WM_COMMAND, psh15, (LPARAM)lpp->lpcc);
	       break;

          case IDOK :
		cokmsg = RegisterWindowMessageA(COLOROKSTRINGA);
		    if (lpp->lpcc->hwndOwner)
			if (SendMessageA(lpp->lpcc->hwndOwner, cokmsg, 0, (LPARAM)lpp->lpcc))
			break;    /* do NOT close */
		EndDialog(lpp->hwndSelf, 1) ;
		return TRUE ;

	  case IDCANCEL :
		EndDialog(lpp->hwndSelf, 0) ;
		return TRUE ;

       }
       return FALSE;
}

/***********************************************************************
 *                              CC_WMPaint                    [internal]
 */
static LRESULT CC_WMPaint( CCPRIV *lpp )
{
    PAINTSTRUCT ps;

    BeginPaint(lpp->hwndSelf, &ps);
    /* we have to paint dialog children except text and buttons */
    CC_PaintPredefColorArray(lpp, 6, 8);
    CC_PaintUserColorArray(lpp, 2, 8);
    CC_PaintLumBar(lpp);
    CC_PaintTriangle(lpp);
    CC_PaintSelectedColor(lpp);
    CC_PaintColorGraph(lpp);
    CC_PaintCross(lpp);
    EndPaint(lpp->hwndSelf, &ps);

    return TRUE;
}

/***********************************************************************
 *                              CC_WMLButtonUp              [internal]
 */
static LRESULT CC_WMLButtonUp( CCPRIV *infoPtr )
{
   if (infoPtr->capturedGraph)
   {
       infoPtr->capturedGraph = 0;
       ReleaseCapture();
       CC_PaintColorGraph(infoPtr);
       CC_PaintCross(infoPtr);
       return 1;
   }
   return 0;
}

/***********************************************************************
 *                              CC_WMMouseMove              [internal]
 */
static LRESULT CC_WMMouseMove( CCPRIV *infoPtr, LPARAM lParam )
{
   if (infoPtr->capturedGraph)
   {
      int *ptrh = NULL, *ptrs = &infoPtr->l;
      if (infoPtr->capturedGraph == COLOR_RAINBOW)
      {
          ptrh = &infoPtr->h;
          ptrs = &infoPtr->s;
      }
      if (CC_MouseCheckColorGraph( infoPtr->hwndSelf, infoPtr->capturedGraph, ptrh, ptrs, lParam))
      {
          infoPtr->lpcc->rgbResult = CC_HSLtoRGB(infoPtr->h, infoPtr->s, infoPtr->l);
          CC_EditSetRGB(infoPtr);
          CC_EditSetHSL(infoPtr);
          CC_PaintColorGraph(infoPtr);
          CC_PaintCross(infoPtr);
          CC_PaintTriangle(infoPtr);
          CC_PaintSelectedColor(infoPtr);
      }
      else
      {
          ReleaseCapture();
          infoPtr->capturedGraph = 0;
      }
      return 1;
   }
   return 0;
}

/***********************************************************************
 *                              CC_WMLButtonDown              [internal]
 */
static LRESULT CC_WMLButtonDown( CCPRIV *infoPtr, LPARAM lParam )
{
   int i = 0;

   if (CC_MouseCheckPredefColorArray(infoPtr, 6, 8, lParam))
      i = 1;
   else
      if (CC_MouseCheckUserColorArray(infoPtr, 2, 8, lParam))
         i = 1;
      else
	 if (CC_MouseCheckColorGraph(infoPtr->hwndSelf, COLOR_RAINBOW, &infoPtr->h, &infoPtr->s, lParam))
         {
	    i = 2;
            infoPtr->capturedGraph = COLOR_RAINBOW;
         }
	 else
	    if (CC_MouseCheckColorGraph(infoPtr->hwndSelf, COLOR_LUMSCROLL, NULL, &infoPtr->l, lParam))
            {
	       i = 2;
               infoPtr->capturedGraph = COLOR_LUMSCROLL;
            }
   if ( i == 2 )
   {
      SetCapture(infoPtr->hwndSelf);
      infoPtr->lpcc->rgbResult = CC_HSLtoRGB(infoPtr->h, infoPtr->s, infoPtr->l);
   }
   if ( i == 1 )
   {
      infoPtr->h = CC_RGBtoHSL('H', infoPtr->lpcc->rgbResult);
      infoPtr->s = CC_RGBtoHSL('S', infoPtr->lpcc->rgbResult);
      infoPtr->l = CC_RGBtoHSL('L', infoPtr->lpcc->rgbResult);
   }
   if (i)
   {
      CC_EditSetRGB(infoPtr);
      CC_EditSetHSL(infoPtr);
      CC_PaintColorGraph(infoPtr);
      CC_PaintCross(infoPtr);
      CC_PaintTriangle(infoPtr);
      CC_PaintSelectedColor(infoPtr);
      return TRUE;
   }
   return FALSE;
}

/***********************************************************************
 *           ColorDlgProc32 [internal]
 *
 */
static INT_PTR CALLBACK ColorDlgProc( HWND hDlg, UINT message,
                                   WPARAM wParam, LPARAM lParam )
{

 int res;
 CCPRIV *lpp = GetPropW( hDlg, L"colourdialogprop" );

 if (message != WM_INITDIALOG)
 {
  if (!lpp)
     return FALSE;
  res = 0;
  if (CC_HookCallChk(lpp->lpcc))
     res = CallWindowProcA( (WNDPROC)lpp->lpcc->lpfnHook, hDlg, message, wParam, lParam);
  if ( res )
     return res;
 }

 /* FIXME: SetRGB message
 if (message && message == msetrgb)
    return HandleSetRGB(hDlg, lParam);
 */

 switch (message)
	{
	  case WM_INITDIALOG:
	                return CC_WMInitDialog(hDlg, wParam, lParam);
	  case WM_NCDESTROY:
	                DeleteDC(lpp->hdcMem);
	                DeleteObject(lpp->hbmMem);
                        heap_free(lpp);
                        RemovePropW( hDlg, L"colourdialogprop" );
	                break;
	  case WM_COMMAND:
	                if (CC_WMCommand(lpp, wParam, lParam, HIWORD(wParam), (HWND) lParam))
	                   return TRUE;
	                break;
	  case WM_PAINT:
	                if (CC_WMPaint(lpp))
	                   return TRUE;
	                break;
	  case WM_LBUTTONDBLCLK:
	                if (CC_MouseCheckResultWindow(hDlg, lParam))
			  return TRUE;
			break;
	  case WM_MOUSEMOVE:
	                if (CC_WMMouseMove(lpp, lParam))
			  return TRUE;
			break;
	  case WM_LBUTTONUP:  /* FIXME: ClipCursor off (if in color graph)*/
                        if (CC_WMLButtonUp(lpp))
                           return TRUE;
			break;
	  case WM_LBUTTONDOWN:/* FIXME: ClipCursor on  (if in color graph)*/
	                if (CC_WMLButtonDown(lpp, lParam))
	                   return TRUE;
	                break;
	}
     return FALSE ;
}

/***********************************************************************
 *            ChooseColorW  (COMDLG32.@)
 *
 * Create a color dialog box.
 *
 * PARAMS
 *  lpChCol [I/O] in:  information to initialize the dialog box.
 *                out: User's color selection
 *
 * RETURNS
 *  TRUE:  Ok button clicked.
 *  FALSE: Cancel button clicked, or error.
 */
BOOL WINAPI ChooseColorW( CHOOSECOLORW *lpChCol )
{
    HANDLE hDlgTmpl = 0;
    const void *template;

    TRACE("(%p)\n", lpChCol);

    if (!lpChCol) return FALSE;

    if (lpChCol->Flags & CC_ENABLETEMPLATEHANDLE)
    {
        if (!(template = LockResource(lpChCol->hInstance)))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
            return FALSE;
        }
    }
    else if (lpChCol->Flags & CC_ENABLETEMPLATE)
    {
	HRSRC hResInfo;
        if (!(hResInfo = FindResourceW((HINSTANCE)lpChCol->hInstance,
                                        lpChCol->lpTemplateName,
                                        (LPWSTR)RT_DIALOG)))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
            return FALSE;
        }
        if (!(hDlgTmpl = LoadResource((HINSTANCE)lpChCol->hInstance, hResInfo)) ||
            !(template = LockResource(hDlgTmpl)))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
            return FALSE;
        }
    }
    else
    {
	HRSRC hResInfo;
	HGLOBAL hDlgTmpl;
        if (!(hResInfo = FindResourceW(COMDLG32_hInstance, L"CHOOSE_COLOR", (LPWSTR)RT_DIALOG)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	    return FALSE;
	}
	if (!(hDlgTmpl = LoadResource(COMDLG32_hInstance, hResInfo )) ||
	    !(template = LockResource(hDlgTmpl)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
    }

    return DialogBoxIndirectParamW(COMDLG32_hInstance, template, lpChCol->hwndOwner,
                     ColorDlgProc, (LPARAM)lpChCol);
}

/***********************************************************************
 *            ChooseColorA  (COMDLG32.@)
 *
 * See ChooseColorW.
 */
BOOL WINAPI ChooseColorA( LPCHOOSECOLORA lpChCol )

{
  LPWSTR template_name = NULL;
  BOOL ret;

  CHOOSECOLORW *lpcc = heap_alloc_zero(sizeof(*lpcc));
  lpcc->lStructSize = sizeof(*lpcc);
  lpcc->hwndOwner = lpChCol->hwndOwner;
  lpcc->hInstance = lpChCol->hInstance;
  lpcc->rgbResult = lpChCol->rgbResult;
  lpcc->lpCustColors = lpChCol->lpCustColors;
  lpcc->Flags = lpChCol->Flags;
  lpcc->lCustData = lpChCol->lCustData;
  lpcc->lpfnHook = lpChCol->lpfnHook;
  if ((lpcc->Flags & CC_ENABLETEMPLATE) && (lpChCol->lpTemplateName)) {
      if (!IS_INTRESOURCE(lpChCol->lpTemplateName)) {
	  INT len = MultiByteToWideChar( CP_ACP, 0, lpChCol->lpTemplateName, -1, NULL, 0);
          template_name = heap_alloc( len * sizeof(WCHAR) );
          MultiByteToWideChar( CP_ACP, 0, lpChCol->lpTemplateName, -1, template_name, len );
          lpcc->lpTemplateName = template_name;
      } else {
	  lpcc->lpTemplateName = (LPCWSTR)lpChCol->lpTemplateName;
      }
  }

  ret = ChooseColorW(lpcc);

  if (ret)
      lpChCol->rgbResult = lpcc->rgbResult;

  heap_free(template_name);
  heap_free(lpcc);
  return ret;
}
