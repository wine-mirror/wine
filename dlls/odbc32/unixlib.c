/*
 * Win32 ODBC functions
 *
 * Copyright 1999 Xiang Li, Corel Corporation
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
 *
 * NOTES:
 *   Proxy ODBC driver manager.  This manager delegates all ODBC
 *   calls to a real ODBC driver manager named by the environment
 *   variable LIB_ODBC_DRIVER_MANAGER, or to libodbc.so if the
 *   variable is not set.
 */

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dlfcn.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/debug.h"
#include "sql.h"
#include "sqltypes.h"
#include "sqlext.h"

#include "unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(odbc);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static void *libodbc;

static SQLRETURN (*pSQLAllocConnect)(SQLHENV,SQLHDBC*);
static SQLRETURN (*pSQLAllocEnv)(SQLHENV*);
static SQLRETURN (*pSQLAllocHandle)(SQLSMALLINT,SQLHANDLE,SQLHANDLE*);
static SQLRETURN (*pSQLAllocHandleStd)(SQLSMALLINT,SQLHANDLE,SQLHANDLE*);
static SQLRETURN (*pSQLAllocStmt)(SQLHDBC,SQLHSTMT*);
static SQLRETURN (*pSQLBindCol)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLPOINTER,SQLLEN,SQLLEN*);
static SQLRETURN (*pSQLBindParam)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLULEN,SQLSMALLINT,SQLPOINTER,SQLLEN*);
static SQLRETURN (*pSQLBindParameter)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLULEN,SQLSMALLINT,SQLPOINTER,SQLLEN,SQLLEN*);
static SQLRETURN (*pSQLBrowseConnect)(SQLHDBC,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLBrowseConnectW)(SQLHDBC,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLBulkOperations)(SQLHSTMT,SQLSMALLINT);
static SQLRETURN (*pSQLCancel)(SQLHSTMT);
static SQLRETURN (*pSQLCloseCursor)(SQLHSTMT);
static SQLRETURN (*pSQLColAttribute)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLLEN*);
static SQLRETURN (*pSQLColAttributeW)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLLEN*);
static SQLRETURN (*pSQLColAttributes)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLLEN*);
static SQLRETURN (*pSQLColAttributesW)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLLEN*);
static SQLRETURN (*pSQLColumnPrivileges)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLColumnPrivilegesW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLColumns)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLColumnsW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLConnect)(SQLHDBC,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLConnectW)(SQLHDBC,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLCopyDesc)(SQLHDESC,SQLHDESC);
static SQLRETURN (*pSQLDataSources)(SQLHENV,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLDataSourcesW)(SQLHENV,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLDescribeCol)(SQLHSTMT,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLULEN*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLDescribeColW)(SQLHSTMT,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLULEN*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLDescribeParam)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT*,SQLULEN*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLDisconnect)(SQLHDBC);
static SQLRETURN (*pSQLDriverConnect)(SQLHDBC,SQLHWND,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLUSMALLINT);
static SQLRETURN (*pSQLDriverConnectW)(SQLHDBC,SQLHWND,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLUSMALLINT);
static SQLRETURN (*pSQLDrivers)(SQLHENV,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLDriversW)(SQLHENV,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLEndTran)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT);
static SQLRETURN (*pSQLError)(SQLHENV,SQLHDBC,SQLHSTMT,SQLCHAR*,SQLINTEGER*,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLErrorW)(SQLHENV,SQLHDBC,SQLHSTMT,SQLWCHAR*,SQLINTEGER*,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLExecDirect)(SQLHSTMT,SQLCHAR*,SQLINTEGER);
static SQLRETURN (*pSQLExecDirectW)(SQLHSTMT,SQLWCHAR*,SQLINTEGER);
static SQLRETURN (*pSQLExecute)(SQLHSTMT);
static SQLRETURN (*pSQLExtendedFetch)(SQLHSTMT,SQLUSMALLINT,SQLLEN,SQLULEN*,SQLUSMALLINT*);
static SQLRETURN (*pSQLFetch)(SQLHSTMT);
static SQLRETURN (*pSQLFetchScroll)(SQLHSTMT,SQLSMALLINT,SQLLEN);
static SQLRETURN (*pSQLForeignKeys)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLForeignKeysW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLFreeConnect)(SQLHDBC);
static SQLRETURN (*pSQLFreeEnv)(SQLHENV);
static SQLRETURN (*pSQLFreeHandle)(SQLSMALLINT,SQLHANDLE);
static SQLRETURN (*pSQLFreeStmt)(SQLHSTMT,SQLUSMALLINT);
static SQLRETURN (*pSQLGetConnectAttr)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetConnectAttrW)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetConnectOption)(SQLHDBC,SQLUSMALLINT,SQLPOINTER);
static SQLRETURN (*pSQLGetConnectOptionW)(SQLHDBC,SQLUSMALLINT,SQLPOINTER);
static SQLRETURN (*pSQLGetCursorName)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetCursorNameW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetData)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLPOINTER,SQLLEN,SQLLEN*);
static SQLRETURN (*pSQLGetDescField)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetDescFieldW)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetDescRec)(SQLHDESC,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*,SQLLEN*,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDescRecW)(SQLHDESC,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*,SQLLEN*,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDiagField)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDiagFieldW)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDiagRec)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLCHAR*,SQLINTEGER*,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDiagRecW)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLWCHAR*,SQLINTEGER*,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetEnvAttr)(SQLHENV,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetFunctions)(SQLHDBC,SQLUSMALLINT,SQLUSMALLINT*);
static SQLRETURN (*pSQLGetInfo)(SQLHDBC,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetInfoW)(SQLHDBC,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetStmtAttr)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetStmtAttrW)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetStmtOption)(SQLHSTMT,SQLUSMALLINT,SQLPOINTER);
static SQLRETURN (*pSQLGetTypeInfo)(SQLHSTMT,SQLSMALLINT);
static SQLRETURN (*pSQLGetTypeInfoW)(SQLHSTMT,SQLSMALLINT);
static SQLRETURN (*pSQLMoreResults)(SQLHSTMT);
static SQLRETURN (*pSQLNativeSql)(SQLHDBC,SQLCHAR*,SQLINTEGER,SQLCHAR*,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLNativeSqlW)(SQLHDBC,SQLWCHAR*,SQLINTEGER,SQLWCHAR*,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLNumParams)(SQLHSTMT,SQLSMALLINT*);
static SQLRETURN (*pSQLNumResultCols)(SQLHSTMT,SQLSMALLINT*);
static SQLRETURN (*pSQLParamData)(SQLHSTMT,SQLPOINTER*);
static SQLRETURN (*pSQLParamOptions)(SQLHSTMT,SQLULEN,SQLULEN*);
static SQLRETURN (*pSQLPrepare)(SQLHSTMT,SQLCHAR*,SQLINTEGER);
static SQLRETURN (*pSQLPrepareW)(SQLHSTMT,SQLWCHAR*,SQLINTEGER);
static SQLRETURN (*pSQLPrimaryKeys)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLPrimaryKeysW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLProcedureColumns)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLProcedureColumnsW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLProcedures)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLProceduresW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLPutData)(SQLHSTMT,SQLPOINTER,SQLLEN);
static SQLRETURN (*pSQLRowCount)(SQLHSTMT,SQLLEN*);
static SQLRETURN (*pSQLSetConnectAttr)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetConnectAttrW)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetConnectOption)(SQLHDBC,SQLUSMALLINT,SQLULEN);
static SQLRETURN (*pSQLSetConnectOptionW)(SQLHDBC,SQLUSMALLINT,SQLULEN);
static SQLRETURN (*pSQLSetCursorName)(SQLHSTMT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLSetCursorNameW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLSetDescField)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetDescFieldW)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetDescRec)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLLEN,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLLEN*,SQLLEN*);
static SQLRETURN (*pSQLSetEnvAttr)(SQLHENV,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetParam)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLULEN,SQLSMALLINT,SQLPOINTER,SQLLEN*);
static SQLRETURN (*pSQLSetPos)(SQLHSTMT,SQLSETPOSIROW,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLSetScrollOptions)(SQLHSTMT,SQLUSMALLINT,SQLLEN,SQLUSMALLINT);
static SQLRETURN (*pSQLSetStmtAttr)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetStmtAttrW)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetStmtOption)(SQLHSTMT,SQLUSMALLINT,SQLULEN);
static SQLRETURN (*pSQLSpecialColumns)(SQLHSTMT,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLSpecialColumnsW)(SQLHSTMT,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLStatistics)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLStatisticsW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLTablePrivileges)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLTablePrivilegesW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLTables)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLTablesW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLTransact)(SQLHENV,SQLHDBC,SQLUSMALLINT);

static inline void init_unicode_string( UNICODE_STRING *str, const WCHAR *data, ULONG data_size )
{
    str->Length = str->MaximumLength = data_size;
    str->Buffer = (WCHAR *)data;
}

