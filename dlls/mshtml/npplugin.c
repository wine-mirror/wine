/*
 * Copyright 2010 Jacek Caban for CodeWeavers
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "shlobj.h"

#include "mshtml_private.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

/* Parts of npapi.h */

#define NPERR_BASE                         0
#define NPERR_NO_ERROR                    (NPERR_BASE + 0)
#define NPERR_GENERIC_ERROR               (NPERR_BASE + 1)
#define NPERR_INVALID_INSTANCE_ERROR      (NPERR_BASE + 2)
#define NPERR_INVALID_FUNCTABLE_ERROR     (NPERR_BASE + 3)
#define NPERR_MODULE_LOAD_FAILED_ERROR    (NPERR_BASE + 4)
#define NPERR_OUT_OF_MEMORY_ERROR         (NPERR_BASE + 5)
#define NPERR_INVALID_PLUGIN_ERROR        (NPERR_BASE + 6)
#define NPERR_INVALID_PLUGIN_DIR_ERROR    (NPERR_BASE + 7)
#define NPERR_INCOMPATIBLE_VERSION_ERROR  (NPERR_BASE + 8)
#define NPERR_INVALID_PARAM               (NPERR_BASE + 9)
#define NPERR_INVALID_URL                 (NPERR_BASE + 10)
#define NPERR_FILE_NOT_FOUND              (NPERR_BASE + 11)
#define NPERR_NO_DATA                     (NPERR_BASE + 12)
#define NPERR_STREAM_NOT_SEEKABLE         (NPERR_BASE + 13)

/* Parts of npfunctions.h */

typedef NPError (CDECL *NPP_NewProcPtr)(NPMIMEType,NPP,UINT16,INT16,char**,char**,NPSavedData*);
typedef NPError (CDECL *NPP_DestroyProcPtr)(NPP,NPSavedData**);
typedef NPError (CDECL *NPP_SetWindowProcPtr)(NPP,NPWindow*);
typedef NPError (CDECL *NPP_NewStreamProcPtr)(NPP,NPMIMEType,NPStream*,NPBool,UINT16*);
typedef NPError (CDECL *NPP_DestroyStreamProcPtr)(NPP,NPStream*,NPReason);
typedef INT32 (CDECL *NPP_WriteReadyProcPtr)(NPP,NPStream*);
typedef INT32 (CDECL *NPP_WriteProcPtr)(NPP,NPStream*,INT32,INT32,void*);
typedef void (CDECL *NPP_StreamAsFileProcPtr)(NPP,NPStream*,const char*);
typedef void (CDECL *NPP_PrintProcPtr)(NPP,NPPrint*);
typedef INT16 (CDECL *NPP_HandleEventProcPtr)(NPP,void*);
typedef void (CDECL *NPP_URLNotifyProcPtr)(NPP,const char*,NPReason,void*);
typedef NPError (CDECL *NPP_GetValueProcPtr)(NPP,NPPVariable,void*);
typedef NPError (CDECL *NPP_SetValueProcPtr)(NPP,NPNVariable,void*);
typedef NPBool (CDECL *NPP_GotFocusPtr)(NPP,NPFocusDirection);
typedef void (CDECL *NPP_LostFocusPtr)(NPP);

typedef struct _NPPluginFuncs {
    UINT16 size;
    UINT16 version;
    NPP_NewProcPtr newp;
    NPP_DestroyProcPtr destroy;
    NPP_SetWindowProcPtr setwindow;
    NPP_NewStreamProcPtr newstream;
    NPP_DestroyStreamProcPtr destroystream;
    NPP_StreamAsFileProcPtr asfile;
    NPP_WriteReadyProcPtr writeready;
    NPP_WriteProcPtr write;
    NPP_PrintProcPtr print;
    NPP_HandleEventProcPtr event;
    NPP_URLNotifyProcPtr urlnotify;
    void *javaClass;
    NPP_GetValueProcPtr getvalue;
    NPP_SetValueProcPtr setvalue;
    NPP_GotFocusPtr gotfocus;
    NPP_LostFocusPtr lostfocus;
} NPPluginFuncs;

