/*
 * COMMDLG - Color Dialog
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 */

/* BUGS : still seems to not refresh correctly
   sometimes, especially when 2 instances of the
   dialog are loaded at the same time */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "ldt.h"
#include "heap.h"
#include "commdlg.h"
#include "dialog.h"
#include "dlgs.h"
#include "module.h"
#include "debugtools.h"
#include "winproc.h"
#include "cderr.h"

DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"

static LRESULT WINAPI ColorDlgProc( HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam );

#define CONV_POINTSTOPOINT( ps, p ) \
            ((p)->x = (LONG)(ps)->x, (p)->y = (LONG)(ps)->y)
 
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

struct CCPRIVATE
{
 LPCHOOSECOLORW lpcc;  /* points to public known data structure */
 LPCHOOSECOLOR16 lpcc16; /* save the 16 bits pointer */
 int nextuserdef;     /* next free place in user defined color array */
 HDC hdcMem;        /* color graph used for BitBlt() */
 HBITMAP hbmMem;    /* color graph bitmap */    
 RECT fullsize;     /* original dialog window size */
 UINT msetrgb;        /* # of SETRGBSTRING message (today not used)  */
 RECT old3angle;    /* last position of l-marker */
 RECT oldcross;     /* last position of color/satuation marker */
 BOOL updating;     /* to prevent recursive WM_COMMAND/EN_UPDATE procesing */
 int h;
 int s;
 int l;               /* for temporary storing of hue,sat,lum */
 int capturedGraph; /* control mouse captured */
 RECT focusRect;    /* rectangle last focused item */
 HWND hwndFocus;    /* handle last focused item */
};

#define LCCPRIV struct CCPRIVATE *

/***********************************************************************
 *                             CC_HSLtoRGB                    [internal]
 */
static int CC_HSLtoRGB(char c, int hue, int sat, int lum)
{
 int res = 0, maxrgb;

 /* hue */
 switch(c)
 {
  case 'R': if (hue > 80)  hue -= 80; else hue += 160; break;
  case 'G': if (hue > 160) hue -= 160; else hue += 80; break;
  case 'B': break;
 }

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
 *                             CC_RGBtoHSL                    [internal]
 */
static int CC_RGBtoHSL(char c, int r, int g, int b)
{
 WORD maxi, mini, mmsum, mmdif, result = 0;
 int iresult = 0;

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
	    result /= (mmsum > 255 ? mmsum = 510 - mmsum : mmsum); /* 0..255 */
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
void CC_DrawCurrentFocusRect( LCCPRIV lpp )
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
void CC_DrawFocusRect( LCCPRIV lpp, HWND hwnd, int x, int y, int rows, int cols)
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
  CopyRect(&lpp->focusRect, &rect);
  lpp->hwndFocus = hwnd;
  ReleaseDC(hwnd, hdc);
}

#define DISTANCE 4

/***********************************************************************
 *                CC_MouseCheckPredefColorArray               [internal]
 *                returns 1 if one of the predefined colors is clicked
 */
static int CC_MouseCheckPredefColorArray( LCCPRIV lpp, HWND hDlg, int dlgitem, int rows, int cols,
	    LPARAM lParam )
{
 HWND hwnd;
 POINT point;
 RECT rect;
 int dx, dy, x, y;

 CONV_POINTSTOPOINT(&(MAKEPOINTS(lParam)) , &point);
 ClientToScreen(hDlg, &point);
 hwnd = GetDlgItem(hDlg, dlgitem);
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
   return 1;
  }
 }
 return 0;
}

/***********************************************************************
 *                  CC_MouseCheckUserColorArray               [internal]
 *                  return 1 if the user clicked a color
 */
static int CC_MouseCheckUserColorArray( LCCPRIV lpp, HWND hDlg, int dlgitem, int rows, int cols,
	    LPARAM lParam )
{
 HWND hwnd;
 POINT point;
 RECT rect;
 int dx, dy, x, y;
 COLORREF *crarr = lpp->lpcc->lpCustColors;

 CONV_POINTSTOPOINT(&(MAKEPOINTS(lParam)), &point);
 ClientToScreen(hDlg, &point);
 hwnd = GetDlgItem(hDlg, dlgitem);
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
   return 1;
  }
 }
 return 0;
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
static int CC_MouseCheckColorGraph( HWND hDlg, int dlgitem, int *hori, int *vert, LPARAM lParam )
{
 HWND hwnd;
 POINT point;
 RECT rect;
 long x,y;

 CONV_POINTSTOPOINT(&(MAKEPOINTS(lParam)), &point);
 ClientToScreen(hDlg, &point);
 hwnd = GetDlgItem( hDlg, dlgitem );
 GetWindowRect(hwnd, &rect);
 if (PtInRect(&rect, point))
 {
  GetClientRect(hwnd, &rect);
  ScreenToClient(hwnd, &point);

  x = (long) point.x * MAXHORI;
  x /= rect.right;
  y = (long) (rect.bottom - point.y) * MAXVERT;
  y /= rect.bottom;

  if (hori)
   *hori = x;
  if (vert)
   *vert = y;
  return 1;
 }
 else
  return 0;
}
/***********************************************************************
 *                  CC_MouseCheckResultWindow                 [internal]
 *                  test if double click one of the result colors
 */
static int CC_MouseCheckResultWindow( HWND hDlg, LPARAM lParam )
{
 HWND hwnd;
 POINT point;
 RECT rect;

 CONV_POINTSTOPOINT(&(MAKEPOINTS(lParam)), &point);
 ClientToScreen(hDlg, &point);
 hwnd = GetDlgItem(hDlg, 0x2c5);
 GetWindowRect(hwnd, &rect);
 if (PtInRect(&rect, point))
 {
  PostMessageA(hDlg, WM_COMMAND, 0x2c9, 0);
  return 1;
 }
 return 0;
}

/***********************************************************************
 *                       CC_CheckDigitsInEdit                 [internal]
 */
static int CC_CheckDigitsInEdit( HWND hwnd, int maxval )
{
 int i, k, m, result, value;
 long editpos;
 char buffer[30];

 GetWindowTextA(hwnd, buffer, sizeof(buffer));
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
  sprintf(buffer, "%d", maxval);
  result = 2;
 }
 if (result)
 {
  editpos = SendMessageA(hwnd, EM_GETSEL, 0, 0);
  SetWindowTextA(hwnd, buffer );
  SendMessageA(hwnd, EM_SETSEL, 0, editpos);
 }
 return value;
}



