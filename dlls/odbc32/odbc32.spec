  1 stdcall SQLAllocConnect(long ptr) SQLAllocConnect
  2 stdcall SQLAllocEnv(ptr)  SQLAllocEnv
  3 stdcall SQLAllocStmt(long ptr) SQLAllocStmt
  4 stdcall SQLBindCol(long long long ptr long ptr) SQLBindCol
  5 stdcall SQLCancel(long) SQLCancel
  6 stdcall SQLColAttributes(long long long ptr long ptr ptr) SQLColAttributes
  7 stdcall SQLConnect(long str long str long str long) SQLConnect
  8 stdcall SQLDescribeCol(long long str long ptr ptr ptr ptr ptr) SQLDescribeCol
  9 stdcall SQLDisconnect(long) SQLDisconnect
 10 stdcall SQLError(long long long str ptr str long ptr) SQLError
 11 stdcall SQLExecDirect(long str long) SQLExecDirect
 12 stdcall SQLExecute(long) SQLExecute
 13 stdcall SQLFetch(long) SQLFetch
 14 stdcall SQLFreeConnect(long) SQLFreeConnect
 15 stdcall SQLFreeEnv(long) SQLFreeEnv
 16 stdcall SQLFreeStmt(long long ) SQLFreeStmt
 17 stdcall SQLGetCursorName(long str long ptr) SQLGetCursorName
 18 stdcall SQLNumResultCols(long ptr) SQLNumResultCols
 19 stdcall SQLPrepare(long str long) SQLPrepare
 20 stdcall SQLRowCount(long ptr) SQLRowCount
 21 stdcall SQLSetCursorName(long str long) SQLSetCursorName
 22 stdcall SQLSetParam(long long long long long long ptr ptr) SQLSetParam
 23 stdcall SQLTransact(long long long) SQLTransact
 24 stdcall SQLAllocHandle(long long ptr) SQLAllocHandle
 25 stdcall SQLBindParam(long long long long long long ptr ptr) SQLBindParam
 26 stdcall SQLCloseCursor(long) SQLCloseCursor
 27 stdcall SQLColAttribute(long long long ptr long ptr ptr) SQLColAttribute
 28 stdcall SQLCopyDesc(long long) SQLCopyDesc
 29 stdcall SQLEndTran(long long long) SQLEndTran
 30 stdcall SQLFetchScroll(long long long) SQLFetchScroll
 31 stdcall SQLFreeHandle(long long) SQLFreeHandle
 32 stdcall SQLGetConnectAttr(long long ptr long ptr) SQLGetConnectAttr
 33 stdcall SQLGetDescField(long long long ptr long ptr) SQLGetDescField
 34 stdcall SQLGetDescRec(long long str long ptr ptr ptr ptr ptr ptr ptr) SQLGetDescRec
 35 stdcall SQLGetDiagField(long long long long ptr long ptr) SQLGetDiagField
 36 stdcall SQLGetDiagRec(long long long str ptr str long ptr) SQLGetDiagRec
 37 stdcall SQLGetEnvAttr(long long ptr long ptr) SQLGetEnvAttr
 38 stdcall SQLGetStmtAttr(long long ptr long ptr) SQLGetStmtAttr
 39 stdcall SQLSetConnectAttr(long long ptr long) SQLSetConnectAttr
 40 stdcall SQLColumns(long str long str long str long str long) SQLColumns
 41 stdcall SQLDriverConnect(long long str long str long str long) SQLDriverConnect
 42 stdcall SQLGetConnectOption(long long ptr) SQLGetConnectOption
 43 stdcall SQLGetData(long long long ptr long ptr) SQLGetData
 44 stdcall SQLGetFunctions(long long ptr) SQLGetFunctions
 45 stdcall SQLGetInfo(long long ptr long ptr) SQLGetInfo
 46 stdcall SQLGetStmtOption(long long ptr) SQLGetStmtOption
 47 stdcall SQLGetTypeInfo(long long) SQLGetTypeInfo
 48 stdcall SQLParamData(long ptr) SQLParamData
 49 stdcall SQLPutData(long ptr long) SQLPutData
 50 stdcall SQLSetConnectOption(long long long) SQLSetConnectOption
 51 stdcall SQLSetStmtOption(long long long) SQLSetStmtOption
 52 stdcall SQLSpecialColumns(long long str long str long str long long long) SQLSpecialColumns
 53 stdcall SQLStatistics(long str long str long str long long long) SQLStatistics
 54 stdcall SQLTables(long str long str long str long str long) SQLTables
 55 stdcall SQLBrowseConnect(long str long str long ptr) SQLBrowseConnect
 56 stdcall SQLColumnPrivileges(long str long str long str long str long) SQLColumnPrivileges
 57 stdcall SQLDataSources(long long str long ptr str long ptr) SQLDataSources
 58 stdcall SQLDescribeParam(long long ptr ptr ptr ptr) SQLDescribeParam
 59 stdcall SQLExtendedFetch(long long long ptr ptr) SQLExtendedFetch
 60 stdcall SQLForeignKeys(long str long str long str long str long str long str long) SQLForeignKeys
 61 stdcall SQLMoreResults(long) SQLMoreResults
 62 stdcall SQLNativeSql(long str long str long ptr) SQLNativeSql
 63 stdcall SQLNumParams(long ptr) SQLNumParams
 64 stdcall SQLParamOptions(long long ptr) SQLParamOptions
 65 stdcall SQLPrimaryKeys(long str long str long str long) SQLPrimaryKeys
 66 stdcall SQLProcedureColumns(long str long str long str long str long) SQLProcedureColumns
 67 stdcall SQLProcedures(long str long str long str long) SQLProcedures
 68 stdcall SQLSetPos(long long long long) SQLSetPos
 69 stdcall SQLSetScrollOptions(long long long long) SQLSetScrollOptions
 70 stdcall SQLTablePrivileges(long str long str long str long) SQLTablePrivileges
 71 stdcall SQLDrivers(long long str long ptr str long ptr) SQLDrivers
 72 stdcall SQLBindParameter(long long long long long long long ptr long ptr) SQLBindParameter
 73 stdcall SQLSetDescField(long long long ptr long) SQLSetDescField
 74 stdcall SQLSetDescRec(long long long long long long long ptr ptr ptr) SQLSetDescRec
 75 stdcall SQLSetEnvAttr(long long ptr long) SQLSetEnvAttr
 76 stdcall SQLSetStmtAttr(long long ptr long) SQLSetStmtAttr
 77 stdcall SQLAllocHandleStd(long long ptr) SQLAllocHandleStd
 78 stdcall SQLBulkOperations(long long) SQLBulkOperations
 79 stub    CloseODBCPerfData
 80 stub    CollectODBCPerfData
 81 stub    CursorLibLockDbc
 82 stub    CursorLibLockDesc
 83 stub    CursorLibLockStmt
 84 stub    ODBCGetTryWaitValue
 85 stub    CursorLibTransact
 86 stub    ODBSetTryWaitValue
 87 stub    LockHandle
 88 stub    ODBCInternalConnectW
 89 stub    ODBCSharedPerfMon
 90 stub    ODBCSharedVSFlag
 91 stub    OpenODBCPerfData
 92 stub    PostComponentError
 93 stub    PostODBCComponentError
 94 stub    PostODBCError
 95 stub    SearchStatusCode
 96 stub    VFreeErrors
 97 stub    VRetrieveDriverErrorsRowCol
 98 stub    ValidateErrorQueue
 99 stub    SQLColAttributesW
