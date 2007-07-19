/*
 * Activation contexts
 *
 * Copyright 2004 Jon Griffiths
 * Copyright 2007 Eric Pouech
 * Copyright 2007 Jacek Caban for CodeWeavers
 * Copyright 2007 Alexandre Julliard
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
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(actctx);

#define ACTCTX_FLAGS_ALL (\
 ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID |\
 ACTCTX_FLAG_LANGID_VALID |\
 ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID |\
 ACTCTX_FLAG_RESOURCE_NAME_VALID |\
 ACTCTX_FLAG_SET_PROCESS_DEFAULT |\
 ACTCTX_FLAG_APPLICATION_NAME_VALID |\
 ACTCTX_FLAG_SOURCE_IS_ASSEMBLYREF |\
 ACTCTX_FLAG_HMODULE_VALID )

#define ACTCTX_MAGIC       0xC07E3E11

/* we don't want to include winuser.h */
#define RT_MANIFEST                        ((ULONG_PTR)24)
#define CREATEPROCESS_MANIFEST_RESOURCE_ID ((ULONG_PTR)1)

typedef struct
{
    const char*         ptr;
    const char*         end;
} xmlbuf_t;

struct file_info
{
    ULONG               type;
    WCHAR              *info;
};

struct version
{
    USHORT              major;
    USHORT              minor;
    USHORT              build;
    USHORT              revision;
};

struct assembly_identity
{
    WCHAR              *name;
    struct version      version;
};

enum assembly_type
{
    APPLICATION_MANIFEST,
    ASSEMBLY_MANIFEST
};

struct assembly
{
    enum assembly_type       type;
    struct assembly_identity id;
    struct file_info         manifest;
};

typedef struct _ACTIVATION_CONTEXT
{
    ULONG               magic;
    int                 ref_count;
    struct file_info    config;
    struct file_info    appdir;
    struct assembly    *assemblies;
    unsigned int        num_assemblies;
    unsigned int        allocated_assemblies;
} ACTIVATION_CONTEXT;

struct actctx_loader
{
    ACTIVATION_CONTEXT       *actctx;
    struct assembly_identity *dependencies;
    unsigned int              num_dependencies;
    unsigned int              allocated_dependencies;
};

static const WCHAR dotManifestW[] = {'.','m','a','n','i','f','e','s','t',0};


static WCHAR *strdupW(const WCHAR* str)
{
    WCHAR*      ptr;

    if (!str) return NULL;
    if (!(ptr = RtlAllocateHeap(GetProcessHeap(), 0, (strlenW(str) + 1) * sizeof(WCHAR))))
        return NULL;
    return strcpyW(ptr, str);
}

static struct assembly *add_assembly(ACTIVATION_CONTEXT *actctx, enum assembly_type at)
{
    struct assembly *assembly;

    if (actctx->num_assemblies == actctx->allocated_assemblies)
    {
        void *ptr;
        unsigned int new_count;
        if (actctx->assemblies)
        {
            new_count = actctx->allocated_assemblies * 2;
            ptr = RtlReAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                     actctx->assemblies, new_count * sizeof(*assembly) );
        }
        else
        {
            new_count = 4;
            ptr = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, new_count * sizeof(*assembly) );
        }
        if (!ptr) return NULL;
        actctx->assemblies = ptr;
        actctx->allocated_assemblies = new_count;
    }

    assembly = &actctx->assemblies[actctx->num_assemblies++];
    assembly->type = at;
    return assembly;
}

static void free_assembly_identity(struct assembly_identity *ai)
{
    RtlFreeHeap( GetProcessHeap(), 0, ai->name );
}

static ACTIVATION_CONTEXT *check_actctx( HANDLE h )
{
    ACTIVATION_CONTEXT *actctx = h;

    if (!h || h == INVALID_HANDLE_VALUE) return NULL;
    switch (actctx->magic)
    {
    case ACTCTX_MAGIC:
        return actctx;
    default:
        return NULL;
    }
}

static inline void actctx_addref( ACTIVATION_CONTEXT *actctx )
{
    interlocked_xchg_add( &actctx->ref_count, 1 );
}

