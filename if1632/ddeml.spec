name	ddeml
type	win16

2 pascal16 DdeInitialize(ptr segptr long long) DdeInitialize16
3 pascal16 DdeUninitialize(long) DdeUninitialize16
4 pascal DdeConnectList(long word word word ptr) DdeConnectList16
5 pascal DdeQueryNextServer(word word) DdeQueryNextServer16
6 pascal DdeDisconnectList(word) DdeDisconnectList16
7 pascal   DdeConnect(long long long ptr) DdeConnect16
8 pascal16 DdeDisconnect(long) DdeDisconnect16
9 stub DdeQueryConvInfo #(word long ptr) DdeQueryConvInfo
10 pascal DdeSetUserHandle(word long long) DdeSetUserHandle
11 pascal   DdeClientTransaction(ptr long long long s_word s_word long ptr)
            DdeClientTransaction16
12 pascal DdeAbandonTransaction(long word long) DdeAbandonTransaction
13 pascal DdePostAdvise(long word word) DdePostAdvise16
14 pascal DdeCreateDataHandle(long ptr long long word word word) DdeCreateDataHandle
15 pascal DdeAddData(word ptr long long) DdeAddData
16 pascal DdeGetData(word ptr long long) DdeGetData16
17 pascal DdeAccessData(word ptr) DdeAccessData
18 pascal DdeUnaccessData(word) DdeUnaccessData
19 pascal16 DdeFreeDataHandle(long) DdeFreeDataHandle16
20 pascal16 DdeGetLastError(long) DdeGetLastError16
21 pascal   DdeCreateStringHandle(long str s_word) DdeCreateStringHandle16
22 pascal16 DdeFreeStringHandle(long long) DdeFreeStringHandle16
23 stub DdeQueryString #(long word ptr long word) DdeQueryString
24 pascal16 DdeKeepStringHandle(long long) DdeKeepStringHandle16

26 pascal DdeEnableCallback(long word word) DdeEnableCallback
27 pascal   DdeNameService(long long long s_word) DdeNameService16

36 pascal DdeCmpStringHandles(word word) DdeCmpStringHandles
37 pascal   DdeReconnect(long) DdeReconnect
