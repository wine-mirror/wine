package urlmon;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "CoInternetGetSession" => ["long",  ["long", "ptr", "long"]],
    "CreateAsyncBindCtxEx" => ["long",  ["ptr", "long", "ptr", "ptr", "ptr", "long"]],
    "CreateURLMoniker" => ["long",  ["ptr", "wstr", "ptr"]],
    "DllCanUnloadNow" => ["long",  []],
    "DllGetClassObject" => ["long",  ["ptr", "ptr", "ptr"]],
    "DllInstall" => ["long",  ["long", "wstr"]],
    "DllRegisterServer" => ["long",  []],
    "DllRegisterServerEx" => ["long",  []],
    "DllUnregisterServer" => ["long",  []],
    "Extract" => ["long",  ["long", "str"]],
    "ObtainUserAgentString" => ["long",  ["long", "str", "ptr"]],
    "RegisterBindStatusCallback" => ["long",  ["ptr", "ptr", "ptr", "long"]],
    "RevokeBindStatusCallback" => ["long",  ["ptr", "ptr"]],
    "UrlMkSetSessionOption" => ["long",  ["long", "ptr", "long", "long"]]
};

&wine::declare("urlmon",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
