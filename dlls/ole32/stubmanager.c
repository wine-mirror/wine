/*
 * A stub manager is an object that controls interface stubs. It is
 * identified by an OID (object identifier) and acts as the network
 * identity of the object. There can be many stub managers in a
 * process or apartment.
 *
 * Copyright 2002 Marcus Meissner
 * Copyright 2004 Mike Hearn for CodeWeavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "ole2ver.h"
#include "rpc.h"
#include "wine/debug.h"
#include "compobj_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

struct stub_manager *new_stub_manager(APARTMENT *apt, IUnknown *object)
{
    struct stub_manager *sm;

    assert( apt );
    
    sm = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct stub_manager));
    if (!sm) return NULL;

    list_init(&sm->ifstubs);
    InitializeCriticalSection(&sm->lock);
    IUnknown_AddRef(object);
    sm->object = object;
    sm->apt    = apt;
    EnterCriticalSection(&apt->cs);
    sm->oid    = apt->oidc++;
    LeaveCriticalSection(&apt->cs);
    
    /* yes, that's right, this starts at zero. that's zero EXTERNAL
     * refs, ie nobody has unmarshalled anything yet. we can't have
     * negative refs because the stub manager cannot be explicitly
     * killed, it has to die by somebody unmarshalling then releasing
     * the marshalled ifptr.
     */
    sm->refcount = 0;

    EnterCriticalSection(&apt->cs);
    list_add_head(&apt->stubmgrs, &sm->entry);
    LeaveCriticalSection(&apt->cs);

    TRACE("Created new stub manager (oid=%s) at %p for object with IUnknown %p\n", wine_dbgstr_longlong(sm->oid), sm, object);
    
    return sm;
}

struct stub_manager *get_stub_manager_from_object(OXID oxid, void *object)
{
    struct stub_manager *result = NULL;
    struct list         *cursor;
    APARTMENT           *apt;

    if (!(apt = COM_ApartmentFromOXID(oxid, TRUE)))
    {
        WARN("Could not map OXID %s to apartment object\n", wine_dbgstr_longlong(oxid));
        return NULL;
    }

    EnterCriticalSection(&apt->cs);
    LIST_FOR_EACH( cursor, &apt->stubmgrs )
    {
        struct stub_manager *m = LIST_ENTRY( cursor, struct stub_manager, entry );

        if (m->object == object)
        {
            result = m;
            break;
        }
    }
    LeaveCriticalSection(&apt->cs);

    COM_ApartmentRelease(apt);
    
    TRACE("found %p from object %p\n", result, object);

    return result;    
}

struct stub_manager *get_stub_manager(OXID oxid, OID oid)
{
    struct stub_manager *result = NULL;
    struct list         *cursor;
    APARTMENT           *apt;

    if (!(apt = COM_ApartmentFromOXID(oxid, TRUE)))
    {
        WARN("Could not map OXID %s to apartment object\n", wine_dbgstr_longlong(oxid));
        return NULL;
    }
    
    EnterCriticalSection(&apt->cs);
    LIST_FOR_EACH( cursor, &apt->stubmgrs )
    {
        struct stub_manager *m = LIST_ENTRY( cursor, struct stub_manager, entry );

        if (m->oid == oid)
        {
            result = m;
            break;
        }
    }
    LeaveCriticalSection(&apt->cs);

    COM_ApartmentRelease(apt);

    TRACE("found %p from oid %s\n", result, wine_dbgstr_longlong(oid));

    return result;
}

/* add some external references (ie from a client that demarshalled an ifptr) */
int stub_manager_ref(struct stub_manager *m, int refs)
{
    int rc = InterlockedExchangeAdd(&m->refcount, refs) + refs;
    TRACE("added %d refs to %p (oid %s), rc is now %d\n", refs, m, wine_dbgstr_longlong(m->oid), rc);
    return rc;
}

