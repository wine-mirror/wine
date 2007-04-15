/*
 * Custom Action processing for the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Aric Stewart for CodeWeavers
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

#define COBJMACROS

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "msidefs.h"
#include "msipriv.h"
#include "winuser.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

#define CUSTOM_ACTION_TYPE_MASK 0x3F
static const WCHAR c_collen[] = {'C',':','\\',0};
static const WCHAR cszTempFolder[]= {'T','e','m','p','F','o','l','d','e','r',0};

typedef struct tagMSIRUNNINGACTION
{
    struct list entry;
    HANDLE handle;
    BOOL   process;
    LPWSTR name;
} MSIRUNNINGACTION;

static UINT HANDLE_CustomType1(MSIPACKAGE *package, LPCWSTR source,
                               LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType2(MSIPACKAGE *package, LPCWSTR source,
                               LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType17(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType18(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType19(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType50(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType34(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);

typedef UINT (WINAPI *MsiCustomActionEntryPoint)( MSIHANDLE );

static CRITICAL_SECTION msi_custom_action_cs;
static CRITICAL_SECTION_DEBUG msi_custom_action_cs_debug =
{
    0, 0, &msi_custom_action_cs,
    { &msi_custom_action_cs_debug.ProcessLocksList,
      &msi_custom_action_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": msi_custom_action_cs") }
};
static CRITICAL_SECTION msi_custom_action_cs = { &msi_custom_action_cs_debug, -1, 0, 0, 0, 0 };

static struct list msi_pending_custom_actions = LIST_INIT( msi_pending_custom_actions );

static BOOL check_execution_scheduling_options(MSIPACKAGE *package, LPCWSTR action, UINT options)
{
    if (!package->script)
        return TRUE;

    if ((options & msidbCustomActionTypeClientRepeat) ==
            msidbCustomActionTypeClientRepeat)
    {
        if (!(package->script->InWhatSequence & SEQUENCE_UI &&
            package->script->InWhatSequence & SEQUENCE_EXEC))
        {
            TRACE("Skipping action due to dbCustomActionTypeClientRepeat option.\n");
            return FALSE;
        }
    }
    else if (options & msidbCustomActionTypeFirstSequence)
    {
        if (package->script->InWhatSequence & SEQUENCE_UI &&
            package->script->InWhatSequence & SEQUENCE_EXEC )
        {
            TRACE("Skipping action due to msidbCustomActionTypeFirstSequence option.\n");
            return FALSE;
        }
    }
    else if (options & msidbCustomActionTypeOncePerProcess)
    {
        if (check_unique_action(package,action))
        {
            TRACE("Skipping action due to msidbCustomActionTypeOncePerProcess option.\n");
            return FALSE;
        }
        else
            register_unique_action(package,action);
    }

    return TRUE;
}

/* stores the CustomActionData before the action:
 *     [CustomActionData]Action
 */
static LPWSTR msi_get_deferred_action(LPCWSTR action, LPWSTR actiondata)
{
    LPWSTR deferred;
    DWORD len;

    static const WCHAR begin[] = {'[',0};
    static const WCHAR end[] = {']',0};

    if (!actiondata)
        return strdupW(action);

    len = lstrlenW(action) + lstrlenW(actiondata) + 3;
    deferred = msi_alloc(len * sizeof(WCHAR));

    lstrcpyW(deferred, begin);
    lstrcatW(deferred, actiondata);
    lstrcatW(deferred, end);
    lstrcatW(deferred, action);

    return deferred;
}

