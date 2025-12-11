/*
 * User32 Modern DPI Awareness APIs
 * Support for high-DPI displays and modern Windows 10/11 DPI features
 * Required by Electron apps and modern Windows applications
 *
 * Copyright 2025 Wine Project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include <stdarg.h>
#include "user_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(user32);

/* DPI_AWARENESS_CONTEXT values */
#define DPI_AWARENESS_CONTEXT_UNAWARE              ((DPI_AWARENESS_CONTEXT)-1)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE         ((DPI_AWARENESS_CONTEXT)-2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    ((DPI_AWARENESS_CONTEXT)-3)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#define DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED    ((DPI_AWARENESS_CONTEXT)-5)

typedef enum DPI_AWARENESS {
    DPI_AWARENESS_INVALID = -1,
    DPI_AWARENESS_UNAWARE = 0,
    DPI_AWARENESS_SYSTEM_AWARE = 1,
    DPI_AWARENESS_PER_MONITOR_AWARE = 2
} DPI_AWARENESS;

typedef enum DPI_HOSTING_BEHAVIOR {
    DPI_HOSTING_BEHAVIOR_INVALID = -1,
    DPI_HOSTING_BEHAVIOR_DEFAULT = 0,
    DPI_HOSTING_BEHAVIOR_MIXED = 1
} DPI_HOSTING_BEHAVIOR;

/* Default DPI value */
#define DEFAULT_DPI 96

/* Current process DPI awareness context */
static DPI_AWARENESS_CONTEXT current_dpi_context = DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;

/***********************************************************************
 *          SetProcessDpiAwarenessContext
 */
BOOL WINAPI SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT context )
{
    TRACE( "context %p\n", context );

    /* Validate context */
    if ((LONG_PTR)context < -5 || (LONG_PTR)context > -1)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    current_dpi_context = context;
    TRACE( "Set DPI awareness context to %p\n", context );
    return TRUE;
}

/***********************************************************************
 *          GetThreadDpiAwarenessContext
 */
DPI_AWARENESS_CONTEXT WINAPI GetThreadDpiAwarenessContext(void)
{
    TRACE( "returning %p\n", current_dpi_context );
    return current_dpi_context;
}

/***********************************************************************
 *          SetThreadDpiAwarenessContext
 */
DPI_AWARENESS_CONTEXT WINAPI SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT context )
{
    DPI_AWARENESS_CONTEXT old = current_dpi_context;

    TRACE( "context %p\n", context );

    if ((LONG_PTR)context < -5 || (LONG_PTR)context > -1)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    current_dpi_context = context;
    return old;
}

/***********************************************************************
 *          GetDpiFromDpiAwarenessContext
 */
UINT WINAPI GetDpiFromDpiAwarenessContext( DPI_AWARENESS_CONTEXT context )
{
    TRACE( "context %p\n", context );

    /* For now, return default DPI - in real implementation this would
     * consider the monitor and context */
    return DEFAULT_DPI;
}

/***********************************************************************
 *          GetAwarenessFromDpiAwarenessContext
 */
DPI_AWARENESS WINAPI GetAwarenessFromDpiAwarenessContext( DPI_AWARENESS_CONTEXT context )
{
    TRACE( "context %p\n", context );

    switch ((LONG_PTR)context)
    {
    case (LONG_PTR)DPI_AWARENESS_CONTEXT_UNAWARE:
    case (LONG_PTR)DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED:
        return DPI_AWARENESS_UNAWARE;
    case (LONG_PTR)DPI_AWARENESS_CONTEXT_SYSTEM_AWARE:
        return DPI_AWARENESS_SYSTEM_AWARE;
    case (LONG_PTR)DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE:
    case (LONG_PTR)DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2:
        return DPI_AWARENESS_PER_MONITOR_AWARE;
    default:
        return DPI_AWARENESS_INVALID;
    }
}

/***********************************************************************
 *          AreDpiAwarenessContextsEqual
 */
