package config;

use strict;

use setup qw($current_dir $wine_dir $winapi_dir $winapi_check_dir);

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw(
    &file_type &file_skip &files_skip &get_spec_files
);
@EXPORT_OK = qw(
    $current_dir $wine_dir $winapi_dir $winapi_check_dir
);

use vars qw($current_dir $wine_dir $winapi_dir $winapi_check_dir);

sub file_type {
    my $file = shift;

    my $file_dir = $file;
    if(!($file_dir =~ s/^(.*?)\/[^\/]*$/$1/)) {
	$file_dir = ".";
    }

    $file_dir =~ s/^$wine_dir\///;

    if($file_dir =~ /^(libtest|programs|rc|server|tests|tools)/ || 
       $file =~ /dbgmain\.c$/ ||
       $file =~ /wineclipsrv\.c$/) # FIXME: Kludge
    {
	return "application";
    } elsif($file_dir =~ /^(debugger|miscemu)/) {
	return "emulator";
    } else {
	return "library";
    }
}

sub file_skip {
    local $_ = shift;

    $_ = "$current_dir/$_";
    s%^\./%%;

    m%^(?:libtest|programs|rc|server|tests|tools)/% && return 1;
    m%^(?:debugger|miscemu|tsx11|server|unicode)/% && return 1;
    m%^dlls/wineps/data/% && return 1;
    m%^windows/x11drv/wineclipsrv.c% && return 1;
    m%^dlls/winmm/wineoss/midipatch.c% && return 1;

    return 0;
}

sub files_skip {
    my @files;
    foreach my $file (@_) {
	if(!file_skip($file)) {
	    push @files, $file;
	}
    }

    return @files;
}

sub get_spec_files {
    output->progress("$wine_dir: searching for *.spec");

    my @spec_files = map {
	s/^$wine_dir\/(.*)$/$1/;
	if(file_type($_) eq "library") {
	    $_;
	} else {
	    ();
	}
    } split(/\n/, `find $wine_dir -name \\*.spec`);

    return @spec_files;
}

1;
