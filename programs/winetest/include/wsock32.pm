package wsock32;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "inet_network" => ["long",  ["ptr"]],
    "getnetbyname" => ["ptr",  ["ptr"]],
    "WSARecvEx" => ["long",  ["long", "ptr", "long", "ptr"]],
    "s_perror" => ["void",  ["str"]],
    "EnumProtocolsA" => ["long",  ["ptr", "ptr", "ptr"]],
    "EnumProtocolsW" => ["long",  ["ptr", "ptr", "ptr"]]
};

&wine::declare("wsock32",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