/***********************************************************************
 *                    CC_PaintSelectedColor                   [internal]
 */
static void CC_PaintSelectedColor( HWND hDlg, COLORREF cr )
{
 RECT rect;
 HDC  hdc;
 HBRUSH hBrush;
 HWND hwnd = GetDlgItem(hDlg, 0x2c5);
 if (IsWindowVisible( GetDlgItem(hDlg, 0x2c6) ))   /* if full size */
 {
  hdc = GetDC(hwnd);
  GetClientRect(hwnd, &rect) ;
  hBrush = CreateSolidBrush(cr);
  if (hBrush)
  {
   hBrush = SelectObject(hdc, hBrush) ;
   Rectangle(hdc, rect.left, rect.top, rect.right/2, rect.bottom);
   DeleteObject ( SelectObject(hdc, hBrush) ) ;
   hBrush = CreateSolidBrush( GetNearestColor(hdc, cr) );
   if (hBrush)
   {
    hBrush = SelectObject(hdc, hBrush) ;
    Rectangle(hdc, rect.right/2-1, rect.top, rect.right, rect.bottom);
    DeleteObject(SelectObject(hdc, hBrush)) ;
   }
  }
  ReleaseDC(hwnd, hdc);
 }
}

/***********************************************************************
 *                    CC_PaintTriangle                        [internal]
 */
static void CC_PaintTriangle( HWND hDlg, int y)
{
 HDC hDC;
 long temp;
 int w = LOWORD(GetDialogBaseUnits());
 POINT points[3];
 int height;
 int oben;
 RECT rect;
 HWND hwnd = GetDlgItem(hDlg, 0x2be);
 LCCPRIV lpp = (LCCPRIV)GetWindowLongA( hDlg, DWL_USER); 

 if (IsWindowVisible( GetDlgItem(hDlg, 0x2c6)))   /* if full size */
 {
   GetClientRect(hwnd, &rect);
   height = rect.bottom;
   hDC = GetDC(hDlg);
   points[0].y = rect.top;
   points[0].x = rect.right;           /*  |  /|  */
   ClientToScreen(hwnd, points);    /*  | / |  */
   ScreenToClient(hDlg, points);    /*  |<  |  */
   oben = points[0].y;                 /*  | \ |  */
				     /*  |  \|  */
   temp = (long)height * (long)y;
   points[0].y = oben + height - temp / (long)MAXVERT;
   points[1].y = points[0].y + w;
   points[2].y = points[0].y - w;
   points[2].x = points[1].x = points[0].x + w;

   FillRect(hDC, &lpp->old3angle, GetClassLongA( hwnd, GCL_HBRBACKGROUND));
   lpp->old3angle.left  = points[0].x;
   lpp->old3angle.right = points[1].x + 1;
   lpp->old3angle.top   = points[2].y - 1;
   lpp->old3angle.bottom= points[1].y + 1;
   Polygon(hDC, points, 3);
   ReleaseDC(hDlg, hDC);
 }
}


/***********************************************************************
 *                    CC_PaintCross                           [internal]
 */
static void CC_PaintCross( HWND hDlg, int x, int y)
{
 HDC hDC;
 int w = GetDialogBaseUnits();
 HWND hwnd = GetDlgItem(hDlg, 0x2c6);
 LCCPRIV lpp = (LCCPRIV)GetWindowLongA( hDlg, DWL_USER ); 
 RECT rect;
 POINT point, p;
 HPEN hPen;

 if (IsWindowVisible( GetDlgItem(hDlg, 0x2c6) ))   /* if full size */
 {
   GetClientRect(hwnd, &rect);
   hDC = GetDC(hwnd);
   SelectClipRgn( hDC, CreateRectRgnIndirect(&rect));
   hPen = CreatePen(PS_SOLID, 2, 0xffffff); /* -white- color */
   hPen = SelectObject(hDC, hPen);
   point.x = ((long)rect.right * (long)x) / (long)MAXHORI;
   point.y = rect.bottom - ((long)rect.bottom * (long)y) / (long)MAXVERT;
   if ( lpp->oldcross.left != lpp->oldcross.right )
     BitBlt(hDC, lpp->oldcross.left, lpp->oldcross.top,
              lpp->oldcross.right - lpp->oldcross.left,
              lpp->oldcross.bottom - lpp->oldcross.top,
              lpp->hdcMem, lpp->oldcross.left, lpp->oldcross.top, SRCCOPY);
   lpp->oldcross.left   = point.x - w - 1;
   lpp->oldcross.right  = point.x + w + 1;
   lpp->oldcross.top    = point.y - w - 1;
   lpp->oldcross.bottom = point.y + w + 1; 

   MoveToEx(hDC, point.x - w, point.y, &p); 
   LineTo(hDC, point.x + w, point.y);
   MoveToEx(hDC, point.x, point.y - w, &p); 
   LineTo(hDC, point.x, point.y + w);
   DeleteObject( SelectObject(hDC, hPen)) ;
   ReleaseDC(hwnd, hDC);
 }
}


#define XSTEPS 48
#define YSTEPS 24


/***********************************************************************
 *                    CC_PrepareColorGraph                    [internal]
 */
