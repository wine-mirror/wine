name	odbc32
type	win32
init    MAIN_OdbcInit

001 stdcall SQLAllocConnect(long ptr) SQLAllocConnect
002 stdcall SQLAllocEnv(ptr)  SQLAllocEnv
003 stdcall SQLAllocStmt(long ptr) SQLAllocStmt
004 stdcall SQLBindCol(long long long ptr long ptr) SQLBindCol
005 stdcall SQLCancel(long) SQLCancel
006 stdcall SQLColAttributes(long long long ptr long ptr ptr) SQLColAttributes
007 stdcall SQLConnect(long str long str long str long) SQLConnect
008 stdcall SQLDescribeCol(long long str long ptr ptr ptr ptr ptr) SQLDescribeCol
009 stdcall SQLDisconnect(long) SQLDisconnect
010 stdcall SQLError(long long long str ptr str long ptr) SQLError
011 stdcall SQLExecDirect(long str long) SQLExecDirect
012 stdcall SQLExecute(long) SQLExecute
013 stdcall SQLFetch(long) SQLFetch
014 stdcall SQLFreeConnect(long) SQLFreeConnect
015 stdcall SQLFreeEnv(long) SQLFreeEnv
016 stdcall SQLFreeStmt(long long ) SQLFreeStmt
017 stdcall SQLGetCursorName(long str long ptr) SQLGetCursorName
018 stdcall SQLNumResultCols(long ptr) SQLNumResultCols
019 stdcall SQLPrepare(long str long) SQLPrepare
020 stdcall SQLRowCount(long ptr) SQLRowCount
021 stdcall SQLSetCursorName(long str long) SQLSetCursorName
022 stdcall SQLSetParam(long long long long long long ptr ptr) SQLSetParam
023 stdcall SQLTransact(long long long) SQLTransact
024 stdcall SQLAllocHandle(long long ptr) SQLAllocHandle
025 stdcall SQLBindParam(long long long long long long ptr ptr) SQLBindParam
026 stdcall SQLCloseCursor(long) SQLCloseCursor
027 stdcall SQLColAttribute(long long long ptr long ptr ptr) SQLColAttribute
028 stdcall SQLCopyDesc(long long) SQLCopyDesc
029 stdcall SQLEndTran(long long long) SQLEndTran
030 stdcall SQLFetchScroll(long long long) SQLFetchScroll
031 stdcall SQLFreeHandle(long long) SQLFreeHandle
032 stdcall SQLGetConnectAttr(long long ptr long ptr) SQLGetConnectAttr
033 stdcall SQLGetDescField(long long long ptr long ptr) SQLGetDescField
034 stdcall SQLGetDescRec(long long str long ptr ptr ptr ptr ptr ptr ptr) SQLGetDescRec
035 stdcall SQLGetDiagField(long long long long ptr long ptr) SQLGetDiagField
036 stdcall SQLGetDiagRec(long long long str ptr str long ptr) SQLGetDiagRec
037 stdcall SQLGetEnvAttr(long long ptr long ptr) SQLGetEnvAttr
038 stdcall SQLGetStmtAttr(long long ptr long ptr) SQLGetStmtAttr
039 stdcall SQLSetConnectAttr(long long ptr long) SQLSetConnectAttr
040 stdcall SQLColumns(long str long str long str long str long) SQLColumns
041 stdcall SQLDriverConnect(long long str long str long str long) SQLDriverConnect
042 stdcall SQLGetConnectOption(long long ptr) SQLGetConnectOption
043 stdcall SQLGetData(long long long ptr long ptr) SQLGetData
044 stdcall SQLGetFunctions(long long ptr) SQLGetFunctions
045 stdcall SQLGetInfo(long long ptr long ptr) SQLGetInfo
046 stdcall SQLGetStmtOption(long long ptr) SQLGetStmtOption
047 stdcall SQLGetTypeInfo(long long) SQLGetTypeInfo
048 stdcall SQLParamData(long ptr) SQLParamData
049 stdcall SQLPutData(long ptr long) SQLPutData
050 stdcall SQLSetConnectOption(long long long) SQLSetConnectOption
051 stdcall SQLSetStmtOption(long long long) SQLSetStmtOption
052 stdcall SQLSpecialColumns(long long str long str long str long long long) SQLSpecialColumns
053 stdcall SQLStatistics(long str long str long str long long long) SQLStatistics
054 stdcall SQLTables(long str long str long str long str long) SQLTables
055 stdcall SQLBrowseConnect(long str long str long ptr) SQLBrowseConnect
056 stdcall SQLColumnPrivileges(long str long str long str long str long) SQLColumnPrivileges
057 stdcall SQLDataSources(long long str long ptr str long ptr) SQLDataSources
058 stdcall SQLDescribeParam(long long ptr ptr ptr ptr) SQLDescribeParam
059 stdcall SQLExtendedFetch(long long long ptr ptr) SQLExtendedFetch
060 stdcall SQLForeignKeys(long str long str long str long str long str long str long) SQLForeignKeys
061 stdcall SQLMoreResults(long) SQLMoreResults
062 stdcall SQLNativeSql(long str long str long ptr) SQLNativeSql
063 stdcall SQLNumParams(long ptr) SQLNumParams
064 stdcall SQLParamOptions(long long ptr) SQLParamOptions
065 stdcall SQLPrimaryKeys(long str long str long str long) SQLPrimaryKeys
066 stdcall SQLProcedureColumns(long str long str long str long str long) SQLProcedureColumns
067 stdcall SQLProcedures(long str long str long str long) SQLProcedures
068 stdcall SQLSetPos(long long long long) SQLSetPos
069 stdcall SQLSetScrollOptions(long long long long) SQLSetScrollOptions
070 stdcall SQLTablePrivileges(long str long str long str long) SQLTablePrivileges
071 stdcall SQLDrivers(long long str long ptr str long ptr) SQLDrivers
072 stdcall SQLBindParameter(long long long long long long long ptr long ptr) SQLBindParameter
073 stdcall SQLSetDescField(long long long ptr long) SQLSetDescField
074 stdcall SQLSetDescRec(long long long long long long long ptr ptr ptr) SQLSetDescRec
075 stdcall SQLSetEnvAttr(long long ptr long) SQLSetEnvAttr
076 stdcall SQLSetStmtAttr(long long ptr long) SQLSetStmtAttr
077 stdcall SQLAllocHandleStd(long long ptr) SQLAllocHandleStd
078 stdcall SQLBulkOperations(long long) SQLBulkOperations
079 stub    CloseODBCPerfData
080 stub    CollectODBCPerfData
081 stub    CursorLibLockDbc
082 stub    CursorLibLockDesc
083 stub    CursorLibLockStmt
084 stub    ODBCGetTryWaitValue
085 stub    CursorLibTransact
086 stub    ODBSetTryWaitValue
087 stub    LockHandle
088 stub    ODBCInternalConnectW
089 stub    ODBCSharedPerfMon
090 stub    ODBCSharedVSFlag
091 stub    OpenODBCPerfData
092 stub    PostComponentError
093 stub    PostODBCComponentError
094 stub    PostODBCError
095 stub    SearchStatusCode
096 stub    VFreeErrors
097 stub    VRetrieveDriverErrorsRowCol
098 stub    ValidateErrorQueue
099 stub    SQLColAttributesW
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
  
