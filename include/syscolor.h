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
    HBRUSH32 hbrushBackground;       /* COLOR_BACKGROUND          */
    HBRUSH32 hbrushActiveCaption;    /* COLOR_ACTIVECAPTION       */
    HBRUSH32 hbrushInactiveCaption;  /* COLOR_INACTIVECAPTION     */
    HBRUSH32 hbrushMenu;             /* COLOR_MENU                */
    HBRUSH32 hbrushWindow;           /* COLOR_WINDOW              */
    HBRUSH32 hbrushWindowFrame;      /* COLOR_WINDOWFRAME         */
    HBRUSH32 hbrushMenuText;         /* COLOR_MENUTEXT            */
    HBRUSH32 hbrushWindowText;       /* COLOR_WINDOWTEXT          */
    HBRUSH32 hbrushCaptionText;      /* COLOR_CAPTIONTEXT         */
    HBRUSH32 hbrushActiveBorder;     /* COLOR_ACTIVEBORDER        */
    HBRUSH32 hbrushInactiveBorder;   /* COLOR_INACTIVEBORDER      */
    HBRUSH32 hbrushAppWorkspace;     /* COLOR_APPWORKSPACE        */
    HBRUSH32 hbrushHighlight;        /* COLOR_HIGHLIGHT           */
    HBRUSH32 hbrushHighlightText;    /* COLOR_HIGHLIGHTTEXT       */
    HBRUSH32 hbrushBtnFace;          /* COLOR_BTNFACE             */
    HBRUSH32 hbrushBtnShadow;        /* COLOR_BTNSHADOW           */
    HBRUSH32 hbrushGrayText;         /* COLOR_GRAYTEXT            */
    HBRUSH32 hbrushBtnText;          /* COLOR_BTNTEXT             */
    HBRUSH32 hbrushInactiveCaptionText; /* COLOR_INACTIVECAPTIONTEXT */
    HBRUSH32 hbrushBtnHighlight;     /* COLOR_BTNHIGHLIGHT        */
    HBRUSH32 hbrush3DDkShadow;       /* COLOR_3DDKSHADOW          */
    HBRUSH32 hbrush3DLight;          /* COLOR_3DLIGHT             */
    HBRUSH32 hbrushInfoText;         /* COLOR_INFOTEXT            */
    HBRUSH32 hbrushInfoBk;           /* COLOR_INFOBK              */

    HPEN32   hpenWindowFrame;        /* COLOR_WINDOWFRAME         */
    HPEN32   hpenWindowText;         /* COLOR_WINDOWTEXT          */
};

extern void SYSCOLOR_Init(void);
extern struct SysColorObjects sysColorObjects;

#endif  /* __WINE_SYSCOLOR_H */
