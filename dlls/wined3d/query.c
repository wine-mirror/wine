/*
 * IWineD3DQuery implementation
 *
 * Copyright 2005 Oliver Stieber
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2009 Henri Verbeet for CodeWeavers.
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
#include "wined3d_private.h"

/*
 * Occlusion Queries:
 * http://www.gris.uni-tuebingen.de/~bartz/Publications/paper/hww98.pdf
 * http://oss.sgi.com/projects/ogl-sample/registry/ARB/occlusion_query.txt
 */

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
#define GLINFO_LOCATION This->device->adapter->gl_info

/* *******************************************
   IWineD3DQuery IUnknown parts follow
   ******************************************* */
static HRESULT  WINAPI IWineD3DQueryImpl_QueryInterface(IWineD3DQuery *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DQuery)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG  WINAPI IWineD3DQueryImpl_AddRef(IWineD3DQuery *iface) {
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    TRACE("(%p) : AddRef increasing from %d\n", This, This->ref);
    return InterlockedIncrement(&This->ref);
}

static ULONG  WINAPI IWineD3DQueryImpl_Release(IWineD3DQuery *iface) {
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %d\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
        /* Queries are specific to the GL context that created them. Not
         * deleting the query will obviously leak it, but that's still better
         * than potentially deleting a different query with the same id in this
         * context, and (still) leaking the actual query. */
        if (This->type == WINED3DQUERYTYPE_EVENT)
        {
            struct wined3d_event_query *query = This->extendedData;

            if (query->context) context_free_event_query(query);
        }
        else if (This->type == WINED3DQUERYTYPE_OCCLUSION)
        {
            struct wined3d_occlusion_query *query = This->extendedData;

            if (query->context) context_free_occlusion_query(query);
        }

        HeapFree(GetProcessHeap(), 0, This->extendedData);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* *******************************************
   IWineD3DQuery IWineD3DQuery parts follow
   ******************************************* */
static HRESULT WINAPI IWineD3DQueryImpl_GetParent(IWineD3DQuery *iface, IUnknown **parent)
{
    TRACE("iface %p, parent %p.\n", iface, parent);

    *parent = (IUnknown *)parent;
    IUnknown_AddRef(*parent);

    TRACE("Returning %p.\n", *parent);

    return WINED3D_OK;
}

static HRESULT  WINAPI IWineD3DOcclusionQueryImpl_GetData(IWineD3DQuery* iface, void* pData, DWORD dwSize, DWORD dwGetDataFlags) {
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *) iface;
    struct wined3d_occlusion_query *query = This->extendedData;
    IWineD3DDeviceImpl *device = This->device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_context *context;
    DWORD* data = pData;
    GLuint available;
    GLuint samples;
    HRESULT res;

    TRACE("(%p) : type D3DQUERY_OCCLUSION, pData %p, dwSize %#x, dwGetDataFlags %#x\n", This, pData, dwSize, dwGetDataFlags);

    if (!query->context) This->state = QUERY_CREATED;

    if (This->state == QUERY_CREATED)
    {
        /* D3D allows GetData on a new query, OpenGL doesn't. So just invent the data ourselves */
        TRACE("Query wasn't yet started, returning S_OK\n");
        if(data) *data = 0;
        return S_OK;
    }

    if (This->state == QUERY_BUILDING)
    {
        /* Msdn says this returns an error, but our tests show that S_FALSE is returned */
        TRACE("Query is building, returning S_FALSE\n");
        return S_FALSE;
    }

    if (!gl_info->supported[ARB_OCCLUSION_QUERY])
    {
        WARN("(%p) : Occlusion queries not supported. Returning 1.\n", This);
        *data = 1;
        return S_OK;
    }

    if (query->context->tid != GetCurrentThreadId())
    {
        FIXME("%p Wrong thread, returning 1.\n", This);
        *data = 1;
        return S_OK;
    }

    context = context_acquire(This->device, query->context->current_rt, CTXUSAGE_RESOURCELOAD);

    ENTER_GL();

    GL_EXTCALL(glGetQueryObjectuivARB(query->id, GL_QUERY_RESULT_AVAILABLE_ARB, &available));
    checkGLcall("glGetQueryObjectuivARB(GL_QUERY_RESULT_AVAILABLE)");
    TRACE("(%p) : available %d.\n", This, available);

    if (available)
    {
        if (data)
        {
            GL_EXTCALL(glGetQueryObjectuivARB(query->id, GL_QUERY_RESULT_ARB, &samples));
            checkGLcall("glGetQueryObjectuivARB(GL_QUERY_RESULT)");
            TRACE("(%p) : Returning %d samples.\n", This, samples);
            *data = samples;
        }
        res = S_OK;
    }
    else
    {
        res = S_FALSE;
    }

    LEAVE_GL();

    context_release(context);

    return res;
}

