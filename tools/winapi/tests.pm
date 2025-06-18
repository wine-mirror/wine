#
# Copyright 2002 Patrik Stridvall
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

package tests;

use strict;
use warnings 'all';

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw($tests);

use vars qw($tests);

use config qw($current_dir $wine_dir $winapi_dir);
use options qw($options);
use output qw($output);

sub import(@) {
    $Exporter::ExportLevel++;
    Exporter::import(@_);
    $Exporter::ExportLevel--;

    $tests = 'tests'->new;
}

sub parse_tests_file($);

sub new($) {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    $self->parse_tests_file();

    return $self;
}

sub parse_tests_file($) {
    my $self = shift;

    my $file = "tests.dat";

    my $tests = \%{$self->{TESTS}};

    $output->lazy_progress($file);

    my $test_dir;
    my $test;
    my $section;

    open(IN, "< $winapi_dir/$file") || die "$winapi_dir/$file: $!\n";
    while(<IN>) {
	s/^\s*?(.*?)\s*$/$1/; # remove whitespace at beginning and end of line
	s/^(.*?)\s*#.*$/$1/;  # remove comments
	/^$/ && next;         # skip empty lines

	if (/^%%%\s*(\S+)$/) {
	    $test_dir = $1;
	} elsif (/^%%\s*(\w+)$/) {
	    $test = $1;
	} elsif (/^%\s*(\w+)$/) {
	    $section = $1;
	} elsif (!/^%/) {
	    if (!exists($$tests{$test_dir}{$test}{$section})) {
		$$tests{$test_dir}{$test}{$section} = [];
	    }
	    push @{$$tests{$test_dir}{$test}{$section}}, $_;
	} else {
	    $output->write("$file:$.: parse error: '$_'\n");
	    exit 1;
	}
    }
    close(IN);
}

sub get_tests($$) {
    my $self = shift;

    my $tests = \%{$self->{TESTS}};

    my $test_dir = shift;

    my %tests = ();
    if (defined($test_dir)) {
	foreach my $test (sort(keys(%{$$tests{$test_dir}}))) {
	    $tests{$test}++;
	}
    } else {
	foreach my $test_dir (sort(keys(%$tests))) {
	    foreach my $test (sort(keys(%{$$tests{$test_dir}}))) {
		$tests{$test}++;
	    }
	}
    }
    return sort(keys(%tests));
}

sub get_test_dirs($$) {
    my $self = shift;

    my $tests = \%{$self->{TESTS}};

    my $test = shift;

    my %test_dirs = ();    
    if (defined($test)) {
	foreach my $test_dir (sort(keys(%$tests))) {
	    if (exists($$tests{$test_dir}{$test})) {
		$test_dirs{$test_dir}++;
	    }
	}
    } else {
	foreach my $test_dir (sort(keys(%$tests))) {
	    $test_dirs{$test_dir}++;
	}
    }

    return sort(keys(%test_dirs));
}

sub get_sections($$$) {
    my $self = shift;

    my $tests = \%{$self->{TESTS}};

    my $test_dir = shift;
    my $test = shift;

    my %sections = ();   
    if (defined($test_dir)) { 
	if (defined($test)) {
	    foreach my $section (sort(keys(%{$$tests{$test_dir}{$test}}))) {
		$sections{$section}++;
	    }
	} else {
	    foreach my $test (sort(keys(%{$$tests{$test_dir}}))) {
		foreach my $section (sort(keys(%{$$tests{$test_dir}{$test}}))) {
		    $sections{$section}++;
		}
	    }
	}
    } elsif (defined($test)) {
	foreach my $test_dir (sort(keys(%$tests))) {
	    foreach my $section (sort(keys(%{$$tests{$test_dir}{$test}}))) {
		$sections{$section}++;
	    }
	}
    } else {
	foreach my $test_dir (sort(keys(%$tests))) {
	    foreach my $test (sort(keys(%{$$tests{$test_dir}}))) {
		foreach my $section (sort(keys(%{$$tests{$test_dir}{$test}}))) {
		    $sections{$section}++;
		}
	    }
	}
    }

    return sort(keys(%sections));
}

sub get_section($$$$) {
    my $self = shift;

    my $tests = \%{$self->{TESTS}};

    my $test_dir = shift;
    my $test = shift;
    my $section = shift;

    my $array = $$tests{$test_dir}{$test}{$section};
    if (defined($array)) {
	return @$array;
    } else {
	return ();
    }
}

1;
