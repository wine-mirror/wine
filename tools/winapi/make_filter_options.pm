package make_filter_options;
use base qw(options);

use strict;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw($options);

use options qw($options &parse_comma_list &parse_value);

my %options_long = (
    "debug" => { default => 0, description => "debug mode" },
    "help" => { default => 0, description => "help mode" },
    "verbose" => { default => 0, description => "verbose mode" },

    "progress" => { default => 1, description => "show progress" },

    "make" => { default => "make",
		parser => \&parse_value,
		description => "use which make" },
    "pedantic" => { default => 0, description => "be pedantic" },
);

my %options_short = (
    "d" => "debug",
    "?" => "help",
    "v" => "verbose"
);

my $options_usage = "usage: make_filter [--help]\n";

$options = '_options'->new(\%options_long, \%options_short, $options_usage);

1;
