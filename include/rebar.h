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

    LPSTR     lpText;

} REBAR_BAND;


typedef struct tagREBAR_INFO
{
    COLORREF   clrBk;      /* background color */
    COLORREF   clrText;    /* text color */
    HIMAGELIST himl;       /* handle to imagelist */
    UINT32     uNumBands;  /* number of bands in the rebar */

    REBAR_BAND *bands;     /* pointer to the array of rebar bands */

} REBAR_INFO;


extern void REBAR_Register (void);

#endif  /* __WINE_REBAR_H */
