/*
 * Win32 VxD functions
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Ulrich Weigand
 * Copyright 1998 Patrik Stridvall
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

#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "winioctl.h"
#include "kernel16_private.h"
#include "dosexe.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vxd);

typedef DWORD (WINAPI *VxDCallProc)(DWORD, CONTEXT *);
typedef BOOL (WINAPI *DeviceIoProc)(DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);

struct vxd_module
{
    LARGE_INTEGER index;
    HANDLE        handle;
    HMODULE       module;
    DeviceIoProc  proc;
};

struct vxdcall_service
{
    WCHAR       name[12];
    DWORD       service;
    HMODULE     module;
    VxDCallProc proc;
};

#define MAX_VXD_MODULES 32

static struct vxd_module vxd_modules[MAX_VXD_MODULES];

static struct vxdcall_service vxd_services[] =
{
    { {'v','m','m','.','v','x','d',0},             0x0001, NULL, NULL },
    { {'v','w','i','n','3','2','.','v','x','d',0}, 0x002a, NULL, NULL }
};

#define W32S_APP2WINE(addr) ((addr)? (DWORD)(addr) + W32S_offset : 0)
#define W32S_WINE2APP(addr) ((addr)? (DWORD)(addr) - W32S_offset : 0)

#define VXD_BARF(context,name) \
    TRACE( "vxd %s: unknown/not implemented parameters:\n" \
                     "vxd %s: AX %04x, BX %04x, CX %04x, DX %04x, " \
                     "SI %04x, DI %04x, DS %04x, ES %04x\n", \
             (name), (name), AX_reg(context), BX_reg(context), \
             CX_reg(context), DX_reg(context), SI_reg(context), \
             DI_reg(context), (WORD)context->SegDs, (WORD)context->SegEs )

static CRITICAL_SECTION vxd_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &vxd_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": vxd_section") }
};
static CRITICAL_SECTION vxd_section = { &critsect_debug, -1, 0, 0, 0, 0 };

static UINT W32S_offset;

static WORD VXD_WinVersion(void)
{
    WORD version = LOWORD(GetVersion16());
    return (version >> 8) | (version << 8);
}

/* retrieve the DeviceIoControl function for a Vxd given a file handle */
DeviceIoProc __wine_vxd_get_proc( HANDLE handle )
{
    DeviceIoProc ret = NULL;
    int status, i;
    IO_STATUS_BLOCK io;
    FILE_INTERNAL_INFORMATION info;

    status = NtQueryInformationFile( handle, &io, &info, sizeof(info), FileInternalInformation );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    RtlEnterCriticalSection( &vxd_section );

    for (i = 0; i < MAX_VXD_MODULES; i++)
    {
        if (!vxd_modules[i].module) break;
        if (vxd_modules[i].index.QuadPart == info.IndexNumber.QuadPart)
        {
            if (!(ret = vxd_modules[i].proc)) SetLastError( ERROR_INVALID_FUNCTION );
            goto done;
        }
    }
    /* FIXME: Here we could go through the directory to find the VxD name and load it. */
    /* Let's wait to find out if there are actually apps out there that try to share   */
    /* VxD handles between processes, before we go to the trouble of implementing it.  */
    ERR( "handle %p not found in module list, inherited from another process?\n", handle );

done:
    RtlLeaveCriticalSection( &vxd_section );
    return ret;
}


/* load a VxD and return a file handle to it */
HANDLE __wine_vxd_open( LPCWSTR filenameW, DWORD access, SECURITY_ATTRIBUTES *sa )
{
    int i;
    HANDLE handle;
    HMODULE module;
    WCHAR *p, name[13];

    /* normalize the filename */

    if (wcschr( filenameW, '/' ) || wcschr( filenameW, '\\' ))
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        return 0;
    }
    p = wcschr( filenameW, '.' );
    if (!p && lstrlenW( filenameW ) <= 8)
    {
        wcscpy( name, filenameW );
        wcscat( name, L".vxd" );
    }
    else if (p && !wcsicmp( p, L".vxd" ) && lstrlenW( filenameW ) <= 12)  /* existing extension has to be .vxd */
    {
        wcscpy( name, filenameW );
    }
    else
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        return 0;
    }
    wcslwr( name );

    /* try to load the module first */

    if (!(module = LoadLibraryW( name )))
    {
        FIXME( "Unknown/unsupported VxD %s. Try setting Windows version to 'nt40' or 'win31'.\n",
               debugstr_w(name) );
        SetLastError( ERROR_FILE_NOT_FOUND );
        return 0;
    }

    /* register the module in the global list if necessary */

    RtlEnterCriticalSection( &vxd_section );

    for (i = 0; i < MAX_VXD_MODULES; i++)
    {
        if (vxd_modules[i].module == module)
        {
            handle = vxd_modules[i].handle;
            goto done;  /* already registered */
        }
        if (!vxd_modules[i].module)  /* new one, register it */
        {
            WCHAR path[MAX_PATH];
            IO_STATUS_BLOCK io;
            FILE_INTERNAL_INFORMATION info;

            GetModuleFileNameW( module, path, MAX_PATH );
            handle = CreateFileW( path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
            if (handle == INVALID_HANDLE_VALUE)
            {
                FreeLibrary( module );
                goto done;
            }
            if (!NtQueryInformationFile( handle, &io, &info, sizeof(info), FileInternalInformation ))
                vxd_modules[i].index = info.IndexNumber;

            vxd_modules[i].module = module;
            vxd_modules[i].handle = handle;
            vxd_modules[i].proc = (DeviceIoProc)GetProcAddress( module, "DeviceIoControl" );
            goto done;
        }
    }

    ERR("too many open VxD modules, please report\n" );
    FreeLibrary( module );
    handle = 0;

done:
    RtlLeaveCriticalSection( &vxd_section );
    if (!DuplicateHandle( GetCurrentProcess(), handle, GetCurrentProcess(), &handle, 0,
                          (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle),
                          DUPLICATE_SAME_ACCESS ))
        handle = 0;
    return handle;
}


/***********************************************************************
 *		VxDCall0 (KERNEL32.1)
 *		VxDCall1 (KERNEL32.2)
 *		VxDCall2 (KERNEL32.3)
 *		VxDCall3 (KERNEL32.4)
 *		VxDCall4 (KERNEL32.5)
 *		VxDCall5 (KERNEL32.6)
 *		VxDCall6 (KERNEL32.7)
 *		VxDCall7 (KERNEL32.8)
 *		VxDCall8 (KERNEL32.9)
 */
void WINAPI __regs_VxDCall( CONTEXT *context )
{
    unsigned int i;
    VxDCallProc proc = NULL;
    DWORD service = stack32_pop( context );

    RtlEnterCriticalSection( &vxd_section );
    for (i = 0; i < ARRAY_SIZE(vxd_services); i++)
    {
        if (HIWORD(service) != vxd_services[i].service) continue;
        if (!vxd_services[i].module)  /* need to load it */
        {
            if ((vxd_services[i].module = LoadLibraryW( vxd_services[i].name )))
                vxd_services[i].proc = (VxDCallProc)GetProcAddress( vxd_services[i].module, "VxDCall" );
        }
        proc = vxd_services[i].proc;
        break;
    }
    RtlLeaveCriticalSection( &vxd_section );

    if (proc) context->Eax = proc( service, context );
    else
    {
        FIXME( "Unknown/unimplemented VxD (%08lx)\n", service);
        context->Eax = 0xffffffff; /* FIXME */
    }
}
DEFINE_REGS_ENTRYPOINT( VxDCall )


