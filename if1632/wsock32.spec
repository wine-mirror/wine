name	wsock32
type	win32
base	0

001 stub accept
002 stub bind
003 stub closesocket
004 stub connect
005 stub getpeername
006 stub getsockname
007 stub getsockopt
008 stdcall htonl(long) htonl
009 stdcall htons(long) htons
010 stdcall inet_addr(ptr) inet_addr
011 stdcall inet_ntoa(ptr) inet_ntoa
012 stub ioctlsocket
013 stub listen
014 stub ntohl
015 stdcall ntohs(long) ntohs
016 stub recv
017 stub recvfrom
018 stub select
019 stub send
020 stub sendto
021 stub setsockopt
022 stub shutdown
023 stub socket
051 stdcall gethostbyaddr(ptr long long) gethostbyaddr
052 stdcall gethostbyname(ptr) gethostbyname
053 stub getprotobyname
054 stub getprotobynumber
055 stdcall getservbyname(ptr ptr) getservbyname
056 stub getservbyport
057 stdcall gethostname(ptr long) gethostname
101 stub WSAAsyncSelect
102 stub WSAAsyncGetHostByAddr
103 stub WSAAsyncGetHostByName
104 stub WSAAsyncGetProtoByNumber
105 stub WSAAsyncGetProtoByName
106 stub WSAAsyncGetServByPort
107 stub WSAAsyncGetServByName
108 stub WSACancelAsyncRequest
109 stub WSASetBlockingHook
110 stub WSAUnhookBlockingHook
111 stub WSAGetLastError
112 stub WSASetLastError
113 stub WSACancelBlockingCall
114 stub WSAIsBlocking
115 stdcall WSAStartup(long ptr) WSAStartup
116 stdcall WSACleanup() WSACleanup
151 stub __WSAFDIsSet
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
