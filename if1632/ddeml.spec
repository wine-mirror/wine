name	ddeml
type	win16

2 pascal16 DdeInitialize(ptr segptr long long) DdeInitialize16
3 pascal16 DdeUninitialize(long) DdeUninitialize16
4 stub DdeConnectList #(long word word word ptr) DdeConnectList
5 stub DdeQueryNextServer #(word word) DdeQueryNextServer
6 stub DdeDisconnectList #(word) DdeDisconnectList
7 pascal   DdeConnect(long long long ptr) DdeConnect16
8 pascal16 DdeDisconnect(long) DdeDisconnect16
9 stub DdeQueryConvInfo #(word long ptr) DdeQueryConvInfo
10 stub DdeSetUserHandle #(word long long) DdeSetUserHandle
11 pascal   DdeClientTransaction(ptr long long long s_word s_word long ptr)
            DdeClientTransaction16
12 stub DdeAbandonTransaction #(long word long) DdeAbandonTransaction
13 stub DdePostAdvise #(long word word) DdePostAdvise
14 stub DdeCreateDataHandle #(long ptr long long word word word) DdeCreateDataHandle
15 stub DdeAddData #(word ptr long long) DdeAddData
16 stub DdeGetData #(word ptr long long) DdeGetData
17 stub DdeAccessData #(word ptr) DdeAccessData
18 stub DdeUnaccessData #(word) DdeUnaccessData
19 pascal16 DdeFreeDataHandle(long) DdeFreeDataHandle16
20 pascal16 DdeGetLastError(long) DdeGetLastError16
21 pascal   DdeCreateStringHandle(long str s_word) DdeCreateStringHandle16
22 pascal16 DdeFreeStringHandle(long long) DdeFreeStringHandle16
23 stub DdeQueryString #(long word ptr long word) DdeQueryString
24 pascal16 DdeKeepStringHandle(long long) DdeKeepStringHandle16

26 stub DdeEnableCallback #(long word word) DdeEnableCallback
27 pascal   DdeNameService(long long long s_word) DdeNameService16

36 stub DdeCmpStringHandles #(word word) DdeCmpStringHandles
37 pascal   DdeReconnect(long) DdeReconnect
