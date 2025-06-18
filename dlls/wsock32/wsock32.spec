  1 stdcall accept(long ptr ptr) ws2_32.accept
  2 stdcall bind(long ptr long) ws2_32.bind
  3 stdcall closesocket(long) ws2_32.closesocket
  4 stdcall connect(long ptr long) ws2_32.connect
  5 stdcall getpeername(long ptr ptr) ws2_32.getpeername
  6 stdcall getsockname(long ptr ptr) ws2_32.getsockname
  7 stdcall getsockopt(long long long ptr ptr) WS1_getsockopt
  8 stdcall htonl(long) ws2_32.htonl
  9 stdcall htons(long) ws2_32.htons
 10 stdcall inet_addr(str) ws2_32.inet_addr
 11 stdcall inet_ntoa(ptr) ws2_32.inet_ntoa
 12 stdcall ioctlsocket(long long ptr) ws2_32.ioctlsocket
 13 stdcall listen(long long) ws2_32.listen
 14 stdcall ntohl(long) ws2_32.ntohl
 15 stdcall ntohs(long) ws2_32.ntohs
 16 stdcall recv(long ptr long long) ws2_32.recv
 17 stdcall recvfrom(long ptr long long ptr ptr) ws2_32.recvfrom
 18 stdcall select(long ptr ptr ptr ptr) ws2_32.select
 19 stdcall send(long ptr long long) ws2_32.send
 20 stdcall sendto(long ptr long long ptr long) ws2_32.sendto
 21 stdcall setsockopt(long long long ptr long) WS1_setsockopt
 22 stdcall shutdown(long long) ws2_32.shutdown
 23 stdcall socket(long long long) ws2_32.socket
 51 stdcall gethostbyaddr(ptr long long) ws2_32.gethostbyaddr
 52 stdcall gethostbyname(str) ws2_32.gethostbyname
 53 stdcall getprotobyname(str) ws2_32.getprotobyname
 54 stdcall getprotobynumber(long) ws2_32.getprotobynumber
 55 stdcall getservbyname(str str) ws2_32.getservbyname
 56 stdcall getservbyport(long str) ws2_32.getservbyport
 57 stdcall gethostname(ptr long) ws2_32.gethostname
101 stdcall WSAAsyncSelect(long long long long) ws2_32.WSAAsyncSelect
102 stdcall WSAAsyncGetHostByAddr(long long ptr long long ptr long) ws2_32.WSAAsyncGetHostByAddr
103 stdcall WSAAsyncGetHostByName(long long str ptr long) ws2_32.WSAAsyncGetHostByName
104 stdcall WSAAsyncGetProtoByNumber(long long long ptr long) ws2_32.WSAAsyncGetProtoByNumber
105 stdcall WSAAsyncGetProtoByName(long long str ptr long) ws2_32.WSAAsyncGetProtoByName
106 stdcall WSAAsyncGetServByPort(long long long str ptr long) ws2_32.WSAAsyncGetServByPort
107 stdcall WSAAsyncGetServByName(long long str str ptr long) ws2_32.WSAAsyncGetServByName
108 stdcall WSACancelAsyncRequest(long) ws2_32.WSACancelAsyncRequest
109 stdcall WSASetBlockingHook(ptr) ws2_32.WSASetBlockingHook
110 stdcall WSAUnhookBlockingHook() ws2_32.WSAUnhookBlockingHook
111 stdcall WSAGetLastError() ws2_32.WSAGetLastError
112 stdcall WSASetLastError(long) ws2_32.WSASetLastError
113 stdcall WSACancelBlockingCall() ws2_32.WSACancelBlockingCall
114 stdcall WSAIsBlocking() ws2_32.WSAIsBlocking
115 stdcall WSAStartup(long ptr) ws2_32.WSAStartup
116 stdcall WSACleanup() ws2_32.WSACleanup
151 stdcall __WSAFDIsSet(long ptr) ws2_32.__WSAFDIsSet
500 stdcall WEP() ws2_32.WEP

# applications *should* 'degrade gracefully if these are not present
# ... as it is, they don't
#1000 stub WSApSetPostRoutine
1001 stdcall WsControl(long long ptr ptr ptr ptr)
1100 stdcall inet_network(str) WSOCK32_inet_network
1101 stdcall getnetbyname(str) WSOCK32_getnetbyname
#1102 stub rcmd
#1103 stub rexec
#1104 stub rresvport
#1105 stub sethostname
#1106 stub dn_expand
1107 stdcall WSARecvEx(long ptr long ptr)
1108 stdcall s_perror(str)
1109 stdcall GetAddressByNameA(long ptr str ptr long ptr ptr ptr ptr ptr)
1110 stdcall GetAddressByNameW(long ptr wstr ptr long ptr ptr ptr ptr ptr)
1111 stdcall EnumProtocolsA(ptr ptr ptr)
1112 stdcall EnumProtocolsW(ptr ptr ptr)
1113 stdcall GetTypeByNameA(str ptr)
1114 stdcall GetTypeByNameW(wstr ptr)
#1115 stub GetNameByTypeA
#1116 stub GetNameByTypeW
1117 stdcall SetServiceA(long long long ptr ptr ptr)
1118 stdcall SetServiceW(long long long ptr ptr ptr)
1119 stdcall GetServiceA(long ptr ptr long ptr ptr ptr)
1120 stdcall GetServiceW(long ptr ptr long ptr ptr ptr)
#1130 stub NPLoadNameSpaces
1140 stdcall TransmitFile(long ptr long long ptr long) mswsock.TransmitFile
1141 stdcall AcceptEx(long long ptr long long long ptr ptr) mswsock.AcceptEx
1142 stdcall GetAcceptExSockaddrs(ptr long long long ptr ptr ptr ptr) mswsock.GetAcceptExSockaddrs
