/*
 * Xbox One CreateXAudio2Object entry point for xaudio2_9
 *
 * This is the Xbox ERA variant of XAudio2Create. The extra
 * pSharedShapeContexts parameter is Xbox-graphics-context specific
 * and is ignored on PC/Wine.
 *
 * Reference: XWine1/XboxAudio2 (MIT)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "objbase.h"

/* xaudio_private.h is in the PARENTSRC (xaudio2_7) directory */
#include "xaudio_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xaudio2);

/* Declared in xaudio_dll.c (PARENTSRC) */
extern HRESULT WINAPI XAudio2Create(IXAudio2 **ppxa2, UINT32 flags,
        XAUDIO2_PROCESSOR proc);

/***********************************************************************
 *   CreateXAudio2Object  (xaudio2_9.@)
 *
 * Xbox One ERA entry point. pSharedShapeContexts is an Xbox-specific
 * graphics-context sharing parameter; ignored on PC.
 */
HRESULT WINAPI CreateXAudio2Object(IXAudio2 **ppXAudio2, UINT32 Flags,
        XAUDIO2_PROCESSOR XAudio2Processor, void *pSharedShapeContexts)
{
    TRACE("(%p, %u, %u, %p)\n", ppXAudio2, Flags, XAudio2Processor,
          pSharedShapeContexts);
    return XAudio2Create(ppXAudio2, Flags, XAudio2Processor);
}