UINT ACTION_CustomAction(MSIPACKAGE *package,LPCWSTR action, BOOL execute)
{
    UINT rc = ERROR_SUCCESS;
    MSIRECORD * row = 0;
    static const WCHAR ExecSeqQuery[] =
    {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
     '`','C','u','s','t','o' ,'m','A','c','t','i','o','n','`',
     ' ','W','H','E','R','E',' ','`','A','c','t','i' ,'o','n','`',' ',
     '=',' ','\'','%','s','\'',0};
    UINT type;
    LPCWSTR source, target;
    LPWSTR ptr, deferred_data = NULL;
    LPWSTR action_copy = strdupW(action);
    WCHAR *deformated=NULL;

    /* deferred action: [CustomActionData]Action */
    if ((ptr = strchrW(action_copy, ']')))
    {
        deferred_data = action_copy + 1;
        *ptr = '\0';
        action = ptr + 1;
    }

    row = MSI_QueryGetRecord( package->db, ExecSeqQuery, action );
    if (!row)
    {
        msi_free(action_copy);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    type = MSI_RecordGetInteger(row,2);

    source = MSI_RecordGetString(row,3);
    target = MSI_RecordGetString(row,4);

    TRACE("Handling custom action %s (%x %s %s)\n",debugstr_w(action),type,
          debugstr_w(source), debugstr_w(target));

    /* handle some of the deferred actions */
    if (type & msidbCustomActionTypeTSAware)
        FIXME("msidbCustomActionTypeTSAware not handled\n");

    if (type & msidbCustomActionTypeInScript)
    {
        if (type & msidbCustomActionTypeNoImpersonate)
            FIXME("msidbCustomActionTypeNoImpersonate not handled\n");

        if (type & msidbCustomActionTypeRollback)
        {
            FIXME("Rollback only action... rollbacks not supported yet\n");
            schedule_action(package, ROLLBACK_SCRIPT, action);
            rc = ERROR_SUCCESS;
            goto end;
        }
        if (!execute)
        {
            LPWSTR actiondata = msi_dup_property(package, action);
            LPWSTR deferred = msi_get_deferred_action(action, actiondata);

            if (type & msidbCustomActionTypeCommit)
            {
                TRACE("Deferring Commit Action!\n");
                schedule_action(package, COMMIT_SCRIPT, deferred);
            }
            else
            {
                TRACE("Deferring Action!\n");
                schedule_action(package, INSTALL_SCRIPT, deferred);
            }

            rc = ERROR_SUCCESS;
            msi_free(deferred);
            goto end;
        }
        else
        {
            /*Set ActionData*/

            static const WCHAR szActionData[] = {
            'C','u','s','t','o','m','A','c','t','i','o','n','D','a','t','a',0};
            static const WCHAR szBlank[] = {0};
            LPWSTR actiondata = msi_dup_property( package, action );
            if (deferred_data)
                MSI_SetPropertyW(package,szActionData,deferred_data);
            else if (actiondata)
                MSI_SetPropertyW(package,szActionData,actiondata);
            else
                MSI_SetPropertyW(package,szActionData,szBlank);
            msi_free(actiondata);
        }
    }
    else if (!check_execution_scheduling_options(package,action,type))
    {
        rc = ERROR_SUCCESS;
        goto end;
    }

    switch (type & CUSTOM_ACTION_TYPE_MASK)
    {
        case 1: /* DLL file stored in a Binary table stream */
            rc = HANDLE_CustomType1(package,source,target,type,action);
            break;
        case 2: /* EXE file stored in a Binary table stream */
            rc = HANDLE_CustomType2(package,source,target,type,action);
            break;
        case 18: /*EXE file installed with package */
            rc = HANDLE_CustomType18(package,source,target,type,action);
            break;
        case 19: /* Error that halts install */
            rc = HANDLE_CustomType19(package,source,target,type,action);
            break;
        case 17:
            rc = HANDLE_CustomType17(package,source,target,type,action);
            break;
        case 50: /*EXE file specified by a property value */
            rc = HANDLE_CustomType50(package,source,target,type,action);
            break;
        case 34: /*EXE to be run in specified directory */
            rc = HANDLE_CustomType34(package,source,target,type,action);
            break;
        case 35: /* Directory set with formatted text. */
            deformat_string(package,target,&deformated);
            MSI_SetTargetPathW(package, source, deformated);
            msi_free(deformated);
            break;
        case 51: /* Property set with formatted text. */
            deformat_string(package,target,&deformated);
            rc = MSI_SetPropertyW(package,source,deformated);
            msi_free(deformated);
            break;
        default:
            FIXME("UNHANDLED ACTION TYPE %i (%s %s)\n",
             type & CUSTOM_ACTION_TYPE_MASK, debugstr_w(source),
             debugstr_w(target));
    }

end:
    msi_free(action_copy);
    msiobj_release(&row->hdr);
    return rc;
}


static UINT store_binary_to_temp(MSIPACKAGE *package, LPCWSTR source,
                                LPWSTR tmp_file)
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
        '`','B','i' ,'n','a','r','y','`',' ','W','H','E','R','E',' ',
        '`','N','a','m','e','`',' ','=',' ','\'','%','s','\'',0};
    MSIRECORD *row = 0;
    HANDLE file;
    CHAR buffer[1024];
    static const WCHAR f1[] = {'m','s','i',0};
    WCHAR fmt[MAX_PATH];
    DWORD sz = MAX_PATH;
    UINT r;

    if (MSI_GetPropertyW(package, cszTempFolder, fmt, &sz) != ERROR_SUCCESS)
        GetTempPathW(MAX_PATH, fmt);

    if (GetTempFileNameW(fmt, f1, 0, tmp_file) == 0)
    {
        TRACE("Unable to create file\n");
        return ERROR_FUNCTION_FAILED;
    }
    track_tempfile(package, tmp_file);

    row = MSI_QueryGetRecord(package->db, query, source);
    if (!row)
        return ERROR_FUNCTION_FAILED;

    /* write out the file */
    file = CreateFileW(tmp_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        r = ERROR_FUNCTION_FAILED;
    else
    {
        do
        {
            DWORD write;
            sz = sizeof buffer;
            r = MSI_RecordReadStream(row, 2, buffer, &sz);
            if (r != ERROR_SUCCESS)
            {
                ERR("Failed to get stream\n");
                break;
            }
            WriteFile(file, buffer, sz, &write, NULL);
        } while (sz == sizeof buffer);
        CloseHandle(file);
    }

    msiobj_release(&row->hdr);

    return r;
}

