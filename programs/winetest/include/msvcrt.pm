package msvcrt;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "\?\?8type_info\@\@QBEHABV0\@\@Z" => ["long",  ["ptr", "ptr"]],
    "\?\?9type_info\@\@QBEHABV0\@\@Z" => ["long",  ["ptr", "ptr"]],
    "\?name\@type_info\@\@QBEPBDXZ" => ["ptr",  ["ptr"]],
    "\?raw_name\@type_info\@\@QBEPBDXZ" => ["ptr",  ["ptr"]]
};

&wine::declare("msvcrt",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