static HRESULT  WINAPI IWineD3DEventQueryImpl_GetData(IWineD3DQuery* iface, void* pData, DWORD dwSize, DWORD dwGetDataFlags) {
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *) iface;
    struct wined3d_event_query *query = This->extendedData;
    struct wined3d_context *context;
    BOOL *data = pData;

    TRACE("(%p) : type D3DQUERY_EVENT, pData %p, dwSize %#x, dwGetDataFlags %#x\n", This, pData, dwSize, dwGetDataFlags);

    if (!pData || !dwSize) return S_OK;

    if (!query->context)
    {
        TRACE("Query not started, returning TRUE.\n");
        *data = TRUE;

        return S_OK;
    }

    if (query->context->tid != GetCurrentThreadId())
    {
        /* See comment in IWineD3DQuery::Issue, event query codeblock */
        FIXME("Wrong thread, reporting GPU idle.\n");
        *data = TRUE;

        return S_OK;
    }

    context = context_acquire(This->device, query->context->current_rt, CTXUSAGE_RESOURCELOAD);

    ENTER_GL();

    if (context->gl_info->supported[APPLE_FENCE])
    {
        *data = GL_EXTCALL(glTestFenceAPPLE(query->id));
        checkGLcall("glTestFenceAPPLE");
    }
    else if (context->gl_info->supported[NV_FENCE])
    {
        *data = GL_EXTCALL(glTestFenceNV(query->id));
        checkGLcall("glTestFenceNV");
    }
    else
    {
        WARN("(%p): reporting GPU idle\n", This);
        *data = TRUE;
    }

    LEAVE_GL();

    context_release(context);

    return S_OK;
}

static DWORD  WINAPI IWineD3DEventQueryImpl_GetDataSize(IWineD3DQuery* iface){
    TRACE("(%p) : type D3DQUERY_EVENT\n", iface);

    return sizeof(BOOL);
}

static DWORD  WINAPI IWineD3DOcclusionQueryImpl_GetDataSize(IWineD3DQuery* iface){
    TRACE("(%p) : type D3DQUERY_OCCLUSION\n", iface);

    return sizeof(DWORD);
}

static WINED3DQUERYTYPE  WINAPI IWineD3DQueryImpl_GetType(IWineD3DQuery* iface){
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    return This->type;
}


static HRESULT  WINAPI IWineD3DEventQueryImpl_Issue(IWineD3DQuery* iface,  DWORD dwIssueFlags) {
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;

    TRACE("(%p) : dwIssueFlags %#x, type D3DQUERY_EVENT\n", This, dwIssueFlags);
    if (dwIssueFlags & WINED3DISSUE_END)
    {
        struct wined3d_event_query *query = This->extendedData;
        struct wined3d_context *context;

        if (query->context)
        {
            if (query->context->tid != GetCurrentThreadId())
            {
                context_free_event_query(query);
                context = context_acquire(This->device, NULL, CTXUSAGE_RESOURCELOAD);
                context_alloc_event_query(context, query);
            }
            else
            {
                context = context_acquire(This->device, query->context->current_rt, CTXUSAGE_RESOURCELOAD);
            }
        }
        else
        {
            context = context_acquire(This->device, NULL, CTXUSAGE_RESOURCELOAD);
            context_alloc_event_query(context, query);
        }

        ENTER_GL();

        if (context->gl_info->supported[APPLE_FENCE])
        {
            GL_EXTCALL(glSetFenceAPPLE(query->id));
            checkGLcall("glSetFenceAPPLE");
        }
        else if (context->gl_info->supported[NV_FENCE])
        {
            GL_EXTCALL(glSetFenceNV(query->id, GL_ALL_COMPLETED_NV));
            checkGLcall("glSetFenceNV");
        }

        LEAVE_GL();

        context_release(context);
    }
    else if(dwIssueFlags & WINED3DISSUE_BEGIN)
    {
        /* Started implicitly at device creation */
        ERR("Event query issued with START flag - what to do?\n");
    }

    if(dwIssueFlags & WINED3DISSUE_BEGIN) {
        This->state = QUERY_BUILDING;
    } else {
        This->state = QUERY_SIGNALLED;
    }

    return WINED3D_OK;
}

