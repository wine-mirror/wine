/*
 * Win16 printer driver definitions
 *
 * Copyright (C) the Wine project
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WIN16DRV_H
#define __WINE_WIN16DRV_H

#include "windef.h"
#include "wingdi.h"
#include "gdi.h"

#define SETHIGHBIT
#undef SETHIGHBIT
#ifdef SETHIGHBIT
#define GETGDIINFO  0x8001
#define INITPDEVICE 0x8000
#else
#define GETGDIINFO  0x0001
#define INITPDEVICE 0x0000
#endif

#define     OS_ARC		3
#define     OS_SCANLINES	4
#define     OS_RECTANGLE	6
#define     OS_ELLIPSE		7
#define     OS_MARKER		8
#define     OS_POLYLINE 	18
#define     OS_ALTPOLYGON	22
#define     OS_WINDPOLYGON	20
#define     OS_PIE		23
#define     OS_POLYMARKER	24
#define     OS_CHORD		39
#define     OS_CIRCLE		55
#define     OS_ROUNDRECT	72

/* Internal Data */
#define ORD_BITBLT		1
#define ORD_COLORINFO		2
#define ORD_CONTROL		3
#define ORD_DISABLE		4
#define ORD_ENABLE		5
#define ORD_ENUMDFONTS		6
#define ORD_ENUMOBJ		7
#define ORD_OUTPUT		8
#define ORD_PIXEL		9
#define ORD_REALIZEOBJECT	10
#define ORD_STRBLT		11
#define ORD_SCANLR		12
#define ORD_DEVICEMODE		13
#define ORD_EXTTEXTOUT		14
#define ORD_GETCHARWIDTH	15
#define ORD_DEVICEBITMAP	16
#define ORD_FASTBORDER		17
#define ORD_SETATTRIBUTE	18

#define ORD_STRETCHBLT		27
#define ORD_STRETCHDIBITS	28
#define ORD_SELECTBITMAP	29
#define ORD_BITMAPBITS		30

#define ORD_EXTDEVICEMODE	90
#define ORD_DEVICECAPABILITIES	91
#define ORD_ADVANCEDSETUPDIALOG	93

#define ORD_DIALOGFN		100
#define ORD_PSEUDOEDIT		101

enum {
    FUNC_BITBLT = 0,
    FUNC_COLORINFO,
    FUNC_CONTROL,
    FUNC_DISABLE,
    FUNC_ENABLE,
    FUNC_ENUMDFONTS,
    FUNC_ENUMOBJ,
    FUNC_OUTPUT,
    FUNC_PIXEL,
    FUNC_REALIZEOBJECT,
    FUNC_STRBLT,
    FUNC_SCANLR,
    FUNC_DEVICEMODE,
    FUNC_EXTTEXTOUT,
    FUNC_GETCHARWIDTH,
    FUNC_DEVICEBITMAP,
    FUNC_FASTBORDER,
    FUNC_SETATTRIBUTE,
    FUNC_STRETCHBLT,
    FUNC_STRETCHDIBITS,
    FUNC_SELECTBITMAP,
    FUNC_BITMAPBITS,
    FUNC_EXTDEVICEMODE,
    FUNC_DEVICECAPABILITIES,
    FUNC_ADVANCEDSETUPDIALOG,
    FUNC_DIALOGFN,
    FUNC_PSEUDOEDIT,
    TOTAL_PRINTER_DRIVER_FUNCTIONS /* insert functions before here */
};

typedef struct
{
    LPSTR 	szDriver;		/* Driver name eg EPSON */
    HINSTANCE16	hInst;			/* Handle for driver */
    WORD	ds_reg;			/* DS of driver */
    FARPROC16 	fn[TOTAL_PRINTER_DRIVER_FUNCTIONS];	/* Printer functions */
    int		nUsageCount;		/* Usage count, unload == 0 */
    int		nIndex;			/* Index in global driver array */
} LOADED_PRINTER_DRIVER;

typedef struct PDEVICE_HEADER
{
    LOADED_PRINTER_DRIVER *pLPD;	/* Associated printer driver */
} PDEVICE_HEADER;

#include "pshpack1.h"
#define PCOLOR DWORD
typedef struct DRAWMODE
{
    SHORT    Rop2;
    SHORT    bkMode;
    PCOLOR   bkColor;
    PCOLOR   TextColor;
    SHORT    TBreakExtra;
    SHORT    BreakExtra;
    SHORT    BreakErr;
    SHORT    BreakRem;
    SHORT    BreakCount;
    SHORT    CharExtra;
    COLORREF LbkColor;
    COLORREF LTextColor;
    DWORD    ICMCXform;
    SHORT    StretchBltMode;
    DWORD    eMiterLimit;
} DRAWMODE, *LPDRAWMODE;


