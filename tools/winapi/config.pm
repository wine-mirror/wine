package config;

use strict;

use setup qw($current_dir $wine_dir $winapi_dir $winapi_check_dir);

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw(
    &file_absolutize &file_normalize
    &file_type &files_filter
    &file_skip &files_skip 
    &get_spec_files
);
@EXPORT_OK = qw(
    $current_dir $wine_dir $winapi_dir $winapi_check_dir
);

use vars qw($current_dir $wine_dir $winapi_dir $winapi_check_dir);

my $output = "output";

sub file_type {
    local $_ = shift;

    $_ = file_absolutize($_);

    m%^(?:libtest|rc|server|tests|tools)/% && return "";
    m%^(?:programs|debugger|miscemu)/% && return "wineapp";
    m%^(?:library|tsx11|unicode)/% && return "library";
    m%^windows/x11drv/wineclipsrv.c% && return "application";

    return "winelib";
}

sub files_filter {
    my $type = shift;

    my @files;
    foreach my $file (@_) {
	if(file_type($file) eq $type) {
	    push @files, $file;
	}
    }

    return @files;
}

sub file_skip {
    local $_ = shift;

    $_ = file_absolutize($_);

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

sub file_absolutize {
    local $_ = shift;

    $_ = file_normalize($_);
    if(!s%^$wine_dir/%%) {
	$_ = "$current_dir/$_";
    }
    s%^\./%%;

    return $_;
}

sub file_normalize {
    local $_ = shift;

    foreach my $dir (split(m%/%, $current_dir)) {
	s%^(\.\./)*\.\./$dir/%%;
	if(defined($1)) {
	    $_ = "$1$_";
	}
    }

    return $_;
}

sub get_spec_files {
    $output->progress("$wine_dir: searching for *.spec");

    my @spec_files = map {
	s%^\./%%;
	s%^$wine_dir/%%;
	if(file_type($_) eq "winelib") {
	    $_;
	} else {
	    ();
	}
    } split(/\n/, `find $wine_dir -name \\*.spec`);

    return @spec_files;
}

1;
