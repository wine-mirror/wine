/*
 * VMM VxD implementation
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vxd);

/*
 * VMM VxDCall service names are (mostly) taken from Stan Mitchell's
 * "Inside the Windows 95 File System"
 */

#define N_VMM_SERVICE 41

static const char * const VMM_Service_Name[N_VMM_SERVICE] =
{
    "PageReserve",            /* 0x0000 */
    "PageCommit",             /* 0x0001 */
    "PageDecommit",           /* 0x0002 */
    "PagerRegister",          /* 0x0003 */
    "PagerQuery",             /* 0x0004 */
    "HeapAllocate",           /* 0x0005 */
    "ContextCreate",          /* 0x0006 */
    "ContextDestroy",         /* 0x0007 */
    "PageAttach",             /* 0x0008 */
    "PageFlush",              /* 0x0009 */
    "PageFree",               /* 0x000A */
    "ContextSwitch",          /* 0x000B */
    "HeapReAllocate",         /* 0x000C */
    "PageModifyPermissions",  /* 0x000D */
    "PageQuery",              /* 0x000E */
    "GetCurrentContext",      /* 0x000F */
    "HeapFree",               /* 0x0010 */
    "RegOpenKey",             /* 0x0011 */
    "RegCreateKey",           /* 0x0012 */
    "RegCloseKey",            /* 0x0013 */
    "RegDeleteKey",           /* 0x0014 */
    "RegSetValue",            /* 0x0015 */
    "RegDeleteValue",         /* 0x0016 */
    "RegQueryValue",          /* 0x0017 */
    "RegEnumKey",             /* 0x0018 */
    "RegEnumValue",           /* 0x0019 */
    "RegQueryValueEx",        /* 0x001A */
    "RegSetValueEx",          /* 0x001B */
    "RegFlushKey",            /* 0x001C */
    "RegQueryInfoKey",        /* 0x001D */
    "GetDemandPageInfo",      /* 0x001E */
    "BlockOnID",              /* 0x001F */
    "SignalID",               /* 0x0020 */
    "RegLoadKey",             /* 0x0021 */
    "RegUnLoadKey",           /* 0x0022 */
    "RegSaveKey",             /* 0x0023 */
    "RegRemapPreDefKey",      /* 0x0024 */
    "PageChangePager",        /* 0x0025 */
    "RegQueryMultipleValues", /* 0x0026 */
    "RegReplaceKey",          /* 0x0027 */
    "<KERNEL32.101>"          /* 0x0028 -- What does this do??? */
};

/* PageReserve arena values */
#define PR_PRIVATE    0x80000400 /* anywhere in private arena */
#define PR_SHARED     0x80060000 /* anywhere in shared arena */
#define PR_SYSTEM     0x80080000 /* anywhere in system arena */

/* PageReserve flags */
#define PR_FIXED      0x00000008 /* don't move during PageReAllocate */
#define PR_4MEG       0x00000001 /* allocate on 4mb boundary */
#define PR_STATIC     0x00000010 /* see PageReserve documentation */

/* PageCommit default pager handle values */
#define PD_ZEROINIT   0x00000001 /* swappable zero-initialized pages */
#define PD_NOINIT     0x00000002 /* swappable uninitialized pages */
#define PD_FIXEDZERO  0x00000003 /* fixed zero-initialized pages */
#define PD_FIXED      0x00000004 /* fixed uninitialized pages */

/* PageCommit flags */
#define PC_FIXED      0x00000008 /* pages are permanently locked */
#define PC_LOCKED     0x00000080 /* pages are made present and locked */
#define PC_LOCKEDIFDP 0x00000100 /* pages are locked if swap via DOS */
#define PC_WRITABLE   0x00020000 /* make the pages writable */
#define PC_USER       0x00040000 /* make the pages ring 3 accessible */
#define PC_INCR       0x40000000 /* increment "pagerdata" each page */
#define PC_PRESENT    0x80000000 /* make pages initially present */
#define PC_STATIC     0x20000000 /* allow commit in PR_STATIC object */
#define PC_DIRTY      0x08000000 /* make pages initially dirty */
#define PC_CACHEDIS   0x00100000 /* Allocate uncached pages - new for WDM */
#define PC_CACHEWT    0x00080000 /* Allocate write through cache pages - new for WDM */
#define PC_PAGEFLUSH  0x00008000 /* Touch device mapped pages on alloc - new for WDM */

/* PageCommitContig additional flags */
#define PCC_ZEROINIT  0x00000001 /* zero-initialize new pages */
#define PCC_NOLIN     0x10000000 /* don't map to any linear address */


static const DWORD page_size = 0x1000;  /* we only care about x86 */