/***********************************************************************
 *           __wine_vxd_vmm (WPROCS.401)
 */
void WINAPI __wine_vxd_vmm ( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE("[%04x] VMM\n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
        SET_AX( context, VXD_WinVersion() );
        RESET_CFLAG(context);
        break;

    case 0x026d: /* Get_Debug_Flag '/m' */
    case 0x026e: /* Get_Debug_Flag '/n' */
        SET_AL( context, 0 );
        RESET_CFLAG(context);
        break;

    default:
        VXD_BARF( context, "VMM" );
    }
}

/***********************************************************************
 *           __wine_vxd_pagefile (WPROCS.433)
 */
void WINAPI __wine_vxd_pagefile( CONTEXT *context )
{
    unsigned	service = AX_reg(context);

    /* taken from Ralf Brown's Interrupt List */

    TRACE("[%04x] PageFile\n", (UINT16)service );

    switch(service)
    {
    case 0x00: /* get version, is this windows version? */
	TRACE("returning version\n");
        SET_AX( context, VXD_WinVersion() );
	RESET_CFLAG(context);
	break;

    case 0x01: /* get swap file info */
	TRACE("VxD PageFile: returning swap file info\n");
	SET_AX( context, 0x00 ); /* paging disabled */
	context->Ecx = 0;   /* maximum size of paging file */
	/* FIXME: do I touch DS:SI or DS:DI? */
	RESET_CFLAG(context);
	break;

    case 0x02: /* delete permanent swap on exit */
	TRACE("VxD PageFile: supposed to delete swap\n");
	RESET_CFLAG(context);
	break;

    case 0x03: /* current temporary swap file size */
	TRACE("VxD PageFile: what is current temp. swap size\n");
	RESET_CFLAG(context);
	break;

    case 0x04: /* read or write?? INTERRUP.D */
    case 0x05: /* cancel?? INTERRUP.D */
    case 0x06: /* test I/O valid INTERRUP.D */
    default:
	VXD_BARF( context, "pagefile" );
	break;
    }
}

/***********************************************************************
 *           __wine_vxd_reboot (WPROCS.409)
 */
void WINAPI __wine_vxd_reboot( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE("[%04x] Reboot\n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
        SET_AX( context, VXD_WinVersion() );
        RESET_CFLAG(context);
        break;

    default:
        VXD_BARF( context, "REBOOT" );
    }
}

/***********************************************************************
 *           __wine_vxd_vdd (WPROCS.410)
 */
void WINAPI __wine_vxd_vdd( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE("[%04x] VDD\n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
        SET_AX( context, VXD_WinVersion() );
        RESET_CFLAG(context);
        break;

    default:
        VXD_BARF( context, "VDD" );
    }
}

/***********************************************************************
 *           __wine_vxd_vmd (WPROCS.412)
 */
void WINAPI __wine_vxd_vmd( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE("[%04x] VMD\n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
        SET_AX( context, VXD_WinVersion() );
        RESET_CFLAG(context);
        break;

    default:
        VXD_BARF( context, "VMD" );
    }
}

/***********************************************************************
 *           __wine_vxd_vxdloader (WPROCS.439)
 */
void WINAPI __wine_vxd_vxdloader( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE("[%04x] VXDLoader\n", (UINT16)service);

    switch (service)
    {
    case 0x0000: /* get version */
	TRACE("returning version\n");
	SET_AX( context, 0x0000 );
	SET_DX( context, VXD_WinVersion() );
	RESET_CFLAG(context);
	break;

    case 0x0001: /* load device */
	FIXME("load device %04lx:%04x (%s)\n",
	      context->SegDs, DX_reg(context),
	      debugstr_a(MapSL(MAKESEGPTR(context->SegDs, DX_reg(context)))));
	SET_AX( context, 0x0000 );
	context->SegEs = 0x0000;
	SET_DI( context, 0x0000 );
	RESET_CFLAG(context);
	break;

    case 0x0002: /* unload device */
	FIXME("unload device (%08lx)\n", context->Ebx);
	SET_AX( context, 0x0000 );
	RESET_CFLAG(context);
	break;

    default:
	VXD_BARF( context, "VXDLDR" );
	SET_AX( context, 0x000B ); /* invalid function number */
	SET_CFLAG(context);
	break;
    }
}

/***********************************************************************
 *           __wine_vxd_shell (WPROCS.423)
 */
void WINAPI __wine_vxd_shell( CONTEXT *context )
{
    unsigned	service = DX_reg(context);

    TRACE("[%04x] Shell\n", (UINT16)service);

    switch (service) /* Ralf Brown says EDX, but I use DX instead */
    {
    case 0x0000:
	TRACE("returning version\n");
        SET_AX( context, VXD_WinVersion() );
	context->Ebx = 1; /* system VM Handle */
	break;

    case 0x0001:
    case 0x0002:
    case 0x0003:
        /* SHELL_SYSMODAL_Message
	ebx virtual machine handle
	eax message box flags
	ecx address of message
	edi address of caption
	return response in eax
	*/
    case 0x0004:
	/* SHELL_Message
	ebx virtual machine handle
	eax message box flags
	ecx address of message
	edi address of caption
	esi address callback
	edx reference data for callback
	return response in eax
	*/
    case 0x0005:
	VXD_BARF( context, "shell" );
	break;

    case 0x0006: /* SHELL_Get_VM_State */
	TRACE("VxD Shell: returning VM state\n");
	/* Actually we don't, not yet. We have to return a structure
         * and I am not to sure how to set it up and return it yet,
         * so for now let's do nothing. I can (hopefully) get this
         * by the next release
         */
	/* RESET_CFLAG(context); */
	break;

    case 0x0007:
    case 0x0008:
    case 0x0009:
    case 0x000A:
    case 0x000B:
    case 0x000C:
    case 0x000D:
    case 0x000E:
    case 0x000F:
    case 0x0010:
    case 0x0011:
    case 0x0012:
    case 0x0013:
    case 0x0014:
    case 0x0015:
    case 0x0016:
	VXD_BARF( context, "SHELL" );
	break;

    /* the new Win95 shell API */
    case 0x0100:     /* get version */
        SET_AX( context, VXD_WinVersion() );
	break;

    case 0x0104:   /* retrieve Hook_Properties list */
    case 0x0105:   /* call Hook_Properties callbacks */
	VXD_BARF( context, "SHELL" );
	break;

    case 0x0106:   /* install timeout callback */
	TRACE("VxD Shell: ignoring shell callback (%ld sec.)\n", context->Ebx);
	SET_CFLAG(context);
	break;

    case 0x0107:   /* get version of any VxD */
    default:
	VXD_BARF( context, "SHELL" );
	break;
    }
}


/***********************************************************************
 *           __wine_vxd_comm (WPROCS.414)
 */