#include "poppack.h"

typedef struct WINE_ENUM_PRINTER_FONT_CALLBACK
{
    DEVICEFONTENUMPROC proc;
    LPARAM lp;
} WEPFC;

#define DRVOBJ_PEN 	1
#define DRVOBJ_BRUSH 	2
#define DRVOBJ_FONT 	3
#define DRVOBJ_PBITMAP 	5

typedef struct tagDeviceCaps
{
    WORD   version;       /*   0: driver version */
    WORD   technology;    /*   2: device technology */
    WORD   horzSize;      /*   4: width of display in mm */
    WORD   vertSize;      /*   6: height of display in mm */
    WORD   horzRes;       /*   8: width of display in pixels */
    WORD   vertRes;       /*  10: width of display in pixels */
    WORD   bitsPixel;     /*  12: bits per pixel */
    WORD   planes;        /*  14: color planes */
    WORD   numBrushes;    /*  16: device-specific brushes */
    WORD   numPens;       /*  18: device-specific pens */
    WORD   numMarkers;    /*  20: device-specific markers */
    WORD   numFonts;      /*  22: device-specific fonts */
    WORD   numColors;     /*  24: size of color table */
    WORD   pdeviceSize;   /*  26: size of PDEVICE structure */
    WORD   curveCaps;     /*  28: curve capabilities */
    WORD   lineCaps;      /*  30: line capabilities */
    WORD   polygonalCaps; /*  32: polygon capabilities */
    WORD   textCaps;      /*  34: text capabilities */
    WORD   clipCaps;      /*  36: clipping capabilities */
    WORD   rasterCaps;    /*  38: raster capabilities */
    WORD   aspectX;       /*  40: relative width of device pixel */
    WORD   aspectY;       /*  42: relative height of device pixel */
    WORD   aspectXY;      /*  44: relative diagonal width of device pixel */
    WORD   pad1[21];      /*  46-86: reserved */
    WORD   logPixelsX;    /*  88: pixels / logical X inch */
    WORD   logPixelsY;    /*  90: pixels / logical Y inch */
    WORD   pad2[6];       /*  92-102: reserved */
    WORD   sizePalette;   /* 104: entries in system palette */
    WORD   numReserved;   /* 106: reserved entries */
    WORD   colorRes;      /* 108: color resolution */
} DeviceCaps;

/* Win16 printer driver physical DC */
typedef struct
{
    SEGPTR		segptrPDEVICE;	/* PDEVICE used by 16 bit printer drivers */
    LOGFONT16		lf;		/* Current font details */
    TEXTMETRIC16	tm;		/* Current font metrics */
    LPFONTINFO16       	FontInfo;       /* Current font realized by printer driver */
    LPLOGBRUSH16	BrushInfo;      /* Current brush realized by printer driver */
    LPLOGPEN16		PenInfo;        /* Current pen realized by printer driver */
    HDC                 hdc;
    DC                 *dc;
    POINT               org;            /* Device origin */
    DeviceCaps          DevCaps;        /* Device caps */
} WIN16DRV_PDEVICE;

/*
 * Printer driver functions
 */
typedef SEGPTR LPPDEVICE;
LOADED_PRINTER_DRIVER *LoadPrinterDriver(const char *pszDriver);

extern INT16 PRTDRV_Control(LPPDEVICE lpDestDev, WORD wfunction, SEGPTR lpInData, SEGPTR lpOutData);
extern WORD PRTDRV_Enable(LPVOID lpDevInfo, WORD wStyle, LPCSTR  lpDestDevType,
                          LPCSTR lpDeviceName, LPCSTR lpOutputFile, LPVOID lpData);
extern WORD PRTDRV_EnumDFonts(LPPDEVICE lpDestDev, LPSTR lpFaceName,
		       FARPROC16 lpCallbackFunc, LPVOID lpClientData);
extern DWORD PRTDRV_RealizeObject(LPPDEVICE lpDestDev, WORD wStyle,
				  LPVOID lpInObj, LPVOID lpOutObj,
				  SEGPTR lpTextXForm);

