package odbc32;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "SQLAllocConnect" => ["long",  ["long", "ptr"]],
    "SQLAllocEnv" => ["long",  ["ptr"]],
    "SQLAllocStmt" => ["long",  ["long", "ptr"]],
    "SQLBindCol" => ["long",  ["long", "long", "long", "ptr", "long", "ptr"]],
    "SQLCancel" => ["long",  ["long"]],
    "SQLColAttributes" => ["long",  ["long", "long", "long", "ptr", "long", "ptr", "ptr"]],
    "SQLConnect" => ["long",  ["long", "ptr", "long", "ptr", "long", "ptr", "long"]],
    "SQLDescribeCol" => ["long",  ["long", "long", "ptr", "long", "ptr", "ptr", "ptr", "ptr", "ptr"]],
    "SQLDisconnect" => ["long",  ["long"]],
    "SQLError" => ["long",  ["long", "long", "long", "ptr", "ptr", "ptr", "long", "ptr"]],
    "SQLExecDirect" => ["long",  ["long", "ptr", "long"]],
    "SQLExecute" => ["long",  ["long"]],
    "SQLFetch" => ["long",  ["long"]],
    "SQLFreeConnect" => ["long",  ["long"]],
    "SQLFreeEnv" => ["long",  ["long"]],
    "SQLFreeStmt" => ["long",  ["long", "long"]],
    "SQLGetCursorName" => ["long",  ["long", "ptr", "long", "ptr"]],
    "SQLNumResultCols" => ["long",  ["long", "ptr"]],
    "SQLPrepare" => ["long",  ["long", "ptr", "long"]],
    "SQLRowCount" => ["long",  ["long", "ptr"]],
    "SQLSetCursorName" => ["long",  ["long", "ptr", "long"]],
    "SQLSetParam" => ["long",  ["long", "long", "long", "long", "long", "long", "ptr", "ptr"]],
    "SQLTransact" => ["long",  ["long", "long", "long"]],
    "SQLAllocHandle" => ["long",  ["long", "long", "ptr"]],
    "SQLBindParam" => ["long",  ["long", "long", "long", "long", "long", "long", "ptr", "ptr"]],
    "SQLCloseCursor" => ["long",  ["long"]],
    "SQLColAttribute" => ["long",  ["long", "long", "long", "ptr", "long", "ptr", "ptr"]],
    "SQLCopyDesc" => ["long",  ["long", "long"]],
    "SQLEndTran" => ["long",  ["long", "long", "long"]],
    "SQLFetchScroll" => ["long",  ["long", "long", "long"]],
    "SQLFreeHandle" => ["long",  ["long", "long"]],
    "SQLGetConnectAttr" => ["long",  ["long", "long", "ptr", "long", "ptr"]],
    "SQLGetDescField" => ["long",  ["long", "long", "long", "ptr", "long", "ptr"]],
    "SQLGetDescRec" => ["long",  ["long", "long", "ptr", "long", "ptr", "ptr", "ptr", "ptr", "ptr", "ptr", "ptr"]],
    "SQLGetDiagField" => ["long",  ["long", "long", "long", "long", "ptr", "long", "ptr"]],
    "SQLGetDiagRec" => ["long",  ["long", "long", "long", "ptr", "ptr", "ptr", "long", "ptr"]],
    "SQLGetEnvAttr" => ["long",  ["long", "long", "ptr", "long", "ptr"]],
    "SQLGetStmtAttr" => ["long",  ["long", "long", "ptr", "long", "ptr"]],
    "SQLSetConnectAttr" => ["long",  ["long", "long", "ptr", "long"]],
    "SQLColumns" => ["long",  ["long", "ptr", "long", "ptr", "long", "ptr", "long", "ptr", "long"]],
    "SQLDriverConnect" => ["long",  ["long", "long", "ptr", "long", "ptr", "long", "ptr", "long"]],
    "SQLGetConnectOption" => ["long",  ["long", "long", "ptr"]],
    "SQLGetData" => ["long",  ["long", "long", "long", "ptr", "long", "ptr"]],
    "SQLGetFunctions" => ["long",  ["long", "long", "ptr"]],
    "SQLGetInfo" => ["long",  ["long", "long", "ptr", "long", "ptr"]],
    "SQLGetStmtOption" => ["long",  ["long", "long", "ptr"]],
    "SQLGetTypeInfo" => ["long",  ["long", "long"]],
    "SQLParamData" => ["long",  ["long", "ptr"]],
    "SQLPutData" => ["long",  ["long", "ptr", "long"]],
    "SQLSetConnectOption" => ["long",  ["long", "long", "long"]],
    "SQLSetStmtOption" => ["long",  ["long", "long", "long"]],
    "SQLSpecialColumns" => ["long",  ["long", "long", "ptr", "long", "ptr", "long", "ptr", "long", "long", "long"]],
    "SQLStatistics" => ["long",  ["long", "ptr", "long", "ptr", "long", "ptr", "long", "long", "long"]],
    "SQLTables" => ["long",  ["long", "ptr", "long", "ptr", "long", "ptr", "long", "ptr", "long"]],
    "SQLBrowseConnect" => ["long",  ["long", "ptr", "long", "ptr", "long", "ptr"]],
    "SQLColumnPrivileges" => ["long",  ["long", "ptr", "long", "ptr", "long", "ptr", "long", "ptr", "long"]],
    "SQLDataSources" => ["long",  ["long", "long", "ptr", "long", "ptr", "ptr", "long", "ptr"]],
    "SQLDescribeParam" => ["long",  ["long", "long", "ptr", "ptr", "ptr", "ptr"]],
    "SQLExtendedFetch" => ["long",  ["long", "long", "long", "ptr", "ptr"]],
    "SQLForeignKeys" => ["long",  ["long", "ptr", "long", "ptr", "long", "ptr", "long", "ptr", "long", "ptr", "long", "ptr", "long"]],
    "SQLMoreResults" => ["long",  ["long"]],
    "SQLNativeSql" => ["long",  ["long", "ptr", "long", "ptr", "long", "ptr"]],
    "SQLNumParams" => ["long",  ["long", "ptr"]],
    "SQLParamOptions" => ["long",  ["long", "long", "ptr"]],
    "SQLPrimaryKeys" => ["long",  ["long", "ptr", "long", "ptr", "long", "ptr", "long"]],
    "SQLProcedureColumns" => ["long",  ["long", "ptr", "long", "ptr", "long", "ptr", "long", "ptr", "long"]],
    "SQLProcedures" => ["long",  ["long", "ptr", "long", "ptr", "long", "ptr", "long"]],
    "SQLSetPos" => ["long",  ["long", "long", "long", "long"]],
    "SQLSetScrollOptions" => ["long",  ["long", "long", "long", "long"]],
    "SQLTablePrivileges" => ["long",  ["long", "ptr", "long", "ptr", "long", "ptr", "long"]],
    "SQLDrivers" => ["long",  ["long", "long", "ptr", "long", "ptr", "ptr", "long", "ptr"]],
    "SQLBindParameter" => ["long",  ["long", "long", "long", "long", "long", "long", "long", "ptr", "long", "ptr"]],
    "SQLSetDescField" => ["long",  ["long", "long", "long", "ptr", "long"]],
    "SQLSetDescRec" => ["long",  ["long", "long", "long", "long", "long", "long", "long", "ptr", "ptr", "ptr"]],
    "SQLSetEnvAttr" => ["long",  ["long", "long", "ptr", "long"]],
    "SQLSetStmtAttr" => ["long",  ["long", "long", "ptr", "long"]],
    "SQLAllocHandleStd" => ["long",  ["long", "long", "ptr"]],
    "SQLBulkOperations" => ["long",  ["long", "long"]]
};

&wine::declare("odbc32",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