/* Pop a DWORD from the 32-bit stack */
static inline DWORD stack32_pop( CONTEXT *context )
{
    DWORD ret = *(DWORD *)context->Esp;
    context->Esp += sizeof(DWORD);
    return ret;
}


/***********************************************************************
 *           VxDCall   (VMM.VXD.@)
 */
DWORD WINAPI VMM_VxDCall( DWORD service, CONTEXT *context )
{
    static int warned;

    switch ( LOWORD(service) )
    {
    case 0x0000: /* PageReserve */
    {
        LPVOID address;
        LPVOID ret;
        ULONG page   = stack32_pop( context );
        ULONG npages = stack32_pop( context );
        ULONG flags  = stack32_pop( context );

        TRACE("PageReserve: page: %08lx, npages: %08lx, flags: %08lx partial stub!\n",
              page, npages, flags );

        if ( page == PR_SYSTEM ) {
          ERR("Can't reserve ring 1 memory\n");
          return -1;
        }
        /* FIXME: This has to be handled separately for the separate
           address-spaces we now have */
        if ( page == PR_PRIVATE || page == PR_SHARED ) page = 0;
        /* FIXME: Handle flags in some way */
        address = (LPVOID )(page * page_size);
        ret = VirtualAlloc ( address, npages * page_size, MEM_RESERVE, PAGE_EXECUTE_READWRITE );
        TRACE("PageReserve: returning: %p\n", ret );
        if ( ret == NULL )
          return -1;
        else
          return (DWORD )ret;
    }

    case 0x0001: /* PageCommit */
    {
        LPVOID address;
        LPVOID ret;
        DWORD virt_perm;
        ULONG page   = stack32_pop( context );
        ULONG npages = stack32_pop( context );
        ULONG hpd  = stack32_pop( context );
        ULONG pagerdata   = stack32_pop( context );
        ULONG flags  = stack32_pop( context );

        TRACE("PageCommit: page: %08lx, npages: %08lx, hpd: %08lx pagerdata: "
              "%08lx, flags: %08lx partial stub\n",
              page, npages, hpd, pagerdata, flags );

        if ( flags & PC_USER )
          if ( flags & PC_WRITABLE )
            virt_perm = PAGE_EXECUTE_READWRITE;
          else
            virt_perm = PAGE_EXECUTE_READ;
        else
          virt_perm = PAGE_NOACCESS;

        address = (LPVOID )(page * page_size);
        ret = VirtualAlloc ( address, npages * page_size, MEM_COMMIT, virt_perm );
        TRACE("PageCommit: Returning: %p\n", ret );
        return (DWORD )ret;

    }

    case 0x0002: /* PageDecommit */
    {
        LPVOID address;
        BOOL ret;
        ULONG page = stack32_pop( context );
        ULONG npages = stack32_pop( context );
        ULONG flags = stack32_pop( context );

        TRACE("PageDecommit: page: %08lx, npages: %08lx, flags: %08lx partial stub\n",
              page, npages, flags );
        address = (LPVOID )( page * page_size );
        ret = VirtualFree ( address, npages * page_size, MEM_DECOMMIT );
        TRACE("PageDecommit: Returning: %s\n", ret ? "TRUE" : "FALSE" );
        return ret;
    }

    case 0x000d: /* PageModifyPermissions */
    {
        DWORD pg_old_perm;
        DWORD pg_new_perm;
        DWORD virt_old_perm;
        DWORD virt_new_perm;
        MEMORY_BASIC_INFORMATION mbi;
        LPVOID address;
        ULONG page = stack32_pop ( context );
        ULONG npages = stack32_pop ( context );
        ULONG permand = stack32_pop ( context );
        ULONG permor = stack32_pop ( context );

        TRACE("PageModifyPermissions %08lx %08lx %08lx %08lx partial stub\n",
              page, npages, permand, permor );
        address = (LPVOID )( page * page_size );

        VirtualQuery ( address, &mbi, sizeof ( MEMORY_BASIC_INFORMATION ));
        virt_old_perm = mbi.Protect;

        switch ( virt_old_perm & mbi.Protect ) {
        case PAGE_READONLY:
        case PAGE_EXECUTE:
        case PAGE_EXECUTE_READ:
          pg_old_perm = PC_USER;
          break;
        case PAGE_READWRITE:
        case PAGE_WRITECOPY:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
          pg_old_perm = PC_USER | PC_WRITABLE;
          break;
        case PAGE_NOACCESS:
        default:
          pg_old_perm = 0;
          break;
        }
        pg_new_perm = pg_old_perm;
        pg_new_perm &= permand & ~PC_STATIC;
        pg_new_perm |= permor  & ~PC_STATIC;

        virt_new_perm = ( virt_old_perm )  & ~0xff;
        if ( pg_new_perm & PC_USER )
        {
          if ( pg_new_perm & PC_WRITABLE )
            virt_new_perm |= PAGE_EXECUTE_READWRITE;
          else
            virt_new_perm |= PAGE_EXECUTE_READ;
        }

        if ( ! VirtualProtect ( address, npages * page_size, virt_new_perm, &virt_old_perm ) ) {
          ERR("Can't change page permissions for %p\n", address );
          return 0xffffffff;
        }
        TRACE("Returning: %08lx\n", pg_old_perm );
        return pg_old_perm;
    }

    case 0x000a: /* PageFree */
    {
        BOOL ret;
        LPVOID hmem = (LPVOID) stack32_pop( context );
        DWORD flags = stack32_pop( context );

        TRACE("PageFree: hmem: %p, flags: %08lx partial stub\n",
              hmem, flags );

        ret = VirtualFree ( hmem, 0, MEM_RELEASE );
        TRACE("Returning: %d\n", ret );
        return ret;
    }

    case 0x0011:  /* RegOpenKey */
        stack32_pop( context ); /* kkey */
        stack32_pop( context ); /* lpszSubKey */
        stack32_pop( context ); /* retkey */
        /* return RegOpenKeyExA( hkey, lpszSubKey, 0, KEY_ALL_ACCESS, retkey ); */
        if (!warned)
        {
            ERR( "Using the native Win95 advapi32.dll is no longer supported.\n" );
            ERR( "Please configure advapi32 to builtin.\n" );
            warned++;
        }
        return ERROR_CALL_NOT_IMPLEMENTED;

    case 0x0012:  /* RegCreateKey */
        stack32_pop( context ); /* hkey */
        stack32_pop( context ); /* lpszSubKey */
        stack32_pop( context ); /* retkey */
        /* return RegCreateKeyA( hkey, lpszSubKey, retkey ); */
        if (!warned)
        {
            ERR( "Using the native Win95 advapi32.dll is no longer supported.\n" );
            ERR( "Please configure advapi32 to builtin.\n" );
            warned++;
        }
        return ERROR_CALL_NOT_IMPLEMENTED;

    case 0x0013:  /* RegCloseKey */
        stack32_pop( context ); /* hkey */
        /* return RegCloseKey( hkey ); */
        return ERROR_CALL_NOT_IMPLEMENTED;

    case 0x0014:  /* RegDeleteKey */
        stack32_pop( context ); /* hkey */
        stack32_pop( context ); /* lpszSubKey */
        /* return RegDeleteKeyA( hkey, lpszSubKey ); */
        return ERROR_CALL_NOT_IMPLEMENTED;

    case 0x0015:  /* RegSetValue */
        stack32_pop( context ); /* hkey */
        stack32_pop( context ); /* lpszSubKey */
        stack32_pop( context ); /* dwType */
        stack32_pop( context ); /* lpszData */
        stack32_pop( context ); /* cbData */
        /* return RegSetValueA( hkey, lpszSubKey, dwType, lpszData, cbData ); */
        return ERROR_CALL_NOT_IMPLEMENTED;

    case 0x0016:  /* RegDeleteValue */
        stack32_pop( context ); /* hkey */
        stack32_pop( context ); /* lpszValue */
        /* return RegDeleteValueA( hkey, lpszValue ); */
        return ERROR_CALL_NOT_IMPLEMENTED;

    case 0x0017:  /* RegQueryValue */
        stack32_pop( context ); /* hkey */
        stack32_pop( context ); /* lpszSubKey */
        stack32_pop( context ); /* lpszData */
        stack32_pop( context ); /* lpcbData */
        /* return RegQueryValueA( hkey, lpszSubKey, lpszData, lpcbData ); */
        return ERROR_CALL_NOT_IMPLEMENTED;

    case 0x0018:  /* RegEnumKey */
        stack32_pop( context ); /* hkey */
        stack32_pop( context ); /* iSubkey */
        stack32_pop( context ); /* lpszName */
        stack32_pop( context ); /* lpcchName */
        /* return RegEnumKeyA( hkey, iSubkey, lpszName, lpcchName ); */
        return ERROR_CALL_NOT_IMPLEMENTED;

    case 0x0019:  /* RegEnumValue */
        stack32_pop( context ); /* hkey */
        stack32_pop( context ); /* iValue */
        stack32_pop( context ); /* lpszValue */
        stack32_pop( context ); /* lpcchValue */
        stack32_pop( context ); /* lpReserved */
        stack32_pop( context ); /* lpdwType */
        stack32_pop( context ); /* lpbData */
        stack32_pop( context ); /* lpcbData */
        /* return RegEnumValueA( hkey, iValue, lpszValue, lpcchValue, lpReserved, lpdwType, lpbData, lpcbData ); */
        return ERROR_CALL_NOT_IMPLEMENTED;

    case 0x001A:  /* RegQueryValueEx */
        stack32_pop( context ); /* hkey */
        stack32_pop( context ); /* lpszValue */
        stack32_pop( context ); /* lpReserved */
        stack32_pop( context ); /* lpdwType */
        stack32_pop( context ); /* lpbData */
        stack32_pop( context ); /* lpcbData */
        /* return RegQueryValueExA( hkey, lpszValue, lpReserved, lpdwType, lpbData, lpcbData ); */
        return ERROR_CALL_NOT_IMPLEMENTED;

    case 0x001B:  /* RegSetValueEx */
        stack32_pop( context ); /* hkey */
        stack32_pop( context ); /* lpszValue */
        stack32_pop( context ); /* dwReserved */
        stack32_pop( context ); /* dwType */
        stack32_pop( context ); /* lpbData */
        stack32_pop( context ); /* cbData */
        /* return RegSetValueExA( hkey, lpszValue, dwReserved, dwType, lpbData, cbData ); */
        return ERROR_CALL_NOT_IMPLEMENTED;

    case 0x001C:  /* RegFlushKey */
        stack32_pop( context ); /* hkey */
        /* return RegFlushKey( hkey ); */
        return ERROR_CALL_NOT_IMPLEMENTED;

    case 0x001D:  /* RegQueryInfoKey */
        /* NOTE: This VxDCall takes only a subset of the parameters that the
                 corresponding Win32 API call does. The implementation in Win95
                 ADVAPI32 sets all output parameters not mentioned here to zero. */
        stack32_pop( context ); /* hkey */
        stack32_pop( context ); /* lpcSubKeys */
        stack32_pop( context ); /* lpcchMaxSubKey */
        stack32_pop( context ); /* lpcValues */
        stack32_pop( context ); /* lpcchMaxValueName */
        stack32_pop( context ); /* lpcchMaxValueData */
        /* return RegQueryInfoKeyA( hkey, lpcSubKeys, lpcchMaxSubKey, lpcValues, lpcchMaxValueName, lpcchMaxValueData ); */
        return ERROR_CALL_NOT_IMPLEMENTED;

    case 0x001e: /* GetDemandPageInfo */
    {
         DWORD dinfo = stack32_pop( context );
         DWORD flags = stack32_pop( context );

         /* GetDemandPageInfo is supposed to fill out the struct at
          * "dinfo" with various low-level memory management information.
          * Apps are certainly not supposed to call this, although it's
          * demoed and documented by Pietrek on pages 441-443 of "Windows
          * 95 System Programming Secrets" if any program needs a real
          * implementation of this.
          */

         FIXME("GetDemandPageInfo(%08lx %08lx): stub!\n", dinfo, flags);

         return 0;
    }

    case 0x0021:  /* RegLoadKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        LPCSTR  lpszFile   = (LPCSTR)stack32_pop( context );
        FIXME("RegLoadKey(%p,%s,%s): stub\n",hkey, debugstr_a(lpszSubKey), debugstr_a(lpszFile));
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    case 0x0022:  /* RegUnLoadKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        FIXME("RegUnLoadKey(%p,%s): stub\n",hkey, debugstr_a(lpszSubKey));
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    case 0x0023:  /* RegSaveKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszFile   = (LPCSTR)stack32_pop( context );
        LPSECURITY_ATTRIBUTES sa = (LPSECURITY_ATTRIBUTES)stack32_pop( context );
        FIXME("RegSaveKey(%p,%s,%p): stub\n",hkey, debugstr_a(lpszFile),sa);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

#if 0 /* Functions are not yet implemented in misc/registry.c */
    case 0x0024:  /* RegRemapPreDefKey */
    case 0x0026:  /* RegQueryMultipleValues */
#endif

    case 0x0027:  /* RegReplaceKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        LPCSTR  lpszNewFile= (LPCSTR)stack32_pop( context );
        LPCSTR  lpszOldFile= (LPCSTR)stack32_pop( context );
        FIXME("RegReplaceKey(%p,%s,%s,%s): stub\n", hkey, debugstr_a(lpszSubKey),
              debugstr_a(lpszNewFile),debugstr_a(lpszOldFile));
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    default:
        if (LOWORD(service) < N_VMM_SERVICE)
            FIXME( "Unimplemented service %s (%08lx)\n",
                   VMM_Service_Name[LOWORD(service)], service);
        else
            FIXME( "Unknown service %08lx\n", service);
        return 0xffffffff;  /* FIXME */
    }
}