static HRESULT  WINAPI IWineD3DOcclusionQueryImpl_Issue(IWineD3DQuery* iface,  DWORD dwIssueFlags) {
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    IWineD3DDeviceImpl *device = This->device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;

    if (gl_info->supported[ARB_OCCLUSION_QUERY])
    {
        struct wined3d_occlusion_query *query = This->extendedData;
        struct wined3d_context *context;

        /* This is allowed according to msdn and our tests. Reset the query and restart */
        if (dwIssueFlags & WINED3DISSUE_BEGIN)
        {
            if (This->state == QUERY_BUILDING)
            {
                if (query->context->tid != GetCurrentThreadId())
                {
                    FIXME("Wrong thread, can't restart query.\n");

                    context_free_occlusion_query(query);
                    context = context_acquire(This->device, NULL, CTXUSAGE_RESOURCELOAD);
                    context_alloc_occlusion_query(context, query);
                }
                else
                {
                    context = context_acquire(This->device, query->context->current_rt, CTXUSAGE_RESOURCELOAD);

                    ENTER_GL();
                    GL_EXTCALL(glEndQueryARB(GL_SAMPLES_PASSED_ARB));
                    checkGLcall("glEndQuery()");
                    LEAVE_GL();
                }
            }
            else
            {
                if (query->context) context_free_occlusion_query(query);
                context = context_acquire(This->device, NULL, CTXUSAGE_RESOURCELOAD);
                context_alloc_occlusion_query(context, query);
            }

            ENTER_GL();
            GL_EXTCALL(glBeginQueryARB(GL_SAMPLES_PASSED_ARB, query->id));
            checkGLcall("glBeginQuery()");
            LEAVE_GL();

            context_release(context);
        }
        if (dwIssueFlags & WINED3DISSUE_END) {
            /* Msdn says _END on a non-building occlusion query returns an error, but
             * our tests show that it returns OK. But OpenGL doesn't like it, so avoid
             * generating an error
             */
            if (This->state == QUERY_BUILDING)
            {
                if (query->context->tid != GetCurrentThreadId())
                {
                    FIXME("Wrong thread, can't end query.\n");
                }
                else
                {
                    context = context_acquire(This->device, query->context->current_rt, CTXUSAGE_RESOURCELOAD);

                    ENTER_GL();
                    GL_EXTCALL(glEndQueryARB(GL_SAMPLES_PASSED_ARB));
                    checkGLcall("glEndQuery()");
                    LEAVE_GL();

                    context_release(context);
                }
            }
        }
    } else {
        FIXME("(%p) : Occlusion queries not supported\n", This);
    }

    if(dwIssueFlags & WINED3DISSUE_BEGIN) {
        This->state = QUERY_BUILDING;
    } else {
        This->state = QUERY_SIGNALLED;
    }
    return WINED3D_OK; /* can be WINED3DERR_INVALIDCALL.    */
}

