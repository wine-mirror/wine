package w32skrnl;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "GetCurrentTask32" => ["long",  []],
    "_kGetWin32sDirectory\@0" => ["str",  []]
};

&wine::declare("w32skrnl",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
