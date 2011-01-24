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
#include "winreg.h"
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
    HRESULT hr;

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
    hr = pLoadLibraryShim( szFusion, NULL, NULL, &hfusion );
    if (FAILED( hr ))
    {
        WARN("fusion.dll not available 0x%08x\n", hr);
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
    UINT    count;
    UINT    index;
    WCHAR **attrs;
};

static UINT get_assembly_name_attribute( MSIRECORD *rec, LPVOID param )
{
    static const WCHAR fmtW[] = {'%','s','=','"','%','s','"',0};
    static const WCHAR nameW[] = {'n','a','m','e',0};
    struct assembly_name *name = param;
    const WCHAR *attr = MSI_RecordGetString( rec, 2 );
    const WCHAR *value = MSI_RecordGetString( rec, 3 );
    int len = strlenW( fmtW ) + strlenW( attr ) + strlenW( value );

    if (!(name->attrs[name->index] = msi_alloc( len * sizeof(WCHAR) )))
        return ERROR_OUTOFMEMORY;

    if (!strcmpiW( attr, nameW )) strcpyW( name->attrs[name->index++], value );
    else sprintfW( name->attrs[name->index++], fmtW, attr, value );
    return ERROR_SUCCESS;
}

static WCHAR *get_assembly_display_name( MSIDATABASE *db, const WCHAR *comp, MSIASSEMBLY *assembly )
{
    static const WCHAR commaW[] = {',',0};
    static const WCHAR queryW[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
        '`','M','s','i','A','s','s','e','m','b','l','y','N','a','m','e','`',' ',
        'W','H','E','R','E',' ','`','C','o','m','p','o','n','e','n','t','_','`',
        ' ','=',' ','\'','%','s','\'',0};
    struct assembly_name name;
    WCHAR *display_name = NULL;
    MSIQUERY *view;
    UINT i, r;
    int len;

    r = MSI_OpenQuery( db, &view, queryW, comp );
    if (r != ERROR_SUCCESS)
        return NULL;

    name.count = 0;
    name.index = 0;
    name.attrs = NULL;
    MSI_IterateRecords( view, &name.count, NULL, NULL );
    if (!name.count) goto done;

    name.attrs = msi_alloc( name.count * sizeof(WCHAR *) );
    if (!name.attrs) goto done;

    MSI_IterateRecords( view, NULL, get_assembly_name_attribute, &name );

    len = 0;
    for (i = 0; i < name.count; i++) len += strlenW( name.attrs[i] ) + 1;

    display_name = msi_alloc( (len + 1) * sizeof(WCHAR) );
    if (display_name)
    {
        display_name[0] = 0;
        for (i = 0; i < name.count; i++)
        {
            strcatW( display_name, name.attrs[i] );
            if (i < name.count - 1) strcatW( display_name, commaW );
        }
    }

done:
    msiobj_release( &view->hdr );
    for (i = 0; i < name.count; i++) msi_free( name.attrs[i] );
    msi_free( name.attrs );
    return display_name;
}

