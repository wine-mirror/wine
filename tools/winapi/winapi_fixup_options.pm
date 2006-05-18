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

package winapi_fixup_options;
use base qw(options);

use strict;

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

    "win16" => { default => 1, description => "Win16 fixup" },
    "win32" => { default => 1, description => "Win32 fixup" },

    "local" =>  { default => 1, description => "local fixup" },
    "documentation" => { default => 1, parent => "local", description => "documentation fixup" },
    "documentation-missing" => { default => 1, parent => "documentation", description => "documentation missing fixup" },
    "documentation-name" => { default => 1, parent => "documentation", description => "documentation name fixup" },
    "documentation-ordinal" => { default => 1, parent => "documentation", description => "documentation ordinal fixup" },
    "documentation-wrong" => { default => 1, parent => "documentation", description => "documentation wrong fixup" },
    "statements" => { default => 1, parent => "local", description => "statements fixup" },
    "statements-windowsx" => { default => 0, parent => "local", description => "statements windowsx fixup" },
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
