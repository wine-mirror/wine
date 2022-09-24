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
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winreg.h"
#include "wine/debug.h"

#include "sql.h"
#include "sqltypes.h"
#include "sqlext.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(odbc);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static unixlib_handle_t odbc_handle;

#define ODBC_CALL( func, params ) __wine_unix_call( odbc_handle, unix_ ## func, params )

/***********************************************************************
 * ODBC_ReplicateODBCInstToRegistry
 *
 * PARAMS
 *
 * RETURNS
 *
 * Utility to ODBC_ReplicateToRegistry to replicate the drivers of the
 * ODBCINST.INI settings
 *
 * The driver settings are not replicated to the registry.  If we were to 
 * replicate them we would need to decide whether to replicate all settings
 * or to do some translation; whether to remove any entries present only in
 * the windows registry, etc.
 */

static void ODBC_ReplicateODBCInstToRegistry (SQLHENV hEnv)
{
    HKEY hODBCInst;
    LONG reg_ret;
    BOOL success;

    success = FALSE;
    TRACE ("Driver settings are not currently replicated to the registry\n");
    if ((reg_ret = RegCreateKeyExA (HKEY_LOCAL_MACHINE,
            "Software\\ODBC\\ODBCINST.INI", 0, NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS /* a couple more than we need */, NULL,
            &hODBCInst, NULL)) == ERROR_SUCCESS)
    {
        HKEY hDrivers;
        if ((reg_ret = RegCreateKeyExA (hODBCInst, "ODBC Drivers", 0,
                NULL, REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS /* overkill */, NULL, &hDrivers, NULL))
                == ERROR_SUCCESS)
        {
            SQLRETURN sql_ret;
            SQLUSMALLINT dirn;
            CHAR desc [256];
            SQLSMALLINT sizedesc;

            success = TRUE;
            dirn = SQL_FETCH_FIRST;
            while ((sql_ret = SQLDrivers (hEnv, dirn, (SQLCHAR*)desc, sizeof(desc),
                    &sizedesc, NULL, 0, NULL)) == SQL_SUCCESS ||
                    sql_ret == SQL_SUCCESS_WITH_INFO)
            {
                /* FIXME Do some proper handling of the SUCCESS_WITH_INFO */
                dirn = SQL_FETCH_NEXT;
                if (sizedesc == lstrlenA(desc))
                {
                    HKEY hThis;
                    if ((reg_ret = RegQueryValueExA (hDrivers, desc, NULL,
                            NULL, NULL, NULL)) == ERROR_FILE_NOT_FOUND)
                    {
                        if ((reg_ret = RegSetValueExA (hDrivers, desc, 0,
                                REG_SZ, (const BYTE *)"Installed", 10)) != ERROR_SUCCESS)
                        {
                            TRACE ("Error %ld replicating driver %s\n",
                                    reg_ret, desc);
                            success = FALSE;
                        }
                    }
                    else if (reg_ret != ERROR_SUCCESS)
                    {
                        TRACE ("Error %ld checking for %s in drivers\n",
                                reg_ret, desc);
                        success = FALSE;
                    }
                    if ((reg_ret = RegCreateKeyExA (hODBCInst, desc, 0,
                            NULL, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL, &hThis, NULL))
                            == ERROR_SUCCESS)
                    {
                        /* FIXME This is where the settings go.
                         * I suggest that if the disposition says it 
                         * exists then we leave it alone.  Alternatively
                         * include an extra value to flag that it is 
                         * a replication of the unixODBC/iODBC/...
                         */
                        if ((reg_ret = RegCloseKey (hThis)) !=
                                ERROR_SUCCESS)
                            TRACE ("Error %ld closing %s key\n", reg_ret,
                                    desc);
                    }
                    else
                    {
                        TRACE ("Error %ld ensuring driver key %s\n",
                                reg_ret, desc);
                        success = FALSE;
                    }
                }
                else
                {
                    WARN ("Unusually long driver name %s not replicated\n",
                            desc);
                    success = FALSE;
                }
            }
            if (sql_ret != SQL_NO_DATA)
            {
                TRACE ("Error %d enumerating drivers\n", (int)sql_ret);
                success = FALSE;
            }
            if ((reg_ret = RegCloseKey (hDrivers)) != ERROR_SUCCESS)
            {
                TRACE ("Error %ld closing hDrivers\n", reg_ret);
            }
        }
        else
        {
            TRACE ("Error %ld opening HKLM\\S\\O\\OI\\Drivers\n", reg_ret);
        }
        if ((reg_ret = RegCloseKey (hODBCInst)) != ERROR_SUCCESS)
        {
            TRACE ("Error %ld closing HKLM\\S\\O\\ODBCINST.INI\n", reg_ret);
        }
    }
    else
    {
        TRACE ("Error %ld opening HKLM\\S\\O\\ODBCINST.INI\n", reg_ret);
    }
    if (!success)
    {
        WARN ("May not have replicated all ODBC drivers to the registry\n");
    }
}

/***********************************************************************
 * ODBC_ReplicateODBCToRegistry
 *
 * PARAMS
 *
 * RETURNS
 *
 * Utility to ODBC_ReplicateToRegistry to replicate either the USER or 
 * SYSTEM dsns
 *
 * For now simply place the "Driver description" (as returned by SQLDataSources)
 * into the registry as the driver.  This is enough to satisfy Crystal's 
 * requirement that there be a driver entry.  (It doesn't seem to care what
 * the setting is).
 * A slightly more accurate setting would be to access the registry to find
 * the actual driver library for the given description (which appears to map
 * to one of the HKLM/Software/ODBC/ODBCINST.INI keys).  (If you do this note
 * that this will add a requirement that this function be called after
 * ODBC_ReplicateODBCInstToRegistry)
 */
static void ODBC_ReplicateODBCToRegistry (BOOL is_user, SQLHENV hEnv)
{
    HKEY hODBC;
    LONG reg_ret;
    SQLRETURN sql_ret;
    SQLUSMALLINT dirn;
    CHAR dsn [SQL_MAX_DSN_LENGTH + 1];
    SQLSMALLINT sizedsn;
    CHAR desc [256];
    SQLSMALLINT sizedesc;
    BOOL success;
    const char *which = is_user ? "user" : "system";

    success = FALSE;
    if ((reg_ret = RegCreateKeyExA (
            is_user ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE,
            "Software\\ODBC\\ODBC.INI", 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS /* a couple more than we need */, NULL, &hODBC,
            NULL)) == ERROR_SUCCESS)
    {
        success = TRUE;
        dirn = is_user ? SQL_FETCH_FIRST_USER : SQL_FETCH_FIRST_SYSTEM;
        while ((sql_ret = SQLDataSources (hEnv, dirn,
                (SQLCHAR*)dsn, sizeof(dsn), &sizedsn,
                (SQLCHAR*)desc, sizeof(desc), &sizedesc)) == SQL_SUCCESS
                || sql_ret == SQL_SUCCESS_WITH_INFO)
        {
            /* FIXME Do some proper handling of the SUCCESS_WITH_INFO */
            dirn = SQL_FETCH_NEXT;
            if (sizedsn == lstrlenA(dsn) && sizedesc == lstrlenA(desc))
            {
                HKEY hDSN;
                if ((reg_ret = RegCreateKeyExA (hODBC, dsn, 0,
                        NULL, REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS, NULL, &hDSN, NULL))
                        == ERROR_SUCCESS)
                {
                    static const char DRIVERKEY[] = "Driver";
                    if ((reg_ret = RegQueryValueExA (hDSN, DRIVERKEY,
                            NULL, NULL, NULL, NULL))
                            == ERROR_FILE_NOT_FOUND)
                    {
                        if ((reg_ret = RegSetValueExA (hDSN, DRIVERKEY, 0,
                                REG_SZ, (LPBYTE)desc, sizedesc)) != ERROR_SUCCESS)
                        {
                            TRACE ("Error %ld replicating description of "
                                    "%s(%s)\n", reg_ret, dsn, desc);
                            success = FALSE;
                        }
                    }
                    else if (reg_ret != ERROR_SUCCESS)
                    {
                        TRACE ("Error %ld checking for description of %s\n",
                                reg_ret, dsn);
                        success = FALSE;
                    }
                    if ((reg_ret = RegCloseKey (hDSN)) != ERROR_SUCCESS)
                    {
                        TRACE ("Error %ld closing %s DSN key %s\n",
                                reg_ret, which, dsn);
                    }
                }
                else
                {
                    TRACE ("Error %ld opening %s DSN key %s\n",
                            reg_ret, which, dsn);
                    success = FALSE;
                }
            }
            else
            {
                WARN ("Unusually long %s data source name %s (%s) not "
                        "replicated\n", which, dsn, desc);
                success = FALSE;
            }
        }
        if (sql_ret != SQL_NO_DATA)
        {
            TRACE ("Error %d enumerating %s datasources\n",
                    (int)sql_ret, which);
            success = FALSE;
        }
        if ((reg_ret = RegCloseKey (hODBC)) != ERROR_SUCCESS)
        {
            TRACE ("Error %ld closing %s ODBC.INI registry key\n", reg_ret,
                    which);
        }
    }
    else
    {
        TRACE ("Error %ld creating/opening %s ODBC.INI registry key\n",
                reg_ret, which);
    }
    if (!success)
    {
        WARN ("May not have replicated all %s ODBC DSNs to the registry\n",
                which);
    }
}

/***********************************************************************
 * ODBC_ReplicateToRegistry
 *
 * PARAMS
 *
 * RETURNS
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

static void ODBC_ReplicateToRegistry (void)
{
    SQLRETURN sql_ret;
    SQLHENV hEnv;

    if ((sql_ret = SQLAllocEnv(&hEnv)) == SQL_SUCCESS)
    {
        ODBC_ReplicateODBCInstToRegistry (hEnv);
        ODBC_ReplicateODBCToRegistry (FALSE /* system dsns */, hEnv);
        ODBC_ReplicateODBCToRegistry (TRUE /* user dsns */, hEnv);

        if ((sql_ret = SQLFreeEnv(hEnv)) != SQL_SUCCESS)
        {
            TRACE ("Error %d freeing the SQL environment.\n", (int)sql_ret);
        }
    }
    else
    {
        TRACE ("Error %d opening an SQL environment.\n", (int)sql_ret);
        WARN ("The external ODBC settings have not been replicated to the"
                " Wine registry\n");
    }
}

/*************************************************************************
 *				SQLAllocConnect           [ODBC32.001]
 */
