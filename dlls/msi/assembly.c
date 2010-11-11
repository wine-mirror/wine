/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2010 Hans Leidekker for CodeWeavers
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "msipriv.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static HRESULT (WINAPI *pCreateAssemblyCacheNet)( IAssemblyCache **, DWORD );
static HRESULT (WINAPI *pCreateAssemblyCacheSxs)( IAssemblyCache **, DWORD );
static HRESULT (WINAPI *pLoadLibraryShim)( LPCWSTR, LPCWSTR, LPVOID, HMODULE * );

static BOOL init_function_pointers( void )
{
    static const WCHAR szFusion[] = {'f','u','s','i','o','n','.','d','l','l',0};
    HMODULE hfusion, hmscoree, hsxs;

    if (pCreateAssemblyCacheNet) return TRUE;

    if (!(hmscoree = LoadLibraryA( "mscoree.dll" )))
    {
        WARN("mscoree.dll not available\n");
        return FALSE;
    }
    if (!(pLoadLibraryShim = (void *)GetProcAddress( hmscoree, "LoadLibraryShim" )))
    {
        WARN("LoadLibraryShim not available\n");
        FreeLibrary( hmscoree );
        return FALSE;
    }
    if (FAILED( pLoadLibraryShim( szFusion, NULL, NULL, &hfusion )))
    {
        WARN("fusion.dll not available\n");
        FreeLibrary( hmscoree );
        return FALSE;
    }
    pCreateAssemblyCacheNet = (void *)GetProcAddress( hfusion, "CreateAssemblyCache" );
    FreeLibrary( hmscoree );
    if (!(hsxs = LoadLibraryA( "sxs.dll" )))
    {
        WARN("sxs.dll not available\n");
        FreeLibrary( hfusion );
        return FALSE;
    }
    pCreateAssemblyCacheSxs = (void *)GetProcAddress( hsxs, "CreateAssemblyCache" );
    return TRUE;
}

static BOOL init_assembly_caches( MSIPACKAGE *package )
{
    HRESULT hr;

    if (!init_function_pointers()) return FALSE;
    if (package->cache_net) return TRUE;

    hr = pCreateAssemblyCacheNet( &package->cache_net, 0 );
    if (hr != S_OK) return FALSE;

    hr = pCreateAssemblyCacheSxs( &package->cache_sxs, 0 );
    if (hr != S_OK)
    {
        IAssemblyCache_Release( package->cache_net );
        package->cache_net = NULL;
        return FALSE;
    }
    return TRUE;
}