/* remove some external references */
int stub_manager_unref(struct stub_manager *m, int refs)
{
    int rc = InterlockedExchangeAdd(&m->refcount, -refs) - refs;
    
    TRACE("removed %d refs from %p (oid %s), rc is now %d\n", refs, m, wine_dbgstr_longlong(m->oid), rc);
    
    if (rc == 0)
    {
        TRACE("destroying %p (oid=%s)\n", m, wine_dbgstr_longlong(m->oid));

        EnterCriticalSection(&m->apt->cs);
        list_remove(&m->entry);
        LeaveCriticalSection(&m->apt->cs);

        /* table strong and normal marshals have a ref on us, so we
         * can't die while they are outstanding unless the app does
         * something weird like explicitly killing us (how?)
         */

        EnterCriticalSection(&m->lock);
        if (!list_empty(&m->ifstubs))
        {
            ERR("PANIC: Stub manager %s is being destroyed with outstanding interface stubs\n", wine_dbgstr_longlong(m->oid));
            
            /* assert( FALSE ); */
            /* fixme: this will happen sometimes until we do proper lifecycle management via IRemUnknown */
        }

        /* fixme: the lifecycle of table-weak marshals is not
         * currently understood. results of testing against dcom98
         * appear to contradict Essential COM   -m
         */
        
        LeaveCriticalSection(&m->lock);        

        IUnknown_Release(m->object);

        HeapFree(GetProcessHeap(), 0, m);
    }

    return refs;
}

static struct ifstub *stub_manager_iid_to_ifstub(struct stub_manager *m, IID *iid)
{
    struct list    *cursor;
    struct ifstub  *result = NULL;
    
    EnterCriticalSection(&m->lock);
    LIST_FOR_EACH( cursor, &m->ifstubs )
    {
        struct ifstub *ifstub = LIST_ENTRY( cursor, struct ifstub, entry );

        if (IsEqualIID(iid, &ifstub->iid))
        {
            result = ifstub;
            break;
        }
    }
    LeaveCriticalSection(&m->lock);

    return result;
}

IRpcStubBuffer *stub_manager_iid_to_stubbuffer(struct stub_manager *m, IID *iid)
{
    struct ifstub *ifstub = stub_manager_iid_to_ifstub(m, iid);
    
    return ifstub ? ifstub->stubbuffer : NULL;
}

/* registers a new interface stub COM object with the stub manager and returns registration record */
struct ifstub *stub_manager_new_ifstub(struct stub_manager *m, IRpcStubBuffer *sb, IUnknown *iptr, IID *iid, BOOL tablemarshal)
{
    struct ifstub *stub;

    TRACE("oid=%s, stubbuffer=%p, iptr=%p, iid=%s, tablemarshal=%s\n",
          wine_dbgstr_longlong(m->oid), sb, iptr, debugstr_guid(iid), tablemarshal ? "TRUE" : "FALSE");

    stub = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct ifstub));
    if (!stub) return NULL;

    stub->stubbuffer = sb;
    IUnknown_AddRef(sb);

    /* no need to ref this, same object as sb */
    stub->iface = iptr;
    stub->table = tablemarshal;
    stub->iid = *iid;
    
    EnterCriticalSection(&m->lock);
    list_add_head(&m->ifstubs, &stub->entry);
    LeaveCriticalSection(&m->lock);

    return stub;
}

/* fixme: should ifstubs be refcounted? iid should be ipid */
void stub_manager_delete_ifstub(struct stub_manager *m, IID *iid)
{
    struct ifstub *ifstub;

    TRACE("m=%p, m->oid=%s, iid=%s\n", m, wine_dbgstr_longlong(m->oid), debugstr_guid(iid));
    
    EnterCriticalSection(&m->lock);
    
    if ((ifstub = stub_manager_iid_to_ifstub(m, iid)))
    {
        list_remove(&ifstub->entry);
        
        IUnknown_Release(ifstub->stubbuffer);
        IUnknown_Release(ifstub->iface);
        
        HeapFree(GetProcessHeap(), 0, ifstub);
    }
    else
    {
        WARN("could not map iid %s to ifstub\n", debugstr_guid(iid));
    }
    
    LeaveCriticalSection(&m->lock);
}