static inline void ascii_to_unicode( WCHAR *dst, const char *src, size_t len )
{
    while (len--) *dst++ = (unsigned char)*src++;
}

static HANDLE create_hkcu_key( const WCHAR *path, ULONG path_size )
{
    NTSTATUS status;
    char buffer[512 + ARRAY_SIZE("\\Registry\\User\\")];
    WCHAR bufferW[512 + ARRAY_SIZE("\\Registry\\User\\")];
    DWORD_PTR sid_data[(sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE) / sizeof(DWORD_PTR)];
    DWORD i, len = sizeof(sid_data);
    SID *sid;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    HANDLE ret;

    status = NtQueryInformationToken( GetCurrentThreadEffectiveToken(), TokenUser, sid_data, len, &len );
    if (status) return NULL;

    sid = ((TOKEN_USER *)sid_data)->User.Sid;
    len = snprintf( buffer, sizeof(buffer), "\\Registry\\User\\S-%u-%u", sid->Revision,
                   (int)MAKELONG( MAKEWORD( sid->IdentifierAuthority.Value[5], sid->IdentifierAuthority.Value[4] ),
                                  MAKEWORD( sid->IdentifierAuthority.Value[3], sid->IdentifierAuthority.Value[2] )));
    for (i = 0; i < sid->SubAuthorityCount; i++)
        len += snprintf( buffer + len, sizeof(buffer) - len, "-%u", (int)sid->SubAuthority[i] );
    buffer[len++] = '\\';

    ascii_to_unicode( bufferW, buffer, len );
    memcpy( bufferW + len, path, path_size );
    init_unicode_string( &str, bufferW, len * sizeof(WCHAR) + path_size );
    InitializeObjectAttributes( &attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL );
    if (!NtCreateKey( &ret, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL )) return ret;
    return NULL;
}

static HANDLE create_hklm_key( const WCHAR *path, ULONG path_size )
{
    static const WCHAR machineW[] = {'\\','R','e','g','i','s','t','r','y','\\','M','a','c','h','i','n','e','\\'};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    WCHAR bufferW[256 + ARRAY_SIZE(machineW)];
    HANDLE ret;

    memcpy( bufferW, machineW, sizeof(machineW) );
    memcpy( bufferW + ARRAY_SIZE(machineW), path, path_size );
    init_unicode_string( &str, bufferW, sizeof(machineW) + path_size );
    InitializeObjectAttributes( &attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL );
    if (!NtCreateKey( &ret, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL )) return ret;
    return NULL;
}

static HANDLE create_key( HANDLE root, const WCHAR *path, ULONG path_size, ULONG options, ULONG *disposition )
{
    UNICODE_STRING name = { path_size, path_size, (WCHAR *)path };
    OBJECT_ATTRIBUTES attr;
    HANDLE ret;

    attr.Length = sizeof(attr);
    attr.RootDirectory = root;
    attr.ObjectName = &name;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    if (NtCreateKey( &ret, MAXIMUM_ALLOWED, &attr, 0, NULL, options, disposition )) return NULL;
    return ret;
}

static ULONG query_value( HANDLE key, const WCHAR *name, ULONG name_size, KEY_VALUE_PARTIAL_INFORMATION *info,
                          ULONG size )
{
    UNICODE_STRING str = { name_size, name_size, (WCHAR *)name };
    if (NtQueryValueKey( key, &str, KeyValuePartialInformation, info, size, &size )) return 0;
    return size - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
}

static BOOL set_value( HANDLE key, const WCHAR *name, ULONG name_size, ULONG type, const void *value, ULONG count )
{
    UNICODE_STRING str = { name_size, name_size, (WCHAR *)name };
    return !NtSetValueKey( key, &str, 0, type, value, count );
}

/***********************************************************************
 * odbc_replicate_odbcinst_to_registry
 *
 * Utility to odbc_replicate_odbcinst_to_registry() to replicate the drivers of the
 * ODBCINST.INI settings
 *
 * The driver settings are not replicated to the registry.  If we were to
 * replicate them we would need to decide whether to replicate all settings
 * or to do some translation; whether to remove any entries present only in
 * the windows registry, etc.
 */
static void replicate_odbcinst_to_registry( SQLHENV env )
{
    static const WCHAR odbcW[] = {'S','o','f','t','w','a','r','e','\\','O','D','B','C'};
    static const WCHAR odbcinstW[] = {'O','D','B','C','I','N','S','T','.','I','N','I'};
    static const WCHAR driversW[] = {'O','D','B','C',' ','D','r','i','v','e','r','s'};
    HANDLE key_odbc, key_odbcinst, key_drivers;
    BOOL success = FALSE;

    if (!(key_odbc = create_hklm_key( odbcW, sizeof(odbcW) ))) return;

    if ((key_odbcinst = create_key( key_odbc, odbcinstW, sizeof(odbcinstW), 0, NULL )))
    {
        if ((key_drivers = create_key( key_odbcinst, driversW, sizeof(driversW), 0, NULL )))
        {
            SQLRETURN ret;
            SQLUSMALLINT dir = SQL_FETCH_FIRST;
            WCHAR desc [256];
            SQLSMALLINT len;

            success = TRUE;
            while (SUCCESS((ret = pSQLDriversW( env, dir, (SQLWCHAR *)desc, sizeof(desc), &len, NULL, 0, NULL ))))
            {
                dir = SQL_FETCH_NEXT;
                if (len == lstrlenW( desc ))
                {
                    static const WCHAR installedW[] = {'I','n','s','t','a','l','l','e','d',0};
                    HANDLE key_driver;
                    WCHAR buffer[256];
                    KEY_VALUE_PARTIAL_INFORMATION *info = (void *)buffer;

                    if (!query_value( key_drivers, desc, len * sizeof(WCHAR), info, sizeof(buffer) ))
                    {
                        if (!set_value( key_drivers, desc, len * sizeof(WCHAR), REG_SZ, (const BYTE *)installedW,
                                        sizeof(installedW) ))
                        {
                            TRACE( "error replicating driver %s\n", debugstr_w(desc) );
                            success = FALSE;
                        }
                    }
                    if ((key_driver = create_key( key_odbcinst, desc, lstrlenW( desc ) * sizeof(WCHAR), 0, NULL )))
                        NtClose( key_driver );
                    else
                    {
                        TRACE( "error ensuring driver key %s\n", debugstr_w(desc) );
                        success = FALSE;
                    }
                }
                else
                {
                    WARN( "unusually long driver name %s not replicated\n", debugstr_w(desc) );
                    success = FALSE;
                }
            }
            NtClose( key_drivers );
        }
        else TRACE( "error opening Drivers key\n" );

        NtClose( key_odbcinst );
    }
    else TRACE( "error creating/opening ODBCINST.INI key\n" );

    if (!success) WARN( "may not have replicated all ODBC drivers to the registry\n" );
    NtClose( key_odbc );
}

/***********************************************************************
 * replicate_odbc_to_registry
 *
 * Utility to replicate_to_registry() to replicate either the USER or
 * SYSTEM data sources.
 *
 * For now simply place the "Driver description" (as returned by SQLDataSources)
 * into the registry as the driver.  This is enough to satisfy Crystal's
 * requirement that there be a driver entry.  (It doesn't seem to care what
 * the setting is).
 * A slightly more accurate setting would be to access the registry to find
 * the actual driver library for the given description (which appears to map
 * to one of the HKLM/Software/ODBC/ODBCINST.INI keys).  (If you do this note
 * that this will add a requirement that this function be called after
 * replicate_odbcinst_to_registry())
 */