void WINAPI __wine_vxd_comm( CONTEXT *context )
{
    unsigned	service = AX_reg(context);

    TRACE("[%04x] Comm\n", (UINT16)service);

    switch (service)
    {
    case 0x0000: /* get version */
	TRACE("returning version\n");
        SET_AX( context, VXD_WinVersion() );
	RESET_CFLAG(context);
	break;

    case 0x0001: /* set port global */
    case 0x0002: /* get focus */
    case 0x0003: /* virtualise port */
    default:
        VXD_BARF( context, "comm" );
    }
}

/***********************************************************************
 *           __wine_vxd_timer (WPROCS.405)
 */
void WINAPI __wine_vxd_timer( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE("[%04x] Virtual Timer\n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
	SET_AX( context, VXD_WinVersion() );
	RESET_CFLAG(context);
	break;

    case 0x0100: /* clock tick time, in 840nsecs */
	context->Eax = GetTickCount();

	context->Edx = context->Eax >> 22;
	context->Eax <<= 10; /* not very precise */
	break;

    case 0x0101: /* current Windows time, msecs */
    case 0x0102: /* current VM time, msecs */
	context->Eax = GetTickCount();
	break;

    default:
	VXD_BARF( context, "VTD" );
    }
}


/***********************************************************************
 *           timer_thread
 */
static DWORD CALLBACK timer_thread( void *arg )
{
    DWORD *system_time = arg;

    for (;;)
    {
        *system_time = GetTickCount();
        Sleep( 55 );
    }

    return 0;
}


/***********************************************************************
 *           __wine_vxd_timerapi (WPROCS.1490)
 */
void WINAPI __wine_vxd_timerapi( CONTEXT *context )
{
    static WORD System_Time_Selector;

    unsigned service = AX_reg(context);

    TRACE("[%04x] TimerAPI\n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
        SET_AX( context, VXD_WinVersion() );
        RESET_CFLAG(context);
        break;

    case 0x0009: /* get system time selector */
        if ( !System_Time_Selector )
        {
            HANDLE16 handle = GlobalAlloc16( GMEM_FIXED, sizeof(DWORD) );
            System_Time_Selector = handle | 7;
            CloseHandle( CreateThread( NULL, 0, timer_thread, GlobalLock16(handle), 0, NULL ) );
        }
        SET_AX( context, System_Time_Selector );
        RESET_CFLAG(context);
        break;

    default:
        VXD_BARF( context, "VTDAPI" );
    }
}

/***********************************************************************
 *           __wine_vxd_configmg (WPROCS.451)
 */
void WINAPI __wine_vxd_configmg( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE("[%04x] ConfigMG\n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
        SET_AX( context, VXD_WinVersion() );
        RESET_CFLAG(context);
        break;

    default:
        VXD_BARF( context, "CONFIGMG" );
    }
}

/***********************************************************************
 *           __wine_vxd_enable (WPROCS.455)
 */
void WINAPI __wine_vxd_enable( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE("[%04x] Enable\n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
        SET_AX( context, VXD_WinVersion() );
        RESET_CFLAG(context);
        break;

    default:
        VXD_BARF( context, "ENABLE" );
    }
}

/***********************************************************************
 *           __wine_vxd_apm (WPROCS.438)
 */
void WINAPI __wine_vxd_apm( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE("[%04x] APM\n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
        SET_AX( context, VXD_WinVersion() );
        RESET_CFLAG(context);
        break;

    default:
        VXD_BARF( context, "APM" );
    }
}

/***********************************************************************
 *           __wine_vxd_win32s (WPROCS.445)
 *
 * This is an implementation of the services of the Win32s VxD.
 * Since official documentation of these does not seem to be available,
 * certain arguments of some of the services remain unclear.
 *
 * FIXME: The following services are currently unimplemented:
 *        Exception handling      (0x01, 0x1C)
 *        Debugger support        (0x0C, 0x14, 0x17)
 *        Low-level memory access (0x02, 0x03, 0x0A, 0x0B)
 *        Memory Statistics       (0x1B)
 *
 *
 * We have a specific problem running Win32s on Linux (and probably also
 * the other x86 unixes), since Win32s tries to allocate its main 'flat
 * code/data segment' selectors with a base of 0xffff0000 (and limit 4GB).
 * The rationale for this seems to be that they want one the one hand to
 * be able to leave the Win 3.1 memory (starting with the main DOS memory)
 * at linear address 0, but want at other hand to have offset 0 of the
 * flat data/code segment point to an unmapped page (to catch NULL pointer
 * accesses). Hence they allocate the flat segments with a base of 0xffff0000
 * so that the Win 3.1 memory area at linear address zero shows up in the
 * flat segments at offset 0x10000 (since linear addresses wrap around at
 * 4GB). To compensate for that discrepancy between flat segment offsets
 * and plain linear addresses, all flat pointers passed between the 32-bit
 * and the 16-bit parts of Win32s are shifted by 0x10000 in the appropriate
 * direction by the glue code (mainly) in W32SKRNL and WIN32S16.
 *
 * The problem for us is now that Linux does not allow a LDT selector with
 * base 0xffff0000 to be created, since it would 'see' a part of the kernel
 * address space. To address this problem we introduce *another* offset:
 * We add 0x10000 to every linear address we get as an argument from Win32s.
 * This means especially that the flat code/data selectors get actually
 * allocated with base 0x0, so that flat offsets and (real) linear addresses
 * do again agree!  In fact, every call e.g. of a Win32s VxD service now
 * has all pointer arguments (which are offsets in the flat data segment)
 * first reduced by 0x10000 by the W32SKRNL glue code, and then again
 * increased by 0x10000 by *our* code.
 *
 * Note that to keep everything consistent, this offset has to be applied by
 * every Wine function that operates on 'linear addresses' passed to it by
 * Win32s. Fortunately, since Win32s does not directly call any Wine 32-bit
 * API routines, this affects only two locations: this VxD and the DPMI
 * handler. (NOTE: Should any Win32s application pass a linear address to
 * any routine apart from those, e.g. some other VxD handler, that code
 * would have to take the offset into account as well!)
 *
 * The offset is set the first time any application calls the GetVersion()
 * service of the Win32s VxD. (Note that the offset is never reset.)
 *
 */
