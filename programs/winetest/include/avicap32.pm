package avicap32;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "capCreateCaptureWindowA" => ["long",  ["str", "long", "long", "long", "long", "long", "long", "long"]],
    "capCreateCaptureWindowW" => ["long",  ["wstr", "long", "long", "long", "long", "long", "long", "long"]],
    "capGetDriverDescriptionA" => ["long",  ["long", "str", "long", "str", "long"]],
    "capGetDriverDescriptionW" => ["long",  ["long", "wstr", "long", "wstr", "long"]]
};

&wine::declare("avicap32",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