static void actctx_release( ACTIVATION_CONTEXT *actctx )
{
    if (interlocked_xchg_add( &actctx->ref_count, -1 ) == 1)
    {
        unsigned int i;

        for (i = 0; i < actctx->num_assemblies; i++)
        {
            RtlFreeHeap( GetProcessHeap(), 0, actctx->assemblies[i].manifest.info );
            free_assembly_identity(&actctx->assemblies[i].id);
        }
        RtlFreeHeap( GetProcessHeap(), 0, actctx->config.info );
        RtlFreeHeap( GetProcessHeap(), 0, actctx->appdir.info );
        RtlFreeHeap( GetProcessHeap(), 0, actctx->assemblies );
        actctx->magic = 0;
        RtlFreeHeap( GetProcessHeap(), 0, actctx );
    }
}

static NTSTATUS parse_manifest( struct actctx_loader* acl, struct assembly_identity* ai,
                                LPCWSTR filename, xmlbuf_t* xmlbuf )
{
    TRACE( "parsing manifest loaded from %s\n", debugstr_w(filename) );
    return STATUS_SUCCESS;
}

static NTSTATUS open_nt_file( HANDLE handle, UNICODE_STRING *name )
{
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    return NtOpenFile( handle, GENERIC_READ, &attr, &io, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_ALERT );
}

static NTSTATUS get_module_filename( HMODULE module, UNICODE_STRING *str, unsigned int extra_len )
{
    NTSTATUS status;
    ULONG magic;
    LDR_MODULE *pldr;

    LdrLockLoaderLock(0, NULL, &magic);
    status = LdrFindEntryForAddress( module, &pldr );
    if (status == STATUS_SUCCESS)
    {
        if ((str->Buffer = RtlAllocateHeap( GetProcessHeap(), 0,
                                            pldr->FullDllName.Length + extra_len + sizeof(WCHAR) )))
        {
            memcpy( str->Buffer, pldr->FullDllName.Buffer, pldr->FullDllName.Length + sizeof(WCHAR) );
            str->Length = pldr->FullDllName.Length;
            str->MaximumLength = pldr->FullDllName.Length + extra_len + sizeof(WCHAR);
        }
        else status = STATUS_NO_MEMORY;
    }
    LdrUnlockLoaderLock(0, magic);
    return status;
}

static NTSTATUS get_manifest_in_module( struct actctx_loader* acl, struct assembly_identity* ai,
                                        LPCWSTR filename, HANDLE hModule, LPCWSTR resname, ULONG lang )
{
    NTSTATUS status;
    UNICODE_STRING nameW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DATA_ENTRY* entry = NULL;
    void *ptr;

    TRACE( "looking for res %s in module %p %s\n", debugstr_w(resname), hModule, debugstr_w(filename) );

    if (!resname) return STATUS_INVALID_PARAMETER;

    info.Type = RT_MANIFEST;
    info.Language = lang;
    if (!((ULONG_PTR)resname >> 16))
    {
        info.Name = (ULONG_PTR)resname;
        status = LdrFindResource_U(hModule, &info, 3, &entry);
    }
    else if (resname[0] == '#')
    {
        ULONG value;
        RtlInitUnicodeString(&nameW, resname + 1);
        if (RtlUnicodeStringToInteger(&nameW, 10, &value) != STATUS_SUCCESS || HIWORD(value))
            return STATUS_INVALID_PARAMETER;
        info.Name = value;
        status = LdrFindResource_U(hModule, &info, 3, &entry);
    }
    else
    {
        RtlCreateUnicodeString(&nameW, resname);
        RtlUpcaseUnicodeString(&nameW, &nameW, FALSE);
        info.Name = (ULONG_PTR)nameW.Buffer;
        status = LdrFindResource_U(hModule, &info, 3, &entry);
        RtlFreeUnicodeString(&nameW);
    }
    if (status == STATUS_SUCCESS) status = LdrAccessResource(hModule, entry, &ptr, NULL);

    if (status == STATUS_SUCCESS)
    {
        xmlbuf_t buf;
        buf.ptr = ptr;
        buf.end = buf.ptr + entry->Size;
        status = parse_manifest(acl, ai, filename, &buf);
    }
    return status;
}

