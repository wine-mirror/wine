/*
 * COMMDLG - File Dialog
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "ldt.h"
#include "heap.h"
#include "commdlg.h"
#include "resource.h"
#include "dialog.h"
#include "dlgs.h"
#include "module.h"
#include "debugtools.h"
#include "winproc.h"
#include "cderr.h"

DEFAULT_DEBUG_CHANNEL(commdlg)

#include "cdlg.h"

/***********************************************************************
 *           ColorDlgProc [internal]
 *
 * FIXME: Convert to real 32-bit message processing
 */
static LRESULT WINAPI ColorDlgProc(HWND hDlg, UINT msg,
                                   WPARAM wParam, LPARAM lParam)
{
    UINT16 msg16;
    MSGPARAM16 mp16;

    mp16.lParam = lParam;
    if (WINPROC_MapMsg32ATo16( hDlg, msg, wParam,
                               &msg16, &mp16.wParam, &mp16.lParam ) == -1)
        return 0;
    mp16.lResult = ColorDlgProc16( (HWND16)hDlg, msg16, mp16.wParam, mp16.lParam );

    WINPROC_UnmapMsg32ATo16( hDlg, msg, wParam, lParam, &mp16 );
    return mp16.lResult;
}

/***********************************************************************
 *           ChooseColor   (COMMDLG.5)
 */
BOOL16 WINAPI ChooseColor16(LPCHOOSECOLOR16 lpChCol)
{
    HINSTANCE16 hInst;
    HANDLE16 hDlgTmpl = 0;
    BOOL16 bRet = FALSE, win32Format = FALSE;
    LPCVOID template;
    HWND hwndDialog;

    TRACE("ChooseColor\n");
    if (!lpChCol) return FALSE;    

    if (lpChCol->Flags & CC_ENABLETEMPLATEHANDLE)
    {
        if (!(template = LockResource16( lpChCol->hInstance )))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
            return FALSE;
        }
    }
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
        if (!(hDlgTmpl = LoadResource16( lpChCol->hInstance, hResInfo )) ||
            !(template = LockResource16( hDlgTmpl )))
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
	    !(template = LockResource( hDlgTmpl )))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
        win32Format = TRUE;
    }

    hInst = GetWindowLongA( lpChCol->hwndOwner, GWL_HINSTANCE );
    hwndDialog = DIALOG_CreateIndirect( hInst, template, win32Format,
                                        lpChCol->hwndOwner,
                                        (DLGPROC16)ColorDlgProc,
                                        (DWORD)lpChCol, WIN_PROC_32A );
    if (hwndDialog) bRet = DIALOG_DoDialogBox( hwndDialog, lpChCol->hwndOwner);
    if (hDlgTmpl) FreeResource16( hDlgTmpl );

    return bRet;
}


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
 LPCHOOSECOLOR16 lpcc;  /* points to public known data structure */
 int nextuserdef;     /* next free place in user defined color array */
 HDC16 hdcMem;        /* color graph used for BitBlt() */
 HBITMAP16 hbmMem;    /* color graph bitmap */    
 RECT16 fullsize;     /* original dialog window size */
 UINT16 msetrgb;        /* # of SETRGBSTRING message (today not used)  */
 RECT16 old3angle;    /* last position of l-marker */
 RECT16 oldcross;     /* last position of color/satuation marker */
 BOOL updating;     /* to prevent recursive WM_COMMAND/EN_UPDATE procesing */
 int h;
 int s;
 int l;               /* for temporary storing of hue,sat,lum */
};

/***********************************************************************
 *                             CC_HSLtoRGB                    [internal]
 */
static int CC_HSLtoRGB(char c,int hue,int sat,int lum)
{
 int res=0,maxrgb;

 /* hue */
 switch(c)
 {
  case 'R':if (hue>80)  hue-=80;  else hue+=160; break;
  case 'G':if (hue>160) hue-=160; else hue+=80;  break;
  case 'B':break;
 }

 /* l below 120 */
 maxrgb=(256*MIN(120,lum))/120;  /* 0 .. 256 */
 if (hue< 80)
  res=0;
 else
  if (hue< 120)
  {
   res=(hue-80)* maxrgb;           /* 0...10240 */
   res/=40;                        /* 0...256 */
  }
  else
   if (hue< 200)
    res=maxrgb;
   else
    {
     res=(240-hue)* maxrgb;
     res/=40;
    }
 res=res-maxrgb/2;                 /* -128...128 */

 /* saturation */
 res=maxrgb/2 + (sat*res) /240;    /* 0..256 */

 /* lum above 120 */
 if (lum>120 && res<256)
  res+=((lum-120) * (256-res))/120;

 return MIN(res,255);
}

/***********************************************************************
 *                             CC_RGBtoHSL                    [internal]
 */
static int CC_RGBtoHSL(char c,int r,int g,int b)
{
 WORD maxi,mini,mmsum,mmdif,result=0;
 int iresult=0;

 maxi=MAX(r,b);
 maxi=MAX(maxi,g);
 mini=MIN(r,b);
 mini=MIN(mini,g);

 mmsum=maxi+mini;
 mmdif=maxi-mini;

 switch(c)
 {
  /* lum */
  case 'L':mmsum*=120;              /* 0...61200=(255+255)*120 */
	   result=mmsum/255;        /* 0...240 */
	   break;
  /* saturation */
  case 'S':if (!mmsum)
	    result=0;
	   else
	    if (!mini || maxi==255)
	     result=240;
	   else
	   {
	    result=mmdif*240;       /* 0...61200=255*240 */
	    result/= (mmsum>255 ? mmsum=510-mmsum : mmsum); /* 0..255 */
	   }
	   break;
  /* hue */
  case 'H':if (!mmdif)
	    result=160;
	   else
	   {
	    if (maxi==r)
	    {
	     iresult=40*(g-b);       /* -10200 ... 10200 */
	     iresult/=(int)mmdif;    /* -40 .. 40 */
	     if (iresult<0)
	      iresult+=240;          /* 0..40 and 200..240 */
	    }
	    else
	     if (maxi==g)
	     {
	      iresult=40*(b-r);
	      iresult/=(int)mmdif;
	      iresult+=80;           /* 40 .. 120 */
	     }
	     else
	      if (maxi==b)
	      {
	       iresult=40*(r-g);
	       iresult/=(int)mmdif;
	       iresult+=160;         /* 120 .. 200 */
	      }
	    result=iresult;
	   }
	   break;
 }
 return result;    /* is this integer arithmetic precise enough ? */
}

