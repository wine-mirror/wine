package quartz;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "AMGetErrorTextA" => ["long",  ["long", "str", "long"]],
    "AMGetErrorTextW" => ["long",  ["long", "wstr", "long"]],
    "AmpFactorToDB" => ["long",  ["long"]],
    "DBToAmpFactor" => ["long",  ["long"]],
    "DllCanUnloadNow" => ["long",  []],
    "DllGetClassObject" => ["long",  ["ptr", "ptr", "ptr"]],
    "DllRegisterServer" => ["long",  []],
    "DllUnregisterServer" => ["long",  []]
};

&wine::declare("quartz",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
