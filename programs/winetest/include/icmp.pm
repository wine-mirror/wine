package icmp;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "IcmpCloseHandle" => ["long",  ["long"]],
    "IcmpCreateFile" => ["long",  []],
    "IcmpSendEcho" => ["long",  ["long", "long", "ptr", "long", "ptr", "ptr", "long", "long"]]
};

&wine::declare("icmp",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