static void file_running_action(MSIPACKAGE* package, HANDLE Handle,
                                BOOL process, LPCWSTR name)
{
    MSIRUNNINGACTION *action;

    action = msi_alloc( sizeof(MSIRUNNINGACTION) );

    action->handle = Handle;
    action->process = process;
    action->name = strdupW(name);

    list_add_tail( &package->RunningActions, &action->entry );
}

static UINT custom_get_process_return( HANDLE process )
{
    DWORD rc = 0;

    GetExitCodeProcess( process, &rc );
    if (rc != 0)
        return ERROR_FUNCTION_FAILED;
    return ERROR_SUCCESS;
}

static UINT custom_get_thread_return( HANDLE thread )
{
    DWORD rc = 0;

    GetExitCodeThread( thread, &rc );

    switch (rc)
    {
    case ERROR_FUNCTION_NOT_CALLED:
    case ERROR_SUCCESS:
    case ERROR_INSTALL_USEREXIT:
    case ERROR_INSTALL_FAILURE:
        return rc;
    case ERROR_NO_MORE_ITEMS:
        return ERROR_SUCCESS;
    default:
        ERR("Invalid Return Code %d\n",rc);
        return ERROR_INSTALL_FAILURE;
    }
}

static UINT wait_process_handle(MSIPACKAGE* package, UINT type,
                           HANDLE ProcessHandle, LPCWSTR name)
{
    UINT rc = ERROR_SUCCESS;

    if (!(type & msidbCustomActionTypeAsync))
    {
        TRACE("waiting for %s\n", debugstr_w(name));

        msi_dialog_check_messages(ProcessHandle);

        if (!(type & msidbCustomActionTypeContinue))
            rc = custom_get_process_return(ProcessHandle);

        CloseHandle(ProcessHandle);
    }
    else
    {
        TRACE("%s running in background\n", debugstr_w(name));

        if (!(type & msidbCustomActionTypeContinue))
            file_running_action(package, ProcessHandle, TRUE, name);
        else
            CloseHandle(ProcessHandle);
    }

    return rc;
}

typedef struct _msi_custom_action_info {
    struct list entry;
    MSIPACKAGE *package;
    LPWSTR source;
    LPWSTR target;
    HANDLE handle;
    LPWSTR action;
    INT type;
    GUID guid;
} msi_custom_action_info;

static void free_custom_action_data( msi_custom_action_info *info )
{
    EnterCriticalSection( &msi_custom_action_cs );
    list_remove( &info->entry );
    LeaveCriticalSection( &msi_custom_action_cs );
    if (info->handle)
        CloseHandle( info->handle );
    msi_free( info->action );
    msi_free( info->source );
    msi_free( info->target );
    msiobj_release( &info->package->hdr );
    msi_free( info );
}

