/*
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 Justin Bradford
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Sylvain St-Germain
 * Copyright 2002 Marcus Meissner
 * Copyright 2003 Ove Kåven, TransGaming Technologies
 * Copyright 2004 Mike Hearn, Rob Shearman, CodeWeavers Inc
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

#ifndef __WINE_OLE_COMPOBJ_H
#define __WINE_OLE_COMPOBJ_H

/* All private prototype functions used by OLE will be added to this header file */

#include <stdarg.h>

#include "wine/list.h"
#include "wine/heap.h"

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "dcom.h"
#include "winreg.h"
#include "winternl.h"

DEFINE_OLEGUID( CLSID_DfMarshal, 0x0000030b, 0, 0 );

/* this is what is stored in TEB->ReservedForOle */
struct oletls
{
    struct apartment *apt;
    IErrorInfo       *errorinfo;   /* see errorinfo.c */
    DWORD             thread_seqid;/* returned with CoGetCurrentProcess */
    DWORD             apt_mask;    /* apartment mask (+0Ch on x86) */
    void            *unknown0;
    DWORD            inits;        /* number of times CoInitializeEx called */
    DWORD            ole_inits;    /* number of times OleInitialize called */
    GUID             causality_id; /* unique identifier for each COM call */
    LONG             pending_call_count_client; /* number of client calls pending */
    LONG             pending_call_count_server; /* number of server calls pending */
    DWORD            unknown;
    IObjContext     *context_token; /* (+38h on x86) */
    IUnknown        *call_state;    /* current call context (+3Ch on x86) */
    DWORD            unknown2[46];
    IUnknown        *cancel_object; /* cancel object set by CoSetCancelObject (+F8h on x86) */
    IUnknown        *state;       /* see CoSetState */
    struct list      spies;         /* Spies installed with CoRegisterInitializeSpy */
    DWORD            spies_lock;
    DWORD            cancelcount;
};

/* Global Interface Table Functions */
extern void release_std_git(void) DECLSPEC_HIDDEN;
extern HRESULT StdGlobalInterfaceTable_GetFactory(LPVOID *ppv) DECLSPEC_HIDDEN;

HRESULT COM_OpenKeyForCLSID(REFCLSID clsid, LPCWSTR keyname, REGSAM access, HKEY *key) DECLSPEC_HIDDEN;
HRESULT MARSHAL_GetStandardMarshalCF(LPVOID *ppv) DECLSPEC_HIDDEN;
HRESULT FTMarshalCF_Create(REFIID riid, LPVOID *ppv) DECLSPEC_HIDDEN;

/* Drag and drop */
void OLEDD_UnInitialize(void) DECLSPEC_HIDDEN;

extern HRESULT WINAPI InternalTlsAllocData(struct oletls **tlsdata);

/* will create if necessary */
static inline struct oletls *COM_CurrentInfo(void)
{
    struct oletls *oletls;

    if (!NtCurrentTeb()->ReservedForOle)
        InternalTlsAllocData(&oletls);

    return NtCurrentTeb()->ReservedForOle;
}

static inline struct apartment * COM_CurrentApt(void)
{  
    return COM_CurrentInfo()->apt;
}

#define CHARS_IN_GUID 39 /* including NULL */

/* from dlldata.c */
extern HINSTANCE hProxyDll DECLSPEC_HIDDEN;
extern HRESULT WINAPI OLE32_DllGetClassObject(REFCLSID rclsid, REFIID iid,LPVOID *ppv) DECLSPEC_HIDDEN;
extern HRESULT WINAPI OLE32_DllRegisterServer(void) DECLSPEC_HIDDEN;
extern HRESULT WINAPI OLE32_DllUnregisterServer(void) DECLSPEC_HIDDEN;

extern HRESULT Handler_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv) DECLSPEC_HIDDEN;
extern HRESULT HandlerCF_Create(REFCLSID rclsid, REFIID riid, LPVOID *ppv) DECLSPEC_HIDDEN;

extern HRESULT WINAPI GlobalOptions_CreateInstance(IClassFactory *iface, IUnknown *pUnk,
                                                   REFIID riid, void **ppv) DECLSPEC_HIDDEN;
extern IClassFactory GlobalOptionsCF DECLSPEC_HIDDEN;
extern HRESULT WINAPI GlobalInterfaceTable_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid,
        void **obj) DECLSPEC_HIDDEN;
extern IClassFactory GlobalInterfaceTableCF DECLSPEC_HIDDEN;
extern HRESULT WINAPI ManualResetEvent_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid,
        void **obj) DECLSPEC_HIDDEN;
extern IClassFactory ManualResetEventCF DECLSPEC_HIDDEN;
extern HRESULT WINAPI Ole32DllGetClassObject(REFCLSID clsid, REFIID riid, void **obj) DECLSPEC_HIDDEN;

/* Exported non-interface Data Advise Holder functions */
HRESULT DataAdviseHolder_OnConnect(IDataAdviseHolder *iface, IDataObject *pDelegate) DECLSPEC_HIDDEN;
void DataAdviseHolder_OnDisconnect(IDataAdviseHolder *iface) DECLSPEC_HIDDEN;

extern UINT ownerlink_clipboard_format DECLSPEC_HIDDEN;
extern UINT filename_clipboard_format DECLSPEC_HIDDEN;
extern UINT filenameW_clipboard_format DECLSPEC_HIDDEN;
extern UINT dataobject_clipboard_format DECLSPEC_HIDDEN;
extern UINT embedded_object_clipboard_format DECLSPEC_HIDDEN;
extern UINT embed_source_clipboard_format DECLSPEC_HIDDEN;
extern UINT custom_link_source_clipboard_format DECLSPEC_HIDDEN;
extern UINT link_source_clipboard_format DECLSPEC_HIDDEN;
extern UINT object_descriptor_clipboard_format DECLSPEC_HIDDEN;
extern UINT link_source_descriptor_clipboard_format DECLSPEC_HIDDEN;
extern UINT ole_private_data_clipboard_format DECLSPEC_HIDDEN;

extern LSTATUS create_classes_key(HKEY, const WCHAR *, REGSAM, HKEY *) DECLSPEC_HIDDEN;
extern LSTATUS open_classes_key(HKEY, const WCHAR *, REGSAM, HKEY *) DECLSPEC_HIDDEN;

extern BOOL actctx_get_miscstatus(const CLSID*, DWORD, DWORD*) DECLSPEC_HIDDEN;

extern const char *debugstr_formatetc(const FORMATETC *formatetc) DECLSPEC_HIDDEN;

static inline HRESULT copy_formatetc(FORMATETC *dst, const FORMATETC *src)
{
    *dst = *src;
    if (src->ptd)
    {
        dst->ptd = CoTaskMemAlloc( src->ptd->tdSize );
        if (!dst->ptd) return E_OUTOFMEMORY;
        memcpy( dst->ptd, src->ptd, src->ptd->tdSize );
    }
    return S_OK;
}

extern HRESULT EnumSTATDATA_Construct(IUnknown *holder, ULONG index, DWORD array_len, STATDATA *data,
                                      BOOL copy, IEnumSTATDATA **ppenum) DECLSPEC_HIDDEN;

#endif /* __WINE_OLE_COMPOBJ_H */