static void CC_PrepareColorGraph( HWND hDlg )    
{
 int sdif, hdif, xdif, ydif, r, g, b, hue, sat;
 HWND hwnd = GetDlgItem(hDlg, 0x2c6);
 LCCPRIV lpp = (LCCPRIV)GetWindowLongA(hDlg, DWL_USER);  
 HBRUSH hbrush;
 HDC hdc ;
 RECT rect, client;
 HCURSOR hcursor = SetCursor( LoadCursorA(0, IDC_WAITA) );

 GetClientRect(hwnd, &client);
 hdc = GetDC(hwnd);
 lpp->hdcMem = CreateCompatibleDC(hdc);
 lpp->hbmMem = CreateCompatibleBitmap(hdc, client.right, client.bottom);
 SelectObject(lpp->hdcMem, lpp->hbmMem);

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
   r = CC_HSLtoRGB('R', hue, sat, 120);
   g = CC_HSLtoRGB('G', hue, sat, 120);
   b = CC_HSLtoRGB('B', hue, sat, 120);
   hbrush = CreateSolidBrush( RGB(r, g, b));
   FillRect(lpp->hdcMem, &rect, hbrush);
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
static void CC_PaintColorGraph( HWND hDlg )
{
 HWND hwnd = GetDlgItem( hDlg, 0x2c6 );
 LCCPRIV lpp = (LCCPRIV)GetWindowLongA(hDlg, DWL_USER); 
 HDC  hDC;
 RECT rect;
 if (IsWindowVisible(hwnd))   /* if full size */
 {
  if (!lpp->hdcMem)
   CC_PrepareColorGraph(hDlg);   /* should not be necessary */

  hDC = GetDC(hwnd);
  GetClientRect(hwnd, &rect);
  if (lpp->hdcMem)
      BitBlt(hDC, 0, 0, rect.right, rect.bottom, lpp->hdcMem, 0, 0, SRCCOPY);
  else
      WARN("choose color: hdcMem is not defined\n");
  ReleaseDC(hwnd, hDC);
 }
}

/***********************************************************************
 *                           CC_PaintLumBar                   [internal]
 */
static void CC_PaintLumBar( HWND hDlg, int hue, int sat )
{
 HWND hwnd = GetDlgItem(hDlg, 0x2be);
 RECT rect, client;
 int lum, ldif, ydif, r, g, b;
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
   r = CC_HSLtoRGB('R', hue, sat, lum);
   g = CC_HSLtoRGB('G', hue, sat, lum);
   b = CC_HSLtoRGB('B', hue, sat, lum);
   hbrush = CreateSolidBrush( RGB(r, g, b) );
   FillRect(hDC, &rect, hbrush);
   DeleteObject(hbrush);
   rect.bottom = rect.top;
  }
  GetClientRect(hwnd, &rect);
  FrameRect(hDC, &rect, GetStockObject(BLACK_BRUSH) );
  ReleaseDC(hwnd, hDC);
 }
}

/***********************************************************************
 *                             CC_EditSetRGB                  [internal]
 */
static void CC_EditSetRGB( HWND hDlg, COLORREF cr )
{
 char buffer[10];
 LCCPRIV lpp = (LCCPRIV)GetWindowLongA(hDlg, DWL_USER); 
 int r = GetRValue(cr);
 int g = GetGValue(cr);
 int b = GetBValue(cr);
 if (IsWindowVisible( GetDlgItem(hDlg, 0x2c6) ))   /* if full size */
 {
   lpp->updating = TRUE;
   sprintf(buffer, "%d", r);
   SetWindowTextA( GetDlgItem(hDlg, 0x2c2), buffer);
   sprintf(buffer, "%d", g);
   SetWindowTextA( GetDlgItem(hDlg, 0x2c3), buffer);
   sprintf( buffer, "%d", b );
   SetWindowTextA( GetDlgItem(hDlg, 0x2c4),buffer);
   lpp->updating = FALSE;
 }
}

/***********************************************************************
 *                             CC_EditSetHSL                  [internal]
 */
static void CC_EditSetHSL( HWND hDlg, int h, int s, int l )
{
 char buffer[10];
 LCCPRIV lpp = (LCCPRIV)GetWindowLongA(hDlg, DWL_USER);
 lpp->updating = TRUE;
 if (IsWindowVisible( GetDlgItem(hDlg, 0x2c6) ))   /* if full size */
 {
   lpp->updating = TRUE;
   sprintf(buffer, "%d", h);
   SetWindowTextA( GetDlgItem(hDlg, 0x2bf), buffer);
   sprintf(buffer, "%d", s);
   SetWindowTextA( GetDlgItem(hDlg, 0x2c0), buffer);
   sprintf(buffer, "%d", l);
   SetWindowTextA( GetDlgItem(hDlg, 0x2c1), buffer);
   lpp->updating = FALSE;
 }
 CC_PaintLumBar(hDlg, h, s);
}

/***********************************************************************
 *                       CC_SwitchToFullSize                  [internal]
 */
static void CC_SwitchToFullSize( HWND hDlg, COLORREF result, LPRECT lprect )
{
 int i;
 LCCPRIV lpp = (LCCPRIV)GetWindowLongA(hDlg, DWL_USER); 
 
 EnableWindow( GetDlgItem(hDlg, 0x2cf), FALSE);
 CC_PrepareColorGraph(hDlg);
 for (i = 0x2bf; i < 0x2c5; i++)
   ShowWindow( GetDlgItem(hDlg, i), SW_SHOW);
 for (i = 0x2d3; i < 0x2d9; i++)
   ShowWindow( GetDlgItem(hDlg, i), SW_SHOW);
 ShowWindow( GetDlgItem(hDlg, 0x2c9), SW_SHOW);
 ShowWindow( GetDlgItem(hDlg, 0x2c8), SW_SHOW);
 ShowWindow( GetDlgItem(hDlg, 1090), SW_SHOW);

 if (lprect)
  SetWindowPos(hDlg, 0, 0, 0, lprect->right-lprect->left,
   lprect->bottom-lprect->top, SWP_NOMOVE|SWP_NOZORDER);

 ShowWindow( GetDlgItem(hDlg, 0x2be), SW_SHOW);
 ShowWindow( GetDlgItem(hDlg, 0x2c5), SW_SHOW);

 CC_EditSetRGB(hDlg, result);
 CC_EditSetHSL(hDlg, lpp->h, lpp->s, lpp->l);
 ShowWindow( GetDlgItem( hDlg, 0x2c6), SW_SHOW);
 UpdateWindow( GetDlgItem(hDlg, 0x2c6) );
}

/***********************************************************************
 *                           CC_PaintPredefColorArray         [internal]
 *                Paints the default standard 48 colors
 */
