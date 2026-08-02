/* WinRT Windows.Storage.ApplicationData ApplicationData Implementation
 *
 * Copyright (C) 2023 Mohamad Al-Jaf
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

#include "private.h"
#include "wine/debug.h"
#include "pathcch.h"

WINE_DEFAULT_DEBUG_CHANNEL(data);

struct application_data_statics
{
    IActivationFactory IActivationFactory_iface;
    IApplicationDataStatics IApplicationDataStatics_iface;
    LONG ref;
};

static inline struct application_data_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct application_data_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct application_data_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IApplicationDataStatics ))
    {
        *out = &impl->IApplicationDataStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct application_data_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct application_data_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    FIXME( "iface %p, instance %p stub!\n", iface, instance );
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    /* IInspectable methods */
    factory_GetIids,
    factory_GetRuntimeClassName,
    factory_GetTrustLevel,
    /* IActivationFactory methods */
    factory_ActivateInstance,
};

struct storage_folder
{
    IStorageFolder IStorageFolder_iface;
    IStorageItem IStorageItem_iface;
    LONG ref;
};

static inline struct storage_folder *impl_from_IStorageFolder( IStorageFolder *iface )
{
    return CONTAINING_RECORD( iface, struct storage_folder, IStorageFolder_iface );
}

