/*
 * OpenCL.dll proxy for native OpenCL implementation.
 *
 * Copyright 2010 Peter Urbanec
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
#include "opencl_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(opencl);

const struct opencl_funcs *opencl_funcs = NULL;

cl_int WINAPI wine_clGetPlatformInfo(cl_platform_id platform, cl_platform_info param_name,
                                     SIZE_T param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("(%p, 0x%x, %ld, %p, %p)\n", platform, param_name, param_value_size, param_value, param_value_size_ret);

    /* Hide all extensions.
     * TODO: Add individual extension support as needed.
     */
    if (param_name == CL_PLATFORM_EXTENSIONS)
    {
        ret = CL_INVALID_VALUE;

        if (param_value && param_value_size > 0)
        {
            char *exts = (char *) param_value;
            exts[0] = '\0';
            ret = CL_SUCCESS;
        }

        if (param_value_size_ret)
        {
            *param_value_size_ret = 1;
            ret = CL_SUCCESS;
        }
    }
    else
    {
        ret = opencl_funcs->pclGetPlatformInfo(platform, param_name, param_value_size, param_value, param_value_size_ret);
    }

    TRACE("(%p, 0x%x, %ld, %p, %p)=%d\n", platform, param_name, param_value_size, param_value, param_value_size_ret, ret);
    return ret;
}


cl_int WINAPI wine_clGetDeviceInfo(cl_device_id device, cl_device_info param_name,
                                   SIZE_T param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("(%p, 0x%x, %ld, %p, %p)\n",device, param_name, param_value_size, param_value, param_value_size_ret);

    /* Hide all extensions.
     * TODO: Add individual extension support as needed.
     */
    if (param_name == CL_DEVICE_EXTENSIONS)
    {
        ret = CL_INVALID_VALUE;

        if (param_value && param_value_size > 0)
        {
            char *exts = (char *) param_value;
            exts[0] = '\0';
            ret = CL_SUCCESS;
        }

        if (param_value_size_ret)
        {
            *param_value_size_ret = 1;
            ret = CL_SUCCESS;
        }
    }
    else
    {
        ret = opencl_funcs->pclGetDeviceInfo(device, param_name, param_value_size, param_value, param_value_size_ret);
    }

    /* Filter out the CL_EXEC_NATIVE_KERNEL flag */
    if (param_name == CL_DEVICE_EXECUTION_CAPABILITIES)
    {
        cl_device_exec_capabilities *caps = (cl_device_exec_capabilities *) param_value;
        *caps &= ~CL_EXEC_NATIVE_KERNEL;
    }

    TRACE("(%p, 0x%x, %ld, %p, %p)=%d\n",device, param_name, param_value_size, param_value, param_value_size_ret, ret);
    return ret;
}


void * WINAPI wine_clGetExtensionFunctionAddress(const char * func_name)
{
    void * ret = 0;
    TRACE("(%s)\n",func_name);
#if 0
    ret = clGetExtensionFunctionAddress(func_name);
#else
    FIXME("extensions not implemented\n");
#endif
    TRACE("(%s)=%p\n",func_name, ret);
    return ret;
}

BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, void *reserved )
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls( instance );
        return __wine_init_unix_lib( instance, reason, NULL, &opencl_funcs );
    }
    return TRUE;
}
