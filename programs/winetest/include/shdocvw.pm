package shdocvw;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "DllCanUnloadNow" => ["long",  []],
    "DllGetVersion" => ["long",  ["ptr"]],
    "DllInstall" => ["long",  ["long", "wstr"]],
    "DllRegisterServer" => ["long",  ["undef"]],
    "DllUnregisterServer" => ["long",  ["undef"]],
    "DllGetClassObject" => ["long",  ["ptr", "ptr", "ptr"]]
};

&wine::declare("shdocvw",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
