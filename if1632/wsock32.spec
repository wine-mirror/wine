name	wsock32
type	win32
base	0

001 stdcall accept(long ptr ptr) WINSOCK_accept32
002 stdcall bind(long ptr long) WINSOCK_bind32
003 stdcall closesocket(long) WINSOCK_closesocket32
004 stdcall connect(long ptr long) WINSOCK_connect32
005 stdcall getpeername(long ptr ptr) WINSOCK_getpeername32
006 stdcall getsockname(long ptr ptr) WINSOCK_getsockname32
007 stdcall getsockopt(long long long ptr ptr) WINSOCK_getsockopt32
008 stdcall htonl(long) WINSOCK_htonl
009 stdcall htons(long) WINSOCK_htons
010 stdcall inet_addr(ptr) inet_addr
011 stdcall inet_ntoa(ptr) inet_ntoa
012 stdcall ioctlsocket(long long ptr) WINSOCK_ioctlsocket32
013 stdcall listen(long long) WINSOCK_listen32
014 stdcall ntohl(long) WINSOCK_ntohl
015 stdcall ntohs(long) WINSOCK_ntohs
016 stdcall recv(long ptr long long) WINSOCK_recv32
017 stdcall recvfrom(long ptr long long ptr ptr) WINSOCK_recvfrom32
018 stdcall select(long ptr ptr ptr ptr) WINSOCK_select32
019 stdcall send(long ptr long long) WINSOCK_send32
020 stdcall sendto(long ptr long long ptr long) WINSOCK_sendto32
021 stdcall setsockopt(long long long ptr long) WINSOCK_setsockopt32
022 stdcall shutdown(long long) WINSOCK_shutdown32
023 stdcall socket(long long long) WINSOCK_socket32
051 stdcall gethostbyaddr(ptr long long) WINSOCK_gethostbyaddr32
052 stdcall gethostbyname(ptr) WINSOCK_gethostbyname32
053 stdcall getprotobyname(ptr) WINSOCK_getprotobyname32
054 stdcall getprotobynumber(long) WINSOCK_getprotobynumber32
055 stdcall getservbyname(ptr ptr) WINSOCK_getservbyname32
056 stdcall getservbyport(long ptr) WINSOCK_getservbyport32
057 stdcall gethostname(ptr long) WINSOCK_gethostname32
101 stub WSAAsyncSelect
102 stub WSAAsyncGetHostByAddr
103 stub WSAAsyncGetHostByName
104 stub WSAAsyncGetProtoByNumber
105 stub WSAAsyncGetProtoByName
106 stub WSAAsyncGetServByPort
107 stub WSAAsyncGetServByName
108 stub WSACancelAsyncRequest
109 stdcall WSASetBlockingHook(ptr) WSASetBlockingHook32
110 stdcall WSAUnhookBlockingHook() WSAUnhookBlockingHook32
111 stdcall WSAGetLastError() WSAGetLastError
112 stdcall WSASetLastError(long) WSASetLastError32
113 stdcall WSACancelBlockingCall() WSACancelBlockingCall
114 stdcall WSAIsBlocking() WSAIsBlocking
115 stdcall WSAStartup(long ptr) WSAStartup32
116 stdcall WSACleanup() WSACleanup
151 stdcall __WSAFDIsSet(long ptr) __WSAFDIsSet32
#500 stub WEP
# applications *should* 'degrade gracefully if these are not present
# ... as it is, they don't
#1000 stub WSApSetPostRoutine
1001 stdcall WsControl(long long long long long long) WsControl
1100 stdcall inet_network(ptr) inet_network
1101 stdcall getnetbyname(ptr) getnetbyname
#1102 stub rcmd
#1103 stub rexec
#1104 stub rresvport
#1105 stub sethostname
#1106 stub dn_expand
1107 stub WSARecvEx
1108 stub s_perror
1109 stub GetAddressByNameA
1110 stub GetAddressByNameW
#1111 stub EnumProtocolsA
#1112 stub EnumProtocolsW
#1113 stub GetTypeByNameA
#1114 stub GetTypeByNameW
#1115 stub GetNameByTypeA
#1116 stub GetNameByTypeW
#1117 stub SetServiceA
#1118 stub SetServiceW
#1119 stub GetServiceA
#1120 stub GetServiceW
#1130 stub NPLoadNameSpaces
#1140 stub TransmitFile
#1141 stub AcceptEx
#1142 stub GetAcceptExSockaddrs
