package winedos;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "ASPIHandler" => ["void",  ["ptr"]],
    "AllocRMCB" => ["void",  ["ptr"]],
    "CallRMInt" => ["void",  ["ptr"]],
    "CallRMProc" => ["void",  ["undef"]],
    "Enter" => ["long",  ["ptr"]],
    "FreeRMCB" => ["void",  ["ptr"]],
    "GetTimer" => ["long",  []],
    "KbdReadScan" => ["long",  ["ptr"]],
    "LoadDosExe" => ["void",  ["str", "long"]],
    "OutPIC" => ["void",  ["long", "long"]],
    "QueueEvent" => ["void",  ["long", "long", "undef", "ptr"]],
    "SetTimer" => ["void",  ["long"]],
    "Wait" => ["void",  ["long", "long"]]
};

&wine::declare("winedos",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
