2 pascal -ret16 DdeInitialize(ptr segptr long long) DdeInitialize16
3 pascal -ret16 DdeUninitialize(long) DdeUninitialize16
4 pascal DdeConnectList(long long long long ptr) DdeConnectList16
5 pascal DdeQueryNextServer(long long) DdeQueryNextServer16
6 pascal -ret16 DdeDisconnectList(long) DdeDisconnectList16
7 pascal DdeConnect(long long long ptr) DdeConnect16
8 pascal -ret16 DdeDisconnect(long) DdeDisconnect16
9 pascal -ret16 DdeQueryConvInfo(long long ptr) DdeQueryConvInfo16
10 pascal -ret16 DdeSetUserHandle(long long long) DdeSetUserHandle16
11 pascal DdeClientTransaction(long long long long word word long ptr) DdeClientTransaction16
12 pascal -ret16 DdeAbandonTransaction(long long long) DdeAbandonTransaction16
13 pascal -ret16 DdePostAdvise(long long long) DdePostAdvise16
14 pascal DdeCreateDataHandle(long ptr long long long word word) DdeCreateDataHandle16
15 pascal DdeAddData(long ptr long long) DdeAddData16
16 pascal DdeGetData(long ptr long long) DdeGetData16
17 pascal DdeAccessData(long ptr) DdeAccessData16
18 pascal -ret16 DdeUnaccessData(long) DdeUnaccessData16
19 pascal -ret16 DdeFreeDataHandle(long) DdeFreeDataHandle16
20 pascal -ret16 DdeGetLastError(long) DdeGetLastError16
21 pascal DdeCreateStringHandle(long str s_word) DdeCreateStringHandle16
22 pascal -ret16 DdeFreeStringHandle(long long) DdeFreeStringHandle16
23 pascal DdeQueryString(long long ptr long s_word) DdeQueryString16
24 pascal -ret16 DdeKeepStringHandle(long long) DdeKeepStringHandle16

26 pascal -ret16 DdeEnableCallback(long long word) DdeEnableCallback16
27 pascal DdeNameService(long long long word) DdeNameService16

36 pascal -ret16 DdeCmpStringHandles(long long) DdeCmpStringHandles16
37 pascal DdeReconnect(long) DdeReconnect
