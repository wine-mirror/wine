package msacm_drv;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "DriverProc" => ["long",  ["long", "long", "long", "long", "long"]],
    "widMessage" => ["long",  ["long", "long", "long", "long", "long"]],
    "wodMessage" => ["long",  ["long", "long", "long", "long", "long"]]
};

&wine::declare("msacm.drv",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
