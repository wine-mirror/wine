name	wsock32
type	win32

import	ws2_32.dll

  1 forward accept ws2_32.accept
  2 forward bind ws2_32.bind
  3 forward closesocket ws2_32.closesocket
  4 forward connect ws2_32.connect
  5 forward getpeername ws2_32.getpeername
  6 forward getsockname ws2_32.getsockname
  7 forward getsockopt ws2_32.getsockopt
  8 forward htonl ws2_32.htonl
  9 forward htons ws2_32.htons
 10 forward inet_addr ws2_32.inet_addr
 11 forward inet_ntoa ws2_32.inet_ntoa
 12 forward ioctlsocket ws2_32.ioctlsocket
 13 forward listen ws2_32.listen
 14 forward ntohl ws2_32.ntohl
 15 forward ntohs ws2_32.ntohs
 16 forward recv ws2_32.recv
 17 forward recvfrom ws2_32.recvfrom
 18 forward select ws2_32.select
 19 forward send ws2_32.send
 20 forward sendto ws2_32.sendto
 21 forward setsockopt ws2_32.setsockopt
 22 forward shutdown ws2_32.shutdown
 23 forward socket ws2_32.socket
 51 forward gethostbyaddr ws2_32.gethostbyaddr
 52 forward gethostbyname ws2_32.gethostbyname
 53 forward getprotobyname ws2_32.getprotobyname
 54 forward getprotobynumber ws2_32.getprotobynumber
 55 forward getservbyname ws2_32.getservbyname
 56 forward getservbyport ws2_32.getservbyport
 57 forward gethostname ws2_32.gethostname
101 forward WSAAsyncSelect ws2_32.WSAAsyncSelect
102 forward WSAAsyncGetHostByAddr ws2_32.WSAAsyncGetHostByAddr
103 forward WSAAsyncGetHostByName ws2_32.WSAAsyncGetHostByName
104 forward WSAAsyncGetProtoByNumber ws2_32.WSAAsyncGetProtoByNumber
105 forward WSAAsyncGetProtoByName ws2_32.WSAAsyncGetProtoByName
106 forward WSAAsyncGetServByPort ws2_32.WSAAsyncGetServByPort
107 forward WSAAsyncGetServByName ws2_32.WSAAsyncGetServByName
108 forward WSACancelAsyncRequest ws2_32.WSACancelAsyncRequest
109 forward WSASetBlockingHook ws2_32.WSASetBlockingHook
110 forward WSAUnhookBlockingHook ws2_32.WSAUnhookBlockingHook
111 forward WSAGetLastError ws2_32.WSAGetLastError
112 forward WSASetLastError ws2_32.WSASetLastError
113 forward WSACancelBlockingCall ws2_32.WSACancelBlockingCall
114 forward WSAIsBlocking ws2_32.WSAIsBlocking
115 forward WSAStartup ws2_32.WSAStartup
116 forward WSACleanup ws2_32.WSACleanup
151 forward __WSAFDIsSet ws2_32.__WSAFDIsSet
500 forward WEP ws2_32.WEP

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
