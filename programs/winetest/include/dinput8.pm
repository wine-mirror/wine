package dinput8;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "DirectInput8Create" => ["long",  ["long", "long", "ptr", "ptr", "ptr"]],
    "DllCanUnloadNow" => ["long",  []],
    "DllGetClassObject" => ["long",  ["ptr", "ptr", "ptr"]],
    "DllRegisterServer" => ["long",  []],
    "DllUnregisterServer" => ["long",  []]
};

&wine::declare("dinput8",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