static void CC_PaintPredefColorArray( HWND hDlg, int rows, int cols)
{
 HWND hwnd = GetDlgItem(hDlg, 0x2d0);
 RECT rect;
 HDC  hdc;
 HBRUSH hBrush;
 int dx, dy, i, j, k;
 LCCPRIV lpp = (LCCPRIV)GetWindowLongA(hDlg, DWL_USER);

 GetClientRect(hwnd, &rect);
 dx = rect.right / cols;
 dy = rect.bottom / rows;
 k = rect.left;

 hdc = GetDC(hwnd);
 GetClientRect(hwnd, &rect);
 FillRect(hdc, &rect, GetClassLongA(hwnd, GCL_HBRBACKGROUND));
 for ( j = 0; j < rows; j++ )
 {
  for ( i = 0; i < cols; i++ ) 
  {
   hBrush = CreateSolidBrush(predefcolors[j][i]);
   if (hBrush)
   {
    hBrush = SelectObject(hdc, hBrush);
    Rectangle(hdc, rect.left, rect.top,
                rect.left + dx - DISTANCE, rect.top + dy - DISTANCE);
    rect.left = rect.left + dx;
    DeleteObject(SelectObject(hdc, hBrush)) ;
   }
  }
  rect.top = rect.top + dy;
  rect.left = k;
 }
 ReleaseDC(hwnd, hdc);
 if (lpp->hwndFocus == hwnd)
   CC_DrawCurrentFocusRect(lpp);
}
/***********************************************************************
 *                             CC_PaintUserColorArray         [internal]
 *               Paint the 16 user-selected colors 
 */
static void CC_PaintUserColorArray( HWND hDlg, int rows, int cols, COLORREF* lpcr )
{
 HWND hwnd = GetDlgItem(hDlg, 0x2d1);
 RECT rect;
 HDC  hdc;
 HBRUSH hBrush;
 int dx, dy, i, j, k;
 LCCPRIV lpp = (LCCPRIV)GetWindowLongA(hDlg, DWL_USER);

 GetClientRect(hwnd, &rect);

 dx = rect.right / cols;
 dy = rect.bottom / rows;
 k = rect.left;

 hdc = GetDC(hwnd);
 if (hdc) 
 {
  FillRect(hdc, &rect, GetClassLongA(hwnd, GCL_HBRBACKGROUND) );
  for (j = 0; j < rows; j++)
  {
   for (i = 0; i < cols; i++)
   {
    hBrush = CreateSolidBrush(lpcr[i+j*cols]);
    if (hBrush)
    { 
     hBrush = SelectObject(hdc, hBrush) ;
     Rectangle(hdc, rect.left, rect.top,
                  rect.left + dx - DISTANCE, rect.top + dy - DISTANCE);
     rect.left = rect.left + dx;
     DeleteObject( SelectObject(hdc, hBrush) ) ;
    }
   }
   rect.top = rect.top + dy;
   rect.left = k;
  }
  ReleaseDC(hwnd, hdc);
 }
 if (lpp->hwndFocus == hwnd)
   CC_DrawCurrentFocusRect(lpp);
}



/***********************************************************************
 *                             CC_HookCallChk                 [internal]
 */
static BOOL CC_HookCallChk( LPCHOOSECOLORW lpcc )
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
static LONG CC_WMInitDialog( HWND hDlg, WPARAM wParam, LPARAM lParam, BOOL b16 ) 
{
   int i, res;
   int r, g, b;
   HWND hwnd;
   RECT rect;
   POINT point;
   LCCPRIV lpp; 
   
   TRACE("WM_INITDIALOG lParam=%08lX\n", lParam);
   lpp = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct CCPRIVATE) );
   if (b16)
   {
       CHOOSECOLORW *ch32;
       CHOOSECOLOR16 *ch16 = (CHOOSECOLOR16 *) lParam;
       ch32 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CHOOSECOLORW) );
       lpp->lpcc = ch32;
       lpp->lpcc16 = ch16;
       if (lpp->lpcc16->lStructSize != sizeof(CHOOSECOLOR16) )
       {
           EndDialog (hDlg, 0) ;
           return FALSE;
       }
       ch32->lStructSize = sizeof(CHOOSECOLORW);
       ch32->hwndOwner = ch16->hwndOwner; 
       ch32->hInstance = ch16->hInstance;
       ch32->lpCustColors = PTR_SEG_TO_LIN(ch16->lpCustColors);
       ch32->lpfnHook = (LPCCHOOKPROC) ch16->lpfnHook; /* only used as flag */
       ch32->Flags = ch16->Flags;
   }
   else
       lpp->lpcc = (LPCHOOSECOLORW) lParam;

   if (lpp->lpcc->lStructSize != sizeof(CHOOSECOLORW) )
   {
      EndDialog (hDlg, 0) ;
      return FALSE;
   }
   SetWindowLongA(hDlg, DWL_USER, (LONG)lpp); 

   if (!(lpp->lpcc->Flags & CC_SHOWHELP))
      ShowWindow( GetDlgItem(hDlg,0x40e), SW_HIDE);
   lpp->msetrgb = RegisterWindowMessageA(SETRGBSTRING);

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
      hwnd = GetDlgItem(hDlg, 0x2cf);
      EnableWindow(hwnd, FALSE);
   }
   if (!(lpp->lpcc->Flags & CC_FULLOPEN ) || lpp->lpcc->Flags & CC_PREVENTFULLOPEN)
   {
      rect = lpp->fullsize;
      res = rect.bottom - rect.top;
      hwnd = GetDlgItem(hDlg, 0x2c6); /* cut at left border */
      point.x = point.y = 0;
      ClientToScreen(hwnd, &point);
      ScreenToClient(hDlg,&point);
      GetClientRect(hDlg, &rect);
      point.x += GetSystemMetrics(SM_CXDLGFRAME);
      SetWindowPos(hDlg, 0, 0, 0, point.x, res, SWP_NOMOVE|SWP_NOZORDER);

      for (i = 0x2bf; i < 0x2c5; i++)
         ShowWindow( GetDlgItem(hDlg, i), SW_HIDE);
      for (i = 0x2d3; i < 0x2d9; i++)
         ShowWindow( GetDlgItem(hDlg, i), SW_HIDE);
      ShowWindow( GetDlgItem(hDlg, 0x2c9), SW_HIDE);
      ShowWindow( GetDlgItem(hDlg, 0x2c8), SW_HIDE);
      ShowWindow( GetDlgItem(hDlg, 0x2c6), SW_HIDE);
      ShowWindow( GetDlgItem(hDlg, 0x2c5), SW_HIDE);
      ShowWindow( GetDlgItem(hDlg, 1090 ), SW_HIDE);
   }
   else
      CC_SwitchToFullSize(hDlg, lpp->lpcc->rgbResult, NULL);
   res = TRUE;
   for (i = 0x2bf; i < 0x2c5; i++)
     SendMessageA( GetDlgItem(hDlg, i), EM_LIMITTEXT, 3, 0);  /* max 3 digits:  xyz  */
   if (CC_HookCallChk(lpp->lpcc))
   {
      if (b16)
          res = CallWindowProc16( (WNDPROC16)lpp->lpcc16->lpfnHook, hDlg, WM_INITDIALOG, wParam, lParam);
      else
          res = CallWindowProcA( (WNDPROC)lpp->lpcc->lpfnHook, hDlg, WM_INITDIALOG, wParam, lParam);
   }
   
   /* Set the initial values of the color chooser dialog */
   r = GetRValue(lpp->lpcc->rgbResult);
   g = GetGValue(lpp->lpcc->rgbResult);
   b = GetBValue(lpp->lpcc->rgbResult);

   CC_PaintSelectedColor(hDlg, lpp->lpcc->rgbResult);
   lpp->h = CC_RGBtoHSL('H', r, g, b);
   lpp->s = CC_RGBtoHSL('S', r, g, b);
   lpp->l = CC_RGBtoHSL('L', r, g, b);

   /* Doing it the long way becaus CC_EditSetRGB/HSL doesn'nt seem to work */
   SetDlgItemInt(hDlg, 703, lpp->h, TRUE);
   SetDlgItemInt(hDlg, 704, lpp->s, TRUE);
   SetDlgItemInt(hDlg, 705, lpp->l, TRUE);
   SetDlgItemInt(hDlg, 706, r, TRUE);
   SetDlgItemInt(hDlg, 707, g, TRUE);
   SetDlgItemInt(hDlg, 708, b, TRUE);

   CC_PaintCross(hDlg, lpp->h, lpp->s);
   CC_PaintTriangle(hDlg, lpp->l);

   return res;
}


