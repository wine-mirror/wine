#  
#         File: winsock.def 
#       System: MS-Windows 3.x 
#      Summary: Module definition file for Windows Sockets DLL.  
#  
name	winsock
type	win16

1   pascal16 accept(word ptr ptr) WINSOCK_accept16
2   pascal16 bind(word ptr word) WINSOCK_bind16
3   pascal16 closesocket(word) WINSOCK_closesocket16
4   pascal16 connect(word ptr word) WINSOCK_connect16
5   pascal16 getpeername(word ptr ptr) WINSOCK_getpeername16
6   pascal16 getsockname(word ptr ptr) WINSOCK_getsockname16
7   pascal16 getsockopt(word word word ptr ptr) WINSOCK_getsockopt16
8   pascal   htonl(long) WINSOCK_htonl
9   pascal16 htons(word) WINSOCK_htons
10  pascal   inet_addr(ptr) WINSOCK_inet_addr
11  pascal   inet_ntoa(long) WINSOCK_inet_ntoa
12  pascal16 ioctlsocket(word long ptr) WINSOCK_ioctlsocket16
13  pascal16 listen(word word) WINSOCK_listen16
14  pascal   ntohl(long) WINSOCK_ntohl
15  pascal16 ntohs(word) WINSOCK_ntohs
16  pascal16 recv(word ptr word word) WINSOCK_recv16
17  pascal16 recvfrom(word ptr word word ptr ptr) WINSOCK_recvfrom16
18  pascal16 select(word ptr ptr ptr ptr) WINSOCK_select16
19  pascal16 send(word ptr word word) WINSOCK_send16
20  pascal16 sendto(word ptr word word ptr word) WINSOCK_sendto16
21  pascal16 setsockopt(word word word ptr word) WINSOCK_setsockopt16
22  pascal16 shutdown(word word) WINSOCK_shutdown16
23  pascal16 socket(word word word) WINSOCK_socket16
51  pascal   gethostbyaddr(ptr word word) WINSOCK_gethostbyaddr16
52  pascal   gethostbyname(ptr) WINSOCK_gethostbyname16
53  pascal   getprotobyname(ptr) WINSOCK_getprotobyname16
54  pascal   getprotobynumber(word) WINSOCK_getprotobynumber16
55  pascal   getservbyname(ptr ptr) WINSOCK_getservbyname16
56  pascal   getservbyport(word ptr) WINSOCK_getservbyport16
57  pascal   gethostname(ptr word) WINSOCK_gethostname16
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
109 pascal16 WSASetBlockingHook(segptr) WSASetBlockingHook16
110 pascal16 WSAUnhookBlockingHook() WSAUnhookBlockingHook16
111 pascal16 WSAGetLastError() WSAGetLastError
112 pascal   WSASetLastError(word) WSASetLastError16
113 pascal16 WSACancelBlockingCall() WSACancelBlockingCall
114 pascal16 WSAIsBlocking() WSAIsBlocking
115 pascal   WSAStartup(word ptr) WSAStartup16
116 pascal   WSACleanup() WSACleanup
151 pascal16 __WSAFDIsSet(word ptr) __WSAFDIsSet16
