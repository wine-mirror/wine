package msisys_ocx;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "DllCanUnloadNow" => ["long",  []],
    "DllGetClassObject" => ["long",  ["ptr", "ptr", "ptr"]],
    "DllRegisterServer" => ["long",  []],
    "DllUnregisterServer" => ["long",  []]
};

&wine::declare("msisys.ocx",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
