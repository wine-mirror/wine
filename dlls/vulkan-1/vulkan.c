/* Wine Vulkan loader implementation
 *
 * Copyright 2018 Roderick Colenbrander
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "wine/debug.h"
#define VK_NO_PROTOTYPES
#include "wine/vulkan.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

VkResult WINAPI wine_vkEnumerateInstanceExtensionProperties(const char *, uint32_t *, VkExtensionProperties *);
PFN_vkVoidFunction WINAPI wine_vkGetInstanceProcAddr(VkInstance, const char *);

VkResult WINAPI vkEnumerateInstanceExtensionProperties(const char *layer_name,
        uint32_t *count, VkExtensionProperties *properties)
{
    TRACE("%p %p %p\n", layer_name, count, properties);

    if (layer_name)
        return VK_ERROR_LAYER_NOT_PRESENT;

    return wine_vkEnumerateInstanceExtensionProperties(NULL, count, properties);
}

VkResult WINAPI vkEnumerateInstanceLayerProperties(uint32_t *count,
        VkLayerProperties *properties)
{
    TRACE("%p, %p\n", count, properties);

    /* We don't support any layers. */
    *count = 0;
    return VK_SUCCESS;
}

PFN_vkVoidFunction WINAPI vkGetInstanceProcAddr(VkInstance instance, const char *name)
{
    TRACE("%p %s\n", instance, debugstr_a(name));

    if (!strcmp(name, "vkEnumerateInstanceExtensionProperties"))
        return (PFN_vkVoidFunction)vkEnumerateInstanceExtensionProperties;

    if (!strcmp(name, "vkEnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction)vkEnumerateInstanceLayerProperties;

    if (!strcmp(name, "vkGetInstanceProcAddr"))
        return (PFN_vkVoidFunction)vkGetInstanceProcAddr;

    return wine_vkGetInstanceProcAddr(instance, name);
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, void *reserved)
{
    TRACE("(%p, %u, %p)\n", hinst, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            /* Prefer native as it provides more functionality. */
            return FALSE;

        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinst);
            return TRUE;
    }
    return TRUE;
}