static void replicate_odbc_to_registry( BOOL is_user, SQLHENV env )
{
    static const WCHAR odbcW[] = {'S','o','f','t','w','a','r','e','\\','O','D','B','C'};
    static const WCHAR odbciniW[] = {'O','D','B','C','.','I','N','I'};
    HANDLE key_odbc, key_odbcini, key_source;
    SQLRETURN ret;
    SQLUSMALLINT dir;
    WCHAR dsn[SQL_MAX_DSN_LENGTH + 1], desc[256];
    SQLSMALLINT len_dsn, len_desc;
    BOOL success = FALSE;
    const char *which;

    if (is_user)
    {
        key_odbc = create_hkcu_key( odbcW, sizeof(odbcW) );
        which = "user";
    }
    else
    {
        key_odbc = create_hklm_key( odbcW, sizeof(odbcW) );
        which = "system";
    }
    if (!key_odbc) return;

    if ((key_odbcini = create_key( key_odbc, odbciniW, sizeof(odbciniW), 0, NULL )))
    {
        success = TRUE;
        dir = is_user ? SQL_FETCH_FIRST_USER : SQL_FETCH_FIRST_SYSTEM;
        while (SUCCESS((ret = pSQLDataSourcesW( env, dir, (SQLWCHAR *)dsn, sizeof(dsn), &len_dsn, (SQLWCHAR *)desc,
                                                sizeof(desc), &len_desc ))))
        {
            dir = SQL_FETCH_NEXT;
            if (len_dsn == lstrlenW( dsn ) && len_desc == lstrlenW( desc ))
            {
                if ((key_source = create_key( key_odbcini, dsn, len_dsn * sizeof(WCHAR), 0, NULL )))
                {
                    static const WCHAR driverW[] = {'D','r','i','v','e','r'};
                    WCHAR buffer[256];
                    KEY_VALUE_PARTIAL_INFORMATION *info = (void *)buffer;
                    ULONG size;

                    if (!(size = query_value( key_source, driverW, sizeof(driverW), info, sizeof(buffer) )))
                    {
                        if (!set_value( key_source, driverW, sizeof(driverW), REG_SZ, (const BYTE *)desc,
                                        len_desc * sizeof(WCHAR) ))
                        {
                            TRACE( "error replicating description of %s (%s)\n", debugstr_w(dsn), debugstr_w(desc) );
                            success = FALSE;
                        }
                    }
                    NtClose( key_source );
                }
                else
                {
                    TRACE( "error opening %s DSN key %s\n", which, debugstr_w(dsn) );
                    success = FALSE;
                }
            }
            else
            {
                WARN( "unusually long %s data source name %s (%s) not replicated\n", which, debugstr_w(dsn), debugstr_w(desc) );
                success = FALSE;
            }
        }
        NtClose( key_odbcini );
    }
    else TRACE( "error creating/opening %s ODBC.INI registry key\n", which );

    if (!success) WARN( "may not have replicated all %s ODBC DSNs to the registry\n", which );
    NtClose( key_odbc );
}

/***********************************************************************
 * replicate_to_registry
 *
 * Unfortunately some of the functions that Windows documents as being part
 * of the ODBC API it implements directly during compilation or something
 * in terms of registry access functions.
 * e.g. SQLGetInstalledDrivers queries the list at
 * HKEY_LOCAL_MACHINE\Software\ODBC\ODBCINST.INI\ODBC Drivers
 *
 * This function is called when the driver manager is loaded and is used
 * to replicate the appropriate details into the Wine registry
 */
static void replicate_to_registry(void)
{
    SQLHENV env;
    SQLRETURN ret;

    if (!(ret = pSQLAllocEnv( &env )))
    {
        replicate_odbcinst_to_registry( env );
        replicate_odbc_to_registry( FALSE /* system dsn */, env );
        replicate_odbc_to_registry( TRUE /* user dsn */, env );
        pSQLFreeEnv( env );
    }
    else
    {
        TRACE( "error %d opening an SQL environment\n", (int)ret );
        WARN( "external ODBC settings have not been replicated to the Wine registry\n" );
    }
}

static NTSTATUS load_odbc(void);

static NTSTATUS odbc_process_attach( void *args )
{
    if (load_odbc()) return STATUS_DLL_NOT_FOUND;
    replicate_to_registry();
    return STATUS_SUCCESS;
}

static NTSTATUS odbc_process_detach( void *args )
{
    if (libodbc) dlclose( libodbc );
    libodbc = NULL;
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_SQLAllocConnect( void *args )
{
    struct SQLAllocConnect_params *params = args;

    if (!pSQLAllocConnect) return SQL_ERROR;
    return pSQLAllocConnect( (SQLHENV)(ULONG_PTR)params->EnvironmentHandle, (SQLHDBC *)&params->ConnectionHandle );
}

static NTSTATUS wrap_SQLAllocEnv( void *args )
{
    struct SQLAllocEnv_params *params = args;

    if (!pSQLAllocEnv) return SQL_ERROR;
    return pSQLAllocEnv( (SQLHENV *)&params->EnvironmentHandle );
}

static NTSTATUS wrap_SQLAllocHandle( void *args )
{
    struct SQLAllocHandle_params *params = args;

    if (!pSQLAllocHandle) return SQL_ERROR;
    return pSQLAllocHandle( params->HandleType, (SQLHANDLE)(ULONG_PTR)params->InputHandle,
                            (SQLHANDLE *)&params->OutputHandle );
}

static NTSTATUS wrap_SQLAllocHandleStd( void *args )
{
    struct SQLAllocHandleStd_params *params = args;

    if (!pSQLAllocHandleStd) return SQL_ERROR;
    return pSQLAllocHandleStd( params->HandleType, (SQLHANDLE)(ULONG_PTR)params->InputHandle,
                               (SQLHANDLE *)&params->OutputHandle );
}

static NTSTATUS wrap_SQLAllocStmt( void *args )
{
    struct SQLAllocStmt_params *params = args;

    if (!pSQLAllocStmt) return SQL_ERROR;
    return pSQLAllocStmt( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, (SQLHSTMT *)&params->StatementHandle );
}

static NTSTATUS wrap_SQLBindCol( void *args )
{
    struct SQLBindCol_params *params = args;

    if (!pSQLBindCol) return SQL_ERROR;
    return pSQLBindCol( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnNumber, params->TargetType,
                        params->TargetValue, params->BufferLength, (SQLLEN *)(ULONG_PTR)params->StrLen_or_Ind );
}

static NTSTATUS wrap_SQLBindParam( void *args )
{
    struct SQLBindParam_params *params = args;

    if (!pSQLBindParam) return SQL_ERROR;
    return pSQLBindParam( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ParameterNumber, params->ValueType,
                          params->ParameterType, params->LengthPrecision, params->ParameterScale,
                          params->ParameterValue, (SQLLEN *)(ULONG_PTR)params->StrLen_or_Ind );
}

static NTSTATUS wrap_SQLBindParameter( void *args )
{
    struct SQLBindParameter_params *params = args;

    if (!pSQLBindParameter) return SQL_ERROR;
    return pSQLBindParameter( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ParameterNumber,
                              params->InputOutputType, params->ValueType, params->ParameterType, params->ColumnSize,
                              params->DecimalDigits, params->ParameterValue, params->BufferLength,
                              (SQLLEN *)(ULONG_PTR)params->StrLen_or_Ind );
}

static NTSTATUS wrap_SQLBrowseConnect( void *args )
{
    struct SQLBrowseConnect_params *params = args;

    if (!pSQLBrowseConnect) return SQL_ERROR;
    return pSQLBrowseConnect( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->InConnectionString,
                              params->StringLength1, params->OutConnectionString, params->BufferLength,
                              params->StringLength2 );
}

static NTSTATUS wrap_SQLBrowseConnectW( void *args )
{
    struct SQLBrowseConnectW_params *params = args;

    if (!pSQLBrowseConnectW) return SQL_ERROR;
    return pSQLBrowseConnectW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->InConnectionString,
                               params->StringLength1, params->OutConnectionString, params->BufferLength,
                               params->StringLength2 );
}

static NTSTATUS wrap_SQLBulkOperations( void *args )
{
    struct SQLBulkOperations_params *params = args;

    if (!pSQLBulkOperations) return SQL_ERROR;
    return pSQLBulkOperations( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Operation );
}

static NTSTATUS wrap_SQLCancel( void *args )
{
    struct SQLCancel_params *params = args;

    if (!pSQLCancel) return SQL_ERROR;
    return pSQLCancel( (SQLHSTMT)(ULONG_PTR)params->StatementHandle );
}

static NTSTATUS wrap_SQLCloseCursor( void *args )
{
    struct SQLCloseCursor_params *params = args;

    if (!pSQLCloseCursor) return SQL_ERROR;
    return pSQLCloseCursor( (SQLHSTMT)(ULONG_PTR)params->StatementHandle );
}

static NTSTATUS wrap_SQLColAttribute( void *args )
{
    struct SQLColAttribute_params *params = args;

    if (!pSQLColAttribute) return SQL_ERROR;
    return pSQLColAttribute( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnNumber,
                             params->FieldIdentifier, params->CharacterAttribute, params->BufferLength,
                             params->StringLength, (SQLLEN *)(ULONG_PTR)params->NumericAttribute );
}

static NTSTATUS wrap_SQLColAttributeW( void *args )
{
    struct SQLColAttributeW_params *params = args;

    if (!pSQLColAttributeW) return SQL_ERROR;
    return pSQLColAttributeW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnNumber,
                              params->FieldIdentifier, params->CharacterAttribute,
                              params->BufferLength, params->StringLength,
                              (SQLLEN *)(ULONG_PTR)params->NumericAttribute );
}

static NTSTATUS wrap_SQLColAttributes( void *args )
{
    struct SQLColAttributes_params *params = args;

    if (!pSQLColAttributes) return SQL_ERROR;
    return pSQLColAttributes( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnNumber,
                              params->FieldIdentifier, params->CharacterAttributes, params->BufferLength,
                              params->StringLength, (SQLLEN *)(ULONG_PTR)params->NumericAttributes );
}

static NTSTATUS wrap_SQLColAttributesW( void *args )
{
    struct SQLColAttributesW_params *params = args;

    if (!pSQLColAttributesW) return SQL_ERROR;
    return pSQLColAttributesW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnNumber,
                               params->FieldIdentifier, params->CharacterAttributes, params->BufferLength,
                               params->StringLength, (SQLLEN *)(ULONG_PTR)params->NumericAttributes );
}