/***********************************************************************
 *                              CC_WMCommand                  [internal]
 */
static LRESULT CC_WMCommand( HWND hDlg, WPARAM wParam, LPARAM lParam, WORD notifyCode, HWND hwndCtl ) 
{
    int  r, g, b, i, xx;
    UINT cokmsg;
    HDC hdc;
    COLORREF *cr;
    LCCPRIV lpp = (LCCPRIV)GetWindowLongA(hDlg, DWL_USER); 
    TRACE("CC_WMCommand wParam=%x lParam=%lx\n", wParam, lParam);
    switch (wParam)
    {
          case 0x2c2:  /* edit notify RGB */
	  case 0x2c3:
	  case 0x2c4:
	       if (notifyCode == EN_UPDATE && !lpp->updating)
			 {
			   i = CC_CheckDigitsInEdit(hwndCtl, 255);
			   r = GetRValue(lpp->lpcc->rgbResult);
			   g = GetGValue(lpp->lpcc->rgbResult);
			   b= GetBValue(lpp->lpcc->rgbResult);
			   xx = 0;
			   switch (wParam)
			   {
			    case 0x2c2: if ((xx = (i != r))) r = i; break;
			    case 0x2c3: if ((xx = (i != g))) g = i; break;
			    case 0x2c4: if ((xx = (i != b))) b = i; break;
			   }
			   if (xx) /* something has changed */
			   {
			    lpp->lpcc->rgbResult = RGB(r, g, b);
			    CC_PaintSelectedColor(hDlg, lpp->lpcc->rgbResult);
			    lpp->h = CC_RGBtoHSL('H', r, g, b);
			    lpp->s = CC_RGBtoHSL('S', r, g, b);
			    lpp->l = CC_RGBtoHSL('L', r, g, b);
			    CC_EditSetHSL(hDlg, lpp->h, lpp->s, lpp->l);
			    CC_PaintCross(hDlg, lpp->h, lpp->s);
			    CC_PaintTriangle(hDlg, lpp->l);
			   }
			 }
		 break;
		 
	  case 0x2bf:  /* edit notify HSL */
	  case 0x2c0:
	  case 0x2c1:
	       if (notifyCode == EN_UPDATE && !lpp->updating)
			 {
			   i = CC_CheckDigitsInEdit(hwndCtl , wParam == 0x2bf ? 239:240);
			   xx = 0;
			   switch (wParam)
			   {
			    case 0x2bf: if ((xx = ( i != lpp->h))) lpp->h = i; break;
			    case 0x2c0: if ((xx = ( i != lpp->s))) lpp->s = i; break;
			    case 0x2c1: if ((xx = ( i != lpp->l))) lpp->l = i; break;
			   }
			   if (xx) /* something has changed */
			   {
			    r = CC_HSLtoRGB('R', lpp->h, lpp->s, lpp->l);
			    g = CC_HSLtoRGB('G', lpp->h, lpp->s, lpp->l);
			    b = CC_HSLtoRGB('B', lpp->h, lpp->s, lpp->l);
			    lpp->lpcc->rgbResult = RGB(r, g, b);
			    CC_PaintSelectedColor(hDlg, lpp->lpcc->rgbResult);
			    CC_EditSetRGB(hDlg, lpp->lpcc->rgbResult);
			    CC_PaintCross(hDlg, lpp->h, lpp->s);
			    CC_PaintTriangle(hDlg, lpp->l);
			   }
			 }
	       break;
	       
          case 0x2cf:
               CC_SwitchToFullSize(hDlg, lpp->lpcc->rgbResult, &lpp->fullsize);
	       SetFocus( GetDlgItem(hDlg, 0x2bf));
	       break;

          case 0x2c8:    /* add colors ... column by column */
               cr = lpp->lpcc->lpCustColors;
               cr[(lpp->nextuserdef % 2) * 8 + lpp->nextuserdef / 2] = lpp->lpcc->rgbResult;
               if (++lpp->nextuserdef == 16)
		   lpp->nextuserdef = 0;
	       CC_PaintUserColorArray(hDlg, 2, 8, lpp->lpcc->lpCustColors);
	       break;

          case 0x2c9:              /* resulting color */
	       hdc = GetDC(hDlg);
	       lpp->lpcc->rgbResult = GetNearestColor(hdc, lpp->lpcc->rgbResult);
	       ReleaseDC(hDlg, hdc);
	       CC_EditSetRGB(hDlg, lpp->lpcc->rgbResult);
	       CC_PaintSelectedColor(hDlg, lpp->lpcc->rgbResult);
	       r = GetRValue(lpp->lpcc->rgbResult);
	       g = GetGValue(lpp->lpcc->rgbResult);
	       b = GetBValue(lpp->lpcc->rgbResult);
	       lpp->h = CC_RGBtoHSL('H', r, g, b);
	       lpp->s = CC_RGBtoHSL('S', r, g, b);
	       lpp->l = CC_RGBtoHSL('L', r, g, b);
	       CC_EditSetHSL(hDlg, lpp->h, lpp->s, lpp->l);
	       CC_PaintCross(hDlg, lpp->h, lpp->s);
	       CC_PaintTriangle(hDlg, lpp->l);
	       break;

	  case 0x40e:           /* Help! */ /* The Beatles, 1965  ;-) */
	       i = RegisterWindowMessageA(HELPMSGSTRING);
               if (lpp->lpcc16)
               {
                   if (lpp->lpcc->hwndOwner)
		       SendMessage16(lpp->lpcc->hwndOwner, i, 0, (LPARAM)lpp->lpcc16);
                   if ( CC_HookCallChk(lpp->lpcc))
		       CallWindowProc16( (WNDPROC16) lpp->lpcc16->lpfnHook, hDlg,
		          WM_COMMAND, psh15, (LPARAM)lpp->lpcc16);
               }
               else
               {
                   if (lpp->lpcc->hwndOwner)
		       SendMessageA(lpp->lpcc->hwndOwner, i, 0, (LPARAM)lpp->lpcc);
                   if ( CC_HookCallChk(lpp->lpcc))
		       CallWindowProcA( (WNDPROC) lpp->lpcc->lpfnHook, hDlg,
		          WM_COMMAND, psh15, (LPARAM)lpp->lpcc);
               }
	       break;

          case IDOK :
		cokmsg = RegisterWindowMessageA(COLOROKSTRING);
                if (lpp->lpcc16)
                {
		    if (lpp->lpcc->hwndOwner)
			if (SendMessage16(lpp->lpcc->hwndOwner, cokmsg, 0, (LPARAM)lpp->lpcc16))
			   break;    /* do NOT close */
                }
                else
                {
		    if (lpp->lpcc->hwndOwner)
			if (SendMessageA(lpp->lpcc->hwndOwner, cokmsg, 0, (LPARAM)lpp->lpcc))
			   break;    /* do NOT close */
                }
                if (lpp->lpcc16)
                {
                    BYTE *ptr = PTR_SEG_TO_LIN(lpp->lpcc16->lpCustColors);
                    memcpy(ptr, lpp->lpcc->lpCustColors, sizeof(COLORREF)*16);
                    lpp->lpcc16->rgbResult = lpp->lpcc->rgbResult;
                }
		EndDialog(hDlg, 1) ;
		return TRUE ;
	
	  case IDCANCEL :
		EndDialog(hDlg, 0) ;
		return TRUE ;

       }
       return FALSE;
}

