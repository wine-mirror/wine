package ws2_32;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "accept" => ["long",  ["long", "ptr", "ptr"]],
    "bind" => ["long",  ["long", "ptr", "long"]],
    "closesocket" => ["long",  ["long"]],
    "connect" => ["long",  ["long", "ptr", "long"]],
    "getpeername" => ["long",  ["long", "ptr", "ptr"]],
    "getsockname" => ["long",  ["long", "ptr", "ptr"]],
    "getsockopt" => ["long",  ["long", "long", "long", "ptr", "ptr"]],
    "htonl" => ["long",  ["long"]],
    "htons" => ["long",  ["long"]],
    "ioctlsocket" => ["long",  ["long", "long", "ptr"]],
    "inet_addr" => ["long",  ["ptr"]],
    "inet_ntoa" => ["ptr",  ["unknown"]],
    "listen" => ["long",  ["long", "long"]],
    "ntohl" => ["long",  ["long"]],
    "ntohs" => ["long",  ["long"]],
    "recv" => ["long",  ["long", "ptr", "long", "long"]],
    "recvfrom" => ["long",  ["long", "ptr", "long", "long", "ptr", "ptr"]],
    "select" => ["long",  ["long", "ptr", "ptr", "ptr", "ptr"]],
    "send" => ["long",  ["long", "ptr", "long", "long"]],
    "sendto" => ["long",  ["long", "ptr", "long", "long", "ptr", "long"]],
    "setsockopt" => ["long",  ["long", "long", "long", "ptr", "long"]],
    "shutdown" => ["long",  ["long", "long"]],
    "socket" => ["long",  ["long", "long", "long"]],
    "WSApSetPostRoutine" => ["long",  ["ptr"]],
    "WSAAccept" => ["long",  ["long", "ptr", "ptr", "ptr", "long"]],
    "WSACloseEvent" => ["long",  ["long"]],
    "WSACreateEvent" => ["long",  []],
    "WSAEnumNetworkEvents" => ["long",  ["long", "long", "ptr"]],
    "WSAEventSelect" => ["long",  ["long", "long", "long"]],
    "gethostbyaddr" => ["ptr",  ["ptr", "long", "long"]],
    "gethostbyname" => ["ptr",  ["ptr"]],
    "getprotobyname" => ["ptr",  ["ptr"]],
    "getprotobynumber" => ["ptr",  ["long"]],
    "getservbyname" => ["ptr",  ["ptr", "ptr"]],
    "getservbyport" => ["ptr",  ["long", "ptr"]],
    "gethostname" => ["long",  ["ptr", "long"]],
    "WSARecvFrom" => ["long",  ["long", "ptr", "long", "ptr", "ptr", "ptr", "ptr", "ptr", "ptr"]],
    "WSASend" => ["long",  ["long", "ptr", "long", "ptr", "long", "ptr", "ptr"]],
    "WSASocketA" => ["long",  ["long", "long", "long", "ptr", "long", "long"]],
    "WSCDeinstallProvider" => ["long",  ["ptr", "ptr"]],
    "WSCInstallProvider" => ["long",  ["ptr", "wstr", "ptr", "long", "ptr"]],
    "WSAAsyncSelect" => ["long",  ["long", "long", "long", "long"]],
    "WSAAsyncGetHostByAddr" => ["long",  ["long", "long", "str", "long", "long", "str", "long"]],
    "WSAAsyncGetHostByName" => ["long",  ["long", "long", "str", "str", "long"]],
    "WSAAsyncGetProtoByNumber" => ["long",  ["long", "long", "long", "str", "long"]],
    "WSAAsyncGetProtoByName" => ["long",  ["long", "long", "str", "str", "long"]],
    "WSAAsyncGetServByPort" => ["long",  ["long", "long", "long", "str", "str", "long"]],
    "WSAAsyncGetServByName" => ["long",  ["long", "long", "str", "str", "str", "long"]],
    "WSACancelAsyncRequest" => ["long",  ["long"]],
    "WSASetBlockingHook" => ["ptr",  ["ptr"]],
    "WSAGetLastError" => ["long",  []],
    "WSASetLastError" => ["void",  ["long"]],
    "WSACancelBlockingCall" => ["long",  []],
    "WSAIsBlocking" => ["long",  []],
    "WSAStartup" => ["long",  ["long", "ptr"]],
    "WSACleanup" => ["long",  []],
    "__WSAFDIsSet" => ["long",  ["long", "ptr"]]
};

&wine::declare("ws2_32",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
