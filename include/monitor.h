/*
 * Monitor definitions
 *
 */

#ifndef __WINE_MONITOR_H
#define __WINE_MONITOR_H

#include "windef.h"

typedef struct tagMONITOR 
{
    RECT rect;
    int  depth;
} MONITOR;

extern MONITOR MONITOR_PrimaryMonitor;

static int inline MONITOR_GetWidth(MONITOR *pMonitor)
{
    return pMonitor->rect.right - pMonitor->rect.left;
}

static int inline MONITOR_GetHeight(MONITOR *pMonitor)
{
    return pMonitor->rect.bottom - pMonitor->rect.top;
}

static int inline MONITOR_GetDepth(MONITOR *pMonitor)
{
    return pMonitor->depth;
}

#endif /* __WINE_MONITOR_H */
