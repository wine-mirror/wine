package msimg32;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "AlphaBlend" => ["long",  ["long", "long", "long", "long", "long", "long", "long", "long", "long", "long", "ptr"]],
    "GradientFill" => ["long",  ["long", "ptr", "long", "ptr", "long", "long"]],
    "TransparentBlt" => ["long",  ["long", "long", "long", "long", "long", "long", "long", "long", "long", "long", "long"]],
    "vSetDdrawflag" => ["void",  []]
};

&wine::declare("msimg32",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