BOOL WINAPI AreDpiAwarenessContextsEqual( DPI_AWARENESS_CONTEXT ctx1, DPI_AWARENESS_CONTEXT ctx2 )
{
    TRACE( "ctx1 %p, ctx2 %p\n", ctx1, ctx2 );

    /* Simple equality check */
    if (ctx1 == ctx2)
        return TRUE;

    /* Check if they map to the same awareness level */
    return GetAwarenessFromDpiAwarenessContext(ctx1) == GetAwarenessFromDpiAwarenessContext(ctx2);
}

/***********************************************************************
 *          IsValidDpiAwarenessContext
 */
BOOL WINAPI IsValidDpiAwarenessContext( DPI_AWARENESS_CONTEXT context )
{
    TRACE( "context %p\n", context );

    LONG_PTR val = (LONG_PTR)context;
    return (val >= -5 && val <= -1);
}

/***********************************************************************
 *          GetDpiForWindow
 */
UINT WINAPI GetDpiForWindow( HWND hwnd )
{
    TRACE( "hwnd %p\n", hwnd );

    /* For now, return default DPI
     * Real implementation would get DPI from the monitor containing the window */
    if (!IsWindow( hwnd ))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    return DEFAULT_DPI;
}

/***********************************************************************
 *          GetDpiForSystem
 */
UINT WINAPI GetDpiForSystem(void)
{
    TRACE( "returning default DPI %d\n", DEFAULT_DPI );

    /* Return system DPI - could be read from display settings */
    return DEFAULT_DPI;
}

/***********************************************************************
 *          GetSystemDpiForProcess
 */
UINT WINAPI GetSystemDpiForProcess( HANDLE process )
{
    TRACE( "process %p\n", process );

    /* Return system DPI for the process */
    return DEFAULT_DPI;
}

/***********************************************************************
 *          GetDpiHostingBehavior
 */
DPI_HOSTING_BEHAVIOR WINAPI GetDpiHostingBehavior(void)
{
    TRACE( "returning default behavior\n" );
    return DPI_HOSTING_BEHAVIOR_DEFAULT;
}

/***********************************************************************
 *          SetDpiHostingBehavior
 */
DPI_HOSTING_BEHAVIOR WINAPI SetDpiHostingBehavior( DPI_HOSTING_BEHAVIOR behavior )
{
    TRACE( "behavior %d\n", behavior );

    /* For now, just accept it and return previous (default) */
    return DPI_HOSTING_BEHAVIOR_DEFAULT;
}

/***********************************************************************
 *          EnableNonClientDpiScaling
 */
BOOL WINAPI EnableNonClientDpiScaling( HWND hwnd )
{
    TRACE( "hwnd %p\n", hwnd );

    if (!IsWindow( hwnd ))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }

    /* Accept the call - actual DPI scaling would be done by window manager */
    return TRUE;
}

/***********************************************************************
 *          AdjustWindowRectExForDpi
 */
BOOL WINAPI AdjustWindowRectExForDpi( RECT *rect, DWORD style, BOOL menu, DWORD exstyle, UINT dpi )
{
    TRACE( "rect %p, style %#lx, menu %d, exstyle %#lx, dpi %u\n", rect, style, menu, exstyle, dpi );

    if (!rect)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /* For now, just call the standard version - proper implementation would scale based on DPI */
    return AdjustWindowRectEx( rect, style, menu, exstyle );
}

/***********************************************************************
 *          SystemParametersInfoForDpi
 */
BOOL WINAPI SystemParametersInfoForDpi( UINT action, UINT param, void *ptr, UINT flags, UINT dpi )
{
    TRACE( "action %u, param %u, ptr %p, flags %#x, dpi %u\n", action, param, ptr, flags, dpi );

    /* For now, call standard SystemParametersInfo - proper implementation would scale based on DPI */
    return SystemParametersInfoW( action, param, ptr, flags );
}