void WINAPI __wine_vxd_win32s( CONTEXT *context )
{
    switch (AX_reg(context))
    {
    case 0x0000: /* Get Version */
        /*
         * Input:   None
         *
         * Output:  EAX: LoWord: Win32s Version (1.30)
         *               HiWord: VxD Version (200)
         *
         *          EBX: Build (172)
         *
         *          ECX: ???   (1)
         *
         *          EDX: Debugging Flags
         *
         *          EDI: Error Flag
         *               0 if OK,
         *               1 if VMCPD VxD not found
         */

        TRACE("GetVersion()\n");

	context->Eax = VXD_WinVersion() | (200 << 16);
        context->Ebx = 0;
        context->Ecx = 0;
        context->Edx = 0;
        context->Edi = 0;

        /*
         * If this is the first time we are called for this process,
         * hack the memory image of WIN32S16 so that it doesn't try
         * to access the GDT directly ...
         *
         * The first code segment of WIN32S16 (version 1.30) contains
         * an unexported function somewhere between the exported functions
         * SetFS and StackLinearToSegmented that tries to find a selector
         * in the LDT that maps to the memory image of the LDT itself.
         * If it succeeds, it stores this selector into a global variable
         * which will be used to speed up execution by using this selector
         * to modify the LDT directly instead of using the DPMI calls.
         *
         * To perform this search of the LDT, this function uses the
         * sgdt and sldt instructions to find the linear address of
         * the (GDT and then) LDT. While those instructions themselves
         * execute without problem, the linear address that sgdt returns
         * points (at least under Linux) to the kernel address space, so
         * that any subsequent access leads to a segfault.
         *
         * Fortunately, WIN32S16 still contains as a fallback option the
         * mechanism of using DPMI calls to modify LDT selectors instead
         * of direct writes to the LDT. Thus we can circumvent the problem
         * by simply replacing the first byte of the offending function
         * with an 'retf' instruction. This means that the global variable
         * supposed to contain the LDT alias selector will remain zero,
         * and hence WIN32S16 will fall back to using DPMI calls.
         *
         * The heuristic we employ to _find_ that function is as follows:
         * We search between the addresses of the exported symbols SetFS
         * and StackLinearToSegmented for the byte sequence '0F 01 04'
         * (this is the opcode of 'sgdt [si]'). We then search backwards
         * from this address for the last occurrence of 'CB' (retf) that marks
         * the end of the preceding function. The following byte (which
         * should now be the first byte of the function we are looking for)
         * will be replaced by 'CB' (retf).
         *
         * This heuristic works for the retail as well as the debug version
         * of Win32s version 1.30. For versions earlier than that this
         * hack should not be necessary at all, since the whole mechanism
         * ('PERF130') was introduced only in 1.30 to improve the overall
         * performance of Win32s.
         */

        if (!W32S_offset)
        {
            HMODULE16 hModule = GetModuleHandle16("win32s16");
            SEGPTR func1 = (SEGPTR)GetProcAddress16(hModule, "SetFS");
            SEGPTR func2 = (SEGPTR)GetProcAddress16(hModule, "StackLinearToSegmented");

            if (   hModule && func1 && func2
                && SELECTOROF(func1) == SELECTOROF(func2))
            {
                BYTE *start = MapSL(func1);
                BYTE *end   = MapSL(func2);
                BYTE *p, *retv = NULL;
                int found = 0;

                for (p = start; p < end; p++)
                    if (*p == 0xCB) found = 0, retv = p;
                    else if (*p == 0x0F) found = 1;
                    else if (*p == 0x01 && found == 1) found = 2;
                    else if (*p == 0x04 && found == 2) { found = 3; break; }
                    else found = 0;

                if (found == 3 && retv)
                {
                    TRACE("PERF130 hack: "
                               "Replacing byte %02X at offset %04X:%04X\n",
                               *(retv+1), SELECTOROF(func1),
                                          OFFSETOF(func1) + retv+1-start);

                    *(retv+1) = (BYTE)0xCB;
                }
            }
        }

        /*
         * Mark process as Win32s, so that subsequent DPMI calls
         * will perform the W32S_APP2WINE/W32S_WINE2APP address shift.
         */
        W32S_offset = 0x10000;
        break;


    case 0x0001: /* Install Exception Handling */
        /*
         * Input:   EBX: Flat address of W32SKRNL Exception Data
         *
         *          ECX: LoWord: Flat Code Selector
         *               HiWord: Flat Data Selector
         *
         *          EDX: Flat address of W32SKRNL Exception Handler
         *               (this is equal to W32S_BackTo32 + 0x40)
         *
         *          ESI: SEGPTR KERNEL.HASGPHANDLER
         *
         *          EDI: SEGPTR phCurrentTask (KERNEL.THHOOK + 0x10)
         *
         * Output:  EAX: 0 if OK
         */

        TRACE("[0001] EBX=%lx ECX=%lx EDX=%lx ESI=%lx EDI=%lx\n",
                   context->Ebx, context->Ecx, context->Edx,
                   context->Esi, context->Edi);

        /* FIXME */

        context->Eax = 0;
        break;


    case 0x0002: /* Set Page Access Flags */
        /*
         * Input:   EBX: New access flags
         *               Bit 2: User Page if set, Supervisor Page if clear
         *               Bit 1: Read-Write if set, Read-Only if clear
         *
         *          ECX: Size of memory area to change
         *
         *          EDX: Flat start address of memory area
         *
         * Output:  EAX: Size of area changed
         */

        TRACE("[0002] EBX=%lx ECX=%lx EDX=%lx\n",
                   context->Ebx, context->Ecx, context->Edx);

        /* FIXME */

        context->Eax = context->Ecx;
        break;


    case 0x0003: /* Get Page Access Flags */
        /*
         * Input:   EDX: Flat address of page to query
         *
         * Output:  EAX: Page access flags
         *               Bit 2: User Page if set, Supervisor Page if clear
         *               Bit 1: Read-Write if set, Read-Only if clear
         */

        TRACE("[0003] EDX=%lx\n", context->Edx);

        /* FIXME */

        context->Eax = 6;
        break;


    case 0x0004: /* Map Module */
        /*
         * Input:   ECX: IMTE (offset in Module Table) of new module
         *
         *          EDX: Flat address of Win32s Module Table
         *
         * Output:  EAX: 0 if OK
         */

    if (!context->Edx || CX_reg(context) == 0xFFFF)
    {
        TRACE("MapModule: Initialization call\n");
        context->Eax = 0;
    }
    else
    {
        /*
         * Structure of a Win32s Module Table Entry:
         */
        struct Win32sModule
        {
            DWORD  flags;
            DWORD  flatBaseAddr;
            LPCSTR moduleName;
            LPCSTR pathName;
            LPCSTR unknown;
            LPBYTE baseAddr;
            DWORD  hModule;
            DWORD  relocDelta;
        };

        /*
         * Note: This function should set up a demand-paged memory image
         *       of the given module. Since mmap does not allow file offsets
         *       not aligned at 1024 bytes, we simply load the image fully
         *       into memory.
         */

        struct Win32sModule *moduleTable =
                            (struct Win32sModule *)W32S_APP2WINE(context->Edx);
        struct Win32sModule *module = moduleTable + context->Ecx;

        IMAGE_NT_HEADERS *nt_header = RtlImageNtHeader( (HMODULE)module->baseAddr );
        IMAGE_SECTION_HEADER *pe_seg = IMAGE_FIRST_SECTION( nt_header );
        HFILE image = _lopen(module->pathName, OF_READ);
        BOOL error = (image == HFILE_ERROR);
        UINT i;

        TRACE("MapModule: Loading %s\n", module->pathName);

        for (i = 0;
             !error && i < nt_header->FileHeader.NumberOfSections;
             i++, pe_seg++)
            if(!(pe_seg->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA))
            {
                DWORD  off  = pe_seg->PointerToRawData;
                DWORD  len  = pe_seg->SizeOfRawData;
                LPBYTE addr = module->baseAddr + pe_seg->VirtualAddress;

                TRACE("MapModule: "
                           "Section %d at %08lx from %08lx len %08lx\n",
                           i, (DWORD)addr, off, len);

                if (   _llseek(image, off, SEEK_SET) != off
                    || _lread(image, addr, len) != len)
                    error = TRUE;
            }

        _lclose(image);

        if (error)
            ERR("MapModule: Unable to load %s\n", module->pathName);

        else if (module->relocDelta != 0)
        {
            IMAGE_DATA_DIRECTORY *dir = nt_header->OptionalHeader.DataDirectory
                                      + IMAGE_DIRECTORY_ENTRY_BASERELOC;
            IMAGE_BASE_RELOCATION *r = (IMAGE_BASE_RELOCATION *)
                (dir->Size? module->baseAddr + dir->VirtualAddress : 0);

            TRACE("MapModule: Reloc delta %08lx\n", module->relocDelta);

            while (r && r->VirtualAddress)
            {
                LPBYTE page  = module->baseAddr + r->VirtualAddress;
                WORD *TypeOffset = (WORD *)(r + 1);
                unsigned int count = (r->SizeOfBlock - sizeof(*r)) / sizeof(*TypeOffset);

                TRACE("MapModule: %d relocations for page %08lx\n",
                           count, (DWORD)page);

                for(i = 0; i < count; i++)
                {
                    int offset = TypeOffset[i] & 0xFFF;
                    int type   = TypeOffset[i] >> 12;
                    switch(type)
                    {
                    case IMAGE_REL_BASED_ABSOLUTE:
                        break;
                    case IMAGE_REL_BASED_HIGH:
                        *(WORD *)(page+offset) += HIWORD(module->relocDelta);
                        break;
                    case IMAGE_REL_BASED_LOW:
                        *(WORD *)(page+offset) += LOWORD(module->relocDelta);
                        break;
                    case IMAGE_REL_BASED_HIGHLOW:
                        *(DWORD*)(page+offset) += module->relocDelta;
                        break;
                    default:
                        WARN("MapModule: Unsupported fixup type\n");
                        break;
                    }
                }

                r = (IMAGE_BASE_RELOCATION *)((LPBYTE)r + r->SizeOfBlock);
            }
        }

        context->Eax = 0;
        RESET_CFLAG(context);
    }
    break;


    case 0x0005: /* UnMap Module */
        /*
         * Input:   EDX: Flat address of module image
         *
         * Output:  EAX: 1 if OK
         */

        TRACE("UnMapModule: %lx\n", W32S_APP2WINE(context->Edx));

        /* As we didn't map anything, there's nothing to unmap ... */

        context->Eax = 1;
        break;


    case 0x0006: /* VirtualAlloc */
        /*
         * Input:   ECX: Current Process
         *
         *          EDX: Flat address of arguments on stack
         *
         *   DWORD *retv     [out] Flat base address of allocated region
         *   LPVOID base     [in]  Flat address of region to reserve/commit
         *   DWORD  size     [in]  Size of region
         *   DWORD  type     [in]  Type of allocation
         *   DWORD  prot     [in]  Type of access protection
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack  = (DWORD *)W32S_APP2WINE(context->Edx);
        DWORD *retv   = (DWORD *)W32S_APP2WINE(stack[0]);
        LPVOID base   = (LPVOID) W32S_APP2WINE(stack[1]);
        DWORD  size   = stack[2];
        DWORD  type   = stack[3];
        DWORD  prot   = stack[4];
        DWORD  result;

        TRACE("VirtualAlloc(%lx, %lx, %lx, %lx, %lx)\n",
                   (DWORD)retv, (DWORD)base, size, type, prot);

        if (type & 0x80000000)
        {
            WARN("VirtualAlloc: strange type %lx\n", type);
            type &= 0x7fffffff;
        }

        if (!base && (type & MEM_COMMIT) && prot == PAGE_READONLY)
        {
            WARN("VirtualAlloc: NLS hack, allowing write access!\n");
            prot = PAGE_READWRITE;
        }

        result = (DWORD)VirtualAlloc(base, size, type, prot);

        if (W32S_WINE2APP(result))
            *retv            = W32S_WINE2APP(result),
            context->Eax = STATUS_SUCCESS;
        else
            *retv            = 0,
            context->Eax = STATUS_NO_MEMORY;  /* FIXME */
    }
    break;


    case 0x0007: /* VirtualFree */
        /*
         * Input:   ECX: Current Process
         *
         *          EDX: Flat address of arguments on stack
         *
         *   DWORD *retv     [out] TRUE if success, FALSE if failure
         *   LPVOID base     [in]  Flat address of region
         *   DWORD  size     [in]  Size of region
         *   DWORD  type     [in]  Type of operation
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack  = (DWORD *)W32S_APP2WINE(context->Edx);
        DWORD *retv   = (DWORD *)W32S_APP2WINE(stack[0]);
        LPVOID base   = (LPVOID) W32S_APP2WINE(stack[1]);
        DWORD  size   = stack[2];
        DWORD  type   = stack[3];
        DWORD  result;

        TRACE("VirtualFree(%lx, %lx, %lx, %lx)\n",
                   (DWORD)retv, (DWORD)base, size, type);

        result = VirtualFree(base, size, type);

        if (result)
            *retv            = TRUE,
            context->Eax = STATUS_SUCCESS;
        else
            *retv            = FALSE,
            context->Eax = STATUS_NO_MEMORY;  /* FIXME */
    }
    break;


    case 0x0008: /* VirtualProtect */
        /*
         * Input:   ECX: Current Process
         *
         *          EDX: Flat address of arguments on stack
         *
         *   DWORD *retv     [out] TRUE if success, FALSE if failure
         *   LPVOID base     [in]  Flat address of region
         *   DWORD  size     [in]  Size of region
         *   DWORD  new_prot [in]  Desired access protection
         *   DWORD *old_prot [out] Previous access protection
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack    = (DWORD *)W32S_APP2WINE(context->Edx);
        DWORD *retv     = (DWORD *)W32S_APP2WINE(stack[0]);
        LPVOID base     = (LPVOID) W32S_APP2WINE(stack[1]);
        DWORD  size     = stack[2];
        DWORD  new_prot = stack[3];
        DWORD *old_prot = (DWORD *)W32S_APP2WINE(stack[4]);
        DWORD  result;

        TRACE("VirtualProtect(%lx, %lx, %lx, %lx, %lx)\n",
                   (DWORD)retv, (DWORD)base, size, new_prot, (DWORD)old_prot);

        result = VirtualProtect(base, size, new_prot, old_prot);

        if (result)
            *retv            = TRUE,
            context->Eax = STATUS_SUCCESS;
        else
            *retv            = FALSE,
            context->Eax = STATUS_NO_MEMORY;  /* FIXME */
    }
    break;


    case 0x0009: /* VirtualQuery */
        /*
         * Input:   ECX: Current Process
         *
         *          EDX: Flat address of arguments on stack
         *
         *   DWORD *retv                     [out] Nr. bytes returned
         *   LPVOID base                     [in]  Flat address of region
         *   LPMEMORY_BASIC_INFORMATION info [out] Info buffer
         *   DWORD  len                      [in]  Size of buffer
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack  = (DWORD *)W32S_APP2WINE(context->Edx);
        DWORD *retv   = (DWORD *)W32S_APP2WINE(stack[0]);
        LPVOID base   = (LPVOID) W32S_APP2WINE(stack[1]);
        PMEMORY_BASIC_INFORMATION info =
                        (PMEMORY_BASIC_INFORMATION)W32S_APP2WINE(stack[2]);
        DWORD  len    = stack[3];
        DWORD  result;

        TRACE("VirtualQuery(%lx, %lx, %lx, %lx)\n",
                   (DWORD)retv, (DWORD)base, (DWORD)info, len);

        result = VirtualQuery(base, info, len);

        *retv            = result;
        context->Eax = STATUS_SUCCESS;
    }
    break;


    case 0x000A: /* SetVirtMemProcess */
        /*
         * Input:   ECX: Process Handle
         *
         *          EDX: Flat address of region
         *
         * Output:  EAX: NtStatus
         */

        TRACE("[000a] ECX=%lx EDX=%lx\n",
                   context->Ecx, context->Edx);

        /* FIXME */

        context->Eax = STATUS_SUCCESS;
        break;


    case 0x000B: /* ??? some kind of cleanup */
        /*
         * Input:   ECX: Process Handle
         *
         * Output:  EAX: NtStatus
         */

        TRACE("[000b] ECX=%lx\n", context->Ecx);

        /* FIXME */

        context->Eax = STATUS_SUCCESS;
        break;


    case 0x000C: /* Set Debug Flags */
        /*
         * Input:   EDX: Debug Flags
         *
         * Output:  EDX: Previous Debug Flags
         */

        FIXME("[000c] EDX=%lx\n", context->Edx);

        /* FIXME */

        context->Edx = 0;
        break;


    case 0x000D: /* NtCreateSection */
        /*
         * Input:   EDX: Flat address of arguments on stack
         *
         *   HANDLE32 *retv      [out] Handle of Section created
         *   DWORD  flags1       [in]  (?? unknown ??)
         *   DWORD  atom         [in]  Name of Section to create
         *   LARGE_INTEGER *size [in]  Size of Section
         *   DWORD  protect      [in]  Access protection
         *   DWORD  flags2       [in]  (?? unknown ??)
         *   HFILE32 hFile       [in]  Handle of file to map
         *   DWORD  psp          [in]  (Win32s: PSP that hFile belongs to)
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack    = (DWORD *)   W32S_APP2WINE(context->Edx);
        HANDLE *retv  = (HANDLE *)W32S_APP2WINE(stack[0]);
        DWORD  flags1   = stack[1];
        DWORD  atom     = stack[2];
        LARGE_INTEGER *size = (LARGE_INTEGER *)W32S_APP2WINE(stack[3]);
        DWORD  protect  = stack[4];
        DWORD  flags2   = stack[5];
        HANDLE hFile    = DosFileHandleToWin32Handle(stack[6]);
        DWORD  psp      = stack[7];

        HANDLE result = INVALID_HANDLE_VALUE;
        char name[128];

        TRACE("NtCreateSection(%lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx)\n",
                   (DWORD)retv, flags1, atom, (DWORD)size, protect, flags2,
                   (DWORD)hFile, psp);

        if (!atom || GlobalGetAtomNameA(atom, name, sizeof(name)))
        {
            TRACE("NtCreateSection: name=%s\n", atom? name : "");

            result = CreateFileMappingA(hFile, NULL, protect,
                                          size? size->u.HighPart : 0,
                                          size? size->u.LowPart  : 0,
                                          atom? name : NULL);
        }

        if (result == INVALID_HANDLE_VALUE)
            WARN("NtCreateSection: failed!\n");
        else
            TRACE("NtCreateSection: returned %lx\n", (DWORD)result);

        if (result != INVALID_HANDLE_VALUE)
            *retv            = result,
            context->Eax = STATUS_SUCCESS;
        else
            *retv            = result,
            context->Eax = STATUS_NO_MEMORY;   /* FIXME */
    }
    break;


    case 0x000E: /* NtOpenSection */
        /*
         * Input:   EDX: Flat address of arguments on stack
         *
         *   HANDLE32 *retv  [out] Handle of Section opened
         *   DWORD  protect  [in]  Access protection
         *   DWORD  atom     [in]  Name of Section to create
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack    = (DWORD *)W32S_APP2WINE(context->Edx);
        HANDLE *retv  = (HANDLE *)W32S_APP2WINE(stack[0]);
        DWORD  protect  = stack[1];
        DWORD  atom     = stack[2];

        HANDLE result = INVALID_HANDLE_VALUE;
        char name[128];

        TRACE("NtOpenSection(%lx, %lx, %lx)\n",
                   (DWORD)retv, protect, atom);

        if (atom && GlobalGetAtomNameA(atom, name, sizeof(name)))
        {
            TRACE("NtOpenSection: name=%s\n", name);

            result = OpenFileMappingA(protect, FALSE, name);
        }

        if (result == INVALID_HANDLE_VALUE)
            WARN("NtOpenSection: failed!\n");
        else
            TRACE("NtOpenSection: returned %lx\n", (DWORD)result);

        if (result != INVALID_HANDLE_VALUE)
            *retv            = result,
            context->Eax = STATUS_SUCCESS;
        else
            *retv            = result,
            context->Eax = STATUS_NO_MEMORY;   /* FIXME */
    }
    break;


    case 0x000F: /* NtCloseSection */
        /*
         * Input:   EDX: Flat address of arguments on stack
         *
         *   HANDLE32 handle  [in]  Handle of Section to close
         *   DWORD *id        [out] Unique ID  (?? unclear ??)
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack    = (DWORD *)W32S_APP2WINE(context->Edx);
        HANDLE handle   = (HANDLE)stack[0];
        DWORD *id       = (DWORD *)W32S_APP2WINE(stack[1]);

        TRACE("NtCloseSection(%lx, %lx)\n", (DWORD)handle, (DWORD)id);

        CloseHandle(handle);
        if (id) *id = 0; /* FIXME */

        context->Eax = STATUS_SUCCESS;
    }
    break;


    case 0x0010: /* NtDupSection */
        /*
         * Input:   EDX: Flat address of arguments on stack
         *
         *   HANDLE32 handle  [in]  Handle of Section to duplicate
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack    = (DWORD *)W32S_APP2WINE(context->Edx);
        HANDLE handle   = (HANDLE)stack[0];
        HANDLE new_handle;

        TRACE("NtDupSection(%lx)\n", (DWORD)handle);

        DuplicateHandle( GetCurrentProcess(), handle,
                         GetCurrentProcess(), &new_handle,
                         0, FALSE, DUPLICATE_SAME_ACCESS );
        context->Eax = STATUS_SUCCESS;
    }
    break;


    case 0x0011: /* NtMapViewOfSection */
        /*
         * Input:   EDX: Flat address of arguments on stack
         *
         *   HANDLE32 SectionHandle       [in]     Section to be mapped
         *   DWORD    ProcessHandle       [in]     Process to be mapped into
         *   DWORD *  BaseAddress         [in/out] Address to be mapped at
         *   DWORD    ZeroBits            [in]     Upper address bits that should be null, starting from bit 31
         *   DWORD    CommitSize          [in]     (?? unclear ??)
         *   LARGE_INTEGER *SectionOffset [in]     Offset within section
         *   DWORD *  ViewSize            [in]     Size of view
         *   DWORD    InheritDisposition  [in]     (?? unclear ??)
         *   DWORD    AllocationType      [in]     (?? unclear ??)
         *   DWORD    Protect             [in]     Access protection
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *  stack          = (DWORD *)W32S_APP2WINE(context->Edx);
        HANDLE   SectionHandle  = (HANDLE)stack[0];
        DWORD    ProcessHandle  = stack[1]; /* ignored */
        DWORD *  BaseAddress    = (DWORD *)W32S_APP2WINE(stack[2]);
        DWORD    ZeroBits       = stack[3];
        DWORD    CommitSize     = stack[4];
        LARGE_INTEGER *SectionOffset = (LARGE_INTEGER *)W32S_APP2WINE(stack[5]);
        DWORD *  ViewSize       = (DWORD *)W32S_APP2WINE(stack[6]);
        DWORD    InheritDisposition = stack[7];
        DWORD    AllocationType = stack[8];
        DWORD    Protect        = stack[9];

        LPBYTE address = (LPBYTE)(BaseAddress?
			W32S_APP2WINE(*BaseAddress) : 0);
        DWORD  access = 0, result;

        switch (Protect & ~(PAGE_GUARD|PAGE_NOCACHE))
        {
            case PAGE_READONLY:           access = FILE_MAP_READ;  break;
            case PAGE_READWRITE:          access = FILE_MAP_WRITE; break;
            case PAGE_WRITECOPY:          access = FILE_MAP_COPY;  break;

            case PAGE_EXECUTE_READ:       access = FILE_MAP_READ;  break;
            case PAGE_EXECUTE_READWRITE:  access = FILE_MAP_WRITE; break;
            case PAGE_EXECUTE_WRITECOPY:  access = FILE_MAP_COPY;  break;
        }

        TRACE("NtMapViewOfSection"
                   "(%lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx)\n",
                   (DWORD)SectionHandle, ProcessHandle, (DWORD)BaseAddress,
                   ZeroBits, CommitSize, (DWORD)SectionOffset, (DWORD)ViewSize,
                   InheritDisposition, AllocationType, Protect);
        TRACE("NtMapViewOfSection: "
                   "base=%lx, offset=%lx, size=%lx, access=%lx\n",
                   (DWORD)address, SectionOffset? SectionOffset->u.LowPart : 0,
                   ViewSize? *ViewSize : 0, access);

        result = (DWORD)MapViewOfFileEx(SectionHandle, access,
                            SectionOffset? SectionOffset->u.HighPart : 0,
                            SectionOffset? SectionOffset->u.LowPart  : 0,
                            ViewSize? *ViewSize : 0, address);

        TRACE("NtMapViewOfSection: result=%lx\n", result);

        if (W32S_WINE2APP(result))
        {
            if (BaseAddress) *BaseAddress = W32S_WINE2APP(result);
            context->Eax = STATUS_SUCCESS;
        }
        else
            context->Eax = STATUS_NO_MEMORY; /* FIXME */
    }
    break;


    case 0x0012: /* NtUnmapViewOfSection */
        /*
         * Input:   EDX: Flat address of arguments on stack
         *
         *   DWORD  ProcessHandle  [in]  Process (defining address space)
         *   LPBYTE BaseAddress    [in]  Base address of view to be unmapped
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack          = (DWORD *)W32S_APP2WINE(context->Edx);
        DWORD  ProcessHandle  = stack[0]; /* ignored */
        LPBYTE BaseAddress    = (LPBYTE)W32S_APP2WINE(stack[1]);

        TRACE("NtUnmapViewOfSection(%lx, %lx)\n",
                   ProcessHandle, (DWORD)BaseAddress);

        UnmapViewOfFile(BaseAddress);

        context->Eax = STATUS_SUCCESS;
    }
    break;


    case 0x0013: /* NtFlushVirtualMemory */
        /*
         * Input:   EDX: Flat address of arguments on stack
         *
         *   DWORD   ProcessHandle  [in]  Process (defining address space)
         *   LPBYTE *BaseAddress    [in?] Base address of range to be flushed
         *   DWORD  *ViewSize       [in?] Number of bytes to be flushed
         *   DWORD  *unknown        [???] (?? unknown ??)
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack          = (DWORD *)W32S_APP2WINE(context->Edx);
        DWORD  ProcessHandle  = stack[0]; /* ignored */
        DWORD *BaseAddress    = (DWORD *)W32S_APP2WINE(stack[1]);
        DWORD *ViewSize       = (DWORD *)W32S_APP2WINE(stack[2]);
        DWORD *unknown        = (DWORD *)W32S_APP2WINE(stack[3]);

        LPBYTE address = (LPBYTE)(BaseAddress? W32S_APP2WINE(*BaseAddress) : 0);
        DWORD  size    = ViewSize? *ViewSize : 0;

        TRACE("NtFlushVirtualMemory(%lx, %lx, %lx, %lx)\n",
                   ProcessHandle, (DWORD)BaseAddress, (DWORD)ViewSize,
                   (DWORD)unknown);
        TRACE("NtFlushVirtualMemory: base=%lx, size=%lx\n",
                   (DWORD)address, size);

        FlushViewOfFile(address, size);

        context->Eax = STATUS_SUCCESS;
    }
    break;


    case 0x0014: /* Get/Set Debug Registers */
        /*
         * Input:   ECX: 0 if Get, 1 if Set
         *
         *          EDX: Get: Flat address of buffer to receive values of
         *                    debug registers DR0 .. DR7
         *               Set: Flat address of buffer containing values of
         *                    debug registers DR0 .. DR7 to be set
         * Output:  None
         */

        FIXME("[0014] ECX=%lx EDX=%lx\n",
                   context->Ecx, context->Edx);

        /* FIXME */
        break;


    case 0x0015: /* Set Coprocessor Emulation Flag */
        /*
         * Input:   EDX: 0 to deactivate, 1 to activate coprocessor emulation
         *
         * Output:  None
         */

        TRACE("[0015] EDX=%lx\n", context->Edx);

        /* We don't care, as we always have a coprocessor anyway */
        break;


    case 0x0016: /* Init Win32S VxD PSP */
        /*
         * If called to query required PSP size:
         *
         *     Input:  EBX: 0
         *     Output: EDX: Required size of Win32s VxD PSP
         *
         * If called to initialize allocated PSP:
         *
         *     Input:  EBX: LoWord: Selector of Win32s VxD PSP
         *                  HiWord: Paragraph of Win32s VxD PSP (DOSMEM)
         *     Output: None
         */

        if (context->Ebx == 0)
            context->Edx = 0x80;
        else
        {
            PDB16 *psp = MapSL( MAKESEGPTR( BX_reg(context), 0 ));
            psp->nbFiles = 32;
            psp->fileHandlesPtr = MAKELONG(HIWORD(context->Ebx), 0x5c);
            memset((LPBYTE)psp + 0x5c, '\xFF', 32);
        }
        break;


    case 0x0017: /* Set Break Point */
        /*
         * Input:   EBX: Offset of Break Point
         *          CX:  Selector of Break Point
         *
         * Output:  None
         */

        FIXME("[0017] EBX=%lx CX=%x\n",
                   context->Ebx, CX_reg(context));

        /* FIXME */
        break;


    case 0x0018: /* VirtualLock */
        /*
         * Input:   ECX: Current Process
         *
         *          EDX: Flat address of arguments on stack
         *
         *   DWORD *retv     [out] TRUE if success, FALSE if failure
         *   LPVOID base     [in]  Flat address of range to lock
         *   DWORD  size     [in]  Size of range
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack  = (DWORD *)W32S_APP2WINE(context->Edx);
        DWORD *retv   = (DWORD *)W32S_APP2WINE(stack[0]);
        LPVOID base   = (LPVOID) W32S_APP2WINE(stack[1]);
        DWORD  size   = stack[2];
        DWORD  result;

        TRACE("VirtualLock(%lx, %lx, %lx)\n",
                   (DWORD)retv, (DWORD)base, size);

        result = VirtualLock(base, size);

        if (result)
            *retv            = TRUE,
            context->Eax = STATUS_SUCCESS;
        else
            *retv            = FALSE,
            context->Eax = STATUS_NO_MEMORY;  /* FIXME */
    }
    break;


    case 0x0019: /* VirtualUnlock */
        /*
         * Input:   ECX: Current Process
         *
         *          EDX: Flat address of arguments on stack
         *
         *   DWORD *retv     [out] TRUE if success, FALSE if failure
         *   LPVOID base     [in]  Flat address of range to unlock
         *   DWORD  size     [in]  Size of range
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack  = (DWORD *)W32S_APP2WINE(context->Edx);
        DWORD *retv   = (DWORD *)W32S_APP2WINE(stack[0]);
        LPVOID base   = (LPVOID) W32S_APP2WINE(stack[1]);
        DWORD  size   = stack[2];
        DWORD  result;

        TRACE("VirtualUnlock(%lx, %lx, %lx)\n",
                   (DWORD)retv, (DWORD)base, size);

        result = VirtualUnlock(base, size);

        if (result)
            *retv            = TRUE,
            context->Eax = STATUS_SUCCESS;
        else
            *retv            = FALSE,
            context->Eax = STATUS_NO_MEMORY;  /* FIXME */
    }
    break;


    case 0x001A: /* KGetSystemInfo */
        /*
         * Input:   None
         *
         * Output:  ECX:  Start of sparse memory arena
         *          EDX:  End of sparse memory arena
         */

        TRACE("KGetSystemInfo()\n");

        /*
         * Note: Win32s reserves 0GB - 2GB for Win 3.1 and uses 2GB - 4GB as
         *       sparse memory arena. We do it the other way around, since
         *       we have to reserve 3GB - 4GB for Linux, and thus use
         *       0GB - 3GB as sparse memory arena.
         *
         *       FIXME: What about other OSes ?
         */

        context->Ecx = W32S_WINE2APP(0x00000000);
        context->Edx = W32S_WINE2APP(0xbfffffff);
        break;


    case 0x001B: /* KGlobalMemStat */
        /*
         * Input:   ESI: Flat address of buffer to receive memory info
         *
         * Output:  None
         */
    {
        struct Win32sMemoryInfo
        {
            DWORD DIPhys_Count;       /* Total physical pages */
            DWORD DIFree_Count;       /* Free physical pages */
            DWORD DILin_Total_Count;  /* Total virtual pages (private arena) */
            DWORD DILin_Total_Free;   /* Free virtual pages (private arena) */

            DWORD SparseTotal;        /* Total size of sparse arena (bytes ?) */
            DWORD SparseFree;         /* Free size of sparse arena (bytes ?) */
        };

        struct Win32sMemoryInfo *info =
                       (struct Win32sMemoryInfo *)W32S_APP2WINE(context->Esi);

        FIXME("KGlobalMemStat(%lx)\n", (DWORD)info);

        /* FIXME */
    }
    break;


    case 0x001C: /* Enable/Disable Exceptions */
        /*
         * Input:   ECX: 0 to disable, 1 to enable exception handling
         *
         * Output:  None
         */

        TRACE("[001c] ECX=%lx\n", context->Ecx);

        /* FIXME */
        break;


    case 0x001D: /* VirtualAlloc called from 16-bit code */
        /*
         * Input:   EDX: Segmented address of arguments on stack
         *
         *   LPVOID base     [in]  Flat address of region to reserve/commit
         *   DWORD  size     [in]  Size of region
         *   DWORD  type     [in]  Type of allocation
         *   DWORD  prot     [in]  Type of access protection
         *
         * Output:  EAX: NtStatus
         *          EDX: Flat base address of allocated region
         */
    {
        DWORD *stack  = MapSL( MAKESEGPTR( LOWORD(context->Edx), HIWORD(context->Edx) ));
        LPVOID base   = (LPVOID)W32S_APP2WINE(stack[0]);
        DWORD  size   = stack[1];
        DWORD  type   = stack[2];
        DWORD  prot   = stack[3];
        DWORD  result;

        TRACE("VirtualAlloc16(%lx, %lx, %lx, %lx)\n",
                   (DWORD)base, size, type, prot);

        if (type & 0x80000000)
        {
            WARN("VirtualAlloc16: strange type %lx\n", type);
            type &= 0x7fffffff;
        }

        result = (DWORD)VirtualAlloc(base, size, type, prot);

        if (W32S_WINE2APP(result))
            context->Edx = W32S_WINE2APP(result),
            context->Eax = STATUS_SUCCESS;
        else
            context->Edx = 0,
            context->Eax = STATUS_NO_MEMORY;  /* FIXME */
	TRACE("VirtualAlloc16: returning base %lx\n", context->Edx);
    }
    break;


    case 0x001E: /* VirtualFree called from 16-bit code */
        /*
         * Input:   EDX: Segmented address of arguments on stack
         *
         *   LPVOID base     [in]  Flat address of region
         *   DWORD  size     [in]  Size of region
         *   DWORD  type     [in]  Type of operation
         *
         * Output:  EAX: NtStatus
         *          EDX: TRUE if success, FALSE if failure
         */
    {
        DWORD *stack  = MapSL( MAKESEGPTR( LOWORD(context->Edx), HIWORD(context->Edx) ));
        LPVOID base   = (LPVOID)W32S_APP2WINE(stack[0]);
        DWORD  size   = stack[1];
        DWORD  type   = stack[2];
        DWORD  result;

        TRACE("VirtualFree16(%lx, %lx, %lx)\n",
                   (DWORD)base, size, type);

        result = VirtualFree(base, size, type);

        if (result)
            context->Edx = TRUE,
            context->Eax = STATUS_SUCCESS;
        else
            context->Edx = FALSE,
            context->Eax = STATUS_NO_MEMORY;  /* FIXME */
    }
    break;


    case 0x001F: /* FWorkingSetSize */
        /*
         * Input:   EDX: 0 if Get, 1 if Set
         *
         *          ECX: Get: Buffer to receive Working Set Size
         *               Set: Buffer containing Working Set Size
         *
         * Output:  NtStatus
         */
    {
        DWORD *ptr = (DWORD *)W32S_APP2WINE(context->Ecx);
        BOOL set = context->Edx;

        TRACE("FWorkingSetSize(%lx, %lx)\n", (DWORD)ptr, (DWORD)set);

        if (set)
            /* We do it differently ... */;
        else
            *ptr = 0x100;

        context->Eax = STATUS_SUCCESS;
    }
    break;

    default:
	VXD_BARF( context, "W32S" );
    }
}
