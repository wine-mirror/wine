package wineoss_drv;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "DriverProc" => ["long",  ["long", "long", "long", "long", "long"]],
    "auxMessage" => ["long",  ["long", "long", "long", "long", "long"]],
    "mixMessage" => ["long",  ["long", "long", "long", "long", "long"]],
    "midMessage" => ["long",  ["long", "long", "long", "long", "long"]],
    "modMessage" => ["long",  ["long", "long", "long", "long", "long"]],
    "widMessage" => ["long",  ["long", "long", "long", "long", "long"]],
    "wodMessage" => ["long",  ["long", "long", "long", "long", "long"]]
};

&wine::declare("wineoss.drv",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
