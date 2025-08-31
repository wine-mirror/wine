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

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

enum d3dkmt_type
{
    D3DKMT_ADAPTER      = 1,
    D3DKMT_DEVICE       = 2,
    D3DKMT_SOURCE       = 3,
};

struct d3dkmt_object
{
    enum d3dkmt_type    type;           /* object type */
    D3DKMT_HANDLE       local;          /* object local handle */
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

static struct d3dkmt_object **objects;
static unsigned int object_count, object_capacity, last_index;

/* return the position of the first object which handle is not less than
 * the given local handle, d3dkmt_lock must be held. */
static unsigned int object_handle_lower_bound( D3DKMT_HANDLE local )
{
    unsigned int begin = 0, end = object_count, mid;

    while (begin < end)
    {
        mid = begin + (end - begin) / 2;
        if (objects[mid]->local < local) begin = mid + 1;
        else end = mid;
    }

    return begin;
}

/* allocate a d3dkmt object with a local handle */
static NTSTATUS alloc_object_handle( struct d3dkmt_object *object )
{
    D3DKMT_HANDLE handle = 0;
    unsigned int index;

    pthread_mutex_lock( &d3dkmt_lock );

    if (object_count >= object_capacity)
    {
        unsigned int capacity = max( 32, object_capacity * 3 / 2 );
        struct d3dkmt_object **tmp;
        assert( capacity > object_capacity );

        if (capacity >= 0xffff) goto done;
        if (!(tmp = realloc( objects, capacity * sizeof(*objects) ))) goto done;
        object_capacity = capacity;
        objects = tmp;
    }

    last_index += 0x40;
    handle = object->local = (last_index & ~0xc0000000) | 0x40000000;
    index = object_handle_lower_bound( object->local );
    if (index < object_count) memmove( objects + index + 1, objects, (object_count - index) * sizeof(*objects) );
    objects[index] = object;
    object_count++;

done:
    pthread_mutex_unlock( &d3dkmt_lock );
    return handle ? STATUS_SUCCESS : STATUS_NO_MEMORY;
}

/* free a d3dkmt local object handle */
static void free_object_handle( struct d3dkmt_object *object )
{
    unsigned int index;

    pthread_mutex_lock( &d3dkmt_lock );
    index = object_handle_lower_bound( object->local );
    assert( index < object_count && objects[index] == object );
    object_count--;
    object->local = 0;
    memmove( objects + index, objects + index + 1, (object_count - index) * sizeof(*objects) );
    pthread_mutex_unlock( &d3dkmt_lock );
}

/* return a pointer to a d3dkmt object from its local handle */
static void *get_d3dkmt_object( D3DKMT_HANDLE local, enum d3dkmt_type type )
{
    struct d3dkmt_object *object;
    unsigned int index;

    pthread_mutex_lock( &d3dkmt_lock );
    index = object_handle_lower_bound( local );
    if (index >= object_count) object = NULL;
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

static void d3dkmt_object_free( struct d3dkmt_object *object )
{
    if (object->local) free_object_handle( object );
    free( object );
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
    FIXME( "(%p): stub\n", desc );
    return STATUS_NO_MEMORY;
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
                desc->CurrentUsage += budget.heapUsage[i];
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
    FIXME( "count %u, handles %p, attr %p, access %#x, handle %p stub!\n", count, handles, attr, access, handle );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDICreateAllocation2    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICreateAllocation2( D3DKMT_CREATEALLOCATION *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDICreateAllocation    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICreateAllocation( D3DKMT_CREATEALLOCATION *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIDestroyAllocation2    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIDestroyAllocation2( const D3DKMT_DESTROYALLOCATION2 *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIDestroyAllocation    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIDestroyAllocation( const D3DKMT_DESTROYALLOCATION *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIOpenResource    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenResource( D3DKMT_OPENRESOURCE *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIOpenResource2    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenResource2( D3DKMT_OPENRESOURCE *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIOpenResourceFromNtHandle    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenResourceFromNtHandle( D3DKMT_OPENRESOURCEFROMNTHANDLE *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIQueryResourceInfo    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIQueryResourceInfo( D3DKMT_QUERYRESOURCEINFO *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIQueryResourceInfoFromNtHandle    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIQueryResourceInfoFromNtHandle( D3DKMT_QUERYRESOURCEINFOFROMNTHANDLE *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *           NtGdiDdDDICreateKeyedMutex2    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICreateKeyedMutex2( D3DKMT_CREATEKEYEDMUTEX2 *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDICreateKeyedMutex    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICreateKeyedMutex( D3DKMT_CREATEKEYEDMUTEX *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIDestroyKeyedMutex    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIDestroyKeyedMutex( const D3DKMT_DESTROYKEYEDMUTEX *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIOpenKeyedMutex2    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenKeyedMutex2( D3DKMT_OPENKEYEDMUTEX2 *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIOpenKeyedMutex    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenKeyedMutex( D3DKMT_OPENKEYEDMUTEX *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIOpenKeyedMutexFromNtHandle    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenKeyedMutexFromNtHandle( D3DKMT_OPENKEYEDMUTEXFROMNTHANDLE *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *           NtGdiDdDDICreateSynchronizationObject2    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICreateSynchronizationObject2( D3DKMT_CREATESYNCHRONIZATIONOBJECT2 *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDICreateSynchronizationObject    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICreateSynchronizationObject( D3DKMT_CREATESYNCHRONIZATIONOBJECT *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIOpenSyncObjectFromNtHandle2    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenSyncObjectFromNtHandle2( D3DKMT_OPENSYNCOBJECTFROMNTHANDLE2 *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIOpenSyncObjectFromNtHandle    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenSyncObjectFromNtHandle( D3DKMT_OPENSYNCOBJECTFROMNTHANDLE *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIOpenSyncObjectNtHandleFromName    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenSyncObjectNtHandleFromName( D3DKMT_OPENSYNCOBJECTNTHANDLEFROMNAME *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIOpenSynchronizationObject    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenSynchronizationObject( D3DKMT_OPENSYNCHRONIZATIONOBJECT *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDIDestroySynchronizationObject    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIDestroySynchronizationObject( const D3DKMT_DESTROYSYNCHRONIZATIONOBJECT *params )
{
    FIXME( "params %p stub!\n", params );
    return STATUS_NOT_IMPLEMENTED;
}
