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

package config;

use strict;
use warnings 'all';

use setup qw($current_dir $wine_dir $winapi_dir);

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw(
    file_absolutize file_normalize
    file_directory
    file_type files_filter
    file_skip files_skip
    get_c_files
    get_h_files
    get_makefile_in_files
    get_spec_files
);
@EXPORT_OK = qw(
    $current_dir $wine_dir $winapi_dir
);

use vars qw($current_dir $wine_dir $winapi_dir);

use output qw($output);

use File::Find;

sub file_normalize($) {
    local $_ = shift;

    foreach my $dir (split(m%/%, $current_dir)) {
	if(s%^(\.\./)*\.\./$dir/%%) {
	    if(defined($1)) {
		$_ = "$1$_";
	    }
	}
    }

    while(m%^(.*?)([^/\.]+)/\.\./(.*?)$%) {
	if($2 ne "." && $2 ne "..") {
	    $_ = "$1$3";
	}
    }

    return $_;
}

sub file_absolutize($) {
    local $_ = shift;

    $_ = file_normalize($_);
    if(!s%^$wine_dir/%%) {
	$_ = "$current_dir/$_";
    }
    s%^\./%%;

    return $_;
}

sub file_type($) {
    local $_ = shift;

    $_ = file_absolutize($_);

    m%^(?:server|tests|tools)/% && return "";
    m%^(?:programs)/% && return "wineapp";
    m%^(?:libs)/% && return "library";

    return "winelib";
}

sub files_filter($@) {
    my $type = shift;

    my @files;
    foreach my $file (@_) {
	if(file_type($file) eq $type) {
	    push @files, $file;
	}
    }

    return @files;
}

sub file_skip($) {
    local $_ = shift;

    $_ = file_absolutize($_);

    m%^(?:dlls|include)/% || return 1;
    m%^dlls/wineps\.drv/data/% && return 1;
    return 0;
}

sub files_skip(@) {
    my @files;
    foreach my $file (@_) {
	if(!file_skip($file)) {
	    push @files, $file;
	}
    }

    return @files;
}

sub file_directory($) {
    local $_ = shift;

    s%/?[^/]*$%%;
    if(!$_) {
	$_ = ".";
    }

    s%^(?:\./)?(.*?)(?:/\.)?%$1%;

    return $_;
}

sub _get_files($$;$) {
    my $pattern = shift;
    my $type = shift;
    my $dir = shift;

    $output->progress("$wine_dir: searching for /$pattern/");

    if(!defined($dir)) {
	$dir = $wine_dir;
    }

    my @files;

    my @dirs = ($dir);
    while(defined(my $dir = shift @dirs)) {
	opendir(DIR, $dir) || die "Can't open directory $dir: $!\n";
	my @entries= readdir(DIR);
	closedir(DIR);
	foreach (@entries) {
	    my $basefile = $_;
	    $_ = "$dir/$_";
	    if(/\.\.?$/) {
		# Nothing
	    } elsif(-d $_) {
		push @dirs, $_;
	    } elsif($basefile =~ /$pattern/ && (!defined($type) || file_type($_) eq $type)) {
		s%^$wine_dir/%%;
		push @files, $_;
	    }
	}
    }

    return @files;
}

sub get_c_files($;$) { return _get_files('\.c$', $_[0], $_[1]); }
sub get_h_files($;$) { return _get_files('\.h$', $_[0], $_[1]); }
sub get_spec_files($;$) { return _get_files('\.spec$', $_[0], $_[1]); }
sub get_makefile_in_files($;$) { return _get_files('^Makefile.in$', $_[0], $_[1]); }

1;