#define DISTANCE 4

/***********************************************************************
 *                CC_MouseCheckPredefColorArray               [internal]
 */
static int CC_MouseCheckPredefColorArray(HWND16 hDlg,int dlgitem,int rows,int cols,
	    LPARAM lParam,COLORREF *cr)
{
 HWND16 hwnd;
 POINT16 point = MAKEPOINT16(lParam);
 RECT16 rect;
 int dx,dy,x,y;

 ClientToScreen16(hDlg,&point);
 hwnd=GetDlgItem(hDlg,dlgitem);
 GetWindowRect16(hwnd,&rect);
 if (PtInRect16(&rect,point))
 {
  dx=(rect.right-rect.left)/cols;
  dy=(rect.bottom-rect.top)/rows;
  ScreenToClient16(hwnd,&point);

  if (point.x % dx < (dx-DISTANCE) && point.y % dy < (dy-DISTANCE))
  {
   x=point.x/dx;
   y=point.y/dy;
   *cr=predefcolors[y][x];
   /* FIXME: Draw_a_Focus_Rect() */
   return 1;
  }
 }
 return 0;
}

/***********************************************************************
 *                  CC_MouseCheckUserColorArray               [internal]
 */
static int CC_MouseCheckUserColorArray(HWND16 hDlg,int dlgitem,int rows,int cols,
	    LPARAM lParam,COLORREF *cr,COLORREF*crarr)
{
 HWND16 hwnd;
 POINT16 point = MAKEPOINT16(lParam);
 RECT16 rect;
 int dx,dy,x,y;

 ClientToScreen16(hDlg,&point);
 hwnd=GetDlgItem(hDlg,dlgitem);
 GetWindowRect16(hwnd,&rect);
 if (PtInRect16(&rect,point))
 {
  dx=(rect.right-rect.left)/cols;
  dy=(rect.bottom-rect.top)/rows;
  ScreenToClient16(hwnd,&point);

  if (point.x % dx < (dx-DISTANCE) && point.y % dy < (dy-DISTANCE))
  {
   x=point.x/dx;
   y=point.y/dy;
   *cr=crarr[x+cols*y];
   /* FIXME: Draw_a_Focus_Rect() */
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
static int CC_MouseCheckColorGraph(HWND16 hDlg,int dlgitem,int *hori,int *vert,LPARAM lParam)
{
 HWND hwnd;
 POINT16 point = MAKEPOINT16(lParam);
 RECT16 rect;
 long x,y;

 ClientToScreen16(hDlg,&point);
 hwnd=GetDlgItem(hDlg,dlgitem);
 GetWindowRect16(hwnd,&rect);
 if (PtInRect16(&rect,point))
 {
  GetClientRect16(hwnd,&rect);
  ScreenToClient16(hwnd,&point);

  x=(long)point.x*MAXHORI;
  x/=rect.right;
  y=(long)(rect.bottom-point.y)*MAXVERT;
  y/=rect.bottom;

  if (hori)
   *hori=x;
  if (vert)
   *vert=y;
  return 1;
 }
 else
  return 0;
}
/***********************************************************************
 *                  CC_MouseCheckResultWindow                 [internal]
 */
static int CC_MouseCheckResultWindow(HWND16 hDlg,LPARAM lParam)
{
 HWND16 hwnd;
 POINT16 point = MAKEPOINT16(lParam);
 RECT16 rect;

 ClientToScreen16(hDlg,&point);
 hwnd=GetDlgItem(hDlg,0x2c5);
 GetWindowRect16(hwnd,&rect);
 if (PtInRect16(&rect,point))
 {
  PostMessage16(hDlg,WM_COMMAND,0x2c9,0);
  return 1;
 }
 return 0;
}

/***********************************************************************
 *                       CC_CheckDigitsInEdit                 [internal]
 */
static int CC_CheckDigitsInEdit(HWND16 hwnd,int maxval)
{
 int i,k,m,result,value;
 long editpos;
 char buffer[30];
 GetWindowTextA(hwnd,buffer,sizeof(buffer));
 m=strlen(buffer);
 result=0;

 for (i=0;i<m;i++)
  if (buffer[i]<'0' || buffer[i]>'9')
  {
   for (k=i+1;k<=m;k++)   /* delete bad character */
   {
    buffer[i]=buffer[k];
    m--;
   }
   buffer[m]=0;
   result=1;
  }

 value=atoi(buffer);
 if (value>maxval)       /* build a new string */
 {
  sprintf(buffer,"%d",maxval);
  result=2;
 }
 if (result)
 {
  editpos=SendMessage16(hwnd,EM_GETSEL16,0,0);
  SetWindowTextA(hwnd,buffer);
  SendMessage16(hwnd,EM_SETSEL16,0,editpos);
 }
 return value;
}



/***********************************************************************
 *                    CC_PaintSelectedColor                   [internal]
 */
static void CC_PaintSelectedColor(HWND16 hDlg,COLORREF cr)
{
 RECT16 rect;
 HDC  hdc;
 HBRUSH hBrush;
 HWND hwnd=GetDlgItem(hDlg,0x2c5);
 if (IsWindowVisible(GetDlgItem(hDlg,0x2c6)))   /* if full size */
 {
  hdc=GetDC(hwnd);
  GetClientRect16 (hwnd, &rect) ;
  hBrush = CreateSolidBrush(cr);
  if (hBrush)
  {
   hBrush = SelectObject (hdc, hBrush) ;
   Rectangle(hdc, rect.left,rect.top,rect.right/2,rect.bottom);
   DeleteObject (SelectObject (hdc,hBrush)) ;
   hBrush=CreateSolidBrush(GetNearestColor(hdc,cr));
   if (hBrush)
   {
    hBrush= SelectObject (hdc, hBrush) ;
    Rectangle( hdc, rect.right/2-1,rect.top,rect.right,rect.bottom);
    DeleteObject( SelectObject (hdc, hBrush)) ;
   }
  }
  ReleaseDC(hwnd,hdc);
 }
}

/***********************************************************************
 *                    CC_PaintTriangle                        [internal]
 */
static void CC_PaintTriangle(HWND16 hDlg,int y)
{
 HDC hDC;
 long temp;
 int w=GetDialogBaseUnits();
 POINT16 points[3];
 int height;
 int oben;
 RECT16 rect;
 HWND16 hwnd=GetDlgItem(hDlg,0x2be);
 struct CCPRIVATE *lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 

 if (IsWindowVisible(GetDlgItem(hDlg,0x2c6)))   /* if full size */
 {
   GetClientRect16(hwnd,&rect);
   height=rect.bottom;
   hDC=GetDC(hDlg);

   points[0].y=rect.top;
   points[0].x=rect.right;           /*  |  /|  */
   ClientToScreen16(hwnd,points);    /*  | / |  */
   ScreenToClient16(hDlg,points);    /*  |<  |  */
   oben=points[0].y;                 /*  | \ |  */
				     /*  |  \|  */
   temp=(long)height*(long)y;
   points[0].y=oben+height -temp/(long)MAXVERT;
   points[1].y=points[0].y+w;
   points[2].y=points[0].y-w;
   points[2].x=points[1].x=points[0].x + w;

   if (lpp->old3angle.left)
    FillRect16(hDC,&lpp->old3angle,GetStockObject(WHITE_BRUSH));
   lpp->old3angle.left  =points[0].x;
   lpp->old3angle.right =points[1].x+1;
   lpp->old3angle.top   =points[2].y-1;
   lpp->old3angle.bottom=points[1].y+1;
   Polygon16(hDC,points,3);
   ReleaseDC(hDlg,hDC);
 }
}


/***********************************************************************
 *                    CC_PaintCross                           [internal]
 */
static void CC_PaintCross(HWND16 hDlg,int x,int y)
{
 HDC hDC;
 int w=GetDialogBaseUnits();
 HWND16 hwnd=GetDlgItem(hDlg,0x2c6);
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 
 RECT16 rect;
 POINT16 point;
 HPEN hPen;

 if (IsWindowVisible(GetDlgItem(hDlg,0x2c6)))   /* if full size */
 {
   GetClientRect16(hwnd,&rect);
   hDC=GetDC(hwnd);
   SelectClipRgn(hDC,CreateRectRgnIndirect16(&rect));
   hPen=CreatePen(PS_SOLID,2,0);
   hPen=SelectObject(hDC,hPen);
   point.x=((long)rect.right*(long)x)/(long)MAXHORI;
   point.y=rect.bottom-((long)rect.bottom*(long)y)/(long)MAXVERT;
   if (lpp->oldcross.left!=lpp->oldcross.right)
     BitBlt(hDC,lpp->oldcross.left,lpp->oldcross.top,
              lpp->oldcross.right-lpp->oldcross.left,
              lpp->oldcross.bottom-lpp->oldcross.top,
              lpp->hdcMem,lpp->oldcross.left,lpp->oldcross.top,SRCCOPY);
   lpp->oldcross.left  =point.x-w-1;
   lpp->oldcross.right =point.x+w+1;
   lpp->oldcross.top   =point.y-w-1;
   lpp->oldcross.bottom=point.y+w+1; 

   MoveTo16(hDC,point.x-w,point.y); 
   LineTo(hDC,point.x+w,point.y);
   MoveTo16(hDC,point.x,point.y-w); 
   LineTo(hDC,point.x,point.y+w);
   DeleteObject(SelectObject(hDC,hPen));
   ReleaseDC(hwnd,hDC);
 }
}


#define XSTEPS 48
#define YSTEPS 24


/***********************************************************************
 *                    CC_PrepareColorGraph                    [internal]
 */
static void CC_PrepareColorGraph(HWND16 hDlg)    
{
 int sdif,hdif,xdif,ydif,r,g,b,hue,sat;
 HWND hwnd=GetDlgItem(hDlg,0x2c6);
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER);  
 HBRUSH hbrush;
 HDC hdc ;
 RECT16 rect,client;
 HCURSOR16 hcursor=SetCursor16(LoadCursor16(0,IDC_WAIT16));

 GetClientRect16(hwnd,&client);
 hdc=GetDC(hwnd);
 lpp->hdcMem = CreateCompatibleDC(hdc);
 lpp->hbmMem = CreateCompatibleBitmap(hdc,client.right,client.bottom);
 SelectObject(lpp->hdcMem,lpp->hbmMem);

 xdif=client.right /XSTEPS;
 ydif=client.bottom/YSTEPS+1;
 hdif=239/XSTEPS;
 sdif=240/YSTEPS;
 for(rect.left=hue=0;hue<239+hdif;hue+=hdif)
 {
  rect.right=rect.left+xdif;
  rect.bottom=client.bottom;
  for(sat=0;sat<240+sdif;sat+=sdif)
  {
   rect.top=rect.bottom-ydif;
   r=CC_HSLtoRGB('R',hue,sat,120);
   g=CC_HSLtoRGB('G',hue,sat,120);
   b=CC_HSLtoRGB('B',hue,sat,120);
   hbrush=CreateSolidBrush(RGB(r,g,b));
   FillRect16(lpp->hdcMem,&rect,hbrush);
   DeleteObject(hbrush);
   rect.bottom=rect.top;
  }
  rect.left=rect.right;
 }
 ReleaseDC(hwnd,hdc);
 SetCursor16(hcursor);
}

/***********************************************************************
 *                          CC_PaintColorGraph                [internal]
 */
static void CC_PaintColorGraph(HWND16 hDlg)
{
 HWND hwnd=GetDlgItem(hDlg,0x2c6);
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 
 HDC  hDC;
 RECT16 rect;
 if (IsWindowVisible(hwnd))   /* if full size */
 {
  if (!lpp->hdcMem)
   CC_PrepareColorGraph(hDlg);   /* should not be necessary */

  hDC=GetDC(hwnd);
  GetClientRect16(hwnd,&rect);
  if (lpp->hdcMem)
      BitBlt(hDC,0,0,rect.right,rect.bottom,lpp->hdcMem,0,0,SRCCOPY);
  else
    WARN("choose color: hdcMem is not defined\n");
  ReleaseDC(hwnd,hDC);
 }
}
/***********************************************************************
 *                           CC_PaintLumBar                   [internal]
 */
static void CC_PaintLumBar(HWND16 hDlg,int hue,int sat)
{
 HWND hwnd=GetDlgItem(hDlg,0x2be);
 RECT16 rect,client;
 int lum,ldif,ydif,r,g,b;
 HBRUSH hbrush;
 HDC hDC;

 if (IsWindowVisible(hwnd))
 {
  hDC=GetDC(hwnd);
  GetClientRect16(hwnd,&client);
  rect=client;

  ldif=240/YSTEPS;
  ydif=client.bottom/YSTEPS+1;
  for(lum=0;lum<240+ldif;lum+=ldif)
  {
   rect.top=MAX(0,rect.bottom-ydif);
   r=CC_HSLtoRGB('R',hue,sat,lum);
   g=CC_HSLtoRGB('G',hue,sat,lum);
   b=CC_HSLtoRGB('B',hue,sat,lum);
   hbrush=CreateSolidBrush(RGB(r,g,b));
   FillRect16(hDC,&rect,hbrush);
   DeleteObject(hbrush);
   rect.bottom=rect.top;
  }
  GetClientRect16(hwnd,&rect);
  FrameRect16(hDC,&rect,GetStockObject(BLACK_BRUSH));
  ReleaseDC(hwnd,hDC);
 }
}

/***********************************************************************
 *                             CC_EditSetRGB                  [internal]
 */
static void CC_EditSetRGB(HWND16 hDlg,COLORREF cr)
{
 char buffer[10];
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 
 int r=GetRValue(cr);
 int g=GetGValue(cr);
 int b=GetBValue(cr);
 if (IsWindowVisible(GetDlgItem(hDlg,0x2c6)))   /* if full size */
 {
   lpp->updating=TRUE;
   sprintf(buffer,"%d",r);
   SetWindowTextA(GetDlgItem(hDlg,0x2c2),buffer);
   sprintf(buffer,"%d",g);
   SetWindowTextA(GetDlgItem(hDlg,0x2c3),buffer);
   sprintf(buffer,"%d",b);
   SetWindowTextA(GetDlgItem(hDlg,0x2c4),buffer);
   lpp->updating=FALSE;
 }
}

/***********************************************************************
 *                             CC_EditSetHSL                  [internal]
 */
static void CC_EditSetHSL(HWND16 hDlg,int h,int s,int l)
{
 char buffer[10];
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 
 lpp->updating=TRUE;
 if (IsWindowVisible(GetDlgItem(hDlg,0x2c6)))   /* if full size */
 {
   lpp->updating=TRUE;
   sprintf(buffer,"%d",h);
   SetWindowTextA(GetDlgItem(hDlg,0x2bf),buffer);
   sprintf(buffer,"%d",s);
   SetWindowTextA(GetDlgItem(hDlg,0x2c0),buffer);
   sprintf(buffer,"%d",l);
   SetWindowTextA(GetDlgItem(hDlg,0x2c1),buffer);
   lpp->updating=FALSE;
 }
 CC_PaintLumBar(hDlg,h,s);
}

/***********************************************************************
 *                       CC_SwitchToFullSize                  [internal]
 */
static void CC_SwitchToFullSize(HWND16 hDlg,COLORREF result,LPRECT16 lprect)
{
 int i;
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 
 
 EnableWindow(GetDlgItem(hDlg,0x2cf),FALSE);
 CC_PrepareColorGraph(hDlg);
 for (i=0x2bf;i<0x2c5;i++)
   EnableWindow(GetDlgItem(hDlg,i),TRUE);
 for (i=0x2d3;i<0x2d9;i++)
   EnableWindow(GetDlgItem(hDlg,i),TRUE);
 EnableWindow(GetDlgItem(hDlg,0x2c9),TRUE);
 EnableWindow(GetDlgItem(hDlg,0x2c8),TRUE);

 if (lprect)
  SetWindowPos(hDlg,0,0,0,lprect->right-lprect->left,
   lprect->bottom-lprect->top, SWP_NOMOVE|SWP_NOZORDER);

 ShowWindow(GetDlgItem(hDlg,0x2c6),SW_SHOW);
 ShowWindow(GetDlgItem(hDlg,0x2be),SW_SHOW);
 ShowWindow(GetDlgItem(hDlg,0x2c5),SW_SHOW);

 CC_EditSetRGB(hDlg,result);
 CC_EditSetHSL(hDlg,lpp->h,lpp->s,lpp->l);
}

/***********************************************************************
 *                           CC_PaintPredefColorArray         [internal]
 */
static void CC_PaintPredefColorArray(HWND16 hDlg,int rows,int cols)
{
 HWND hwnd=GetDlgItem(hDlg,0x2d0);
 RECT16 rect;
 HDC  hdc;
 HBRUSH hBrush;
 int dx,dy,i,j,k;

 GetClientRect16(hwnd,&rect);
 dx=rect.right/cols;
 dy=rect.bottom/rows;
 k=rect.left;

 hdc=GetDC(hwnd);
 GetClientRect16 (hwnd, &rect) ;

 for (j=0;j<rows;j++)
 {
  for (i=0;i<cols;i++)
  {
   hBrush = CreateSolidBrush(predefcolors[j][i]);
   if (hBrush)
   {
    hBrush = SelectObject (hdc, hBrush) ;
    Rectangle(hdc, rect.left, rect.top,
                rect.left+dx-DISTANCE, rect.top+dy-DISTANCE);
    rect.left=rect.left+dx;
    DeleteObject( SelectObject (hdc, hBrush)) ;
   }
  }
  rect.top=rect.top+dy;
  rect.left=k;
 }
 ReleaseDC(hwnd,hdc);
 /* FIXME: draw_a_focus_rect */
}
/***********************************************************************
 *                             CC_PaintUserColorArray         [internal]
 */
static void CC_PaintUserColorArray(HWND16 hDlg,int rows,int cols,COLORREF* lpcr)
{
 HWND hwnd=GetDlgItem(hDlg,0x2d1);
 RECT16 rect;
 HDC  hdc;
 HBRUSH hBrush;
 int dx,dy,i,j,k;

 GetClientRect16(hwnd,&rect);

 dx=rect.right/cols;
 dy=rect.bottom/rows;
 k=rect.left;

 hdc=GetDC(hwnd);
 if (hdc)
 {
  for (j=0;j<rows;j++)
  {
   for (i=0;i<cols;i++)
   {
    hBrush = CreateSolidBrush(lpcr[i+j*cols]);
    if (hBrush)
    {
     hBrush = SelectObject (hdc, hBrush) ;
     Rectangle( hdc, rect.left, rect.top,
                  rect.left+dx-DISTANCE, rect.top+dy-DISTANCE);
     rect.left=rect.left+dx;
     DeleteObject( SelectObject (hdc, hBrush)) ;
    }
   }
   rect.top=rect.top+dy;
   rect.left=k;
  }
  ReleaseDC(hwnd,hdc);
 }
 /* FIXME: draw_a_focus_rect */
}



/***********************************************************************
 *                             CC_HookCallChk                 [internal]
 */
static BOOL CC_HookCallChk(LPCHOOSECOLOR16 lpcc)
{
 if (lpcc)
  if(lpcc->Flags & CC_ENABLEHOOK)
   if (lpcc->lpfnHook)
    return TRUE;
 return FALSE;
}

/***********************************************************************
 *                            CC_WMInitDialog                 [internal]
 */
static LONG CC_WMInitDialog(HWND16 hDlg, WPARAM16 wParam, LPARAM lParam) 
{
   int i,res;
   int r, g, b;
   HWND16 hwnd;
   RECT16 rect;
   POINT16 point;
   struct CCPRIVATE * lpp; 
   
   TRACE("WM_INITDIALOG lParam=%08lX\n", lParam);
   lpp=calloc(1,sizeof(struct CCPRIVATE));
   lpp->lpcc=(LPCHOOSECOLOR16)lParam;

   if (lpp->lpcc->lStructSize != sizeof(CHOOSECOLOR16))
   {
      EndDialog (hDlg, 0) ;
      return FALSE;
   }
   SetWindowLongA(hDlg, DWL_USER, (LONG)lpp); 

   if (!(lpp->lpcc->Flags & CC_SHOWHELP))
      ShowWindow(GetDlgItem(hDlg,0x40e),SW_HIDE);
   lpp->msetrgb=RegisterWindowMessageA( SETRGBSTRING );

#if 0
   cpos=MAKELONG(5,7); /* init */
   if (lpp->lpcc->Flags & CC_RGBINIT)
   {
     for (i=0;i<6;i++)
       for (j=0;j<8;j++)
        if (predefcolors[i][j]==lpp->lpcc->rgbResult)
        {
          cpos=MAKELONG(i,j);
          goto found;
        }
   }
   found:
   /* FIXME: Draw_a_focus_rect & set_init_values */
#endif

   GetWindowRect16(hDlg,&lpp->fullsize);
   if (lpp->lpcc->Flags & CC_FULLOPEN || lpp->lpcc->Flags & CC_PREVENTFULLOPEN)
   {
      hwnd=GetDlgItem(hDlg,0x2cf);
      EnableWindow(hwnd,FALSE);
   }
   if (!(lpp->lpcc->Flags & CC_FULLOPEN) || lpp->lpcc->Flags & CC_PREVENTFULLOPEN)
   {
      rect=lpp->fullsize;
      res=rect.bottom-rect.top;
      hwnd=GetDlgItem(hDlg,0x2c6); /* cut at left border */
      point.x=point.y=0;
      ClientToScreen16(hwnd,&point);
      ScreenToClient16(hDlg,&point);
      GetClientRect16(hDlg,&rect);
      point.x+=GetSystemMetrics(SM_CXDLGFRAME);
      SetWindowPos(hDlg,0,0,0,point.x,res,SWP_NOMOVE|SWP_NOZORDER);

      ShowWindow(GetDlgItem(hDlg,0x2c6),SW_HIDE);
      ShowWindow(GetDlgItem(hDlg,0x2c5),SW_HIDE);
   }
   else
      CC_SwitchToFullSize(hDlg,lpp->lpcc->rgbResult,NULL);
   res=TRUE;
   for (i=0x2bf;i<0x2c5;i++)
     SendMessage16(GetDlgItem(hDlg,i),EM_LIMITTEXT16,3,0);      /* max 3 digits:  xyz  */
   if (CC_HookCallChk(lpp->lpcc))
      res=CallWindowProc16((WNDPROC16)lpp->lpcc->lpfnHook,hDlg,WM_INITDIALOG,wParam,lParam);

   
   /* Set the initial values of the color chooser dialog */
   r = GetRValue(lpp->lpcc->rgbResult);
   g = GetGValue(lpp->lpcc->rgbResult);
   b = GetBValue(lpp->lpcc->rgbResult);

   CC_PaintSelectedColor(hDlg,lpp->lpcc->rgbResult);
   lpp->h=CC_RGBtoHSL('H',r,g,b);
   lpp->s=CC_RGBtoHSL('S',r,g,b);
   lpp->l=CC_RGBtoHSL('L',r,g,b);

   /* Doing it the long way becaus CC_EditSetRGB/HSL doesn'nt seem to work */
   SetDlgItemInt(hDlg, 703, lpp->h, TRUE);
   SetDlgItemInt(hDlg, 704, lpp->s, TRUE);
   SetDlgItemInt(hDlg, 705, lpp->l, TRUE);
   SetDlgItemInt(hDlg, 706, r, TRUE);
   SetDlgItemInt(hDlg, 707, g, TRUE);
   SetDlgItemInt(hDlg, 708, b, TRUE);

   CC_PaintCross(hDlg,lpp->h,lpp->s);
   CC_PaintTriangle(hDlg,lpp->l);

   return res;
}

/***********************************************************************
 *                              CC_WMCommand                  [internal]
 */
static LRESULT CC_WMCommand(HWND16 hDlg, WPARAM16 wParam, LPARAM lParam) 
{
    int r,g,b,i,xx;
    UINT16 cokmsg;
    HDC hdc;
    COLORREF *cr;
    struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 
    TRACE("CC_WMCommand wParam=%x lParam=%lx\n",wParam,lParam);
    switch (wParam)
    {
          case 0x2c2:  /* edit notify RGB */
	  case 0x2c3:
	  case 0x2c4:
	       if (HIWORD(lParam)==EN_UPDATE && !lpp->updating)
			 {
			   i=CC_CheckDigitsInEdit(LOWORD(lParam),255);
			   r=GetRValue(lpp->lpcc->rgbResult);
			   g=GetGValue(lpp->lpcc->rgbResult);
			   b=GetBValue(lpp->lpcc->rgbResult);
			   xx=0;
			   switch (wParam)
			   {
			    case 0x2c2:if ((xx=(i!=r))) r=i;break;
			    case 0x2c3:if ((xx=(i!=g))) g=i;break;
			    case 0x2c4:if ((xx=(i!=b))) b=i;break;
			   }
			   if (xx) /* something has changed */
			   {
			    lpp->lpcc->rgbResult=RGB(r,g,b);
			    CC_PaintSelectedColor(hDlg,lpp->lpcc->rgbResult);
			    lpp->h=CC_RGBtoHSL('H',r,g,b);
			    lpp->s=CC_RGBtoHSL('S',r,g,b);
			    lpp->l=CC_RGBtoHSL('L',r,g,b);
			    CC_EditSetHSL(hDlg,lpp->h,lpp->s,lpp->l);
			    CC_PaintCross(hDlg,lpp->h,lpp->s);
			    CC_PaintTriangle(hDlg,lpp->l);
			   }
			 }
		 break;
		 
	  case 0x2bf:  /* edit notify HSL */
	  case 0x2c0:
	  case 0x2c1:
	       if (HIWORD(lParam)==EN_UPDATE && !lpp->updating)
			 {
			   i=CC_CheckDigitsInEdit(LOWORD(lParam),wParam==0x2bf?239:240);
			   xx=0;
			   switch (wParam)
			   {
			    case 0x2bf:if ((xx=(i!=lpp->h))) lpp->h=i;break;
			    case 0x2c0:if ((xx=(i!=lpp->s))) lpp->s=i;break;
			    case 0x2c1:if ((xx=(i!=lpp->l))) lpp->l=i;break;
			   }
			   if (xx) /* something has changed */
			   {
			    r=CC_HSLtoRGB('R',lpp->h,lpp->s,lpp->l);
			    g=CC_HSLtoRGB('G',lpp->h,lpp->s,lpp->l);
			    b=CC_HSLtoRGB('B',lpp->h,lpp->s,lpp->l);
			    lpp->lpcc->rgbResult=RGB(r,g,b);
			    CC_PaintSelectedColor(hDlg,lpp->lpcc->rgbResult);
			    CC_EditSetRGB(hDlg,lpp->lpcc->rgbResult);
			    CC_PaintCross(hDlg,lpp->h,lpp->s);
			    CC_PaintTriangle(hDlg,lpp->l);
			   }
			 }
	       break;
	       
          case 0x2cf:
               CC_SwitchToFullSize(hDlg,lpp->lpcc->rgbResult,&lpp->fullsize);
	       InvalidateRect( hDlg, NULL, TRUE );
	       SetFocus(GetDlgItem(hDlg,0x2bf));
	       break;

          case 0x2c8:    /* add colors ... column by column */
               cr=PTR_SEG_TO_LIN(lpp->lpcc->lpCustColors);
               cr[(lpp->nextuserdef%2)*8 + lpp->nextuserdef/2]=lpp->lpcc->rgbResult;
               if (++lpp->nextuserdef==16)
		   lpp->nextuserdef=0;
	       CC_PaintUserColorArray(hDlg,2,8,PTR_SEG_TO_LIN(lpp->lpcc->lpCustColors));
	       break;

          case 0x2c9:              /* resulting color */
	       hdc=GetDC(hDlg);
	       lpp->lpcc->rgbResult=GetNearestColor(hdc,lpp->lpcc->rgbResult);
	       ReleaseDC(hDlg,hdc);
	       CC_EditSetRGB(hDlg,lpp->lpcc->rgbResult);
	       CC_PaintSelectedColor(hDlg,lpp->lpcc->rgbResult);
	       r=GetRValue(lpp->lpcc->rgbResult);
	       g=GetGValue(lpp->lpcc->rgbResult);
	       b=GetBValue(lpp->lpcc->rgbResult);
	       lpp->h=CC_RGBtoHSL('H',r,g,b);
	       lpp->s=CC_RGBtoHSL('S',r,g,b);
	       lpp->l=CC_RGBtoHSL('L',r,g,b);
	       CC_EditSetHSL(hDlg,lpp->h,lpp->s,lpp->l);
	       CC_PaintCross(hDlg,lpp->h,lpp->s);
	       CC_PaintTriangle(hDlg,lpp->l);
	       break;

	  case 0x40e:           /* Help! */ /* The Beatles, 1965  ;-) */
	       i=RegisterWindowMessageA( HELPMSGSTRING );
	       if (lpp->lpcc->hwndOwner)
		   SendMessage16(lpp->lpcc->hwndOwner,i,0,(LPARAM)lpp->lpcc);
	       if (CC_HookCallChk(lpp->lpcc))
		   CallWindowProc16((WNDPROC16)lpp->lpcc->lpfnHook,hDlg,
		      WM_COMMAND,psh15,(LPARAM)lpp->lpcc);
	       break;

          case IDOK :
		cokmsg=RegisterWindowMessageA( COLOROKSTRING );
		if (lpp->lpcc->hwndOwner)
			if (SendMessage16(lpp->lpcc->hwndOwner,cokmsg,0,(LPARAM)lpp->lpcc))
			   break;    /* do NOT close */

		EndDialog (hDlg, 1) ;
		return TRUE ;
	
	  case IDCANCEL :
		EndDialog (hDlg, 0) ;
		return TRUE ;

       }
       return FALSE;
}

/***********************************************************************
 *                              CC_WMPaint                    [internal]
 */
static LRESULT CC_WMPaint(HWND16 hDlg, WPARAM16 wParam, LPARAM lParam) 
{
    HDC hdc;
    PAINTSTRUCT ps;
    struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 

    hdc=BeginPaint(hDlg,&ps);
    EndPaint(hDlg,&ps);
    /* we have to paint dialog children except text and buttons */
 
    CC_PaintPredefColorArray(hDlg,6,8);
    CC_PaintUserColorArray(hDlg,2,8,PTR_SEG_TO_LIN(lpp->lpcc->lpCustColors));
    CC_PaintColorGraph(hDlg);
    CC_PaintLumBar(hDlg,lpp->h,lpp->s);
    CC_PaintCross(hDlg,lpp->h,lpp->s);
    CC_PaintTriangle(hDlg,lpp->l);
    CC_PaintSelectedColor(hDlg,lpp->lpcc->rgbResult);

    /* special necessary for Wine */
    ValidateRect(GetDlgItem(hDlg,0x2d0),NULL);
    ValidateRect(GetDlgItem(hDlg,0x2d1),NULL);
    ValidateRect(GetDlgItem(hDlg,0x2c6),NULL);
    ValidateRect(GetDlgItem(hDlg,0x2be),NULL);
    ValidateRect(GetDlgItem(hDlg,0x2c5),NULL);
    /* hope we can remove it later -->FIXME */
 return TRUE;
}


/***********************************************************************
 *                              CC_WMLButtonDown              [internal]
 */
static LRESULT CC_WMLButtonDown(HWND16 hDlg, WPARAM16 wParam, LPARAM lParam) 
{
   struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 
   int r,g,b,i;
   i=0;
   if (CC_MouseCheckPredefColorArray(hDlg,0x2d0,6,8,lParam,&lpp->lpcc->rgbResult))
      i=1;
   else
      if (CC_MouseCheckUserColorArray(hDlg,0x2d1,2,8,lParam,&lpp->lpcc->rgbResult,
	      PTR_SEG_TO_LIN(lpp->lpcc->lpCustColors)))
         i=1;
      else
	 if (CC_MouseCheckColorGraph(hDlg,0x2c6,&lpp->h,&lpp->s,lParam))
	    i=2;
	 else
	    if (CC_MouseCheckColorGraph(hDlg,0x2be,NULL,&lpp->l,lParam))
	       i=2;
   if (i==2)
   {
      r=CC_HSLtoRGB('R',lpp->h,lpp->s,lpp->l);
      g=CC_HSLtoRGB('G',lpp->h,lpp->s,lpp->l);
      b=CC_HSLtoRGB('B',lpp->h,lpp->s,lpp->l);
      lpp->lpcc->rgbResult=RGB(r,g,b);
   }
   if (i==1)
   {
      r=GetRValue(lpp->lpcc->rgbResult);
      g=GetGValue(lpp->lpcc->rgbResult);
      b=GetBValue(lpp->lpcc->rgbResult);
      lpp->h=CC_RGBtoHSL('H',r,g,b);
      lpp->s=CC_RGBtoHSL('S',r,g,b);
      lpp->l=CC_RGBtoHSL('L',r,g,b);
   }
   if (i)
   {
      CC_EditSetRGB(hDlg,lpp->lpcc->rgbResult);
      CC_EditSetHSL(hDlg,lpp->h,lpp->s,lpp->l);
      CC_PaintCross(hDlg,lpp->h,lpp->s);
      CC_PaintTriangle(hDlg,lpp->l);
      CC_PaintSelectedColor(hDlg,lpp->lpcc->rgbResult);
      return TRUE;
   }
   return FALSE;
}

/***********************************************************************
 *           ColorDlgProc   (COMMDLG.8)
 */
LRESULT WINAPI ColorDlgProc16(HWND16 hDlg, UINT16 message,
                            WPARAM16 wParam, LONG lParam)
{
 int res;
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 
 if (message!=WM_INITDIALOG)
 {
  if (!lpp)
     return FALSE;
  res=0;
  if (CC_HookCallChk(lpp->lpcc))
     res=CallWindowProc16((WNDPROC16)lpp->lpcc->lpfnHook,hDlg,message,wParam,lParam);
  if (res)
     return res;
 }

 /* FIXME: SetRGB message
 if (message && message==msetrgb)
    return HandleSetRGB(hDlg,lParam);
 */

 switch (message)
	{
	  case WM_INITDIALOG:
	                return CC_WMInitDialog(hDlg,wParam,lParam);
	  case WM_NCDESTROY:
	                DeleteDC(lpp->hdcMem); 
	                DeleteObject(lpp->hbmMem); 
	                free(lpp);
	                SetWindowLongA(hDlg, DWL_USER, 0L); /* we don't need it anymore */
	                break;
	  case WM_COMMAND:
	                if (CC_WMCommand(hDlg, wParam, lParam))
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
	  case WM_MOUSEMOVE:  /* FIXME: calculate new hue,sat,lum (if in color graph) */
			break;
	  case WM_LBUTTONUP:  /* FIXME: ClipCursor off (if in color graph)*/
			break;
	  case WM_LBUTTONDOWN:/* FIXME: ClipCursor on  (if in color graph)*/
	                if (CC_WMLButtonDown(hDlg, wParam, lParam))
	                   return TRUE;
	                break;     
	}
     return FALSE ;
}

/***********************************************************************
 *            ChooseColorA  (COMDLG32.1)
 */
BOOL WINAPI ChooseColorA(LPCHOOSECOLORA lpChCol )

{
  BOOL16 ret;
  char *str = NULL;
  COLORREF* ccref=SEGPTR_ALLOC(64);
  LPCHOOSECOLOR16 lpcc16=SEGPTR_ALLOC(sizeof(CHOOSECOLOR16));

  memset(lpcc16,'\0',sizeof(*lpcc16));
  lpcc16->lStructSize=sizeof(*lpcc16);
  lpcc16->hwndOwner=lpChCol->hwndOwner;
  lpcc16->hInstance=MapHModuleLS(lpChCol->hInstance);
  lpcc16->rgbResult=lpChCol->rgbResult;
  memcpy(ccref,lpChCol->lpCustColors,64);
  lpcc16->lpCustColors=(COLORREF*)SEGPTR_GET(ccref);
  lpcc16->Flags=lpChCol->Flags;
  lpcc16->lCustData=lpChCol->lCustData;
  lpcc16->lpfnHook=(LPCCHOOKPROC16)lpChCol->lpfnHook;
  if (lpChCol->lpTemplateName)
    str = SEGPTR_STRDUP(lpChCol->lpTemplateName );
  lpcc16->lpTemplateName=SEGPTR_GET(str);
  
  ret = ChooseColor16(lpcc16);

  if(ret)
      lpChCol->rgbResult = lpcc16->rgbResult;

  if(str)
    SEGPTR_FREE(str);
  memcpy(lpChCol->lpCustColors,ccref,64);
  SEGPTR_FREE(ccref);
  SEGPTR_FREE(lpcc16);
  return (BOOL)ret;
}

/***********************************************************************
 *            ChooseColorW  (COMDLG32.2)
 */
BOOL WINAPI ChooseColorW(LPCHOOSECOLORW lpChCol )

{
  BOOL16 ret;
  char *str = NULL;
  COLORREF* ccref=SEGPTR_ALLOC(64);
  LPCHOOSECOLOR16 lpcc16=SEGPTR_ALLOC(sizeof(CHOOSECOLOR16));

  memset(lpcc16,'\0',sizeof(*lpcc16));
  lpcc16->lStructSize=sizeof(*lpcc16);
  lpcc16->hwndOwner=lpChCol->hwndOwner;
  lpcc16->hInstance=MapHModuleLS(lpChCol->hInstance);
  lpcc16->rgbResult=lpChCol->rgbResult;
  memcpy(ccref,lpChCol->lpCustColors,64);
  lpcc16->lpCustColors=(COLORREF*)SEGPTR_GET(ccref);
  lpcc16->Flags=lpChCol->Flags;
  lpcc16->lCustData=lpChCol->lCustData;
  lpcc16->lpfnHook=(LPCCHOOKPROC16)lpChCol->lpfnHook;
  if (lpChCol->lpTemplateName)
    str = SEGPTR_STRDUP_WtoA(lpChCol->lpTemplateName );
  lpcc16->lpTemplateName=SEGPTR_GET(str);
  
  ret = ChooseColor16(lpcc16);

  if(ret)
      lpChCol->rgbResult = lpcc16->rgbResult;

  if(str)
    SEGPTR_FREE(str);
  memcpy(lpChCol->lpCustColors,ccref,64);
  SEGPTR_FREE(ccref);
  SEGPTR_FREE(lpcc16);
  return (BOOL)ret;
}
