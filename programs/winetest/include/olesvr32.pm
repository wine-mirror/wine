package olesvr32;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "OleRegisterServer" => ["long",  ["str", "ptr", "ptr", "long", "long"]],
    "OleBlockServer" => ["long",  ["long"]],
    "OleUnblockServer" => ["long",  ["long", "ptr"]],
    "OleRegisterServerDoc" => ["long",  ["long", "str", "ptr", "ptr"]],
    "OleRevokeServerDoc" => ["long",  ["long"]],
    "OleRenameServerDoc" => ["long",  ["long", "str"]]
};

&wine::declare("olesvr32",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
