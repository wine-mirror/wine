#  Winsock 2 DLL ~ ws2_32.dll
#
#  Export table information obtained from Windows 2000 ws2_32.dll

1   stdcall  accept(long ptr ptr) WS_accept
2   stdcall  bind(long ptr long) WS_bind
3   stdcall  closesocket(long) WS_closesocket
4   stdcall  connect(long ptr long) WS_connect
5   stdcall  getpeername(long ptr ptr) WS_getpeername
6   stdcall  getsockname(long ptr ptr) WS_getsockname
7   stdcall  getsockopt(long long long ptr ptr) WS_getsockopt
8   stdcall  htonl(long) WS_htonl
9   stdcall  htons(long) WS_htons
10  stdcall  ioctlsocket(long long ptr) WS_ioctlsocket
11  stdcall  inet_addr(str) WS_inet_addr
12  stdcall  inet_ntoa(ptr) WS_inet_ntoa
13  stdcall  listen(long long) WS_listen
14  stdcall  ntohl(long) WS_ntohl
15  stdcall  ntohs(long) WS_ntohs
16  stdcall  recv(long ptr long long) WS_recv
17  stdcall  recvfrom(long ptr long long ptr ptr) WS_recvfrom
18  stdcall  select(long ptr ptr ptr ptr) WS_select
19  stdcall  send(long ptr long long) WS_send
20  stdcall  sendto(long ptr long long ptr long) WS_sendto
21  stdcall  setsockopt(long long long ptr long) WS_setsockopt
22  stdcall  shutdown(long long) WS_shutdown
23  stdcall  socket(long long long) WS_socket
24  stdcall  WSApSetPostRoutine(ptr)
25  stub     WPUCompleteOverlappedRequest
26  stdcall  WSAAccept(long ptr ptr ptr long)
27  stub     WSAAddressToStringA
28  stub     WSAAddressToStringW
29  stdcall  WSACloseEvent(long)
30  stdcall  WSAConnect(long ptr long ptr ptr ptr ptr)
31  stdcall  WSACreateEvent ()
32  stdcall  WSADuplicateSocketA(long long ptr)
33  stub     WSADuplicateSocketW
34  stub     WSAEnumNameSpaceProvidersA
35  stub     WSAEnumNameSpaceProvidersW
36  stdcall  WSAEnumNetworkEvents(long long ptr)
37  stdcall  WSAEnumProtocolsA(ptr ptr ptr)
38  stdcall  WSAEnumProtocolsW(ptr ptr ptr)
39  stdcall  WSAEventSelect(long long long)
40  stdcall  WSAGetOverlappedResult(long ptr ptr long ptr)
41  stub     WSAGetQOSByName
42  stub     WSAGetServiceClassInfoA
43  stub     WSAGetServiceClassInfoW
44  stub     WSAGetServiceClassNameByClassIdA
45  stub     WSAGetServiceClassNameByClassIdW
46  stub     WSAHtonl
47  stub     WSAHtons
48  stdcall  WSAInstallServiceClassA(ptr)
49  stdcall  WSAInstallServiceClassW(ptr)
50  stdcall  WSAIoctl(long long ptr long ptr long ptr ptr ptr)
51  stdcall  gethostbyaddr(ptr long long) WS_gethostbyaddr
52  stdcall  gethostbyname(str) WS_gethostbyname
53  stdcall  getprotobyname(str) WS_getprotobyname
54  stdcall  getprotobynumber(long) WS_getprotobynumber
55  stdcall  getservbyname(str str) WS_getservbyname
56  stdcall  getservbyport(long str) WS_getservbyport
57  stdcall  gethostname(ptr long) WS_gethostname
58  stub     WSAJoinLeaf
59  stub     WSALookupServiceBeginA
60  stub     WSALookupServiceBeginW
61  stub     WSALookupServiceEnd
62  stub     WSALookupServiceNextA
63  stub     WSALookupServiceNextW
64  stub     WSANtohl
65  stub     WSANtohs
66  stub     WSAProviderConfigChange
67  stdcall  WSARecv(long ptr long ptr ptr ptr ptr)
68  stub     WSARecvDisconnect
69  stdcall  WSARecvFrom(long ptr long ptr ptr ptr ptr ptr ptr )
70  stdcall  WSARemoveServiceClass(ptr)
71  stdcall  WSAResetEvent(long) kernel32.ResetEvent
72  stdcall  WSASend(long ptr long ptr long ptr ptr)
73  stdcall  WSASendDisconnect(long ptr)
74  stdcall  WSASendTo(long ptr long ptr long ptr long ptr ptr)
75  stub     WSASetEvent
76  stub     WSASetServiceA
77  stub     WSASetServiceW
78  stdcall  WSASocketA(long long long ptr long long)
79  stub     WSASocketW
80  stub     WSAStringToAddressA
81  stub     WSAStringToAddressW
82  stdcall  WSAWaitForMultipleEvents(long ptr long long long) kernel32.WaitForMultipleObjectsEx
83  stdcall  WSCDeinstallProvider(ptr ptr)
84  stub     WSCEnableNSProvider
85  stub     WSCEnumProtocols
86  stub     WSCGetProviderPath
87  stub     WSCInstallNameSpace
88  stdcall  WSCInstallProvider(ptr wstr ptr long ptr)
89  stub     WSCUnInstallNameSpace
90  stub     WSCWriteNameSpaceOrder
91  stub     WSCWriteProviderOrder

#  92 ~ 100   UNKNOWN

101 stdcall WSAAsyncSelect(long long long long)
102 stdcall WSAAsyncGetHostByAddr(long long ptr long long ptr long)
103 stdcall WSAAsyncGetHostByName(long long str ptr long)
104 stdcall WSAAsyncGetProtoByNumber(long long long ptr long)
105 stdcall WSAAsyncGetProtoByName(long long str ptr long)
106 stdcall WSAAsyncGetServByPort(long long long str ptr long)
107 stdcall WSAAsyncGetServByName(long long str str ptr long)
108 stdcall WSACancelAsyncRequest(long)
109 stdcall WSASetBlockingHook(ptr)
110 stdcall WSAUnhookBlockingHook()
111 stdcall WSAGetLastError()
112 stdcall WSASetLastError(long)
113 stdcall WSACancelBlockingCall()
114 stdcall WSAIsBlocking()
115 stdcall WSAStartup(long ptr)
116 stdcall WSACleanup()

#  117 ~ 150  UNKNOWN

151 stdcall  __WSAFDIsSet(long ptr)

#  152 ~ 499  UNKNOWN

500 stub     WEP
