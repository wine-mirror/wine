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

package nativeapi;

use strict;
use warnings 'all';

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw($nativeapi);

use vars qw($nativeapi);

use config qw(file_type $current_dir $wine_dir $winapi_dir);
use options qw($options);
use output qw($output);

$nativeapi = 'nativeapi'->new;

sub new($) {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $functions = \%{$self->{FUNCTIONS}};
    my $conditionals = \%{$self->{CONDITIONALS}};
    my $conditional_headers = \%{$self->{CONDITIONAL_HEADERS}};
    my $conditional_functions = \%{$self->{CONDITIONAL_FUNCTIONS}};

    my $api_file = "$winapi_dir/nativeapi.dat";
    my $configure_ac_file = "$wine_dir/configure.ac";
    my $config_h_in_file = "$wine_dir/include/config.h.in";

    $api_file =~ s/^\.\///;
    $configure_ac_file =~ s/^\.\///;
    $config_h_in_file =~ s/^\.\///;


    $$conditional_headers{"config.h"}++;

    $output->progress("$api_file");

    open(IN, "< $api_file") || die "Error: Can't open $api_file: $!\n";
    local $/ = "\n";
    while(<IN>) {
	s/^\s*(.*?)\s*$/$1/; # remove whitespace at begin and end of line
	s/^(.*?)\s*#.*$/$1/; # remove comments
	/^$/ && next;        # skip empty lines

	$$functions{$_}++;
    }
    close(IN);

    $output->progress("$configure_ac_file");

    my $again = 0;
    open(IN, "< $configure_ac_file") || die "Error: Can't open $configure_ac_file: $!\n";
    local $/ = "\n";
    while($again || (defined($_ = <IN>))) {
	$again = 0;
	chomp;
	if(/^(.*?)\\$/) {
	    my $current = $1;
	    my $next = <IN>;
	    if(defined($next)) {
		# remove trailing whitespace
		$current =~ s/\s+$//;

		# remove leading whitespace
		$next =~ s/^\s+//;

		$_ = $current . " " . $next;

		$again = 1;
		next;
	    }
	}

	# remove leading and trailing whitespace
	s/^\s*(.*?)\s*$/$1/;

	# skip empty lines
	if(/^$/) { next; }

	# skip comments
	if(/^dnl/) { next; }

	if(/AC_CHECK_HEADERS\(\s*([^,\)]*)(?:,|\))?/) {
	    my $headers = $1;
	    $headers =~ s/^\s*\[\s*(.*?)\s*\]\s*$/$1/;
	    foreach my $name (split(/\s+/, $headers)) {
		$$conditional_headers{$name}++;
	    }
	} elsif(/AC_HEADER_STAT\(\)/) {
            # This checks for a bunch of standard headers
            # There's stdlib.h, string.h and sys/types.h too but we don't
            # want to force ifdefs for those at this point.
	    foreach my $name ("sys/stat.h", "memory.h", "strings.h",
                              "inttypes.h", "stdint.h", "unistd.h") {
		$$conditional_headers{$name}++;
	    }
	} elsif(/AC_CHECK_FUNCS\(\s*([^,\)]*)(?:,|\))?/) {
	    my $funcs = $1;
	    $funcs =~ s/^\s*\[\s*(.*?)\s*\]\s*$/$1/;
	    foreach my $name (split(/\s+/, $funcs)) {
		$$conditional_functions{$name}++;
	    }
	} elsif(/AC_FUNC_ALLOCA/) {
	    $$conditional_headers{"alloca.h"}++;
        } elsif (/AC_DEFINE\(\s*HAVE_(.*?)_H/) {
	    my $name = lc($1);
	    $name =~ s/_/\//;
	    $name .= ".h";

	    next if $name =~ m%correct/%;

	    $$conditional_headers{$name}++;
	}

    }
    close(IN);

    $output->progress("$config_h_in_file");

    open(IN, "< $config_h_in_file") || die "Error: Can't open $config_h_in_file: $!\n";
    local $/ = "\n";
    while(<IN>) {
	# remove leading and trailing whitespace
	s/^\s*(.*?)\s*$/$1/;

	# skip empty lines
	if(/^$/) { next; }

	if(/^\#undef\s+(\S+)$/) {
	    $$conditionals{$1}++;
	}
    }
    close(IN);

    $nativeapi = $self;

    return $self;
}

sub is_function($$) {
    my $self = shift;
    my $functions = \%{$self->{FUNCTIONS}};

    my $name = shift;

    return ($$functions{$name} || 0);
}

sub is_conditional($$) {
    my $self = shift;
    my $conditionals = \%{$self->{CONDITIONALS}};

    my $name = shift;

    return ($$conditionals{$name} || 0);
}

sub found_conditional($$) {
    my $self = shift;
    my $conditional_found = \%{$self->{CONDITIONAL_FOUND}};

    my $name = shift;

    $$conditional_found{$name}++;
}

sub is_conditional_header($$) {
    my $self = shift;
    my $conditional_headers = \%{$self->{CONDITIONAL_HEADERS}};

    my $name = shift;

    return ($$conditional_headers{$name} || 0);
}

sub is_conditional_function($$) {
    my $self = shift;
    my $conditional_functions = \%{$self->{CONDITIONAL_FUNCTIONS}};

    my $name = shift;

    return ($$conditional_functions{$name} || 0);
}

sub global_report($) {
    my $self = shift;

    my $output = \${$self->{OUTPUT}};
    my $conditional_found = \%{$self->{CONDITIONAL_FOUND}};
    my $conditionals = \%{$self->{CONDITIONALS}};

    my @messages;
    foreach my $name (sort(keys(%$conditionals))) {
	if($name =~ /^(?:const|inline|size_t)$/) { next; }

	if(0 && !$$conditional_found{$name}) {
	    push @messages, "config.h.in: conditional $name not used\n";
	}
    }

    foreach my $message (sort(@messages)) {
	$output->write($message);
    }
}

1;
