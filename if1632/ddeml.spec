name	ddeml
type	win16

2 pascal16 DdeInitialize(ptr segptr long long) DdeInitialize16
3 pascal16 DdeUninitialize(long) DdeUninitialize16
4 pascal DdeConnectList(long word word word ptr) DdeConnectList16
5 pascal DdeQueryNextServer(word word) DdeQueryNextServer16
6 pascal DdeDisconnectList(word) DdeDisconnectList16
7 pascal   DdeConnect(long long long ptr) DdeConnect16
8 pascal16 DdeDisconnect(long) DdeDisconnect16
9 pascal16  DdeQueryConvInfo (word long ptr) DdeQueryConvInfo16
10 pascal DdeSetUserHandle(word long long) DdeSetUserHandle16
11 pascal   DdeClientTransaction(ptr long long long s_word s_word long ptr)
            DdeClientTransaction16
12 pascal DdeAbandonTransaction(long word long) DdeAbandonTransaction16
13 pascal DdePostAdvise(long word word) DdePostAdvise16
14 pascal DdeCreateDataHandle(long ptr long long word word word) DdeCreateDataHandle16
15 pascal DdeAddData(word ptr long long) DdeAddData16
16 pascal DdeGetData(word ptr long long) DdeGetData16
17 pascal DdeAccessData(word ptr) DdeAccessData16
18 pascal DdeUnaccessData(word) DdeUnaccessData16
19 pascal16 DdeFreeDataHandle(long) DdeFreeDataHandle16
20 pascal16 DdeGetLastError(long) DdeGetLastError16
21 pascal   DdeCreateStringHandle(long str s_word) DdeCreateStringHandle16
22 pascal16 DdeFreeStringHandle(long long) DdeFreeStringHandle16
23 pascal  DdeQueryString (long word ptr long word) DdeQueryString16
24 pascal16 DdeKeepStringHandle(long long) DdeKeepStringHandle16

26 pascal DdeEnableCallback(long word word) DdeEnableCallback16
27 pascal   DdeNameService(long long long s_word) DdeNameService16

36 pascal DdeCmpStringHandles(word word) DdeCmpStringHandles16
37 pascal   DdeReconnect(long) DdeReconnect
