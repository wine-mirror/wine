name	wsock32
type	win32
init	WSOCK32_LibMain

001 stdcall accept(long ptr ptr) WINSOCK_accept
002 stdcall bind(long ptr long) WINSOCK_bind
003 stdcall closesocket(long) WINSOCK_closesocket
004 stdcall connect(long ptr long) WINSOCK_connect
005 stdcall getpeername(long ptr ptr) WINSOCK_getpeername
006 stdcall getsockname(long ptr ptr) WINSOCK_getsockname
007 stdcall getsockopt(long long long ptr ptr) WINSOCK_getsockopt
008 stdcall htonl(long) WINSOCK_htonl
009 stdcall htons(long) WINSOCK_htons
010 stdcall inet_addr(str) WINSOCK_inet_addr
011 stdcall inet_ntoa(ptr) WINSOCK_inet_ntoa
012 stdcall ioctlsocket(long long ptr) WINSOCK_ioctlsocket
013 stdcall listen(long long) WINSOCK_listen
014 stdcall ntohl(long) WINSOCK_ntohl
015 stdcall ntohs(long) WINSOCK_ntohs
016 stdcall recv(long ptr long long) WINSOCK_recv
017 stdcall recvfrom(long ptr long long ptr ptr) WINSOCK_recvfrom
018 stdcall select(long ptr ptr ptr ptr) WINSOCK_select
019 stdcall send(long ptr long long) WINSOCK_send
020 stdcall sendto(long ptr long long ptr long) WINSOCK_sendto
021 stdcall setsockopt(long long long ptr long) WINSOCK_setsockopt
022 stdcall shutdown(long long) WINSOCK_shutdown
023 stdcall socket(long long long) WINSOCK_socket
051 stdcall gethostbyaddr(ptr long long) WINSOCK_gethostbyaddr
052 stdcall gethostbyname(str) WINSOCK_gethostbyname
053 stdcall getprotobyname(str) WINSOCK_getprotobyname
054 stdcall getprotobynumber(long) WINSOCK_getprotobynumber
055 stdcall getservbyname(str str) WINSOCK_getservbyname
056 stdcall getservbyport(long str) WINSOCK_getservbyport
057 stdcall gethostname(ptr long) WINSOCK_gethostname
101 stdcall WSAAsyncSelect(long long long long) WSAAsyncSelect
102 stdcall WSAAsyncGetHostByAddr(long long ptr long long ptr long) WSAAsyncGetHostByAddr
103 stdcall WSAAsyncGetHostByName(long long str ptr long) WSAAsyncGetHostByName
104 stdcall WSAAsyncGetProtoByNumber(long long long ptr long) WSAAsyncGetProtoByNumber
105 stdcall WSAAsyncGetProtoByName(long long str ptr long) WSAAsyncGetProtoByName
106 stdcall WSAAsyncGetServByPort(long long long str ptr long) WSAAsyncGetServByPort
107 stdcall WSAAsyncGetServByName(long long str str ptr long) WSAAsyncGetServByName
108 stdcall WSACancelAsyncRequest(long) WSACancelAsyncRequest
109 stdcall WSASetBlockingHook(ptr) WSASetBlockingHook
110 stdcall WSAUnhookBlockingHook() WSAUnhookBlockingHook
111 stdcall WSAGetLastError() WSAGetLastError
112 stdcall WSASetLastError(long) WSASetLastError
113 stdcall WSACancelBlockingCall() WSACancelBlockingCall
114 stdcall WSAIsBlocking() WSAIsBlocking
115 stdcall WSAStartup(long ptr) WSAStartup
116 stdcall WSACleanup() WSACleanup
151 stdcall __WSAFDIsSet(long ptr) __WSAFDIsSet
#500 stub WEP
# applications *should* 'degrade gracefully if these are not present
# ... as it is, they don't
#1000 stub WSApSetPostRoutine
1001 stdcall WsControl(long long ptr ptr ptr ptr) WsControl
1100 stdcall inet_network(str) inet_network
1101 stdcall getnetbyname(str) getnetbyname
#1102 stub rcmd
#1103 stub rexec
#1104 stub rresvport
#1105 stub sethostname
#1106 stub dn_expand
1107 stdcall WSARecvEx(long ptr long ptr) WSARecvEx
1108 stdcall s_perror(str) WS_s_perror
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
