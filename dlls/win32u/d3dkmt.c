/*
 * Copyright 2024 Rémi Bernon for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <assert.h>
#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "ntgdi_private.h"
#include "win32u_private.h"
#include "ntuser_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dkmt);

struct d3dkmt_object
{
    enum d3dkmt_type    type;           /* object type */
    D3DKMT_HANDLE       local;          /* object local handle */
    D3DKMT_HANDLE       global;         /* object global handle */
    BOOL                shared;         /* object is shared using nt handles */
    HANDLE              handle;         /* internal handle of the server object */
};

struct d3dkmt_mutex
{
    struct d3dkmt_object obj;
};

struct d3dkmt_resource
{
    struct d3dkmt_object obj;
    D3DKMT_HANDLE allocation;
};

struct d3dkmt_adapter
{
    struct d3dkmt_object obj;           /* object header */
    LUID                 luid;          /* LUID of the adapter */
};

struct d3dkmt_device
{
    struct d3dkmt_object obj;           /* object header */
    LUID                 luid;          /* LUID of the device adapter */
};

struct d3dkmt_vidpn_source
{
    D3DKMT_VIDPNSOURCEOWNER_TYPE type;      /* VidPN source owner type */
    D3DDDI_VIDEO_PRESENT_SOURCE_ID id;      /* VidPN present source id */
    D3DKMT_HANDLE device;                   /* Kernel mode device context */
    struct list entry;                      /* List entry */
};

static pthread_mutex_t d3dkmt_lock = PTHREAD_MUTEX_INITIALIZER;
static struct list d3dkmt_vidpn_sources = LIST_INIT( d3dkmt_vidpn_sources );   /* VidPN source information list */

static struct d3dkmt_object **objects, **objects_end, **objects_next;

#define D3DKMT_HANDLE_BIT  0x40000000

static BOOL is_d3dkmt_global( D3DKMT_HANDLE handle )
{
    return (handle & 0xc0000000) && (handle & 0x3f) == 2;
}

static D3DKMT_HANDLE index_to_handle( int index )
{
    return (index << 6) | D3DKMT_HANDLE_BIT;
}

static int handle_to_index( D3DKMT_HANDLE handle )
{
    return (handle & ~0xc0000000) >> 6;
}

static NTSTATUS init_handle_table(void)
{
    if (!(objects = calloc( 1024, sizeof(*objects) ))) return STATUS_NO_MEMORY;
    objects_end = objects + 1024;
    objects_next = objects;
    return STATUS_SUCCESS;
}

static struct d3dkmt_object **grow_handle_table(void)
{
    size_t old_capacity = objects_end - objects, max_capacity = handle_to_index( D3DKMT_HANDLE_BIT - 1 );
    unsigned int new_capacity = old_capacity * 3 / 2;
    struct d3dkmt_object **tmp;

    if (new_capacity > max_capacity) new_capacity = max_capacity;
    if (new_capacity <= old_capacity) return NULL; /* exhausted handle capacity */

    if (!(tmp = realloc( objects, new_capacity * sizeof(*objects) ))) return NULL;
    memset( tmp + old_capacity, 0, (new_capacity - old_capacity) * sizeof(*tmp) );

    objects = tmp;
    objects_end = tmp + new_capacity;
    objects_next = tmp + old_capacity;

    return objects_next;
}

/* allocate a d3dkmt object with a local handle */
static NTSTATUS alloc_object_handle( struct d3dkmt_object *object )
{
    struct d3dkmt_object **entry;

    pthread_mutex_lock( &d3dkmt_lock );
    if (!objects && init_handle_table()) goto done;

    for (entry = objects_next; entry < objects_end; entry++) if (!*entry) break;
    if (entry == objects_end)
    {
        for (entry = objects; entry < objects_next; entry++) if (!*entry) break;
        if (entry == objects_next && !(entry = grow_handle_table())) goto done;
    }

    object->local = index_to_handle( entry - objects );
    objects_next = entry + 1;
    *entry = object;

done:
    pthread_mutex_unlock( &d3dkmt_lock );
    return object->local ? STATUS_SUCCESS : STATUS_NO_MEMORY;
}

/* free a d3dkmt local object handle */
static void free_object_handle( struct d3dkmt_object *object )
{
    unsigned int index = handle_to_index( object->local );

    pthread_mutex_lock( &d3dkmt_lock );
    assert( objects + index < objects_end && objects[index] == object );
    objects[index] = NULL;
    object->local = 0;
    pthread_mutex_unlock( &d3dkmt_lock );
}

/* return a pointer to a d3dkmt object from its local handle */
static void *get_d3dkmt_object( D3DKMT_HANDLE local, enum d3dkmt_type type )
{
    unsigned int index = handle_to_index( local );
    struct d3dkmt_object *object;

    pthread_mutex_lock( &d3dkmt_lock );
    if (objects + index >= objects_end) object = NULL;
    else object = objects[index];
    pthread_mutex_unlock( &d3dkmt_lock );

    if (!object || object->local != local || (type != -1 && object->type != type)) return NULL;
    return object;
}

static NTSTATUS d3dkmt_object_alloc( UINT size, enum d3dkmt_type type, void **obj )
{
    struct d3dkmt_object *object;

    if (!(object = calloc( 1, size ))) return STATUS_NO_MEMORY;
    object->type = type;

    *obj = object;
    return STATUS_SUCCESS;
}

/* create a global D3DKMT object, either with a global handle or later shareable */
static NTSTATUS d3dkmt_object_create( struct d3dkmt_object *object, int fd, UINT value, BOOL shared,
                                      const void *runtime, UINT runtime_size )
{
    NTSTATUS status;

    if (fd >= 0) wine_server_send_fd( fd );

    SERVER_START_REQ( d3dkmt_object_create )
    {
        req->type = object->type;
        req->fd = fd;
        req->value = value;
        if (runtime_size) wine_server_add_data( req, runtime, runtime_size );
        status = wine_server_call( req );
        object->handle = wine_server_ptr_handle( reply->handle );
        object->global = reply->global;
        object->shared = shared;
    }
    SERVER_END_REQ;

    if (!status) status = alloc_object_handle( object );

    if (status) WARN( "Failed to create global object for %p, status %#x\n", object, status );
    else TRACE( "Created global object %#x for %p/%#x\n", object->global, object, object->local );
    return status;
}

static NTSTATUS d3dkmt_object_update( struct d3dkmt_object *object, const void *runtime, UINT runtime_size )
{
    NTSTATUS status;

    SERVER_START_REQ( d3dkmt_object_update )
    {
        req->type = object->type;
        req->global = object->global;
        if (runtime_size) wine_server_add_data( req, runtime, runtime_size );
        status = wine_server_call( req );
    }
    SERVER_END_REQ;

    if (status) WARN( "Failed to update object %#x/%p global %#x, status %#x\n", object->local, object, object->global, status );
    else TRACE( "Updated object %#x/%p global %#x\n", object->local, object, object->global );
    return status;
}

static NTSTATUS d3dkmt_object_open( struct d3dkmt_object *obj, D3DKMT_HANDLE global, HANDLE handle,
                                    void *runtime, UINT *runtime_size )
{
    NTSTATUS status;

    SERVER_START_REQ( d3dkmt_object_open )
    {
        req->type = obj->type;
        req->global = global;
        req->handle = wine_server_obj_handle( handle );
        if (runtime) wine_server_set_reply( req, runtime, *runtime_size );
        status = wine_server_call( req );
        obj->handle = wine_server_ptr_handle( reply->handle );
        obj->global = reply->global;
        obj->shared = !global;
        *runtime_size = reply->runtime_size;
    }
    SERVER_END_REQ;
    if (!status) status = alloc_object_handle( obj );

    if (status) WARN( "Failed to open global object %#x/%p, status %#x\n", global, handle, status );
    else TRACE( "Opened global object %#x/%p as %p/%#x\n", global, handle, obj, obj->local );
    return status;
}

static NTSTATUS d3dkmt_object_query( enum d3dkmt_type type, D3DKMT_HANDLE global, HANDLE handle,
                                     UINT *runtime_size )
{
    NTSTATUS status;

    SERVER_START_REQ( d3dkmt_object_query )
    {
        req->type = type;
        req->global = global;
        req->handle = wine_server_obj_handle( handle );
        status = wine_server_call( req );
        *runtime_size = reply->runtime_size;
    }
    SERVER_END_REQ;

    if (status) WARN( "Failed to query global object %#x/%p, status %#x\n", global, handle, status );
    else TRACE( "Found global object %#x/%p with runtime size %#x\n", global, handle, *runtime_size );
    return status;
}

static void d3dkmt_object_free( struct d3dkmt_object *object )
{
    TRACE( "object %p/%#x, global %#x\n", object, object->local, object->global );
    if (object->local) free_object_handle( object );
    if (object->handle) NtClose( object->handle );
    free( object );
}

