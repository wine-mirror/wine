name	olesvr32
type	win32

 1 stub WEP
 2 stdcall OleRegisterServer(str ptr ptr long long) OleRegisterServer32
 3 stub OleRevokeServer
 4 stdcall OleBlockServer(long) OleBlockServer32
 5 stdcall OleUnblockServer(long ptr) OleUnblockServer32
 6 stdcall OleRegisterServerDoc(ptr str ptr ptr) OleRegisterServerDoc32
 7 stdcall OleRevokeServerDoc(long) OleRevokeServerDoc32
 8 stdcall OleRenameServerDoc(long str) OleRenameServerDoc32
 9 stub OleRevertServerDoc
10 stub OleSavedServerDoc
11 stub OleRevokeObject
12 stub OleQueryServerVersion
