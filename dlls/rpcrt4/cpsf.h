/*
 * COM proxy definitions
 *
 * Copyright 2001 Ove Kåven, TransGaming Technologies
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

#ifndef __WINE_CPSF_H
#define __WINE_CPSF_H

typedef struct
{
    IRpcProxyBuffer IRpcProxyBuffer_iface;
    void **PVtbl;
    LONG RefCount;
    const IID *piid;
    IUnknown *pUnkOuter;
    /* offset of base_object from PVtbl must match assembly thunks; see
     * fill_delegated_proxy_table() */
    IUnknown *base_object;
    IRpcProxyBuffer *base_proxy;
    PCInterfaceName name;
    IPSFactoryBuffer *pPSFactory;
    IRpcChannelBuffer *pChannel;
} StdProxyImpl;

typedef struct
{
    IUnknown base_obj;
    IRpcStubBuffer *base_stub;
    CStdStubBuffer stub_buffer;
} cstdstubbuffer_delegating_t;

HRESULT StdProxy_Construct(REFIID riid, LPUNKNOWN pUnkOuter, const ProxyFileInfo *ProxyInfo,
                           int Index, LPPSFACTORYBUFFER pPSFactory, LPRPCPROXYBUFFER *ppProxy,
                           LPVOID *ppvObj);
HRESULT WINAPI StdProxy_QueryInterface(IRpcProxyBuffer *iface, REFIID iid, void **obj);
ULONG WINAPI StdProxy_AddRef(IRpcProxyBuffer *iface);
HRESULT WINAPI StdProxy_Connect(IRpcProxyBuffer *iface, IRpcChannelBuffer *channel);
void WINAPI StdProxy_Disconnect(IRpcProxyBuffer *iface);

HRESULT CStdStubBuffer_Construct(REFIID riid, LPUNKNOWN pUnkServer, PCInterfaceName name,
                                 CInterfaceStubVtbl *vtbl, LPPSFACTORYBUFFER pPSFactory,
                                 LPRPCSTUBBUFFER *ppStub);

HRESULT CStdStubBuffer_Delegating_Construct(REFIID riid, LPUNKNOWN pUnkServer, PCInterfaceName name,
                                            CInterfaceStubVtbl *vtbl, REFIID delegating_iid,
                                            LPPSFACTORYBUFFER pPSFactory, LPRPCSTUBBUFFER *ppStub);

const MIDL_SERVER_INFO *CStdStubBuffer_GetServerInfo(IRpcStubBuffer *iface);

extern const IRpcStubBufferVtbl CStdStubBuffer_Vtbl;
extern const IRpcStubBufferVtbl CStdStubBuffer_Delegating_Vtbl;

BOOL fill_delegated_proxy_table(IUnknownVtbl *vtbl, DWORD num);
HRESULT create_proxy(REFIID iid, IUnknown *pUnkOuter, IRpcProxyBuffer **pproxy, void **ppv);
HRESULT create_stub(REFIID iid, IUnknown *pUnk, IRpcStubBuffer **ppstub);
BOOL fill_stubless_table(IUnknownVtbl *vtbl, DWORD num);
const IUnknownVtbl *get_delegating_vtbl(DWORD num_methods);

#define THUNK_ENTRY_FIRST_BLOCK() \
    THUNK_ENTRY(3) \
    THUNK_ENTRY(4) \
    THUNK_ENTRY(5) \
    THUNK_ENTRY(6) \
    THUNK_ENTRY(7) \
    THUNK_ENTRY(8) \
    THUNK_ENTRY(9) \
    THUNK_ENTRY(10) \
    THUNK_ENTRY(11) \
    THUNK_ENTRY(12) \
    THUNK_ENTRY(13) \
    THUNK_ENTRY(14) \
    THUNK_ENTRY(15) \
    THUNK_ENTRY(16) \
    THUNK_ENTRY(17) \
    THUNK_ENTRY(18) \
    THUNK_ENTRY(19) \
    THUNK_ENTRY(20) \
    THUNK_ENTRY(21) \
    THUNK_ENTRY(22) \
    THUNK_ENTRY(23) \
    THUNK_ENTRY(24) \
    THUNK_ENTRY(25) \
    THUNK_ENTRY(26) \
    THUNK_ENTRY(27) \
    THUNK_ENTRY(28) \
    THUNK_ENTRY(29) \
    THUNK_ENTRY(30) \
    THUNK_ENTRY(31)