extern BOOL16 PRTDRV_EnumObj(LPPDEVICE lpDestDev, WORD iStyle, FARPROC16 lpfn, LPVOID lpb);
extern DWORD PRTDRV_ExtTextOut(LPPDEVICE lpDestDev, WORD wDestXOrg, WORD wDestYOrg,
			       RECT16 *lpClipRect, LPCSTR lpString, WORD wCount,
			       LPFONTINFO16 lpFontInfo, SEGPTR lpDrawMode,
			       SEGPTR lpTextXForm, SHORT *lpCharWidths,
			       RECT16 *     lpOpaqueRect, WORD wOptions);

extern WORD PRTDRV_Output(LPPDEVICE 	 lpDestDev,
			  WORD 	 wStyle,
			  WORD 	 wCount,
			  POINT16       *points,
			  LPLOGPEN16 	 lpPen,
			  LPLOGBRUSH16	 lpBrush,
			  SEGPTR	 lpDrawMode,
			  HRGN 	 hClipRgn);

DWORD PRTDRV_StretchBlt(LPPDEVICE lpDestDev,
                        WORD wDestX, WORD wDestY,
                        WORD wDestXext, WORD wDestYext,
                        LPPDEVICE lpSrcDev,
                        WORD wSrcX, WORD wSrcY,
                        WORD wSrcXext, WORD wSrcYext,
                        DWORD Rop3,
                        LPLOGBRUSH16 lpBrush,
                        SEGPTR lpDrawMode,
                        RECT16 *lpClipRect);

extern WORD PRTDRV_GetCharWidth(LPPDEVICE lpDestDev, LPINT lpBuffer,
		      WORD wFirstChar, WORD wLastChar, LPFONTINFO16 lpFontInfo,
		      SEGPTR lpDrawMode, SEGPTR lpTextXForm );

/* Wine driver functions */

extern const DC_FUNCTIONS *WIN16DRV_Init(void);
extern BOOL WIN16DRV_GetCharWidth( PHYSDEV dev, UINT firstChar, UINT lastChar, LPINT buffer );

extern BOOL WIN16DRV_GetTextExtentPoint( PHYSDEV dev, LPCWSTR str, INT count,
                                           LPSIZE size );
extern BOOL WIN16DRV_GetTextMetrics( PHYSDEV dev, TEXTMETRICW *metrics );

extern BOOL WIN16DRV_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags,
                                  const RECT *lprect, LPCWSTR str, UINT count,
                                  const INT *lpDx );
extern BOOL WIN16DRV_LineTo( PHYSDEV dev, INT x, INT y );
extern BOOL WIN16DRV_Polygon(PHYSDEV dev, const POINT* pt, INT count );
extern BOOL WIN16DRV_Polyline(PHYSDEV dev, const POINT* pt, INT count );
extern BOOL WIN16DRV_Rectangle(PHYSDEV dev, INT left, INT top, INT right, INT bottom);
extern HBITMAP WIN16DRV_SelectBitmap( PHYSDEV dev, HBITMAP handle );
extern HBRUSH WIN16DRV_SelectBrush( PHYSDEV dev, HBRUSH handle );
extern HFONT WIN16DRV_SelectFont( PHYSDEV dev, HFONT handle );
extern HPEN WIN16DRV_SelectPen( PHYSDEV dev, HPEN handle );
extern BOOL WIN16DRV_PatBlt( PHYSDEV dev, INT left, INT top,
                               INT width, INT height, DWORD rop );
extern BOOL WIN16DRV_Ellipse(PHYSDEV dev, INT left, INT top, INT right, INT bottom);
extern BOOL WIN16DRV_EnumDeviceFonts( PHYSDEV dev, LPLOGFONTW plf,
                                      DEVICEFONTENUMPROC proc, LPARAM lp );

extern INT WIN16DRV_ExtDeviceMode(LPSTR lpszDriver, HWND hwnd,
				  LPDEVMODEA lpdmOutput,
				  LPSTR lpszDevice,
				  LPSTR lpszPort, LPDEVMODEA lpdmInput,
				  LPSTR lpszProfile, DWORD dwMode);
extern DWORD WIN16DRV_DeviceCapabilities(LPSTR lpszDriver, LPCSTR lpszDevice,
					 LPCSTR lpszPort, WORD fwCapability,
					 LPSTR lpszOutput, LPDEVMODEA lpdm);

/*
 * Wine 16bit driver global variables
 */
extern SEGPTR		win16drv_SegPtr_TextXForm;
extern LPTEXTXFORM16 	win16drv_TextXFormP;
extern SEGPTR		win16drv_SegPtr_DrawMode;
extern LPDRAWMODE 	win16drv_DrawModeP;

#endif  /* __WINE_WIN16DRV_H */