/* create a struct security_descriptor and contained information in one contiguous piece of memory */
static unsigned int alloc_object_attributes( const OBJECT_ATTRIBUTES *attr, struct object_attributes **ret,
                                             data_size_t *ret_len )
{
    unsigned int len = sizeof(**ret);
    SID *owner = NULL, *group = NULL;
    ACL *dacl = NULL, *sacl = NULL;
    SECURITY_DESCRIPTOR *sd;

    *ret = NULL;
    *ret_len = 0;

    if (!attr) return STATUS_SUCCESS;

    if (attr->Length != sizeof(*attr)) return STATUS_INVALID_PARAMETER;

    if ((sd = attr->SecurityDescriptor))
    {
        len += sizeof(struct security_descriptor);
    if (sd->Revision != SECURITY_DESCRIPTOR_REVISION) return STATUS_UNKNOWN_REVISION;
        if (sd->Control & SE_SELF_RELATIVE)
        {
            SECURITY_DESCRIPTOR_RELATIVE *rel = (SECURITY_DESCRIPTOR_RELATIVE *)sd;
            if (rel->Owner) owner = (PSID)((BYTE *)rel + rel->Owner);
            if (rel->Group) group = (PSID)((BYTE *)rel + rel->Group);
            if ((sd->Control & SE_SACL_PRESENT) && rel->Sacl) sacl = (PSID)((BYTE *)rel + rel->Sacl);
            if ((sd->Control & SE_DACL_PRESENT) && rel->Dacl) dacl = (PSID)((BYTE *)rel + rel->Dacl);
        }
        else
        {
            owner = sd->Owner;
            group = sd->Group;
            if (sd->Control & SE_SACL_PRESENT) sacl = sd->Sacl;
            if (sd->Control & SE_DACL_PRESENT) dacl = sd->Dacl;
        }

        if (owner) len += offsetof( SID, SubAuthority[owner->SubAuthorityCount] );
        if (group) len += offsetof( SID, SubAuthority[group->SubAuthorityCount] );
        if (sacl) len += sacl->AclSize;
        if (dacl) len += dacl->AclSize;

        /* fix alignment for the Unicode name that follows the structure */
        len = (len + sizeof(WCHAR) - 1) & ~(sizeof(WCHAR) - 1);
    }

    if (attr->ObjectName)
    {
        if ((ULONG_PTR)attr->ObjectName->Buffer & (sizeof(WCHAR) - 1)) return STATUS_DATATYPE_MISALIGNMENT;
        if (attr->ObjectName->Length & (sizeof(WCHAR) - 1)) return STATUS_OBJECT_NAME_INVALID;
        len += attr->ObjectName->Length;
    }
    else if (attr->RootDirectory) return STATUS_OBJECT_NAME_INVALID;

    len = (len + 3) & ~3;  /* DWORD-align the entire structure */

    if (!(*ret = calloc( len, 1 ))) return STATUS_NO_MEMORY;

    (*ret)->rootdir = wine_server_obj_handle( attr->RootDirectory );
    (*ret)->attributes = attr->Attributes;

    if (attr->SecurityDescriptor)
    {
        struct security_descriptor *descr = (struct security_descriptor *)(*ret + 1);
        unsigned char *ptr = (unsigned char *)(descr + 1);

        descr->control = sd->Control & ~SE_SELF_RELATIVE;
        if (owner) descr->owner_len = offsetof( SID, SubAuthority[owner->SubAuthorityCount] );
        if (group) descr->group_len = offsetof( SID, SubAuthority[group->SubAuthorityCount] );
        if (sacl) descr->sacl_len = sacl->AclSize;
        if (dacl) descr->dacl_len = dacl->AclSize;

        memcpy( ptr, owner, descr->owner_len );
        ptr += descr->owner_len;
        memcpy( ptr, group, descr->group_len );
        ptr += descr->group_len;
        memcpy( ptr, sacl, descr->sacl_len );
        ptr += descr->sacl_len;
        memcpy( ptr, dacl, descr->dacl_len );
        (*ret)->sd_len = (sizeof(*descr) + descr->owner_len + descr->group_len + descr->sacl_len +
                          descr->dacl_len + sizeof(WCHAR) - 1) & ~(sizeof(WCHAR) - 1);
    }

    if (attr->ObjectName)
    {
        unsigned char *ptr = (unsigned char *)(*ret + 1) + (*ret)->sd_len;
        (*ret)->name_len = attr->ObjectName->Length;
        memcpy( ptr, attr->ObjectName->Buffer, (*ret)->name_len );
    }

    *ret_len = len;
    return STATUS_SUCCESS;
}

static VkInstance d3dkmt_vk_instance; /* Vulkan instance for D3DKMT functions */
static PFN_vkGetPhysicalDeviceMemoryProperties2KHR pvkGetPhysicalDeviceMemoryProperties2KHR;
static PFN_vkGetPhysicalDeviceMemoryProperties pvkGetPhysicalDeviceMemoryProperties;
static PFN_vkGetPhysicalDeviceProperties2KHR pvkGetPhysicalDeviceProperties2KHR;
static PFN_vkEnumeratePhysicalDevices pvkEnumeratePhysicalDevices;

static void d3dkmt_init_vulkan(void)
{
    static const char *extensions[] =
    {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
    };
    VkInstanceCreateInfo create_info =
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .enabledExtensionCount = ARRAY_SIZE( extensions ),
        .ppEnabledExtensionNames = extensions,
    };
    PFN_vkDestroyInstance p_vkDestroyInstance;
    PFN_vkCreateInstance p_vkCreateInstance;
    VkResult vr;

    if (!vulkan_init())
    {
        WARN( "Failed to open the Vulkan driver\n" );
        return;
    }

    p_vkCreateInstance = (PFN_vkCreateInstance)p_vkGetInstanceProcAddr( NULL, "vkCreateInstance" );
    if ((vr = p_vkCreateInstance( &create_info, NULL, &d3dkmt_vk_instance )))
    {
        WARN( "Failed to create a Vulkan instance, vr %d.\n", vr );
        return;
    }

    p_vkDestroyInstance = (PFN_vkDestroyInstance)p_vkGetInstanceProcAddr( d3dkmt_vk_instance, "vkDestroyInstance" );