#define THUNK_ENTRY_BLOCK(block) \
    THUNK_ENTRY(32 * (block) + 0) \
    THUNK_ENTRY(32 * (block) + 1) \
    THUNK_ENTRY(32 * (block) + 2) \
    THUNK_ENTRY(32 * (block) + 3) \
    THUNK_ENTRY(32 * (block) + 4) \
    THUNK_ENTRY(32 * (block) + 5) \
    THUNK_ENTRY(32 * (block) + 6) \
    THUNK_ENTRY(32 * (block) + 7) \
    THUNK_ENTRY(32 * (block) + 8) \
    THUNK_ENTRY(32 * (block) + 9) \
    THUNK_ENTRY(32 * (block) + 10) \
    THUNK_ENTRY(32 * (block) + 11) \
    THUNK_ENTRY(32 * (block) + 12) \
    THUNK_ENTRY(32 * (block) + 13) \
    THUNK_ENTRY(32 * (block) + 14) \
    THUNK_ENTRY(32 * (block) + 15) \
    THUNK_ENTRY(32 * (block) + 16) \
    THUNK_ENTRY(32 * (block) + 17) \
    THUNK_ENTRY(32 * (block) + 18) \
    THUNK_ENTRY(32 * (block) + 19) \
    THUNK_ENTRY(32 * (block) + 20) \
    THUNK_ENTRY(32 * (block) + 21) \
    THUNK_ENTRY(32 * (block) + 22) \
    THUNK_ENTRY(32 * (block) + 23) \
    THUNK_ENTRY(32 * (block) + 24) \
    THUNK_ENTRY(32 * (block) + 25) \
    THUNK_ENTRY(32 * (block) + 26) \
    THUNK_ENTRY(32 * (block) + 27) \
    THUNK_ENTRY(32 * (block) + 28) \
    THUNK_ENTRY(32 * (block) + 29) \
    THUNK_ENTRY(32 * (block) + 30) \
    THUNK_ENTRY(32 * (block) + 31)

#define ALL_THUNK_ENTRIES \
    THUNK_ENTRY_FIRST_BLOCK() \
    THUNK_ENTRY_BLOCK(1) \
    THUNK_ENTRY_BLOCK(2) \
    THUNK_ENTRY_BLOCK(3) \
    THUNK_ENTRY_BLOCK(4) \
    THUNK_ENTRY_BLOCK(5) \
    THUNK_ENTRY_BLOCK(6) \
    THUNK_ENTRY_BLOCK(7) \
    THUNK_ENTRY_BLOCK(8) \
    THUNK_ENTRY_BLOCK(9) \
    THUNK_ENTRY_BLOCK(10) \
    THUNK_ENTRY_BLOCK(11) \
    THUNK_ENTRY_BLOCK(12) \
    THUNK_ENTRY_BLOCK(13) \
    THUNK_ENTRY_BLOCK(14) \
    THUNK_ENTRY_BLOCK(15) \
    THUNK_ENTRY_BLOCK(16) \
    THUNK_ENTRY_BLOCK(17) \
    THUNK_ENTRY_BLOCK(18) \
    THUNK_ENTRY_BLOCK(19) \
    THUNK_ENTRY_BLOCK(20) \
    THUNK_ENTRY_BLOCK(21) \
    THUNK_ENTRY_BLOCK(22) \
    THUNK_ENTRY_BLOCK(23) \
    THUNK_ENTRY_BLOCK(24) \
    THUNK_ENTRY_BLOCK(25) \
    THUNK_ENTRY_BLOCK(26) \
    THUNK_ENTRY_BLOCK(27) \
    THUNK_ENTRY_BLOCK(28) \
    THUNK_ENTRY_BLOCK(29) \
    THUNK_ENTRY_BLOCK(30) \
    THUNK_ENTRY_BLOCK(31)

#define NB_THUNK_ENTRIES (32 * 32)

#endif  /* __WINE_CPSF_H */