static NTSTATUS get_manifest_in_pe_file( struct actctx_loader* acl, struct assembly_identity* ai,
                                         LPCWSTR filename, HANDLE file, LPCWSTR resname, ULONG lang )
{
    HANDLE              mapping;
    OBJECT_ATTRIBUTES   attr;
    LARGE_INTEGER       size;
    LARGE_INTEGER       offset;
    NTSTATUS            status;
    SIZE_T              count;
    void               *base;

    TRACE( "looking for res %s in %s\n", debugstr_w(resname), debugstr_w(filename) );

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_CASE_INSENSITIVE | OBJ_OPENIF;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;

    size.QuadPart = 0;
    status = NtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ,
                              &attr, &size, PAGE_READONLY, SEC_COMMIT, file );
    if (status != STATUS_SUCCESS) return status;

    offset.QuadPart = 0;
    count = 0;
    base = NULL;
    status = NtMapViewOfSection( mapping, GetCurrentProcess(), &base, 0, 0, &offset,
                                 &count, ViewShare, 0, PAGE_READONLY );
    NtClose( mapping );
    if (status != STATUS_SUCCESS) return status;

    if (RtlImageNtHeader(base)) /* we got a PE file */
    {
        HANDLE module = (HMODULE)((ULONG_PTR)base | 1);  /* make it a LOAD_LIBRARY_AS_DATAFILE handle */
        status = get_manifest_in_module( acl, ai, filename, module, resname, lang );
    }
    else status = STATUS_INVALID_IMAGE_FORMAT;

    NtUnmapViewOfSection( GetCurrentProcess(), base );
    return status;
}

static NTSTATUS get_manifest_in_manifest_file( struct actctx_loader* acl, struct assembly_identity* ai,
                                               LPCWSTR filename, HANDLE file )
{
    HANDLE              mapping;
    OBJECT_ATTRIBUTES   attr;
    LARGE_INTEGER       size;
    LARGE_INTEGER       offset;
    NTSTATUS            status;
    SIZE_T              count;
    void               *base;
    xmlbuf_t            buf;

    TRACE( "loading manifest file %s\n", debugstr_w(filename) );

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_CASE_INSENSITIVE | OBJ_OPENIF;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;

    size.QuadPart = 0;
    status = NtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ,
                              &attr, &size, PAGE_READONLY, SEC_COMMIT, file );
    if (status != STATUS_SUCCESS) return status;

    offset.QuadPart = 0;
    count = 0;
    base = NULL;
    status = NtMapViewOfSection( mapping, GetCurrentProcess(), &base, 0, 0, &offset,
                                 &count, ViewShare, 0, PAGE_READONLY );
    NtClose( mapping );
    if (status != STATUS_SUCCESS) return status;

    buf.ptr = base;
    buf.end = buf.ptr + count;
    status = parse_manifest(acl, ai, filename, &buf);

    NtUnmapViewOfSection( GetCurrentProcess(), base );
    return status;
}

/* try to load the .manifest file associated to the file */
static NTSTATUS get_manifest_in_associated_manifest( struct actctx_loader* acl, struct assembly_identity* ai,
                                                     LPCWSTR filename, HMODULE module, LPCWSTR resname )
{
    static const WCHAR fmtW[] = { '.','%','l','u',0 };
    WCHAR *buffer;
    NTSTATUS status;
    UNICODE_STRING nameW;
    HANDLE file;
    ULONG_PTR resid = CREATEPROCESS_MANIFEST_RESOURCE_ID;

    if (!((ULONG_PTR)resname >> 16)) resid = (ULONG_PTR)resname & 0xffff;

    TRACE( "looking for manifest associated with %s id %lu\n", debugstr_w(filename), resid );

    if (module) /* use the module filename */
    {
        UNICODE_STRING name;

        if (!(status = get_module_filename( module, &name, sizeof(dotManifestW) + 10*sizeof(WCHAR) )))
        {
            if (resid != 1) sprintfW( name.Buffer + strlenW(name.Buffer), fmtW, resid );
            strcatW( name.Buffer, dotManifestW );
            if (!RtlDosPathNameToNtPathName_U( name.Buffer, &nameW, NULL, NULL ))
                status = STATUS_NO_SUCH_FILE;
            RtlFreeUnicodeString( &name );
        }
        if (status) return status;
    }
    else
    {
        if (!(buffer = RtlAllocateHeap( GetProcessHeap(), 0,
                                        (strlenW(filename) + 10) * sizeof(WCHAR) + sizeof(dotManifestW) )))
            return STATUS_NO_MEMORY;
        strcpyW( buffer, filename );
        if (resid != 1) sprintfW( buffer + strlenW(buffer), fmtW, resid );
        strcatW( buffer, dotManifestW );
        RtlInitUnicodeString( &nameW, buffer );
    }

    status = open_nt_file( &file, &nameW );
    if (status == STATUS_SUCCESS)
    {
        status = get_manifest_in_manifest_file( acl, ai, nameW.Buffer, file );
        NtClose( file );
    }
    RtlFreeUnicodeString( &nameW );
    return status;
}