static HRESULT WINAPI storage_folder_QueryInterface( IStorageFolder *iface, REFIID iid, void **out )
{
    struct storage_folder *impl = impl_from_IStorageFolder( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IStorageFolder ))
    {
        *out = &impl->IStorageFolder_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IStorageItem ))
    {
        *out = &impl->IStorageItem_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI storage_folder_AddRef( IStorageFolder *iface )
{
    struct storage_folder *impl = impl_from_IStorageFolder( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI storage_folder_Release( IStorageFolder *iface )
{
    struct storage_folder *impl = impl_from_IStorageFolder( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI storage_folder_GetIids( IStorageFolder *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_folder_GetRuntimeClassName( IStorageFolder *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_folder_GetTrustLevel( IStorageFolder *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_folder_CreateFileAsyncOverloadDefaultOptions( IStorageFolder *iface, HSTRING name,
                                                                            IAsyncOperation_StorageFile **operation )
{
    FIXME( "iface %p, name %s, operation %p stub!\n", iface, debugstr_hstring(name), operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_folder_CreateFileAsync( IStorageFolder *iface, HSTRING name, CreationCollisionOption options,
                                                      IAsyncOperation_StorageFile **operation )
{
    FIXME( "iface %p, name %s, options %d, operation %p stub!\n", iface, debugstr_hstring(name), options, operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_folder_CreateFolderAsyncOverloadDefaultOptions( IStorageFolder *iface, HSTRING name,
                                                                              IAsyncOperation_StorageFolder **operation )
{
    FIXME( "iface %p, name %s, operation %p stub!\n", iface, debugstr_hstring(name), operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_folder_CreateFolderAsync( IStorageFolder *iface, HSTRING name, CreationCollisionOption options,
                                                        IAsyncOperation_StorageFolder **operation )
{
    FIXME( "iface %p, name %s, options %d, operation %p stub!\n", iface, debugstr_hstring(name), options, operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_folder_GetFileAsync( IStorageFolder *iface, HSTRING name, IAsyncOperation_StorageFile **operation )
{
    FIXME( "iface %p, name %s, operation %p stub!\n", iface, debugstr_hstring(name), operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_folder_GetFolderAsync( IStorageFolder *iface, HSTRING name, IAsyncOperation_StorageFolder **operation )
{
    FIXME( "iface %p, name %s, operation %p stub!\n", iface, debugstr_hstring(name), operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_folder_GetItemAsync( IStorageFolder *iface, HSTRING name, IAsyncOperation_IStorageItem **operation )
{
    FIXME( "iface %p, name %s, operation %p stub!\n", iface, debugstr_hstring(name), operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_folder_GetFilesAsyncOverloadDefaultOptionsStartAndCount( IStorageFolder *iface,
                                                                                       IAsyncOperation_IVectorView_StorageFile **operation )
{
    FIXME( "iface %p, operation %p stub!\n", iface, operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_folder_GetFoldersAsyncOverloadDefaultOptionsStartAndCount( IStorageFolder *iface,
                                                                                         IAsyncOperation_IVectorView_StorageFolder **operation )
{
    FIXME( "iface %p, operation %p stub!\n", iface, operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_folder_GetItemsAsyncOverloadDefaultStartAndCount( IStorageFolder *iface,
                                                                                IAsyncOperation_IVectorView_IStorageItem **operation )
{
    FIXME( "iface %p, operation %p stub!\n", iface, operation );
    return E_NOTIMPL;
}

static const struct IStorageFolderVtbl storage_folder_vtbl =
{
    storage_folder_QueryInterface,
    storage_folder_AddRef,
    storage_folder_Release,
    /* IInspectable methods */
    storage_folder_GetIids,
    storage_folder_GetRuntimeClassName,
    storage_folder_GetTrustLevel,
    /* IStorageFolder methods */
    storage_folder_CreateFileAsyncOverloadDefaultOptions,
    storage_folder_CreateFileAsync,
    storage_folder_CreateFolderAsyncOverloadDefaultOptions,
    storage_folder_CreateFolderAsync,
    storage_folder_GetFileAsync,
    storage_folder_GetFolderAsync,
    storage_folder_GetItemAsync,
    storage_folder_GetFilesAsyncOverloadDefaultOptionsStartAndCount,
    storage_folder_GetFoldersAsyncOverloadDefaultOptionsStartAndCount,
    storage_folder_GetItemsAsyncOverloadDefaultStartAndCount,
};

DEFINE_IINSPECTABLE( storage_item, IStorageItem, struct storage_folder, IStorageFolder_iface )

static HRESULT WINAPI storage_item_RenameAsyncOverloadDefaultOptions( IStorageItem *iface, HSTRING name, IAsyncAction **operation )
{
    FIXME( "iface %p, name %s, operation %p stub!\n", iface, debugstr_hstring(name), operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_item_RenameAsync( IStorageItem *iface, HSTRING name, NameCollisionOption option, IAsyncAction **operation )
{
    FIXME( "iface %p, name %s, option %d, operation %p stub!\n", iface, debugstr_hstring(name), option, operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_item_DeleteAsyncOverloadDefaultOptions( IStorageItem *iface, IAsyncAction **operation )
{
    FIXME( "iface %p, operation %p stub!\n", iface, operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_item_DeleteAsync( IStorageItem *iface, StorageDeleteOption option, IAsyncAction **operation )
{
    FIXME( "iface %p, option %d, operation %p stub!\n", iface, option, operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_item_GetBasicPropertiesAsync( IStorageItem *iface, IAsyncOperation_BasicProperties **operation )
{
    FIXME( "iface %p, operation %p stub!\n", iface, operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_item_get_Name( IStorageItem *iface, HSTRING *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_item_get_Path( IStorageItem *iface, HSTRING *value )
{
    WCHAR buffer[MAX_PATH];
    HRESULT hr;

    TRACE( "iface %p, value %p\n", iface, value );

    if (!GetModuleFileNameW( NULL, buffer, MAX_PATH )) return HRESULT_FROM_WIN32( GetLastError() );
    if (FAILED( hr = PathCchRemoveFileSpec( buffer, ARRAY_SIZE(buffer) ) )) return hr;

    return WindowsCreateString( buffer, wcslen(buffer), value );
}

static HRESULT WINAPI storage_item_get_Attributes( IStorageItem *iface, FileAttributes *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_item_get_DateCreated( IStorageItem *iface, DateTime *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI storage_item_IsOfType( IStorageItem *iface, StorageItemTypes type, boolean *value )
{
    FIXME( "iface %p, type %d, value %p stub!\n", iface, type, value );
    return E_NOTIMPL;
}

static const struct IStorageItemVtbl storage_item_vtbl =
{
    storage_item_QueryInterface,
    storage_item_AddRef,
    storage_item_Release,
    /* IInspectable methods */
    storage_item_GetIids,
    storage_item_GetRuntimeClassName,
    storage_item_GetTrustLevel,
    /* IStorageItem methods */
    storage_item_RenameAsyncOverloadDefaultOptions,
    storage_item_RenameAsync,
    storage_item_DeleteAsyncOverloadDefaultOptions,
    storage_item_DeleteAsync,
    storage_item_GetBasicPropertiesAsync,
    storage_item_get_Name,
    storage_item_get_Path,
    storage_item_get_Attributes,
    storage_item_get_DateCreated,
    storage_item_IsOfType,
};

struct application_data
{
    IApplicationData IApplicationData_iface;
    LONG ref;
};

static inline struct application_data *impl_from_IApplicationData( IApplicationData *iface )
{
    return CONTAINING_RECORD( iface, struct application_data, IApplicationData_iface );
}

static HRESULT WINAPI application_data_QueryInterface( IApplicationData *iface, REFIID iid, void **out )
{
    struct application_data *impl = impl_from_IApplicationData( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IApplicationData ))
    {
        *out = &impl->IApplicationData_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI application_data_AddRef( IApplicationData *iface )
{
    struct application_data *impl = impl_from_IApplicationData( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI application_data_Release( IApplicationData *iface )
{
    struct application_data *impl = impl_from_IApplicationData( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI application_data_GetIids( IApplicationData *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI application_data_GetRuntimeClassName( IApplicationData *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI application_data_GetTrustLevel( IApplicationData *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI application_data_get_Version( IApplicationData *iface, UINT32 *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI application_data_SetVersionAsync( IApplicationData *iface, UINT32 version, IApplicationDataSetVersionHandler *handler,
                                                        IAsyncAction **operation )
{
    FIXME( "iface %p, version %d, handler %p, operation %p stub!\n", iface, version, handler, operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI application_data_ClearAllAsync( IApplicationData *iface, IAsyncAction **operation )
{
    FIXME( "iface %p, operation %p stub!\n", iface, operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI application_data_ClearAsync( IApplicationData *iface, ApplicationDataLocality locality, IAsyncAction **operation )
{
    FIXME( "iface %p, %d locality, operation %p stub!\n", iface, locality, operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI application_data_get_LocalSettings( IApplicationData *iface, IApplicationDataContainer **value )
{
    FIXME( "iface %p, value %p semi-stub!\n", iface, value );
    *value = create_data_container();
    return S_OK;
}

static HRESULT WINAPI application_data_get_RoamingSettings( IApplicationData *iface, IApplicationDataContainer **value )
{
    FIXME( "iface %p, value %p semi-stub!\n", iface, value );
    *value = create_data_container();
    return S_OK;
}

static HRESULT WINAPI application_data_get_LocalFolder( IApplicationData *iface, IStorageFolder **value )
{
    struct storage_folder *impl;

    FIXME( "iface %p, value %p semi-stub!\n", iface, value );

    if (!value) return E_INVALIDARG;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IStorageFolder_iface.lpVtbl = &storage_folder_vtbl;
    impl->IStorageItem_iface.lpVtbl = &storage_item_vtbl;
    impl->ref = 1;

    *value = &impl->IStorageFolder_iface;
    TRACE( "created IStorageFolder %p.\n", *value );
    return S_OK;
}

static HRESULT WINAPI application_data_get_RoamingFolder( IApplicationData *iface, IStorageFolder **value )
{
    struct storage_folder *impl;

    FIXME( "iface %p, value %p semi-stub!\n", iface, value );

    if (!value) return E_INVALIDARG;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IStorageFolder_iface.lpVtbl = &storage_folder_vtbl;
    impl->IStorageItem_iface.lpVtbl = &storage_item_vtbl;
    impl->ref = 1;

    *value = &impl->IStorageFolder_iface;
    TRACE( "created IStorageFolder %p.\n", *value );
    return S_OK;
}

static HRESULT WINAPI application_data_get_TemporaryFolder( IApplicationData *iface, IStorageFolder **value )
{
    struct storage_folder *impl;
    
    FIXME( "iface %p, value %p semi-stub!\n", iface, value );

    if (!value) return E_INVALIDARG;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IStorageFolder_iface.lpVtbl = &storage_folder_vtbl;
    impl->IStorageItem_iface.lpVtbl = &storage_item_vtbl;
    impl->ref = 1;

    *value = &impl->IStorageFolder_iface;
    TRACE( "created IStorageFolder %p.\n", *value );
    return S_OK;
}

static HRESULT WINAPI application_data_add_DataChanged( IApplicationData *iface, ITypedEventHandler_ApplicationData_IInspectable *handler,
                                                        EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    return E_NOTIMPL;
}

static HRESULT WINAPI application_data_remove_DataChanged( IApplicationData *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static HRESULT WINAPI application_data_SignalDataChanged( IApplicationData *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI application_data_get_RoamingStorageQuota( IApplicationData *iface, UINT64 *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static const struct IApplicationDataVtbl application_data_vtbl =
{
    application_data_QueryInterface,
    application_data_AddRef,
    application_data_Release,
    /* IInspectable methods */
    application_data_GetIids,
    application_data_GetRuntimeClassName,
    application_data_GetTrustLevel,
    /* IApplicationData methods */
    application_data_get_Version,
    application_data_SetVersionAsync,
    application_data_ClearAllAsync,
    application_data_ClearAsync,
    application_data_get_LocalSettings,
    application_data_get_RoamingSettings,
    application_data_get_LocalFolder,
    application_data_get_RoamingFolder,
    application_data_get_TemporaryFolder,
    application_data_add_DataChanged,
    application_data_remove_DataChanged,
    application_data_SignalDataChanged,
    application_data_get_RoamingStorageQuota,
};

DEFINE_IINSPECTABLE( application_data_statics, IApplicationDataStatics, struct application_data_statics, IActivationFactory_iface )

static HRESULT WINAPI application_data_statics_get_Current( IApplicationDataStatics *iface, IApplicationData **value )
{
    struct application_data *impl;

    TRACE( "iface %p, value %p\n", iface, value );

    if (!value) return E_INVALIDARG;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IApplicationData_iface.lpVtbl = &application_data_vtbl;
    impl->ref = 1;

    *value = &impl->IApplicationData_iface;
    TRACE( "created IApplicationData %p.\n", *value );
    return S_OK;
}

static const struct IApplicationDataStaticsVtbl application_data_statics_vtbl =
{
    application_data_statics_QueryInterface,
    application_data_statics_AddRef,
    application_data_statics_Release,
    /* IInspectable methods */
    application_data_statics_GetIids,
    application_data_statics_GetRuntimeClassName,
    application_data_statics_GetTrustLevel,
    /* IApplicationDataStatics methods */
    application_data_statics_get_Current,
};

static struct application_data_statics application_data_statics =
{
    {&factory_vtbl},
    {&application_data_statics_vtbl},
    1,
};

IActivationFactory *application_data_factory = &application_data_statics.IActivationFactory_iface;
