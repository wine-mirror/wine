package mpr;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "MultinetGetConnectionPerformanceA" => ["long",  ["ptr", "ptr"]],
    "MultinetGetConnectionPerformanceW" => ["long",  ["ptr", "ptr"]],
    "MultinetGetErrorTextA" => ["long",  ["long", "long", "long"]],
    "MultinetGetErrorTextW" => ["long",  ["long", "long", "long"]],
    "NPSAuthenticationDialogA" => ["long",  ["ptr"]],
    "NPSCopyStringA" => ["long",  ["str", "ptr", "ptr"]],
    "NPSDeviceGetNumberA" => ["long",  ["str", "ptr", "ptr"]],
    "NPSDeviceGetStringA" => ["long",  ["long", "long", "str", "ptr"]],
    "NPSGetProviderHandleA" => ["long",  ["ptr"]],
    "NPSGetProviderNameA" => ["long",  ["long", "ptr"]],
    "NPSGetSectionNameA" => ["long",  ["long", "ptr"]],
    "NPSNotifyGetContextA" => ["ptr",  ["ptr"]],
    "NPSNotifyRegisterA" => ["long",  ["long", "ptr"]],
    "NPSSetCustomTextA" => ["void",  ["str"]],
    "NPSSetExtendedErrorA" => ["long",  ["long", "str"]],
    "WNetAddConnection2A" => ["long",  ["ptr", "str", "str", "long"]],
    "WNetAddConnection2W" => ["long",  ["ptr", "wstr", "wstr", "long"]],
    "WNetAddConnection3A" => ["long",  ["long", "ptr", "str", "str", "long"]],
    "WNetAddConnection3W" => ["long",  ["long", "ptr", "wstr", "wstr", "long"]],
    "WNetAddConnectionA" => ["long",  ["str", "str", "str"]],
    "WNetAddConnectionW" => ["long",  ["wstr", "wstr", "wstr"]],
    "WNetCachePassword" => ["long",  ["str", "long", "str", "long", "long", "long"]],
    "WNetCancelConnection2A" => ["long",  ["str", "long", "long"]],
    "WNetCancelConnection2W" => ["long",  ["wstr", "long", "long"]],
    "WNetCancelConnectionA" => ["long",  ["str", "long"]],
    "WNetCancelConnectionW" => ["long",  ["wstr", "long"]],
    "WNetCloseEnum" => ["long",  ["long"]],
    "WNetConnectionDialog" => ["long",  ["long", "long"]],
    "WNetConnectionDialog1A" => ["long",  ["ptr"]],
    "WNetConnectionDialog1W" => ["long",  ["ptr"]],
    "WNetDisconnectDialog" => ["long",  ["long", "long"]],
    "WNetDisconnectDialog1A" => ["long",  ["ptr"]],
    "WNetDisconnectDialog1W" => ["long",  ["ptr"]],
    "WNetEnumCachedPasswords" => ["long",  ["str", "long", "long", "ptr", "long"]],
    "WNetEnumResourceA" => ["long",  ["long", "ptr", "ptr", "ptr"]],
    "WNetEnumResourceW" => ["long",  ["long", "ptr", "ptr", "ptr"]],
    "WNetGetCachedPassword" => ["long",  ["str", "long", "str", "ptr", "long"]],
    "WNetGetConnectionA" => ["long",  ["str", "str", "ptr"]],
    "WNetGetConnectionW" => ["long",  ["wstr", "wstr", "ptr"]],
    "WNetGetLastErrorA" => ["long",  ["ptr", "str", "long", "str", "long"]],
    "WNetGetLastErrorW" => ["long",  ["ptr", "wstr", "long", "wstr", "long"]],
    "WNetGetNetworkInformationA" => ["long",  ["str", "ptr"]],
    "WNetGetNetworkInformationW" => ["long",  ["wstr", "ptr"]],
    "WNetGetProviderNameA" => ["long",  ["long", "str", "ptr"]],
    "WNetGetProviderNameW" => ["long",  ["long", "wstr", "ptr"]],
    "WNetGetResourceInformationA" => ["long",  ["ptr", "ptr", "ptr", "ptr"]],
    "WNetGetResourceInformationW" => ["long",  ["ptr", "ptr", "ptr", "ptr"]],
    "WNetGetResourceParentA" => ["long",  ["ptr", "ptr", "ptr"]],
    "WNetGetResourceParentW" => ["long",  ["ptr", "ptr", "ptr"]],
    "WNetGetUniversalNameA" => ["long",  ["str", "long", "ptr", "ptr"]],
    "WNetGetUniversalNameW" => ["long",  ["wstr", "long", "ptr", "ptr"]],
    "WNetGetUserA" => ["long",  ["str", "str", "ptr"]],
    "WNetGetUserW" => ["long",  ["wstr", "wstr", "ptr"]],
    "WNetLogoffA" => ["long",  ["str", "long"]],
    "WNetLogoffW" => ["long",  ["wstr", "long"]],
    "WNetLogonA" => ["long",  ["str", "long"]],
    "WNetLogonW" => ["long",  ["wstr", "long"]],
    "WNetOpenEnumA" => ["long",  ["long", "long", "long", "ptr", "ptr"]],
    "WNetOpenEnumW" => ["long",  ["long", "long", "long", "ptr", "ptr"]],
    "WNetRemoveCachedPassword" => ["long",  ["str", "long", "long"]],
    "WNetRestoreConnectionA" => ["long",  ["long", "str"]],
    "WNetRestoreConnectionW" => ["long",  ["long", "wstr"]],
    "WNetSetConnectionA" => ["long",  ["str", "long", "ptr"]],
    "WNetSetConnectionW" => ["long",  ["wstr", "long", "ptr"]],
    "WNetUseConnectionA" => ["long",  ["long", "ptr", "str", "str", "long", "str", "ptr", "ptr"]],
    "WNetUseConnectionW" => ["long",  ["long", "ptr", "wstr", "wstr", "long", "wstr", "ptr", "ptr"]],
    "WNetVerifyPasswordA" => ["long",  ["str", "ptr"]],
    "WNetVerifyPasswordW" => ["long",  ["wstr", "ptr"]]
};

&wine::declare("mpr",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