static NTSTATUS wrap_SQLColumnPrivileges( void *args )
{
    struct SQLColumnPrivileges_params *params = args;

    if (!pSQLColumnPrivileges) return SQL_ERROR;
    return pSQLColumnPrivileges( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName,
                                 params->NameLength1, params->SchemaName, params->NameLength2,
                                 params->TableName, params->NameLength3, params->ColumnName, params->NameLength4 );
}

static NTSTATUS wrap_SQLColumnPrivilegesW( void *args )
{
    struct SQLColumnPrivilegesW_params *params = args;

    if (!pSQLColumnPrivilegesW) return SQL_ERROR;
    return pSQLColumnPrivilegesW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName,
                                  params->NameLength1, params->SchemaName, params->NameLength2,
                                  params->TableName, params->NameLength3, params->ColumnName, params->NameLength4 );
}

static NTSTATUS wrap_SQLColumns( void *args )
{
    struct SQLColumns_params *params = args;

    if (!pSQLColumns) return SQL_ERROR;
    return pSQLColumns( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                        params->SchemaName, params->NameLength2, params->TableName, params->NameLength3,
                        params->ColumnName, params->NameLength4 );
}

static NTSTATUS wrap_SQLColumnsW( void *args )
{
    struct SQLColumnsW_params *params = args;

    if (!pSQLColumnsW) return SQL_ERROR;
    return pSQLColumnsW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                         params->SchemaName, params->NameLength2, params->TableName, params->NameLength3,
                         params->ColumnName, params->NameLength4 );
}

static NTSTATUS wrap_SQLConnect( void *args )
{
    struct SQLConnect_params *params = args;

    if (!pSQLConnect) return SQL_ERROR;
    return pSQLConnect( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->ServerName, params->NameLength1,
                        params->UserName, params->NameLength2, params->Authentication, params->NameLength3 );
}

static NTSTATUS wrap_SQLConnectW( void *args )
{
    struct SQLConnectW_params *params = args;

    if (!pSQLConnectW) return SQL_ERROR;
    return pSQLConnectW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->ServerName, params->NameLength1,
                         params->UserName, params->NameLength2, params->Authentication, params->NameLength3 );
}

static NTSTATUS wrap_SQLCopyDesc( void *args )
{
    struct SQLCopyDesc_params *params = args;

    if (!pSQLCopyDesc) return SQL_ERROR;
    return pSQLCopyDesc( (SQLHDESC)(ULONG_PTR)params->SourceDescHandle, (SQLHDESC)(ULONG_PTR)params->TargetDescHandle );
}

static NTSTATUS wrap_SQLDataSources( void *args )
{
    struct SQLDataSources_params *params = args;

    if (!pSQLDataSources) return SQL_ERROR;
    return pSQLDataSources( (SQLHENV)(ULONG_PTR)params->EnvironmentHandle, params->Direction, params->ServerName,
                            params->BufferLength1, params->NameLength1, params->Description,
                            params->BufferLength2, params->NameLength2 );
}

static NTSTATUS wrap_SQLDataSourcesW( void *args )
{
    struct SQLDataSourcesW_params *params = args;

    if (!pSQLDataSourcesW) return SQL_ERROR;
    return pSQLDataSourcesW( (SQLHENV)(ULONG_PTR)params->EnvironmentHandle, params->Direction, params->ServerName,
                             params->BufferLength1, params->NameLength1, params->Description,
                             params->BufferLength2, params->NameLength2 );
}

static NTSTATUS wrap_SQLDescribeCol( void *args )
{
    struct SQLDescribeCol_params *params = args;

    if (!pSQLDescribeCol) return SQL_ERROR;
    return pSQLDescribeCol( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnNumber, params->ColumnName,
                            params->BufferLength, params->NameLength, params->DataType,
                            (SQLULEN *)(ULONG_PTR)params->ColumnSize, params->DecimalDigits, params->Nullable );
}

static NTSTATUS wrap_SQLDescribeColW( void *args )
{
    struct SQLDescribeColW_params *params = args;

    if (!pSQLDescribeColW) return SQL_ERROR;
    return pSQLDescribeColW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnNumber, params->ColumnName,
                             params->BufferLength, params->NameLength, params->DataType,
                             (SQLULEN *)(ULONG_PTR)params->ColumnSize, params->DecimalDigits, params->Nullable );
}

static NTSTATUS wrap_SQLDescribeParam( void *args )
{
    struct SQLDescribeParam_params *params = args;

    if (!pSQLDescribeParam) return SQL_ERROR;
    return pSQLDescribeParam( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ParameterNumber, params->DataType,
                              (SQLULEN *)(ULONG_PTR)params->ParameterSize, params->DecimalDigits, params->Nullable );
}

static NTSTATUS wrap_SQLDisconnect( void *args )
{
    struct SQLDisconnect_params *params = args;

    if (!pSQLDisconnect) return SQL_ERROR;
    return pSQLDisconnect( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle );
}

static NTSTATUS wrap_SQLDriverConnect( void *args )
{
    struct SQLDriverConnect_params *params = args;

    if (!pSQLDriverConnect) return SQL_ERROR;
    return pSQLDriverConnect( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, (SQLHWND)(ULONG_PTR)params->WindowHandle,
                              params->ConnectionString, params->Length, params->OutConnectionString,
                              params->BufferLength, params->Length2, params->DriverCompletion );
}

static NTSTATUS wrap_SQLDriverConnectW( void *args )
{
    struct SQLDriverConnectW_params *params = args;

    if (!pSQLDriverConnectW) return SQL_ERROR;
    return pSQLDriverConnectW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, (SQLHWND)(ULONG_PTR)params->WindowHandle,
                               params->InConnectionString, params->Length, params->OutConnectionString,
                               params->BufferLength, params->Length2, params->DriverCompletion );
}

static NTSTATUS wrap_SQLDrivers( void *args )
{
    struct SQLDrivers_params *params = args;

    if (!pSQLDrivers) return SQL_ERROR;
    return pSQLDrivers( (SQLHENV)(ULONG_PTR)params->EnvironmentHandle, params->Direction, params->DriverDescription,
                        params->BufferLength1, params->DescriptionLength, params->DriverAttributes,
                        params->BufferLength2, params->AttributesLength );
}

static NTSTATUS wrap_SQLDriversW( void *args )
{
    struct SQLDriversW_params *params = args;

    if (!pSQLDriversW) return SQL_ERROR;
    return pSQLDriversW( (SQLHENV)(ULONG_PTR)params->EnvironmentHandle, params->Direction, params->DriverDescription,
                         params->BufferLength1, params->DescriptionLength, params->DriverAttributes,
                         params->BufferLength2, params->AttributesLength );
}

static NTSTATUS wrap_SQLEndTran( void *args )
{
    struct SQLEndTran_params *params = args;

    if (!pSQLEndTran) return SQL_ERROR;
    return pSQLEndTran( params->HandleType, (SQLHANDLE)(ULONG_PTR)params->Handle, params->CompletionType );
}