static NPError CDECL NPP_New(NPMIMEType pluginType, NPP instance, UINT16 mode, INT16 argc, char **argn,
        char **argv, NPSavedData *saved)
{
    FIXME("(%s %p %x %d %p %p %p)\n", debugstr_a(pluginType), instance, mode, argc, argn, argv, saved);
    return NPERR_GENERIC_ERROR;
}

static NPError CDECL NPP_Destroy(NPP instance, NPSavedData **save)
{
    FIXME("(%p %p)\n", instance, save);
    return NPERR_GENERIC_ERROR;
}

static NPError CDECL NPP_SetWindow(NPP instance, NPWindow *window)
{
    FIXME("(%p %p)\n", instance, window);
    return NPERR_GENERIC_ERROR;
}

static NPError CDECL NPP_NewStream(NPP instance, NPMIMEType type, NPStream *stream, NPBool seekable, UINT16 *stype)
{
    TRACE("\n");
    return NPERR_GENERIC_ERROR;
}

static NPError CDECL NPP_DestroyStream(NPP instance, NPStream *stream, NPReason reason)
{
    TRACE("\n");
    return NPERR_GENERIC_ERROR;
}

static INT32 CDECL NPP_WriteReady(NPP instance, NPStream *stream)
{
    TRACE("\n");
    return NPERR_GENERIC_ERROR;
}

static INT32 CDECL NPP_Write(NPP instance, NPStream *stream, INT32 offset, INT32 len, void *buffer)
{
    TRACE("\n");
    return NPERR_GENERIC_ERROR;
}

static void CDECL NPP_StreamAsFile(NPP instance, NPStream *stream, const char *fname)
{
    TRACE("\n");
}

static void CDECL NPP_Print(NPP instance, NPPrint *platformPrint)
{
    FIXME("\n");
}

static INT16 CDECL NPP_HandleEvent(NPP instance, void *event)
{
    TRACE("\n");
    return NPERR_GENERIC_ERROR;
}

static void CDECL NPP_URLNotify(NPP instance, const char *url, NPReason reason, void *notifyData)
{
    TRACE("\n");
}

static NPError CDECL NPP_GetValue(NPP instance, NPPVariable variable, void *ret_value)
{
    TRACE("\n");
    return NPERR_GENERIC_ERROR;
}

static NPError CDECL NPP_SetValue(NPP instance, NPNVariable variable, void *value)
{
    TRACE("\n");
    return NPERR_GENERIC_ERROR;
}

static NPBool CDECL NPP_GotFocus(NPP instance, NPFocusDirection direction)
{
    FIXME("\n");
    return NPERR_GENERIC_ERROR;
}

static void CDECL NPP_LostFocus(NPP instance)
{
    FIXME("\n");
}

/***********************************************************************
 *          NP_GetEntryPoints (npmshtml.@)
 */
NPError WINAPI NP_GetEntryPoints(NPPluginFuncs* funcs)
{
    TRACE("(%p)\n", funcs);

    funcs->version = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
    funcs->newp = NPP_New;
    funcs->destroy = NPP_Destroy;
    funcs->setwindow = NPP_SetWindow;
    funcs->newstream = NPP_NewStream;
    funcs->destroystream = NPP_DestroyStream;
    funcs->asfile = NPP_StreamAsFile;
    funcs->writeready = NPP_WriteReady;
    funcs->write = NPP_Write;
    funcs->print = NPP_Print;
    funcs->event = NPP_HandleEvent;
    funcs->urlnotify = NPP_URLNotify;
    funcs->javaClass = NULL;
    funcs->getvalue = NPP_GetValue;
    funcs->setvalue = NPP_SetValue;
    funcs->gotfocus = NPP_GotFocus;
    funcs->lostfocus = NPP_LostFocus;

    return NPERR_NO_ERROR;
}
