/*
 * System color objects
 *
 * Copyright  Alexandre Julliard, 1994
 */

#ifndef SYSCOLOR_H
#define SYSCOLOR_H

#include "gdi.h"

struct SysColorObjects
{
    HBRUSH hbrushScrollbar;        /* COLOR_SCROLLBAR           */
                                   /* COLOR_BACKGROUND          */
    HBRUSH hbrushActiveCaption;    /* COLOR_ACTIVECAPTION       */
    HBRUSH hbrushInactiveCaption;  /* COLOR_INACTIVECAPTION     */
    HBRUSH hbrushMenu;             /* COLOR_MENU                */
    HBRUSH hbrushWindow;           /* COLOR_WINDOW              */
    HPEN hpenWindowFrame;          /* COLOR_WINDOWFRAME         */
                                   /* COLOR_MENUTEXT            */
    HPEN hpenWindowText;           /* COLOR_WINDOWTEXT          */
                                   /* COLOR_CAPTIONTEXT         */
    HBRUSH hbrushActiveBorder;     /* COLOR_ACTIVEBORDER        */
    HBRUSH hbrushInactiveBorder;   /* COLOR_INACTIVEBORDER      */
                                   /* COLOR_APPWORKSPACE        */
    HBRUSH hbrushHighlight;        /* COLOR_HIGHLIGHT           */
                                   /* COLOR_HIGHLIGHTTEXT       */
    HBRUSH hbrushBtnFace;          /* COLOR_BTNFACE             */
    HBRUSH hbrushBtnShadow;        /* COLOR_BTNSHADOW           */
                                   /* COLOR_GRAYTEXT            */
                                   /* COLOR_BTNTEXT             */
                                   /* COLOR_INACTIVECAPTIONTEXT */
    HBRUSH hbrushBtnHighlight;     /* COLOR_BTNHIGHLIGHT        */
};

extern struct SysColorObjects sysColorObjects;

#endif  /* SYSCOLOR_H */
