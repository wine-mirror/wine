package wininet;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "CommitUrlCacheEntryA" => ["long",  ["str", "str", "long", "long", "long", "ptr", "long", "str", "long"]],
    "DllInstall" => ["long",  ["long", "wstr"]],
    "FtpCreateDirectoryA" => ["long",  ["long", "str"]],
    "FtpDeleteFileA" => ["long",  ["long", "str"]],
    "FtpFindFirstFileA" => ["long",  ["long", "str", "ptr", "long", "long"]],
    "FtpGetCurrentDirectoryA" => ["long",  ["long", "str", "ptr"]],
    "FtpGetFileA" => ["long",  ["long", "str", "str", "long", "long", "long", "long"]],
    "FtpOpenFileA" => ["long",  ["long", "str", "long", "long", "long"]],
    "FtpPutFileA" => ["long",  ["long", "str", "str", "long", "long"]],
    "FtpRemoveDirectoryA" => ["long",  ["long", "str"]],
    "FtpRenameFileA" => ["long",  ["long", "str", "str"]],
    "FtpSetCurrentDirectoryA" => ["long",  ["long", "str"]],
    "GetUrlCacheEntryInfoA" => ["long",  ["str", "ptr", "ptr"]],
    "HttpAddRequestHeadersA" => ["long",  ["long", "str", "long", "long"]],
    "HttpOpenRequestA" => ["long",  ["long", "str", "str", "str", "str", "ptr", "long", "long"]],
    "HttpQueryInfoA" => ["long",  ["long", "long", "ptr", "ptr", "ptr"]],
    "HttpSendRequestA" => ["long",  ["long", "str", "long", "ptr", "long"]],
    "HttpSendRequestExA" => ["long",  ["long", "ptr", "ptr", "long", "long"]],
    "InternetAttemptConnect" => ["long",  ["long"]],
    "InternetCanonicalizeUrlA" => ["long",  ["str", "str", "ptr", "long"]],
    "InternetCheckConnectionA" => ["long",  ["str", "long", "long"]],
    "InternetCloseHandle" => ["long",  ["long"]],
    "InternetConnectA" => ["long",  ["long", "str", "long", "str", "str", "long", "long", "long"]],
    "InternetCrackUrlA" => ["long",  ["str", "long", "long", "ptr"]],
    "InternetFindNextFileA" => ["long",  ["long", "ptr"]],
    "InternetGetConnectedState" => ["long",  ["ptr", "long"]],
    "InternetGetCookieA" => ["long",  ["str", "str", "str", "ptr"]],
    "InternetGetLastResponseInfoA" => ["long",  ["ptr", "str", "ptr"]],
    "InternetOpenA" => ["long",  ["str", "long", "str", "str", "long"]],
    "InternetOpenUrlA" => ["long",  ["long", "str", "str", "long", "long", "long"]],
    "InternetQueryOptionA" => ["long",  ["long", "long", "ptr", "ptr"]],
    "InternetReadFile" => ["long",  ["long", "ptr", "long", "ptr"]],
    "InternetSetCookieA" => ["long",  ["str", "str", "str"]],
    "InternetSetOptionA" => ["long",  ["long", "long", "ptr", "long"]],
    "InternetSetOptionW" => ["long",  ["long", "long", "ptr", "long"]],
    "InternetSetStatusCallback" => ["ptr",  ["long", "ptr"]],
    "InternetWriteFile" => ["long",  ["long", "ptr", "long", "ptr"]]
};

&wine::declare("wininet",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
