/*
 * System color objects
 *
 * Copyright  Alexandre Julliard, 1994
 */

#ifndef __WINE_SYSCOLOR_H
#define __WINE_SYSCOLOR_H

#include "windows.h"

struct SysColorObjects
{
    HBRUSH32 hbrushScrollbar;        /* COLOR_SCROLLBAR           */
                                     /* COLOR_BACKGROUND          */
    HBRUSH32 hbrushActiveCaption;    /* COLOR_ACTIVECAPTION       */
    HBRUSH32 hbrushInactiveCaption;  /* COLOR_INACTIVECAPTION     */
    HBRUSH32 hbrushMenu;             /* COLOR_MENU                */
    HBRUSH32 hbrushWindow;           /* COLOR_WINDOW              */
    HPEN32   hpenWindowFrame;        /* COLOR_WINDOWFRAME         */
                                     /* COLOR_MENUTEXT            */
    HPEN32   hpenWindowText;         /* COLOR_WINDOWTEXT          */
                                     /* COLOR_CAPTIONTEXT         */
    HBRUSH32 hbrushActiveBorder;     /* COLOR_ACTIVEBORDER        */
    HBRUSH32 hbrushInactiveBorder;   /* COLOR_INACTIVEBORDER      */
                                     /* COLOR_APPWORKSPACE        */
    HBRUSH32 hbrushHighlight;        /* COLOR_HIGHLIGHT           */
                                     /* COLOR_HIGHLIGHTTEXT       */
    HBRUSH32 hbrushBtnFace;          /* COLOR_BTNFACE             */
    HBRUSH32 hbrushBtnShadow;        /* COLOR_BTNSHADOW           */
                                     /* COLOR_GRAYTEXT            */
                                     /* COLOR_BTNTEXT             */
                                     /* COLOR_INACTIVECAPTIONTEXT */
    HBRUSH32 hbrushBtnHighlight;     /* COLOR_BTNHIGHLIGHT        */
};

extern void SYSCOLOR_Init(void);
extern struct SysColorObjects sysColorObjects;

#endif  /* __WINE_SYSCOLOR_H */
