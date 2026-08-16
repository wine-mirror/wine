/*
 * Win32 processes
 *
 * Copyright 1996, 1998 Alexandre Julliard
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
#include <string.h>

#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wincontypes.h"
#include "winreg.h"
#include "winternl.h"
#include "processsnapshot.h"

#include "kernelbase.h"
#include "wine/debug.h"
#include "wine/condrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(process);

static DWORD shutdown_flags = 0;
static DWORD shutdown_priority = 0x280;

/***********************************************************************
 * Processes
 ***********************************************************************/


/***********************************************************************
 *           find_exe_file
 */
static BOOL find_exe_file( const WCHAR *name, WCHAR *buffer, DWORD buflen )
{
    WCHAR *load_path;
    BOOL ret;

    if (!set_ntstatus( RtlGetExePath( name, &load_path ))) return FALSE;

    TRACE( "looking for %s in %s\n", debugstr_w(name), debugstr_w(load_path) );

    ret = (SearchPathW( load_path, name, L".exe", buflen, buffer, NULL ) ||
           /* not found, try without extension in case it is a Unix app */
           SearchPathW( load_path, name, NULL, buflen, buffer, NULL ));

    if (ret)  /* make sure it can be opened, SearchPathW also returns directories */
    {
        HANDLE handle = CreateFileW( buffer, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_DELETE,
                                     NULL, OPEN_EXISTING, 0, 0 );
        if ((ret = (handle != INVALID_HANDLE_VALUE))) CloseHandle( handle );
        else SetLastError( ERROR_FILE_NOT_FOUND );
    }
    RtlReleasePath( load_path );
    return ret;
}


/*************************************************************************
 *               get_file_name
 *
 * Helper for CreateProcess: retrieve the file name to load from the
 * app name and command line. Store the file name in buffer, and
 * return a possibly modified command line.
 */
static WCHAR *get_file_name( WCHAR *cmdline, WCHAR *buffer, DWORD buflen )
{
    WCHAR *name, *pos, *first_space, *ret = NULL;
    const WCHAR *p;

    /* first check for a quoted file name */

    if (cmdline[0] == '"' && (p = wcschr( cmdline + 1, '"' )))
    {
        int len = p - cmdline - 1;
        /* trim spaces in quotes */
        while (len && cmdline[len] == L' ') len--;
        /* extract the quoted portion as file name */
        if (!(name = RtlAllocateHeap( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) ))) return NULL;
        memcpy( name, cmdline + 1, len * sizeof(WCHAR) );
        name[len] = 0;

        if (!find_exe_file( name, buffer, buflen )) goto done;
        ret = cmdline;  /* no change necessary */
        goto done;
    }

    /* now try the command-line word by word */

    if (!(name = RtlAllocateHeap( GetProcessHeap(), 0, (lstrlenW(cmdline) + 1) * sizeof(WCHAR) )))
        return NULL;
    pos = name;
    p = cmdline;
    first_space = NULL;

    for (;;)
    {
        while (*p && *p != ' ' && *p != '\t') *pos++ = *p++;
        *pos = 0;
        if (find_exe_file( name, buffer, buflen ))
        {
            ret = cmdline;
            break;
        }
        if (GetLastError() != ERROR_FILE_NOT_FOUND) break;
        if (!first_space) first_space = pos;
        if (!(*pos++ = *p++)) break;
    }

    if (ret && first_space)  /* build a new command-line with quotes */
    {
        if (!(ret = HeapAlloc( GetProcessHeap(), 0, (lstrlenW(cmdline) + 3) * sizeof(WCHAR) )))
            goto done;
        swprintf( ret, lstrlenW(cmdline) + 3, L"\"%s\"%s", name, p );
    }

 done:
    RtlFreeHeap( GetProcessHeap(), 0, name );
    return ret;
}


/***********************************************************************
 *           create_process_params
 */
static RTL_USER_PROCESS_PARAMETERS *create_process_params( const WCHAR *filename, const WCHAR *cmdline,
                                                           const WCHAR *cur_dir, void *env, DWORD flags,
                                                           const STARTUPINFOW *startup )
{
    RTL_USER_PROCESS_PARAMETERS *params;
    UNICODE_STRING imageW, curdirW, cmdlineW, titleW, desktopW, runtimeW, newdirW;
    WCHAR imagepath[MAX_PATH];
    WCHAR *envW = env;

    if (!GetLongPathNameW( filename, imagepath, MAX_PATH )) lstrcpynW( imagepath, filename, MAX_PATH );
    if (!GetFullPathNameW( imagepath, MAX_PATH, imagepath, NULL )) lstrcpynW( imagepath, filename, MAX_PATH );

    if (env && !(flags & CREATE_UNICODE_ENVIRONMENT))  /* convert environment to unicode */
    {
        char *e = env;
        DWORD lenW;

        while (*e) e += strlen(e) + 1;
        e++;  /* final null */
        lenW = MultiByteToWideChar( CP_ACP, 0, env, e - (char *)env, NULL, 0 );
        if ((envW = RtlAllocateHeap( GetProcessHeap(), 0, lenW * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, env, e - (char *)env, envW, lenW );
    }

    newdirW.Buffer = NULL;
    if (cur_dir)
    {
        if (RtlDosPathNameToNtPathName_U( cur_dir, &newdirW, NULL, NULL ))
            cur_dir = newdirW.Buffer + 4;  /* skip \??\ prefix */
        else
            cur_dir = NULL;
    }
    RtlInitUnicodeString( &imageW, imagepath );
    RtlInitUnicodeString( &curdirW, cur_dir );
    RtlInitUnicodeString( &cmdlineW, cmdline );
    RtlInitUnicodeString( &titleW, startup->lpTitle ? startup->lpTitle : imagepath );
    RtlInitUnicodeString( &desktopW, startup->lpDesktop );
    runtimeW.Buffer = (WCHAR *)startup->lpReserved2;
    runtimeW.Length = runtimeW.MaximumLength = startup->cbReserved2;
    if (RtlCreateProcessParametersEx( &params, &imageW, NULL, cur_dir ? &curdirW : NULL,
                                      &cmdlineW, envW, &titleW, &desktopW,
                                      NULL, &runtimeW, PROCESS_PARAMS_FLAG_NORMALIZED ))
    {
        RtlFreeUnicodeString( &newdirW );
        if (envW != env) RtlFreeHeap( GetProcessHeap(), 0, envW );
        return NULL;
    }
    RtlFreeUnicodeString( &newdirW );

    if (!(flags & CREATE_NEW_PROCESS_GROUP))
        params->ProcessGroupId = NtCurrentTeb()->Peb->ProcessParameters->ProcessGroupId;
    else if (!(flags & CREATE_NEW_CONSOLE))
        params->ConsoleFlags = 1;

    if (flags & CREATE_NEW_CONSOLE) params->ConsoleHandle = CONSOLE_HANDLE_ALLOC;
    else if (!(flags & DETACHED_PROCESS))
    {
        if (flags & CREATE_NO_WINDOW) params->ConsoleHandle = CONSOLE_HANDLE_ALLOC_NO_WINDOW;
        else
        {
            params->ConsoleHandle = NtCurrentTeb()->Peb->ProcessParameters->ConsoleHandle;
            if (!params->ConsoleHandle) params->ConsoleHandle = CONSOLE_HANDLE_ALLOC;
        }
    }

    if (startup->dwFlags & STARTF_USESTDHANDLES)
    {
        params->hStdInput  = startup->hStdInput;
        params->hStdOutput = startup->hStdOutput;
        params->hStdError  = startup->hStdError;
    }
    else if (!(flags & (DETACHED_PROCESS | CREATE_NEW_CONSOLE)))
    {
        params->hStdInput  = NtCurrentTeb()->Peb->ProcessParameters->hStdInput;
        params->hStdOutput = NtCurrentTeb()->Peb->ProcessParameters->hStdOutput;
        params->hStdError  = NtCurrentTeb()->Peb->ProcessParameters->hStdError;
    }

    if (params->hStdInput  == INVALID_HANDLE_VALUE) params->hStdInput  = NULL;
    if (params->hStdOutput == INVALID_HANDLE_VALUE) params->hStdOutput = NULL;
    if (params->hStdError  == INVALID_HANDLE_VALUE) params->hStdError  = NULL;

    params->dwX             = startup->dwX;
    params->dwY             = startup->dwY;
    params->dwXSize         = startup->dwXSize;
    params->dwYSize         = startup->dwYSize;
    params->dwXCountChars   = startup->dwXCountChars;
    params->dwYCountChars   = startup->dwYCountChars;
    params->dwFillAttribute = startup->dwFillAttribute;
    params->dwFlags         = startup->dwFlags;
    params->wShowWindow     = startup->wShowWindow;

    if (envW != env) RtlFreeHeap( GetProcessHeap(), 0, envW );
    return params;
}

struct proc_thread_attr
{
    DWORD_PTR attr;
    SIZE_T size;
    void *value;
};

struct _PROC_THREAD_ATTRIBUTE_LIST
{
    DWORD mask;  /* bitmask of items in list */
    DWORD size;  /* max number of items in list */
    DWORD count; /* number of items in list */
    DWORD pad;
    DWORD_PTR unk;
    struct proc_thread_attr attrs[1];
};

/***********************************************************************
 *           create_nt_process
 */
static NTSTATUS create_nt_process( HANDLE token, HANDLE debug, SECURITY_ATTRIBUTES *psa,
                                   SECURITY_ATTRIBUTES *tsa, DWORD process_flags,
                                   RTL_USER_PROCESS_PARAMETERS *params,
                                   RTL_USER_PROCESS_INFORMATION *info,
                                   HANDLE parent, USHORT machine,
                                   const struct proc_thread_attr *handle_list,
                                   const struct proc_thread_attr *job_list)
{
    OBJECT_ATTRIBUTES process_attr, thread_attr;
    PS_CREATE_INFO create_info;
    ULONG_PTR buffer[offsetof( PS_ATTRIBUTE_LIST, Attributes[9] ) / sizeof(ULONG_PTR)];
    PS_ATTRIBUTE_LIST *attr = (PS_ATTRIBUTE_LIST *)buffer;
    UNICODE_STRING nameW;
    NTSTATUS status;
    UINT pos = 0;

    if (!params->ImagePathName.Buffer[0]) return STATUS_OBJECT_PATH_NOT_FOUND;
    status = RtlDosPathNameToNtPathName_U_WithStatus( params->ImagePathName.Buffer, &nameW, NULL, NULL );
    if (!status)
    {
        RtlNormalizeProcessParams( params );

        attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_IMAGE_NAME;
        attr->Attributes[pos].Size         = nameW.Length;
        attr->Attributes[pos].ValuePtr     = nameW.Buffer;
        attr->Attributes[pos].ReturnLength = NULL;
        pos++;
        attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_CLIENT_ID;
        attr->Attributes[pos].Size         = sizeof(info->ClientId);
        attr->Attributes[pos].ValuePtr     = &info->ClientId;
        attr->Attributes[pos].ReturnLength = NULL;
        pos++;
        attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_IMAGE_INFO;
        attr->Attributes[pos].Size         = sizeof(info->ImageInformation);
        attr->Attributes[pos].ValuePtr     = &info->ImageInformation;
        attr->Attributes[pos].ReturnLength = NULL;
        pos++;
        if (parent)
        {
            attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_PARENT_PROCESS;
            attr->Attributes[pos].Size         = sizeof(parent);
            attr->Attributes[pos].ValuePtr     = parent;
            attr->Attributes[pos].ReturnLength = NULL;
            pos++;
        }
        if ((process_flags & PROCESS_CREATE_FLAGS_INHERIT_HANDLES) && handle_list)
        {
            attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_HANDLE_LIST;
            attr->Attributes[pos].Size         = handle_list->size;
            attr->Attributes[pos].ValuePtr     = handle_list->value;
            attr->Attributes[pos].ReturnLength = NULL;
            pos++;
        }
        if (token)
        {
            attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_TOKEN;
            attr->Attributes[pos].Size         = sizeof(token);
            attr->Attributes[pos].ValuePtr     = token;
            attr->Attributes[pos].ReturnLength = NULL;
            pos++;
        }
        if (debug)
        {
            attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_DEBUG_PORT;
            attr->Attributes[pos].Size         = sizeof(debug);
            attr->Attributes[pos].ValuePtr     = debug;
            attr->Attributes[pos].ReturnLength = NULL;
            pos++;
        }
        if (job_list)
        {
            attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_JOB_LIST;
            attr->Attributes[pos].Size         = job_list->size;
            attr->Attributes[pos].ValuePtr     = job_list->value;
            attr->Attributes[pos].ReturnLength = NULL;
            pos++;
        }
        if (machine)
        {
            attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_MACHINE_TYPE;
            attr->Attributes[pos].Size         = sizeof(machine);
            attr->Attributes[pos].Value        = machine;
            attr->Attributes[pos].ReturnLength = NULL;
            pos++;
        }
        attr->TotalLength = offsetof( PS_ATTRIBUTE_LIST, Attributes[pos] );

        InitializeObjectAttributes( &process_attr, NULL, 0, NULL, psa ? psa->lpSecurityDescriptor : NULL );
        InitializeObjectAttributes( &thread_attr, NULL, 0, NULL, tsa ? tsa->lpSecurityDescriptor : NULL );

        status = NtCreateUserProcess( &info->Process, &info->Thread, PROCESS_ALL_ACCESS, THREAD_ALL_ACCESS,
                                      &process_attr, &thread_attr, process_flags,
                                      THREAD_CREATE_FLAGS_CREATE_SUSPENDED, params,
                                      &create_info, attr );

        RtlFreeUnicodeString( &nameW );
    }
    return status;
}


/* Windows builds of DOSBox forks, tried (in this order) as a legacy fallback
 * when winevdm.exe isn't installed in the prefix. Every listed fork accepts
 * a target program path as a plain argument and auto-mounts/runs it. */
static const WCHAR *const dosbox_candidates[] =
{
    L"C:\\Program Files\\DOSBox-staging\\dosbox.exe",
    L"C:\\Program Files (x86)\\DOSBox-staging\\dosbox.exe",
    L"C:\\Program Files\\DOSBox-X\\dosbox-x.exe",
    L"C:\\Program Files (x86)\\DOSBox-X\\dosbox-x.exe",
    L"C:\\Program Files\\DOSBox\\dosbox.exe",
    L"C:\\Program Files (x86)\\DOSBox\\dosbox.exe",
};

static const WCHAR *find_legacy_dosbox(void)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(dosbox_candidates); i++)
    {
        DWORD attr = GetFileAttributesW( dosbox_candidates[i] );
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
            return dosbox_candidates[i];
    }
    return NULL;
}