static UINT wait_thread_handle( msi_custom_action_info *info )
{
    UINT rc = ERROR_SUCCESS;

    if (!(info->type & msidbCustomActionTypeAsync))
    {
        TRACE("waiting for %s\n", debugstr_w( info->action ));

        msi_dialog_check_messages( info->handle );

        if (!(info->type & msidbCustomActionTypeContinue))
            rc = custom_get_thread_return( info->handle );

        free_custom_action_data( info );
    }
    else
    {
        TRACE("%s running in background\n", debugstr_w( info->action ));
    }

    return rc;
}

static msi_custom_action_info *find_action_by_guid( const GUID *guid )
{
    msi_custom_action_info *info;
    BOOL found = FALSE;

    EnterCriticalSection( &msi_custom_action_cs );

    LIST_FOR_EACH_ENTRY( info, &msi_pending_custom_actions, msi_custom_action_info, entry )
    {
        if (IsEqualGUID( &info->guid, guid ))
        {
            found = TRUE;
            break;
        }
    }

    LeaveCriticalSection( &msi_custom_action_cs );

    if (!found)
        return NULL;

    return info;
}

static DWORD WINAPI ACTION_CallDllFunction( const GUID *guid )
{
    msi_custom_action_info *info;
    MsiCustomActionEntryPoint fn;
    MSIHANDLE hPackage;
    HANDLE hModule;
    LPSTR proc;
    UINT r = ERROR_FUNCTION_FAILED;

    info = find_action_by_guid( guid );
    if (!info)
    {
        ERR("failed to find action %s\n", debugstr_guid( guid) );
        return r;
    }

    TRACE("%s %s\n", debugstr_w( info->source ), debugstr_w( info->target ) );

    hModule = LoadLibraryW( info->source );
    if (!hModule)
    {
        ERR("failed to load dll %s\n", debugstr_w( info->source ) );
        return r;
    }

    proc = strdupWtoA( info->target );
    fn = (MsiCustomActionEntryPoint) GetProcAddress( hModule, proc );
    msi_free( proc );
    if (fn)
    {
        hPackage = alloc_msihandle( &info->package->hdr );
        if (hPackage)
        {
            TRACE("calling %s\n", debugstr_w( info->target ) );
            r = fn( hPackage );
            MsiCloseHandle( hPackage );
        }
        else
            ERR("failed to create handle for %p\n", info->package );
    }
    else
        ERR("GetProcAddress(%s) failed\n", debugstr_w( info->target ) );

    FreeLibrary(hModule);

    if (info->type & msidbCustomActionTypeAsync &&
        info->type & msidbCustomActionTypeContinue)
        free_custom_action_data( info );

    return r;
}

static DWORD WINAPI DllThread( LPVOID arg )
{
    LPGUID guid = arg;
    DWORD rc = 0;

    TRACE("custom action (%x) started\n", GetCurrentThreadId() );

    rc = ACTION_CallDllFunction( guid );

    TRACE("custom action (%x) returned %i\n", GetCurrentThreadId(), rc );

    MsiCloseAllHandles();
    return rc;
}

static msi_custom_action_info *do_msidbCustomActionTypeDll(
    MSIPACKAGE *package, INT type, LPCWSTR source, LPCWSTR target, LPCWSTR action )
{
    msi_custom_action_info *info;

    info = msi_alloc( sizeof *info );
    if (!info)
        return NULL;

    msiobj_addref( &package->hdr );
    info->package = package;
    info->type = type;
    info->target = strdupW( target );
    info->source = strdupW( source );
    info->action = strdupW( action );
    CoCreateGuid( &info->guid );

    EnterCriticalSection( &msi_custom_action_cs );
    list_add_tail( &msi_pending_custom_actions, &info->entry );
    LeaveCriticalSection( &msi_custom_action_cs );

    info->handle = CreateThread( NULL, 0, DllThread, &info->guid, 0, NULL );
    if (!info->handle)
    {
        free_custom_action_data( info );
        return NULL;
    }

    return info;
}