/***********************************************************************
 * RtlCreateActivationContext (NTDLL.@)
 *
 * Create an activation context.
 *
 * FIXME: function signature/prototype is wrong
 */
NTSTATUS WINAPI RtlCreateActivationContext( HANDLE *handle, const void *ptr )
{
    const ACTCTXW *pActCtx = ptr;  /* FIXME: not the right structure */
    ACTIVATION_CONTEXT *actctx;
    UNICODE_STRING nameW;
    struct assembly *assembly;
    ULONG lang = 0;
    NTSTATUS status = STATUS_NO_MEMORY;
    HANDLE file = 0;
    struct actctx_loader acl;

    TRACE("%p %08x\n", pActCtx, pActCtx ? pActCtx->dwFlags : 0);

    if (!pActCtx || pActCtx->cbSize != sizeof(*pActCtx) ||
        (pActCtx->dwFlags & ~ACTCTX_FLAGS_ALL))
        return STATUS_INVALID_PARAMETER;

    if (!(actctx = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*actctx) )))
        return STATUS_NO_MEMORY;

    actctx->magic = ACTCTX_MAGIC;
    actctx->ref_count = 1;

    if (!(assembly = add_assembly( actctx, APPLICATION_MANIFEST ))) goto error;
    if (!(assembly->id.name = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR) )))
        goto error;
    assembly->id.version.major = 1;
    assembly->id.version.minor = 0;
    assembly->id.version.build = 0;
    assembly->id.version.revision = 0;
    assembly->manifest.type = ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE;
    assembly->manifest.info = NULL;

    actctx->config.type = ACTIVATION_CONTEXT_PATH_TYPE_NONE;
    actctx->config.info = NULL;
    actctx->appdir.type = ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE;
    if (pActCtx->dwFlags & ACTCTX_FLAG_APPLICATION_NAME_VALID)
    {
        if (!(actctx->appdir.info = strdupW( pActCtx->lpApplicationName ))) goto error;
    }
    else
    {
        UNICODE_STRING dir;
        WCHAR *p;

        if ((status = get_module_filename( NtCurrentTeb()->Peb->ImageBaseAddress, &dir, 0 )))
            goto error;
        if ((p = strrchrW( dir.Buffer, '\\' ))) *p = 0;
        actctx->appdir.info = dir.Buffer;
    }

    nameW.Buffer = NULL;
    if (pActCtx->lpSource)
    {
        if (!RtlDosPathNameToNtPathName_U(pActCtx->lpSource, &nameW, NULL, NULL))
        {
            status = STATUS_NO_SUCH_FILE;
            goto error;
        }
        status = open_nt_file( &file, &nameW );
        if (status)
        {
            RtlFreeUnicodeString( &nameW );
            goto error;
        }
    }

    acl.actctx = actctx;
    acl.dependencies = NULL;
    acl.num_dependencies = 0;
    acl.allocated_dependencies = 0;

    if (pActCtx->dwFlags & ACTCTX_FLAG_LANGID_VALID) lang = pActCtx->wLangId;

    if (pActCtx->dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID)
    {
        /* if we have a resource it's a PE file */
        if (pActCtx->dwFlags & ACTCTX_FLAG_HMODULE_VALID)
        {
            status = get_manifest_in_module( &acl, NULL, NULL, pActCtx->hModule,
                                             pActCtx->lpResourceName, lang );
            if (status)
                /* FIXME: what to do if pActCtx->lpSource is set */
                status = get_manifest_in_associated_manifest( &acl, NULL, NULL, pActCtx->hModule,
                                                              pActCtx->lpResourceName );
        }
        else if (pActCtx->lpSource)
        {
            status = get_manifest_in_pe_file( &acl, NULL, nameW.Buffer, file,
                                              pActCtx->lpResourceName, lang );
            if (status)
                status = get_manifest_in_associated_manifest( &acl, NULL, nameW.Buffer, NULL,
                                                              pActCtx->lpResourceName );
        }
        else status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = get_manifest_in_manifest_file( &acl, NULL, nameW.Buffer, file );
    }

    if (file) NtClose( file );
    RtlFreeUnicodeString( &nameW );
    if (status == STATUS_SUCCESS) *handle = actctx;
    else actctx_release( actctx );
    return status;

