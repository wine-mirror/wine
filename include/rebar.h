/*
 * Rebar class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_REBAR_H
#define __WINE_REBAR_H

typedef struct tagREBAR_BAND
{
    UINT32    fStyle;
    COLORREF  clrFore;
    COLORREF  clrBack;
    INT32     iImage;
    HWND32    hwndChild;
    UINT32    cxMinChild;
    UINT32    cyMinChild;
    UINT32    cx;
    HBITMAP32 hbmBack;
    UINT32    wID;
    UINT32    cyChild;
    UINT32    cyMaxChild;
    UINT32    cyIntegral;
    UINT32    cxIdeal;
    LPARAM    lParam;
    UINT32    cxHeader;

    UINT32    uMinHeight;
    UINT32    fDraw;          /* drawing flags */
    RECT32    rcBand;         /* calculated band rectangle */
    RECT32    rcGripper;      /* calculated gripper rectangle */
    RECT32    rcCapImage;     /* calculated caption image rectangle */
    RECT32    rcCapText;      /* calculated caption text rectangle */
    RECT32    rcChild;        /* calculated child rectangle */

    LPSTR     lpText;
    HWND32    hwndPrevParent;
} REBAR_BAND;

typedef struct tagREBAR_INFO
{
    COLORREF   clrBk;       /* background color */
    COLORREF   clrText;     /* text color */
    HIMAGELIST himl;        /* handle to imagelist */
    UINT32     uNumBands;   /* number of bands in the rebar */
    HWND32     hwndToolTip; /* handle to the tool tip control */
    HWND32     hwndNotify;  /* notification window (parent) */
    HFONT32    hFont;       /* handle to the rebar's font */
    SIZE32     imageSize;   /* image size (image list) */

    SIZE32     calcSize;    /* calculated rebar size */
    BOOL32     bAutoResize; /* auto resize deadlock flag */
    HCURSOR32  hcurArrow;   /* handle to the arrow cursor */
    HCURSOR32  hcurHorz;    /* handle to the EW cursor */
    HCURSOR32  hcurVert;    /* handle to the NS cursor */
    HCURSOR32  hcurDrag;    /* handle to the drag cursor */

    REBAR_BAND *bands;      /* pointer to the array of rebar bands */

} REBAR_INFO;


extern VOID REBAR_Register (VOID);
extern VOID REBAR_Unregister (VOID);

#endif  /* __WINE_REBAR_H */