/***********************************************************************
 *                              CC_WMPaint                    [internal]
 */
static LRESULT CC_WMPaint( HWND hDlg, WPARAM wParam, LPARAM lParam ) 
{
    HDC hdc;
    PAINTSTRUCT ps;
    LCCPRIV lpp = (LCCPRIV)GetWindowLongA(hDlg, DWL_USER); 

    hdc = BeginPaint(hDlg, &ps);
    /* we have to paint dialog children except text and buttons */
    CC_PaintPredefColorArray(hDlg, 6, 8);
    CC_PaintUserColorArray(hDlg, 2, 8, lpp->lpcc->lpCustColors);
    CC_PaintLumBar(hDlg, lpp->h, lpp->s);
    CC_PaintCross(hDlg, lpp->h, lpp->s);
    CC_PaintTriangle(hDlg, lpp->l);
    CC_PaintSelectedColor(hDlg, lpp->lpcc->rgbResult);
    CC_PaintColorGraph(hDlg);
    EndPaint(hDlg, &ps);

 return TRUE;
}


/***********************************************************************
 *                              CC_WMLButtonUp              [internal]
 */
static LRESULT CC_WMLButtonUp( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   LCCPRIV lpp = (LCCPRIV)GetWindowLongA(hDlg, DWL_USER);
   if (lpp->capturedGraph)
   {
       lpp->capturedGraph = 0;
       ReleaseCapture();
       CC_PaintCross(hDlg, lpp->h, lpp->s);
       return 1;
   } 
   return 0;
}


/***********************************************************************
 *                              CC_WMMouseMove              [internal]
 */
static LRESULT CC_WMMouseMove( HWND hDlg, LPARAM lParam ) 
{
   LCCPRIV lpp = (LCCPRIV)GetWindowLongA(hDlg, DWL_USER); 
   int r, g, b;

   if (lpp->capturedGraph)
   {
      int *ptrh = NULL, *ptrs = &lpp->l;
      if (lpp->capturedGraph == 0x2c6) 
      {
          ptrh = &lpp->h;
          ptrs = &lpp->s;
      }
      if (CC_MouseCheckColorGraph( hDlg, lpp->capturedGraph, ptrh, ptrs, lParam))
      {
          r = CC_HSLtoRGB('R', lpp->h, lpp->s, lpp->l);
          g = CC_HSLtoRGB('G', lpp->h, lpp->s, lpp->l);
          b = CC_HSLtoRGB('B', lpp->h, lpp->s, lpp->l);
          lpp->lpcc->rgbResult = RGB(r, g, b);
          CC_EditSetRGB(hDlg, lpp->lpcc->rgbResult);
          CC_EditSetHSL(hDlg,lpp->h, lpp->s, lpp->l);
          CC_PaintCross(hDlg, lpp->h, lpp->s);
          CC_PaintTriangle(hDlg, lpp->l);
          CC_PaintSelectedColor(hDlg, lpp->lpcc->rgbResult);
      }
      else
      {
          ReleaseCapture();
          lpp->capturedGraph = 0;
      }
      return 1;
   }
   return 0;
}
/***********************************************************************
 *                              CC_WMLButtonDown              [internal]
 */
