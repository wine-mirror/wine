name	olesvr32
type	win32

 1 stub WEP
 2 stdcall OleRegisterServer(str ptr ptr long long) OleRegisterServer
 3 stub OleRevokeServer
 4 stdcall OleBlockServer(long) OleBlockServer
 5 stdcall OleUnblockServer(long ptr) OleUnblockServer
 6 stdcall OleRegisterServerDoc(ptr str ptr ptr) OleRegisterServerDoc
 7 stdcall OleRevokeServerDoc(long) OleRevokeServerDoc
 8 stdcall OleRenameServerDoc(long str) OleRenameServerDoc
 9 stub OleRevertServerDoc
10 stub OleSavedServerDoc
11 stub OleRevokeObject
12 stub OleQueryServerVersion
