/*
 * Private winebth.sys defs
 *
 * Copyright 2024 Vibhav Pant
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

#ifndef __WINE_WINEBTH_WINEBTH_H_
#define __WINE_WINEBTH_WINEBTH_H_

#ifdef __ASM_USE_FASTCALL_WRAPPER
extern void * WINAPI wrap_fastcall_func1(void *func, const void *a);
__ASM_STDCALL_FUNC(wrap_fastcall_func1, 8,
                   "popl %ecx\n\t"
                   "popl %eax\n\t"
                   "xchgl (%esp),%ecx\n\t"
                   "jmp *%eax");
#define call_fastcall_func1(func,a) wrap_fastcall_func1(func,a)
#else
#define call_fastcall_func1(func,a) func(a)
#endif

#define XX(i) case (i): return #i


static inline const char *debugstr_minor_function_code( UCHAR code )
{
    switch(code)
    {
        XX(IRP_MN_START_DEVICE);
        XX(IRP_MN_QUERY_REMOVE_DEVICE);
        XX(IRP_MN_REMOVE_DEVICE);
        XX(IRP_MN_CANCEL_REMOVE_DEVICE);
        XX(IRP_MN_STOP_DEVICE);
        XX(IRP_MN_QUERY_STOP_DEVICE);
        XX(IRP_MN_CANCEL_STOP_DEVICE);
        XX(IRP_MN_QUERY_DEVICE_RELATIONS);
        XX(IRP_MN_QUERY_INTERFACE);
        XX(IRP_MN_QUERY_CAPABILITIES);
        XX(IRP_MN_QUERY_RESOURCES);
        XX(IRP_MN_QUERY_RESOURCE_REQUIREMENTS);
        XX(IRP_MN_QUERY_DEVICE_TEXT);
        XX(IRP_MN_FILTER_RESOURCE_REQUIREMENTS);
        XX(IRP_MN_READ_CONFIG);
        XX(IRP_MN_WRITE_CONFIG);
        XX(IRP_MN_EJECT);
        XX(IRP_MN_SET_LOCK);
        XX(IRP_MN_QUERY_ID);
        XX(IRP_MN_QUERY_PNP_DEVICE_STATE);
        XX(IRP_MN_QUERY_BUS_INFORMATION);
        XX(IRP_MN_DEVICE_USAGE_NOTIFICATION);
        XX(IRP_MN_SURPRISE_REMOVAL);
        XX(IRP_MN_QUERY_LEGACY_BUS_INFORMATION);
        default:
            return wine_dbg_sprintf( "(unknown %#x)", code );
    }
}
#undef XX


#endif /* __WINE_WINEBTH_WINEBTH_H_ */
