#  Winsock 2 DLL ~ ws2_32.dll
#
#  Export table information obtained from Windows 2000 ws2_32.dll

name ws2_32
type win32
init WSOCK32_LibMain

#  EXPORTS ***********
1   stdcall  accept(long ptr ptr) WSOCK32_accept
2   stdcall  bind(long ptr long) WSOCK32_bind
3   stdcall  closesocket(long) WSOCK32_closesocket
4   stdcall  connect(long ptr long) WSOCK32_connect
5   stdcall  getpeername(long ptr ptr) WSOCK32_getpeername
6   stdcall  getsockname(long ptr ptr) WSOCK32_getsockname
7   stdcall  getsockopt(long long long ptr ptr) WSOCK32_getsockopt
8   stdcall  htonl(long) WINSOCK_htonl
9   stdcall  htons(long) WINSOCK_htons
10  stdcall  ioctlsocket(long long ptr) WSOCK32_ioctlsocket
11  stdcall  inet_addr(str) WINSOCK_inet_addr
12  stdcall  inet_ntoa(ptr) WSOCK32_inet_ntoa
13  stdcall  listen(long long) WSOCK32_listen
14  stdcall  ntohl(long) WINSOCK_ntohl
15  stdcall  ntohs(long) WINSOCK_ntohs
16  stdcall  recv(long ptr long long) WSOCK32_recv
17  stdcall  recvfrom(long ptr long long ptr ptr) WSOCK32_recvfrom
18  stdcall  select(long ptr ptr ptr ptr) WSOCK32_select
19  stdcall  send(long ptr long long) WSOCK32_send
20  stdcall  sendto(long ptr long long ptr long) WSOCK32_sendto
21  stdcall  setsockopt(long long long ptr long) WSOCK32_setsockopt
22  stdcall  shutdown(long long) WSOCK32_shutdown
23  stdcall  socket(long long long) WSOCK32_socket
24  stub     WSApSetPostRoutine
25  stub     WPUCompleteOverlappedRequest
26  stub     WSAAccept
27  stub     WSAAddressToStringA
28  stub     WSAAddressToStringW
29  stdcall  WSACloseEvent(long) WSACloseEvent
30  stub     WSAConnect
31  stdcall  WSACreateEvent ()  WSACreateEvent
32  stub     WSADuplicateSocketA
33  stub     WSADuplicateSocketW
34  stub     WSAEnumNameSpaceProvidersA
35  stub     WSAEnumNameSpaceProvidersW
36  stdcall  WSAEnumNetworkEvents(long long ptr) WSAEnumNetworkEvents
37  stub     WSAEnumProtocolsA
38  stub     WSAEnumProtocolsW
39  stdcall  WSAEventSelect(long long long) WSAEventSelect
40  stub     WSAGetOverlappedResult
41  stub     WSAGetQOSByName
42  stub     WSAGetServiceClassInfoA
43  stub     WSAGetServiceClassInfoW
44  stub     WSAGetServiceClassNameByClassIdA
45  stub     WSAGetServiceClassNameByClassIdW
46  stub     WSAHtonl
47  stub     WSAHtons
48  stub     WSAInstallServiceClassA
49  stub     WSAInstallServiceClassW
50  stub     WSAIoctl
51  stdcall  gethostbyaddr(ptr long long) WSOCK32_gethostbyaddr
52  stdcall  gethostbyname(str) WSOCK32_gethostbyname
53  stdcall  getprotobyname(str) WSOCK32_getprotobyname
54  stdcall  getprotobynumber(long) WSOCK32_getprotobynumber
55  stdcall  getservbyname(str str) WSOCK32_getservbyname
56  stdcall  getservbyport(long str) WSOCK32_getservbyport
57  stdcall  gethostname(ptr long) WSOCK32_gethostname
58  stub     WSAJoinLeaf
59  stub     WSALookupServiceBeginA
60  stub     WSALookupServiceBeginW
61  stub     WSALookupServiceEnd
62  stub     WSALookupServiceNextA
63  stub     WSALookupServiceNextW
64  stub     WSANtohl
65  stub     WSANtohs
66  stub     WSAProviderConfigChange
67  stub     WSARecv
68  stub     WSARecvDisconnect
69  stub     WSARecvFrom
70  stub     WSARemoveServiceClass
71  stub     WSAResetEvent
72  stub     WSASend
73  stub     WSASendDisconnect
74  stub     WSASendTo
75  stub     WSASetEvent
76  stub     WSASetServiceA
77  stub     WSASetServiceW
78  stdcall  WSASocketA(long long long ptr long long) WSASocketA
79  stub     WSASocketW
80  stub     WSAStringToAddressA
81  stub     WSAStringToAddressW
82  stub     WSAWaitForMultipleEvents
83  stub     WSCDeinstallProvider
84  stub     WSCEnableNSProvider
85  stub     WSCEnumProtocols
86  stub     WSCGetProviderPath
87  stub     WSCInstallNameSpace
88  stub     WSCInstallProvider
89  stub     WSCUnInstallNameSpace
90  stub     WSCWriteNameSpaceOrder
91  stub     WSCWriteProviderOrder

#  92 ~ 100   UNKNOWN

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
115 stdcall WSAStartup (long ptr)  WS2_WSAStartup
116 stdcall WSACleanup ()          WSACleanup

#  117 ~ 150  UNKNOWN

151 stdcall  __WSAFDIsSet(long ptr) __WSAFDIsSet

#  152 ~ 499  UNKNOWN
 
500 stub     WEP