100 stub    SQLConnectW
101 stub    SQLDescribeColW
102 stub    SQLErrorW
103 stub    SQLExecDirectW
104 stub    SQLGetCursorNameW
105 stub    SQLPrepareW
106 stub    SQLSetCursorNameW
107 stub    SQLColAttributeW
108 stub    SQLGetConnectAttrW
109 stub    SQLGetDescFieldW
110 stub    SQLGetDescRecW
111 stub    SQLGetDiagFieldW
112 stub    SQLGetDiagRecW
113 stub    SQLGetStmtAttrW
114 stub    SQLSetConnectAttrW
115 stub    SQLColumnsW
116 stub    SQLDriverConnectW
117 stub    SQLGetConnectOptionW
118 stub    SQLGetInfoW
119 stub    SQLGetTypeInfoW
120 stub    SQLSetConnectOptionW
121 stub    SQLSpecialColumnsW
122 stub    SQLStatisticsW
123 stub    SQLTablesW
124 stub    SQLBrowseConnectW
125 stub    SQLDataSourcesW
126 stub    SQLColumnPrivilegesW
127 stub    SQLForeignKeysW
128 stub    SQLNativeSqlW
129 stub    SQLPrimaryKeysW
130 stub    SQLProcedureColumnsW
131 stub    SQLProceduresW
132 stub    SQLTablePrivilegesW
133 stub    SQLDriversW
134 stub    SQLSetDescFieldW
135 stub    SQLSetStmtAttrW
136 stub    SQLColAttributesA
137 stub    SQLConnectA
138 stub    SQLDescribeColA
139 stub    SQLErrorA
140 stub    SQLExecDirectA
141 stub    SQLGetCursorNameA
142 stub    SQLPrepareA
143 stub    SQLSetCursorNameA
144 stub    SQLColAttributeA
145 stub    SQLGetConnectAttrA
146 stub    SQLGetDescFieldA
147 stub    SQLGetDescRecA
148 stub    SQLGetDiagFieldA
149 stub    SQLGetDiagRecA
150 stub    SQLGetStmtAttrA
151 stub    SQLSetConnectAttrA
152 stub    SQLColumnsA
153 stub    SQLDriverConnectA
154 stub    SQLGetConnectOptionA
155 stub    SQLGetInfoA
156 stub    SQLGetTypeInfoA
157 stub    SQLSetConnectOptionA
158 stub    SQLSpecialColumnsA
159 stub    SQLStatisticsA
160 stub    SQLTablesA
161 stub    SQLBrowseConnectA
162 stub    SQLColumnPrivilegesA
163 stub    SQLDataSourcesA
164 stub    SQLForeignKeysA
165 stub    SQLNativeSqlA
166 stub    SQLPrimaryKeysA
167 stub    SQLProcedureColumnsA
168 stub    SQLProceduresA
169 stub    SQLTablePrivilegesA
170 stub    SQLDriversA
171 stub    SQLSetDescFieldA
172 stub    SQLSetStmtAttrA
173 stub    ODBCSharedTraceFlag
174 stub    ODBCQualifyFileDSNW
