package winapi_fixup_options;
use base qw(options);

use strict;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw($options);

use options qw($options &parse_comma_list);

my %options_long = (
    "debug" => { default => 0, description => "debug mode" },
    "help" => { default => 0, description => "help mode" },
    "verbose" => { default => 0, description => "verbose mode" },

    "progress" => { default => 1, description => "show progress" },

    "win16" => { default => 1, description => "Win16 fixup" },
    "win32" => { default => 1, description => "Win32 fixup" },

    "local" =>  { default => 1, description => "local fixup" },
    "documentation" => { default => 1, parent => "local", description => "documentation fixup" },
    "documentation-missing" => { default => 1, parent => "documentation", description => "documentation missing fixup" },
    "documentation-name" => { default => 1, parent => "documentation", description => "documentation name fixup" },
    "documentation-ordinal" => { default => 1, parent => "documentation", description => "documentation ordinal fixup" },
    "documentation-wrong" => { default => 1, parent => "documentation", description => "documentation wrong fixup" },
    "stub" => { default => 0, parent => "local", description => "stub fixup" },

    "global" => { default => 1, description => "global fixup" },

    "modify" => { default => 0, description => "actually perform the fixups" },	    
);

my %options_short = (
    "d" => "debug",
    "?" => "help",
    "v" => "verbose"
);

my $options_usage = "usage: winapi_fixup [--help] [<files>]\n";

$options = '_options'->new(\%options_long, \%options_short, $options_usage);

1;