static NTSTATUS wrap_SQLError( void *args )
{
    struct SQLError_params *params = args;

    if (!pSQLError) return SQL_ERROR;
    return pSQLError( (SQLHENV)(ULONG_PTR)params->EnvironmentHandle, (SQLHDBC)(ULONG_PTR)params->ConnectionHandle,
                      (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->SqlState, params->NativeError,
                      params->MessageText, params->BufferLength, params->TextLength );
}

static NTSTATUS wrap_SQLErrorW( void *args )
{
    struct SQLErrorW_params *params = args;

    if (!pSQLErrorW) return SQL_ERROR;
    return pSQLErrorW( (SQLHENV)(ULONG_PTR)params->EnvironmentHandle, (SQLHDBC)(ULONG_PTR)params->ConnectionHandle,
                       (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->SqlState, params->NativeError,
                       params->MessageText, params->BufferLength, params->TextLength );
}

static NTSTATUS wrap_SQLExecDirect( void *args )
{
    struct SQLExecDirect_params *params = args;

    if (!pSQLExecDirect) return SQL_ERROR;
    return pSQLExecDirect( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->StatementText, params->TextLength );
}

static NTSTATUS wrap_SQLExecDirectW( void *args )
{
    struct SQLExecDirectW_params *params = args;

    if (!pSQLExecDirectW) return SQL_ERROR;
    return pSQLExecDirectW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->StatementText, params->TextLength );
}

static NTSTATUS wrap_SQLExecute( void *args )
{
    struct SQLExecute_params *params = args;

    if (!pSQLExecute) return SQL_ERROR;
    return pSQLExecute( (SQLHSTMT)(ULONG_PTR)params->StatementHandle );
}

static NTSTATUS wrap_SQLExtendedFetch( void *args )
{
    struct SQLExtendedFetch_params *params = args;

    if (!pSQLExtendedFetch) return SQL_ERROR;
    return pSQLExtendedFetch( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->FetchOrientation,
                              params->FetchOffset, (SQLULEN *)(ULONG_PTR)params->RowCount, params->RowStatusArray );
}

static NTSTATUS wrap_SQLFetch( void *args )
{
    struct SQLFetch_params *params = args;

    if (!pSQLFetch) return SQL_ERROR;
    return pSQLFetch( (SQLHSTMT)(ULONG_PTR)params->StatementHandle );
}

static NTSTATUS wrap_SQLFetchScroll( void *args )
{
    struct SQLFetchScroll_params *params = args;

    if (!pSQLFetchScroll) return SQL_ERROR;
    return pSQLFetchScroll( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->FetchOrientation,
                            params->FetchOffset );
}

static NTSTATUS wrap_SQLForeignKeys( void *args )
{
    struct SQLForeignKeys_params *params = args;

    if (!pSQLForeignKeys) return SQL_ERROR;
    return pSQLForeignKeys( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->PkCatalogName,
                            params->NameLength1, params->PkSchemaName, params->NameLength2,
                            params->PkTableName, params->NameLength3, params->FkCatalogName,
                            params->NameLength4, params->FkSchemaName, params->NameLength5,
                            params->FkTableName, params->NameLength6 );
}

static NTSTATUS wrap_SQLForeignKeysW( void *args )
{
    struct SQLForeignKeysW_params *params = args;

    if (!pSQLForeignKeysW) return SQL_ERROR;
    return pSQLForeignKeysW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->PkCatalogName,
                             params->NameLength1, params->PkSchemaName, params->NameLength2,
                             params->PkTableName, params->NameLength3, params->FkCatalogName,
                             params->NameLength4, params->FkSchemaName, params->NameLength5,
                             params->FkTableName, params->NameLength6 );
}

static NTSTATUS wrap_SQLFreeConnect( void *args )
{
    struct SQLFreeConnect_params *params = args;

    if (!pSQLFreeConnect) return SQL_ERROR;
    return pSQLFreeConnect( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle );
}

static NTSTATUS wrap_SQLFreeEnv( void *args )
{
    struct SQLFreeEnv_params *params = args;

    if (!pSQLFreeEnv) return SQL_ERROR;
    return pSQLFreeEnv( (SQLHENV)(ULONG_PTR)params->EnvironmentHandle );
}

static NTSTATUS wrap_SQLFreeHandle( void *args )
{
    struct SQLFreeHandle_params *params = args;

    if (!pSQLFreeHandle) return SQL_ERROR;
    return pSQLFreeHandle( params->HandleType, (SQLHANDLE)(ULONG_PTR)params->Handle );
}

static NTSTATUS wrap_SQLFreeStmt( void *args )
{
    struct SQLFreeStmt_params *params = args;

    if (!pSQLFreeStmt) return SQL_ERROR;
    return pSQLFreeStmt( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Option );
}