SQLRETURN WINAPI SQLAllocConnect(SQLHENV EnvironmentHandle, SQLHDBC *ConnectionHandle)
{
    struct SQLAllocConnect_params params = { EnvironmentHandle, ConnectionHandle };
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p)\n", EnvironmentHandle, ConnectionHandle);

    *ConnectionHandle = SQL_NULL_HDBC;
    ret = ODBC_CALL( SQLAllocConnect, &params );
    TRACE("Returning %d, ConnectionHandle %p\n", ret, *ConnectionHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocEnv           [ODBC32.002]
 */
SQLRETURN WINAPI SQLAllocEnv(SQLHENV *EnvironmentHandle)
{
    struct SQLAllocEnv_params params = { EnvironmentHandle };
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p)\n", EnvironmentHandle);

    *EnvironmentHandle = SQL_NULL_HENV;
    ret = ODBC_CALL( SQLAllocEnv, &params );
    TRACE("Returning %d, EnvironmentHandle %p\n", ret, *EnvironmentHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocHandle           [ODBC32.024]
 */
SQLRETURN WINAPI SQLAllocHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
    struct SQLAllocHandle_params params = { HandleType, InputHandle, OutputHandle };
    SQLRETURN ret;

    TRACE("(HandleType %d, InputHandle %p, OutputHandle %p)\n", HandleType, InputHandle, OutputHandle);

    *OutputHandle = 0;
    ret = ODBC_CALL( SQLAllocHandle, &params );
    TRACE("Returning %d, Handle %p\n", ret, *OutputHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocStmt           [ODBC32.003]
 */
SQLRETURN WINAPI SQLAllocStmt(SQLHDBC ConnectionHandle, SQLHSTMT *StatementHandle)
{
    struct SQLAllocStmt_params params = { ConnectionHandle, StatementHandle };
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, StatementHandle %p)\n", ConnectionHandle, StatementHandle);

    *StatementHandle = SQL_NULL_HSTMT;
    ret = ODBC_CALL( SQLAllocStmt, &params );
    TRACE ("Returning %d, StatementHandle %p\n", ret, *StatementHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocHandleStd           [ODBC32.077]
 */
SQLRETURN WINAPI SQLAllocHandleStd(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
    struct SQLAllocHandleStd_params params = { HandleType, InputHandle, OutputHandle };
    SQLRETURN ret;

    TRACE("(HandleType %d, InputHandle %p, OutputHandle %p)\n", HandleType, InputHandle, OutputHandle);

    *OutputHandle = 0;
    ret = ODBC_CALL( SQLAllocHandleStd, &params );
    TRACE ("Returning %d, OutputHandle %p\n", ret, *OutputHandle);
    return ret;
}

static const char *debugstr_sqllen( SQLLEN len )
{
#ifdef _WIN64
    return wine_dbg_sprintf( "%Id", len );
#else
    return wine_dbg_sprintf( "%d", len );
#endif
}

/*************************************************************************
 *				SQLBindCol           [ODBC32.004]
 */
SQLRETURN WINAPI SQLBindCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                            SQLPOINTER TargetValue, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
    struct SQLBindCol_params params = { StatementHandle, ColumnNumber, TargetType, TargetValue,
                                        BufferLength, StrLen_or_Ind };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, TargetType %d, TargetValue %p, BufferLength %s, StrLen_or_Ind %p)\n",
          StatementHandle, ColumnNumber, TargetType, TargetValue, debugstr_sqllen(BufferLength), StrLen_or_Ind);

    ret = ODBC_CALL( SQLBindCol, &params );
    TRACE ("Returning %d\n", ret);
    return ret;
}

static const char *debugstr_sqlulen( SQLULEN len )
{
#ifdef _WIN64
    return wine_dbg_sprintf( "%Iu", len );
#else
    return wine_dbg_sprintf( "%u", len );
#endif
}

/*************************************************************************
 *				SQLBindParam           [ODBC32.025]
 */
SQLRETURN WINAPI SQLBindParam(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
                              SQLSMALLINT ParameterType, SQLULEN LengthPrecision, SQLSMALLINT ParameterScale,
                              SQLPOINTER ParameterValue, SQLLEN *StrLen_or_Ind)
{
    struct SQLBindParam_params params = { StatementHandle, ParameterNumber, ValueType, ParameterType,
                                          LengthPrecision, ParameterScale, ParameterValue, StrLen_or_Ind };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ParameterNumber %d, ValueType %d, ParameterType %d, LengthPrecision %s,"
          " ParameterScale %d, ParameterValue %p, StrLen_or_Ind %p)\n", StatementHandle, ParameterNumber, ValueType,
          ParameterType, debugstr_sqlulen(LengthPrecision), ParameterScale, ParameterValue, StrLen_or_Ind);

    ret = ODBC_CALL( SQLBindParam, &params );
    TRACE ("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLCancel           [ODBC32.005]
 */
SQLRETURN WINAPI SQLCancel(SQLHSTMT StatementHandle)
{
    struct SQLCancel_params params = { StatementHandle };
    SQLRETURN ret;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    ret = ODBC_CALL( SQLCancel, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLCloseCursor           [ODBC32.026]
 */
SQLRETURN WINAPI SQLCloseCursor(SQLHSTMT StatementHandle)
{
    struct SQLCloseCursor_params params = { StatementHandle };
    SQLRETURN ret;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    ret = ODBC_CALL( SQLCloseCursor, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColAttribute           [ODBC32.027]
 */
SQLRETURN WINAPI SQLColAttribute(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
                                 SQLUSMALLINT FieldIdentifier, SQLPOINTER CharacterAttribute,
                                 SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                 SQLLEN *NumericAttribute)
{
    struct SQLColAttribute_params params = { StatementHandle, ColumnNumber, FieldIdentifier,
                                             CharacterAttribute, BufferLength, StringLength, NumericAttribute };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, FieldIdentifier %d, CharacterAttribute %p, BufferLength %d,"
          " StringLength %p, NumericAttribute %p)\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttribute, BufferLength, StringLength, NumericAttribute);

    ret = ODBC_CALL( SQLColAttribute, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColumns           [ODBC32.040]
 */
SQLRETURN WINAPI SQLColumns(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                            SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                            SQLSMALLINT NameLength3, SQLCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    struct SQLColumns_params params = { StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2,
                                        TableName, NameLength3, ColumnName, NameLength4 };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3,
          debugstr_an((const char *)ColumnName, NameLength4), NameLength4);

    ret = ODBC_CALL( SQLColumns, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLConnect           [ODBC32.007]
 */
SQLRETURN WINAPI SQLConnect(SQLHDBC ConnectionHandle, SQLCHAR *ServerName, SQLSMALLINT NameLength1,
                            SQLCHAR *UserName, SQLSMALLINT NameLength2, SQLCHAR *Authentication,
                            SQLSMALLINT NameLength3)
{
    struct SQLConnect_params params = { ConnectionHandle, ServerName, NameLength1, UserName, NameLength2,
                                        Authentication, NameLength3 };
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, ServerName %s, NameLength1 %d, UserName %s, NameLength2 %d, Authentication %s,"
          " NameLength3 %d)\n", ConnectionHandle,
          debugstr_an((const char *)ServerName, NameLength1), NameLength1,
          debugstr_an((const char *)UserName, NameLength2), NameLength2,
          debugstr_an((const char *)Authentication, NameLength3), NameLength3);

    ret = ODBC_CALL( SQLConnect, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLCopyDesc           [ODBC32.028]
 */
SQLRETURN WINAPI SQLCopyDesc(SQLHDESC SourceDescHandle, SQLHDESC TargetDescHandle)
{
    struct SQLCopyDesc_params params = { SourceDescHandle, TargetDescHandle };
    SQLRETURN ret;

    TRACE("(SourceDescHandle %p, TargetDescHandle %p)\n", SourceDescHandle, TargetDescHandle);

    ret = ODBC_CALL( SQLCopyDesc, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDataSources           [ODBC32.057]
 */
SQLRETURN WINAPI SQLDataSources(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLCHAR *ServerName,
                                SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, SQLCHAR *Description,
                                SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    struct SQLDataSources_params params = { EnvironmentHandle, Direction, ServerName, BufferLength1,
                                            NameLength1, Description, BufferLength2, NameLength2 };
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, ServerName %p, BufferLength1 %d, NameLength1 %p, Description %p,"
          " BufferLength2 %d, NameLength2 %p)\n", EnvironmentHandle, Direction, ServerName, BufferLength1,
          NameLength1, Description, BufferLength2, NameLength2);

    ret = ODBC_CALL( SQLDataSources, &params );
    if (ret >= 0 && TRACE_ON(odbc))
    {
        if (ServerName && NameLength1 && *NameLength1 > 0)
            TRACE(" DataSource %s", debugstr_an((const char *)ServerName, *NameLength1));
        if (Description && NameLength2 && *NameLength2 > 0)
            TRACE(" Description %s", debugstr_an((const char *)Description, *NameLength2));
        TRACE("\n");
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

SQLRETURN WINAPI SQLDataSourcesA(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLCHAR *ServerName,
                                 SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, SQLCHAR *Description,
                                 SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    struct SQLDataSourcesA_params params = { EnvironmentHandle, Direction, ServerName, BufferLength1,
                                             NameLength1, Description, BufferLength2, NameLength2 };
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, ServerName %p, BufferLength1 %d, NameLength1 %p, Description %p,"
          " BufferLength2 %d, NameLength2 %p)\n", EnvironmentHandle, Direction, ServerName, BufferLength1,
          NameLength1, Description, BufferLength2, NameLength2);

    ret = ODBC_CALL( SQLDataSourcesA, &params );
    if (TRACE_ON(odbc))
    {
       if (ServerName && NameLength1 && *NameLength1 > 0)
            TRACE(" DataSource %s", debugstr_an((const char *)ServerName, *NameLength1));
       if (Description && NameLength2 && *NameLength2 > 0)
            TRACE(" Description %s", debugstr_an((const char *)Description, *NameLength2));
       TRACE("\n");
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDescribeCol           [ODBC32.008]
 */
SQLRETURN WINAPI SQLDescribeCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLCHAR *ColumnName,
                                SQLSMALLINT BufferLength, SQLSMALLINT *NameLength, SQLSMALLINT *DataType,
                                SQLULEN *ColumnSize, SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
    struct SQLDescribeCol_params params = { StatementHandle, ColumnNumber, ColumnName, BufferLength,
                                            NameLength, DataType, ColumnSize, DecimalDigits, Nullable };
    SQLSMALLINT dummy;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, ColumnName %p, BufferLength %d, NameLength %p, DataType %p,"
          " ColumnSize %p, DecimalDigits %p, Nullable %p)\n", StatementHandle, ColumnNumber, ColumnName,
          BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);

    if (!params.NameLength) params.NameLength = &dummy; /* workaround for drivers that don't accept NULL NameLength */

    ret = ODBC_CALL( SQLDescribeCol, &params );
    if (ret >= 0)
    {
        if (ColumnName && NameLength) TRACE(" ColumnName %s\n", debugstr_an((const char *)ColumnName, *NameLength));
        if (DataType) TRACE(" DataType %d\n", *DataType);
        if (ColumnSize) TRACE(" ColumnSize %s\n", debugstr_sqlulen(*ColumnSize));
        if (DecimalDigits) TRACE(" DecimalDigits %d\n", *DecimalDigits);
        if (Nullable) TRACE(" Nullable %d\n", *Nullable);
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDisconnect           [ODBC32.009]
 */
SQLRETURN WINAPI SQLDisconnect(SQLHDBC ConnectionHandle)
{
    struct SQLDisconnect_params params = { ConnectionHandle };
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p)\n", ConnectionHandle);

    ret = ODBC_CALL( SQLDisconnect, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLEndTran           [ODBC32.029]
 */
SQLRETURN WINAPI SQLEndTran(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT CompletionType)
{
    struct SQLEndTran_params params = { HandleType, Handle, CompletionType };
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, CompletionType %d)\n", HandleType, Handle, CompletionType);

    ret = ODBC_CALL( SQLEndTran, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLError           [ODBC32.010]
 */
SQLRETURN WINAPI SQLError(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
                          SQLCHAR *Sqlstate, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                          SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    struct SQLError_params params = { EnvironmentHandle, ConnectionHandle, StatementHandle, Sqlstate,
                                      NativeError, MessageText, BufferLength, TextLength };
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, StatementHandle %p, Sqlstate %p, NativeError %p,"
          " MessageText %p, BufferLength %d, TextLength %p)\n", EnvironmentHandle, ConnectionHandle,
          StatementHandle, Sqlstate, NativeError, MessageText, BufferLength, TextLength);

    ret = ODBC_CALL( SQLError, &params );

    if (ret == SQL_SUCCESS)
    {
        TRACE(" SQLState %s\n", debugstr_an((const char *)Sqlstate, 5));
        TRACE(" Error %d\n", *NativeError);
        TRACE(" MessageText %s\n", debugstr_an((const char *)MessageText, *TextLength));
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLExecDirect           [ODBC32.011]
 */
SQLRETURN WINAPI SQLExecDirect(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
    struct SQLExecDirect_params params = { StatementHandle, StatementText, TextLength };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_an((const char *)StatementText, TextLength), TextLength);

    ret = ODBC_CALL( SQLExecDirect, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLExecute           [ODBC32.012]
 */
SQLRETURN WINAPI SQLExecute(SQLHSTMT StatementHandle)
{
    struct SQLExecute_params params = { StatementHandle };
    SQLRETURN ret;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    ret = ODBC_CALL( SQLExecute, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFetch           [ODBC32.013]
 */
SQLRETURN WINAPI SQLFetch(SQLHSTMT StatementHandle)
{
    struct SQLFetch_params params = { StatementHandle };
    SQLRETURN ret;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    ret = ODBC_CALL( SQLFetch, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFetchScroll          [ODBC32.030]
 */
SQLRETURN WINAPI SQLFetchScroll(SQLHSTMT StatementHandle, SQLSMALLINT FetchOrientation, SQLLEN FetchOffset)
{
    struct SQLFetchScroll_params params = { StatementHandle, FetchOrientation, FetchOffset };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, FetchOrientation %d, FetchOffset %s)\n", StatementHandle, FetchOrientation,
          debugstr_sqllen(FetchOffset));

    ret = ODBC_CALL( SQLFetchScroll, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeConnect           [ODBC32.014]
 */
SQLRETURN WINAPI SQLFreeConnect(SQLHDBC ConnectionHandle)
{
    struct SQLFreeConnect_params params = { ConnectionHandle };
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p)\n", ConnectionHandle);

    ret = ODBC_CALL( SQLFreeConnect, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeEnv           [ODBC32.015]
 */
SQLRETURN WINAPI SQLFreeEnv(SQLHENV EnvironmentHandle)
{
    struct SQLFreeEnv_params params = { EnvironmentHandle };
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p)\n", EnvironmentHandle);

    ret = ODBC_CALL( SQLFreeEnv, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeHandle           [ODBC32.031]
 */
SQLRETURN WINAPI SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
    struct SQLFreeHandle_params params = { HandleType, Handle };
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p)\n", HandleType, Handle);

    ret = ODBC_CALL( SQLFreeHandle, &params );
    TRACE ("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeStmt           [ODBC32.016]
 */
SQLRETURN WINAPI SQLFreeStmt(SQLHSTMT StatementHandle, SQLUSMALLINT Option)
{
    struct SQLFreeStmt_params params = { StatementHandle, Option };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Option %d)\n", StatementHandle, Option);

    ret = ODBC_CALL( SQLFreeStmt, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetConnectAttr           [ODBC32.032]
 */
SQLRETURN WINAPI SQLGetConnectAttr(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                   SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct SQLGetConnectAttr_params params = { ConnectionHandle, Attribute, Value, BufferLength, StringLength };
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          Attribute, Value, BufferLength, StringLength);

    ret = ODBC_CALL( SQLGetConnectAttr, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetConnectOption       [ODBC32.042]
 */
SQLRETURN WINAPI SQLGetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    struct SQLGetConnectOption_params params = { ConnectionHandle, Option, Value };
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Option %d, Value %p)\n", ConnectionHandle, Option, Value);

    ret = ODBC_CALL( SQLGetConnectOption, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetCursorName           [ODBC32.017]
 */
SQLRETURN WINAPI SQLGetCursorName(SQLHSTMT StatementHandle, SQLCHAR *CursorName, SQLSMALLINT BufferLength,
                                  SQLSMALLINT *NameLength)
{
    struct SQLGetCursorName_params params = { StatementHandle, CursorName, BufferLength, NameLength };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CursorName %p, BufferLength %d, NameLength %p)\n", StatementHandle, CursorName,
          BufferLength, NameLength);

    ret = ODBC_CALL( SQLGetCursorName, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetData           [ODBC32.043]
 */
SQLRETURN WINAPI SQLGetData(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                            SQLPOINTER TargetValue, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
    struct SQLGetData_params params = { StatementHandle, ColumnNumber, TargetType, TargetValue,
                                        BufferLength, StrLen_or_Ind };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, TargetType %d, TargetValue %p, BufferLength %s, StrLen_or_Ind %p)\n",
          StatementHandle, ColumnNumber, TargetType, TargetValue, debugstr_sqllen(BufferLength), StrLen_or_Ind);

    ret = ODBC_CALL( SQLGetData, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDescField           [ODBC32.033]
 */
SQLRETURN WINAPI SQLGetDescField(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                 SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct SQLGetDescField_params params = { DescriptorHandle, RecNumber, FieldIdentifier, Value,
                                             BufferLength, StringLength };
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d, StringLength %p)\n",
          DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);

    ret = ODBC_CALL( SQLGetDescField, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDescRec           [ODBC32.034]
 */
SQLRETURN WINAPI SQLGetDescRec(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLCHAR *Name,
                               SQLSMALLINT BufferLength, SQLSMALLINT *StringLength, SQLSMALLINT *Type,
                               SQLSMALLINT *SubType, SQLLEN *Length, SQLSMALLINT *Precision,
                               SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
    struct SQLGetDescRec_params params = { DescriptorHandle, RecNumber, Name, BufferLength, StringLength,
                                           Type, SubType, Length, Precision, Scale, Nullable };
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, Name %p, BufferLength %d, StringLength %p, Type %p, SubType %p,"
          " Length %p, Precision %p, Scale %p, Nullable %p)\n", DescriptorHandle, RecNumber, Name, BufferLength,
          StringLength, Type, SubType, Length, Precision, Scale, Nullable);

    ret = ODBC_CALL( SQLGetDescRec, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDiagField           [ODBC32.035]
 */
SQLRETURN WINAPI SQLGetDiagField(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                 SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
                                 SQLSMALLINT *StringLength)
{
    struct SQLGetDiagField_params params = { HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo,
                                             BufferLength, StringLength };
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, DiagIdentifier %d, DiagInfo %p, BufferLength %d,"
          " StringLength %p)\n", HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);

    ret = ODBC_CALL( SQLGetDiagField, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDiagRec           [ODBC32.036]
 */
SQLRETURN WINAPI SQLGetDiagRec(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                               SQLCHAR *Sqlstate, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                               SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    struct SQLGetDiagRec_params params = { HandleType, Handle, RecNumber, Sqlstate, NativeError,
                                           MessageText, BufferLength, TextLength };
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, Sqlstate %p, NativeError %p, MessageText %p, BufferLength %d,"
          " TextLength %p)\n", HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength,
          TextLength);

    ret = ODBC_CALL( SQLGetDiagRec, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetEnvAttr           [ODBC32.037]
 */
SQLRETURN WINAPI SQLGetEnvAttr(SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                               SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct SQLGetEnvAttr_params params = { EnvironmentHandle, Attribute, Value, BufferLength, StringLength };
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n",
          EnvironmentHandle, Attribute, Value, BufferLength, StringLength);

    ret = ODBC_CALL( SQLGetEnvAttr, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetFunctions           [ODBC32.044]
 */
SQLRETURN WINAPI SQLGetFunctions(SQLHDBC ConnectionHandle, SQLUSMALLINT FunctionId, SQLUSMALLINT *Supported)
{
    struct SQLGetFunctions_params params = { ConnectionHandle, FunctionId, Supported };
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, FunctionId %d, Supported %p)\n", ConnectionHandle, FunctionId, Supported);

    ret = ODBC_CALL( SQLGetFunctions, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetInfo           [ODBC32.045]
 */
SQLRETURN WINAPI SQLGetInfo(SQLHDBC ConnectionHandle, SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
                            SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
    struct SQLGetInfo_params params = { ConnectionHandle, InfoType, InfoValue, BufferLength, StringLength };
    SQLRETURN ret;

    TRACE("(ConnectionHandle, %p, InfoType %d, InfoValue %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          InfoType, InfoValue, BufferLength, StringLength);

    ret = ODBC_CALL( SQLGetInfo, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetStmtAttr           [ODBC32.038]
 */
SQLRETURN WINAPI SQLGetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct SQLGetStmtAttr_params params = { StatementHandle, Attribute, Value, BufferLength, StringLength };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", StatementHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!Value)
    {
        WARN("Unexpected NULL Value return address\n");
        return SQL_ERROR;
    }

    ret = ODBC_CALL( SQLGetStmtAttr, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetStmtOption           [ODBC32.046]
 */
SQLRETURN WINAPI SQLGetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    struct SQLGetStmtOption_params params = { StatementHandle, Option, Value };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Option %d, Value %p)\n", StatementHandle, Option, Value);

    ret = ODBC_CALL( SQLGetStmtOption, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetTypeInfo           [ODBC32.047]
 */
SQLRETURN WINAPI SQLGetTypeInfo(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    struct SQLGetTypeInfo_params params = { StatementHandle, DataType };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, DataType %d)\n", StatementHandle, DataType);

    ret = ODBC_CALL( SQLGetTypeInfo, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNumResultCols           [ODBC32.018]
 */
SQLRETURN WINAPI SQLNumResultCols(SQLHSTMT StatementHandle, SQLSMALLINT *ColumnCount)
{
    struct SQLNumResultCols_params params = { StatementHandle, ColumnCount };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnCount %p)\n", StatementHandle, ColumnCount);

    ret = ODBC_CALL( SQLNumResultCols, &params );
    TRACE("Returning %d ColumnCount %d\n", ret, *ColumnCount);
    return ret;
}

/*************************************************************************
 *				SQLParamData           [ODBC32.048]
 */
SQLRETURN WINAPI SQLParamData(SQLHSTMT StatementHandle, SQLPOINTER *Value)
{
    struct SQLParamData_params params = { StatementHandle, Value };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Value %p)\n", StatementHandle, Value);

    ret = ODBC_CALL( SQLParamData, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPrepare           [ODBC32.019]
 */
SQLRETURN WINAPI SQLPrepare(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
    struct SQLPrepare_params params = { StatementHandle, StatementText, TextLength };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_an((const char *)StatementText, TextLength), TextLength);

    ret = ODBC_CALL( SQLPrepare, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPutData           [ODBC32.049]
 */
SQLRETURN WINAPI SQLPutData(SQLHSTMT StatementHandle, SQLPOINTER Data, SQLLEN StrLen_or_Ind)
{
    struct SQLPutData_params params = { StatementHandle, Data, StrLen_or_Ind };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Data %p, StrLen_or_Ind %s)\n", StatementHandle, Data, debugstr_sqllen(StrLen_or_Ind));

    ret = ODBC_CALL( SQLPutData, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLRowCount           [ODBC32.020]
 */
SQLRETURN WINAPI SQLRowCount(SQLHSTMT StatementHandle, SQLLEN *RowCount)
{
    struct SQLRowCount_params params = { StatementHandle, RowCount };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, RowCount %p)\n", StatementHandle, RowCount);

    ret = ODBC_CALL( SQLRowCount, &params );
    if (ret == SQL_SUCCESS && RowCount) TRACE(" RowCount %s\n", debugstr_sqllen(*RowCount));
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectAttr           [ODBC32.039]
 */
SQLRETURN WINAPI SQLSetConnectAttr(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                   SQLINTEGER StringLength)
{
    struct SQLSetConnectAttr_params params = { ConnectionHandle, Attribute, Value, StringLength };
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, StringLength %d)\n", ConnectionHandle, Attribute, Value,
          StringLength);

    ret = ODBC_CALL( SQLSetConnectAttr, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectOption           [ODBC32.050]
 */
SQLRETURN WINAPI SQLSetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    struct SQLSetConnectOption_params params = { ConnectionHandle, Option, Value };
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Option %d, Value %s)\n", ConnectionHandle, Option, debugstr_sqlulen(Value));

    ret = ODBC_CALL( SQLSetConnectOption, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetCursorName           [ODBC32.021]
 */
SQLRETURN WINAPI SQLSetCursorName(SQLHSTMT StatementHandle, SQLCHAR *CursorName, SQLSMALLINT NameLength)
{
    struct SQLSetCursorName_params params = { StatementHandle, CursorName, NameLength };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CursorName %s, NameLength %d)\n", StatementHandle,
          debugstr_an((const char *)CursorName, NameLength), NameLength);

    ret = ODBC_CALL( SQLSetCursorName, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetDescField           [ODBC32.073]
 */
SQLRETURN WINAPI SQLSetDescField(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                 SQLPOINTER Value, SQLINTEGER BufferLength)
{
    struct SQLSetDescField_params params = { DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength };
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d)\n", DescriptorHandle,
          RecNumber, FieldIdentifier, Value, BufferLength);

    ret = ODBC_CALL( SQLSetDescField, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetDescRec           [ODBC32.074]
 */
SQLRETURN WINAPI SQLSetDescRec(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT Type,
                               SQLSMALLINT SubType, SQLLEN Length, SQLSMALLINT Precision, SQLSMALLINT Scale,
                               SQLPOINTER Data, SQLLEN *StringLength, SQLLEN *Indicator)
{
    struct SQLSetDescRec_params params = { DescriptorHandle, RecNumber, Type, SubType, Length, Precision, Scale, Data, StringLength, Indicator };
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, Type %d, SubType %d, Length %s, Precision %d, Scale %d, Data %p,"
          " StringLength %p, Indicator %p)\n", DescriptorHandle, RecNumber, Type, SubType, debugstr_sqllen(Length),
          Precision, Scale, Data, StringLength, Indicator);

    ret = ODBC_CALL( SQLSetDescRec, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetEnvAttr           [ODBC32.075]
 */
SQLRETURN WINAPI SQLSetEnvAttr(SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                               SQLINTEGER StringLength)
{
    struct SQLSetEnvAttr_params params = { EnvironmentHandle, Attribute, Value, StringLength };
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Attribute %d, Value %p, StringLength %d)\n", EnvironmentHandle, Attribute, Value,
          StringLength);

    ret = ODBC_CALL( SQLSetEnvAttr, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetParam           [ODBC32.022]
 */
SQLRETURN WINAPI SQLSetParam(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
                             SQLSMALLINT ParameterType, SQLULEN LengthPrecision, SQLSMALLINT ParameterScale,
                             SQLPOINTER ParameterValue, SQLLEN *StrLen_or_Ind)
{
    struct SQLSetParam_params params = { StatementHandle, ParameterNumber, ValueType, ParameterType,
                                         LengthPrecision, ParameterScale, ParameterValue, StrLen_or_Ind };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ParameterNumber %d, ValueType %d, ParameterType %d, LengthPrecision %s,"
          " ParameterScale %d, ParameterValue %p, StrLen_or_Ind %p)\n", StatementHandle, ParameterNumber, ValueType,
          ParameterType, debugstr_sqlulen(LengthPrecision), ParameterScale, ParameterValue, StrLen_or_Ind);

    ret = ODBC_CALL( SQLSetParam, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetStmtAttr           [ODBC32.076]
 */
SQLRETURN WINAPI SQLSetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                SQLINTEGER StringLength)
{
    struct SQLSetStmtAttr_params params = { StatementHandle, Attribute, Value, StringLength };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, StringLength %d)\n", StatementHandle, Attribute, Value,
          StringLength);

    ret = ODBC_CALL( SQLSetStmtAttr, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetStmtOption           [ODBC32.051]
 */
SQLRETURN WINAPI SQLSetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    struct SQLSetStmtOption_params params = { StatementHandle, Option, Value };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Option %d, Value %s)\n", StatementHandle, Option, debugstr_sqlulen(Value));

    ret = ODBC_CALL( SQLSetStmtOption, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSpecialColumns           [ODBC32.052]
 */
SQLRETURN WINAPI SQLSpecialColumns(SQLHSTMT StatementHandle, SQLUSMALLINT IdentifierType, SQLCHAR *CatalogName,
                                   SQLSMALLINT NameLength1, SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
                                   SQLCHAR *TableName, SQLSMALLINT NameLength3, SQLUSMALLINT Scope,
                                   SQLUSMALLINT Nullable)
{
    struct SQLSpecialColumns_params params = { StatementHandle, IdentifierType, CatalogName, NameLength1,
                                               SchemaName, NameLength2, TableName, NameLength3, Scope, Nullable };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, IdentifierType %d, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d,"
          " TableName %s, NameLength3 %d, Scope %d, Nullable %d)\n", StatementHandle, IdentifierType,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3, Scope, Nullable);

    ret = ODBC_CALL( SQLSpecialColumns, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLStatistics           [ODBC32.053]
 */
SQLRETURN WINAPI SQLStatistics(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                               SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                               SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
    struct SQLStatistics_params params = { StatementHandle, CatalogName, NameLength1, SchemaName,
                                           NameLength2, TableName, NameLength3, Unique, Reserved };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d SchemaName %s, NameLength2 %d, TableName %s"
          " NameLength3 %d, Unique %d, Reserved %d)\n", StatementHandle,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3, Unique, Reserved);

    ret = ODBC_CALL( SQLStatistics, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTables           [ODBC32.054]
 */
SQLRETURN WINAPI SQLTables(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                           SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                           SQLSMALLINT NameLength3, SQLCHAR *TableType, SQLSMALLINT NameLength4)
{
    struct SQLTables_params params = { StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2,
                                       TableName, NameLength3, TableType, NameLength4 };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, TableType %s, NameLength4 %d)\n", StatementHandle,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3,
          debugstr_an((const char *)TableType, NameLength4), NameLength4);

    ret = ODBC_CALL( SQLTables, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTransact           [ODBC32.023]
 */
SQLRETURN WINAPI SQLTransact(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLUSMALLINT CompletionType)
{
    struct SQLTransact_params params = { EnvironmentHandle, ConnectionHandle, CompletionType };
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, CompletionType %d)\n", EnvironmentHandle, ConnectionHandle,
          CompletionType);

    ret = ODBC_CALL( SQLTransact, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLBrowseConnect           [ODBC32.055]
 */
SQLRETURN WINAPI SQLBrowseConnect(SQLHDBC hdbc, SQLCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn,
                                  SQLCHAR *szConnStrOut, SQLSMALLINT cbConnStrOutMax,
                                  SQLSMALLINT *pcbConnStrOut)
{
    struct SQLBrowseConnect_params params = { hdbc, szConnStrIn, cbConnStrIn, szConnStrOut,
                                              cbConnStrOutMax, pcbConnStrOut };
    SQLRETURN ret;

    TRACE("(hdbc %p, szConnStrIn %s, cbConnStrIn %d, szConnStrOut %p, cbConnStrOutMax %d, pcbConnStrOut %p)\n",
          hdbc, debugstr_an((const char *)szConnStrIn, cbConnStrIn), cbConnStrIn, szConnStrOut, cbConnStrOutMax,
          pcbConnStrOut);

    ret = ODBC_CALL( SQLBrowseConnect, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLBulkOperations           [ODBC32.078]
 */
SQLRETURN WINAPI SQLBulkOperations(SQLHSTMT StatementHandle, SQLSMALLINT Operation)
{
    struct SQLBulkOperations_params params = { StatementHandle, Operation };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Operation %d)\n", StatementHandle, Operation);

    ret = ODBC_CALL( SQLBulkOperations, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColAttributes           [ODBC32.006]
 */
SQLRETURN WINAPI SQLColAttributes(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLUSMALLINT fDescType,
                                  SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT *pcbDesc,
                                  SQLLEN *pfDesc)
{
    struct SQLColAttributes_params params = { hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc };
    SQLRETURN ret;

    TRACE("(hstmt %p, icol %d, fDescType %d, rgbDesc %p, cbDescMax %d, pcbDesc %p, pfDesc %p)\n", hstmt, icol,
          fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);

    ret = ODBC_CALL( SQLColAttributes, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColumnPrivileges           [ODBC32.056]
 */
SQLRETURN WINAPI SQLColumnPrivileges(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                     SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szTableName,
                                     SQLSMALLINT cbTableName, SQLCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    struct SQLColumnPrivileges_params params = { hstmt, szCatalogName, cbCatalogName, szSchemaName,
                                                 cbSchemaName, szTableName, cbTableName, szColumnName, cbColumnName };
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d, szColumnName %s, cbColumnName %d)\n", hstmt,
          debugstr_an((const char *)szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_an((const char *)szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_an((const char *)szTableName, cbTableName), cbTableName,
          debugstr_an((const char *)szColumnName, cbColumnName), cbColumnName);

    ret = ODBC_CALL( SQLColumnPrivileges, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDescribeParam          [ODBC32.058]
 */
SQLRETURN WINAPI SQLDescribeParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, SQLSMALLINT *pfSqlType,
                                  SQLULEN *pcbParamDef, SQLSMALLINT *pibScale, SQLSMALLINT *pfNullable)
{
    struct SQLDescribeParam_params params = { hstmt, ipar, pfSqlType, pcbParamDef, pibScale, pfNullable };
    SQLRETURN ret;

    TRACE("(hstmt %p, ipar %d, pfSqlType %p, pcbParamDef %p, pibScale %p, pfNullable %p)\n", hstmt, ipar,
          pfSqlType, pcbParamDef, pibScale, pfNullable);

    ret = ODBC_CALL( SQLDescribeParam, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLExtendedFetch           [ODBC32.059]
 */
SQLRETURN WINAPI SQLExtendedFetch(SQLHSTMT hstmt, SQLUSMALLINT fFetchType, SQLLEN irow, SQLULEN *pcrow,
                                  SQLUSMALLINT *rgfRowStatus)
{
    struct SQLExtendedFetch_params params = { hstmt, fFetchType, irow, pcrow, rgfRowStatus };
    SQLRETURN ret;

    TRACE("(hstmt %p, fFetchType %d, irow %s, pcrow %p, rgfRowStatus %p)\n", hstmt, fFetchType, debugstr_sqllen(irow),
          pcrow, rgfRowStatus);

    ret = ODBC_CALL( SQLExtendedFetch, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLForeignKeys           [ODBC32.060]
 */
SQLRETURN WINAPI SQLForeignKeys(SQLHSTMT hstmt, SQLCHAR *szPkCatalogName, SQLSMALLINT cbPkCatalogName,
                                SQLCHAR *szPkSchemaName, SQLSMALLINT cbPkSchemaName, SQLCHAR *szPkTableName,
                                SQLSMALLINT cbPkTableName, SQLCHAR *szFkCatalogName,
                                SQLSMALLINT cbFkCatalogName, SQLCHAR *szFkSchemaName,
                                SQLSMALLINT cbFkSchemaName, SQLCHAR *szFkTableName, SQLSMALLINT cbFkTableName)
{
    struct SQLForeignKeys_params params = { hstmt, szPkCatalogName, cbPkCatalogName, szPkSchemaName,
                                            cbPkSchemaName, szPkTableName, cbPkTableName, szFkCatalogName,
                                            cbFkCatalogName, szFkSchemaName, cbFkSchemaName,
                                            szFkTableName, cbFkTableName };
    SQLRETURN ret;

    TRACE("(hstmt %p, szPkCatalogName %s, cbPkCatalogName %d, szPkSchemaName %s, cbPkSchemaName %d,"
          " szPkTableName %s, cbPkTableName %d, szFkCatalogName %s, cbFkCatalogName %d, szFkSchemaName %s,"
          " cbFkSchemaName %d, szFkTableName %s, cbFkTableName %d)\n", hstmt,
          debugstr_an((const char *)szPkCatalogName, cbPkCatalogName), cbPkCatalogName,
          debugstr_an((const char *)szPkSchemaName, cbPkSchemaName), cbPkSchemaName,
          debugstr_an((const char *)szPkTableName, cbPkTableName), cbPkTableName,
          debugstr_an((const char *)szFkCatalogName, cbFkCatalogName), cbFkCatalogName,
          debugstr_an((const char *)szFkSchemaName, cbFkSchemaName), cbFkSchemaName,
          debugstr_an((const char *)szFkTableName, cbFkTableName), cbFkTableName);

    ret = ODBC_CALL( SQLForeignKeys, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLMoreResults           [ODBC32.061]
 */
SQLRETURN WINAPI SQLMoreResults(SQLHSTMT StatementHandle)
{
    struct SQLMoreResults_params params = { StatementHandle };
    SQLRETURN ret;

    TRACE("(%p)\n", StatementHandle);

    ret = ODBC_CALL( SQLMoreResults, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNativeSql           [ODBC32.062]
 */
SQLRETURN WINAPI SQLNativeSql(SQLHDBC hdbc, SQLCHAR *szSqlStrIn, SQLINTEGER cbSqlStrIn, SQLCHAR *szSqlStr,
                              SQLINTEGER cbSqlStrMax, SQLINTEGER *pcbSqlStr)
{
    struct SQLNativeSql_params params = { hdbc, szSqlStrIn, cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr };
    SQLRETURN ret;

    TRACE("(hdbc %p, szSqlStrIn %s, cbSqlStrIn %d, szSqlStr %p, cbSqlStrMax %d, pcbSqlStr %p)\n", hdbc,
          debugstr_an((const char *)szSqlStrIn, cbSqlStrIn), cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr);

    ret = ODBC_CALL( SQLNativeSql, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNumParams           [ODBC32.063]
 */
SQLRETURN WINAPI SQLNumParams(SQLHSTMT hstmt, SQLSMALLINT *pcpar)
{
    struct SQLNumParams_params params = { hstmt, pcpar };
    SQLRETURN ret;

    TRACE("(hstmt %p, pcpar %p)\n", hstmt, pcpar);

    ret = ODBC_CALL( SQLNumParams, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLParamOptions           [ODBC32.064]
 */
SQLRETURN WINAPI SQLParamOptions(SQLHSTMT hstmt, SQLULEN crow, SQLULEN *pirow)
{
    struct SQLParamOptions_params params = { hstmt, crow, pirow };
    SQLRETURN ret;

    TRACE("(hstmt %p, crow %s, pirow %p)\n", hstmt, debugstr_sqlulen(crow), pirow);

    ret = ODBC_CALL( SQLParamOptions, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPrimaryKeys           [ODBC32.065]
 */
SQLRETURN WINAPI SQLPrimaryKeys(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szTableName,
                                SQLSMALLINT cbTableName)
{
    struct SQLPrimaryKeys_params params = { hstmt, szCatalogName, cbCatalogName, szSchemaName,
                                            cbSchemaName, szTableName, cbTableName };
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d)\n", hstmt,
          debugstr_an((const char *)szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_an((const char *)szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_an((const char *)szTableName, cbTableName), cbTableName);

    ret = ODBC_CALL( SQLPrimaryKeys, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLProcedureColumns           [ODBC32.066]
 */
SQLRETURN WINAPI SQLProcedureColumns(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                     SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szProcName,
                                     SQLSMALLINT cbProcName, SQLCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    struct SQLProcedureColumns_params params = { hstmt, szCatalogName, cbCatalogName, szSchemaName,
                                                 cbSchemaName, szProcName, cbProcName,
                                                 szColumnName, cbColumnName };
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szProcName %s,"
          " cbProcName %d, szColumnName %s, cbColumnName %d)\n", hstmt,
          debugstr_an((const char *)szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_an((const char *)szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_an((const char *)szProcName, cbProcName), cbProcName,
          debugstr_an((const char *)szColumnName, cbColumnName), cbColumnName);

    ret = ODBC_CALL( SQLProcedureColumns, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLProcedures           [ODBC32.067]
 */
SQLRETURN WINAPI SQLProcedures(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                               SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szProcName,
                               SQLSMALLINT cbProcName)
{
    struct SQLProcedures_params params = { hstmt, szCatalogName, cbCatalogName, szSchemaName,
                                           cbSchemaName, szProcName, cbProcName };
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szProcName %s,"
          " cbProcName %d)\n", hstmt,
          debugstr_an((const char *)szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_an((const char *)szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_an((const char *)szProcName, cbProcName), cbProcName);

    ret = ODBC_CALL( SQLProcedures, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetPos           [ODBC32.068]
 */
SQLRETURN WINAPI SQLSetPos(SQLHSTMT hstmt, SQLSETPOSIROW irow, SQLUSMALLINT fOption, SQLUSMALLINT fLock)
{
    struct SQLSetPos_params params = { hstmt, irow, fOption, fLock };
    SQLRETURN ret;

    TRACE("(hstmt %p, irow %s, fOption %d, fLock %d)\n", hstmt, debugstr_sqlulen(irow), fOption, fLock);

    ret = ODBC_CALL( SQLSetPos, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTablePrivileges           [ODBC32.070]
 */
SQLRETURN WINAPI SQLTablePrivileges(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                    SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szTableName,
                                    SQLSMALLINT cbTableName)
{
    struct SQLTablePrivileges_params params = { hstmt, szCatalogName, cbCatalogName, szSchemaName,
                                                cbSchemaName, szTableName, cbTableName };
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d)\n", hstmt,
          debugstr_an((const char *)szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_an((const char *)szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_an((const char *)szTableName, cbTableName), cbTableName);

    ret = ODBC_CALL( SQLTablePrivileges, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDrivers           [ODBC32.071]
 */
SQLRETURN WINAPI SQLDrivers(SQLHENV EnvironmentHandle, SQLUSMALLINT fDirection, SQLCHAR *szDriverDesc,
                            SQLSMALLINT cbDriverDescMax, SQLSMALLINT *pcbDriverDesc,
                            SQLCHAR *szDriverAttributes, SQLSMALLINT cbDriverAttrMax,
                            SQLSMALLINT *pcbDriverAttr)
{
    struct SQLDrivers_params params = { EnvironmentHandle, fDirection, szDriverDesc, cbDriverDescMax,
                                        pcbDriverDesc, szDriverAttributes, cbDriverAttrMax, pcbDriverAttr };
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, szDriverDesc %p, cbDriverDescMax %d, pcbDriverDesc %p,"
          " DriverAttributes %p, cbDriverAttrMax %d, pcbDriverAttr %p)\n", EnvironmentHandle, fDirection,
          szDriverDesc, cbDriverDescMax, pcbDriverDesc, szDriverAttributes, cbDriverAttrMax, pcbDriverAttr);

    ret = ODBC_CALL( SQLDrivers, &params );

    if (ret == SQL_NO_DATA && fDirection == SQL_FETCH_FIRST)
        ERR_(winediag)("No ODBC drivers could be found. Check the settings for your libodbc provider.\n");

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLBindParameter           [ODBC32.072]
 */
SQLRETURN WINAPI SQLBindParameter(SQLHSTMT hstmt, SQLUSMALLINT ipar, SQLSMALLINT fParamType,
                                  SQLSMALLINT fCType, SQLSMALLINT fSqlType, SQLULEN cbColDef,
                                  SQLSMALLINT ibScale, SQLPOINTER rgbValue, SQLLEN cbValueMax,
                                  SQLLEN *pcbValue)
{
    struct SQLBindParameter_params params = { hstmt, ipar, fParamType, fCType, fSqlType, cbColDef,
                                              ibScale, rgbValue, cbValueMax, pcbValue };
    SQLRETURN ret;

    TRACE("(hstmt %p, ipar %d, fParamType %d, fCType %d, fSqlType %d, cbColDef %s, ibScale %d, rgbValue %p,"
          " cbValueMax %s, pcbValue %p)\n", hstmt, ipar, fParamType, fCType, fSqlType, debugstr_sqlulen(cbColDef),
          ibScale, rgbValue, debugstr_sqllen(cbValueMax), pcbValue);

    ret = ODBC_CALL( SQLBindParameter, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDriverConnect           [ODBC32.041]
 */
SQLRETURN WINAPI SQLDriverConnect(SQLHDBC hdbc, SQLHWND hwnd, SQLCHAR *ConnectionString, SQLSMALLINT Length,
                                  SQLCHAR *conn_str_out, SQLSMALLINT conn_str_out_max,
                                  SQLSMALLINT *ptr_conn_str_out, SQLUSMALLINT driver_completion)
{
    struct SQLDriverConnect_params params = { hdbc, hwnd, ConnectionString, Length, conn_str_out,
                                              conn_str_out_max, ptr_conn_str_out, driver_completion };
    SQLRETURN ret;

    TRACE("(hdbc %p, hwnd %p, ConnectionString %s, Length %d, conn_str_out %p, conn_str_out_max %d,"
          " ptr_conn_str_out %p, driver_completion %d)\n", hdbc, hwnd,
          debugstr_an((const char *)ConnectionString, Length), Length, conn_str_out, conn_str_out_max,
          ptr_conn_str_out, driver_completion);

    ret = ODBC_CALL( SQLDriverConnect, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetScrollOptions           [ODBC32.069]
 */
SQLRETURN WINAPI SQLSetScrollOptions(SQLHSTMT statement_handle, SQLUSMALLINT f_concurrency, SQLLEN crow_keyset,
                                     SQLUSMALLINT crow_rowset)
{
    struct SQLSetScrollOptions_params params = { statement_handle, f_concurrency, crow_keyset, crow_rowset };
    SQLRETURN ret;

    TRACE("(statement_handle %p, f_concurrency %d, crow_keyset %s, crow_rowset %d)\n", statement_handle,
          f_concurrency, debugstr_sqllen(crow_keyset), crow_rowset);

    ret = ODBC_CALL( SQLSetScrollOptions, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

static BOOL SQLColAttributes_KnownStringAttribute(SQLUSMALLINT fDescType)
{
    static const SQLUSMALLINT attrList[] =
    {
        SQL_COLUMN_OWNER_NAME,
        SQL_COLUMN_QUALIFIER_NAME,
        SQL_COLUMN_LABEL,
        SQL_COLUMN_NAME,
        SQL_COLUMN_TABLE_NAME,
        SQL_COLUMN_TYPE_NAME,
        SQL_DESC_BASE_COLUMN_NAME,
        SQL_DESC_BASE_TABLE_NAME,
        SQL_DESC_CATALOG_NAME,
        SQL_DESC_LABEL,
        SQL_DESC_LITERAL_PREFIX,
        SQL_DESC_LITERAL_SUFFIX,
        SQL_DESC_LOCAL_TYPE_NAME,
        SQL_DESC_NAME,
        SQL_DESC_SCHEMA_NAME,
        SQL_DESC_TABLE_NAME,
        SQL_DESC_TYPE_NAME,
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(attrList); i++) {
        if (attrList[i] == fDescType) return TRUE;
    }
    return FALSE;
}

/*************************************************************************
 *				SQLColAttributesW          [ODBC32.106]
 */
SQLRETURN WINAPI SQLColAttributesW(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLUSMALLINT fDescType,
                                   SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT *pcbDesc,
                                   SQLLEN *pfDesc)
{
    struct SQLColAttributesW_params params = { hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc };
    SQLRETURN ret;

    TRACE("(hstmt %p, icol %d, fDescType %d, rgbDesc %p, cbDescMax %d, pcbDesc %p, pfDesc %p)\n", hstmt, icol,
          fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);

    ret = ODBC_CALL( SQLColAttributesW, &params );

    if (ret == SQL_SUCCESS && SQLColAttributes_KnownStringAttribute(fDescType) && rgbDesc && pcbDesc &&
        *pcbDesc != lstrlenW(rgbDesc) * 2)
    {
        TRACE("CHEAT: resetting name length for ADO\n");
        *pcbDesc = lstrlenW(rgbDesc) * 2;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLConnectW          [ODBC32.107]
 */
SQLRETURN WINAPI SQLConnectW(SQLHDBC ConnectionHandle, WCHAR *ServerName, SQLSMALLINT NameLength1,
                             WCHAR *UserName, SQLSMALLINT NameLength2, WCHAR *Authentication,
                             SQLSMALLINT NameLength3)
{
    struct SQLConnectW_params params = { ConnectionHandle, ServerName, NameLength1, UserName, NameLength2,
                                         Authentication, NameLength3 };
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, ServerName %s, NameLength1 %d, UserName %s, NameLength2 %d, Authentication %s,"
          " NameLength3 %d)\n", ConnectionHandle, debugstr_wn(ServerName, NameLength1), NameLength1,
          debugstr_wn(UserName, NameLength2), NameLength2, debugstr_wn(Authentication, NameLength3), NameLength3);

    ret = ODBC_CALL( SQLConnectW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDescribeColW          [ODBC32.108]
 */
SQLRETURN WINAPI SQLDescribeColW(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, WCHAR *ColumnName,
                                 SQLSMALLINT BufferLength, SQLSMALLINT *NameLength, SQLSMALLINT *DataType,
                                 SQLULEN *ColumnSize, SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
    struct SQLDescribeColW_params params = { StatementHandle, ColumnNumber, ColumnName, BufferLength,
                                             NameLength, DataType, ColumnSize, DecimalDigits, Nullable };
    SQLSMALLINT dummy;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, ColumnName %p, BufferLength %d, NameLength %p, DataType %p,"
          " ColumnSize %p, DecimalDigits %p, Nullable %p)\n", StatementHandle, ColumnNumber, ColumnName,
          BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);

    if (!NameLength) NameLength = &dummy; /* workaround for drivers that don't accept NULL NameLength */

    ret = ODBC_CALL( SQLDescribeColW, &params );
    if (ret >= 0)
    {
        if (ColumnName && NameLength) TRACE("ColumnName %s\n", debugstr_wn(ColumnName, *NameLength));
        if (DataType) TRACE("DataType %d\n", *DataType);
        if (ColumnSize) TRACE("ColumnSize %s\n", debugstr_sqlulen(*ColumnSize));
        if (DecimalDigits) TRACE("DecimalDigits %d\n", *DecimalDigits);
        if (Nullable) TRACE("Nullable %d\n", *Nullable);
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLErrorW          [ODBC32.110]
 */
SQLRETURN WINAPI SQLErrorW(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
                           WCHAR *Sqlstate, SQLINTEGER *NativeError, WCHAR *MessageText,
                           SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    struct SQLErrorW_params params = { EnvironmentHandle, ConnectionHandle, StatementHandle, Sqlstate,
                                       NativeError, MessageText, BufferLength, TextLength };
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, StatementHandle %p, Sqlstate %p, NativeError %p,"
          " MessageText %p, BufferLength %d, TextLength %p)\n", EnvironmentHandle, ConnectionHandle,
          StatementHandle, Sqlstate, NativeError, MessageText, BufferLength, TextLength);

    ret = ODBC_CALL( SQLErrorW, &params );

    if (ret == SQL_SUCCESS)
    {
        TRACE(" SQLState %s\n", debugstr_wn(Sqlstate, 5));
        TRACE(" Error %d\n", *NativeError);
        TRACE(" MessageText %s\n", debugstr_wn(MessageText, *TextLength));
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLExecDirectW          [ODBC32.111]
 */
SQLRETURN WINAPI SQLExecDirectW(SQLHSTMT StatementHandle, WCHAR *StatementText, SQLINTEGER TextLength)
{
    struct SQLExecDirectW_params params = { StatementHandle, StatementText, TextLength };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_wn(StatementText, TextLength), TextLength);

    ret = ODBC_CALL( SQLExecDirectW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetCursorNameW          [ODBC32.117]
 */
SQLRETURN WINAPI SQLGetCursorNameW(SQLHSTMT StatementHandle, WCHAR *CursorName, SQLSMALLINT BufferLength,
                                   SQLSMALLINT *NameLength)
{
    struct SQLGetCursorNameW_params params = { StatementHandle, CursorName, BufferLength, NameLength };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CursorName %p, BufferLength %d, NameLength %p)\n", StatementHandle, CursorName,
          BufferLength, NameLength);

    ret = ODBC_CALL( SQLGetCursorNameW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPrepareW          [ODBC32.119]
 */
SQLRETURN WINAPI SQLPrepareW(SQLHSTMT StatementHandle, WCHAR *StatementText, SQLINTEGER TextLength)
{
    struct SQLPrepareW_params params = { StatementHandle, StatementText, TextLength };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_wn(StatementText, TextLength), TextLength);

    ret = ODBC_CALL( SQLPrepareW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetCursorNameW          [ODBC32.121]
 */
SQLRETURN WINAPI SQLSetCursorNameW(SQLHSTMT StatementHandle, WCHAR *CursorName, SQLSMALLINT NameLength)
{
    struct SQLSetCursorNameW_params params = { StatementHandle, CursorName, NameLength };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CursorName %s, NameLength %d)\n", StatementHandle,
          debugstr_wn(CursorName, NameLength), NameLength);

    ret = ODBC_CALL( SQLSetCursorNameW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColAttributeW          [ODBC32.127]
 */
SQLRETURN WINAPI SQLColAttributeW(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
                                  SQLUSMALLINT FieldIdentifier, SQLPOINTER CharacterAttribute,
                                  SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                  SQLLEN *NumericAttribute)
{
    struct SQLColAttributeW_params params = { StatementHandle, ColumnNumber, FieldIdentifier,
                                              CharacterAttribute, BufferLength, StringLength,
                                              NumericAttribute };
    SQLRETURN ret;

    TRACE("StatementHandle %p ColumnNumber %d FieldIdentifier %d CharacterAttribute %p BufferLength %d"
          " StringLength %p NumericAttribute %p\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttribute, BufferLength, StringLength, NumericAttribute);

    ret = ODBC_CALL( SQLColAttributeW, &params );

    if (ret == SQL_SUCCESS && CharacterAttribute != NULL && SQLColAttributes_KnownStringAttribute(FieldIdentifier) &&
        StringLength && *StringLength != lstrlenW(CharacterAttribute) * 2)
    {
        TRACE("CHEAT: resetting name length for ADO\n");
        *StringLength = lstrlenW(CharacterAttribute) * 2;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetConnectAttrW          [ODBC32.132]
 */
SQLRETURN WINAPI SQLGetConnectAttrW(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                    SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct SQLGetConnectAttrW_params params = { ConnectionHandle, Attribute, Value,
                                                BufferLength, StringLength };
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          Attribute, Value, BufferLength, StringLength);

    ret = ODBC_CALL( SQLGetConnectAttrW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDescFieldW          [ODBC32.133]
 */
SQLRETURN WINAPI SQLGetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                  SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct SQLGetDescFieldW_params params = { DescriptorHandle, RecNumber, FieldIdentifier, Value,
                                              BufferLength, StringLength };
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d, StringLength %p)\n",
          DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);

    ret = ODBC_CALL( SQLGetDescFieldW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDescRecW          [ODBC32.134]
 */
SQLRETURN WINAPI SQLGetDescRecW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, WCHAR *Name,
                                SQLSMALLINT BufferLength, SQLSMALLINT *StringLength, SQLSMALLINT *Type,
                                SQLSMALLINT *SubType, SQLLEN *Length, SQLSMALLINT *Precision,
                                SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
    struct SQLGetDescRecW_params params = { DescriptorHandle, RecNumber, Name, BufferLength, StringLength,
                                            Type, SubType, Length, Precision, Scale, Nullable };
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, Name %p, BufferLength %d, StringLength %p, Type %p, SubType %p,"
          " Length %p, Precision %p, Scale %p, Nullable %p)\n", DescriptorHandle, RecNumber, Name, BufferLength,
          StringLength, Type, SubType, Length, Precision, Scale, Nullable);

    ret = ODBC_CALL( SQLGetDescRecW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDiagFieldW          [ODBC32.135]
 */
SQLRETURN WINAPI SQLGetDiagFieldW(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                  SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
                                  SQLSMALLINT *StringLength)
{
    struct SQLGetDiagFieldW_params params = { HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo,
                                              BufferLength, StringLength };
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, DiagIdentifier %d, DiagInfo %p, BufferLength %d,"
          " StringLength %p)\n", HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);

    ret = ODBC_CALL( SQLGetDiagFieldW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDiagRecW           [ODBC32.136]
 */
SQLRETURN WINAPI SQLGetDiagRecW(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                WCHAR *Sqlstate, SQLINTEGER *NativeError, WCHAR *MessageText,
                                SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    struct SQLGetDiagRecW_params params = { HandleType, Handle, RecNumber, Sqlstate, NativeError,
                                            MessageText, BufferLength, TextLength };
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, Sqlstate %p, NativeError %p, MessageText %p, BufferLength %d,"
          " TextLength %p)\n", HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength,
          TextLength);

    ret = ODBC_CALL( SQLGetDiagRecW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetStmtAttrW          [ODBC32.138]
 */
SQLRETURN WINAPI SQLGetStmtAttrW(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                 SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct SQLGetStmtAttrW_params params = { StatementHandle, Attribute, Value, BufferLength, StringLength };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", StatementHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!Value)
    {
        WARN("Unexpected NULL Value return address\n");
        return SQL_ERROR;
    }

    ret = ODBC_CALL( SQLGetStmtAttrW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectAttrW          [ODBC32.139]
 */
SQLRETURN WINAPI SQLSetConnectAttrW(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                    SQLINTEGER StringLength)
{
    struct SQLSetConnectAttrW_params params = { ConnectionHandle, Attribute, Value, StringLength };
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, StringLength %d)\n", ConnectionHandle, Attribute, Value,
          StringLength);

    ret = ODBC_CALL( SQLSetConnectAttrW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColumnsW          [ODBC32.140]
 */
SQLRETURN WINAPI SQLColumnsW(SQLHSTMT StatementHandle, WCHAR *CatalogName, SQLSMALLINT NameLength1,
                             WCHAR *SchemaName, SQLSMALLINT NameLength2, WCHAR *TableName,
                             SQLSMALLINT NameLength3, WCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    struct SQLColumnsW_params params = { StatementHandle, CatalogName, NameLength1, SchemaName,
                                         NameLength2, TableName, NameLength3, ColumnName, NameLength4 };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_wn(CatalogName, NameLength1), NameLength1, debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3, debugstr_wn(ColumnName, NameLength4), NameLength4);

    ret = ODBC_CALL( SQLColumnsW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDriverConnectW          [ODBC32.141]
 */
SQLRETURN WINAPI SQLDriverConnectW(SQLHDBC ConnectionHandle, SQLHWND WindowHandle, WCHAR *InConnectionString,
                                   SQLSMALLINT Length, WCHAR *OutConnectionString, SQLSMALLINT BufferLength,
                                   SQLSMALLINT *Length2, SQLUSMALLINT DriverCompletion)
{
    struct SQLDriverConnectW_params params = { ConnectionHandle, WindowHandle, InConnectionString, Length,
                                               OutConnectionString, BufferLength, Length2, DriverCompletion };
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, WindowHandle %p, InConnectionString %s, Length %d, OutConnectionString %p,"
          " BufferLength %d, Length2 %p, DriverCompletion %d)\n", ConnectionHandle, WindowHandle,
          debugstr_wn(InConnectionString, Length), Length, OutConnectionString, BufferLength, Length2,
          DriverCompletion);

    ret = ODBC_CALL( SQLDriverConnectW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetConnectOptionW      [ODBC32.142]
 */
SQLRETURN WINAPI SQLGetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    struct SQLGetConnectOptionW_params params = { ConnectionHandle, Option, Value };
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Option %d, Value %p)\n", ConnectionHandle, Option, Value);

    ret = ODBC_CALL( SQLGetConnectOptionW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetInfoW          [ODBC32.145]
 */
SQLRETURN WINAPI SQLGetInfoW(SQLHDBC ConnectionHandle, SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
                             SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
    struct SQLGetInfoW_params params = { ConnectionHandle, InfoType, InfoValue, BufferLength, StringLength };
    SQLRETURN ret;

    TRACE("(ConnectionHandle, %p, InfoType %d, InfoValue %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          InfoType, InfoValue, BufferLength, StringLength);

    ret = ODBC_CALL( SQLGetInfoW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetTypeInfoW          [ODBC32.147]
 */
SQLRETURN WINAPI SQLGetTypeInfoW(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    struct SQLGetTypeInfoW_params params = { StatementHandle, DataType };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, DataType %d)\n", StatementHandle, DataType);

    ret = ODBC_CALL( SQLGetTypeInfoW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectOptionW          [ODBC32.150]
 */
SQLRETURN WINAPI SQLSetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLLEN Value)
{
    struct SQLSetConnectOptionW_params params = { ConnectionHandle, Option, Value };
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Option %d, Value %s)\n", ConnectionHandle, Option, debugstr_sqllen(Value));

    ret = ODBC_CALL( SQLSetConnectOptionW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSpecialColumnsW          [ODBC32.152]
 */
SQLRETURN WINAPI SQLSpecialColumnsW(SQLHSTMT StatementHandle, SQLUSMALLINT IdentifierType,
                                    SQLWCHAR *CatalogName, SQLSMALLINT NameLength1, SQLWCHAR *SchemaName,
                                    SQLSMALLINT NameLength2, SQLWCHAR *TableName, SQLSMALLINT NameLength3,
                                    SQLUSMALLINT Scope, SQLUSMALLINT Nullable)
{
    struct SQLSpecialColumnsW_params params = { StatementHandle, IdentifierType, CatalogName, NameLength1,
                                                SchemaName, NameLength2, TableName, NameLength3, Scope, Nullable };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, IdentifierType %d, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d,"
          " TableName %s, NameLength3 %d, Scope %d, Nullable %d)\n", StatementHandle, IdentifierType,
          debugstr_wn(CatalogName, NameLength1), NameLength1, debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3, Scope, Nullable);

    ret = ODBC_CALL( SQLSpecialColumnsW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLStatisticsW          [ODBC32.153]
 */
SQLRETURN WINAPI SQLStatisticsW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                                SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
    struct SQLStatisticsW_params params = { StatementHandle, CatalogName, NameLength1, SchemaName,
                                            NameLength2, TableName, NameLength3, Unique, Reserved };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d SchemaName %s, NameLength2 %d, TableName %s"
          " NameLength3 %d, Unique %d, Reserved %d)\n", StatementHandle,
          debugstr_wn(CatalogName, NameLength1), NameLength1, debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3, Unique, Reserved);

    ret = ODBC_CALL( SQLStatisticsW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTablesW          [ODBC32.154]
 */
SQLRETURN WINAPI SQLTablesW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                            SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                            SQLSMALLINT NameLength3, SQLWCHAR *TableType, SQLSMALLINT NameLength4)
{
    struct SQLTablesW_params params = { StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2,
                                        TableName, NameLength3, TableType, NameLength4 };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, TableType %s, NameLength4 %d)\n", StatementHandle,
          debugstr_wn(CatalogName, NameLength1), NameLength1, debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3, debugstr_wn(TableType, NameLength4), NameLength4);

    ret = ODBC_CALL( SQLTablesW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLBrowseConnectW          [ODBC32.155]
 */
SQLRETURN WINAPI SQLBrowseConnectW(SQLHDBC hdbc, SQLWCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn,
                                   SQLWCHAR *szConnStrOut, SQLSMALLINT cbConnStrOutMax,
                                   SQLSMALLINT *pcbConnStrOut)
{
    struct SQLBrowseConnectW_params params = { hdbc, szConnStrIn, cbConnStrIn, szConnStrOut,
                                               cbConnStrOutMax, pcbConnStrOut };
    SQLRETURN ret;

    TRACE("(hdbc %p, szConnStrIn %s, cbConnStrIn %d, szConnStrOut %p, cbConnStrOutMax %d, pcbConnStrOut %p)\n",
          hdbc, debugstr_wn(szConnStrIn, cbConnStrIn), cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut);

    ret = ODBC_CALL( SQLBrowseConnectW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColumnPrivilegesW          [ODBC32.156]
 */
SQLRETURN WINAPI SQLColumnPrivilegesW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                      SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szTableName,
                                      SQLSMALLINT cbTableName, SQLWCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    struct SQLColumnPrivilegesW_params params = { hstmt, szCatalogName, cbCatalogName, szSchemaName,
                                                  cbSchemaName, szTableName, cbTableName, szColumnName,
                                                  cbColumnName };
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d, szColumnName %s, cbColumnName %d)\n", hstmt,
          debugstr_wn(szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_wn(szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_wn(szTableName, cbTableName), cbTableName,
          debugstr_wn(szColumnName, cbColumnName), cbColumnName);

    ret = ODBC_CALL( SQLColumnPrivilegesW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDataSourcesW          [ODBC32.157]
 */
SQLRETURN WINAPI SQLDataSourcesW(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, WCHAR *ServerName,
                                 SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, WCHAR *Description,
                                 SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    struct SQLDataSourcesW_params params = { EnvironmentHandle, Direction, ServerName, BufferLength1,
                                             NameLength1, Description, BufferLength2, NameLength2 };
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, ServerName %p, BufferLength1 %d, NameLength1 %p, Description %p,"
          " BufferLength2 %d, NameLength2 %p)\n", EnvironmentHandle, Direction, ServerName, BufferLength1,
          NameLength1, Description, BufferLength2, NameLength2);

    ret = ODBC_CALL( SQLDataSourcesW, &params );

    if (ret >= 0 && TRACE_ON(odbc))
    {
        if (ServerName && NameLength1 && *NameLength1 > 0)
            TRACE(" DataSource %s", debugstr_wn(ServerName, *NameLength1));
        if (Description && NameLength2 && *NameLength2 > 0)
            TRACE(" Description %s", debugstr_wn(Description, *NameLength2));
        TRACE("\n");
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLForeignKeysW          [ODBC32.160]
 */
SQLRETURN WINAPI SQLForeignKeysW(SQLHSTMT hstmt, SQLWCHAR *szPkCatalogName, SQLSMALLINT cbPkCatalogName,
                                 SQLWCHAR *szPkSchemaName, SQLSMALLINT cbPkSchemaName, SQLWCHAR *szPkTableName,
                                 SQLSMALLINT cbPkTableName, SQLWCHAR *szFkCatalogName,
                                 SQLSMALLINT cbFkCatalogName, SQLWCHAR *szFkSchemaName,
                                 SQLSMALLINT cbFkSchemaName, SQLWCHAR *szFkTableName, SQLSMALLINT cbFkTableName)
{
    struct SQLForeignKeysW_params params = { hstmt, szPkCatalogName, cbPkCatalogName, szPkSchemaName,
                                             cbPkSchemaName, szPkTableName, cbPkTableName, szFkCatalogName,
                                             cbFkCatalogName, szFkSchemaName, cbFkSchemaName, szFkTableName,
                                             cbFkTableName };
    SQLRETURN ret;

    TRACE("(hstmt %p, szPkCatalogName %s, cbPkCatalogName %d, szPkSchemaName %s, cbPkSchemaName %d,"
          " szPkTableName %s, cbPkTableName %d, szFkCatalogName %s, cbFkCatalogName %d, szFkSchemaName %s,"
          " cbFkSchemaName %d, szFkTableName %s, cbFkTableName %d)\n", hstmt,
          debugstr_wn(szPkCatalogName, cbPkCatalogName), cbPkCatalogName,
          debugstr_wn(szPkSchemaName, cbPkSchemaName), cbPkSchemaName,
          debugstr_wn(szPkTableName, cbPkTableName), cbPkTableName,
          debugstr_wn(szFkCatalogName, cbFkCatalogName), cbFkCatalogName,
          debugstr_wn(szFkSchemaName, cbFkSchemaName), cbFkSchemaName,
          debugstr_wn(szFkTableName, cbFkTableName), cbFkTableName);

    ret = ODBC_CALL( SQLForeignKeysW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNativeSqlW          [ODBC32.162]
 */
SQLRETURN WINAPI SQLNativeSqlW(SQLHDBC hdbc, SQLWCHAR *szSqlStrIn, SQLINTEGER cbSqlStrIn, SQLWCHAR *szSqlStr,
                               SQLINTEGER cbSqlStrMax, SQLINTEGER *pcbSqlStr)
{
    struct SQLNativeSqlW_params params = { hdbc, szSqlStrIn, cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr };
    SQLRETURN ret;

    TRACE("(hdbc %p, szSqlStrIn %s, cbSqlStrIn %d, szSqlStr %p, cbSqlStrMax %d, pcbSqlStr %p)\n", hdbc,
          debugstr_wn(szSqlStrIn, cbSqlStrIn), cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr);

    ret = ODBC_CALL( SQLNativeSqlW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPrimaryKeysW          [ODBC32.165]
 */
SQLRETURN WINAPI SQLPrimaryKeysW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                 SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szTableName,
                                 SQLSMALLINT cbTableName)
{
    struct SQLPrimaryKeysW_params params = { hstmt, szCatalogName, cbCatalogName, szSchemaName,
                                             cbSchemaName, szTableName, cbTableName };
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d)\n", hstmt,
          debugstr_wn(szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_wn(szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_wn(szTableName, cbTableName), cbTableName);

    ret = ODBC_CALL( SQLPrimaryKeysW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLProcedureColumnsW          [ODBC32.166]
 */
SQLRETURN WINAPI SQLProcedureColumnsW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                      SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szProcName,
                                      SQLSMALLINT cbProcName, SQLWCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    struct SQLProcedureColumnsW_params params = { hstmt, szCatalogName, cbCatalogName, szSchemaName,
                                                  cbSchemaName, szProcName, cbProcName,
                                                  szColumnName, cbColumnName };
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szProcName %s,"
          " cbProcName %d, szColumnName %s, cbColumnName %d)\n", hstmt,
          debugstr_wn(szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_wn(szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_wn(szProcName, cbProcName), cbProcName,
          debugstr_wn(szColumnName, cbColumnName), cbColumnName);

    ret = ODBC_CALL( SQLProcedureColumnsW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLProceduresW          [ODBC32.167]
 */
SQLRETURN WINAPI SQLProceduresW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szProcName,
                                SQLSMALLINT cbProcName)
{
    struct SQLProceduresW_params params = { hstmt, szCatalogName, cbCatalogName, szSchemaName,
                                            cbSchemaName, szProcName, cbProcName };
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szProcName %s,"
          " cbProcName %d)\n", hstmt, debugstr_wn(szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_wn(szSchemaName, cbSchemaName), cbSchemaName, debugstr_wn(szProcName, cbProcName), cbProcName);

    ret = ODBC_CALL( SQLProceduresW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTablePrivilegesW          [ODBC32.170]
 */
SQLRETURN WINAPI SQLTablePrivilegesW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                     SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szTableName,
                                     SQLSMALLINT cbTableName)
{
    struct SQLTablePrivilegesW_params params = { hstmt, szCatalogName, cbCatalogName, szSchemaName,
                                                 cbSchemaName, szTableName, cbTableName };
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d)\n", hstmt, debugstr_wn(szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_wn(szSchemaName, cbSchemaName), cbSchemaName, debugstr_wn(szTableName, cbTableName), cbTableName);

    ret = ODBC_CALL( SQLTablePrivilegesW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDriversW          [ODBC32.171]
 */
SQLRETURN WINAPI SQLDriversW(SQLHENV EnvironmentHandle, SQLUSMALLINT fDirection, SQLWCHAR *szDriverDesc,
                             SQLSMALLINT cbDriverDescMax, SQLSMALLINT *pcbDriverDesc,
                             SQLWCHAR *szDriverAttributes, SQLSMALLINT cbDriverAttrMax,
                             SQLSMALLINT *pcbDriverAttr)
{
    struct SQLDriversW_params params = { EnvironmentHandle, fDirection, szDriverDesc, cbDriverDescMax,
                                         pcbDriverDesc, szDriverAttributes, cbDriverAttrMax, pcbDriverAttr };
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, szDriverDesc %p, cbDriverDescMax %d, pcbDriverDesc %p,"
          " DriverAttributes %p, cbDriverAttrMax %d, pcbDriverAttr %p)\n", EnvironmentHandle, fDirection,
          szDriverDesc, cbDriverDescMax, pcbDriverDesc, szDriverAttributes, cbDriverAttrMax, pcbDriverAttr);

    ret = ODBC_CALL( SQLDriversW, &params );

    if (ret == SQL_NO_DATA && fDirection == SQL_FETCH_FIRST)
        ERR_(winediag)("No ODBC drivers could be found. Check the settings for your libodbc provider.\n");

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetDescFieldW          [ODBC32.173]
 */
SQLRETURN WINAPI SQLSetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                  SQLPOINTER Value, SQLINTEGER BufferLength)
{
    struct SQLSetDescFieldW_params params = { DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength };
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d)\n", DescriptorHandle,
          RecNumber, FieldIdentifier, Value, BufferLength);

    ret = ODBC_CALL( SQLSetDescFieldW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetStmtAttrW          [ODBC32.176]
 */
SQLRETURN WINAPI SQLSetStmtAttrW(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                 SQLINTEGER StringLength)
{
    struct SQLSetStmtAttrW_params params = { StatementHandle, Attribute, Value, StringLength };
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, StringLength %d)\n", StatementHandle, Attribute, Value,
          StringLength);

    ret = ODBC_CALL( SQLSetStmtAttrW, &params );
    if (ret == SQL_ERROR && (Attribute == SQL_ROWSET_SIZE || Attribute == SQL_ATTR_ROW_ARRAY_SIZE))
    {
        TRACE("CHEAT: returning SQL_SUCCESS to ADO\n");
        return SQL_SUCCESS;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDiagRecA           [ODBC32.236]
 */
SQLRETURN WINAPI SQLGetDiagRecA(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                SQLCHAR *Sqlstate, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                                SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    struct SQLGetDiagRecA_params params = { HandleType, Handle, RecNumber, Sqlstate, NativeError,
                                            MessageText, BufferLength, TextLength };
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, Sqlstate %p, NativeError %p, MessageText %p, BufferLength %d,"
          " TextLength %p)\n", HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength,
          TextLength);

    ret = ODBC_CALL( SQLGetDiagRecA, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}


/***********************************************************************
 * DllMain [Internal] Initializes the internal 'ODBC32.DLL'.
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, LPVOID reserved)
{
    TRACE("proxy ODBC: %p,%lx,%p\n", hinstDLL, reason, reserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
       DisableThreadLibraryCalls(hinstDLL);
        if (!NtQueryVirtualMemory( GetCurrentProcess(), hinstDLL, MemoryWineUnixFuncs,
                                   &odbc_handle, sizeof(odbc_handle), NULL ) &&
            !__wine_unix_call( odbc_handle, process_attach, NULL))
        {
            ODBC_ReplicateToRegistry();
        }
       break;

    case DLL_PROCESS_DETACH:
      if (reserved) break;
      __wine_unix_call( odbc_handle, process_detach, NULL );
    }

    return TRUE;
}
