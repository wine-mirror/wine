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

package util;

use strict;
use warnings 'all';

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw(
    append_file edit_file read_file replace_file
    normalize_set is_subset
);
@EXPORT_OK = qw();
%EXPORT_TAGS = ();

########################################################################
# _compare_files

sub _compare_files($$) {
    my $file1 = shift;
    my $file2 = shift;

    local $/ = undef;

    return -1 if !open(IN, "< $file1");
    my $s1 = <IN>;
    close(IN);

    return 1 if !open(IN, "< $file2");
    my $s2 = <IN>;
    close(IN);

    return $s1 cmp $s2;
}

########################################################################
# append_file

sub append_file($$@) {
    my $filename = shift;
    my $function = shift;

    open(OUT, ">> $filename") || die "Can't open file '$filename'";
    my $result = &$function(\*OUT, @_);
    close(OUT);

    return $result;
}

########################################################################
# edit_file

sub edit_file($$@) {
    my $filename = shift;
    my $function = shift;

    open(IN, "< $filename") || die "Can't open file '$filename'";
    open(OUT, "> $filename.tmp") || die "Can't open file '$filename.tmp'";

    my $result = &$function(\*IN, \*OUT, @_);

    close(IN);
    close(OUT);

    if($result) {
        unlink("$filename");
        rename("$filename.tmp", "$filename");
    } else {
        unlink("$filename.tmp");
    }

    return $result;
}

########################################################################
# read_file

sub read_file($$@) {
    my $filename = shift;
    my $function = shift;

    open(IN, "< $filename") || die "Can't open file '$filename'";
    my $result = &$function(\*IN, @_);
    close(IN);

    return $result;
}

########################################################################
# replace_file

sub replace_file($$@) {
    my $filename = shift;
    my $function = shift;

    open(OUT, "> $filename.tmp") || die "Can't open file '$filename.tmp'";

    my $result = &$function(\*OUT, @_);

    close(OUT);

    if($result && _compare_files($filename, "$filename.tmp")) {
        unlink("$filename");
        rename("$filename.tmp", $filename);
    } else {
        unlink("$filename.tmp");
    }

    return $result;
}

########################################################################
# normalize_set

sub normalize_set($) {
    local $_ = shift;

    if(!defined($_)) {
	return undef;
    }

    my %hash = ();
    foreach my $key (split(/\s*&\s*/)) {
	$hash{$key}++;
    }

    return join(" & ", sort(keys(%hash)));
}

########################################################################
# is_subset

sub is_subset($$) {
    my $subset = shift;
    my $set = shift;

    foreach my $subitem (split(/ & /, $subset)) {
	my $match = 0;
	foreach my $item (split(/ & /, $set)) {
	    if($subitem eq $item) {
		$match = 1;
		last;
	    }
	}
	if(!$match) {
	    return 0;
	}
    }
    return 1;
}

1;