static NTSTATUS wrap_SQLGetConnectAttr( void *args )
{
    struct SQLGetConnectAttr_params *params = args;

    if (!pSQLGetConnectAttr) return SQL_ERROR;
    return pSQLGetConnectAttr( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->Attribute, params->Value,
                               params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetConnectAttrW( void *args )
{
    struct SQLGetConnectAttrW_params *params = args;

    if (!pSQLGetConnectAttrW) return SQL_ERROR;
    return pSQLGetConnectAttrW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->Attribute, params->Value,
                                params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetConnectOption( void *args )
{
    struct SQLGetConnectOption_params *params = args;

    if (!pSQLGetConnectOption) return SQL_ERROR;
    return pSQLGetConnectOption( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->Option, params->Value );
}

static NTSTATUS wrap_SQLGetConnectOptionW( void *args )
{
    struct SQLGetConnectOptionW_params *params = args;

    if (!pSQLGetConnectOptionW) return SQL_ERROR;
    return pSQLGetConnectOptionW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->Option, params->Value );
}

static NTSTATUS wrap_SQLGetCursorName( void *args )
{
    struct SQLGetCursorName_params *params = args;

    if (!pSQLGetCursorName) return SQL_ERROR;
    return pSQLGetCursorName( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CursorName, params->BufferLength,
                              params->NameLength );
}

static NTSTATUS wrap_SQLGetCursorNameW( void *args )
{
    struct SQLGetCursorNameW_params *params = args;

    if (!pSQLGetCursorNameW) return SQL_ERROR;
    return pSQLGetCursorNameW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CursorName, params->BufferLength,
                               params->NameLength );
}

static NTSTATUS wrap_SQLGetData( void *args )
{
    struct SQLGetData_params *params = args;

    if (!pSQLGetData) return SQL_ERROR;
    return pSQLGetData( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnNumber, params->TargetType,
                        params->TargetValue, params->BufferLength, (SQLLEN *)(ULONG_PTR)params->StrLen_or_Ind );
}

static NTSTATUS wrap_SQLGetDescField( void *args )
{
    struct SQLGetDescField_params *params = args;

    if (!pSQLGetDescField) return SQL_ERROR;
    return pSQLGetDescField( (SQLHDESC)(ULONG_PTR)params->DescriptorHandle, params->RecNumber, params->FieldIdentifier,
                             params->Value, params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetDescFieldW( void *args )
{
    struct SQLGetDescFieldW_params *params = args;

    if (!pSQLGetDescFieldW) return SQL_ERROR;
    return pSQLGetDescFieldW( (SQLHDESC)(ULONG_PTR)params->DescriptorHandle, params->RecNumber, params->FieldIdentifier,
                              params->Value, params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetDescRec( void *args )
{
    struct SQLGetDescRec_params *params = args;

    if (!pSQLGetDescRec) return SQL_ERROR;
    return pSQLGetDescRec( (SQLHDESC)(ULONG_PTR)params->DescriptorHandle, params->RecNumber, params->Name,
                           params->BufferLength, params->StringLength, params->Type, params->SubType,
                           (SQLLEN *)(ULONG_PTR)params->Length, params->Precision, params->Scale, params->Nullable );
}

static NTSTATUS wrap_SQLGetDescRecW( void *args )
{
    struct SQLGetDescRecW_params *params = args;

    if (!pSQLGetDescRecW) return SQL_ERROR;
    return pSQLGetDescRecW( (SQLHDESC)(ULONG_PTR)params->DescriptorHandle, params->RecNumber, params->Name,
                            params->BufferLength, params->StringLength, params->Type, params->SubType,
                            (SQLLEN *)(ULONG_PTR)params->Length, params->Precision, params->Scale, params->Nullable );
}

static NTSTATUS wrap_SQLGetDiagField( void *args )
{
    struct SQLGetDiagField_params *params = args;

    if (!pSQLGetDiagField) return SQL_ERROR;
    return pSQLGetDiagField( params->HandleType, (SQLHANDLE)(ULONG_PTR)params->Handle, params->RecNumber,
                             params->DiagIdentifier, params->DiagInfo, params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetDiagFieldW( void *args )
{
    struct SQLGetDiagFieldW_params *params = args;

    if (!pSQLGetDiagFieldW) return SQL_ERROR;
    return pSQLGetDiagFieldW( params->HandleType, (SQLHANDLE)(ULONG_PTR)params->Handle, params->RecNumber,
                              params->DiagIdentifier, params->DiagInfo, params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetDiagRec( void *args )
{
    struct SQLGetDiagRec_params *params = args;

    if (!pSQLGetDiagRec) return SQL_ERROR;
    return pSQLGetDiagRec( params->HandleType, (SQLHANDLE)(ULONG_PTR)params->Handle, params->RecNumber, params->SqlState,
                           params->NativeError, params->MessageText, params->BufferLength, params->TextLength );
}

static NTSTATUS wrap_SQLGetDiagRecW( void *args )
{
    struct SQLGetDiagRecW_params *params = args;

    if (!pSQLGetDiagRecW) return SQL_ERROR;
    return pSQLGetDiagRecW( params->HandleType, (SQLHANDLE)(ULONG_PTR)params->Handle, params->RecNumber, params->SqlState,
                            params->NativeError, params->MessageText, params->BufferLength, params->TextLength );
}

static NTSTATUS wrap_SQLGetEnvAttr( void *args )
{
    struct SQLGetEnvAttr_params *params = args;

    if (!pSQLGetEnvAttr) return SQL_ERROR;
    return pSQLGetEnvAttr( (SQLHENV)(ULONG_PTR)params->EnvironmentHandle, params->Attribute,
                           params->Value, params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetFunctions( void *args )
{
    struct SQLGetFunctions_params *params = args;

    if (!pSQLGetFunctions) return SQL_ERROR;
    return pSQLGetFunctions( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->FunctionId, params->Supported );
}

static NTSTATUS wrap_SQLGetInfo( void *args )
{
    struct SQLGetInfo_params *params = args;

    if (!pSQLGetInfo) return SQL_ERROR;
    return pSQLGetInfo( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->InfoType, params->InfoValue,
                        params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetInfoW( void *args )
{
    struct SQLGetInfoW_params *params = args;

    if (!pSQLGetInfoW) return SQL_ERROR;
    return pSQLGetInfoW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->InfoType, params->InfoValue,
                         params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetStmtAttr( void *args )
{
    struct SQLGetStmtAttr_params *params = args;

    if (!pSQLGetStmtAttr) return SQL_ERROR;
    return pSQLGetStmtAttr( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Attribute, params->Value,
                            params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetStmtAttrW( void *args )
{
    struct SQLGetStmtAttrW_params *params = args;

    if (!pSQLGetStmtAttrW) return SQL_ERROR;
    return pSQLGetStmtAttrW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Attribute, params->Value,
                             params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetStmtOption( void *args )
{
    struct SQLGetStmtOption_params *params = args;

    if (!pSQLGetStmtOption) return SQL_ERROR;
    return pSQLGetStmtOption( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Option, params->Value );
}

static NTSTATUS wrap_SQLGetTypeInfo( void *args )
{
    struct SQLGetTypeInfo_params *params = args;

    if (!pSQLGetTypeInfo) return SQL_ERROR;
    return pSQLGetTypeInfo( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->DataType );
}

static NTSTATUS wrap_SQLGetTypeInfoW( void *args )
{
    struct SQLGetTypeInfoW_params *params = args;

    if (!pSQLGetTypeInfoW) return SQL_ERROR;
    return pSQLGetTypeInfoW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->DataType );
}

static NTSTATUS wrap_SQLMoreResults( void *args )
{
    struct SQLMoreResults_params *params = args;

    if (!pSQLMoreResults) return SQL_ERROR;
    return pSQLMoreResults( (SQLHSTMT)(ULONG_PTR)params->StatementHandle );
}

static NTSTATUS wrap_SQLNativeSql( void *args )
{
    struct SQLNativeSql_params *params = args;

    if (!pSQLNativeSql) return SQL_ERROR;
    return pSQLNativeSql( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->InStatementText, params->TextLength1,
                          params->OutStatementText, params->BufferLength, params->TextLength2 );
}

static NTSTATUS wrap_SQLNativeSqlW( void *args )
{
    struct SQLNativeSqlW_params *params = args;

    if (!pSQLNativeSqlW) return SQL_ERROR;
    return pSQLNativeSqlW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->InStatementText, params->TextLength1,
                           params->OutStatementText, params->BufferLength, params->TextLength2 );
}

static NTSTATUS wrap_SQLNumParams( void *args )
{
    struct SQLNumParams_params *params = args;

    if (!pSQLNumParams) return SQL_ERROR;
    return pSQLNumParams( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ParameterCount );
}

static NTSTATUS wrap_SQLNumResultCols( void *args )
{
    struct SQLNumResultCols_params *params = args;

    if (!pSQLNumResultCols) return SQL_ERROR;
    return pSQLNumResultCols( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnCount );
}

static NTSTATUS wrap_SQLParamData( void *args )
{
    struct SQLParamData_params *params = args;

    if (!pSQLParamData) return SQL_ERROR;
    return pSQLParamData( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Value );
}

static NTSTATUS wrap_SQLParamOptions( void *args )
{
    struct SQLParamOptions_params *params = args;

    if (!pSQLParamOptions) return SQL_ERROR;
    return pSQLParamOptions( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->RowCount,
                             (SQLULEN *)(ULONG_PTR)params->RowNumber );
}

static NTSTATUS wrap_SQLPrepare( void *args )
{
    struct SQLPrepare_params *params = args;

    if (!pSQLPrepare) return SQL_ERROR;
    return pSQLPrepare( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->StatementText, params->TextLength );
}

static NTSTATUS wrap_SQLPrepareW( void *args )
{
    struct SQLPrepareW_params *params = args;

    if (!pSQLPrepareW) return SQL_ERROR;
    return pSQLPrepareW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->StatementText, params->TextLength );
}

static NTSTATUS wrap_SQLPrimaryKeys( void *args )
{
    struct SQLPrimaryKeys_params *params = args;

    if (!pSQLPrimaryKeys) return SQL_ERROR;
    return pSQLPrimaryKeys( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                            params->SchemaName, params->NameLength2, params->TableName, params->NameLength3 );
}

static NTSTATUS wrap_SQLPrimaryKeysW( void *args )
{
    struct SQLPrimaryKeysW_params *params = args;

    if (!pSQLPrimaryKeysW) return SQL_ERROR;
    return pSQLPrimaryKeysW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                             params->SchemaName, params->NameLength2, params->TableName, params->NameLength3 );
}

static NTSTATUS wrap_SQLProcedureColumns( void *args )
{
    struct SQLProcedureColumns_params *params = args;

    if (!pSQLProcedureColumns) return SQL_ERROR;
    return pSQLProcedureColumns( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName,
                                 params->NameLength1, params->SchemaName, params->NameLength2, params->ProcName,
                                 params->NameLength3, params->ColumnName, params->NameLength4 );
}

static NTSTATUS wrap_SQLProcedureColumnsW( void *args )
{
    struct SQLProcedureColumnsW_params *params = args;

    if (!pSQLProcedureColumnsW) return SQL_ERROR;
    return pSQLProcedureColumnsW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName,
                                  params->NameLength1, params->SchemaName, params->NameLength2, params->ProcName,
                                  params->NameLength3, params->ColumnName, params->NameLength4 );
}

static NTSTATUS wrap_SQLProcedures( void *args )
{
    struct SQLProcedures_params *params = args;

    if (!pSQLProcedures) return SQL_ERROR;
    return pSQLProcedures( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                           params->SchemaName, params->NameLength2, params->ProcName, params->NameLength3 );
}

static NTSTATUS wrap_SQLProceduresW( void *args )
{
    struct SQLProceduresW_params *params = args;

    if (!pSQLProceduresW) return SQL_ERROR;
    return pSQLProceduresW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                            params->SchemaName, params->NameLength2, params->ProcName, params->NameLength3 );
}

static NTSTATUS wrap_SQLPutData( void *args )
{
    struct SQLPutData_params *params = args;

    if (!pSQLPutData) return SQL_ERROR;
    return pSQLPutData( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Data, params->StrLen_or_Ind );
}

static NTSTATUS wrap_SQLRowCount( void *args )
{
    struct SQLRowCount_params *params = args;

    if (!pSQLRowCount) return SQL_ERROR;
    return pSQLRowCount( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, (SQLLEN *)(ULONG_PTR)params->RowCount );
}

static NTSTATUS wrap_SQLSetConnectAttr( void *args )
{
    struct SQLSetConnectAttr_params *params = args;

    if (!pSQLSetConnectAttr) return SQL_ERROR;
    return pSQLSetConnectAttr( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->Attribute, params->Value,
                               params->StringLength );
}

static NTSTATUS wrap_SQLSetConnectAttrW( void *args )
{
    struct SQLSetConnectAttrW_params *params = args;

    if (!pSQLSetConnectAttrW) return SQL_ERROR;
    return pSQLSetConnectAttrW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->Attribute, params->Value,
                                params->StringLength );
}

static NTSTATUS wrap_SQLSetConnectOption( void *args )
{
    struct SQLSetConnectOption_params *params = args;

    if (!pSQLSetConnectOption) return SQL_ERROR;
    return pSQLSetConnectOption( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->Option, params->Value );
}

static NTSTATUS wrap_SQLSetConnectOptionW( void *args )
{
    struct SQLSetConnectOptionW_params *params = args;

    if (!pSQLSetConnectOptionW) return SQL_ERROR;
    return pSQLSetConnectOptionW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->Option, params->Value );
}

static NTSTATUS wrap_SQLSetCursorName( void *args )
{
    struct SQLSetCursorName_params *params = args;

    if (!pSQLSetCursorName) return SQL_ERROR;
    return pSQLSetCursorName( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CursorName, params->NameLength );
}

static NTSTATUS wrap_SQLSetCursorNameW( void *args )
{
    struct SQLSetCursorNameW_params *params = args;

    if (!pSQLSetCursorNameW) return SQL_ERROR;
    return pSQLSetCursorNameW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CursorName, params->NameLength );
}

static NTSTATUS wrap_SQLSetDescField( void *args )
{
    struct SQLSetDescField_params *params = args;

    if (!pSQLSetDescField) return SQL_ERROR;
    return pSQLSetDescField( (SQLHDESC)(ULONG_PTR)params->DescriptorHandle, params->RecNumber, params->FieldIdentifier,
                             params->Value, params->BufferLength );
}

static NTSTATUS wrap_SQLSetDescFieldW( void *args )
{
    struct SQLSetDescFieldW_params *params = args;

    if (!pSQLSetDescFieldW) return SQL_ERROR;
    return pSQLSetDescFieldW( (SQLHDESC)(ULONG_PTR)params->DescriptorHandle, params->RecNumber, params->FieldIdentifier,
                              params->Value, params->BufferLength );
}

static NTSTATUS wrap_SQLSetDescRec( void *args )
{
    struct SQLSetDescRec_params *params = args;

    if (!pSQLSetDescRec) return SQL_ERROR;
    return pSQLSetDescRec( (SQLHDESC)(ULONG_PTR)params->DescriptorHandle, params->RecNumber, params->Type,
                           params->SubType, params->Length, params->Precision, params->Scale,
                           params->Data, (SQLLEN *)(ULONG_PTR)params->StringLength,
                           (SQLLEN *)(ULONG_PTR)params->Indicator );
}

static NTSTATUS wrap_SQLSetEnvAttr( void *args )
{
    struct SQLSetEnvAttr_params *params = args;

    if (!pSQLSetEnvAttr) return SQL_ERROR;
    return pSQLSetEnvAttr( (SQLHENV)(ULONG_PTR)params->EnvironmentHandle, params->Attribute, params->Value,
                           params->StringLength );
}

static NTSTATUS wrap_SQLSetParam( void *args )
{
    struct SQLSetParam_params *params = args;

    if (!pSQLSetParam) return SQL_ERROR;
    return pSQLSetParam( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ParameterNumber, params->ValueType,
                         params->ParameterType, params->LengthPrecision, params->ParameterScale,
                         params->ParameterValue, (SQLLEN *)(ULONG_PTR)params->StrLen_or_Ind );
}

static NTSTATUS wrap_SQLSetPos( void *args )
{
    struct SQLSetPos_params *params = args;

    if (!pSQLSetPos) return SQL_ERROR;
    return pSQLSetPos( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->RowNumber, params->Operation,
                       params->LockType );
}

static NTSTATUS wrap_SQLSetScrollOptions( void *args )
{
    struct SQLSetScrollOptions_params *params = args;

    if (!pSQLSetScrollOptions) return SQL_ERROR;
    return pSQLSetScrollOptions( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Concurrency,
                                 params->KeySetSize, params->RowSetSize );
}

static NTSTATUS wrap_SQLSetStmtAttr( void *args )
{
    struct SQLSetStmtAttr_params *params = args;

    if (!pSQLSetStmtAttr) return SQL_ERROR;
    return pSQLSetStmtAttr( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Attribute, params->Value,
                            params->StringLength );
}

static NTSTATUS wrap_SQLSetStmtAttrW( void *args )
{
    struct SQLSetStmtAttrW_params *params = args;

    if (!pSQLSetStmtAttrW) return SQL_ERROR;
    return pSQLSetStmtAttrW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Attribute, params->Value,
                             params->StringLength );
}

static NTSTATUS wrap_SQLSetStmtOption( void *args )
{
    struct SQLSetStmtOption_params *params = args;

    if (!pSQLSetStmtOption) return SQL_ERROR;
    return pSQLSetStmtOption( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Option, params->Value );
}

static NTSTATUS wrap_SQLSpecialColumns( void *args )
{
    struct SQLSpecialColumns_params *params = args;

    if (!pSQLSpecialColumns) return SQL_ERROR;
    return pSQLSpecialColumns( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->IdentifierType,
                               params->CatalogName, params->NameLength1, params->SchemaName, params->NameLength2,
                               params->TableName, params->NameLength3, params->Scope, params->Nullable );
}

static NTSTATUS wrap_SQLSpecialColumnsW( void *args )
{
    struct SQLSpecialColumnsW_params *params = args;

    if (!pSQLSpecialColumnsW) return SQL_ERROR;
    return pSQLSpecialColumnsW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->IdentifierType,
                                params->CatalogName, params->NameLength1, params->SchemaName, params->NameLength2,
                                params->TableName, params->NameLength3, params->Scope, params->Nullable );
}

static NTSTATUS wrap_SQLStatistics( void *args )
{
    struct SQLStatistics_params *params = args;

    if (!pSQLStatistics) return SQL_ERROR;
    return pSQLStatistics( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                           params->SchemaName, params->NameLength2, params->TableName,
                           params->NameLength3, params->Unique, params->Reserved );
}

static NTSTATUS wrap_SQLStatisticsW( void *args )
{
    struct SQLStatisticsW_params *params = args;

    if (!pSQLStatisticsW) return SQL_ERROR;
    return pSQLStatisticsW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                            params->SchemaName, params->NameLength2, params->TableName,
                            params->NameLength3, params->Unique, params->Reserved );
}

static NTSTATUS wrap_SQLTablePrivileges( void *args )
{
    struct SQLTablePrivileges_params *params = args;

    if (!pSQLTablePrivileges) return SQL_ERROR;
    return pSQLTablePrivileges( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName,
                                params->NameLength1, params->SchemaName, params->NameLength2, params->TableName,
                                params->NameLength3 );
}

static NTSTATUS wrap_SQLTablePrivilegesW( void *args )
{
    struct SQLTablePrivilegesW_params *params = args;

    if (!pSQLTablePrivilegesW) return SQL_ERROR;
    return pSQLTablePrivilegesW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName,
                                 params->NameLength1, params->SchemaName, params->NameLength2, params->TableName,
                                 params->NameLength3 );
}

static NTSTATUS wrap_SQLTables( void *args )
{
    struct SQLTables_params *params = args;

    if (!pSQLTables) return SQL_ERROR;
    return pSQLTables( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                       params->SchemaName, params->NameLength2, params->TableName,
                       params->NameLength3, params->TableType, params->NameLength4 );
}

static NTSTATUS wrap_SQLTablesW( void *args )
{
    struct SQLTablesW_params *params = args;

    if (!pSQLTablesW) return SQL_ERROR;
    return pSQLTablesW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                        params->SchemaName, params->NameLength2, params->TableName,
                        params->NameLength3, params->TableType, params->NameLength4 );
}

static NTSTATUS wrap_SQLTransact( void *args )
{
    struct SQLTransact_params *params = args;

    if (!pSQLTransact) return SQL_ERROR;
    return pSQLTransact( (SQLHENV)(ULONG_PTR)params->EnvironmentHandle, (SQLHDBC)(ULONG_PTR)params->ConnectionHandle,
                         params->CompletionType );
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    odbc_process_attach,
    odbc_process_detach,
    wrap_SQLAllocConnect,
    wrap_SQLAllocEnv,
    wrap_SQLAllocHandle,
    wrap_SQLAllocHandleStd,
    wrap_SQLAllocStmt,
    wrap_SQLBindCol,
    wrap_SQLBindParam,
    wrap_SQLBindParameter,
    wrap_SQLBrowseConnect,
    wrap_SQLBrowseConnectW,
    wrap_SQLBulkOperations,
    wrap_SQLCancel,
    wrap_SQLCloseCursor,
    wrap_SQLColAttribute,
    wrap_SQLColAttributeW,
    wrap_SQLColAttributes,
    wrap_SQLColAttributesW,
    wrap_SQLColumnPrivileges,
    wrap_SQLColumnPrivilegesW,
    wrap_SQLColumns,
    wrap_SQLColumnsW,
    wrap_SQLConnect,
    wrap_SQLConnectW,
    wrap_SQLCopyDesc,
    wrap_SQLDataSources,
    wrap_SQLDataSourcesW,
    wrap_SQLDescribeCol,
    wrap_SQLDescribeColW,
    wrap_SQLDescribeParam,
    wrap_SQLDisconnect,
    wrap_SQLDriverConnect,
    wrap_SQLDriverConnectW,
    wrap_SQLDrivers,
    wrap_SQLDriversW,
    wrap_SQLEndTran,
    wrap_SQLError,
    wrap_SQLErrorW,
    wrap_SQLExecDirect,
    wrap_SQLExecDirectW,
    wrap_SQLExecute,
    wrap_SQLExtendedFetch,
    wrap_SQLFetch,
    wrap_SQLFetchScroll,
    wrap_SQLForeignKeys,
    wrap_SQLForeignKeysW,
    wrap_SQLFreeConnect,
    wrap_SQLFreeEnv,
    wrap_SQLFreeHandle,
    wrap_SQLFreeStmt,
    wrap_SQLGetConnectAttr,
    wrap_SQLGetConnectAttrW,
    wrap_SQLGetConnectOption,
    wrap_SQLGetConnectOptionW,
    wrap_SQLGetCursorName,
    wrap_SQLGetCursorNameW,
    wrap_SQLGetData,
    wrap_SQLGetDescField,
    wrap_SQLGetDescFieldW,
    wrap_SQLGetDescRec,
    wrap_SQLGetDescRecW,
    wrap_SQLGetDiagField,
    wrap_SQLGetDiagFieldW,
    wrap_SQLGetDiagRec,
    wrap_SQLGetDiagRecW,
    wrap_SQLGetEnvAttr,
    wrap_SQLGetFunctions,
    wrap_SQLGetInfo,
    wrap_SQLGetInfoW,
    wrap_SQLGetStmtAttr,
    wrap_SQLGetStmtAttrW,
    wrap_SQLGetStmtOption,
    wrap_SQLGetTypeInfo,
    wrap_SQLGetTypeInfoW,
    wrap_SQLMoreResults,
    wrap_SQLNativeSql,
    wrap_SQLNativeSqlW,
    wrap_SQLNumParams,
    wrap_SQLNumResultCols,
    wrap_SQLParamData,
    wrap_SQLParamOptions,
    wrap_SQLPrepare,
    wrap_SQLPrepareW,
    wrap_SQLPrimaryKeys,
    wrap_SQLPrimaryKeysW,
    wrap_SQLProcedureColumns,
    wrap_SQLProcedureColumnsW,
    wrap_SQLProcedures,
    wrap_SQLProceduresW,
    wrap_SQLPutData,
    wrap_SQLRowCount,
    wrap_SQLSetConnectAttr,
    wrap_SQLSetConnectAttrW,
    wrap_SQLSetConnectOption,
    wrap_SQLSetConnectOptionW,
    wrap_SQLSetCursorName,
    wrap_SQLSetCursorNameW,
    wrap_SQLSetDescField,
    wrap_SQLSetDescFieldW,
    wrap_SQLSetDescRec,
    wrap_SQLSetEnvAttr,
    wrap_SQLSetParam,
    wrap_SQLSetPos,
    wrap_SQLSetScrollOptions,
    wrap_SQLSetStmtAttr,
    wrap_SQLSetStmtAttrW,
    wrap_SQLSetStmtOption,
    wrap_SQLSpecialColumns,
    wrap_SQLSpecialColumnsW,
    wrap_SQLStatistics,
    wrap_SQLStatisticsW,
    wrap_SQLTablePrivileges,
    wrap_SQLTablePrivilegesW,
    wrap_SQLTables,
    wrap_SQLTablesW,
    wrap_SQLTransact,
};

C_ASSERT( ARRAYSIZE( __wine_unix_call_funcs) == unix_funcs_count );

static NTSTATUS load_odbc(void)
{
   const char *s = getenv("LIB_ODBC_DRIVER_MANAGER");

#ifdef SONAME_LIBODBC
   if (!s || !s[0]) s = SONAME_LIBODBC;
#endif
   if (!s || !s[0] || !(libodbc = dlopen( s, RTLD_LAZY | RTLD_GLOBAL )))
   {
       ERR_(winediag)("failed to open library %s: %s\n", debugstr_a(s), dlerror());
       return STATUS_DLL_NOT_FOUND;
   }

#define LOAD_FUNC(name) \
    p##name = dlsym( libodbc, #name );

    LOAD_FUNC(SQLAllocConnect);
    LOAD_FUNC(SQLAllocEnv);
    LOAD_FUNC(SQLAllocHandle);
    LOAD_FUNC(SQLAllocHandleStd);
    LOAD_FUNC(SQLAllocStmt);
    LOAD_FUNC(SQLBindCol);
    LOAD_FUNC(SQLBindParam);
    LOAD_FUNC(SQLBindParameter);
    LOAD_FUNC(SQLBrowseConnect);
    LOAD_FUNC(SQLBrowseConnectW);
    LOAD_FUNC(SQLBulkOperations);
    LOAD_FUNC(SQLCancel);
    LOAD_FUNC(SQLCloseCursor);
    LOAD_FUNC(SQLColAttribute);
    LOAD_FUNC(SQLColAttributeW);
    LOAD_FUNC(SQLColAttributes);
    LOAD_FUNC(SQLColAttributesW);
    LOAD_FUNC(SQLColumnPrivileges);
    LOAD_FUNC(SQLColumnPrivilegesW);
    LOAD_FUNC(SQLColumns);
    LOAD_FUNC(SQLColumnsW);
    LOAD_FUNC(SQLConnect);
    LOAD_FUNC(SQLConnectW);
    LOAD_FUNC(SQLCopyDesc);
    LOAD_FUNC(SQLDataSources);
    LOAD_FUNC(SQLDataSourcesW);
    LOAD_FUNC(SQLDescribeCol);
    LOAD_FUNC(SQLDescribeColW);
    LOAD_FUNC(SQLDescribeParam);
    LOAD_FUNC(SQLDisconnect);
    LOAD_FUNC(SQLDriverConnect);
    LOAD_FUNC(SQLDriverConnectW);
    LOAD_FUNC(SQLDrivers);
    LOAD_FUNC(SQLDriversW);
    LOAD_FUNC(SQLEndTran);
    LOAD_FUNC(SQLError);
    LOAD_FUNC(SQLErrorW);
    LOAD_FUNC(SQLExecDirect);
    LOAD_FUNC(SQLExecDirectW);
    LOAD_FUNC(SQLExecute);
    LOAD_FUNC(SQLExtendedFetch);
    LOAD_FUNC(SQLFetch);
    LOAD_FUNC(SQLFetchScroll);
    LOAD_FUNC(SQLForeignKeys);
    LOAD_FUNC(SQLForeignKeysW);
    LOAD_FUNC(SQLFreeConnect);
    LOAD_FUNC(SQLFreeEnv);
    LOAD_FUNC(SQLFreeHandle);
    LOAD_FUNC(SQLFreeStmt);
    LOAD_FUNC(SQLGetConnectAttr);
    LOAD_FUNC(SQLGetConnectAttrW);
    LOAD_FUNC(SQLGetConnectOption);
    LOAD_FUNC(SQLGetConnectOptionW);
    LOAD_FUNC(SQLGetCursorName);
    LOAD_FUNC(SQLGetCursorNameW);
    LOAD_FUNC(SQLGetData);
    LOAD_FUNC(SQLGetDescField);
    LOAD_FUNC(SQLGetDescFieldW);
    LOAD_FUNC(SQLGetDescRec);
    LOAD_FUNC(SQLGetDescRecW);
    LOAD_FUNC(SQLGetDiagField);
    LOAD_FUNC(SQLGetDiagFieldW);
    LOAD_FUNC(SQLGetDiagRec);
    LOAD_FUNC(SQLGetDiagRecW);
    LOAD_FUNC(SQLGetEnvAttr);
    LOAD_FUNC(SQLGetFunctions);
    LOAD_FUNC(SQLGetInfo);
    LOAD_FUNC(SQLGetInfoW);
    LOAD_FUNC(SQLGetStmtAttr);
    LOAD_FUNC(SQLGetStmtAttrW);
    LOAD_FUNC(SQLGetStmtOption);
    LOAD_FUNC(SQLGetTypeInfo);
    LOAD_FUNC(SQLGetTypeInfoW);
    LOAD_FUNC(SQLMoreResults);
    LOAD_FUNC(SQLNativeSql);
    LOAD_FUNC(SQLNativeSqlW);
    LOAD_FUNC(SQLNumParams);
    LOAD_FUNC(SQLNumResultCols);
    LOAD_FUNC(SQLParamData);
    LOAD_FUNC(SQLParamOptions);
    LOAD_FUNC(SQLPrepare);
    LOAD_FUNC(SQLPrepareW);
    LOAD_FUNC(SQLPrimaryKeys);
    LOAD_FUNC(SQLPrimaryKeysW);
    LOAD_FUNC(SQLProcedureColumns);
    LOAD_FUNC(SQLProcedureColumnsW);
    LOAD_FUNC(SQLProcedures);
    LOAD_FUNC(SQLProceduresW);
    LOAD_FUNC(SQLPutData);
    LOAD_FUNC(SQLRowCount);
    LOAD_FUNC(SQLSetConnectAttr);
    LOAD_FUNC(SQLSetConnectAttrW);
    LOAD_FUNC(SQLSetConnectOption);
    LOAD_FUNC(SQLSetConnectOptionW);
    LOAD_FUNC(SQLSetCursorName);
    LOAD_FUNC(SQLSetCursorNameW);
    LOAD_FUNC(SQLSetDescField);
    LOAD_FUNC(SQLSetDescFieldW);
    LOAD_FUNC(SQLSetDescRec);
    LOAD_FUNC(SQLSetEnvAttr);
    LOAD_FUNC(SQLSetParam);
    LOAD_FUNC(SQLSetPos);
    LOAD_FUNC(SQLSetScrollOptions);
    LOAD_FUNC(SQLSetStmtAttr);
    LOAD_FUNC(SQLSetStmtAttrW);
    LOAD_FUNC(SQLSetStmtOption);
    LOAD_FUNC(SQLSpecialColumns);
    LOAD_FUNC(SQLSpecialColumnsW);
    LOAD_FUNC(SQLStatistics);
    LOAD_FUNC(SQLStatisticsW);
    LOAD_FUNC(SQLTablePrivileges);
    LOAD_FUNC(SQLTablePrivilegesW);
    LOAD_FUNC(SQLTables);
    LOAD_FUNC(SQLTablesW);
    LOAD_FUNC(SQLTransact);
#undef LOAD_FUNC
    return STATUS_SUCCESS;
}