#define IS_OPTION_TRUE(ch) ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')

/* HKCU\Software\Wine\DOSBox "Legacy"=y forces every DOS program through
 * DOSBox, skipping winevdm.exe entirely (an explicit legacy mode, as
 * opposed to the automatic fallback used when winevdm isn't installed or
 * fails to start). */
static BOOL dosbox_legacy_mode_enabled(void)
{
    WCHAR buffer[16];
    DWORD size = sizeof(buffer);
    BOOL enabled = FALSE;
    HKEY key;

    /* @@ Wine registry key: HKCU\Software\Wine\DOSBox */
    if (!RegOpenKeyExW( HKEY_CURRENT_USER, L"Software\\Wine\\DOSBox", 0, KEY_READ, &key ))
    {
        if (!RegQueryValueExW( key, L"Legacy", NULL, NULL, (BYTE *)buffer, &size ))
            enabled = IS_OPTION_TRUE( buffer[0] );
        RegCloseKey( key );
    }
    return enabled;
}

/* Launch either winevdm.exe or a legacy DOSBox fork against the DOS/Win16
 * program named by orig_path/orig_cmdline (params' own ImagePathName and
 * CommandLine get overwritten with the launcher's, so the originals must be
 * passed in separately for a possible second attempt to reuse). */
static NTSTATUS launch_vdm( const WCHAR *vdm, BOOL use_dosbox, const WCHAR *orig_path, const WCHAR *orig_cmdline,
                            HANDLE token, HANDLE debug, SECURITY_ATTRIBUTES *psa, SECURITY_ATTRIBUTES *tsa,
                            DWORD flags, RTL_USER_PROCESS_PARAMETERS *params, RTL_USER_PROCESS_INFORMATION *info )
{
    WCHAR *newcmdline;
    NTSTATUS status;
    UINT len;

    len = lstrlenW(orig_path) + lstrlenW(orig_cmdline) + lstrlenW(vdm) + 16;
    if (!(newcmdline = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
        return STATUS_NO_MEMORY;

    if (use_dosbox)
        swprintf( newcmdline, len, L"%s \"%s\" -exit", vdm, orig_path );
    else
        swprintf( newcmdline, len, L"%s --app-name \"%s\" %s", vdm, orig_path, orig_cmdline );

    RtlInitUnicodeString( &params->ImagePathName, vdm );
    RtlInitUnicodeString( &params->CommandLine, newcmdline );
    status = create_nt_process( token, debug, psa, tsa, flags, params, info, 0, 0, NULL, NULL );
    HeapFree( GetProcessHeap(), 0, newcmdline );
    return status;
}

/***********************************************************************
 *           create_vdm_process
 */
static NTSTATUS create_vdm_process( HANDLE token, HANDLE debug, SECURITY_ATTRIBUTES *psa,
                                    SECURITY_ATTRIBUTES *tsa, DWORD flags,
                                    RTL_USER_PROCESS_PARAMETERS *params,
                                    RTL_USER_PROCESS_INFORMATION *info, BOOL is_dos )
{
    const WCHAR *winevdm = (is_win64 || is_wow64 ?
                            L"C:\\windows\\syswow64\\winevdm.exe" :
                            L"C:\\windows\\system32\\winevdm.exe");
    /* DOSBox forks only emulate real-mode DOS, not the Win16/NE GUI subsystem,
     * so it's only ever a candidate for plain DOS executables. */
    const WCHAR *dosbox = is_dos ? find_legacy_dosbox() : NULL;
    const WCHAR *orig_path = params->ImagePathName.Buffer;
    const WCHAR *orig_cmdline = params->CommandLine.Buffer;
    BOOL force_legacy = is_dos && dosbox && dosbox_legacy_mode_enabled();
    NTSTATUS status = STATUS_OBJECT_NAME_NOT_FOUND;

    if (!force_legacy && GetFileAttributesW( winevdm ) != INVALID_FILE_ATTRIBUTES)
    {
        status = launch_vdm( winevdm, FALSE, orig_path, orig_cmdline,
                              token, debug, psa, tsa, flags, params, info );
        if (!status) return status;
        WARN( "winevdm.exe failed to start %s (%#lx)%s\n", debugstr_w(orig_path), status,
              dosbox ? ", trying legacy DOSBox fallback" : "" );
    }

    if (dosbox)
    {
        TRACE( "%s %s via legacy DOSBox at %s\n", force_legacy ? "forcing" : "starting",
               debugstr_w(orig_path), debugstr_w(dosbox) );
        status = launch_vdm( dosbox, TRUE, orig_path, orig_cmdline,
                              token, debug, psa, tsa, flags, params, info );
    }
    return status;
}


/***********************************************************************
 *           create_xbe_process
 *
 * Launch winexbe.exe with the XBE path as its command-line argument.
 * winexbe.exe handles section mapping, kernel thunk resolution, and
 * thread creation — exactly as winevdm.exe handles Win16/DOS images.
 */
static NTSTATUS create_xbe_process( HANDLE token, HANDLE debug, SECURITY_ATTRIBUTES *psa,
                                    SECURITY_ATTRIBUTES *tsa, DWORD flags,
                                    RTL_USER_PROCESS_PARAMETERS *params,
                                    RTL_USER_PROCESS_INFORMATION *info,
                                    const WCHAR *xbe_path, const WCHAR *orig_cmdline )
{
    const WCHAR *winexbe = (is_win64 || is_wow64 ?
                            L"C:\\windows\\syswow64\\winexbe.exe" :
                            L"C:\\windows\\system32\\winexbe.exe");
    WCHAR *newcmdline;
    NTSTATUS status;
    UINT len;

    len = lstrlenW(winexbe) + lstrlenW(xbe_path) + 4;
    if (!(newcmdline = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
        return STATUS_NO_MEMORY;

    swprintf( newcmdline, len, L"%s \"%s\"", winexbe, xbe_path );
    RtlInitUnicodeString( &params->ImagePathName, winexbe );
    RtlInitUnicodeString( &params->CommandLine, newcmdline );
    status = create_nt_process( token, debug, psa, tsa, flags, params, info, 0, 0, NULL, NULL );
    HeapFree( GetProcessHeap(), 0, newcmdline );
    return status;
}


/***********************************************************************
 *           create_cmd_process
 */
static NTSTATUS create_cmd_process( HANDLE token, HANDLE debug, SECURITY_ATTRIBUTES *psa,
                                    SECURITY_ATTRIBUTES *tsa, DWORD flags,
                                    RTL_USER_PROCESS_PARAMETERS *params,
                                    RTL_USER_PROCESS_INFORMATION *info )
{
    WCHAR comspec[MAX_PATH];
    WCHAR *newcmdline;
    NTSTATUS status;
    UINT len;

    if (!GetEnvironmentVariableW( L"COMSPEC", comspec, ARRAY_SIZE( comspec )))
        lstrcpyW( comspec, L"C:\\windows\\system32\\cmd.exe" );

    len = lstrlenW(comspec) + 7 + lstrlenW(params->CommandLine.Buffer) + 2;
    if (!(newcmdline = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
        return STATUS_NO_MEMORY;

    swprintf( newcmdline, len, L"%s /s/c \"%s\"", comspec, params->CommandLine.Buffer );
    RtlInitUnicodeString( &params->ImagePathName, comspec );
    RtlInitUnicodeString( &params->CommandLine, newcmdline );
    status = create_nt_process( token, debug, psa, tsa, flags, params, info, 0, 0, NULL, NULL );
    RtlFreeHeap( GetProcessHeap(), 0, newcmdline );
    return status;
}


/* ---------------------------------------------------------------------
 * Xbox 360 (.xex) executable recognition.
 *
 * The Xbox 360 (Xenon) CPU is a custom tri-core PowerPC design, an
 * entirely different instruction set from the x86/x86_64 that Wine's
 * process loader runs guest code on. Wine's whole architecture is built
 * around translating Windows *API calls* while letting the guest's own
 * native machine code execute directly on the host CPU - there is no
 * mechanism here for running PowerPC code on an x86 host, and building
 * one (full CPU emulation, or a PPC->x86 static/dynamic recompiler of
 * the kind Xenia and the standalone XenonRecomp project implement) is a
 * separate, much larger undertaking than this file attempts.
 *
 * What follows is real parsing of the XEX2 *container format* - the
 * part of a .xex file that is architecture-independent, analogous to
 * reading a PE header without being able to run the x86 code it
 * describes. The struct layouts and the optional-header-directory
 * walking algorithm are taken directly from Xenia
 * (src/xenia/kernel/util/xex2_info.h, struct xex2_header /
 * xex2_opt_header / xex2_security_info, and the key&0xFF dispatch in
 * XexModule::GetOptHeader() in src/xenia/cpu/xex_module.cc). This lets
 * Wine positively identify a .xex file and log its metadata instead of
 * just failing with a generic "not an MZ image" error; it does not, and
 * is not intended to, execute the contained PowerPC code.
 * --------------------------------------------------------------------- */

#define XEX2_MAGIC 0x58455832  /* "XEX2", big-endian in the file */
#define XEX2_PARSE_BUFFER_SIZE 0x10000  /* generous; real XEX headers are a few KB */

/* All multi-byte fields in a XEX2 header are big-endian (PowerPC-native),
 * regardless of host byte order. */
static inline UINT32 xex2_be32( const BYTE *p ) { return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }
static inline UINT16 xex2_be16( const BYTE *p ) { return (p[0] << 8) | p[1]; }

enum xex2_header_keys
{
    XEX_HEADER_RESOURCE_INFO                 = 0x000002FF,
    XEX_HEADER_FILE_FORMAT_INFO              = 0x000003FF,
    XEX_HEADER_DELTA_PATCH_DESCRIPTOR        = 0x000005FF,
    XEX_HEADER_BASE_REFERENCE                = 0x00000405,
    XEX_HEADER_BOUNDING_PATH                 = 0x000080FF,
    XEX_HEADER_DEVICE_ID                     = 0x00008105,
    XEX_HEADER_ORIGINAL_BASE_ADDRESS         = 0x00010001,
    XEX_HEADER_ENTRY_POINT                   = 0x00010100,
    XEX_HEADER_IMAGE_BASE_ADDRESS            = 0x00010201,
    XEX_HEADER_IMPORT_LIBRARIES              = 0x000103FF,
    XEX_HEADER_CHECKSUM_TIMESTAMP            = 0x00018002,
    XEX_HEADER_ORIGINAL_PE_NAME              = 0x000183FF,
    XEX_HEADER_STATIC_LIBRARIES              = 0x000200FF,
    XEX_HEADER_TLS_INFO                      = 0x00020104,
    XEX_HEADER_DEFAULT_STACK_SIZE            = 0x00020200,
    XEX_HEADER_DEFAULT_FILESYSTEM_CACHE_SIZE = 0x00020301,
    XEX_HEADER_DEFAULT_HEAP_SIZE             = 0x00020401,
    XEX_HEADER_SYSTEM_FLAGS                  = 0x00030000,
    XEX_HEADER_EXECUTION_INFO                = 0x00040006,
    XEX_HEADER_TITLE_WORKSPACE_SIZE          = 0x00040201,
    XEX_HEADER_GAME_RATINGS                  = 0x00040310,
    XEX_HEADER_LAN_KEY                       = 0x00040404,
    XEX_HEADER_MULTIDISC_MEDIA_IDS           = 0x000406FF,
    XEX_HEADER_ALTERNATE_TITLE_IDS           = 0x000407FF,
    XEX_HEADER_ADDITIONAL_TITLE_MEMORY       = 0x00040801,
    XEX_HEADER_EXPORTS_BY_NAME               = 0x00E10402,
};

enum xex2_module_flags
{
    XEX_MODULE_TITLE            = 0x00000001,
    XEX_MODULE_EXPORTS_TO_TITLE = 0x00000002,
    XEX_MODULE_SYSTEM_DEBUGGER  = 0x00000004,
    XEX_MODULE_DLL_MODULE       = 0x00000008,
    XEX_MODULE_MODULE_PATCH     = 0x00000010,
    XEX_MODULE_PATCH_FULL       = 0x00000020,
    XEX_MODULE_PATCH_DELTA      = 0x00000040,
    XEX_MODULE_USER_MODE        = 0x00000080,
};

/* Walk the string table + import-library array pointed to by a
 * XEX_HEADER_IMPORT_LIBRARIES optional header, matching the layout of
 * Xenia's xex2_opt_import_libraries / xex2_import_library and the walk
 * done in XexModule::LoadContinue(). "base" is the start of the whole
 * XEX2 header (offsets below are relative to it), "off" is the
 * XEX_HEADER_IMPORT_LIBRARIES offset, "buf_size" bounds all reads. */
static void trace_xex2_import_libraries( const BYTE *base, UINT32 off, UINT32 buf_size )
{
    const char *string_table[32];
    UINT32 block_size, st_size, st_count, name_count = 0;
    UINT32 i, pos, library_off;

    if (off + 12 > buf_size) return;
    block_size = xex2_be32( base + off );
    st_size    = xex2_be32( base + off + 4 );
    st_count   = xex2_be32( base + off + 8 );
    TRACE( "  import libraries: block size %#x, string table %u bytes / %u entries\n",
           block_size, st_size, st_count );

    if (off + 0xC + st_size > buf_size) return;

    memset( string_table, 0, sizeof(string_table) );
    for (i = 0, pos = 0; pos < st_size && i < st_count && i < ARRAY_SIZE(string_table); i++)
    {
        const char *str = (const char *)(base + off + 0xC + pos);
        UINT32 len = 0;
        while (pos + len < st_size && str[len]) len++;
        string_table[i] = str;
        name_count++;
        pos += len + 1;
        if (pos % 4) pos += 4 - (pos % 4);
    }

    library_off = 0xC + st_size;
    while (off + library_off + 0x28 <= buf_size && library_off < block_size)
    {
        const BYTE *lib = base + off + library_off;
        UINT32 lib_size = xex2_be32( lib );
        UINT32 id = xex2_be32( lib + 0x18 );
        UINT32 version = xex2_be32( lib + 0x1C );
        UINT32 version_min = xex2_be32( lib + 0x20 );
        UINT16 name_index = xex2_be16( lib + 0x24 ) & 0xFF;
        UINT16 count = xex2_be16( lib + 0x26 );

        if (!lib_size) break;
        TRACE( "    import library: %s id %#x version %u.%u.%u.%u min %u.%u.%u.%u, %u imports\n",
               name_index < name_count ? debugstr_a(string_table[name_index]) : "?",
               id,
               (version >> 28) & 0xF, (version >> 24) & 0xF, (version >> 8) & 0xFFFF, version & 0xFF,
               (version_min >> 28) & 0xF, (version_min >> 24) & 0xF, (version_min >> 8) & 0xFFFF, version_min & 0xFF,
               count );
        library_off += lib_size;
    }
}

/* Walk the optional header directory of an already-validated, in-memory
 * XEX2 header (header_count entries of 8 bytes starting at offset 0x18),
 * logging every entry recognised from xex2_header_keys. Matches the
 * key&0xFF dispatch in Xenia's XexModule::GetOptHeader(): a low byte of
 * 0x00 or 0x01 means the second dword *is* the value; anything else means
 * it is a byte offset (from the start of the header) to the real data. */
static void parse_xex2_optional_headers( const BYTE *data, UINT32 buf_size, UINT32 header_count )
{
    UINT32 i;

    for (i = 0; i < header_count; i++)
    {
        UINT32 entry_off = 0x18 + i * 8;
        UINT32 key, raw;

        if (entry_off + 8 > buf_size) { WARN( "  optional header directory truncated\n" ); break; }

        key = xex2_be32( data + entry_off );
        raw = xex2_be32( data + entry_off + 4 );

        if ((key & 0xFF) <= 0x01)
        {
            /* inline value */
            switch (key)
            {
            case XEX_HEADER_ENTRY_POINT:
                TRACE( "  entry point RVA: %#x\n", raw ); break;
            case XEX_HEADER_ORIGINAL_BASE_ADDRESS:
                TRACE( "  original base address: %#x\n", raw ); break;
            case XEX_HEADER_IMAGE_BASE_ADDRESS:
                TRACE( "  image base address: %#x\n", raw ); break;
            case XEX_HEADER_DEFAULT_STACK_SIZE:
                TRACE( "  default stack size: %#x\n", raw ); break;
            case XEX_HEADER_DEFAULT_FILESYSTEM_CACHE_SIZE:
                TRACE( "  default filesystem cache size: %#x\n", raw ); break;
            case XEX_HEADER_DEFAULT_HEAP_SIZE:
                TRACE( "  default heap size: %#x\n", raw ); break;
            case XEX_HEADER_SYSTEM_FLAGS:
                TRACE( "  system flags: %#x\n", raw ); break;
            case XEX_HEADER_TITLE_WORKSPACE_SIZE:
                TRACE( "  title workspace size: %#x\n", raw ); break;
            default:
                TRACE( "  optional header %#x = %#x\n", key, raw ); break;
            }
            continue;
        }

        /* pointer/offset value */
        switch (key)
        {
        case XEX_HEADER_CHECKSUM_TIMESTAMP:
            if (raw + 8 <= buf_size)
                TRACE( "  checksum %#x, timestamp %#x\n", xex2_be32(data+raw), xex2_be32(data+raw+4) );
            break;
        case XEX_HEADER_EXECUTION_INFO:
            if (raw + 0x14 <= buf_size)
                TRACE( "  execution info: media id %#x, version %#x, base version %#x, title id %#x\n",
                       xex2_be32(data+raw), xex2_be32(data+raw+4), xex2_be32(data+raw+8), xex2_be32(data+raw+0xC) );
            break;
        case XEX_HEADER_TLS_INFO:
            if (raw + 0x10 <= buf_size)
                TRACE( "  TLS info: %u slot(s), raw data address %#x, data size %#x, raw data size %#x\n",
                       xex2_be32(data+raw), xex2_be32(data+raw+4), xex2_be32(data+raw+8), xex2_be32(data+raw+0xC) );
            break;
        case XEX_HEADER_ORIGINAL_PE_NAME:
            if (raw + 4 <= buf_size)
            {
                UINT32 name_size = xex2_be32( data + raw );
                UINT32 avail = buf_size - raw - 4;
                TRACE( "  original PE name: %s\n", debugstr_an( (const char *)(data + raw + 4), min(name_size, avail) ) );
            }
            break;
        case XEX_HEADER_RESOURCE_INFO:
            if (raw + 4 <= buf_size)
            {
                UINT32 size = xex2_be32( data + raw );
                UINT32 count = size >= 4 ? (size - 4) / 0x10 : 0;
                UINT32 j;
                TRACE( "  resource info: %u entries\n", count );
                for (j = 0; j < count; j++)
                {
                    UINT32 res_off = raw + 4 + j * 0x10;
                    if (res_off + 0x10 > buf_size) break;
                    TRACE( "    resource[%u]: %s address %#x size %#x\n", j,
                           debugstr_an( (const char *)(data+res_off), 8 ),
                           xex2_be32(data+res_off+8), xex2_be32(data+res_off+0xC) );
                }
            }
            break;
        case XEX_HEADER_IMPORT_LIBRARIES:
            trace_xex2_import_libraries( data, raw, buf_size );
            break;
        default:
            TRACE( "  optional header %#x -> offset %#x\n", key, raw );
            break;
        }
    }
}

/* Parse and log the security-info block referenced by xex2_header.security_offset
 * (struct xex2_security_info in Xenia's xex2_info.h). Only the fixed-size prefix
 * (up to page_descriptor_count) is read; the page descriptor table that follows
 * describes memory layout for the (unimplemented) loader and isn't needed here. */
static void parse_xex2_security_info( const BYTE *data, UINT32 buf_size, UINT32 security_offset )
{
    if (security_offset + 0x184 > buf_size)
    {
        WARN( "  security info at %#x out of range (buffer %#x bytes)\n", security_offset, buf_size );
        return;
    }
    TRACE( "  security info: header size %#x, image size %#x, image flags %#x, load address %#x\n",
           xex2_be32(data+security_offset), xex2_be32(data+security_offset+4),
           xex2_be32(data+security_offset+0x10C), xex2_be32(data+security_offset+0x110) );
    TRACE( "  region %#x, allowed media types %#x, page descriptor count %u\n",
           xex2_be32(data+security_offset+0x178), xex2_be32(data+security_offset+0x17C),
           xex2_be32(data+security_offset+0x180) );
}

/* Read and validate the XEX2 header of "path", logging everything recognised
 * via TRACE. Returns TRUE if this is a well-formed enough XEX2 file to be
 * confidently identified as an Xbox 360 executable (regardless of whether we
 * can do anything with it), FALSE otherwise (I/O error, bad magic, ...). */
static BOOL parse_xex2_file( const WCHAR *path )
{
    BYTE *data;
    HANDLE file;
    DWORD read = 0;
    UINT32 magic, module_flags, header_size, security_offset, header_count;
    BOOL ret = FALSE;

    file = CreateFileW( path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    if (file == INVALID_HANDLE_VALUE)
    {
        WARN( "can't open %s\n", debugstr_w(path) );
        return FALSE;
    }

    if (!(data = RtlAllocateHeap( GetProcessHeap(), 0, XEX2_PARSE_BUFFER_SIZE )))
    {
        CloseHandle( file );
        return FALSE;
    }

    if (!ReadFile( file, data, XEX2_PARSE_BUFFER_SIZE, &read, NULL ) || read < 0x18)
    {
        WARN( "failed to read XEX2 header from %s\n", debugstr_w(path) );
        goto done;
    }

    magic = xex2_be32( data );
    if (magic != XEX2_MAGIC)
    {
        WARN( "%s has .xex extension but bad magic %#x (expected XEX2)\n", debugstr_w(path), magic );
        goto done;
    }

    module_flags     = xex2_be32( data + 4 );
    header_size      = xex2_be32( data + 8 );
    security_offset  = xex2_be32( data + 0x10 );
    header_count     = xex2_be32( data + 0x14 );

    TRACE( "%s: XEX2 header - module flags %#x, header size %#x, security info offset %#x, %u optional header(s)\n",
           debugstr_w(path), module_flags, header_size, security_offset, header_count );
    if (module_flags & XEX_MODULE_TITLE) TRACE( "  module flag: TITLE\n" );
    if (module_flags & XEX_MODULE_DLL_MODULE) TRACE( "  module flag: DLL_MODULE\n" );
    if (module_flags & XEX_MODULE_MODULE_PATCH) TRACE( "  module flag: MODULE_PATCH\n" );
    if (module_flags & XEX_MODULE_SYSTEM_DEBUGGER) TRACE( "  module flag: SYSTEM_DEBUGGER\n" );

    if (header_count > (read - 0x18) / 8) WARN( "  optional header count %u exceeds what was read\n", header_count );
    else parse_xex2_optional_headers( data, read, header_count );

    parse_xex2_security_info( data, read, security_offset );

    ret = TRUE;

done:
    RtlFreeHeap( GetProcessHeap(), 0, data );
    CloseHandle( file );
    return ret;
}


/* ---------------------------------------------------------------------
 * Original Xbox (.xbe) executable recognition.
 *
 * Unlike the Xbox 360 above, the original Xbox's CPU is a Coppermine-
 * based Intel Celeron/Pentium III: plain x86, the same instruction set
 * family Wine's host already runs. There is no CPU-architecture barrier
 * here. What *is* still missing, and is not attempted by this function,
 * is (a) an XBE-aware loader path that maps a file's sections at their
 * absolute virtual addresses and starts a thread at its (XOR-encoded)
 * entry point instead of going through the ordinary NT/PE loader in
 * create_nt_process(), and (b) a working implementation of the ~378
 * ordinal-only xboxkrnl.exe kernel exports that mapped code calls
 * through its "kernel thunk table" - a flat array of function/variable
 * addresses indexed by ordinal, filled in at load time, rather than an
 * ordinary PE import directory. Part (b) is implemented separately as
 * dlls/xboxkrnl (see there for exactly which of the 378 ordinals are
 * real forwards to Wine/NT equivalents vs. FIXME stubs); part (a) - the
 * loader path itself - is not implemented anywhere yet. Both are large
 * undertakings on their own; what follows is real XBE container parsing
 * (header, certificate, section table, library versions, kernel thunk
 * table address), exactly analogous in scope to parse_xex2_file() above.
 *
 * Struct layouts, the entry-point/kernel-thunk-table XOR keys, and the
 * debug/retail/chihiro image type detection algorithm are taken
 * directly from Cxbx-Reloaded: src/common/xbe/Xbe.h (struct Xbe::Header,
 * Xbe::Certificate, Xbe::SectionHeader, Xbe::LibraryVersion and the
 * XOR_EP_ / XOR_KT_ constants) and src/common/xbe/Xbe.cpp (the
 * Xbe::Xbe() constructor's read order/offsets and Xbe::GetXbeType()).
 * --------------------------------------------------------------------- */

#define XBE_MAGIC 0x48454258  /* "XBEH", native little-endian - XBE is a native x86 format */
#define XBE_PARSE_BUFFER_SIZE 0x10000  /* generous; real XBE header+certificate+sections are a few KB */

/* Xbe::GetXbeType() constants (src/common/AddressRanges.h) */
#define XBE_KSEG0_BASE               0x80000000
#define XBE_WRITE_COMBINED_BASE      0xF0000000
#define XBE_SEGABOOT_EP_XOR          0x40000000

/* entry point / kernel thunk table XOR keys (Xbe.h), indexed by xbe_image_type */
enum xbe_image_type { XBE_TYPE_RETAIL, XBE_TYPE_DEBUG, XBE_TYPE_CHIHIRO };
static const UINT32 xbe_xor_ep_key[3] = { 0xA8FC57AB, 0x94859D4B, 0x40B5C16E };
static const UINT32 xbe_xor_kt_key[3] = { 0x5B6D40B6, 0xEFB1F152, 0x2290059D };
static const char *const xbe_image_type_name[3] = { "retail", "debug", "Chihiro (arcade)" };

#include <pshpack1.h>
struct xbe_header
{
    UINT32 magic;                          /* 0x0000 */
    BYTE   digital_signature[256];         /* 0x0004 */
    UINT32 base_addr;                      /* 0x0104 */
    UINT32 sizeof_headers;                 /* 0x0108 */
    UINT32 sizeof_image;                   /* 0x010C */
    UINT32 sizeof_image_header;            /* 0x0110 */
    UINT32 timedate;                       /* 0x0114 */
    UINT32 certificate_addr;               /* 0x0118 */
    UINT32 sections;                       /* 0x011C */
    UINT32 section_headers_addr;           /* 0x0120 */
    UINT32 init_flags;                     /* 0x0124 */
    UINT32 entry_addr;                     /* 0x0128 - XOR-encoded, see xbe_xor_ep_key */
    UINT32 tls_addr;                       /* 0x012C */
    UINT32 pe_stack_commit;                /* 0x0130 */
    UINT32 pe_heap_reserve;                /* 0x0134 */
    UINT32 pe_heap_commit;                 /* 0x0138 */
    UINT32 pe_base_addr;                   /* 0x013C */
    UINT32 pe_sizeof_image;                /* 0x0140 */
    UINT32 pe_checksum;                    /* 0x0144 */
    UINT32 pe_timedate;                    /* 0x0148 */
    UINT32 debug_pathname_addr;            /* 0x014C */
    UINT32 debug_filename_addr;            /* 0x0150 */
    UINT32 debug_unicode_filename_addr;    /* 0x0154 */
    UINT32 kernel_image_thunk_addr;        /* 0x0158 - XOR-encoded, see xbe_xor_kt_key */
    UINT32 non_kernel_import_dir_addr;     /* 0x015C */
    UINT32 library_versions;               /* 0x0160 */
    UINT32 library_versions_addr;          /* 0x0164 */
    UINT32 kernel_library_version_addr;    /* 0x0168 */
    UINT32 xapi_library_version_addr;      /* 0x016C */
    UINT32 logo_bitmap_addr;               /* 0x0170 */
    UINT32 sizeof_logo_bitmap;             /* 0x0174 */
};

struct xbe_certificate
{
    UINT32 size;                           /* 0x0000 */
    UINT32 timedate;                       /* 0x0004 */
    UINT32 title_id;                       /* 0x0008 */
    WCHAR  title_name[40];                 /* 0x000C */
    UINT32 alternate_title_id[16];         /* 0x005C */
    UINT32 allowed_media;                  /* 0x009C */
    UINT32 game_region;                    /* 0x00A0 */
    UINT32 game_ratings;                   /* 0x00A4 */
    UINT32 disc_number;                    /* 0x00A8 */
    UINT32 version;                        /* 0x00AC */
    BYTE   lan_key[16];                    /* 0x00B0 */
    BYTE   signature_key[16];              /* 0x00C0 */
    /* fields below aren't present in every XBE; bounded by certificate.size */
    BYTE   title_alternate_signature_key[16][16]; /* 0x00D0 */
    UINT32 original_certificate_size;      /* 0x01D0 */
    UINT32 online_service;                 /* 0x01D4 */
    UINT32 security_flags;                 /* 0x01D8 */
    BYTE   code_enc_key[16];               /* 0x01DC */
};
#define XBE_CERTIFICATE_MIN_SIZE 0x1D0  /* up to and including signature_key */

struct xbe_section_header
{
    UINT32 flags;
    UINT32 virtual_addr;
    UINT32 virtual_size;
    UINT32 raw_addr;
    UINT32 sizeof_raw;
    UINT32 section_name_addr;
    UINT32 section_ref_count;
    UINT32 head_shared_ref_count_addr;
    UINT32 tail_shared_ref_count_addr;
    BYTE   section_digest[20];
};

struct xbe_library_version
{
    char   name[8];
    UINT16 major_version;
    UINT16 minor_version;
    UINT16 build_version;
    UINT16 flags;
};
#include <poppack.h>

#define XBEIMAGE_SECTION_WRITEABLE          0x00000001
#define XBEIMAGE_SECTION_PRELOAD            0x00000002
#define XBEIMAGE_SECTION_EXECUTABLE         0x00000004
#define XBEIMAGE_SECTION_INSERTFILE         0x00000008
#define XBEIMAGE_SECTION_HEAD_PAGE_READONLY 0x00000010
#define XBEIMAGE_SECTION_TAIL_PAGE_READONLY 0x00000020

/* Translate a virtual address that lies within the XBE's header/certificate/
 * section-header area to an offset in our in-memory read buffer. This only
 * works for addresses inside the initial "headers" region (which is always
 * mapped 1:1 with file offset 0, i.e. buffer offset == vaddr - base_addr) -
 * exactly the assumption Xbe::Xbe()'s fseek()s make for the certificate and
 * section header table. Section *contents* are addressed by raw file offset
 * instead (dwRawAddr), not by this translation - again matching Xbe.cpp. */
static const BYTE *xbe_translate( const BYTE *data, UINT32 buf_size, UINT32 base_addr, UINT32 vaddr, UINT32 need )
{
    UINT32 off;
    if (vaddr < base_addr) return NULL;
    off = vaddr - base_addr;
    if (off > buf_size || need > buf_size - off) return NULL;
    return data + off;
}

/* Read and validate an XBE file's header, certificate and section table,
 * logging everything recognised via TRACE. Returns TRUE if "path" is a
 * well-formed enough XBE to be confidently identified as an original Xbox
 * executable (regardless of whether we can run it), FALSE otherwise. */
static BOOL parse_xbe_file( const WCHAR *path )
{
    BYTE *data;
    HANDLE file;
    DWORD read = 0;
    const struct xbe_header *hdr;
    const struct xbe_certificate *cert;
    enum xbe_image_type type;
    UINT32 entry_point, kernel_thunk_addr;
    UINT32 i;
    BOOL ret = FALSE;

    file = CreateFileW( path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    if (file == INVALID_HANDLE_VALUE)
    {
        WARN( "can't open %s\n", debugstr_w(path) );
        return FALSE;
    }

    if (!(data = RtlAllocateHeap( GetProcessHeap(), 0, XBE_PARSE_BUFFER_SIZE )))
    {
        CloseHandle( file );
        return FALSE;
    }

    if (!ReadFile( file, data, XBE_PARSE_BUFFER_SIZE, &read, NULL ) || read < sizeof(*hdr) )
    {
        WARN( "failed to read XBE header from %s\n", debugstr_w(path) );
        goto done;
    }

    hdr = (const struct xbe_header *)data;
    if (hdr->magic != XBE_MAGIC)
    {
        WARN( "%s has .xbe extension but bad magic %#x (expected \"XBEH\")\n", debugstr_w(path), hdr->magic );
        goto done;
    }

    if (hdr->sections > (XBE_PARSE_BUFFER_SIZE - sizeof(*hdr)) / sizeof(struct xbe_section_header))
    {
        WARN( "%s claims an implausible %u sections, rejecting\n", debugstr_w(path), hdr->sections );
        goto done;
    }

    /* GetXbeType(): entry point / kernel thunk XOR key depends on whether this
     * is a debug, retail, or Chihiro (Sega arcade derivative) build - detected
     * from telltale bit patterns left in the still-XOR-encoded fields. */
    if ((hdr->entry_addr & XBE_WRITE_COMBINED_BASE) == XBE_SEGABOOT_EP_XOR)
        type = XBE_TYPE_CHIHIRO;
    else if (hdr->kernel_image_thunk_addr & XBE_KSEG0_BASE)
        type = XBE_TYPE_DEBUG;
    else
        type = XBE_TYPE_RETAIL;

    entry_point = hdr->entry_addr ^ xbe_xor_ep_key[type];
    kernel_thunk_addr = hdr->kernel_image_thunk_addr ^ xbe_xor_kt_key[type];

    TRACE( "%s: XBE header - %s image, base %#x, %u section(s), timestamp %#x\n",
           debugstr_w(path), xbe_image_type_name[type], hdr->base_addr, hdr->sections, hdr->timedate );
    TRACE( "  size of headers %#x, size of image %#x, size of image header %#x\n",
           hdr->sizeof_headers, hdr->sizeof_image, hdr->sizeof_image_header );
    TRACE( "  entry point (decoded) %#x, kernel thunk table (decoded) %#x\n", entry_point, kernel_thunk_addr );
    TRACE( "  TLS directory %#x, PE stack commit %#x, PE heap reserve/commit %#x/%#x\n",
           hdr->tls_addr, hdr->pe_stack_commit, hdr->pe_heap_reserve, hdr->pe_heap_commit );
    TRACE( "  original PE base %#x, size %#x, checksum %#x, timestamp %#x\n",
           hdr->pe_base_addr, hdr->pe_sizeof_image, hdr->pe_checksum, hdr->pe_timedate );

    cert = (const struct xbe_certificate *)xbe_translate( data, read, hdr->base_addr, hdr->certificate_addr,
                                                            XBE_CERTIFICATE_MIN_SIZE );
    if (cert)
    {
        TRACE( "  certificate: size %#x, title id %#x, title %s\n",
               cert->size, cert->title_id, debugstr_wn( cert->title_name, ARRAY_SIZE(cert->title_name) ) );
        TRACE( "  allowed media %#x, game region %#x, ratings %#x, disc %u, version %u.%u\n",
               cert->allowed_media, cert->game_region, cert->game_ratings, cert->disc_number,
               cert->version & 0xFF, (cert->version & 0xFFFFFF00) >> 8 );
    }
    else WARN( "  certificate at %#x out of range\n", hdr->certificate_addr );

    if (hdr->sections)
    {
        const struct xbe_section_header *sections = (const struct xbe_section_header *)
            xbe_translate( data, read, hdr->base_addr, hdr->section_headers_addr,
                            hdr->sections * sizeof(struct xbe_section_header) );
        if (sections)
        {
            for (i = 0; i < hdr->sections; i++)
            {
                const struct xbe_section_header *sec = &sections[i];
                const char *name = (const char *)xbe_translate( data, read, hdr->base_addr, sec->section_name_addr, 1 );
                TRACE( "  section[%u]: %s flags %#x (%s%s%s), virt %#x/%#x, raw file offset %#x/%#x\n", i,
                       name ? debugstr_an( name, 9 ) : "?", sec->flags,
                       (sec->flags & XBEIMAGE_SECTION_WRITEABLE) ? "W" : "-",
                       (sec->flags & XBEIMAGE_SECTION_EXECUTABLE) ? "X" : "-",
                       (sec->flags & XBEIMAGE_SECTION_PRELOAD) ? "P" : "-",
                       sec->virtual_addr, sec->virtual_size, sec->raw_addr, sec->sizeof_raw );
            }
        }
        else WARN( "  section header table at %#x out of range\n", hdr->section_headers_addr );
    }

    if (hdr->library_versions && hdr->library_versions_addr)
    {
        const struct xbe_library_version *libs = (const struct xbe_library_version *)
            xbe_translate( data, read, hdr->base_addr, hdr->library_versions_addr,
                            hdr->library_versions * sizeof(struct xbe_library_version) );
        if (libs)
        {
            for (i = 0; i < hdr->library_versions; i++)
                TRACE( "  library[%u]: %s version %u.%u.%u\n", i, debugstr_an( libs[i].name, 8 ),
                       libs[i].major_version, libs[i].minor_version, libs[i].build_version );
        }
    }

    ret = TRUE;

done:
    RtlFreeHeap( GetProcessHeap(), 0, data );
    CloseHandle( file );
    return ret;
}


/*********************************************************************
 *           CloseHandle   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CloseHandle( HANDLE handle )
{
    if (handle == (HANDLE)STD_INPUT_HANDLE)
        handle = InterlockedExchangePointer( &NtCurrentTeb()->Peb->ProcessParameters->hStdInput, 0 );
    else if (handle == (HANDLE)STD_OUTPUT_HANDLE)
        handle = InterlockedExchangePointer( &NtCurrentTeb()->Peb->ProcessParameters->hStdOutput, 0 );
    else if (handle == (HANDLE)STD_ERROR_HANDLE)
        handle = InterlockedExchangePointer( &NtCurrentTeb()->Peb->ProcessParameters->hStdError, 0 );

    return set_ntstatus( NtClose( handle ));
}


/**********************************************************************
 *           CreateProcessAsUserA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateProcessAsUserA( HANDLE token, const char *app_name, char *cmd_line,
                                                    SECURITY_ATTRIBUTES *process_attr,
                                                    SECURITY_ATTRIBUTES *thread_attr,
                                                    BOOL inherit, DWORD flags, void *env,
                                                    const char *cur_dir, STARTUPINFOA *startup_info,
                                                    PROCESS_INFORMATION *info )
{
    return CreateProcessInternalA( token, app_name, cmd_line, process_attr, thread_attr,
                                   inherit, flags, env, cur_dir, startup_info, info, NULL );
}


/**********************************************************************
 *           CreateProcessAsUserW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateProcessAsUserW( HANDLE token, const WCHAR *app_name, WCHAR *cmd_line,
                                                    SECURITY_ATTRIBUTES *process_attr,
                                                    SECURITY_ATTRIBUTES *thread_attr,
                                                    BOOL inherit, DWORD flags, void *env,
                                                    const WCHAR *cur_dir, STARTUPINFOW *startup_info,
                                                    PROCESS_INFORMATION *info )
{
    return CreateProcessInternalW( token, app_name, cmd_line, process_attr, thread_attr,
                                   inherit, flags, env, cur_dir, startup_info, info, NULL );
}

/**********************************************************************
 *           CreateProcessInternalA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateProcessInternalA( HANDLE token, const char *app_name, char *cmd_line,
                                                      SECURITY_ATTRIBUTES *process_attr,
                                                      SECURITY_ATTRIBUTES *thread_attr,
                                                      BOOL inherit, DWORD flags, void *env,
                                                      const char *cur_dir, STARTUPINFOA *startup_info,
                                                      PROCESS_INFORMATION *info, HANDLE *new_token )
{
    BOOL ret = FALSE;
    WCHAR *app_nameW = NULL, *cmd_lineW = NULL, *cur_dirW = NULL;
    UNICODE_STRING desktopW, titleW;
    STARTUPINFOEXW infoW;

    desktopW.Buffer = NULL;
    titleW.Buffer = NULL;
    if (app_name && !(app_nameW = file_name_AtoW( app_name, TRUE ))) goto done;
    if (cmd_line && !(cmd_lineW = file_name_AtoW( cmd_line, TRUE ))) goto done;
    if (cur_dir && !(cur_dirW = file_name_AtoW( cur_dir, TRUE ))) goto done;

    if (startup_info->lpDesktop) RtlCreateUnicodeStringFromAsciiz( &desktopW, startup_info->lpDesktop );
    if (startup_info->lpTitle) RtlCreateUnicodeStringFromAsciiz( &titleW, startup_info->lpTitle );

    memcpy( &infoW.StartupInfo, startup_info, sizeof(infoW.StartupInfo) );
    infoW.StartupInfo.lpDesktop = desktopW.Buffer;
    infoW.StartupInfo.lpTitle = titleW.Buffer;

    if (flags & EXTENDED_STARTUPINFO_PRESENT)
        infoW.lpAttributeList = ((STARTUPINFOEXW *)startup_info)->lpAttributeList;

    ret = CreateProcessInternalW( token, app_nameW, cmd_lineW, process_attr, thread_attr,
                                  inherit, flags, env, cur_dirW, (STARTUPINFOW *)&infoW, info, new_token );
done:
    RtlFreeHeap( GetProcessHeap(), 0, app_nameW );
    RtlFreeHeap( GetProcessHeap(), 0, cmd_lineW );
    RtlFreeHeap( GetProcessHeap(), 0, cur_dirW );
    RtlFreeUnicodeString( &desktopW );
    RtlFreeUnicodeString( &titleW );
    return ret;
}

/**********************************************************************
 *           CreateProcessInternalW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateProcessInternalW( HANDLE token, const WCHAR *app_name, WCHAR *cmd_line,
                                                      SECURITY_ATTRIBUTES *process_attr,
                                                      SECURITY_ATTRIBUTES *thread_attr,
                                                      BOOL inherit, DWORD flags, void *env,
                                                      const WCHAR *cur_dir, STARTUPINFOW *startup_info,
                                                      PROCESS_INFORMATION *info, HANDLE *new_token )
{
    const struct proc_thread_attr *handle_list = NULL, *job_list = NULL;
    WCHAR name[MAX_PATH];
    WCHAR *p, *tidy_cmdline = cmd_line;
    RTL_USER_PROCESS_PARAMETERS *params = NULL;
    RTL_USER_PROCESS_INFORMATION rtl_info = { 0 };
    HANDLE parent = 0, debug = 0;
    ULONG nt_flags = 0;
    USHORT machine = 0;
    NTSTATUS status;

    /* Process the AppName and/or CmdLine to get module name and path */

    TRACE( "app %s cmdline %s\n", debugstr_w(app_name), debugstr_w(cmd_line) );

    if (new_token) FIXME( "No support for returning created process token\n" );

    if (app_name)
    {
        if (!cmd_line || !cmd_line[0]) /* no command-line, create one */
        {
            if (!(tidy_cmdline = RtlAllocateHeap( GetProcessHeap(), 0, (lstrlenW(app_name)+3) * sizeof(WCHAR) )))
                return FALSE;
            swprintf( tidy_cmdline, lstrlenW(app_name) + 3, L"\"%s\"", app_name );
        }
    }
    else
    {
        if (!(tidy_cmdline = get_file_name( cmd_line, name, ARRAY_SIZE(name) ))) return FALSE;
        app_name = name;
    }

    /* Warn if unsupported features are used */

    if (flags & (IDLE_PRIORITY_CLASS | HIGH_PRIORITY_CLASS | REALTIME_PRIORITY_CLASS |
                 CREATE_DEFAULT_ERROR_MODE | PROFILE_USER | PROFILE_KERNEL | PROFILE_SERVER))
        WARN( "(%s,...): ignoring some flags in %lx\n", debugstr_w(app_name), flags );

    if (cur_dir)
    {
        DWORD attr = GetFileAttributesW( cur_dir );
        if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
        {
            status = STATUS_NOT_A_DIRECTORY;
            goto done;
        }
    }

    info->hThread = info->hProcess = 0;
    info->dwProcessId = info->dwThreadId = 0;

    if (!(params = create_process_params( app_name, tidy_cmdline, cur_dir, env, flags, startup_info )))
    {
        status = STATUS_NO_MEMORY;
        goto done;
    }

    if (flags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS))
    {
        if ((status = DbgUiConnectToDbg())) goto done;
        debug = DbgUiGetThreadDebugObject();
    }

    if (flags & EXTENDED_STARTUPINFO_PRESENT)
    {
        struct _PROC_THREAD_ATTRIBUTE_LIST *attrs =
                (struct _PROC_THREAD_ATTRIBUTE_LIST *)((STARTUPINFOEXW *)startup_info)->lpAttributeList;
        unsigned int i;

        if (attrs)
        {
            for (i = 0; i < attrs->count; ++i)
            {
                switch(attrs->attrs[i].attr)
                {
                    case PROC_THREAD_ATTRIBUTE_PARENT_PROCESS:
                        parent = *(HANDLE *)attrs->attrs[i].value;
                        TRACE("PROC_THREAD_ATTRIBUTE_PARENT_PROCESS parent %p.\n", parent);
                        if (!parent)
                        {
                            status = STATUS_INVALID_HANDLE;
                            goto done;
                        }
                        break;
                    case PROC_THREAD_ATTRIBUTE_EXTENDED_FLAGS:
                        FIXME("PROC_THREAD_ATTRIBUTE_EXTENDED_FLAGS %lx.\n", *(ULONG *)attrs->attrs[i].value);
                        break;
                    case PROC_THREAD_ATTRIBUTE_HANDLE_LIST:
                        handle_list = &attrs->attrs[i];
                        TRACE("PROC_THREAD_ATTRIBUTE_HANDLE_LIST handle count %Iu.\n", attrs->attrs[i].size / sizeof(HANDLE));
                        break;
                    case PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE:
                        {
                            struct pseudo_console *console = attrs->attrs[i].value;
                            TRACE( "PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE %p reference %p\n",
                                   console, console->reference );
                            params->ConsoleHandle = console->reference;
                            if (!(flags & DETACHED_PROCESS))
                            {
                                params->ConsoleFlags |= 2;
                                /* don't inherit standard handles bound to parent console (but inherit the others) */
                                if (!(startup_info->dwFlags & STARTF_USESTDHANDLES))
                                {
                                    if (is_console_handle(params->hStdInput))  params->hStdInput = NULL;
                                    if (is_console_handle(params->hStdOutput)) params->hStdOutput = NULL;
                                    if (is_console_handle(params->hStdError))  params->hStdError = NULL;
                                }
                            }
                            break;
                        }
                    case PROC_THREAD_ATTRIBUTE_JOB_LIST:
                        job_list = &attrs->attrs[i];
                        TRACE( "PROC_THREAD_ATTRIBUTE_JOB_LIST handle count %Iu.\n",
                               attrs->attrs[i].size / sizeof(HANDLE) );
                        break;
                    case PROC_THREAD_ATTRIBUTE_MACHINE_TYPE:
                        machine = *(USHORT *)attrs->attrs[i].value;
                        TRACE( "PROC_THREAD_ATTRIBUTE_MACHINE %x.\n", machine );
                        break;
                    default:
                        FIXME("Unsupported attribute %#Ix.\n", attrs->attrs[i].attr);
                        break;
                }
            }
        }
    }

    if (inherit) nt_flags |= PROCESS_CREATE_FLAGS_INHERIT_HANDLES;
    if (flags & DEBUG_ONLY_THIS_PROCESS) nt_flags |= PROCESS_CREATE_FLAGS_NO_DEBUG_INHERIT;
    if (flags & CREATE_BREAKAWAY_FROM_JOB) nt_flags |= PROCESS_CREATE_FLAGS_BREAKAWAY;
    if (flags & CREATE_SUSPENDED) nt_flags |= PROCESS_CREATE_FLAGS_SUSPENDED;

    status = create_nt_process( token, debug, process_attr, thread_attr,
                                nt_flags, params, &rtl_info, parent, machine, handle_list, job_list );
    switch (status)
    {
    case STATUS_SUCCESS:
        break;
    case STATUS_INVALID_IMAGE_WIN_16:
    case STATUS_INVALID_IMAGE_NE_FORMAT:
    case STATUS_INVALID_IMAGE_PROTECT:
        TRACE( "starting %s as Win16/DOS binary\n", debugstr_w(app_name) );
        status = create_vdm_process( token, debug, process_attr, thread_attr,
                                     nt_flags, params, &rtl_info, FALSE );
        break;
    case STATUS_INVALID_IMAGE_NOT_MZ:
        /* check for .com or .bat extension */
        if (!(p = wcsrchr( app_name, '.' ))) break;
        if (!wcsicmp( p, L".com" ) || !wcsicmp( p, L".pif" ))
        {
            TRACE( "starting %s as DOS binary\n", debugstr_w(app_name) );
            status = create_vdm_process( token, debug, process_attr, thread_attr,
                                         nt_flags, params, &rtl_info, TRUE );
        }
        else if (!wcsicmp( p, L".bat" ) || !wcsicmp( p, L".cmd" ))
        {
            TRACE( "starting %s as batch binary\n", debugstr_w(app_name) );
            status = create_cmd_process( token, debug, process_attr, thread_attr,
                                         nt_flags, params, &rtl_info );
        }
        else if (!wcsicmp( p, L".xex" ))
        {
            /* Real XEX2 container parsing (see parse_xex2_file() above), not
             * execution: the Xbox 360 is PowerPC and Wine has no PPC->x86
             * recompiler. A recognised XEX2 file fails process creation with
             * STATUS_IMAGE_MACHINE_TYPE_MISMATCH - the file is a legitimate
             * executable image, just for a CPU architecture this host can't
             * run natively, which is exactly what that status describes. */
            if (parse_xex2_file( app_name ))
            {
                WARN( "%s is a recognised Xbox 360 (XEX2) executable; Wine cannot execute "
                      "PowerPC code, this is header/metadata recognition only\n", debugstr_w(app_name) );
                status = STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
            }
            else
            {
                WARN( "%s has a .xex extension but is not a recognisable XEX2 image\n", debugstr_w(app_name) );
                status = STATUS_INVALID_IMAGE_FORMAT;
            }
        }
        else if (!wcsicmp( p, L".xbe" ))
        {
            /* Original Xbox (.xbe) is x86 — dispatch to winexbe.exe, which maps
             * sections at their absolute VAs, resolves the kernel thunk table
             * (loading xboxkrnl.exe by-ordinal), and launches a thread at the
             * decoded entry point.  Unrecognised magic → bad image format. */
            if (parse_xbe_file( app_name ))
            {
                TRACE( "starting %s as XBE via winexbe.exe\n", debugstr_w(app_name) );
                status = create_xbe_process( token, debug, process_attr, thread_attr,
                                             nt_flags, params, &rtl_info,
                                             app_name, tidy_cmdline );
            }
            else
            {
                WARN( "%s has a .xbe extension but is not a recognisable XBE image\n", debugstr_w(app_name) );
                status = STATUS_INVALID_IMAGE_FORMAT;
            }
        }
        break;
    }

    if (!status)
    {
        info->hProcess    = rtl_info.Process;
        info->hThread     = rtl_info.Thread;
        info->dwProcessId = HandleToUlong( rtl_info.ClientId.UniqueProcess );
        info->dwThreadId  = HandleToUlong( rtl_info.ClientId.UniqueThread );
        if (!(flags & CREATE_SUSPENDED)) NtResumeThread( rtl_info.Thread, NULL );
        TRACE( "started process pid %04lx tid %04lx\n", info->dwProcessId, info->dwThreadId );
    }

 done:
    RtlDestroyProcessParameters( params );
    if (tidy_cmdline != cmd_line) HeapFree( GetProcessHeap(), 0, tidy_cmdline );
    return set_ntstatus( status );
}


/**********************************************************************
 *           CreateProcessA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateProcessA( const char *app_name, char *cmd_line,
                                              SECURITY_ATTRIBUTES *process_attr,
                                              SECURITY_ATTRIBUTES *thread_attr, BOOL inherit,
                                              DWORD flags, void *env, const char *cur_dir,
                                              STARTUPINFOA *startup_info, PROCESS_INFORMATION *info )
{
    return CreateProcessInternalA( NULL, app_name, cmd_line, process_attr, thread_attr,
                                   inherit, flags, env, cur_dir, startup_info, info, NULL );
}


/**********************************************************************
 *           CreateProcessW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateProcessW( const WCHAR *app_name, WCHAR *cmd_line,
                                              SECURITY_ATTRIBUTES *process_attr,
                                              SECURITY_ATTRIBUTES *thread_attr, BOOL inherit, DWORD flags,
                                              void *env, const WCHAR *cur_dir, STARTUPINFOW *startup_info,
                                              PROCESS_INFORMATION *info )
{
    return CreateProcessInternalW( NULL, app_name, cmd_line, process_attr, thread_attr,
                                   inherit, flags, env, cur_dir, startup_info, info, NULL );
}


/**********************************************************************
 *           SetProcessInformation   (kernelbase.@)
 */
BOOL WINAPI SetProcessInformation( HANDLE process, PROCESS_INFORMATION_CLASS info_class, void *info, DWORD size )
{
    switch (info_class)
    {
        case ProcessMemoryPriority:
            return set_ntstatus( NtSetInformationProcess( process, ProcessPagePriority, info, size ));
        case ProcessPowerThrottling:
            return set_ntstatus( NtSetInformationProcess( process, ProcessPowerThrottlingState, info, size ));
        case ProcessLeapSecondInfo:
            return set_ntstatus( NtSetInformationProcess( process, ProcessLeapSecondInformation, info, size ));
        default:
            FIXME("Unrecognized information class %d.\n", info_class);
            return FALSE;
    }
}


/*********************************************************************
 *           DuplicateHandle   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DuplicateHandle( HANDLE source_process, HANDLE source,
                                               HANDLE dest_process, HANDLE *dest,
                                               DWORD access, BOOL inherit, DWORD options )
{
    return set_ntstatus( NtDuplicateObject( source_process, source, dest_process, dest,
                                            access, inherit ? OBJ_INHERIT : 0, options ));
}


/***********************************************************************
 *           GetApplicationRestartSettings   (kernelbase.@)
 */
HRESULT WINAPI /* DECLSPEC_HOTPATCH */ GetApplicationRestartSettings( HANDLE process, WCHAR *cmdline,
                                                                      DWORD *size, DWORD *flags )
{
    FIXME( "%p, %p, %p, %p)\n", process, cmdline, size, flags );
    return E_NOTIMPL;
}


/***********************************************************************
 *           GetCurrentProcess   (kernelbase.@)
 */
HANDLE WINAPI kernelbase_GetCurrentProcess(void)
{
    return (HANDLE)~(ULONG_PTR)0;
}


/***********************************************************************
 *           GetCurrentProcessId   (kernelbase.@)
 */
DWORD WINAPI kernelbase_GetCurrentProcessId(void)
{
    return HandleToULong( NtCurrentTeb()->ClientId.UniqueProcess );
}


/***********************************************************************
 *           GetErrorMode   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetErrorMode(void)
{
    UINT mode;

    NtQueryInformationProcess( GetCurrentProcess(), ProcessDefaultHardErrorMode,
                               &mode, sizeof(mode), NULL );
    return mode;
}


/***********************************************************************
 *           GetExitCodeProcess   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetExitCodeProcess( HANDLE process, LPDWORD exit_code )
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION pbi;

    status = NtQueryInformationProcess( process, ProcessBasicInformation, &pbi, sizeof(pbi), NULL );
    if (!status && exit_code) *exit_code = pbi.ExitStatus;
    return set_ntstatus( status );
}


/*********************************************************************
 *           GetHandleInformation   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetHandleInformation( HANDLE handle, DWORD *flags )
{
    OBJECT_HANDLE_FLAG_INFORMATION info;

    if (!set_ntstatus( NtQueryObject( handle, ObjectHandleFlagInformation, &info, sizeof(info), NULL )))
        return FALSE;

    if (flags)
    {
        *flags = 0;
        if (info.Inherit) *flags |= HANDLE_FLAG_INHERIT;
        if (info.ProtectFromClose) *flags |= HANDLE_FLAG_PROTECT_FROM_CLOSE;
    }
    return TRUE;
}


/***********************************************************************
 *           GetMachineTypeAttributes   (kernelbase.@)
 */
HRESULT WINAPI GetMachineTypeAttributes( USHORT machine, MACHINE_ATTRIBUTES *attr )
{
    SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION machines[8];
    HANDLE process = NULL;
    NTSTATUS status;

    status = NtQuerySystemInformationEx( SystemSupportedProcessorArchitectures2, &process, sizeof(process),
                                         machines, sizeof(machines), NULL );
    if (status) return HRESULT_FROM_NT(status);

    *attr = 0;

    for (unsigned int i = 0; machines[i].Machine; i++)
    {
        if (machines[i].Machine == machine)
        {
            if (machines[i].KernelMode)
                *attr |= KernelEnabled;
            if (machines[i].UserMode)
                *attr |= UserEnabled;
            if (machines[i].WoW64Container)
                *attr |= Wow64Container;
        }
    }

    return S_OK;
}


/***********************************************************************
 *           GetPriorityClass   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetPriorityClass( HANDLE process )
{
    PROCESS_PRIORITY_CLASS priority;

    if (!set_ntstatus( NtQueryInformationProcess( process, ProcessPriorityClass,
                                                  &priority, sizeof(priority), NULL )))
        return 0;

    switch (priority.PriorityClass)
    {
    case PROCESS_PRIOCLASS_IDLE: return IDLE_PRIORITY_CLASS;
    case PROCESS_PRIOCLASS_BELOW_NORMAL: return BELOW_NORMAL_PRIORITY_CLASS;
    case PROCESS_PRIOCLASS_NORMAL: return NORMAL_PRIORITY_CLASS;
    case PROCESS_PRIOCLASS_ABOVE_NORMAL: return ABOVE_NORMAL_PRIORITY_CLASS;
    case PROCESS_PRIOCLASS_HIGH: return HIGH_PRIORITY_CLASS;
    case PROCESS_PRIOCLASS_REALTIME: return REALTIME_PRIORITY_CLASS;
    default: return 0;
    }
}


/***********************************************************************
 *           GetProcessGroupAffinity   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessGroupAffinity( HANDLE process, USHORT *count, USHORT *array )
{
    FIXME( "(%p,%p,%p): stub\n", process, count, array );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/******************************************************************
 *           GetProcessHandleCount   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessHandleCount( HANDLE process, DWORD *count )
{
    return set_ntstatus( NtQueryInformationProcess( process, ProcessHandleCount,
                                                    count, sizeof(*count), NULL ));
}


/***********************************************************************
 *           GetProcessHeap   (kernelbase.@)
 */
HANDLE WINAPI kernelbase_GetProcessHeap(void)
{
    return NtCurrentTeb()->Peb->ProcessHeap;
}


/*********************************************************************
 *           GetProcessId   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetProcessId( HANDLE process )
{
    PROCESS_BASIC_INFORMATION pbi;

    if (!set_ntstatus( NtQueryInformationProcess( process, ProcessBasicInformation,
                                                  &pbi, sizeof(pbi), NULL )))
        return 0;
    return pbi.UniqueProcessId;
}


/**********************************************************************
 *           GetProcessMitigationPolicy   (kernelbase.@)
 */
BOOL WINAPI /* DECLSPEC_HOTPATCH */ GetProcessMitigationPolicy( HANDLE process, PROCESS_MITIGATION_POLICY policy,
                                                          void *buffer, SIZE_T length )
{
    FIXME( "(%p, %u, %p, %Iu): stub\n", process, policy, buffer, length );
    return TRUE;
}


/***********************************************************************
 *           GetProcessPriorityBoost   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessPriorityBoost( HANDLE process, PBOOL disable )
{
    return set_ntstatus( NtQueryInformationProcess( process, ProcessPriorityBoost, disable, sizeof(*disable), NULL ));
}


/***********************************************************************
 *           GetProcessShutdownParameters   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessShutdownParameters( LPDWORD level, LPDWORD flags )
{
    *level = shutdown_priority;
    *flags = shutdown_flags;
    return TRUE;
}


/*********************************************************************
 *           GetProcessTimes   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessTimes( HANDLE process, FILETIME *create, FILETIME *exit,
                                               FILETIME *kernel, FILETIME *user )
{
    KERNEL_USER_TIMES time;

    if (!set_ntstatus( NtQueryInformationProcess( process, ProcessTimes, &time, sizeof(time), NULL )))
        return FALSE;

    create->dwLowDateTime  = time.CreateTime.u.LowPart;
    create->dwHighDateTime = time.CreateTime.u.HighPart;
    exit->dwLowDateTime    = time.ExitTime.u.LowPart;
    exit->dwHighDateTime   = time.ExitTime.u.HighPart;
    kernel->dwLowDateTime  = time.KernelTime.u.LowPart;
    kernel->dwHighDateTime = time.KernelTime.u.HighPart;
    user->dwLowDateTime    = time.UserTime.u.LowPart;
    user->dwHighDateTime   = time.UserTime.u.HighPart;
    return TRUE;
}


/***********************************************************************
 *           GetProcessVersion   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetProcessVersion( DWORD pid )
{
    SECTION_IMAGE_INFORMATION info;
    NTSTATUS status;
    HANDLE process;

    if (pid && pid != GetCurrentProcessId())
    {
        if (!(process = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, pid ))) return 0;
        status = NtQueryInformationProcess( process, ProcessImageInformation, &info, sizeof(info), NULL );
        CloseHandle( process );
    }
    else status = NtQueryInformationProcess( GetCurrentProcess(), ProcessImageInformation,
                                             &info, sizeof(info), NULL );

    if (!set_ntstatus( status )) return 0;
    return MAKELONG( info.MinorSubsystemVersion, info.MajorSubsystemVersion );
}


/***********************************************************************
 *           GetProcessWorkingSetSizeEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessWorkingSetSizeEx( HANDLE process, SIZE_T *minset,
                                                          SIZE_T *maxset, DWORD *flags)
{
    FIXME( "(%p,%p,%p,%p): stub\n", process, minset, maxset, flags );
    /* 32 MB working set size */
    if (minset) *minset = 32*1024*1024;
    if (maxset) *maxset = 32*1024*1024;
    if (flags) *flags = QUOTA_LIMITS_HARDWS_MIN_DISABLE | QUOTA_LIMITS_HARDWS_MAX_DISABLE;
    return TRUE;
}


/******************************************************************************
 *           IsProcessInJob   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsProcessInJob( HANDLE process, HANDLE job, BOOL *result )
{
    NTSTATUS status = NtIsProcessInJob( process, job );

    switch (status)
    {
    case STATUS_PROCESS_IN_JOB:
        *result = TRUE;
        return TRUE;
    case STATUS_PROCESS_NOT_IN_JOB:
        *result = FALSE;
        return TRUE;
    default:
        return set_ntstatus( status );
    }
}


/***********************************************************************
 *           IsProcessorFeaturePresent   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsProcessorFeaturePresent ( DWORD feature )
{
    return RtlIsProcessorFeaturePresent( feature );
}


/**********************************************************************
 *           IsWow64Process2   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsWow64Process2( HANDLE process, USHORT *machine, USHORT *native_machine )
{
    return set_ntstatus( RtlWow64GetProcessMachines( process, machine, native_machine ));
}


/**********************************************************************
 *           IsWow64Process   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsWow64Process( HANDLE process, PBOOL wow64 )
{
    ULONG_PTR pbi;
    NTSTATUS status;

    status = NtQueryInformationProcess( process, ProcessWow64Information, &pbi, sizeof(pbi), NULL );
    if (!status) *wow64 = !!pbi;
    return set_ntstatus( status );
}

/*********************************************************************
 *           GetProcessInformation   (kernelbase.@)
 */
BOOL WINAPI GetProcessInformation( HANDLE process, PROCESS_INFORMATION_CLASS info_class, void *data, DWORD size )
{
    switch (info_class)
    {
        case ProcessMachineTypeInfo:
        {
            PROCESS_MACHINE_INFORMATION *mi = data;
            SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION machines[8];
            NTSTATUS status;
            ULONG i;

            if (size != sizeof(*mi))
            {
                SetLastError(ERROR_BAD_LENGTH);
                return FALSE;
            }

            status = NtQuerySystemInformationEx( SystemSupportedProcessorArchitectures2, &process, sizeof(process),
                    machines, sizeof(machines), NULL );
            if (status) return set_ntstatus( status );

            for (i = 0; machines[i].Machine; i++)
            {
                if (machines[i].Process)
                {
                    mi->ProcessMachine = machines[i].Machine;
                    mi->Res0 = 0;
                    mi->MachineAttributes = 0;
                    if (machines[i].KernelMode)
                        mi->MachineAttributes |= KernelEnabled;
                    if (machines[i].UserMode)
                        mi->MachineAttributes |= UserEnabled;
                    if (machines[i].WoW64Container)
                        mi->MachineAttributes |= Wow64Container;

                    return TRUE;
                }
            }

            break;
        }
        default:
            FIXME("Unsupported information class %d.\n", info_class);
    }

    return FALSE;
}


/*********************************************************************
 *           OpenProcess   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenProcess( DWORD access, BOOL inherit, DWORD id )
{
    HANDLE handle;
    OBJECT_ATTRIBUTES attr;
    CLIENT_ID cid;

    if (GetVersion() & 0x80000000) access = PROCESS_ALL_ACCESS;

    InitializeObjectAttributes( &attr, NULL, inherit ? OBJ_INHERIT : 0, 0, NULL );

    cid.UniqueProcess = ULongToHandle(id);
    cid.UniqueThread  = 0;

    if (!set_ntstatus( NtOpenProcess( &handle, access, &attr, &cid ))) return NULL;
    return handle;
}


/***********************************************************************
 *           ProcessIdToSessionId   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ProcessIdToSessionId( DWORD pid, DWORD *id )
{
    HANDLE process;
    NTSTATUS status;

    if (pid == GetCurrentProcessId())
    {
        *id = NtCurrentTeb()->Peb->SessionId;
        return TRUE;
    }
    if (!(process = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid ))) return FALSE;
    status = NtQueryInformationProcess( process, ProcessSessionInformation, id, sizeof(*id), NULL );
    CloseHandle( process );
    return set_ntstatus( status );
}


/***********************************************************************
 *           QueryProcessCycleTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryProcessCycleTime( HANDLE process, ULONG64 *cycle )
{
    PROCESS_CYCLE_TIME_INFORMATION time;

    if (!set_ntstatus( NtQueryInformationProcess( process, ProcessCycleTime, &time, sizeof(time), NULL ) ))
        return FALSE;

    *cycle = time.AccumulatedCycles;
    return TRUE;
}


/***********************************************************************
 *           SetErrorMode   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH SetErrorMode( UINT mode )
{
    UINT old = GetErrorMode();

    NtSetInformationProcess( GetCurrentProcess(), ProcessDefaultHardErrorMode,
                             &mode, sizeof(mode) );
    return old;
}


/*************************************************************************
 *           SetHandleCount   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH SetHandleCount( UINT count )
{
    return count;
}


/*********************************************************************
 *           SetHandleInformation   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetHandleInformation( HANDLE handle, DWORD mask, DWORD flags )
{
    OBJECT_HANDLE_FLAG_INFORMATION info;

    /* if not setting both fields, retrieve current value first */
    if ((mask & (HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE)) !=
        (HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE))
    {
        if (!set_ntstatus( NtQueryObject( handle, ObjectHandleFlagInformation, &info, sizeof(info), NULL )))
            return FALSE;
    }
    if (mask & HANDLE_FLAG_INHERIT)
        info.Inherit = (flags & HANDLE_FLAG_INHERIT) != 0;
    if (mask & HANDLE_FLAG_PROTECT_FROM_CLOSE)
        info.ProtectFromClose = (flags & HANDLE_FLAG_PROTECT_FROM_CLOSE) != 0;

    return set_ntstatus( NtSetInformationObject( handle, ObjectHandleFlagInformation, &info, sizeof(info) ));
}


/***********************************************************************
 *           SetPriorityClass   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetPriorityClass( HANDLE process, DWORD class )
{
    PROCESS_PRIORITY_CLASS ppc;

    ppc.Foreground = FALSE;
    switch (class)
    {
    case IDLE_PRIORITY_CLASS:         ppc.PriorityClass = PROCESS_PRIOCLASS_IDLE; break;
    case BELOW_NORMAL_PRIORITY_CLASS: ppc.PriorityClass = PROCESS_PRIOCLASS_BELOW_NORMAL; break;
    case NORMAL_PRIORITY_CLASS:       ppc.PriorityClass = PROCESS_PRIOCLASS_NORMAL; break;
    case ABOVE_NORMAL_PRIORITY_CLASS: ppc.PriorityClass = PROCESS_PRIOCLASS_ABOVE_NORMAL; break;
    case HIGH_PRIORITY_CLASS:         ppc.PriorityClass = PROCESS_PRIOCLASS_HIGH; break;
    case REALTIME_PRIORITY_CLASS:     ppc.PriorityClass = PROCESS_PRIOCLASS_REALTIME; break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return set_ntstatus( NtSetInformationProcess( process, ProcessPriorityClass, &ppc, sizeof(ppc) ));
}


/***********************************************************************
 *           SetProcessAffinityUpdateMode   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetProcessAffinityUpdateMode( HANDLE process, DWORD flags )
{
    FIXME( "(%p,0x%08lx): stub\n", process, flags );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *           SetProcessGroupAffinity   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetProcessGroupAffinity( HANDLE process, const GROUP_AFFINITY *new,
                                                       GROUP_AFFINITY *old )
{
    FIXME( "(%p,%p,%p): stub\n", process, new, old );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/**********************************************************************
 *           SetProcessMitigationPolicy   (kernelbase.@)
 */
BOOL WINAPI /* DECLSPEC_HOTPATCH */ SetProcessMitigationPolicy( PROCESS_MITIGATION_POLICY policy,
                                                          void *buffer, SIZE_T length )
{
    FIXME( "(%d, %p, %Iu): stub\n", policy, buffer, length );
    return TRUE;
}


/***********************************************************************
 *           SetProcessPriorityBoost   (kernelbase.@)
 */
BOOL WINAPI /* DECLSPEC_HOTPATCH */ SetProcessPriorityBoost( HANDLE process, BOOL disable )
{
    return set_ntstatus( NtSetInformationProcess( process, ProcessPriorityBoost, &disable, sizeof(disable) ));
}


/***********************************************************************
 *           SetProcessShutdownParameters   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetProcessShutdownParameters( DWORD level, DWORD flags )
{
    FIXME( "(%08lx, %08lx): partial stub.\n", level, flags );
    shutdown_flags = flags;
    shutdown_priority = level;
    return TRUE;
}


/***********************************************************************
 *           SetProcessWorkingSetSizeEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetProcessWorkingSetSizeEx( HANDLE process, SIZE_T minset,
                                                          SIZE_T maxset, DWORD flags )
{
    return TRUE;
}


/******************************************************************************
 *           TerminateProcess   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH TerminateProcess( HANDLE handle, UINT exit_code )
{
    if (!handle)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    return set_ntstatus( NtTerminateProcess( handle, exit_code ));
}


/***********************************************************************
 * Process startup information
 ***********************************************************************/


static char *command_lineA;
static WCHAR *command_lineW;

/******************************************************************
 *		init_startup_info
 */
void init_startup_info( RTL_USER_PROCESS_PARAMETERS *params )
{
    ANSI_STRING ansi;

    command_lineW = params->CommandLine.Buffer;
    if (!RtlUnicodeStringToAnsiString( &ansi, &params->CommandLine, TRUE )) command_lineA = ansi.Buffer;
}


/**********************************************************************
 *           BaseFlushAppcompatCache   (kernelbase.@)
 */
BOOL WINAPI BaseFlushAppcompatCache(void)
{
    FIXME( "stub\n" );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *           GetCommandLineA   (kernelbase.@)
 */
LPSTR WINAPI GetCommandLineA(void)
{
    return command_lineA;
}


/***********************************************************************
 *           GetCommandLineW   (kernelbase.@)
 */
LPWSTR WINAPI GetCommandLineW(void)
{
    return command_lineW;
}


/***********************************************************************
 *           GetStartupInfoW    (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH GetStartupInfoW( STARTUPINFOW *info )
{
    RTL_USER_PROCESS_PARAMETERS *params;

    RtlAcquirePebLock();

    params = RtlGetCurrentPeb()->ProcessParameters;

    info->cb              = sizeof(*info);
    info->lpReserved      = NULL;
    info->lpDesktop       = params->Desktop.Buffer;
    info->lpTitle         = params->WindowTitle.Buffer;
    info->dwX             = params->dwX;
    info->dwY             = params->dwY;
    info->dwXSize         = params->dwXSize;
    info->dwYSize         = params->dwYSize;
    info->dwXCountChars   = params->dwXCountChars;
    info->dwYCountChars   = params->dwYCountChars;
    info->dwFillAttribute = params->dwFillAttribute;
    info->dwFlags         = params->dwFlags;
    info->wShowWindow     = params->wShowWindow;
    info->cbReserved2     = params->RuntimeInfo.MaximumLength;
    info->lpReserved2     = params->RuntimeInfo.MaximumLength ? (void *)params->RuntimeInfo.Buffer : NULL;
    if (params->dwFlags & STARTF_USESTDHANDLES)
    {
        info->hStdInput   = params->hStdInput;
        info->hStdOutput  = params->hStdOutput;
        info->hStdError   = params->hStdError;
    }
    RtlReleasePebLock();
}


/***********************************************************************
 *           GetStdHandle    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH GetStdHandle( DWORD std_handle )
{
    switch (std_handle)
    {
    case STD_INPUT_HANDLE:  return NtCurrentTeb()->Peb->ProcessParameters->hStdInput;
    case STD_OUTPUT_HANDLE: return NtCurrentTeb()->Peb->ProcessParameters->hStdOutput;
    case STD_ERROR_HANDLE:  return NtCurrentTeb()->Peb->ProcessParameters->hStdError;
    }
    SetLastError( ERROR_INVALID_HANDLE );
    return INVALID_HANDLE_VALUE;
}


/***********************************************************************
 *           SetStdHandle    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetStdHandle( DWORD std_handle, HANDLE handle )
{
    switch (std_handle)
    {
    case STD_INPUT_HANDLE:  NtCurrentTeb()->Peb->ProcessParameters->hStdInput = handle;  return TRUE;
    case STD_OUTPUT_HANDLE: NtCurrentTeb()->Peb->ProcessParameters->hStdOutput = handle; return TRUE;
    case STD_ERROR_HANDLE:  NtCurrentTeb()->Peb->ProcessParameters->hStdError = handle;  return TRUE;
    }
    SetLastError( ERROR_INVALID_HANDLE );
    return FALSE;
}


/***********************************************************************
 *           SetStdHandleEx    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetStdHandleEx( DWORD std_handle, HANDLE handle, HANDLE *prev )
{
    HANDLE *ptr;

    switch (std_handle)
    {
    case STD_INPUT_HANDLE:  ptr = &NtCurrentTeb()->Peb->ProcessParameters->hStdInput;  break;
    case STD_OUTPUT_HANDLE: ptr = &NtCurrentTeb()->Peb->ProcessParameters->hStdOutput; break;
    case STD_ERROR_HANDLE:  ptr = &NtCurrentTeb()->Peb->ProcessParameters->hStdError;  break;
    default:
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (prev) *prev = *ptr;
    *ptr = handle;
    return TRUE;
}


/***********************************************************************
 * Process environment
 ***********************************************************************/


static inline SIZE_T get_env_length( const WCHAR *env )
{
    const WCHAR *end = env;
    while (*end) end += lstrlenW(end) + 1;
    return end + 1 - env;
}

/***********************************************************************
 *           ExpandEnvironmentStringsA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH ExpandEnvironmentStringsA( LPCSTR src, LPSTR dst, DWORD count )
{
    UNICODE_STRING us_src;
    PWSTR dstW = NULL;
    DWORD count_neededW;
    DWORD count_neededA = 0;

    RtlCreateUnicodeStringFromAsciiz( &us_src, src );

    /* We always need to call ExpandEnvironmentStringsW, since we need the result to calculate the needed buffer size */
    count_neededW = ExpandEnvironmentStringsW( us_src.Buffer, NULL, 0 );
    if (!(dstW = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, count_neededW * sizeof(WCHAR) ))) goto cleanup;
    count_neededW = ExpandEnvironmentStringsW( us_src.Buffer, dstW, count_neededW );

    /* Calculate needed buffer */
    count_neededA = WideCharToMultiByte( CP_ACP, 0, dstW, count_neededW, NULL, 0, NULL, NULL );

    /* If provided buffer is enough, do actual conversion */
    if (count > count_neededA)
        count_neededA = WideCharToMultiByte( CP_ACP, 0, dstW, count_neededW, dst, count, NULL, NULL );
    else if(dst)
        *dst = 0;

cleanup:
    RtlFreeUnicodeString( &us_src );
    HeapFree( GetProcessHeap(), 0, dstW );

    if (count_neededA >= count) /* When the buffer is too small, native over-reports by one byte */
        return count_neededA + 1;
    return count_neededA;
}


/***********************************************************************
 *           ExpandEnvironmentStringsW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH ExpandEnvironmentStringsW( LPCWSTR src, LPWSTR dst, DWORD len )
{
    UNICODE_STRING us_src, us_dst;
    NTSTATUS status;
    DWORD res;

    TRACE( "(%s %p %lu)\n", debugstr_w(src), dst, len );

    RtlInitUnicodeString( &us_src, src );

    /* make sure we don't overflow the maximum UNICODE_STRING size */
    len = min( len, UNICODE_STRING_MAX_CHARS );

    us_dst.Length = 0;
    us_dst.MaximumLength = len * sizeof(WCHAR);
    us_dst.Buffer = dst;

    res = 0;
    status = RtlExpandEnvironmentStrings_U( NULL, &us_src, &us_dst, &res );
    res /= sizeof(WCHAR);
    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        if(!set_ntstatus( status ))
            return 0;
    }
    return res;
}


/***********************************************************************
 *           GetEnvironmentStrings    (kernelbase.@)
 *           GetEnvironmentStringsA   (kernelbase.@)
 */
LPSTR WINAPI DECLSPEC_HOTPATCH GetEnvironmentStringsA(void)
{
    LPWSTR env;
    LPSTR ret;
    SIZE_T lenA, lenW;

    RtlAcquirePebLock();
    env = NtCurrentTeb()->Peb->ProcessParameters->Environment;
    lenW = get_env_length( env );
    lenA = WideCharToMultiByte( CP_ACP, 0, env, lenW, NULL, 0, NULL, NULL );
    if ((ret = HeapAlloc( GetProcessHeap(), 0, lenA )))
        WideCharToMultiByte( CP_ACP, 0, env, lenW, ret, lenA, NULL, NULL );
    RtlReleasePebLock();
    return ret;
}


/***********************************************************************
 *           GetEnvironmentStringsW   (kernelbase.@)
 */
LPWSTR WINAPI DECLSPEC_HOTPATCH GetEnvironmentStringsW(void)
{
    LPWSTR ret;
    SIZE_T len;

    RtlAcquirePebLock();
    len = get_env_length( NtCurrentTeb()->Peb->ProcessParameters->Environment ) * sizeof(WCHAR);
    if ((ret = HeapAlloc( GetProcessHeap(), 0, len )))
        memcpy( ret, NtCurrentTeb()->Peb->ProcessParameters->Environment, len );
    RtlReleasePebLock();
    return ret;
}


/***********************************************************************
 *           SetEnvironmentStringsA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetEnvironmentStringsA( char *env )
{
    WCHAR *envW;
    const char *p = env;
    DWORD len;
    BOOL ret;

    for (p = env; *p; p += strlen( p ) + 1);

    len = MultiByteToWideChar( CP_ACP, 0, env, p - env, NULL, 0 );
    if (!(envW = HeapAlloc( GetProcessHeap(), 0, len )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    MultiByteToWideChar( CP_ACP, 0, env, p - env, envW, len );
    ret = SetEnvironmentStringsW( envW );
    HeapFree( GetProcessHeap(), 0, envW );
    return ret;
}


/***********************************************************************
 *           SetEnvironmentStringsW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetEnvironmentStringsW( WCHAR *env )
{
    WCHAR *p;
    WCHAR *new_env;
    NTSTATUS status;

    for (p = env; *p; p += wcslen( p ) + 1)
    {
        const WCHAR *eq = wcschr( p, '=' );
        if (!eq || eq == p)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }

    if ((status = RtlCreateEnvironment( FALSE, &new_env )))
        return set_ntstatus( status );

    for (p = env; *p; p += wcslen( p ) + 1)
    {
        const WCHAR *eq = wcschr( p, '=' );
        UNICODE_STRING var, value;
        var.Buffer = p;
        var.Length = (eq - p) * sizeof(WCHAR);
        RtlInitUnicodeString( &value, eq + 1 );
        if ((status = RtlSetEnvironmentVariable( &new_env, &var, &value )))
        {
            RtlDestroyEnvironment( new_env );
            return set_ntstatus( status );
        }
    }

    RtlSetCurrentEnvironment( new_env, NULL );
    return TRUE;
}


/***********************************************************************
 *           GetEnvironmentVariableA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetEnvironmentVariableA( LPCSTR name, LPSTR value, DWORD size )
{
    UNICODE_STRING us_name, us_value;
    PWSTR valueW;
    NTSTATUS status;
    DWORD len, ret;

    /* limit the size to sane values */
    size = min( size, 32767 );
    if (!(valueW = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return 0;

    RtlCreateUnicodeStringFromAsciiz( &us_name, name );
    us_value.Length = 0;
    us_value.MaximumLength = (size ? size - 1 : 0) * sizeof(WCHAR);
    us_value.Buffer = valueW;

    status = RtlQueryEnvironmentVariable_U( NULL, &us_name, &us_value );
    len = us_value.Length / sizeof(WCHAR);
    if (status == STATUS_BUFFER_TOO_SMALL) ret = len + 1;
    else if (!set_ntstatus( status )) ret = 0;
    else if (!size) ret = len + 1;
    else
    {
        if (len) WideCharToMultiByte( CP_ACP, 0, valueW, len + 1, value, size, NULL, NULL );
        value[len] = 0;
        ret = len;
    }

    RtlFreeUnicodeString( &us_name );
    HeapFree( GetProcessHeap(), 0, valueW );
    return ret;
}


/***********************************************************************
 *           GetEnvironmentVariableW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetEnvironmentVariableW( LPCWSTR name, LPWSTR val, DWORD size )
{
    UNICODE_STRING us_name, us_value;
    NTSTATUS status;
    DWORD len;

    TRACE( "(%s %p %lu)\n", debugstr_w(name), val, size );

    RtlInitUnicodeString( &us_name, name );
    us_value.Length = 0;
    us_value.MaximumLength = (size ? size - 1 : 0) * sizeof(WCHAR);
    us_value.Buffer = val;

    status = RtlQueryEnvironmentVariable_U( NULL, &us_name, &us_value );
    len = us_value.Length / sizeof(WCHAR);
    if (status == STATUS_BUFFER_TOO_SMALL) return len + 1;
    if (!set_ntstatus( status )) return 0;
    if (!size) return len + 1;
    val[len] = 0;
    return len;
}


/***********************************************************************
 *           FreeEnvironmentStringsA   (kernelbase.@)
 *           FreeEnvironmentStringsW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FreeEnvironmentStringsW( LPWSTR ptr )
{
    return HeapFree( GetProcessHeap(), 0, ptr );
}


/***********************************************************************
 *           SetEnvironmentVariableA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetEnvironmentVariableA( LPCSTR name, LPCSTR value )
{
    UNICODE_STRING us_name, us_value;
    BOOL ret;

    if (!name)
    {
        SetLastError( ERROR_ENVVAR_NOT_FOUND );
        return FALSE;
    }

    RtlCreateUnicodeStringFromAsciiz( &us_name, name );
    if (value)
    {
        RtlCreateUnicodeStringFromAsciiz( &us_value, value );
        ret = SetEnvironmentVariableW( us_name.Buffer, us_value.Buffer );
        RtlFreeUnicodeString( &us_value );
    }
    else ret = SetEnvironmentVariableW( us_name.Buffer, NULL );
    RtlFreeUnicodeString( &us_name );
    return ret;
}


/***********************************************************************
 *           SetEnvironmentVariableW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetEnvironmentVariableW( LPCWSTR name, LPCWSTR value )
{
    UNICODE_STRING us_name, us_value;
    NTSTATUS status;

    TRACE( "(%s %s)\n", debugstr_w(name), debugstr_w(value) );

    if (!name)
    {
        SetLastError( ERROR_ENVVAR_NOT_FOUND );
        return FALSE;
    }

    RtlInitUnicodeString( &us_name, name );
    if (value)
    {
        RtlInitUnicodeString( &us_value, value );
        status = RtlSetEnvironmentVariable( NULL, &us_name, &us_value );
    }
    else status = RtlSetEnvironmentVariable( NULL, &us_name, NULL );

    return set_ntstatus( status );
}


/***********************************************************************
 * Process/thread attribute lists
 ***********************************************************************/

/***********************************************************************
 *           InitializeProcThreadAttributeList   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH InitializeProcThreadAttributeList( struct _PROC_THREAD_ATTRIBUTE_LIST *list,
                                                                 DWORD count, DWORD flags, SIZE_T *size )
{
    SIZE_T needed;
    BOOL ret = FALSE;

    TRACE( "(%p %ld %lx %p)\n", list, count, flags, size );

    needed = FIELD_OFFSET( struct _PROC_THREAD_ATTRIBUTE_LIST, attrs[count] );
    if (list && *size >= needed)
    {
        list->mask = 0;
        list->size = count;
        list->count = 0;
        list->unk = 0;
        ret = TRUE;
    }
    else SetLastError( ERROR_INSUFFICIENT_BUFFER );

    *size = needed;
    return ret;
}


static inline DWORD validate_proc_thread_attribute( DWORD_PTR attr, SIZE_T size )
{
    switch (attr)
    {
    case PROC_THREAD_ATTRIBUTE_PARENT_PROCESS:
        if (size != sizeof(HANDLE)) return ERROR_BAD_LENGTH;
        break;
    case PROC_THREAD_ATTRIBUTE_EXTENDED_FLAGS:
        if (size != sizeof(ULONG)) return ERROR_BAD_LENGTH;
        break;
    case PROC_THREAD_ATTRIBUTE_HANDLE_LIST:
        if ((size / sizeof(HANDLE)) * sizeof(HANDLE) != size) return ERROR_BAD_LENGTH;
        break;
    case PROC_THREAD_ATTRIBUTE_IDEAL_PROCESSOR:
        if (size != sizeof(PROCESSOR_NUMBER)) return ERROR_BAD_LENGTH;
        break;
    case PROC_THREAD_ATTRIBUTE_CHILD_PROCESS_POLICY:
       if (size != sizeof(DWORD) && size != sizeof(DWORD64)) return ERROR_BAD_LENGTH;
       break;
    case PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY:
        if (size != sizeof(DWORD) && size != sizeof(DWORD64) && size != sizeof(DWORD64) * 2)
            return ERROR_BAD_LENGTH;
        break;
    case PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE:
       if (size != sizeof(HPCON)) return ERROR_BAD_LENGTH;
       break;
    case PROC_THREAD_ATTRIBUTE_JOB_LIST:
        if ((size / sizeof(HANDLE)) * sizeof(HANDLE) != size) return ERROR_BAD_LENGTH;
        break;
    case PROC_THREAD_ATTRIBUTE_MACHINE_TYPE:
        if (size != sizeof(USHORT)) return ERROR_BAD_LENGTH;
        break;
    case PROC_THREAD_ATTRIBUTE_GROUP_AFFINITY:
        if (size != sizeof(GROUP_AFFINITY)) return ERROR_BAD_LENGTH;
        break;
    default:
        FIXME( "Unhandled attribute %Iu\n", attr & PROC_THREAD_ATTRIBUTE_NUMBER );
        return ERROR_NOT_SUPPORTED;
    }
    return 0;
}


/***********************************************************************
 *           UpdateProcThreadAttribute   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH UpdateProcThreadAttribute( struct _PROC_THREAD_ATTRIBUTE_LIST *list,
                                                         DWORD flags, DWORD_PTR attr, void *value,
                                                         SIZE_T size, void *prev_ret, SIZE_T *size_ret )
{
    DWORD mask, err;
    struct proc_thread_attr *entry;

    TRACE( "(%p %lx %08Ix %p %Id %p %p)\n", list, flags, attr, value, size, prev_ret, size_ret );

    if (list->count >= list->size)
    {
        SetLastError( ERROR_GEN_FAILURE );
        return FALSE;
    }
    if ((err = validate_proc_thread_attribute( attr, size )))
    {
        SetLastError( err );
        return FALSE;
    }

    mask = 1 << (attr & PROC_THREAD_ATTRIBUTE_NUMBER);
    if (list->mask & mask)
    {
        SetLastError( ERROR_OBJECT_NAME_EXISTS );
        return FALSE;
    }
    list->mask |= mask;

    entry = list->attrs + list->count;
    entry->attr = attr;
    entry->size = size;
    entry->value = value;
    list->count++;
    return TRUE;
}


/***********************************************************************
 *           DeleteProcThreadAttributeList   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH DeleteProcThreadAttributeList( struct _PROC_THREAD_ATTRIBUTE_LIST *list )
{
    return;
}


/***********************************************************************
 *              CompareObjectHandles   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CompareObjectHandles( HANDLE first, HANDLE second )
{
    return set_ntstatus( NtCompareObjects( first, second ));
}

/***********************************************************************
 *           PssQuerySnapshot   (kernelbase.@)
 */
DWORD WINAPI PssQuerySnapshot( HPSS handle, PSS_QUERY_INFORMATION_CLASS class, void *buffer, DWORD len )
{
    FIXME( "(%p %u %p %lu)\n", handle ,class, buffer, len );
    return ERROR_NOT_FOUND;
}

/***********************************************************************
 *           PssFreeSnapshot   (kernelbase.@)
 */
DWORD WINAPI PssFreeSnapshot( HANDLE hprocess, HPSS handle )
{
    FIXME( "(%p %p)\n", hprocess , handle );
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           PssCaptureSnapshot   (kernelbase.@)
 */
DWORD WINAPI PssCaptureSnapshot( HANDLE hprocess, PSS_CAPTURE_FLAGS flags, DWORD ctx_flags, HPSS *handle )
{
    FIXME( "(%p %u %lu %p)\n", hprocess , flags, ctx_flags, handle );
    return ERROR_NOT_FOUND;
}