static const IWineD3DQueryVtbl IWineD3DEventQuery_Vtbl =
{
    /*** IUnknown methods ***/
    IWineD3DQueryImpl_QueryInterface,
    IWineD3DQueryImpl_AddRef,
    IWineD3DQueryImpl_Release,
    /*** IWineD3Dquery methods ***/
    IWineD3DQueryImpl_GetParent,
    IWineD3DEventQueryImpl_GetData,
    IWineD3DEventQueryImpl_GetDataSize,
    IWineD3DQueryImpl_GetType,
    IWineD3DEventQueryImpl_Issue
};

static const IWineD3DQueryVtbl IWineD3DOcclusionQuery_Vtbl =
{
    /*** IUnknown methods ***/
    IWineD3DQueryImpl_QueryInterface,
    IWineD3DQueryImpl_AddRef,
    IWineD3DQueryImpl_Release,
    /*** IWineD3Dquery methods ***/
    IWineD3DQueryImpl_GetParent,
    IWineD3DOcclusionQueryImpl_GetData,
    IWineD3DOcclusionQueryImpl_GetDataSize,
    IWineD3DQueryImpl_GetType,
    IWineD3DOcclusionQueryImpl_Issue
};

HRESULT query_init(IWineD3DQueryImpl *query, IWineD3DDeviceImpl *device,
        WINED3DQUERYTYPE type, IUnknown *parent)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;

    switch (type)
    {
        case WINED3DQUERYTYPE_OCCLUSION:
            TRACE("Occlusion query.\n");
            if (!gl_info->supported[ARB_OCCLUSION_QUERY])
            {
                WARN("Unsupported in local OpenGL implementation: ARB_OCCLUSION_QUERY/NV_OCCLUSION_QUERY\n");
                return WINED3DERR_NOTAVAILABLE;
            }
            query->lpVtbl = &IWineD3DOcclusionQuery_Vtbl;
            query->extendedData = HeapAlloc(GetProcessHeap(), 0, sizeof(struct wined3d_occlusion_query));
            if (!query->extendedData)
            {
                ERR("Failed to allocate occlusion query extended data.\n");
                return E_OUTOFMEMORY;
            }
            ((struct wined3d_occlusion_query *)query->extendedData)->context = NULL;
            break;

        case WINED3DQUERYTYPE_EVENT:
            TRACE("Event query.\n");
            if (!gl_info->supported[NV_FENCE] && !gl_info->supported[APPLE_FENCE])
            {
                /* Half-Life 2 needs this query. It does not render the main
                 * menu correctly otherwise. Pretend to support it, faking
                 * this query does not do much harm except potentially
                 * lowering performance. */
                FIXME("Event query: Unimplemented, but pretending to be supported.\n");
            }
            query->lpVtbl = &IWineD3DEventQuery_Vtbl;
            query->extendedData = HeapAlloc(GetProcessHeap(), 0, sizeof(struct wined3d_event_query));
            if (!query->extendedData)
            {
                ERR("Failed to allocate event query extended data.\n");
                return E_OUTOFMEMORY;
            }
            ((struct wined3d_event_query *)query->extendedData)->context = NULL;
            break;

        case WINED3DQUERYTYPE_VCACHE:
        case WINED3DQUERYTYPE_RESOURCEMANAGER:
        case WINED3DQUERYTYPE_VERTEXSTATS:
        case WINED3DQUERYTYPE_TIMESTAMP:
        case WINED3DQUERYTYPE_TIMESTAMPDISJOINT:
        case WINED3DQUERYTYPE_TIMESTAMPFREQ:
        case WINED3DQUERYTYPE_PIPELINETIMINGS:
        case WINED3DQUERYTYPE_INTERFACETIMINGS:
        case WINED3DQUERYTYPE_VERTEXTIMINGS:
        case WINED3DQUERYTYPE_PIXELTIMINGS:
        case WINED3DQUERYTYPE_BANDWIDTHTIMINGS:
        case WINED3DQUERYTYPE_CACHEUTILIZATION:
        default:
            FIXME("Unhandled query type %#x.\n", type);
            return WINED3DERR_NOTAVAILABLE;
    }

    query->type = type;
    query->state = QUERY_CREATED;
    query->device = device;
    query->parent = parent;
    query->ref = 1;

    return WINED3D_OK;
}
