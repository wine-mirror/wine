/*
 * System color objects
 *
 * Copyright  Alexandre Julliard, 1994
 */

#ifndef SYSCOLOR_H
#define SYSCOLOR_H

#include "windows.h"

struct SysColorObjects
{
    HBRUSH16 hbrushScrollbar;        /* COLOR_SCROLLBAR           */
                                     /* COLOR_BACKGROUND          */
    HBRUSH16 hbrushActiveCaption;    /* COLOR_ACTIVECAPTION       */
    HBRUSH16 hbrushInactiveCaption;  /* COLOR_INACTIVECAPTION     */
    HBRUSH16 hbrushMenu;             /* COLOR_MENU                */
    HBRUSH16 hbrushWindow;           /* COLOR_WINDOW              */
    HPEN16   hpenWindowFrame;        /* COLOR_WINDOWFRAME         */
                                     /* COLOR_MENUTEXT            */
    HPEN16   hpenWindowText;         /* COLOR_WINDOWTEXT          */
                                     /* COLOR_CAPTIONTEXT         */
    HBRUSH16 hbrushActiveBorder;     /* COLOR_ACTIVEBORDER        */
    HBRUSH16 hbrushInactiveBorder;   /* COLOR_INACTIVEBORDER      */
                                     /* COLOR_APPWORKSPACE        */
    HBRUSH16 hbrushHighlight;        /* COLOR_HIGHLIGHT           */
                                     /* COLOR_HIGHLIGHTTEXT       */
    HBRUSH16 hbrushBtnFace;          /* COLOR_BTNFACE             */
    HBRUSH16 hbrushBtnShadow;        /* COLOR_BTNSHADOW           */
                                     /* COLOR_GRAYTEXT            */
                                     /* COLOR_BTNTEXT             */
                                     /* COLOR_INACTIVECAPTIONTEXT */
    HBRUSH16 hbrushBtnHighlight;     /* COLOR_BTNHIGHLIGHT        */
};

extern void SYSCOLOR_Init(void);
extern struct SysColorObjects sysColorObjects;

#endif  /* SYSCOLOR_H */
