#
# Copyright 1999, 2000, 2001 Patrik Stridvall
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
#

package winapi_extract_options;

use strict;
use warnings 'all';

use base qw(options);

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw($options);

use options qw($options);

my %options_long = (
    "debug" => { default => 0, description => "debug mode" },
    "help" => { default => 0, description => "help mode" },
    "verbose" => { default => 0, description => "verbose mode" },

    "progress" => { default => 1, description => "show progress" },

    "win16" => { default => 1, description => "Win16 extraction" },
    "win32" => { default => 1, description => "Win32 extraction" },

    "headers" => { default => 0, description => "parse the .h files as well" },

    "implemented" => { default => 0, description => "implemented functions extraction" },
    "pseudo-implemented" => { default => 0, parent => "implemented", description => "pseudo implemented functions extraction" },
    "struct" => { default => 0, parent => "headers", description => "struct extraction" },
    "stub-statistics" => { default => 0, description => "stub statistics" },
    "pseudo-stub-statistics" => { default => 0, parent => "stub-statistics", description => "pseudo stub statistics" },
    "winetest" => { default => 0, description => "winetest extraction" },
);

my %options_short = (
    "d" => "debug",
    "?" => "help",
    "v" => "verbose"
);

my $options_usage = "usage: winapi_extract [--help] [<files>]\n";

$options = '_options'->new(\%options_long, \%options_short, $options_usage);

1;
