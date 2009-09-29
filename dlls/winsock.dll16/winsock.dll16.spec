1   pascal -ret16 accept(word ptr ptr) accept16
2   pascal -ret16 bind(word ptr word) bind16
3   pascal -ret16 closesocket(word) closesocket16
4   pascal -ret16 connect(word ptr word) connect16
5   pascal -ret16 getpeername(word ptr ptr) getpeername16
6   pascal -ret16 getsockname(word ptr ptr) getsockname16
7   pascal -ret16 getsockopt(word word word ptr ptr) getsockopt16
8   pascal htonl(long) htonl16
9   pascal -ret16 htons(word) htons16
10  pascal inet_addr(ptr) inet_addr16
11  pascal inet_ntoa(long) inet_ntoa16
12  pascal -ret16 ioctlsocket(word long ptr) ioctlsocket16
13  pascal -ret16 listen(word word) listen16
14  pascal ntohl(long) ntohl16
15  pascal -ret16 ntohs(word) ntohs16
16  pascal -ret16 recv(word ptr word word) recv16
17  pascal -ret16 recvfrom(word ptr word word ptr ptr) recvfrom16
18  pascal -ret16 select(word ptr ptr ptr ptr) select16
19  pascal -ret16 send(word ptr word word) send16
20  pascal -ret16 sendto(word ptr word word ptr word) sendto16
21  pascal -ret16 setsockopt(word word word ptr word) setsockopt16
22  pascal -ret16 shutdown(word word) shutdown16
23  pascal -ret16 socket(word word word) socket16
51  pascal gethostbyaddr(ptr word word) gethostbyaddr16
52  pascal gethostbyname(ptr) gethostbyname16
53  pascal getprotobyname(ptr) getprotobyname16
54  pascal getprotobynumber(word) getprotobynumber16
55  pascal getservbyname(ptr ptr) getservbyname16
56  pascal getservbyport(word ptr) getservbyport16
57  pascal gethostname(ptr word) gethostname16
101 pascal -ret16 WSAAsyncSelect(word word word long) WSAAsyncSelect16
102 pascal -ret16 WSAAsyncGetHostByAddr(word word ptr word word segptr word) WSAAsyncGetHostByAddr16
103 pascal -ret16 WSAAsyncGetHostByName(word word str segptr word) WSAAsyncGetHostByName16
104 pascal -ret16 WSAAsyncGetProtoByNumber(word word word segptr word) WSAAsyncGetProtoByNumber16
105 pascal -ret16 WSAAsyncGetProtoByName(word word str segptr word) WSAAsyncGetProtoByName16
106 pascal -ret16 WSAAsyncGetServByPort(word word word str segptr word) WSAAsyncGetServByPort16
107 pascal -ret16 WSAAsyncGetServByName(word word str str segptr word) WSAAsyncGetServByName16
108 pascal -ret16 WSACancelAsyncRequest(word) WSACancelAsyncRequest16
109 pascal -ret16 WSASetBlockingHook(segptr) WSASetBlockingHook16
110 pascal -ret16 WSAUnhookBlockingHook() WSAUnhookBlockingHook16
111 pascal -ret16 WSAGetLastError() WSAGetLastError16
112 pascal   WSASetLastError(word) WSASetLastError16
113 pascal -ret16 WSACancelBlockingCall() WSACancelBlockingCall16
114 pascal -ret16 WSAIsBlocking() WSAIsBlocking16
115 pascal   WSAStartup(word ptr) WSAStartup16
116 pascal   WSACleanup() WSACleanup16
151 pascal -ret16 __WSAFDIsSet(word ptr) __WSAFDIsSet16
1107 pascal -ret16 WSARecvEx(word ptr word ptr) WSARecvEx16
