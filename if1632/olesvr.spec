name	olesvr
id	22
length	31
#1 WEP
2  pascal OleRegisterServer(ptr ptr ptr word word) OleRegisterServer(1 2 3 4 5)
3  pascal OleRevokeServer(long) OleRevokeServer(1)
4  pascal OleBlockServer(long) OleBlockServer(1)
5  pascal OleUnblockServer(long ptr) OleUnblockServer(1 2)
6  pascal OleRegisterServerDoc(long ptr ptr ptr) OleRegisterServerDoc(1 2 3 4)
7  pascal OleRevokeServerDoc(long) OleRevokeServerDoc(1)
#8 OLERENAMESERVERDOC
#9 OLEREVERTSERVERDOC
#10 OLESAVEDSERVERDOC
#11 OLEREVOKEOBJECT
#12 OLEQUERYSERVERVERSION
#21 SRVRWNDPROC
#22 DOCWNDPROC
#23 ITEMWNDPROC
#24 SENDDATAMSG
#25 FINDITEMWND
#26 ITEMCALLBACK
#27 TERMINATECLIENTS
#28 TERMINATEDOCCLIENTS
#29 DELETECLIENTINFO
#30 SENDRENAMEMSG
#31 ENUMFORTERMINATE
