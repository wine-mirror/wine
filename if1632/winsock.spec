#  
#         File: winsock.def 
#       System: MS-Windows 3.x 
#      Summary: Module definition file for Windows Sockets DLL.  
#  
name	winsock
type	win16

1   pascal16 accept(word ptr ptr) WINSOCK_accept
2   pascal16 bind(word ptr word) WINSOCK_bind
3   pascal16 closesocket(word) WINSOCK_closesocket
4   pascal16 connect(word ptr word) WINSOCK_connect
5   pascal16 getpeername(word ptr ptr) WINSOCK_getpeername
6   pascal16 getsockname(word ptr ptr) WINSOCK_getsockname
7   pascal16 getsockopt(word word word ptr ptr) WINSOCK_getsockopt
8   pascal   htonl(long) WINSOCK_htonl
9   pascal16 htons(word) WINSOCK_htons
10  pascal   inet_addr(ptr) WINSOCK_inet_addr
11  pascal   inet_ntoa(long) WINSOCK_inet_ntoa
12  pascal16 ioctlsocket(word long ptr) WINSOCK_ioctlsocket
13  pascal16 listen(word word) WINSOCK_listen
14  pascal   ntohl(long) WINSOCK_ntohl
15  pascal16 ntohs(word) WINSOCK_ntohs
16  pascal16 recv(word ptr word word) WINSOCK_recv
17  pascal16 recvfrom(word ptr word word ptr ptr) WINSOCK_recvfrom
18  pascal16 select(word ptr ptr ptr ptr) WINSOCK_select
19  pascal16 send(word ptr word word) WINSOCK_send
20  pascal16 sendto(word ptr word word ptr word) WINSOCK_sendto
21  pascal16 setsockopt(word word word ptr word) WINSOCK_setsockopt
22  pascal16 shutdown(word word) WINSOCK_shutdown
23  pascal16 socket(word word word) WINSOCK_socket
51  pascal   gethostbyaddr(ptr word word) WINSOCK_gethostbyaddr
52  pascal   gethostbyname(ptr) WINSOCK_gethostbyname
53  pascal   getprotobyname(ptr) WINSOCK_getprotobyname
54  pascal   getprotobynumber(word) WINSOCK_getprotobynumber
55  pascal   getservbyname(ptr ptr) WINSOCK_getservbyname
56  pascal   getservbyport(word ptr) WINSOCK_getservbyport
57  pascal   gethostname(ptr word) WINSOCK_gethostname
101 pascal16 WSAAsyncSelect(word word word long) WSAAsyncSelect
102 pascal16 WSAAsyncGetHostByAddr(word word ptr word word segptr word)
             WSAAsyncGetHostByAddr
103 pascal16 WSAAsyncGetHostByName(word word ptr segptr word)
             WSAAsyncGetHostByName
104 pascal16 WSAAsyncGetProtoByNumber(word word word segptr word)
             WSAAsyncGetProtoByNumber
105 pascal16 WSAAsyncGetProtoByName(word word ptr segptr word)
             WSAAsyncGetProtoByName
106 pascal16 WSAAsyncGetServByPort(word word word ptr segptr word)
             WSAAsyncGetServByPort
107 pascal16 WSAAsyncGetServByName(word word ptr ptr segptr word)
             WSAAsyncGetServByName
108 pascal16 WSACancelAsyncRequest(word) WSACancelAsyncRequest
109 pascal16 WSASetBlockingHook() WSASetBlockingHook
110 pascal16 WSAUnhookBlockingHook() WSAUnhookBlockingHook
111 pascal16 WSAGetLastError() WSAGetLastError
112 pascal   WSASetLastError(word) WSASetLastError
113 pascal16 WSACancelBlockingCall() WSACancelBlockingCall
114 pascal16 WSAIsBlocking() WSAIsBlocking
115 pascal   WSAStartup(word ptr) WSAStartup
116 pascal   WSACleanup() WSACleanup
151 pascal16 __WSAFDIsSet(word ptr) __WSAFDIsSet
