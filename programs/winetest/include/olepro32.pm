package olepro32;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "DllCanUnloadNow" => ["long",  ["undef"]],
    "DllGetClassObject" => ["long",  ["ptr", "ptr", "ptr"]],
    "DllRegisterServer" => ["long",  ["undef"]],
    "DllUnregisterServer" => ["long",  ["undef"]]
};

&wine::declare("olepro32",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