MSIRECORD *get_assembly_record( MSIPACKAGE *package, const WCHAR *comp )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','M','s','i','A','s','s','e','m','b','l','y','`',' ',
         'W','H','E','R','E',' ','`','C','o','m','p','o','n','e','n','t','_','`',
         ' ','=',' ','\'','%','s','\'',0};
    MSIQUERY *view;
    MSIRECORD *rec;
    UINT r;

    r = MSI_OpenQuery( package->db, &view, query, comp );
    if (r != ERROR_SUCCESS)
        return NULL;

    r = MSI_ViewExecute( view, NULL );
    if (r != ERROR_SUCCESS)
    {
        msiobj_release( &view->hdr );
        return NULL;
    }
    r = MSI_ViewFetch( view, &rec );
    if (r != ERROR_SUCCESS)
    {
        msiobj_release( &view->hdr );
        return NULL;
    }
    if (!MSI_RecordGetString( rec, 4 ))
        TRACE("component is a global assembly\n");

    msiobj_release( &view->hdr );
    return rec;
}

struct assembly_name
{
    WCHAR *type;
    WCHAR *name;
    WCHAR *version;
    WCHAR *culture;
    WCHAR *token;
    WCHAR *arch;
};

static UINT get_assembly_name_attribute( MSIRECORD *rec, LPVOID param )
{
    static const WCHAR typeW[] = {'t','y','p','e',0};
    static const WCHAR nameW[] = {'n','a','m','e',0};
    static const WCHAR versionW[] = {'v','e','r','s','i','o','n',0};
    static const WCHAR cultureW[] = {'c','u','l','t','u','r','e',0};
    static const WCHAR tokenW[] = {'p','u','b','l','i','c','K','e','y','T','o','k','e','n',0};
    static const WCHAR archW[] = {'p','r','o','c','e','s','s','o','r','A','r','c','h','i','t','e','c','t','u','r','e',0};
    struct assembly_name *name = param;
    const WCHAR *attr = MSI_RecordGetString( rec, 2 );
    WCHAR *value = msi_dup_record_field( rec, 3 );

    if (!strcmpiW( attr, typeW ))
        name->type = value;
    else if (!strcmpiW( attr, nameW ))
        name->name = value;
    else if (!strcmpiW( attr, versionW ))
        name->version = value;
    else if (!strcmpiW( attr, cultureW ))
        name->culture = value;
    else if (!strcmpiW( attr, tokenW ))
        name->token = value;
    else if (!strcmpiW( attr, archW ))
        name->arch = value;
    else
        msi_free( value );

    return ERROR_SUCCESS;
}

static WCHAR *get_assembly_display_name( MSIDATABASE *db, const WCHAR *comp, MSIASSEMBLY *assembly )
{
    static const WCHAR fmt_netW[] = {
        '%','s',',',' ','v','e','r','s','i','o','n','=','%','s',',',' ',
        'c','u','l','t','u','r','e','=','%','s',',',' ',
        'p','u','b','l','i','c','K','e','y','T','o','k','e','n','=','%','s',0};
    static const WCHAR fmt_sxsW[] = {
        '%','s',',',' ','v','e','r','s','i','o','n','=','%','s',',',' ',
        'p','u','b','l','i','c','K','e','y','T','o','k','e','n','=','%','s',',',' ',
        'p','r','o','c','e','s','s','o','r','A','r','c','h','i','t','e','c','t','u','r','e','=','%','s',0};
    static const WCHAR queryW[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
        '`','M','s','i','A','s','s','e','m','b','l','y','N','a','m','e','`',' ',
        'W','H','E','R','E',' ','`','C','o','m','p','o','n','e','n','t','_','`',
        ' ','=',' ','\'','%','s','\'',0};
    struct assembly_name name;
    WCHAR *display_name = NULL;
    MSIQUERY *view;
    int len;
    UINT r;

    memset( &name, 0, sizeof(name) );

    r = MSI_OpenQuery( db, &view, queryW, comp );
    if (r != ERROR_SUCCESS)
        return NULL;

    MSI_IterateRecords( view, NULL, get_assembly_name_attribute, &name );
    msiobj_release( &view->hdr );

    if (assembly->attributes == msidbAssemblyAttributesWin32)
    {
        if (!name.type || !name.name || !name.version || !name.token || !name.arch)
        {
            WARN("invalid win32 assembly name\n");
            goto done;
        }
        len = strlenW( fmt_sxsW );
        len += strlenW( name.name );
        len += strlenW( name.version );
        len += strlenW( name.token );
        len += strlenW( name.arch );
        if (!(display_name = msi_alloc( len * sizeof(WCHAR) ))) goto done;
        sprintfW( display_name, fmt_sxsW, name.name, name.version, name.token, name.arch );
    }
    else
    {
        if (!name.name || !name.version || !name.culture || !name.token)
        {
            WARN("invalid assembly name\n");
            goto done;
        }
        len = strlenW( fmt_netW );
        len += strlenW( name.name );
        len += strlenW( name.version );
        len += strlenW( name.culture );
        len += strlenW( name.token );
        if (!(display_name = msi_alloc( len * sizeof(WCHAR) ))) goto done;
        sprintfW( display_name, fmt_netW, name.name, name.version, name.culture, name.token );
    }

done:
    msi_free( name.type );
    msi_free( name.name );
    msi_free( name.version );
    msi_free( name.culture );
    msi_free( name.token );
    msi_free( name.arch );

    return display_name;
}

static BOOL check_assembly_installed( MSIPACKAGE *package, MSIASSEMBLY *assembly )
{
    IAssemblyCache *cache;
    ASSEMBLY_INFO info;
    HRESULT hr;

    if (assembly->application)
    {
        /* FIXME: we should probably check the manifest file here */
        return FALSE;
    }

    if (!init_assembly_caches( package ))
        return FALSE;

    if (assembly->attributes == msidbAssemblyAttributesWin32)
        cache = package->cache_sxs;
    else
        cache = package->cache_net;

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    hr = IAssemblyCache_QueryAssemblyInfo( cache, QUERYASMINFO_FLAG_VALIDATE, assembly->display_name, &info );
    if (hr != S_OK)
        return FALSE;

    return (info.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED);
}

MSIASSEMBLY *load_assembly( MSIPACKAGE *package, MSICOMPONENT *comp )
{
    MSIRECORD *rec;
    MSIASSEMBLY *a;

    if (!(rec = get_assembly_record( package, comp->Component )))
        return NULL;

    if (!(a = msi_alloc_zero( sizeof(MSIASSEMBLY) )))
    {
        msiobj_release( &rec->hdr );
        return NULL;
    }
    a->feature = strdupW( MSI_RecordGetString( rec, 2 ) );
    TRACE("feature %s\n", debugstr_w(a->feature));

    a->manifest = strdupW( MSI_RecordGetString( rec, 3 ) );
    TRACE("manifest %s\n", debugstr_w(a->manifest));

    a->application = strdupW( MSI_RecordGetString( rec, 4 ) );
    TRACE("application %s\n", debugstr_w(a->application));

    a->attributes = MSI_RecordGetInteger( rec, 5 );
    TRACE("attributes %u\n", a->attributes);

    if (!(a->display_name = get_assembly_display_name( package->db, comp->Component, a )))
    {
        WARN("can't get display name\n");
        msiobj_release( &rec->hdr );
        msi_free( a );
        return NULL;
    }
    TRACE("display name %s\n", debugstr_w(a->display_name));

    a->installed = check_assembly_installed( package, a );
    TRACE("assembly is %s\n", a->installed ? "installed" : "not installed");

    msiobj_release( &rec->hdr );
    return a;
}

UINT install_assembly( MSIPACKAGE *package, MSICOMPONENT *comp )
{
    HRESULT hr;
    const WCHAR *manifest;
    IAssemblyCache *cache;
    MSIASSEMBLY *assembly = comp->assembly;
    MSIFEATURE *feature = NULL;

    if (comp->assembly->feature)
        feature = get_loaded_feature( package, comp->assembly->feature );

    if (assembly->application)
    {
        if (feature) feature->Action = INSTALLSTATE_LOCAL;
        return ERROR_SUCCESS;
    }
    if (assembly->attributes == msidbAssemblyAttributesWin32)
    {
        if (!assembly->manifest)
        {
            WARN("no manifest\n");
            return ERROR_FUNCTION_FAILED;
        }
        manifest = get_loaded_file( package, assembly->manifest )->TargetPath;
        cache = package->cache_sxs;
    }
    else
    {
        manifest = get_loaded_file( package, comp->KeyPath )->TargetPath;
        cache = package->cache_net;
    }
    TRACE("installing assembly %s\n", debugstr_w(manifest));

    hr = IAssemblyCache_InstallAssembly( cache, 0, manifest, NULL );
    if (hr != S_OK)
    {
        ERR("Failed to install assembly %s (0x%08x)\n", debugstr_w(manifest), hr);
        return ERROR_FUNCTION_FAILED;
    }
    if (feature) feature->Action = INSTALLSTATE_LOCAL;
    assembly->installed = TRUE;
    return ERROR_SUCCESS;
}

UINT ACTION_MsiPublishAssemblies( MSIPACKAGE *package )
{
    MSIRECORD *uirow;
    MSICOMPONENT *comp;

    LIST_FOR_EACH_ENTRY(comp, &package->components, MSICOMPONENT, entry)
    {
        if (!comp->assembly || !comp->Enabled)
            continue;

        /* FIXME: write assembly registry values */

        uirow = MSI_CreateRecord( 2 );
        MSI_RecordSetStringW( uirow, 2, comp->assembly->display_name );
        ui_actiondata( package, szMsiPublishAssemblies, uirow );
        msiobj_release( &uirow->hdr );
    }
    return ERROR_SUCCESS;
}