static LRESULT CC_WMLButtonDown( HWND hDlg, WPARAM wParam, LPARAM lParam ) 
{
   LCCPRIV lpp = (LCCPRIV)GetWindowLongA(hDlg, DWL_USER); 
   int r, g, b, i;
   i = 0;

   if (CC_MouseCheckPredefColorArray(lpp, hDlg, 0x2d0, 6, 8, lParam))
      i = 1;
   else
      if (CC_MouseCheckUserColorArray(lpp, hDlg, 0x2d1, 2, 8, lParam))
         i = 1;
      else
	 if (CC_MouseCheckColorGraph(hDlg, 0x2c6, &lpp->h, &lpp->s, lParam))
         {
	    i = 2;
            lpp->capturedGraph = 0x2c6;
         }
	 else
	    if (CC_MouseCheckColorGraph(hDlg, 0x2be, NULL, &lpp->l, lParam))
            {
	       i = 2;
               lpp->capturedGraph = 0x2be;
            }
   if ( i == 2 )
   {
      SetCapture(hDlg);
      r = CC_HSLtoRGB('R', lpp->h, lpp->s, lpp->l);
      g = CC_HSLtoRGB('G', lpp->h, lpp->s, lpp->l);
      b = CC_HSLtoRGB('B', lpp->h, lpp->s, lpp->l);
      lpp->lpcc->rgbResult = RGB(r, g, b);
   }
   if ( i == 1 )
   {
      r = GetRValue(lpp->lpcc->rgbResult);
      g = GetGValue(lpp->lpcc->rgbResult);
      b = GetBValue(lpp->lpcc->rgbResult);
      lpp->h = CC_RGBtoHSL('H', r, g, b);
      lpp->s = CC_RGBtoHSL('S', r, g, b);
      lpp->l = CC_RGBtoHSL('L', r, g, b);
   }
   if (i)
   {
      CC_EditSetRGB(hDlg, lpp->lpcc->rgbResult);
      CC_EditSetHSL(hDlg,lpp->h, lpp->s, lpp->l);
      CC_PaintCross(hDlg, lpp->h, lpp->s);
      CC_PaintTriangle(hDlg, lpp->l);
      CC_PaintSelectedColor(hDlg, lpp->lpcc->rgbResult);
      return TRUE;
   }
   return FALSE;
}


/***********************************************************************
 *           ColorDlgProc32 [internal]
 *
 */
static LRESULT WINAPI ColorDlgProc( HWND hDlg, UINT message,
                                   WPARAM wParam, LPARAM lParam )
{

 int res;
 LCCPRIV lpp = (LCCPRIV)GetWindowLongA(hDlg, DWL_USER);
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
	                return CC_WMInitDialog(hDlg, wParam, lParam, FALSE);
	  case WM_NCDESTROY:
	                DeleteDC(lpp->hdcMem); 
	                DeleteObject(lpp->hbmMem); 
                        HeapFree(GetProcessHeap(), 0, lpp);
	                SetWindowLongA(hDlg, DWL_USER, 0L); /* we don't need it anymore */
	                break;
	  case WM_COMMAND:
	                if (CC_WMCommand( hDlg, wParam, lParam, HIWORD(wParam), (HWND) lParam))
	                   return TRUE;
	                break;     
	  case WM_PAINT:
	                if ( CC_WMPaint(hDlg, wParam, lParam))
	                   return TRUE;
	                break;
	  case WM_LBUTTONDBLCLK:
	                if (CC_MouseCheckResultWindow(hDlg, lParam))
			  return TRUE;
			break;
	  case WM_MOUSEMOVE:
	                if (CC_WMMouseMove(hDlg, lParam))
			  return TRUE;
			break;
	  case WM_LBUTTONUP:  /* FIXME: ClipCursor off (if in color graph)*/
                        if (CC_WMLButtonUp(hDlg, wParam, lParam))
                           return TRUE;
			break;
	  case WM_LBUTTONDOWN:/* FIXME: ClipCursor on  (if in color graph)*/
	                if (CC_WMLButtonDown(hDlg, wParam, lParam))
	                   return TRUE;
	                break;     
	}
     return FALSE ;
}

/***********************************************************************
 *           ColorDlgProc16   (COMMDLG.8)
 */
LRESULT WINAPI ColorDlgProc16( HWND16 hDlg, UINT16 message,
                            WPARAM16 wParam, LONG lParam )
{
 int res;
 LCCPRIV lpp = (LCCPRIV)GetWindowLongA(hDlg, DWL_USER); 
 if (message != WM_INITDIALOG)
 {
  if (!lpp)
     return FALSE;
  res=0;
  if (CC_HookCallChk(lpp->lpcc))
     res = CallWindowProc16( (WNDPROC16)lpp->lpcc16->lpfnHook, hDlg, message, wParam, lParam);
  if (res)
     return res;
 }

 /* FIXME: SetRGB message
 if (message && message == msetrgb)
    return HandleSetRGB(hDlg, lParam);
 */

 switch (message)
	{
	  case WM_INITDIALOG:
	                return CC_WMInitDialog(hDlg, wParam, lParam, TRUE);
	  case WM_NCDESTROY:
	                DeleteDC(lpp->hdcMem); 
	                DeleteObject(lpp->hbmMem); 
                        HeapFree(GetProcessHeap(), 0, lpp->lpcc);
                        HeapFree(GetProcessHeap(), 0, lpp);
	                SetWindowLongA(hDlg, DWL_USER, 0L); /* we don't need it anymore */
	                break;
	  case WM_COMMAND:
	                if (CC_WMCommand(hDlg, wParam, lParam, HIWORD(lParam), (HWND)LOWORD(lParam)))
	                   return TRUE;
	                break;     
	  case WM_PAINT:
	                if (CC_WMPaint(hDlg, wParam, lParam))
	                   return TRUE;
	                break;
	  case WM_LBUTTONDBLCLK:
	                if (CC_MouseCheckResultWindow(hDlg,lParam))
			  return TRUE;
			break;
	  case WM_MOUSEMOVE:
	                if (CC_WMMouseMove(hDlg, lParam))
			  return TRUE;
			break;
	  case WM_LBUTTONUP:  /* FIXME: ClipCursor off (if in color graph)*/
                        if (CC_WMLButtonUp(hDlg, wParam, lParam))
                           return TRUE;
			break;
	  case WM_LBUTTONDOWN:/* FIXME: ClipCursor on  (if in color graph)*/
	                if (CC_WMLButtonDown(hDlg, wParam, lParam))
	                   return TRUE;
	                break;     
	}
     return FALSE ;
}



