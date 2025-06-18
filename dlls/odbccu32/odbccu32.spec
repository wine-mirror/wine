  4 stdcall SQLBindCol(long long long ptr long ptr) odbc32.SQLBindCol
  5 stdcall SQLCancel(long) odbc32.SQLCancel
  6 stub ReleaseCLStmtResources
 11 stdcall SQLExecDirect(long str long) odbc32.SQLExecDirect
 12 stdcall SQLExecute(long) odbc32.SQLExecute
 13 stdcall SQLFetch(long) odbc32.SQLFetch
 16 stdcall SQLFreeStmt(long long ) odbc32.SQLFreeStmt
 19 stdcall SQLPrepare(long str long) odbc32.SQLPrepare
 20 stdcall SQLRowCount(long ptr) odbc32.SQLRowCount
 23 stdcall SQLTransact(long long long) odbc32.SQLTransact
 26 stdcall SQLCloseCursor(long) odbc32.SQLCloseCursor
 29 stdcall SQLEndTran(long long long) odbc32.SQLEndTran
 30 stdcall SQLFetchScroll(long long long) odbc32.SQLFetchScroll
 31 stdcall SQLFreeHandle(long long) odbc32.SQLFreeHandle
 33 stdcall SQLGetDescField(long long long ptr long ptr) odbc32.SQLGetDescField
 34 stdcall SQLGetDescRec(long long str long ptr ptr ptr ptr ptr ptr ptr) odbc32.SQLGetDescRec
 38 stdcall SQLGetStmtAttr(long long ptr long ptr) odbc32.SQLGetStmtAttr
 39 stdcall SQLSetConnectAttr(long long ptr long) odbc32.SQLSetConnectAttr
 43 stdcall SQLGetData(long long long ptr long ptr) odbc32.SQLGetData
 45 stdcall SQLGetInfo(long long ptr long ptr) odbc32.SQLGetInfo
 46 stdcall SQLGetStmtOption(long long ptr) odbc32.SQLGetStmtOption
 48 stdcall SQLParamData(long ptr) odbc32.SQLParamData
 49 stdcall SQLPutData(long ptr long) odbc32.SQLPutData
 50 stdcall SQLSetConnectOption(long long long) odbc32.SQLSetConnectOption
 51 stdcall SQLSetStmtOption(long long long) odbc32.SQLSetStmtOption
 59 stdcall SQLExtendedFetch(long long long ptr ptr) odbc32.SQLExtendedFetch
 61 stdcall SQLMoreResults(long) odbc32.SQLMoreResults
 62 stdcall SQLNativeSql(long str long str long ptr) odbc32.SQLNativeSql
 63 stdcall SQLNumParams(long ptr) odbc32.SQLNumParams
 64 stdcall SQLParamOptions(long long ptr) odbc32.SQLParamOptions
 68 stdcall SQLSetPos(long long long long) odbc32.SQLSetPos
 69 stdcall SQLSetScrollOptions(long long long long) odbc32.SQLSetScrollOptions
 72 stdcall SQLBindParameter(long long long long long long long ptr long ptr) odbc32.SQLBindParameter
 73 stdcall SQLSetDescField(long long long ptr long) odbc32.SQLSetDescField
 74 stdcall SQLSetDescRec(long long long long long long long ptr ptr ptr) odbc32.SQLSetDescRec
 76 stdcall SQLSetStmtAttr(long long ptr long) odbc32.SQLSetStmtAttr
 78 stdcall SQLBulkOperations(long long) odbc32.SQLBulkOperations
