name	ddeml
id	25

2 stub DdeInitialize #(ptr segptr long long) DdeInitialize
3 stub DdeUnInitialize #(long) DdeUnInitialize
4 stub DdeConnectList #(long word word word ptr) DdeConnectList
5 stub DdeQueryNextServer #(word word) DdeQueryNextServer
6 stub DdeDisconnectList #(word) DdeDisconnectList
7 stub DdeConnect #(long word word ptr) DdeConnect
8 stub DdeDisconnect #(word) DdeDisconnect
9 stub DdeQueryConvInfo #(word long ptr) DdeQueryConvInfo
10 stub DdeSetUserHandle #(word long long) DdeSetUserHandle
11 stub DdeClientTransaction #(ptr long word word word word long ptr) DdeClientTransaction
12 stub DdeAbandonTransaction #(long word long) DdeAbandonTransaction
13 stub DdePostAdvise #(long word word) DdePostAdvise
14 stub DdeCreateDataHandle #(long ptr long long word word word) DdeCreateDataHandle
15 stub DdeAddData #(word ptr long long) DdeAddData
16 stub DdeGetData #(word ptr long long) DdeGetData
17 stub DdeAccessData #(word ptr) DdeAccessData
18 stub DdeUnaccessData #(word) DdeUnaccessData
19 stub DdeFreeDataHandle #(word) DdeFreeDataHandle
20 stub DdeGetLastError #(long) DdeGetLastError
21 stub DdeCreateStringHandle #(long ptr word) DdeCreateStringHandle
22 stub DdeFreeStringHandle #(long word) DdeFreeStringHandle
23 stub   DdeQueryString #(long word ptr long word) DdeQueryString
24 stub DdeKeepStringHandle #(long word) DdeKeepStringHandle

26 stub DdeEnableCallback #(long word word) DdeEnableCallback
27 stub DdeNameService #(long word word word)

36 stub DdeCmpStringHandles #(word word) DdeCmpStringHandles
37 stub DdeReconnect #(word) DdeReconnect