/***********************************************************************
 *            ChooseColor16  (COMMDLG.5)
 */
BOOL16 WINAPI ChooseColor16( LPCHOOSECOLOR16 lpChCol )
{
    HINSTANCE16 hInst;
    HANDLE16 hDlgTmpl16 = 0, hResource16 = 0;
    HGLOBAL16 hGlobal16 = 0;
    BOOL16 bRet = FALSE;
    LPCVOID template;
    FARPROC16 ptr;
 
    TRACE("ChooseColor\n");
    if (!lpChCol) return FALSE;    

    if (lpChCol->Flags & CC_ENABLETEMPLATEHANDLE)
        hDlgTmpl16 = lpChCol->hInstance;
    else if (lpChCol->Flags & CC_ENABLETEMPLATE)
    {
        HANDLE16 hResInfo;
        if (!(hResInfo = FindResource16(lpChCol->hInstance,
                                        lpChCol->lpTemplateName,
                                        RT_DIALOG16)))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
            return FALSE;
        }
        if (!(hDlgTmpl16 = LoadResource16(lpChCol->hInstance, hResInfo)))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
            return FALSE;
        }
        hResource16 = hDlgTmpl16;
    }
    else
    {
	HANDLE hResInfo, hDlgTmpl32;
        LPCVOID template32;
        DWORD size;
	if (!(hResInfo = FindResourceA(COMMDLG_hInstance32, "CHOOSE_COLOR", RT_DIALOGA)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	    return FALSE;
	}
	if (!(hDlgTmpl32 = LoadResource(COMMDLG_hInstance32, hResInfo)) ||
	    !(template32 = LockResource(hDlgTmpl32)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
        size = SizeofResource(GetModuleHandleA("COMDLG32"), hResInfo);
        hGlobal16 = GlobalAlloc16(0, size);
        if (!hGlobal16)
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_MEMALLOCFAILURE);
            ERR("alloc failure for %ld bytes\n", size);
            return FALSE;
        }
        template = GlobalLock16(hGlobal16);
        if (!template) 
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_MEMLOCKFAILURE);
            ERR("global lock failure for %x handle\n", hDlgTmpl16);
            GlobalFree16(hGlobal16);
            return FALSE;
        }
        ConvertDialog32To16((LPVOID)template32, size, (LPVOID)template);
        hDlgTmpl16 = hGlobal16;
    }

    ptr = GetProcAddress16(GetModuleHandle16("COMMDLG"), (SEGPTR) 8);
    hInst = GetWindowLongA(lpChCol->hwndOwner, GWL_HINSTANCE);
    bRet = DialogBoxIndirectParam16(hInst, hDlgTmpl16, lpChCol->hwndOwner,
                     (DLGPROC16) ptr, (DWORD)lpChCol);
    if (hResource16) FreeResource16(hDlgTmpl16);
    if (hGlobal16)
    {
        GlobalUnlock16(hGlobal16);
        GlobalFree16(hGlobal16);
    }
    return bRet;
}

/***********************************************************************
 *            ChooseColorW  (COMDLG32.2)
 */
BOOL WINAPI ChooseColorW( LPCHOOSECOLORW lpChCol )

{

    HANDLE hDlgTmpl = 0;
    BOOL bRet = FALSE;
    LPCVOID template;

    TRACE("ChooseColor\n");
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
        HANDLE hResInfo;
        if (!(hResInfo = FindResourceW(lpChCol->hInstance,
                                        lpChCol->lpTemplateName,
                                        RT_DIALOGW)))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
            return FALSE;
        }
        if (!(hDlgTmpl = LoadResource(lpChCol->hInstance, hResInfo)) ||
            !(template = LockResource(hDlgTmpl)))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
            return FALSE;
        }
    }
    else
    {
	HANDLE hResInfo, hDlgTmpl;
	if (!(hResInfo = FindResourceA(COMMDLG_hInstance32, "CHOOSE_COLOR", RT_DIALOGA)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	    return FALSE;
	}
	if (!(hDlgTmpl = LoadResource(COMMDLG_hInstance32, hResInfo )) ||
	    !(template = LockResource(hDlgTmpl)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
    }

    bRet = DialogBoxIndirectParamW(COMMDLG_hInstance32, template, lpChCol->hwndOwner,
                     (DLGPROC)ColorDlgProc, (DWORD)lpChCol);
    return bRet;
}

/***********************************************************************
 *            ChooseColorA  (COMDLG32.1)
 */
BOOL WINAPI ChooseColorA( LPCHOOSECOLORA lpChCol )

{
  BOOL ret;
  LPCHOOSECOLORW lpcc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CHOOSECOLORW));
  lpcc->lStructSize = sizeof(*lpcc);
  lpcc->hwndOwner = lpChCol->hwndOwner;
  lpcc->hInstance = lpChCol->hInstance;
  lpcc->rgbResult = lpChCol->rgbResult;
  lpcc->lpCustColors = lpChCol->lpCustColors;
  lpcc->Flags = lpChCol->Flags;
  lpcc->lCustData = lpChCol->lCustData;
  lpcc->lpfnHook = (LPCCHOOKPROC) lpChCol->lpfnHook;
  if ((lpcc->Flags & CC_ENABLETEMPLATE) && (lpChCol->lpTemplateName)) {
      if (HIWORD(lpChCol->lpTemplateName))
	  lpcc->lpTemplateName = HEAP_strdupAtoW(GetProcessHeap(), 0, lpChCol->lpTemplateName);
      else
	  lpcc->lpTemplateName = (LPWSTR)lpChCol->lpTemplateName;
  }

  ret = ChooseColorW(lpcc);

  if (ret)
      lpChCol->rgbResult = lpcc->rgbResult;
  if (HIWORD(lpcc->lpTemplateName)) HeapFree(GetProcessHeap(), 0, (LPSTR)lpcc->lpTemplateName);
  HeapFree(GetProcessHeap(), 0, lpcc);
  return ret;
}
