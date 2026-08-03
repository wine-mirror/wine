@ stdcall WSAAccept(long ptr ptr ptr long) WINECE_WSAAccept
@ stdcall WSAAddressToStringW(ptr long ptr ptr ptr) WINECE_WSAAddressToStringW
@ stub WSAAsyncGetAddrInfo
@ stdcall WSAAsyncGetHostByName(long long str ptr long) WINECE_WSAAsyncGetHostByName
@ stdcall WSAAsyncSelect(long long long long) WINECE_WSAAsyncSelect
@ stdcall WSACancelAsyncRequest(long) WINECE_WSACancelAsyncRequest
@ stdcall WSACleanup() WINECE_WSACleanup
@ stdcall WSACloseEvent(long) WINECE_WSACloseEvent
@ stdcall WSAConnect(long ptr long ptr ptr ptr ptr) WINECE_WSAConnect
@ stdcall WSACreateEvent() WINECE_WSACreateEvent
@ stdcall WSAEnumNameSpaceProvidersW(ptr ptr) WINECE_WSAEnumNameSpaceProvidersW
@ stdcall WSAEnumNetworkEvents(long long ptr) WINECE_WSAEnumNetworkEvents
@ stdcall WSAEnumProtocolsW(ptr ptr ptr) WINECE_WSAEnumProtocolsW
@ stdcall WSAEventSelect(long long long) WINECE_WSAEventSelect
@ stdcall WSAGetLastError() WINECE_WSAGetLastError
@ stdcall WSAGetOverlappedResult(long ptr ptr long ptr) WINECE_WSAGetOverlappedResult
@ stdcall WSAHtonl(long long ptr) WINECE_WSAHtonl
@ stdcall WSAHtons(long long ptr) WINECE_WSAHtons
@ stdcall WSAIoctl(long long ptr long ptr long ptr ptr ptr) WINECE_WSAIoctl
@ stdcall WSAJoinLeaf(long ptr long ptr ptr ptr ptr long) WINECE_WSAJoinLeaf
@ stdcall WSALookupServiceBeginW(ptr long ptr) WINECE_WSALookupServiceBeginW
@ stdcall WSALookupServiceEnd(long) WINECE_WSALookupServiceEnd
@ stdcall WSALookupServiceNextW(long long ptr ptr) WINECE_WSALookupServiceNextW
@ stdcall WSANtohl(long long ptr) WINECE_WSANtohl
@ stdcall WSANtohs(long long ptr) WINECE_WSANtohs
@ stdcall WSARecv(long ptr long ptr ptr ptr ptr) WINECE_WSARecv
@ stdcall WSARecvFrom(long ptr long ptr ptr ptr ptr ptr ptr ) WINECE_WSARecvFrom
@ stdcall WSAResetEvent(long) WINECE_WSAResetEvent
@ stdcall WSASend(long ptr long ptr long ptr ptr) WINECE_WSASend
@ stdcall WSASendTo(long ptr long ptr long ptr long ptr ptr) WINECE_WSASendTo
@ stdcall WSASetEvent(long) WINECE_WSASetEvent
@ stdcall WSASetLastError(long) WINECE_WSASetLastError
@ stdcall WSASetServiceW(ptr long long) WINECE_WSASetServiceW
@ stdcall WSASocketW(long long long ptr long long) WINECE_WSASocketW
@ stdcall WSAStartup(long ptr) WINECE_WSAStartup
@ stdcall WSAStringToAddressW(wstr long ptr ptr ptr) WINECE_WSAStringToAddressW
@ stdcall WSAWaitForMultipleEvents(long ptr long long long) WINECE_WSAWaitForMultipleEvents
@ stdcall WSCDeinstallProvider(ptr ptr) WINECE_WSCDeinstallProvider
@ stdcall WSCEnumProtocols(ptr ptr ptr ptr) WINECE_WSCEnumProtocols
@ stdcall WSCInstallNameSpace(wstr wstr long long ptr) WINECE_WSCInstallNameSpace
@ stdcall WSCInstallProvider(ptr wstr ptr long ptr) WINECE_WSCInstallProvider
@ stdcall WSCUnInstallNameSpace(ptr) WINECE_WSCUnInstallNameSpace
@ stdcall __WSAFDIsSet(long ptr) WINECE___WSAFDIsSet
@ stdcall accept(long ptr ptr) WINECE_accept
@ stdcall bind(long ptr long) WINECE_bind
@ stdcall closesocket(long) WINECE_closesocket
@ stdcall connect(long ptr long) WINECE_connect
@ stdcall freeaddrinfo(ptr) WINECE_freeaddrinfo
@ stdcall getaddrinfo(str str ptr ptr) WINECE_getaddrinfo
@ stdcall gethostbyaddr(ptr long long) WINECE_gethostbyaddr
@ stdcall gethostbyname(str) WINECE_gethostbyname
@ stdcall gethostname(ptr long) WINECE_gethostname
@ stdcall getnameinfo(ptr long ptr long ptr long long) WINECE_getnameinfo
@ stdcall getpeername(long ptr ptr) WINECE_getpeername
@ stdcall getprotobyname(str) WINECE_getprotobyname
@ stdcall getprotobynumber(long) WINECE_getprotobynumber
@ stdcall getservbyname(str str) WINECE_getservbyname
@ stdcall getservbyport(long str) WINECE_getservbyport
@ stdcall getsockname(long ptr ptr) WINECE_getsockname
@ stdcall getsockopt(long long long ptr ptr) WINECE_getsockopt
@ stdcall htonl(long) WINECE_htonl
@ stdcall htons(long) WINECE_htons
@ stub in6addr_any
@ stub in6addr_loopback
@ stdcall inet_addr(str) WINECE_inet_addr
@ stdcall inet_ntoa(ptr) WINECE_inet_ntoa
@ stdcall ioctlsocket(long long ptr) WINECE_ioctlsocket
@ stdcall listen(long long) WINECE_listen
@ stdcall ntohl(long) WINECE_ntohl
@ stdcall ntohs(long) WINECE_ntohs
@ stdcall recv(long ptr long long) WINECE_recv
@ stdcall recvfrom(long ptr long long ptr ptr) WINECE_recvfrom
@ stdcall select(long ptr ptr ptr ptr) WINECE_select
@ stdcall send(long ptr long long) WINECE_send
@ stdcall sendto(long ptr long long ptr long) WINECE_sendto
@ stub sethostname
@ stdcall setsockopt(long long long ptr long) WINECE_setsockopt
@ stdcall shutdown(long long) WINECE_shutdown
@ stdcall socket(long long long) WINECE_socket
