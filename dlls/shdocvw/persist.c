/*
 * Implementation of IPersist interfaces for IE Web Browser control
 *
 * 2001 John R. Sheets (for CodeWeavers)
 */

#include "debugtools.h"
#include "shdocvw.h"

DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the IPersistStorage interface
 */

static HRESULT WINAPI WBPS_QueryInterface(LPPERSISTSTORAGE iface,
                                          REFIID riid, LPVOID *ppobj)
{
    ICOM_THIS(IPersistStorageImpl, iface);

    FIXME("(%p)->(%s,%p),stub!\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI WBPS_AddRef(LPPERSISTSTORAGE iface)
{
    ICOM_THIS(IPersistStorageImpl, iface);

    TRACE("\n");
    return ++(This->ref);
}

static ULONG WINAPI WBPS_Release(LPPERSISTSTORAGE iface)
{
    ICOM_THIS(IPersistStorageImpl, iface);

    /* static class, won't be freed */
    TRACE("\n");
    return --(This->ref);
}

static HRESULT WINAPI WBPS_GetClassID(LPPERSISTSTORAGE iface, CLSID *pClassID)
{
    FIXME("stub: CLSID = %s\n", debugstr_guid(pClassID));
    return S_OK;
}

static HRESULT WINAPI WBPS_IsDirty(LPPERSISTSTORAGE iface)
{
    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI WBPS_InitNew(LPPERSISTSTORAGE iface, LPSTORAGE pStg)
{
    FIXME("stub: LPSTORAGE = %p\n", pStg);
    return S_OK;
}

static HRESULT WINAPI WBPS_Load(LPPERSISTSTORAGE iface, LPSTORAGE pStg)
{
    FIXME("stub: LPSTORAGE = %p\n", pStg);
    return S_OK;
}

static HRESULT WINAPI WBPS_Save(LPPERSISTSTORAGE iface, LPSTORAGE pStg,
                                BOOL fSameAsLoad)
{
    FIXME("stub: LPSTORAGE = %p, fSameAsLoad = %d\n", pStg, fSameAsLoad);
    return S_OK;
}

static HRESULT WINAPI WBPS_SaveCompleted(LPPERSISTSTORAGE iface, LPSTORAGE pStgNew)
{
    FIXME("stub: LPSTORAGE = %p\n", pStgNew);
    return S_OK;
}

/**********************************************************************
 * IPersistStorage virtual function table for IE Web Browser component
 */

static ICOM_VTABLE(IPersistStorage) WBPS_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    WBPS_QueryInterface,
    WBPS_AddRef,
    WBPS_Release,
    WBPS_GetClassID,
    WBPS_IsDirty,
    WBPS_InitNew,
    WBPS_Load,
    WBPS_Save,
    WBPS_SaveCompleted
};

IPersistStorageImpl SHDOCVW_PersistStorage = { &WBPS_Vtbl, 1 };


/**********************************************************************
 * Implement the IPersistStreamInit interface
 */

static HRESULT WINAPI WBPSI_QueryInterface(LPPERSISTSTREAMINIT iface,
                                           REFIID riid, LPVOID *ppobj)
{
    ICOM_THIS(IPersistStreamInitImpl, iface);

    FIXME("(%p)->(%s,%p),stub!\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI WBPSI_AddRef(LPPERSISTSTREAMINIT iface)
{
    ICOM_THIS(IPersistStreamInitImpl, iface);

    TRACE("\n");
    return ++(This->ref);
}

static ULONG WINAPI WBPSI_Release(LPPERSISTSTREAMINIT iface)
{
    ICOM_THIS(IPersistStreamInitImpl, iface);

    /* static class, won't be freed */
    TRACE("\n");
    return --(This->ref);
}

static HRESULT WINAPI WBPSI_GetClassID(LPPERSISTSTREAMINIT iface, CLSID *pClassID)
{
    FIXME("stub: CLSID = %s\n", debugstr_guid(pClassID));
    return S_OK;
}

static HRESULT WINAPI WBPSI_IsDirty(LPPERSISTSTREAMINIT iface)
{
    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI WBPSI_Load(LPPERSISTSTREAMINIT iface, LPSTREAM pStg)
{
    FIXME("stub: LPSTORAGE = %p\n", pStg);
    return S_OK;
}

static HRESULT WINAPI WBPSI_Save(LPPERSISTSTREAMINIT iface, LPSTREAM pStg,
                                BOOL fSameAsLoad)
{
    FIXME("stub: LPSTORAGE = %p, fSameAsLoad = %d\n", pStg, fSameAsLoad);
    return S_OK;
}

static HRESULT WINAPI WBPSI_GetSizeMax(LPPERSISTSTREAMINIT iface,
                                       ULARGE_INTEGER *pcbSize)
{
    FIXME("stub: ULARGE_INTEGER = %p\n", pcbSize);
    return S_OK;
}

static HRESULT WINAPI WBPSI_InitNew(LPPERSISTSTREAMINIT iface)
{
    FIXME("stub\n");
    return S_OK;
}

/**********************************************************************
 * IPersistStreamInit virtual function table for IE Web Browser component
 */

static ICOM_VTABLE(IPersistStreamInit) WBPSI_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    WBPSI_QueryInterface,
    WBPSI_AddRef,
    WBPSI_Release,
    WBPSI_GetClassID,
    WBPSI_IsDirty,
    WBPSI_Load,
    WBPSI_Save,
    WBPSI_GetSizeMax,
    WBPSI_InitNew
};

IPersistStreamInitImpl SHDOCVW_PersistStreamInit = { &WBPSI_Vtbl, 1 };