static BOOL check_assembly_installed( MSIPACKAGE *package, MSIASSEMBLY *assembly )
{
    IAssemblyCache *cache;
    ASSEMBLY_INFO info;
    HRESULT hr;

    if (assembly->application)
    {
        FIXME("we should probably check the manifest file here\n");
        if (msi_get_property_int( package->db, szInstalled, 0 )) return TRUE;
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
    if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
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
        msi_free( a->feature );
        msi_free( a->manifest );
        msi_free( a->application );
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

static WCHAR *build_local_assembly_path( const WCHAR *filename )
{
    UINT i;
    WCHAR *ret;

    if (!(ret = msi_alloc( (strlenW( filename ) + 1) * sizeof(WCHAR) )))
        return NULL;

    for (i = 0; filename[i]; i++)
    {
        if (filename[i] == '\\' || filename[i] == '/') ret[i] = '|';
        else ret[i] = filename[i];
    }
    ret[i] = 0;
    return ret;
}

static LONG open_assemblies_key( UINT context, BOOL win32, HKEY *hkey )
{
    static const WCHAR path_win32[] =
        {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
          'I','n','s','t','a','l','l','e','r','\\','W','i','n','3','2','A','s','s','e','m','b','l','i','e','s','\\',0};
    static const WCHAR path_dotnet[] =
        {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
         'I','n','s','t','a','l','l','e','r','\\','A','s','s','e','m','b','l','i','e','s','\\',0};
    static const WCHAR classes_path_win32[] =
        {'I','n','s','t','a','l','l','e','r','\\','W','i','n','3','2','A','s','s','e','m','b','l','i','e','s','\\',0};
    static const WCHAR classes_path_dotnet[] =
        {'I','n','s','t','a','l','l','e','r','\\','A','s','s','e','m','b','l','i','e','s','\\',0};
    HKEY root;
    const WCHAR *path;

    if (context == MSIINSTALLCONTEXT_MACHINE)
    {
        root = HKEY_CLASSES_ROOT;
        if (win32) path = classes_path_win32;
        else path = classes_path_dotnet;
    }
    else
    {
        root = HKEY_CURRENT_USER;
        if (win32) path = path_win32;
        else path = path_dotnet;
    }
    return RegCreateKeyW( root, path, hkey );
}

static LONG open_local_assembly_key( UINT context, BOOL win32, const WCHAR *filename, HKEY *hkey )
{
    LONG res;
    HKEY root;
    WCHAR *path;

    if (!(path = build_local_assembly_path( filename )))
        return ERROR_OUTOFMEMORY;

    if ((res = open_assemblies_key( context, win32, &root )))
    {
        msi_free( path );
        return res;
    }
    res = RegCreateKeyW( root, path, hkey );
    RegCloseKey( root );
    msi_free( path );
    return res;
}

static LONG delete_local_assembly_key( UINT context, BOOL win32, const WCHAR *filename )
{
    LONG res;
    HKEY root;
    WCHAR *path;

    if (!(path = build_local_assembly_path( filename )))
        return ERROR_OUTOFMEMORY;

    if ((res = open_assemblies_key( context, win32, &root )))
    {
        msi_free( path );
        return res;
    }
    res = RegDeleteKeyW( root, path );
    RegCloseKey( root );
    msi_free( path );
    return res;
}

static LONG open_global_assembly_key( UINT context, BOOL win32, HKEY *hkey )
{
    static const WCHAR path_win32[] =
        {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
         'I','n','s','t','a','l','l','e','r','\\','W','i','n','3','2','A','s','s','e','m','b','l','i','e','s','\\',
         'G','l','o','b','a','l',0};
    static const WCHAR path_dotnet[] =
        {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
         'I','n','s','t','a','l','l','e','r','\\','A','s','s','e','m','b','l','i','e','s','\\',
         'G','l','o','b','a','l',0};
    static const WCHAR classes_path_win32[] =
        {'I','n','s','t','a','l','l','e','r','\\','W','i','n','3','2','A','s','s','e','m','b','l','i','e','s','\\',
         'G','l','o','b','a','l',0};
    static const WCHAR classes_path_dotnet[] =
        {'I','n','s','t','a','l','l','e','r','\\','A','s','s','e','m','b','l','i','e','s','\\','G','l','o','b','a','l',0};
    HKEY root;
    const WCHAR *path;

    if (context == MSIINSTALLCONTEXT_MACHINE)
    {
        root = HKEY_CLASSES_ROOT;
        if (win32) path = classes_path_win32;
        else path = classes_path_dotnet;
    }
    else
    {
        root = HKEY_CURRENT_USER;
        if (win32) path = path_win32;
        else path = path_dotnet;
    }
    return RegCreateKeyW( root, path, hkey );
}

UINT ACTION_MsiPublishAssemblies( MSIPACKAGE *package )
{
    MSICOMPONENT *comp;

    LIST_FOR_EACH_ENTRY(comp, &package->components, MSICOMPONENT, entry)
    {
        LONG res;
        HKEY hkey;
        GUID guid;
        DWORD size;
        WCHAR buffer[43];
        MSIRECORD *uirow;
        MSIASSEMBLY *assembly = comp->assembly;
        BOOL win32;

        if (!assembly || !comp->ComponentId) continue;

        if (!comp->Enabled)
        {
            TRACE("component is disabled: %s\n", debugstr_w(comp->Component));
            continue;
        }

        if (comp->ActionRequest != INSTALLSTATE_LOCAL)
        {
            TRACE("Component not scheduled for installation: %s\n", debugstr_w(comp->Component));
            comp->Action = comp->Installed;
            continue;
        }
        comp->Action = INSTALLSTATE_LOCAL;

        TRACE("publishing %s\n", debugstr_w(comp->Component));

        CLSIDFromString( package->ProductCode, &guid );
        encode_base85_guid( &guid, buffer );
        buffer[20] = '>';
        CLSIDFromString( comp->ComponentId, &guid );
        encode_base85_guid( &guid, buffer + 21 );
        buffer[42] = 0;

        win32 = assembly->attributes & msidbAssemblyAttributesWin32;
        if (assembly->application)
        {
            MSIFILE *file = get_loaded_file( package, assembly->application );
            if ((res = open_local_assembly_key( package->Context, win32, file->TargetPath, &hkey )))
            {
                WARN("failed to open local assembly key %d\n", res);
                return ERROR_FUNCTION_FAILED;
            }
        }
        else
        {
            if ((res = open_global_assembly_key( package->Context, win32, &hkey )))
            {
                WARN("failed to open global assembly key %d\n", res);
                return ERROR_FUNCTION_FAILED;
            }
        }
        size = sizeof(buffer);
        if ((res = RegSetValueExW( hkey, assembly->display_name, 0, REG_MULTI_SZ, (const BYTE *)buffer, size )))
        {
            WARN("failed to set assembly value %d\n", res);
        }
        RegCloseKey( hkey );

        uirow = MSI_CreateRecord( 2 );
        MSI_RecordSetStringW( uirow, 2, assembly->display_name );
        ui_actiondata( package, szMsiPublishAssemblies, uirow );
        msiobj_release( &uirow->hdr );
    }
    return ERROR_SUCCESS;
}

UINT ACTION_MsiUnpublishAssemblies( MSIPACKAGE *package )
{
    MSICOMPONENT *comp;

    LIST_FOR_EACH_ENTRY(comp, &package->components, MSICOMPONENT, entry)
    {
        LONG res;
        MSIRECORD *uirow;
        MSIASSEMBLY *assembly = comp->assembly;
        BOOL win32;

        if (!assembly || !comp->ComponentId) continue;

        if (!comp->Enabled)
        {
            TRACE("component is disabled: %s\n", debugstr_w(comp->Component));
            continue;
        }

        if (comp->ActionRequest != INSTALLSTATE_ABSENT)
        {
            TRACE("Component not scheduled for removal: %s\n", debugstr_w(comp->Component));
            comp->Action = comp->Installed;
            continue;
        }
        comp->Action = INSTALLSTATE_ABSENT;

        TRACE("unpublishing %s\n", debugstr_w(comp->Component));

        win32 = assembly->attributes & msidbAssemblyAttributesWin32;
        if (assembly->application)
        {
            MSIFILE *file = get_loaded_file( package, assembly->application );
            if ((res = delete_local_assembly_key( package->Context, win32, file->TargetPath )))
                WARN("failed to delete local assembly key %d\n", res);
        }
        else
        {
            HKEY hkey;
            if ((res = open_global_assembly_key( package->Context, win32, &hkey )))
                WARN("failed to delete global assembly key %d\n", res);
            else
            {
                if ((res = RegDeleteValueW( hkey, assembly->display_name )))
                    WARN("failed to delete global assembly value %d\n", res);
                RegCloseKey( hkey );
            }
        }

        uirow = MSI_CreateRecord( 2 );
        MSI_RecordSetStringW( uirow, 2, assembly->display_name );
        ui_actiondata( package, szMsiPublishAssemblies, uirow );
        msiobj_release( &uirow->hdr );
    }
    return ERROR_SUCCESS;
}