error:
    if (file) NtClose( file );
    actctx_release( actctx );
    return status;
}


/***********************************************************************
 *		RtlAddRefActivationContext (NTDLL.@)
 */
void WINAPI RtlAddRefActivationContext( HANDLE handle )
{
    ACTIVATION_CONTEXT *actctx;

    if ((actctx = check_actctx( handle ))) actctx_addref( actctx );
}


/******************************************************************
 *		RtlReleaseActivationContext (NTDLL.@)
 */
void WINAPI RtlReleaseActivationContext( HANDLE handle )
{
    ACTIVATION_CONTEXT *actctx;

    if ((actctx = check_actctx( handle ))) actctx_release( actctx );
}


/******************************************************************
 *		RtlActivateActivationContext (NTDLL.@)
 */
NTSTATUS WINAPI RtlActivateActivationContext( ULONG unknown, HANDLE handle, ULONG_PTR *cookie )
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame;

    TRACE( "%p %p\n", handle, cookie );

    if (!(frame = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*frame) )))
        return STATUS_NO_MEMORY;

    frame->Previous = NtCurrentTeb()->ActivationContextStack.ActiveFrame;
    frame->ActivationContext = handle;
    frame->Flags = 0;
    NtCurrentTeb()->ActivationContextStack.ActiveFrame = frame;
    RtlAddRefActivationContext( handle );

    *cookie = (ULONG_PTR)frame;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *		RtlDeactivateActivationContext (NTDLL.@)
 */
void WINAPI RtlDeactivateActivationContext( ULONG flags, ULONG_PTR cookie )
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame, *top;

    TRACE( "%x %lx\n", flags, cookie );

    /* find the right frame */
    top = NtCurrentTeb()->ActivationContextStack.ActiveFrame;
    for (frame = top; frame; frame = frame->Previous)
        if ((ULONG_PTR)frame == cookie) break;

    if (!frame)
        RtlRaiseStatus( STATUS_SXS_INVALID_DEACTIVATION );

    if (frame != top && !(flags & DEACTIVATE_ACTCTX_FLAG_FORCE_EARLY_DEACTIVATION))
        RtlRaiseStatus( STATUS_SXS_EARLY_DEACTIVATION );

    /* pop everything up to and including frame */
    NtCurrentTeb()->ActivationContextStack.ActiveFrame = frame->Previous;

    while (top != NtCurrentTeb()->ActivationContextStack.ActiveFrame)
    {
        frame = top->Previous;
        RtlReleaseActivationContext( top->ActivationContext );
        RtlFreeHeap( GetProcessHeap(), 0, top );
        top = frame;
    }
}


/******************************************************************
 *		RtlFreeThreadActivationContextStack (NTDLL.@)
 */
void WINAPI RtlFreeThreadActivationContextStack(void)
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame;

    frame = NtCurrentTeb()->ActivationContextStack.ActiveFrame;
    while (frame)
    {
        RTL_ACTIVATION_CONTEXT_STACK_FRAME *prev = frame->Previous;
        RtlReleaseActivationContext( frame->ActivationContext );
        RtlFreeHeap( GetProcessHeap(), 0, frame );
        frame = prev;
    }
    NtCurrentTeb()->ActivationContextStack.ActiveFrame = NULL;
}


/******************************************************************
 *		RtlGetActiveActivationContext (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetActiveActivationContext( HANDLE *handle )
{
    if (NtCurrentTeb()->ActivationContextStack.ActiveFrame)
    {
        *handle = NtCurrentTeb()->ActivationContextStack.ActiveFrame->ActivationContext;
        RtlAddRefActivationContext( *handle );
    }
    else
        *handle = 0;

    return STATUS_SUCCESS;
}


/******************************************************************
 *		RtlIsActivationContextActive (NTDLL.@)
 */
BOOLEAN WINAPI RtlIsActivationContextActive( HANDLE handle )
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame;

    for (frame = NtCurrentTeb()->ActivationContextStack.ActiveFrame; frame; frame = frame->Previous)
        if (frame->ActivationContext == handle) return TRUE;
    return FALSE;
}
