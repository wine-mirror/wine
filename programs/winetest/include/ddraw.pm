package ddraw;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "DirectDrawCreate" => ["long",  ["ptr", "ptr", "ptr"]],
    "DirectDrawEnumerateA" => ["long",  ["ptr", "ptr"]],
    "DirectDrawEnumerateExA" => ["long",  ["ptr", "ptr", "long"]],
    "DirectDrawEnumerateExW" => ["long",  ["ptr", "ptr", "long"]],
    "DirectDrawEnumerateW" => ["long",  ["ptr", "ptr"]],
    "DllCanUnloadNow" => ["long",  []],
    "DllGetClassObject" => ["long",  ["ptr", "ptr", "ptr"]]
};

&wine::declare("ddraw",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