static UINT HANDLE_CustomType1(MSIPACKAGE *package, LPCWSTR source,
                               LPCWSTR target, const INT type, LPCWSTR action)
{
    msi_custom_action_info *info;
    WCHAR tmp_file[MAX_PATH];
    UINT r;

    r = store_binary_to_temp(package, source, tmp_file);
    if (r != ERROR_SUCCESS)
        return r;

    TRACE("Calling function %s from %s\n",debugstr_w(target),
          debugstr_w(tmp_file));

    if (!strchrW(tmp_file,'.'))
    {
        static const WCHAR dot[]={'.',0};
        strcatW(tmp_file,dot);
    }

    info = do_msidbCustomActionTypeDll( package, type, tmp_file, target, action );

    return wait_thread_handle( info );
}

static UINT HANDLE_CustomType2(MSIPACKAGE *package, LPCWSTR source,
                               LPCWSTR target, const INT type, LPCWSTR action)
{
    WCHAR tmp_file[MAX_PATH];
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    BOOL rc;
    INT len;
    WCHAR *deformated = NULL;
    WCHAR *cmd;
    static const WCHAR spc[] = {' ',0};
    UINT r;

    memset(&si,0,sizeof(STARTUPINFOW));

    r = store_binary_to_temp(package, source, tmp_file);
    if (r != ERROR_SUCCESS)
        return r;

    deformat_string(package,target,&deformated);

    len = strlenW(tmp_file)+2;

    if (deformated)
        len += strlenW(deformated);

    cmd = msi_alloc(sizeof(WCHAR)*len);

    strcpyW(cmd,tmp_file);
    if (deformated)
    {
        strcatW(cmd,spc);
        strcatW(cmd,deformated);

        msi_free(deformated);
    }

    TRACE("executing exe %s\n", debugstr_w(cmd));

    rc = CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL,
                  c_collen, &si, &info);
    msi_free(cmd);

    if ( !rc )
    {
        ERR("Unable to execute command %s\n", debugstr_w(cmd));
        return ERROR_SUCCESS;
    }
    CloseHandle( info.hThread );

    r = wait_process_handle(package, type, info.hProcess, action);

    return r;
}

static UINT HANDLE_CustomType17(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action)
{
    msi_custom_action_info *info;
    MSIFILE *file;

    TRACE("%s %s\n", debugstr_w(source), debugstr_w(target));

    file = get_loaded_file( package, source );
    if (!file)
    {
        ERR("invalid file key %s\n", debugstr_w( source ));
        return ERROR_FUNCTION_FAILED;
    }

    info = do_msidbCustomActionTypeDll( package, type, file->TargetPath, target, action );

    return wait_thread_handle( info );
}

static UINT HANDLE_CustomType18(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    BOOL rc;
    WCHAR *deformated;
    WCHAR *cmd;
    INT len;
    static const WCHAR spc[] = {' ',0};
    MSIFILE *file;

    memset(&si,0,sizeof(STARTUPINFOW));

    file = get_loaded_file(package,source);
    if( !file )
        return ERROR_FUNCTION_FAILED;

    len = lstrlenW( file->TargetPath );

    deformat_string(package,target,&deformated);
    if (deformated)
        len += strlenW(deformated);
    len += 2;

    cmd = msi_alloc(len * sizeof(WCHAR));

    lstrcpyW( cmd, file->TargetPath);
    if (deformated)
    {
        strcatW(cmd, spc);
        strcatW(cmd, deformated);

        msi_free(deformated);
    }

    TRACE("executing exe %s\n", debugstr_w(cmd));

    rc = CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL,
                  c_collen, &si, &info);

    if ( !rc )
    {
        ERR("Unable to execute command %s\n", debugstr_w(cmd));
        msi_free(cmd);
        return ERROR_SUCCESS;
    }
    msi_free(cmd);
    CloseHandle( info.hThread );

    return wait_process_handle(package, type, info.hProcess, action);
}

static UINT HANDLE_CustomType19(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action)
{
    static const WCHAR query[] = {
      'S','E','L','E','C','T',' ','`','M','e','s','s','a','g','e','`',' ',
      'F','R','O','M',' ','`','E','r','r','o','r','`',' ',
      'W','H','E','R','E',' ','`','E','r','r','o','r','`',' ','=',' ',
      '%','s',0
    };
    MSIRECORD *row = 0;
    LPWSTR deformated = NULL;

    deformat_string( package, target, &deformated );

    /* first try treat the error as a number */
    row = MSI_QueryGetRecord( package->db, query, deformated );
    if( row )
    {
        LPCWSTR error = MSI_RecordGetString( row, 1 );
        MessageBoxW( NULL, error, NULL, MB_OK );
        msiobj_release( &row->hdr );
    }
    else
        MessageBoxW( NULL, deformated, NULL, MB_OK );

    msi_free( deformated );

    return ERROR_FUNCTION_FAILED;
}

