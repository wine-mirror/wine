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

package winapi_check_options;

use strict;
use warnings 'all';

use base qw(options);

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw($options);

use config qw($current_dir $wine_dir);
use options qw($options parse_comma_list);

my %options_long = (
    "debug" => { default => 0, description => "debug mode" },
    "help" => { default => 0, description => "help mode" },
    "verbose" => { default => 0, description => "verbose mode" },

    "progress" => { default => 1, description => "show progress" },

    "win16" => { default => 1, description => "Win16 checking" },
    "win32" => { default => 1, description => "Win32 checking" },

    "shared" =>  { default => 0, description => "show shared functions between Win16 and Win32" },
    "shared-segmented" =>  { default => 0, description => "segmented shared functions between Win16 and Win32 checking" },

    "config" => { default => 1, parent => "local", description => "check configuration include consistency" },
    "config-unnecessary" => { default => 0, parent => "config", description => "check for unnecessary #include \"config.h\"" },

    "spec-mismatch" => { default => 0, description => "spec file mismatch checking" },

    "local" =>  { default => 1, description => "local checking" },
    "module" => {
	default => { active => 1, filter => 0, hash => {} },
	parent => "local",
	parser => \&parse_comma_list,
	description => "module filter"
    },

    "argument" => { default => 1, parent => "local", description => "argument checking" },
    "argument-count" => { default => 1, parent => "argument", description => "argument count checking" },
    "argument-forbidden" => {
	default => { active => 1, filter => 0, hash => {} },
	parent => "argument",
	parser => \&parse_comma_list,
	description => "argument forbidden checking"
    },
    "argument-kind" => {
	default => { active => 1, filter => 1, hash => { double => 1 } },
	parent => "argument",
	parser => \&parse_comma_list,
	description => "argument kind checking"
    },
    "calling-convention" => { default => 1, parent => "local", description => "calling convention checking" },
    "calling-convention-win16" => { default => 0, parent => "calling-convention", description => "calling convention checking (Win16)" },
    "calling-convention-win32" => { default => 1, parent => "calling-convention", description => "calling convention checking (Win32)" },
    "misplaced" => { default => 1, parent => "local", description => "check for misplaced functions" },
    "statements"  => { default => 0, parent => "local", description => "check for statements inconsistencies" },
    "cross-call" => { default => 0, parent => ["statements", "win16", "win32"],  description => "check for cross calling functions" },
    "cross-call-win32-win16" => {
	default => 0, parent => "cross-call", description => "check for cross calls between win32 and win16"
     },
    "cross-call-unicode-ascii" => {
	default => 0, parent => "cross-call", description => "check for cross calls between Unicode and ASCII"
    },
    "debug-messages" => { default => 0, parent => "statements", description => "check for debug messages inconsistencies" },

    "comments" => {
	default => 1,
	parent => "local",
	description => "comments checking"
	},
    "comments-cplusplus" => {
	default => 1,
	parent => "comments",
	description => "C++ comments checking"
	},

    "documentation" => {
	default => 1,
	parent => "local",
	description => "check for documentation inconsistencies"
	},
    "documentation-pedantic" => {
	default => 0,
	parent => "documentation",
	description => "be pedantic when checking for documentation inconsistencies"
	},

    "documentation-arguments" => {
	default => 1,
	parent => "documentation",
	description => "check for arguments documentation inconsistencies\n"
	},
    "documentation-comment-indent" => {
	default => 0,
	parent => "documentation", description => "check for documentation comment indent inconsistencies"
	},
    "documentation-comment-width" => {
	default => 0,
	parent => "documentation", description => "check for documentation comment width inconsistencies"
	},
    "documentation-name" => {
	default => 1,
	parent => "documentation",
	description => "check for documentation name inconsistencies\n"
	},
    "documentation-ordinal" => {
	default => 1,
	parent => "documentation",
	description => "check for documentation ordinal inconsistencies\n"
	},
    "documentation-wrong" => {
	default => 1,
	parent => "documentation",
	description => "check for wrong documentation\n"
	},

    "prototype" => {default => 0, parent => ["local", "headers"], description => "prototype checking" },
    "global" => { default => 1, description => "global checking" },
    "declared" => { default => 1, parent => "global", description => "declared checking" },
    "implemented" => { default => 0, parent => "local", description => "implemented checking" },
    "implemented-win32" => { default => 0, parent => "implemented", description => "implemented as win32 checking" },
    "include" => { default => 1, parent => "global", description => "include checking" },

    "headers" => { default => 0, description => "headers checking" },
    "headers-duplicated" => { default => 0, parent => "headers", description => "duplicated function declarations checking" },
    "headers-misplaced" => { default => 0, parent => "headers", description => "misplaced function declarations checking" },
    "headers-needed" => { default => 1, parent => "headers", description => "headers needed checking" },
    "headers-unused" => { default => 0, parent => "headers", description => "headers unused checking" },
);

my %options_short = (
    "d" => "debug",
    "?" => "help",
    "v" => "verbose"
);

my $options_usage = "usage: winapi_check [--help] [<files>]\n";

$options = '_winapi_check_options'->new(\%options_long, \%options_short, $options_usage);

package _winapi_check_options;

use strict;
use warnings 'all';

use base qw(_options);

sub report_module($$) {
    my $self = shift;
    my $refvalue = $self->{MODULE};

    my $name = shift;

    if(defined($name)) {
	return $$refvalue->{active} && (!$$refvalue->{filter} || $$refvalue->{hash}->{$name});
    } else {
	return 0;
    }
}

sub report_argument_forbidden($$) {
    my $self = shift;
    my $refargument_forbidden = $self->{ARGUMENT_FORBIDDEN};

    my $type = shift;

    return $$refargument_forbidden->{active} && (!$$refargument_forbidden->{filter} || $$refargument_forbidden->{hash}->{$type});
}

sub report_argument_kind($$) {
    my $self = shift;
    my $refargument_kind = $self->{ARGUMENT_KIND};

    my $kind = shift;

    return $$refargument_kind->{active} && (!$$refargument_kind->{filter} || $$refargument_kind->{hash}->{$kind});

}

1;
