name	olesvr
type	win16

#1 WEP
2  pascal OleRegisterServer(str ptr ptr word word) OleRegisterServer
3  pascal OleRevokeServer(long) OleRevokeServer
4  pascal OleBlockServer(long) OleBlockServer
5  pascal OleUnblockServer(long ptr) OleUnblockServer
6  pascal OleRegisterServerDoc(long str ptr ptr) OleRegisterServerDoc
7  pascal OleRevokeServerDoc(long) OleRevokeServerDoc
8 stub OLERENAMESERVERDOC
9 stub OLEREVERTSERVERDOC
10 stub OLESAVEDSERVERDOC
11 stub OLEREVOKEOBJECT
12 stub OLEQUERYSERVERVERSION
21 stub SRVRWNDPROC
22 stub DOCWNDPROC
23 stub ITEMWNDPROC
24 stub SENDDATAMSG
25 stub FINDITEMWND
26 stub ITEMCALLBACK
27 stub TERMINATECLIENTS
28 stub TERMINATEDOCCLIENTS
29 stub DELETECLIENTINFO
30 stub SENDRENAMEMSG
31 stub ENUMFORTERMINATE
