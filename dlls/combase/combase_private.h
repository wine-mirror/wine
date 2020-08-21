/*
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

#include "winternl.h"
#include "wine/orpc.h"

#include "wine/list.h"

struct apartment
{
    struct list entry;

    LONG  refs;              /* refcount of the apartment (LOCK) */
    BOOL multi_threaded;     /* multi-threaded or single-threaded apartment? (RO) */
    DWORD tid;               /* thread id (RO) */
    OXID oxid;               /* object exporter ID (RO) */
    LONG ipidc;              /* interface pointer ID counter, starts at 1 (LOCK) */
    CRITICAL_SECTION cs;     /* thread safety */
    struct list proxies;     /* imported objects (CS cs) */
    struct list stubmgrs;    /* stub managers for exported objects (CS cs) */
    BOOL remunk_exported;    /* has the IRemUnknown interface for this apartment been created yet? (CS cs) */
    LONG remoting_started;   /* has the RPC system been started for this apartment? (LOCK) */
    struct list loaded_dlls; /* list of dlls loaded by this apartment (CS cs) */
    DWORD host_apt_tid;      /* thread ID of apartment hosting objects of differing threading model (CS cs) */
    HWND host_apt_hwnd;      /* handle to apartment window of host apartment (CS cs) */
    struct local_server *local_server; /* A marshallable object exposing local servers (CS cs) */
    BOOL being_destroyed;    /* is currently being destroyed */

    /* FIXME: OIDs should be given out by RPCSS */
    OID oidc;                /* object ID counter, starts at 1, zero is invalid OID (CS cs) */

    /* STA-only fields */
    HWND win;                /* message window (LOCK) */
    IMessageFilter *filter;  /* message filter (CS cs) */
    BOOL main;               /* is this a main-threaded-apartment? (RO) */

    /* MTA-only */
    struct list usage_cookies; /* Used for refcount control with CoIncrementMTAUsage()/CoDecrementMTAUsage(). */
};

/* this is what is stored in TEB->ReservedForOle */
struct tlsdata
{
    struct apartment *apt;
    IErrorInfo       *errorinfo;
    DWORD             thread_seqid;/* returned with CoGetCurrentProcess */
    DWORD             apt_mask;    /* apartment mask (+0Ch on x86) */
    void             *unknown0;
    DWORD             inits;        /* number of times CoInitializeEx called */
    DWORD             ole_inits;    /* number of times OleInitialize called */
    GUID              causality_id; /* unique identifier for each COM call */
    LONG              pending_call_count_client; /* number of client calls pending */
    LONG              pending_call_count_server; /* number of server calls pending */
    DWORD             unknown;
    IObjContext      *context_token; /* (+38h on x86) */
    IUnknown         *call_state;    /* current call context (+3Ch on x86) */
    DWORD             unknown2[46];
    IUnknown         *cancel_object; /* cancel object set by CoSetCancelObject (+F8h on x86) */
    IUnknown         *state;         /* see CoSetState */
    struct list       spies;         /* Spies installed with CoRegisterInitializeSpy */
    DWORD             spies_lock;
};

extern HRESULT WINAPI InternalTlsAllocData(struct tlsdata **data);

static inline HRESULT com_get_tlsdata(struct tlsdata **data)
{
    *data = NtCurrentTeb()->ReservedForOle;
    return *data ? S_OK : InternalTlsAllocData(data);
}

static inline struct apartment* com_get_current_apt(void)
{
    struct tlsdata *tlsdata = NULL;
    com_get_tlsdata(&tlsdata);
    return tlsdata->apt;
}

/* RpcSs interface */
DWORD rpcss_get_next_seqid(void) DECLSPEC_HIDDEN;
