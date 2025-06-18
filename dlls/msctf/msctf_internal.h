/*
 * Internal header for msctf.dll
 *
 * Copyright 2008 Aric Stewart, CodeWeavers
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

#ifndef __WINE_MSCTF_I_H
#define __WINE_MSCTF_I_H

#include "wine/list.h"

#define COOKIE_MAGIC_TMSINK  0x0010
#define COOKIE_MAGIC_CONTEXTSINK 0x0020
#define COOKIE_MAGIC_GUIDATOM 0x0030
#define COOKIE_MAGIC_IPPSINK 0x0040
#define COOKIE_MAGIC_EDITCOOKIE 0x0050
#define COOKIE_MAGIC_COMPARTMENTSINK 0x0060
#define COOKIE_MAGIC_DMSINK 0x0070
#define COOKIE_MAGIC_THREADFOCUSSINK 0x0080
#define COOKIE_MAGIC_KEYTRACESINK 0x0090
#define COOKIE_MAGIC_UIELEMENTSINK 0x00a0
#define COOKIE_MAGIC_INPUTPROCESSORPROFILEACTIVATIONSINK 0x00b0
#define COOKIE_MAGIC_ACTIVELANGSINK 0x00c0

extern DWORD tlsIndex;
extern TfClientId processId;
extern ITfCompartmentMgr *globalCompartmentMgr;

extern HRESULT ThreadMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut);
extern HRESULT DocumentMgr_Constructor(ITfThreadMgrEventSink*, ITfDocumentMgr **ppOut);
extern HRESULT Context_Constructor(TfClientId tidOwner, IUnknown *punk, ITfDocumentMgr *mgr, ITfContext **ppOut, TfEditCookie *pecTextStore);
extern HRESULT InputProcessorProfiles_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut);
extern HRESULT CategoryMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut);
extern HRESULT Range_Constructor(ITfContext *context, DWORD anchorStart, DWORD anchorEnd, ITfRange **ppOut);
extern HRESULT CompartmentMgr_Constructor(IUnknown *pUnkOuter, REFIID riid, IUnknown **ppOut);
extern HRESULT CompartmentMgr_Destructor(ITfCompartmentMgr *This);
extern HRESULT LangBarMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut);
extern HRESULT DisplayAttributeMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut);

extern HRESULT Context_Initialize(ITfContext *cxt, ITfDocumentMgr *manager);
extern HRESULT Context_Uninitialize(ITfContext *cxt);
extern void    ThreadMgr_OnDocumentMgrDestruction(ITfThreadMgr *tm, ITfDocumentMgr *mgr);
extern HRESULT TF_SELECTION_to_TS_SELECTION_ACP(const TF_SELECTION *tf, TS_SELECTION_ACP *tsAcp);

/* cookie function */
extern DWORD  generate_Cookie(DWORD magic, LPVOID data);
extern DWORD  get_Cookie_magic(DWORD id);
extern LPVOID get_Cookie_data(DWORD id);
extern LPVOID remove_Cookie(DWORD id);
extern DWORD enumerate_Cookie(DWORD magic, DWORD *index);

/* activated text services functions */
extern HRESULT add_active_textservice(TF_LANGUAGEPROFILE *lp);
extern BOOL get_active_textservice(REFCLSID rclsid, TF_LANGUAGEPROFILE *lp);
extern HRESULT activate_textservices(ITfThreadMgrEx *tm);
extern HRESULT deactivate_textservices(void);

extern CLSID get_textservice_clsid(TfClientId tid);
extern HRESULT get_textservice_sink(TfClientId tid, REFCLSID iid, IUnknown** sink);
extern HRESULT set_textservice_sink(TfClientId tid, REFCLSID iid, IUnknown* sink);

typedef struct {
    struct list entry;
    union {
        IUnknown *pIUnknown;
        ITfThreadMgrEventSink *pITfThreadMgrEventSink;
        ITfCompartmentEventSink *pITfCompartmentEventSink;
        ITfTextEditSink *pITfTextEditSink;
        ITfLanguageProfileNotifySink *pITfLanguageProfileNotifySink;
        ITfTransitoryExtensionSink *pITfTransitoryExtensionSink;
    } interfaces;
} Sink;

#define SINK_ENTRY(cursor,type) (LIST_ENTRY(cursor,Sink,entry)->interfaces.p##type)
#define SINK_FOR_EACH(cursor,list,type,elem) \
    for ((cursor) = (list)->next; \
         (cursor) != (list) && (elem = SINK_ENTRY(cursor, type), 1); \
         (cursor) = (cursor)->next)

HRESULT advise_sink(struct list *sink_list, REFIID riid, DWORD cookie_magic, IUnknown *unk, DWORD *cookie);
HRESULT unadvise_sink(DWORD cookie);
void free_sinks(struct list *sink_list);

extern const WCHAR szwSystemTIPKey[];
extern const WCHAR szwSystemCTFKey[];

#endif /* __WINE_MSCTF_I_H */
