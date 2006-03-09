/*
 * IWineD3DQuery implementation
 *
 * Copyright 2005 Oliver Stieber
 *
 *
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


#include "config.h"
#include "wined3d_private.h"

/*
http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/programmingguide/advancedtopics/Queries.asp
*/

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* *******************************************
   IWineD3DQuery IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DQueryImpl_QueryInterface(IWineD3DQuery *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DQuery)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DQueryImpl_AddRef(IWineD3DQuery *iface) {
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    TRACE("(%p) : AddRef increasing from %ld\n", This, This->ref);
    return InterlockedIncrement(&This->ref);
}

ULONG WINAPI IWineD3DQueryImpl_Release(IWineD3DQuery *iface) {
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %ld\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
        if(This->extendedData != NULL){
            HeapFree(GetProcessHeap(), 0, This->extendedData);
        }
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* *******************************************
   IWineD3DQuery IWineD3DQuery parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DQueryImpl_GetParent(IWineD3DQuery *iface, IUnknown** parent){
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;

    *parent= (IUnknown*) parent;
    IUnknown_AddRef(*parent);
    TRACE("(%p) : returning %p\n", This, *parent);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DQueryImpl_GetDevice(IWineD3DQuery* iface, IWineD3DDevice **pDevice){
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    IWineD3DDevice_AddRef((IWineD3DDevice *)This->wineD3DDevice);
    *pDevice = (IWineD3DDevice *)This->wineD3DDevice;
    TRACE("(%p) returning %p\n", This, *pDevice);
    return D3D_OK;
}


HRESULT WINAPI IWineD3DQueryImpl_GetData(IWineD3DQuery* iface, void* pData, DWORD dwSize, DWORD dwGetDataFlags){
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    if(dwSize == 0){
        /*you can use this method to poll the resource for the query status*/
        /*We return success(S_OK) if we support a feature, and faikure(S_FALSE) if we don't, just return success and fluff it for now*/
        return S_OK;
    }else{
    }

    switch (This->type){

    case WINED3DQUERYTYPE_VCACHE:
    {

        WINED3DDEVINFO_VCACHE *data = (WINED3DDEVINFO_VCACHE *)pData;
        data->Pattern     = MAKEFOURCC('C','A','C','H');
        data->OptMethod   = 0; /*0 get longest strips, 1 optimize vertex cache*/
        data->CacheSize   = 0; /*cache size, only required if OptMethod == 1*/
        data->MagicNumber = 0; /*only required if OptMethod == 1 (used internally)*/

    }
    break;
    case WINED3DQUERYTYPE_RESOURCEMANAGER:
    {
        WINED3DDEVINFO_RESOURCEMANAGER *data = (WINED3DDEVINFO_RESOURCEMANAGER *)pData;
        int i;
        for(i = 0; i < WINED3DRTYPECOUNT; i++){
            /*I'm setting the default values to 1 so as to reduce the risk of a div/0 in the caller*/
            /*  isTextureResident could be used to get some of this infomration  */
            data->stats[i].bThrashing            = FALSE;
            data->stats[i].ApproxBytesDownloaded = 1;
            data->stats[i].NumEvicts             = 1;
            data->stats[i].NumVidCreates         = 1;
            data->stats[i].LastPri               = 1;
            data->stats[i].NumUsed               = 1;
            data->stats[i].NumUsedInVidMem       = 1;
            data->stats[i].WorkingSet            = 1;
            data->stats[i].WorkingSetBytes       = 1;
            data->stats[i].TotalManaged          = 1;
            data->stats[i].TotalBytes            = 1;
        }

    }
    break;
    case WINED3DQUERYTYPE_VERTEXSTATS:
    {
        WINED3DDEVINFO_VERTEXSTATS *data = (WINED3DDEVINFO_VERTEXSTATS *)pData;
        data->NumRenderedTriangles      = 1;
        data->NumExtraClippingTriangles = 1;

    }
    break;
    case WINED3DQUERYTYPE_EVENT:
    {
        BOOL* data = pData;
        *data = TRUE; /*Don't know what this is supposed to be*/
    }
    break;
    case WINED3DQUERYTYPE_OCCLUSION:
    {
        DWORD* data = pData;
        *data = 1;
        /* TODO: opengl occlusion queries
        http://www.gris.uni-tuebingen.de/~bartz/Publications/paper/hww98.pdf
        http://oss.sgi.com/projects/ogl-sample/registry/ARB/occlusion_query.txt
        http://oss.sgi.com/projects/ogl-sample/registry/ARB/occlusion_query.txt
        */
    }
    break;
    case WINED3DQUERYTYPE_TIMESTAMP:
    {
        UINT64* data = pData;
        *data = 1; /*Don't know what this is supposed to be*/
    }
    break;
    case WINED3DQUERYTYPE_TIMESTAMPDISJOINT:
    {
        BOOL* data = pData;
        *data = FALSE; /*Don't know what this is supposed to be*/
    }
    break;
    case WINED3DQUERYTYPE_TIMESTAMPFREQ:
    {
        UINT64* data = pData;
        *data = 1; /*Don't know what this is supposed to be*/
    }
    break;
    case WINED3DQUERYTYPE_PIPELINETIMINGS:
    {
        WINED3DDEVINFO_PIPELINETIMINGS *data = (WINED3DDEVINFO_PIPELINETIMINGS *)pData;

        data->VertexProcessingTimePercent    =   1.0f;
        data->PixelProcessingTimePercent     =   1.0f;
        data->OtherGPUProcessingTimePercent  =  97.0f;
        data->GPUIdleTimePercent             =   1.0f;
    }
    break;
    case WINED3DQUERYTYPE_INTERFACETIMINGS:
    {
        WINED3DDEVINFO_INTERFACETIMINGS *data = (WINED3DDEVINFO_INTERFACETIMINGS *)pData;

        data->WaitingForGPUToUseApplicationResourceTimePercent =   1.0f;
        data->WaitingForGPUToAcceptMoreCommandsTimePercent     =   1.0f;
        data->WaitingForGPUToStayWithinLatencyTimePercent      =   1.0f;
        data->WaitingForGPUExclusiveResourceTimePercent        =   1.0f;
        data->WaitingForGPUOtherTimePercent                    =  96.0f;
    }

    break;
    case WINED3DQUERYTYPE_VERTEXTIMINGS:
    {
        WINED3DDEVINFO_STAGETIMINGS *data = (WINED3DDEVINFO_STAGETIMINGS *)pData;
        data->MemoryProcessingPercent      = 50.0f;
        data->ComputationProcessingPercent = 50.0f;

    }
    break;
    case WINED3DQUERYTYPE_PIXELTIMINGS:
    {
        WINED3DDEVINFO_STAGETIMINGS *data = (WINED3DDEVINFO_STAGETIMINGS *)pData;
        data->MemoryProcessingPercent      = 50.0f;
        data->ComputationProcessingPercent = 50.0f;
    }
    break;
    case WINED3DQUERYTYPE_BANDWIDTHTIMINGS:
    {
        WINED3DDEVINFO_BANDWIDTHTIMINGS *data = (WINED3DDEVINFO_BANDWIDTHTIMINGS *)pData;
        data->MaxBandwidthUtilized                =  1.0f;
        data->FrontEndUploadMemoryUtilizedPercent =  1.0f;
        data->VertexRateUtilizedPercent           =  1.0f;
        data->TriangleSetupRateUtilizedPercent    =  1.0f;
        data->FillRateUtilizedPercent             = 97.0f;
    }
    break;
    case WINED3DQUERYTYPE_CACHEUTILIZATION:
    {
        WINED3DDEVINFO_CACHEUTILIZATION *data = (WINED3DDEVINFO_CACHEUTILIZATION *)pData;

        data->TextureCacheHitRate             = 1.0f;
        data->PostTransformVertexCacheHitRate = 1.0f;
    }


    break;
    default:
        FIXME("(%p) Unhandled query type %d\n",This , This->type);

    };

    /*dwGetDataFlags = 0 || D3DGETDATA_FLUSH
    D3DGETDATA_FLUSH may return D3DERR_DEVICELOST if the device is lost
    */
    FIXME("(%p) : stub\n", This);
    return S_OK; /* S_OK if the query data is available*/
}


DWORD WINAPI IWineD3DQueryImpl_GetDataSize(IWineD3DQuery* iface){
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    int dataSize = 0;
    FIXME("(%p) : stub\n", This);
    switch(This->type){
    case WINED3DQUERYTYPE_VCACHE:
        dataSize = sizeof(WINED3DDEVINFO_VCACHE);
        break;
    case WINED3DQUERYTYPE_RESOURCEMANAGER:
        dataSize = sizeof(WINED3DDEVINFO_RESOURCEMANAGER);
        break;
    case WINED3DQUERYTYPE_VERTEXSTATS:
        dataSize = sizeof(WINED3DDEVINFO_VERTEXSTATS);
        break;
    case WINED3DQUERYTYPE_EVENT:
        dataSize = sizeof(BOOL);
        break;
    case WINED3DQUERYTYPE_OCCLUSION:
        dataSize = sizeof(DWORD);
        /*
        http://www.gris.uni-tuebingen.de/~bartz/Publications/paper/hww98.pdf
        http://oss.sgi.com/projects/ogl-sample/registry/ARB/occlusion_query.txt
        http://oss.sgi.com/projects/ogl-sample/registry/ARB/occlusion_query.txt
        */
        break;
    case WINED3DQUERYTYPE_TIMESTAMP:
        dataSize = sizeof(UINT64);
        break;
    case WINED3DQUERYTYPE_TIMESTAMPDISJOINT:
        dataSize = sizeof(BOOL);
        break;
    case WINED3DQUERYTYPE_TIMESTAMPFREQ:
        dataSize = sizeof(UINT64);
        break;
    case WINED3DQUERYTYPE_PIPELINETIMINGS:
        dataSize = sizeof(WINED3DDEVINFO_PIPELINETIMINGS);
        break;
    case WINED3DQUERYTYPE_INTERFACETIMINGS:
        dataSize = sizeof(WINED3DDEVINFO_INTERFACETIMINGS);
        break;
    case WINED3DQUERYTYPE_VERTEXTIMINGS:
        dataSize = sizeof(WINED3DDEVINFO_STAGETIMINGS);
        break;
    case WINED3DQUERYTYPE_PIXELTIMINGS:
        dataSize = sizeof(WINED3DDEVINFO_STAGETIMINGS);
        break;
    case WINED3DQUERYTYPE_BANDWIDTHTIMINGS:
        dataSize = sizeof(WINED3DQUERYTYPE_BANDWIDTHTIMINGS);
        break;
    case WINED3DQUERYTYPE_CACHEUTILIZATION:
        dataSize = sizeof(WINED3DDEVINFO_CACHEUTILIZATION);
        break;
    default:
       FIXME("(%p) Unhandled query type %d\n",This , This->type);
       dataSize = 0;
    }
    return dataSize;
}


WINED3DQUERYTYPE WINAPI IWineD3DQueryImpl_GetType(IWineD3DQuery* iface){
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    return This->type;
}


HRESULT WINAPI IWineD3DQueryImpl_Issue(IWineD3DQuery* iface,  DWORD dwIssueFlags){
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK; /* can be D3DERR_INVALIDCALL.    */
}


/**********************************************************
 * IWineD3DQuery VTbl follows
 **********************************************************/

const IWineD3DQueryVtbl IWineD3DQuery_Vtbl =
{
    /*** IUnknown methods ***/
    IWineD3DQueryImpl_QueryInterface,
    IWineD3DQueryImpl_AddRef,
    IWineD3DQueryImpl_Release,
     /*** IWineD3Dquery methods ***/
    IWineD3DQueryImpl_GetParent,
    IWineD3DQueryImpl_GetDevice,
    IWineD3DQueryImpl_GetData,
    IWineD3DQueryImpl_GetDataSize,
    IWineD3DQueryImpl_GetType,
    IWineD3DQueryImpl_Issue
};
