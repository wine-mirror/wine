package serialui;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "EnumPropPages" => ["long",  ["ptr", "ptr", "long"]],
    "drvCommConfigDialog" => ["long",  ["str", "long", "ptr"]],
    "drvSetDefaultCommConfig" => ["long",  ["str", "ptr", "long"]],
    "drvGetDefaultCommConfig" => ["long",  ["str", "ptr", "ptr"]]
};

&wine::declare("serialui",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
