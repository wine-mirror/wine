package olecli32;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "OleQueryLinkFromClip" => ["long",  ["str", "long", "long"]],
    "OleQueryCreateFromClip" => ["long",  ["str", "long", "long"]],
    "OleCreateLinkFromClip" => ["long",  ["str", "ptr", "long", "str", "ptr", "long", "long"]],
    "OleCreateFromClip" => ["long",  ["str", "ptr", "long", "str", "ptr", "long", "long"]],
    "OleQueryType" => ["long",  ["ptr", "ptr"]],
    "OleSetHostNames" => ["long",  ["ptr", "str", "str"]],
    "OleRegisterClientDoc" => ["long",  ["str", "str", "long", "ptr"]],
    "OleRevokeClientDoc" => ["long",  ["long"]],
    "OleRenameClientDoc" => ["long",  ["long", "str"]],
    "OleSavedClientDoc" => ["long",  ["long"]],
    "OleIsDcMeta" => ["long",  ["long"]]
};

&wine::declare("olecli32",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
