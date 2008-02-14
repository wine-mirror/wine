/*
 *    MSXML Class Factory
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2005 Mike McCormack
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

#define COBJMACROS

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml.h"
#include "msxml2.h"

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);


static ITypeLib *typelib;
static ITypeInfo *typeinfos[LAST_tid];

static REFIID tid_ids[] = {
    &IID_IXMLDOMAttribute,
    &IID_IXMLDOMCDATASection,
    &IID_IXMLDOMComment,
    &IID_IXMLDOMDocument2,
    &IID_IXMLDOMDocumentFragment,
    &IID_IXMLDOMElement,
    &IID_IXMLDOMEntityReference,
    &IID_IXMLDOMNamedNodeMap,
    &IID_IXMLDOMNodeList,
    &IID_IXMLDOMParseError,
    &IID_IXMLDOMProcessingInstruction,
    &IID_IXMLDOMSchemaCollection,
    &IID_IXMLDOMText,
    &IID_IXMLElement,
    &IID_IXMLDOMDocument
};

HRESULT get_typeinfo(enum tid_t tid, ITypeInfo **typeinfo)
{
    HRESULT hres;

    if(!typelib) {
        ITypeLib *tl;

        hres = LoadRegTypeLib(&LIBID_MSXML2, 3, 0, LOCALE_SYSTEM_DEFAULT, &tl);
        if(FAILED(hres)) {
            ERR("LoadRegTypeLib failed: %08x\n", hres);
            return hres;
        }

        if(InterlockedCompareExchangePointer((void**)&typelib, tl, NULL))
            ITypeLib_Release(tl);
    }

    if(!typeinfos[tid]) {
        ITypeInfo *typeinfo;

        hres = ITypeLib_GetTypeInfoOfGuid(typelib, tid_ids[tid], &typeinfo);
        if(FAILED(hres)) {
            ERR("GetTypeInfoOfGuid failed: %08x\n", hres);
            return hres;
        }

        if(InterlockedCompareExchangePointer((void**)(typeinfos+tid), typeinfo, NULL))
            ITypeInfo_Release(typeinfo);
    }

    *typeinfo = typeinfos[tid];

    ITypeInfo_AddRef(typeinfos[tid]);
    return S_OK;
}

static void process_detach(void)
{
    if(typelib) {
        unsigned i;

        for(i=0; i < sizeof(typeinfos)/sizeof(*typeinfos); i++)
            if(typeinfos[i])
                ITypeInfo_Release(typeinfos[i]);

        ITypeLib_Release(typelib);
    }
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    FIXME("\n");
    return S_FALSE;
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
#ifdef HAVE_LIBXML2
        xmlInitParser();
#endif
        DisableThreadLibraryCalls(hInstDLL);
        break;
    case DLL_PROCESS_DETACH:
#ifdef HAVE_LIBXML2
        xmlCleanupParser();
        process_detach();
#endif
        break;
    }
    return TRUE;
}