#define LOAD_VK_FUNC( f )                                                                      \
    if (!(p##f = (void *)p_vkGetInstanceProcAddr( d3dkmt_vk_instance, #f )))                   \
    {                                                                                          \
        WARN( "Failed to load " #f ".\n" );                                                    \
        p_vkDestroyInstance( d3dkmt_vk_instance, NULL );                                       \
        d3dkmt_vk_instance = NULL;                                                             \
        return;                                                                                \
    }
    LOAD_VK_FUNC( vkEnumeratePhysicalDevices )
    LOAD_VK_FUNC( vkGetPhysicalDeviceProperties2KHR )
    LOAD_VK_FUNC( vkGetPhysicalDeviceMemoryProperties )
    LOAD_VK_FUNC( vkGetPhysicalDeviceMemoryProperties2KHR )
#undef LOAD_VK_FUNC
}

static BOOL d3dkmt_use_vulkan(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once( &once, d3dkmt_init_vulkan );
    return !!d3dkmt_vk_instance;
}

static unsigned int validate_open_object_attributes( const OBJECT_ATTRIBUTES *attr )
{
    if (!attr || attr->Length != sizeof(*attr)) return STATUS_INVALID_PARAMETER;

    if (attr->ObjectName)
    {
        if ((ULONG_PTR)attr->ObjectName->Buffer & (sizeof(WCHAR) - 1)) return STATUS_DATATYPE_MISALIGNMENT;
        if (attr->ObjectName->Length & (sizeof(WCHAR) - 1)) return STATUS_OBJECT_NAME_INVALID;
    }
    else if (attr->RootDirectory) return STATUS_OBJECT_NAME_INVALID;

    return STATUS_SUCCESS;
}

/******************************************************************************
 *           NtGdiDdDDIOpenAdapterFromHdc    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenAdapterFromHdc( D3DKMT_OPENADAPTERFROMHDC *desc )
{
    FIXME( "(%p): stub\n", desc );
    return STATUS_NO_MEMORY;
}

/******************************************************************************
 *           NtGdiDdDDIEscape    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIEscape( const D3DKMT_ESCAPE *desc )
{
    switch (desc->Type)
    {
    case D3DKMT_ESCAPE_UPDATE_RESOURCE_WINE:
    {
        struct d3dkmt_resource *resource;

        TRACE( "D3DKMT_ESCAPE_UPDATE_RESOURCE_WINE hContext %#x, pPrivateDriverData %p, PrivateDriverDataSize %#x\n",
               desc->hContext, desc->pPrivateDriverData, desc->PrivateDriverDataSize );

        if (!(resource = get_d3dkmt_object( desc->hContext, D3DKMT_RESOURCE ))) return STATUS_INVALID_PARAMETER;
        return d3dkmt_object_update( &resource->obj, desc->pPrivateDriverData, desc->PrivateDriverDataSize );
    }

    default:
        FIXME( "(%p): stub\n", desc );
        return STATUS_NO_MEMORY;
    }
}

/******************************************************************************
 *           NtGdiDdDDICloseAdapter    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICloseAdapter( const D3DKMT_CLOSEADAPTER *desc )
{
    struct d3dkmt_object *adapter;

    TRACE( "(%p)\n", desc );

    if (!desc || !desc->hAdapter) return STATUS_INVALID_PARAMETER;
    if (!(adapter = get_d3dkmt_object( desc->hAdapter, D3DKMT_ADAPTER ))) return STATUS_INVALID_PARAMETER;

    d3dkmt_object_free( adapter );
    return STATUS_SUCCESS;
}

static UINT get_vulkan_physical_devices( VkPhysicalDevice **devices )
{
    UINT device_count;
    VkResult vr;

    if ((vr = pvkEnumeratePhysicalDevices( d3dkmt_vk_instance, &device_count, NULL )))
    {
        WARN( "vkEnumeratePhysicalDevices returned %d\n", vr );
        return 0;
    }

    if (!device_count || !(*devices = malloc( device_count * sizeof(**devices) ))) return 0;

    if ((vr = pvkEnumeratePhysicalDevices( d3dkmt_vk_instance, &device_count, *devices )))
    {
        WARN( "vkEnumeratePhysicalDevices returned %d\n", vr );
        free( *devices );
        return 0;
    }

    return device_count;
}

static VkPhysicalDevice get_vulkan_physical_device( const LUID *luid )
{
    VkPhysicalDevice *devices, device;
    UINT device_count, i;
    GUID uuid;

    if (!get_vulkan_uuid_from_luid( luid, &uuid ))
    {
        WARN( "Failed to find Vulkan device with LUID %08x:%08x.\n", luid->HighPart, luid->LowPart );
        return VK_NULL_HANDLE;
    }

    if (!(device_count = get_vulkan_physical_devices( &devices ))) return VK_NULL_HANDLE;

    for (i = 0, device = VK_NULL_HANDLE; i < device_count; ++i)
    {
        VkPhysicalDeviceIDProperties id = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
        VkPhysicalDeviceProperties2 properties2 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &id};

        pvkGetPhysicalDeviceProperties2KHR( devices[i], &properties2 );
        if (IsEqualGUID( &uuid, id.deviceUUID ))
        {
            device = devices[i];
            break;
        }
    }

    free( devices );
    return device;
}

/******************************************************************************
 *           NtGdiDdDDIOpenAdapterFromLuid    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenAdapterFromLuid( D3DKMT_OPENADAPTERFROMLUID *desc )
{
    struct d3dkmt_adapter *adapter;
    NTSTATUS status;

    if ((status = d3dkmt_object_alloc( sizeof(*adapter), D3DKMT_ADAPTER, (void **)&adapter ))) return status;
    if ((status = alloc_object_handle( &adapter->obj ))) goto failed;

    if (!d3dkmt_use_vulkan()) WARN( "Vulkan is unavailable.\n" );
    else if (!get_vulkan_physical_device( &desc->AdapterLuid )) WARN( "Failed to find vulkan device\n" );
    else adapter->luid = desc->AdapterLuid;

    desc->hAdapter = adapter->obj.local;
    return STATUS_SUCCESS;

failed:
    d3dkmt_object_free( &adapter->obj );
    return status;
}

/******************************************************************************
 *           NtGdiDdDDICreateDevice    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICreateDevice( D3DKMT_CREATEDEVICE *desc )
{
    struct d3dkmt_adapter *adapter;
    struct d3dkmt_device *device;
    NTSTATUS status;

    TRACE( "(%p)\n", desc );

    if (!desc) return STATUS_INVALID_PARAMETER;
    if (desc->Flags.LegacyMode || desc->Flags.RequestVSync || desc->Flags.DisableGpuTimeout) FIXME( "Flags unsupported.\n" );

    if (!(adapter = get_d3dkmt_object( desc->hAdapter, D3DKMT_ADAPTER ))) return STATUS_INVALID_PARAMETER;
    if ((status = d3dkmt_object_alloc( sizeof(*device), D3DKMT_DEVICE, (void **)&device ))) return status;
    if ((status = alloc_object_handle( &device->obj ))) goto failed;

    device->luid = adapter->luid;

    desc->hDevice = device->obj.local;
    return STATUS_SUCCESS;

failed:
    d3dkmt_object_free( &device->obj );
    return status;
}

/******************************************************************************
 *           NtGdiDdDDIDestroyDevice    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIDestroyDevice( const D3DKMT_DESTROYDEVICE *desc )
{
    D3DKMT_SETVIDPNSOURCEOWNER set_owner_desc = {0};
    struct d3dkmt_object *device;

    TRACE( "(%p)\n", desc );

    if (!desc || !desc->hDevice) return STATUS_INVALID_PARAMETER;
    if (!(device = get_d3dkmt_object( desc->hDevice, D3DKMT_DEVICE ))) return STATUS_INVALID_PARAMETER;

    set_owner_desc.hDevice = desc->hDevice;
    NtGdiDdDDISetVidPnSourceOwner( &set_owner_desc );

    d3dkmt_object_free( device );
    return STATUS_SUCCESS;
}

/******************************************************************************
 *           NtGdiDdDDIQueryAdapterInfo    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIQueryAdapterInfo( D3DKMT_QUERYADAPTERINFO *desc )
{
    TRACE( "(%p).\n", desc );

    if (!desc || !desc->hAdapter || !desc->pPrivateDriverData)
        return STATUS_INVALID_PARAMETER;

    switch (desc->Type)
    {
    case KMTQAITYPE_CHECKDRIVERUPDATESTATUS:
    {
        BOOL *value = desc->pPrivateDriverData;

        if (desc->PrivateDriverDataSize < sizeof(*value))
            return STATUS_INVALID_PARAMETER;

        *value = FALSE;
        return STATUS_SUCCESS;
    }
    case KMTQAITYPE_DRIVERVERSION:
    {
        D3DKMT_DRIVERVERSION *value = desc->pPrivateDriverData;

        if (desc->PrivateDriverDataSize < sizeof(*value))
            return STATUS_INVALID_PARAMETER;

        *value = KMT_DRIVERVERSION_WDDM_1_3;
        return STATUS_SUCCESS;
    }
    default:
    {
        FIXME( "type %d not handled.\n", desc->Type );
        return STATUS_NOT_IMPLEMENTED;
    }
    }
}

/******************************************************************************
 *           NtGdiDdDDIQueryStatistics    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIQueryStatistics( D3DKMT_QUERYSTATISTICS *stats )
{
    FIXME( "(%p): stub\n", stats );
    return STATUS_SUCCESS;
}

/******************************************************************************
 *           NtGdiDdDDIQueryVideoMemoryInfo    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIQueryVideoMemoryInfo( D3DKMT_QUERYVIDEOMEMORYINFO *desc )
{
    VkPhysicalDeviceMemoryBudgetPropertiesEXT budget;
    VkPhysicalDeviceMemoryProperties2 properties2;
    VkPhysicalDevice phys_dev;
    struct d3dkmt_adapter *adapter;
    OBJECT_BASIC_INFORMATION info;
    NTSTATUS status;
    unsigned int i;

    TRACE( "(%p)\n", desc );

    if (!desc || !desc->hAdapter ||
        (desc->MemorySegmentGroup != D3DKMT_MEMORY_SEGMENT_GROUP_LOCAL &&
         desc->MemorySegmentGroup != D3DKMT_MEMORY_SEGMENT_GROUP_NON_LOCAL))
        return STATUS_INVALID_PARAMETER;

    /* FIXME: Wine currently doesn't support linked adapters */
    if (desc->PhysicalAdapterIndex > 0) return STATUS_INVALID_PARAMETER;

    status = NtQueryObject( desc->hProcess ? desc->hProcess : GetCurrentProcess(),
                            ObjectBasicInformation, &info, sizeof(info), NULL );
    if (status != STATUS_SUCCESS) return status;
    if (!(info.GrantedAccess & PROCESS_QUERY_INFORMATION)) return STATUS_ACCESS_DENIED;

    if (!(adapter = get_d3dkmt_object( desc->hAdapter, D3DKMT_ADAPTER ))) return STATUS_INVALID_PARAMETER;

    desc->Budget = 0;
    desc->CurrentUsage = 0;
    desc->CurrentReservation = 0;
    desc->AvailableForReservation = 0;

    if ((phys_dev = get_vulkan_physical_device( &adapter->luid )))
    {
        memset( &budget, 0, sizeof(budget) );
        budget.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
        properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
        properties2.pNext = &budget;
        pvkGetPhysicalDeviceMemoryProperties2KHR( phys_dev, &properties2 );
        for (i = 0; i < properties2.memoryProperties.memoryHeapCount; ++i)
        {
            if ((desc->MemorySegmentGroup == D3DKMT_MEMORY_SEGMENT_GROUP_LOCAL &&
                 properties2.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ||
                (desc->MemorySegmentGroup == D3DKMT_MEMORY_SEGMENT_GROUP_NON_LOCAL &&
                 !(properties2.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)))
            {
                desc->Budget += budget.heapBudget[i];
                desc->CurrentUsage += min( budget.heapBudget[i], budget.heapUsage[i] );
            }
        }
        desc->AvailableForReservation = desc->Budget / 2;
    }

    return STATUS_SUCCESS;
}

/******************************************************************************
 *           NtGdiDdDDISetQueuedLimit    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDISetQueuedLimit( D3DKMT_SETQUEUEDLIMIT *desc )
{
    FIXME( "(%p): stub\n", desc );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDISetVidPnSourceOwner    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDISetVidPnSourceOwner( const D3DKMT_SETVIDPNSOURCEOWNER *desc )
{
    struct d3dkmt_vidpn_source *source, *source2;
    BOOL found;
    UINT i;

    TRACE( "(%p)\n", desc );

    if (!desc || !desc->hDevice || (desc->VidPnSourceCount && (!desc->pType || !desc->pVidPnSourceId)))
        return STATUS_INVALID_PARAMETER;

    pthread_mutex_lock( &d3dkmt_lock );

    /* Check parameters */
    for (i = 0; i < desc->VidPnSourceCount; ++i)
    {
        LIST_FOR_EACH_ENTRY( source, &d3dkmt_vidpn_sources, struct d3dkmt_vidpn_source, entry )
        {
            if (source->id == desc->pVidPnSourceId[i])
            {
                /* Same device */
                if (source->device == desc->hDevice)
                {
                    if ((source->type == D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE &&
                         (desc->pType[i] == D3DKMT_VIDPNSOURCEOWNER_SHARED ||
                          desc->pType[i] == D3DKMT_VIDPNSOURCEOWNER_EMULATED)) ||
                        (source->type == D3DKMT_VIDPNSOURCEOWNER_EMULATED &&
                         desc->pType[i] == D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE))
                    {
                        pthread_mutex_unlock( &d3dkmt_lock );
                        return STATUS_INVALID_PARAMETER;
                    }
                }
                /* Different devices */
                else
                {
                    if ((source->type == D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE || source->type == D3DKMT_VIDPNSOURCEOWNER_EMULATED) &&
                        (desc->pType[i] == D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE ||
                         desc->pType[i] == D3DKMT_VIDPNSOURCEOWNER_EMULATED))
                    {
                        pthread_mutex_unlock( &d3dkmt_lock );
                        return STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE;
                    }
                }
            }
        }

        /* On Windows, it seems that all video present sources are owned by DMM clients, so any attempt to set
         * D3DKMT_VIDPNSOURCEOWNER_SHARED come back STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE */
        if (desc->pType[i] == D3DKMT_VIDPNSOURCEOWNER_SHARED)
        {
            pthread_mutex_unlock( &d3dkmt_lock );
            return STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE;
        }

        /* FIXME: D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVEGDI unsupported */
        if (desc->pType[i] == D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVEGDI || desc->pType[i] > D3DKMT_VIDPNSOURCEOWNER_EMULATED)
        {
            pthread_mutex_unlock( &d3dkmt_lock );
            return STATUS_INVALID_PARAMETER;
        }
    }

    /* Remove owner */
    if (!desc->VidPnSourceCount && !desc->pType && !desc->pVidPnSourceId)
    {
        LIST_FOR_EACH_ENTRY_SAFE( source, source2, &d3dkmt_vidpn_sources, struct d3dkmt_vidpn_source, entry )
        {
            if (source->device == desc->hDevice)
            {
                list_remove( &source->entry );
                free( source );
            }
        }

        pthread_mutex_unlock( &d3dkmt_lock );
        return STATUS_SUCCESS;
    }

    /* Add owner */
    for (i = 0; i < desc->VidPnSourceCount; ++i)
    {
        found = FALSE;
        LIST_FOR_EACH_ENTRY( source, &d3dkmt_vidpn_sources, struct d3dkmt_vidpn_source, entry )
        {
            if (source->device == desc->hDevice && source->id == desc->pVidPnSourceId[i])
            {
                found = TRUE;
                break;
            }
        }

        if (found) source->type = desc->pType[i];
        else
        {
            source = malloc( sizeof(*source) );
            if (!source)
            {
                pthread_mutex_unlock( &d3dkmt_lock );
                return STATUS_NO_MEMORY;
            }

            source->id = desc->pVidPnSourceId[i];
            source->type = desc->pType[i];
            source->device = desc->hDevice;
            list_add_tail( &d3dkmt_vidpn_sources, &source->entry );
        }
    }

    pthread_mutex_unlock( &d3dkmt_lock );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtGdiDdDDICheckOcclusion( const D3DKMT_CHECKOCCLUSION *desc )
{
    FIXME( "desc %p stub!\n", desc );
    return STATUS_PROCEDURE_NOT_FOUND;
}

/******************************************************************************
 *           NtGdiDdDDICheckVidPnExclusiveOwnership    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICheckVidPnExclusiveOwnership( const D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *desc )
{
    struct d3dkmt_vidpn_source *source;

    TRACE( "(%p)\n", desc );

    if (!desc || !desc->hAdapter) return STATUS_INVALID_PARAMETER;

    pthread_mutex_lock( &d3dkmt_lock );

    LIST_FOR_EACH_ENTRY( source, &d3dkmt_vidpn_sources, struct d3dkmt_vidpn_source, entry )
    {
        if (source->id == desc->VidPnSourceId && source->type == D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE)
        {
            pthread_mutex_unlock( &d3dkmt_lock );
            return STATUS_GRAPHICS_PRESENT_OCCLUDED;
        }
    }

    pthread_mutex_unlock( &d3dkmt_lock );
    return STATUS_SUCCESS;
}

struct vk_physdev_info
{
    VkPhysicalDeviceProperties2 properties2;
    VkPhysicalDeviceIDProperties id;
    VkPhysicalDeviceMemoryProperties mem_properties;
};

static int compare_vulkan_physical_devices( const void *v1, const void *v2 )
{
    static const int device_type_rank[6] = { 100, 1, 0, 2, 3, 200 };
    const struct vk_physdev_info *d1 = v1, *d2 = v2;
    int rank1, rank2;

    rank1 = device_type_rank[ min( d1->properties2.properties.deviceType, ARRAY_SIZE(device_type_rank) - 1) ];
    rank2 = device_type_rank[ min( d2->properties2.properties.deviceType, ARRAY_SIZE(device_type_rank) - 1) ];
    if (rank1 != rank2) return rank1 - rank2;

    return memcmp( &d1->id.deviceUUID, &d2->id.deviceUUID, sizeof(d1->id.deviceUUID) );
}

BOOL get_vulkan_gpus( struct list *gpus )
{
    struct vk_physdev_info *devinfo;
    VkPhysicalDevice *devices;
    UINT i, j, count;

    if (!d3dkmt_use_vulkan()) return FALSE;
    if (!(count = get_vulkan_physical_devices( &devices ))) return FALSE;

    if (!(devinfo = calloc( count, sizeof(*devinfo) )))
    {
        free( devices );
        return FALSE;
    }
    for (i = 0; i < count; ++i)
    {
        devinfo[i].id.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
        devinfo[i].properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        devinfo[i].properties2.pNext = &devinfo[i].id;
        pvkGetPhysicalDeviceProperties2KHR( devices[i], &devinfo[i].properties2 );
        pvkGetPhysicalDeviceMemoryProperties( devices[i], &devinfo[i].mem_properties );
    }
    qsort( devinfo, count, sizeof(*devinfo), compare_vulkan_physical_devices );

    for (i = 0; i < count; ++i)
    {
        struct vulkan_gpu *gpu;

        if (!(gpu = calloc( 1, sizeof(*gpu) ))) break;
        memcpy( &gpu->uuid, devinfo[i].id.deviceUUID, sizeof(gpu->uuid) );
        gpu->name = strdup( devinfo[i].properties2.properties.deviceName );
        gpu->pci_id.vendor = devinfo[i].properties2.properties.vendorID;
        gpu->pci_id.device = devinfo[i].properties2.properties.deviceID;

        for (j = 0; j < devinfo[i].mem_properties.memoryHeapCount; j++)
        {
            if (devinfo[i].mem_properties.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                gpu->memory += devinfo[i].mem_properties.memoryHeaps[j].size;
        }

        list_add_tail( gpus, &gpu->entry );
    }

    free( devinfo );
    free( devices );
    return TRUE;
}

void free_vulkan_gpu( struct vulkan_gpu *gpu )
{
    free( gpu->name );
    free( gpu );
}

/******************************************************************************
 *           NtGdiDdDDIShareObjects    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIShareObjects( UINT count, const D3DKMT_HANDLE *handles, OBJECT_ATTRIBUTES *attr,
                                        UINT access, HANDLE *handle )
{
    struct d3dkmt_object *object, *resource = NULL, *sync = NULL, *mutex = NULL;
    struct object_attributes *objattr;
    data_size_t len;
    NTSTATUS status;

    TRACE( "count %u, handles %p, attr %p, access %#x, handle %p\n", count, handles, attr, access, handle );

    if (count == 1)
    {
        if (!(object = get_d3dkmt_object( handles[0], -1 )) || !object->shared) goto failed;
        if (object->type == D3DKMT_RESOURCE) resource = object;
        else if (object->type == D3DKMT_SYNC) sync = object;
        else goto failed;
    }
    else if (count == 3)
    {
        if (!(object = get_d3dkmt_object( handles[0], -1 )) || !object->shared) goto failed;
        if (object->type != D3DKMT_RESOURCE) goto failed;
        resource = object;

        if (!(object = get_d3dkmt_object( handles[1], -1 )) || !object->shared) goto failed;
        if (object->type != D3DKMT_MUTEX) goto failed;
        mutex = object;

        if (!(object = get_d3dkmt_object( handles[2], -1 )) || !object->shared) goto failed;
        if (object->type != D3DKMT_SYNC) goto failed;
        sync = object;
    }
    else goto failed;

    if ((status = alloc_object_attributes( attr, &objattr, &len ))) return status;

    SERVER_START_REQ( d3dkmt_share_objects )
    {
        req->access = access | STANDARD_RIGHTS_ALL;
        if (resource) req->resource = resource->global;
        if (mutex) req->mutex = mutex->global;
        if (sync) req->sync = sync->global;
        wine_server_add_data( req, objattr, len );
        status = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    free( objattr );

    if (status) WARN( "Failed to share objects, status %#x\n", status );
    else TRACE( "Shared objects with handle %p\n", *handle );
    return status;

failed:
    WARN( "Unsupported object count / types / handles\n" );
    return STATUS_INVALID_PARAMETER;
}

/******************************************************************************
 *           NtGdiDdDDICreateAllocation2    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICreateAllocation2( D3DKMT_CREATEALLOCATION *params )
{
    D3DKMT_CREATESTANDARDALLOCATION *standard;
    struct d3dkmt_resource *resource = NULL;
    D3DDDI_ALLOCATIONINFO *alloc_info;
    struct d3dkmt_object *allocation;
    struct d3dkmt_device *device;
    NTSTATUS status;

    FIXME( "params %p semi-stub!\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;
    if (!(device = get_d3dkmt_object( params->hDevice, D3DKMT_DEVICE ))) return STATUS_INVALID_PARAMETER;

    if (!params->Flags.StandardAllocation) return STATUS_INVALID_PARAMETER;
    if (params->PrivateDriverDataSize) return STATUS_INVALID_PARAMETER;

    if (params->NumAllocations != 1) return STATUS_INVALID_PARAMETER;
    if (!(alloc_info = params->pAllocationInfo)) return STATUS_INVALID_PARAMETER;

    if (!(standard = params->pStandardAllocation)) return STATUS_INVALID_PARAMETER;
    if (standard->Type != D3DKMT_STANDARDALLOCATIONTYPE_EXISTINGHEAP) return STATUS_INVALID_PARAMETER;
    if (standard->ExistingHeapData.Size & 0xfff) return STATUS_INVALID_PARAMETER;
    if (!params->Flags.ExistingSysMem) return STATUS_INVALID_PARAMETER;
    if (!alloc_info->pSystemMem) return STATUS_INVALID_PARAMETER;

    if (params->Flags.CreateResource)
    {
        if (params->hResource && !(resource = get_d3dkmt_object( params->hResource, D3DKMT_RESOURCE )))
            return STATUS_INVALID_HANDLE;
        if ((status = d3dkmt_object_alloc( sizeof(*resource), D3DKMT_RESOURCE, (void **)&resource ))) return status;
        if ((status = d3dkmt_object_alloc( sizeof(*allocation), D3DKMT_ALLOCATION, (void **)&allocation ))) goto failed;

        if (!params->Flags.CreateShared) status = alloc_object_handle( &resource->obj );
        else status = d3dkmt_object_create( &resource->obj, -1, 0, params->Flags.NtSecuritySharing,
                                            params->pPrivateRuntimeData, params->PrivateRuntimeDataSize );
        if (status) goto failed;

        params->hGlobalShare = resource->obj.shared ? 0 : resource->obj.global;
        params->hResource = resource->obj.local;
    }
    else
    {
        if (params->Flags.CreateShared) return STATUS_INVALID_PARAMETER;
        if (params->hResource)
        {
            resource = get_d3dkmt_object( params->hResource, D3DKMT_RESOURCE );
            return resource ? STATUS_INVALID_PARAMETER : STATUS_INVALID_HANDLE;
        }
        if ((status = d3dkmt_object_alloc( sizeof(*allocation), D3DKMT_ALLOCATION, (void **)&allocation ))) return status;
        params->hGlobalShare = 0;
    }

    if ((status = alloc_object_handle( allocation ))) goto failed;
    if (resource) resource->allocation = allocation->local;
    alloc_info->hAllocation = allocation->local;
    return STATUS_SUCCESS;

failed:
    if (resource) d3dkmt_object_free( &resource->obj );
    return status;
}

/******************************************************************************
 *           NtGdiDdDDICreateAllocation    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICreateAllocation( D3DKMT_CREATEALLOCATION *params )
{
    return NtGdiDdDDICreateAllocation2( params );
}

/******************************************************************************
 *           NtGdiDdDDIDestroyAllocation2    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIDestroyAllocation2( const D3DKMT_DESTROYALLOCATION2 *params )
{
    struct d3dkmt_object *device, *allocation;
    D3DKMT_HANDLE alloc_handle = 0;
    UINT i;

    TRACE( "params %p\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;
    if (!(device = get_d3dkmt_object( params->hDevice, D3DKMT_DEVICE ))) return STATUS_INVALID_PARAMETER;

    if (params->AllocationCount && !params->phAllocationList) return STATUS_INVALID_PARAMETER;

    if (params->hResource)
    {
        struct d3dkmt_resource *resource;
        if (!(resource = get_d3dkmt_object( params->hResource, D3DKMT_RESOURCE )))
            return STATUS_INVALID_PARAMETER;
        alloc_handle = resource->allocation;
        d3dkmt_object_free( &resource->obj );
    }

    for (i = 0; i < params->AllocationCount; i++)
    {
        if (!(allocation = get_d3dkmt_object( params->phAllocationList[i], D3DKMT_ALLOCATION )))
            return STATUS_INVALID_PARAMETER;
        d3dkmt_object_free( allocation );
    }

    if (alloc_handle && (allocation = get_d3dkmt_object( alloc_handle, D3DKMT_ALLOCATION )))
        d3dkmt_object_free( allocation );

    return STATUS_SUCCESS;
}

/******************************************************************************
 *           NtGdiDdDDIDestroyAllocation    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIDestroyAllocation( const D3DKMT_DESTROYALLOCATION *params )
{
    D3DKMT_DESTROYALLOCATION2 params2 = {0};

    TRACE( "params %p\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;

    params2.hDevice = params->hDevice;
    params2.hResource = params->hResource;
    params2.phAllocationList = params->phAllocationList;
    params2.AllocationCount = params->AllocationCount;
    return NtGdiDdDDIDestroyAllocation2( &params2 );
}

/******************************************************************************
 *           NtGdiDdDDIOpenResource    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenResource( D3DKMT_OPENRESOURCE *params )
{
    struct d3dkmt_object *device, *allocation;
    D3DDDI_OPENALLOCATIONINFO *alloc_info;
    struct d3dkmt_resource *resource;
    UINT runtime_size;
    NTSTATUS status;

    TRACE( "params %p\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;
    if (!(device = get_d3dkmt_object( params->hDevice, D3DKMT_DEVICE ))) return STATUS_INVALID_PARAMETER;
    if (!is_d3dkmt_global( params->hGlobalShare )) return STATUS_INVALID_PARAMETER;
    if (params->ResourcePrivateDriverDataSize) return STATUS_INVALID_PARAMETER;

    if (!params->NumAllocations) return STATUS_INVALID_PARAMETER;
    if (!(alloc_info = params->pOpenAllocationInfo)) return STATUS_INVALID_PARAMETER;

    if ((status = d3dkmt_object_alloc( sizeof(*resource), D3DKMT_RESOURCE, (void **)&resource ))) return status;
    if ((status = d3dkmt_object_alloc( sizeof(*allocation), D3DKMT_ALLOCATION, (void **)&allocation ))) goto failed;

    runtime_size = params->PrivateRuntimeDataSize;
    if ((status = d3dkmt_object_open( &resource->obj, params->hGlobalShare, NULL, params->pPrivateRuntimeData, &runtime_size ))) goto failed;

    if ((status = alloc_object_handle( allocation ))) goto failed;
    resource->allocation = allocation->local;
    alloc_info->hAllocation = allocation->local;
    alloc_info->PrivateDriverDataSize = 0;

    params->hResource = resource->obj.local;
    params->PrivateRuntimeDataSize = runtime_size;
    params->TotalPrivateDriverDataBufferSize = 0;
    params->ResourcePrivateDriverDataSize = 0;
    return STATUS_SUCCESS;

failed:
    d3dkmt_object_free( &resource->obj );
    return status;
}

/******************************************************************************
 *           NtGdiDdDDIOpenResource2    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenResource2( D3DKMT_OPENRESOURCE *params )
{
    struct d3dkmt_object *device, *allocation;
    D3DDDI_OPENALLOCATIONINFO2 *alloc_info;
    struct d3dkmt_resource *resource;
    UINT runtime_size;
    NTSTATUS status;

    TRACE( "params %p\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;
    if (!(device = get_d3dkmt_object( params->hDevice, D3DKMT_DEVICE ))) return STATUS_INVALID_PARAMETER;
    if (!is_d3dkmt_global( params->hGlobalShare )) return STATUS_INVALID_PARAMETER;
    if (params->ResourcePrivateDriverDataSize) return STATUS_INVALID_PARAMETER;

    if (!params->NumAllocations) return STATUS_INVALID_PARAMETER;
    if (!(alloc_info = params->pOpenAllocationInfo2)) return STATUS_INVALID_PARAMETER;

    if ((status = d3dkmt_object_alloc( sizeof(*resource), D3DKMT_RESOURCE, (void **)&resource ))) return status;
    if ((status = d3dkmt_object_alloc( sizeof(*allocation), D3DKMT_ALLOCATION, (void **)&allocation ))) goto failed;

    runtime_size = params->PrivateRuntimeDataSize;
    if ((status = d3dkmt_object_open( &resource->obj, params->hGlobalShare, NULL, params->pPrivateRuntimeData, &runtime_size ))) goto failed;

    if ((status = alloc_object_handle( allocation ))) goto failed;
    resource->allocation = allocation->local;
    alloc_info->hAllocation = allocation->local;
    alloc_info->PrivateDriverDataSize = 0;

    params->hResource = resource->obj.local;
    params->PrivateRuntimeDataSize = runtime_size;
    params->TotalPrivateDriverDataBufferSize = 0;
    params->ResourcePrivateDriverDataSize = 0;
    return STATUS_SUCCESS;

failed:
    d3dkmt_object_free( &resource->obj );
    return status;
}

/******************************************************************************
 *           NtGdiDdDDIOpenResourceFromNtHandle    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenResourceFromNtHandle( D3DKMT_OPENRESOURCEFROMNTHANDLE *params )
{
    struct d3dkmt_object *sync = NULL;
    struct d3dkmt_mutex *mutex = NULL;
    struct d3dkmt_resource *resource = NULL;
    NTSTATUS status;
    UINT dummy = 0;

    FIXME( "params %p semi-stub!\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;
    if (!params->pPrivateRuntimeData) return STATUS_INVALID_PARAMETER;
    if (!params->pTotalPrivateDriverDataBuffer) return STATUS_INVALID_PARAMETER;
    if (!params->pOpenAllocationInfo2) return STATUS_INVALID_PARAMETER;
    if (!params->NumAllocations) return STATUS_INVALID_PARAMETER;

    if ((status = d3dkmt_object_alloc( sizeof(*resource), D3DKMT_RESOURCE, (void **)&resource ))) return status;
    if ((status = d3dkmt_object_alloc( sizeof(*mutex), D3DKMT_MUTEX, (void **)&mutex ))) goto failed;
    if ((status = d3dkmt_object_alloc( sizeof(*sync), D3DKMT_SYNC, (void **)&sync ))) goto failed;

    if ((status = d3dkmt_object_open( &resource->obj, 0, params->hNtHandle, params->pPrivateRuntimeData,
                                      &params->PrivateRuntimeDataSize )))
        goto failed;

    if (d3dkmt_object_open( &mutex->obj, 0, params->hNtHandle, params->pKeyedMutexPrivateRuntimeData, &params->KeyedMutexPrivateRuntimeDataSize ))
    {
        d3dkmt_object_free( &mutex->obj );
        mutex = NULL;
    }

    if (d3dkmt_object_open( sync, 0, params->hNtHandle, NULL, &dummy ))
    {
        d3dkmt_object_free( sync );
        sync = NULL;
    }

    params->hResource = resource->obj.local;
    params->hKeyedMutex = mutex ? mutex->obj.local : 0;
    params->hSyncObject = sync ? sync->local : 0;
    params->TotalPrivateDriverDataBufferSize = 0;
    params->ResourcePrivateDriverDataSize = 0;
    return STATUS_SUCCESS;

failed:
    if (sync) d3dkmt_object_free( sync );
    if (mutex) d3dkmt_object_free( &mutex->obj );
    if (resource) d3dkmt_object_free( &resource->obj );
    return status;
}


/******************************************************************************
 *           NtGdiDdDDIOpenNtHandleFromName    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenNtHandleFromName( D3DKMT_OPENNTHANDLEFROMNAME *params )
{
    OBJECT_ATTRIBUTES *attr = params->pObjAttrib;
    DWORD access = params->dwDesiredAccess;
    NTSTATUS status;

    TRACE( "params %p\n", params );

    params->hNtHandle = 0;
    if ((status = validate_open_object_attributes( attr ))) return status;

    SERVER_START_REQ( d3dkmt_object_open_name )
    {
        req->type       = D3DKMT_RESOURCE;
        req->access     = access;
        req->attributes = attr->Attributes;
        req->rootdir    = wine_server_obj_handle( attr->RootDirectory );
        if (attr->ObjectName) wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        status = wine_server_call( req );
        params->hNtHandle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    return status;
}


/******************************************************************************
 *           NtGdiDdDDIQueryResourceInfo    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIQueryResourceInfo( D3DKMT_QUERYRESOURCEINFO *params )
{
    struct d3dkmt_object *device;
    NTSTATUS status;

    TRACE( "params %p\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;
    if (!(device = get_d3dkmt_object( params->hDevice, D3DKMT_DEVICE ))) return STATUS_INVALID_PARAMETER;
    if (!is_d3dkmt_global( params->hGlobalShare )) return STATUS_INVALID_PARAMETER;

    if ((status = d3dkmt_object_query( D3DKMT_RESOURCE, params->hGlobalShare, NULL,
                                       &params->PrivateRuntimeDataSize )))
        return status;

    params->TotalPrivateDriverDataSize = 0;
    params->ResourcePrivateDriverDataSize = 0;
    params->NumAllocations = 1;
    return STATUS_SUCCESS;
}

/******************************************************************************
 *           NtGdiDdDDIQueryResourceInfoFromNtHandle    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIQueryResourceInfoFromNtHandle( D3DKMT_QUERYRESOURCEINFOFROMNTHANDLE *params )
{
    NTSTATUS status;

    TRACE( "params %p\n", params );

    if ((status = d3dkmt_object_query( D3DKMT_RESOURCE, 0, params->hNtHandle,
                                       &params->PrivateRuntimeDataSize )))
        return status;

    params->TotalPrivateDriverDataSize = 0;
    params->ResourcePrivateDriverDataSize = 0;
    params->NumAllocations = 1;
    return STATUS_SUCCESS;
}


/******************************************************************************
 *           NtGdiDdDDICreateKeyedMutex2    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICreateKeyedMutex2( D3DKMT_CREATEKEYEDMUTEX2 *params )
{
    struct d3dkmt_mutex *mutex;
    NTSTATUS status;

    FIXME( "params %p semi-stub!\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;

    if ((status = d3dkmt_object_alloc( sizeof(*mutex), D3DKMT_MUTEX, (void **)&mutex ))) return status;
    if ((status = d3dkmt_object_create( &mutex->obj, -1, params->InitialValue, params->Flags.NtSecuritySharing,
                                        params->pPrivateRuntimeData, params->PrivateRuntimeDataSize )))
        goto failed;

    params->hSharedHandle = mutex->obj.shared ? 0 : mutex->obj.global;
    params->hKeyedMutex = mutex->obj.local;
    return STATUS_SUCCESS;

failed:
    d3dkmt_object_free( &mutex->obj );
    return status;
}

/******************************************************************************
 *           NtGdiDdDDICreateKeyedMutex    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICreateKeyedMutex( D3DKMT_CREATEKEYEDMUTEX *params )
{
    D3DKMT_CREATEKEYEDMUTEX2 params2 = {0};
    NTSTATUS status;

    TRACE( "params %p\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;

    params2.InitialValue = params->InitialValue;
    status = NtGdiDdDDICreateKeyedMutex2( &params2 );
    params->hSharedHandle = params2.hSharedHandle;
    params->hKeyedMutex = params2.hKeyedMutex;
    return status;
}

/******************************************************************************
 *           NtGdiDdDDIDestroyKeyedMutex    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIDestroyKeyedMutex( const D3DKMT_DESTROYKEYEDMUTEX *params )
{
    struct d3dkmt_mutex *mutex;

    TRACE( "params %p\n", params );

    if (!(mutex = get_d3dkmt_object( params->hKeyedMutex, D3DKMT_MUTEX ))) return STATUS_INVALID_PARAMETER;
    d3dkmt_object_free( &mutex->obj );

    return STATUS_SUCCESS;
}

/******************************************************************************
 *           NtGdiDdDDIOpenKeyedMutex2    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenKeyedMutex2( D3DKMT_OPENKEYEDMUTEX2 *params )
{
    struct d3dkmt_mutex *mutex;
    UINT runtime_size;
    NTSTATUS status;

    TRACE( "params %p\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;
    if (!is_d3dkmt_global( params->hSharedHandle )) return STATUS_INVALID_PARAMETER;
    if (params->PrivateRuntimeDataSize && !params->pPrivateRuntimeData) return STATUS_INVALID_PARAMETER;

    if ((status = d3dkmt_object_alloc( sizeof(*mutex), D3DKMT_MUTEX, (void **)&mutex ))) return status;

    runtime_size = params->PrivateRuntimeDataSize;
    if ((status = d3dkmt_object_open( &mutex->obj, params->hSharedHandle, NULL, params->pPrivateRuntimeData, &runtime_size ))) goto failed;

    params->hKeyedMutex = mutex->obj.local;
    return STATUS_SUCCESS;

failed:
    d3dkmt_object_free( &mutex->obj );
    return status;
}

/******************************************************************************
 *           NtGdiDdDDIOpenKeyedMutex    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenKeyedMutex( D3DKMT_OPENKEYEDMUTEX *params )
{
    D3DKMT_OPENKEYEDMUTEX2 params2 = {0};
    NTSTATUS status;

    TRACE( "params %p\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;

    params2.hSharedHandle = params->hSharedHandle;
    status = NtGdiDdDDIOpenKeyedMutex2( &params2 );
    params->hKeyedMutex = params2.hKeyedMutex;
    return status;
}

/******************************************************************************
 *           NtGdiDdDDIOpenKeyedMutexFromNtHandle    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenKeyedMutexFromNtHandle( D3DKMT_OPENKEYEDMUTEXFROMNTHANDLE *params )
{
    struct d3dkmt_mutex *mutex;
    NTSTATUS status;

    FIXME( "params %p semi-stub!\n", params );

    if ((status = d3dkmt_object_alloc( sizeof(*mutex), D3DKMT_MUTEX, (void **)&mutex ))) return status;
    if ((status = d3dkmt_object_open( &mutex->obj, 0, params->hNtHandle, params->pPrivateRuntimeData,
                                      &params->PrivateRuntimeDataSize )))
        goto failed;

    params->hKeyedMutex = mutex->obj.local;
    return STATUS_SUCCESS;

failed:
    d3dkmt_object_free( &mutex->obj );
    return status;
}

/******************************************************************************
 *           NtGdiDdDDIAcquireKeyedMutex2    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIAcquireKeyedMutex2( D3DKMT_ACQUIREKEYEDMUTEX2 *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIAcquireKeyedMutex    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIAcquireKeyedMutex( D3DKMT_ACQUIREKEYEDMUTEX *params )
{
    D3DKMT_ACQUIREKEYEDMUTEX2 params2 = {0};
    NTSTATUS status;

    TRACE( "params %p\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;
    params2.hKeyedMutex = params->hKeyedMutex;
    params2.pTimeout = params->pTimeout;
    params2.Key = params->Key;
    params2.FenceValue = params->FenceValue;
    status = NtGdiDdDDIAcquireKeyedMutex2( &params2 );
    params->FenceValue = params2.FenceValue;

    return status;
}

/******************************************************************************
 *           NtGdiDdDDIReleaseKeyedMutex2    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIReleaseKeyedMutex2( D3DKMT_RELEASEKEYEDMUTEX2 *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIReleaseKeyedMutex    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIReleaseKeyedMutex( D3DKMT_RELEASEKEYEDMUTEX *params )
{
    D3DKMT_RELEASEKEYEDMUTEX2 params2 = {0};

    TRACE( "params %p\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;
    params2.hKeyedMutex = params->hKeyedMutex;
    params2.Key = params->Key;
    params2.FenceValue = params->FenceValue;
    return NtGdiDdDDIReleaseKeyedMutex2( &params2 );
}


/******************************************************************************
 *           NtGdiDdDDICreateSynchronizationObject2    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICreateSynchronizationObject2( D3DKMT_CREATESYNCHRONIZATIONOBJECT2 *params )
{
    struct d3dkmt_object *device, *sync;
    NTSTATUS status;

    FIXME( "params %p semi-stub!\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;
    if (!(device = get_d3dkmt_object( params->hDevice, D3DKMT_DEVICE ))) return STATUS_INVALID_PARAMETER;

    if (params->Info.Type < D3DDDI_SYNCHRONIZATION_MUTEX || params->Info.Type > D3DDDI_MONITORED_FENCE)
        return STATUS_INVALID_PARAMETER;

    if (params->Info.Type == D3DDDI_CPU_NOTIFICATION && !params->Info.CPUNotification.Event) return STATUS_INVALID_HANDLE;
    if (params->Info.Flags.NtSecuritySharing && !params->Info.Flags.Shared) return STATUS_INVALID_PARAMETER;

    if ((status = d3dkmt_object_alloc( sizeof(*sync), D3DKMT_SYNC, (void **)&sync ))) return status;
    if (!params->Info.Flags.Shared) status = alloc_object_handle( sync );
    else status = d3dkmt_object_create( sync, -1, 0, params->Info.Flags.NtSecuritySharing, NULL, 0 );
    if (status) goto failed;

    if (params->Info.Flags.Shared) params->Info.SharedHandle = sync->shared ? 0 : sync->global;
    params->hSyncObject = sync->local;
    return STATUS_SUCCESS;

failed:
    d3dkmt_object_free( sync );
    return status;
}

/******************************************************************************
 *           NtGdiDdDDICreateSynchronizationObject    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICreateSynchronizationObject( D3DKMT_CREATESYNCHRONIZATIONOBJECT *params )
{
    D3DKMT_CREATESYNCHRONIZATIONOBJECT2 params2 = {0};
    NTSTATUS status;

    TRACE( "params %p\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;

    if (params->Info.Type != D3DDDI_SYNCHRONIZATION_MUTEX && params->Info.Type != D3DDDI_SEMAPHORE)
        return STATUS_INVALID_PARAMETER;

    params2.hDevice = params->hDevice;
    params2.Info.Type = params->Info.Type;
    params2.Info.Flags.Shared = 1;
    memcpy( &params2.Info.Reserved, &params->Info.Reserved, sizeof(params->Info.Reserved) );
    status = NtGdiDdDDICreateSynchronizationObject2( &params2 );
    params->hSyncObject = params2.hSyncObject;
    return status;
}

/******************************************************************************
 *           NtGdiDdDDIOpenSyncObjectFromNtHandle2    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenSyncObjectFromNtHandle2( D3DKMT_OPENSYNCOBJECTFROMNTHANDLE2 *params )
{
    struct d3dkmt_object *sync, *device;
    NTSTATUS status;
    UINT dummy = 0;

    FIXME( "params %p semi-stub!\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;
    if (!(device = get_d3dkmt_object( params->hDevice, D3DKMT_DEVICE ))) return STATUS_INVALID_PARAMETER;

    if ((status = d3dkmt_object_alloc( sizeof(*sync), D3DKMT_SYNC, (void **)&sync ))) return status;
    if ((status = d3dkmt_object_open( sync, 0, params->hNtHandle, NULL, &dummy ))) goto failed;

    params->hSyncObject = sync->local;
    params->MonitoredFence.FenceValueCPUVirtualAddress = 0;
    params->MonitoredFence.FenceValueGPUVirtualAddress = 0;
    return STATUS_SUCCESS;

failed:
    d3dkmt_object_free( sync );
    return status;
}

/******************************************************************************
 *           NtGdiDdDDIOpenSyncObjectFromNtHandle    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenSyncObjectFromNtHandle( D3DKMT_OPENSYNCOBJECTFROMNTHANDLE *params )
{
    struct d3dkmt_object *sync;
    NTSTATUS status;
    UINT dummy = 0;

    FIXME( "params %p semi-stub!\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;

    if ((status = d3dkmt_object_alloc( sizeof(*sync), D3DKMT_SYNC, (void **)&sync ))) return status;
    if ((status = d3dkmt_object_open( sync, 0, params->hNtHandle, NULL, &dummy ))) goto failed;

    params->hSyncObject = sync->local;
    return STATUS_SUCCESS;

failed:
    d3dkmt_object_free( sync );
    return status;
}

/******************************************************************************
 *           NtGdiDdDDIOpenSyncObjectNtHandleFromName    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenSyncObjectNtHandleFromName( D3DKMT_OPENSYNCOBJECTNTHANDLEFROMNAME *params )
{
    OBJECT_ATTRIBUTES *attr = params->pObjAttrib;
    DWORD access = params->dwDesiredAccess;
    NTSTATUS status;

    TRACE( "params %p\n", params );

    params->hNtHandle = 0;
    if ((status = validate_open_object_attributes( attr ))) return status;

    SERVER_START_REQ( d3dkmt_object_open_name )
    {
        req->type       = D3DKMT_SYNC;
        req->access     = access;
        req->attributes = attr->Attributes;
        req->rootdir    = wine_server_obj_handle( attr->RootDirectory );
        if (attr->ObjectName) wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        status = wine_server_call( req );
        params->hNtHandle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    return status;
}

/******************************************************************************
 *           NtGdiDdDDIOpenSynchronizationObject    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenSynchronizationObject( D3DKMT_OPENSYNCHRONIZATIONOBJECT *params )
{
    struct d3dkmt_object *sync;
    NTSTATUS status;
    UINT dummy = 0;

    TRACE( "params %p\n", params );

    if (!params) return STATUS_INVALID_PARAMETER;
    if (!is_d3dkmt_global( params->hSharedHandle )) return STATUS_INVALID_PARAMETER;

    if ((status = d3dkmt_object_alloc( sizeof(*sync), D3DKMT_SYNC, (void **)&sync ))) return status;
    if ((status = d3dkmt_object_open( sync, params->hSharedHandle, NULL, NULL, &dummy ))) goto failed;

    params->hSyncObject = sync->local;
    return STATUS_SUCCESS;

failed:
    d3dkmt_object_free( sync );
    return status;
}

/******************************************************************************
 *           NtGdiDdDDIDestroySynchronizationObject    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIDestroySynchronizationObject( const D3DKMT_DESTROYSYNCHRONIZATIONOBJECT *params )
{
    struct d3dkmt_object *sync;

    TRACE( "params %p\n", params );

    if (!(sync = get_d3dkmt_object( params->hSyncObject, D3DKMT_SYNC )))
        return STATUS_INVALID_PARAMETER;
    d3dkmt_object_free( sync );

    return STATUS_SUCCESS;
}

/* get a locally opened D3DKMT object host-specific fd */
int d3dkmt_object_get_fd( D3DKMT_HANDLE local )
{
    struct d3dkmt_object *object;
    NTSTATUS status;
    int fd;

    TRACE( "local %#x\n", local );

    if (!(object = get_d3dkmt_object( local, -1 ))) return -1;
    if ((status = wine_server_handle_to_fd( object->handle, GENERIC_ALL, &fd, NULL )))
    {
        WARN( "Failed to receive object %p/%#x fd, status %#x\n", object, local, status );
        return -1;
    }

    return fd;
}

/* create a D3DKMT global or shared resource from a host-specific fd */
D3DKMT_HANDLE d3dkmt_create_resource( int fd, D3DKMT_HANDLE *global )
{
    struct d3dkmt_resource *resource = NULL;
    struct d3dkmt_object *allocation = NULL;
    NTSTATUS status;

    TRACE( "fd %d, global %p\n", fd, global );

    if ((status = d3dkmt_object_alloc( sizeof(*resource), D3DKMT_RESOURCE, (void **)&resource ))) goto failed;
    if ((status = d3dkmt_object_alloc( sizeof(*allocation), D3DKMT_ALLOCATION, (void **)&allocation ))) goto failed;
    if ((status = d3dkmt_object_create( &resource->obj, fd, 0, !global, NULL, 0 ))) goto failed;

    if ((status = alloc_object_handle( allocation ))) goto failed;
    resource->allocation = allocation->local;

    if (global) *global = resource->obj.global;
    return resource->obj.local;

failed:
    WARN( "Failed to create resource, status %#x\n", status );
    if (allocation) d3dkmt_object_free( allocation );
    if (resource) d3dkmt_object_free( &resource->obj );
    return 0;
}

/* open a D3DKMT global or shared resource */
D3DKMT_HANDLE d3dkmt_open_resource( D3DKMT_HANDLE global, HANDLE shared )
{
    struct d3dkmt_object *allocation = NULL;
    struct d3dkmt_resource *resource = NULL;
    NTSTATUS status;
    UINT dummy = 0;

    TRACE( "global %#x, shared %p\n", global, shared );

    if ((status = d3dkmt_object_alloc( sizeof(*resource), D3DKMT_RESOURCE, (void **)&resource ))) goto failed;
    if ((status = d3dkmt_object_alloc( sizeof(*allocation), D3DKMT_ALLOCATION, (void **)&allocation ))) goto failed;
    if ((status = d3dkmt_object_open( &resource->obj, global, shared, NULL, &dummy ))) goto failed;

    if ((status = alloc_object_handle( allocation ))) goto failed;
    resource->allocation = allocation->local;

    return resource->obj.local;

failed:
    WARN( "Failed to open resource, status %#x\n", status );
    if (allocation) d3dkmt_object_free( allocation );
    if (resource) d3dkmt_object_free( &resource->obj );
    return 0;
}

/* destroy a locally opened D3DKMT resource */
NTSTATUS d3dkmt_destroy_resource( D3DKMT_HANDLE local )
{
    struct d3dkmt_resource *resource;
    struct d3dkmt_object *allocation;

    TRACE( "local %#x\n", local );

    if (!(resource = get_d3dkmt_object( local, D3DKMT_RESOURCE ))) return STATUS_INVALID_PARAMETER;
    if ((allocation = get_d3dkmt_object( resource->allocation, D3DKMT_RESOURCE ))) d3dkmt_object_free( allocation );
    d3dkmt_object_free( &resource->obj );

    return STATUS_SUCCESS;
}

/* create a D3DKMT global or shared sync */
D3DKMT_HANDLE d3dkmt_create_sync( int fd, D3DKMT_HANDLE *global )
{
    struct d3dkmt_object *sync = NULL;
    NTSTATUS status;

    TRACE( "global %p\n", global );

    if ((status = d3dkmt_object_alloc( sizeof(*sync), D3DKMT_SYNC, (void **)&sync ))) goto failed;
    if ((status = d3dkmt_object_create( sync, fd, 0, !global, NULL, 0 ))) goto failed;
    if (global) *global = sync->global;
    return sync->local;

failed:
    WARN( "Failed to create sync, status %#x\n", status );
    if (sync) d3dkmt_object_free( sync );
    return 0;
}

/* open a D3DKMT global or shared sync */
D3DKMT_HANDLE d3dkmt_open_sync( D3DKMT_HANDLE global, HANDLE shared )
{
    struct d3dkmt_object *sync = NULL;
    NTSTATUS status;
    UINT dummy = 0;

    TRACE( "global %#x, shared %p\n", global, shared );

    if ((status = d3dkmt_object_alloc( sizeof(*sync), D3DKMT_SYNC, (void **)&sync ))) goto failed;
    if ((status = d3dkmt_object_open( sync, global, shared, NULL, &dummy ))) goto failed;
    return sync->local;

failed:
    WARN( "Failed to open sync, status %#x\n", status );
    if (sync) d3dkmt_object_free( sync );
    return 0;
}

/* destroy a locally opened D3DKMT sync */
NTSTATUS d3dkmt_destroy_sync( D3DKMT_HANDLE local )
{
    struct d3dkmt_object *sync;

    TRACE( "local %#x\n", local );

    if (!(sync = get_d3dkmt_object( local, D3DKMT_SYNC ))) return STATUS_INVALID_PARAMETER;
    d3dkmt_object_free( sync );

    return STATUS_SUCCESS;
}