static UINT HANDLE_CustomType50(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    WCHAR *prop;
    BOOL rc;
    WCHAR *deformated;
    WCHAR *cmd;
    INT len;
    static const WCHAR spc[] = {' ',0};

    memset(&si,0,sizeof(STARTUPINFOW));
    memset(&info,0,sizeof(PROCESS_INFORMATION));

    prop = msi_dup_property( package, source );
    if (!prop)
        return ERROR_SUCCESS;

    deformat_string(package,target,&deformated);
    len = strlenW(prop) + 2;
    if (deformated)
         len += strlenW(deformated);

    cmd = msi_alloc(sizeof(WCHAR)*len);

    strcpyW(cmd,prop);
    if (deformated)
    {
        strcatW(cmd,spc);
        strcatW(cmd,deformated);

        msi_free(deformated);
    }
    msi_free(prop);

    TRACE("executing exe %s\n", debugstr_w(cmd));

    rc = CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL,
                  c_collen, &si, &info);

    if ( !rc )
    {
        ERR("Unable to execute command %s\n", debugstr_w(cmd));
        msi_free(cmd);
        return ERROR_SUCCESS;
    }
    msi_free(cmd);

    CloseHandle( info.hThread );

    return wait_process_handle(package, type, info.hProcess, action);
}

static UINT HANDLE_CustomType34(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action)
{
    LPWSTR filename, deformated;
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    BOOL rc;

    memset(&si,0,sizeof(STARTUPINFOW));

    filename = resolve_folder(package, source, FALSE, FALSE, TRUE, NULL);

    if (!filename)
        return ERROR_FUNCTION_FAILED;

    SetCurrentDirectoryW(filename);
    msi_free(filename);

    deformat_string(package,target,&deformated);

    if (!deformated)
        return ERROR_FUNCTION_FAILED;

    TRACE("executing exe %s\n", debugstr_w(deformated));

    rc = CreateProcessW(NULL, deformated, NULL, NULL, FALSE, 0, NULL,
                  c_collen, &si, &info);

    if ( !rc )
    {
        ERR("Unable to execute command %s\n", debugstr_w(deformated));
        msi_free(deformated);
        return ERROR_SUCCESS;
    }
    msi_free(deformated);
    CloseHandle( info.hThread );

    return wait_process_handle(package, type, info.hProcess, action);
}

void ACTION_FinishCustomActions(MSIPACKAGE* package)
{
    struct list *item;
    HANDLE *wait_handles;
    unsigned int handle_count, i;
    msi_custom_action_info *info, *cursor;

    while ((item = list_head( &package->RunningActions )))
    {
        MSIRUNNINGACTION *action = LIST_ENTRY( item, MSIRUNNINGACTION, entry );

        list_remove( &action->entry );

        TRACE("waiting for %s\n", debugstr_w( action->name ) );
        msi_dialog_check_messages( action->handle );

        CloseHandle( action->handle );
        msi_free( action->name );
        msi_free( action );
    }

    EnterCriticalSection( &msi_custom_action_cs );

    handle_count = list_count( &msi_pending_custom_actions );
    wait_handles = HeapAlloc( GetProcessHeap(), 0, handle_count * sizeof(HANDLE) );

    handle_count = 0;
    LIST_FOR_EACH_ENTRY_SAFE( info, cursor, &msi_pending_custom_actions, msi_custom_action_info, entry )
    {
        if (info->package == package )
        {
            if (DuplicateHandle(GetCurrentProcess(), info->handle, GetCurrentProcess(), &wait_handles[handle_count], SYNCHRONIZE, FALSE, 0))
                handle_count++;
            free_custom_action_data( info );
        }
    }

    LeaveCriticalSection( &msi_custom_action_cs );

    for (i = 0; i < handle_count; i++)
    {
        msi_dialog_check_messages( wait_handles[i] );
        CloseHandle( wait_handles[i] );
    }

    HeapFree( GetProcessHeap(), 0, wait_handles );
}
