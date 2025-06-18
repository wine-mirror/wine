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

package modules;

use strict;
use warnings 'all';

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw($modules);

use vars qw($modules);

use config qw(
    file_type files_skip
    file_directory
    get_c_files get_spec_files
    $current_dir $wine_dir
);
use options qw($options);
use output qw($output);

sub import(@) {
    $Exporter::ExportLevel++;
    Exporter::import(@_);
    $Exporter::ExportLevel--;

    if (defined($modules)) {
	return;
    }

    $modules = 'modules'->new;
}

sub get_spec_file_type($) {
    my $file = shift;

    my $module;
    my $type;

    $module = $file;
    $module =~ s%^.*?([^/]+)\.spec$%$1%;

    open(IN, "< $file") || die "$file: $!\n";
    local $/ = "\n";
    my $header = 1;
    my $lookahead = 0;
    while($lookahead || defined($_ = <IN>)) {
	$lookahead = 0;
	s/^\s*(.*?)\s*$/$1/;
	s/^(.*?)\s*#.*$/$1/;
	/^$/ && next;

	if($header)  {
	    if(/^(?:\d+|@)/) { $header = 0; $lookahead = 1; }
	    next;
	}

	if(/^(\d+|@)\s+pascal(?:16)?/) {
	    $type = "win16";
	    last;
	}
    }
    close(IN);

    if(!defined($type)) {
	$type = "win32";
    }

    return ($type, $module);
}

sub find_spec_files($) {
    my $self = shift;

    my $dir2spec_file = \%{$self->{DIR2SPEC_FILE}};
    my $spec_file2dir = \%{$self->{SPEC_FILE2DIR}};

    $output->progress("modules");

    my $spec_file_found = {};
    my $allowed_dir;
    my $spec_file;

    my @spec_files = <{dlls/*/*.spec}>;

    foreach $spec_file (@spec_files) {
        $spec_file =~ /(.*)\/.*\.spec/;

        $allowed_dir = $1;

        $$spec_file_found{$spec_file}++;
        $$spec_file2dir{$spec_file}{$allowed_dir}++;
        $$dir2spec_file{$allowed_dir}{$spec_file}++;
        # gdi32.dll and gdi.exe have some extra sources in subdirectories
        if ($spec_file =~ m!/gdi32\.spec$!)
        {
            $$spec_file2dir{$spec_file}{"$allowed_dir/enhmfdrv"}++;
            $$dir2spec_file{"$allowed_dir/enhmfdrv"}{$spec_file}++;
        }
        if ($spec_file =~ m!/gdi(?:32|\.exe)\.spec$!)
        {
            $$spec_file2dir{$spec_file}{"$allowed_dir/mfdrv"}++;
            $$dir2spec_file{"$allowed_dir/mfdrv"}{$spec_file}++;
        }
    }

    return $spec_file_found;
}

sub read_spec_files($$) {
    my $self = shift;

    my $spec_file_found = shift;

    my $dir2spec_file = \%{$self->{DIR2SPEC_FILE}};
    my $spec_files16 = \@{$self->{SPEC_FILES16}};
    my $spec_files32 = \@{$self->{SPEC_FILES32}};
    my $spec_file2module = \%{$self->{SPEC_FILE2MODULE}};
    my $module2spec_file = \%{$self->{MODULE2SPEC_FILE}};

    my @spec_files;
    if($wine_dir eq ".") {
	@spec_files = get_spec_files("winelib");
    } else {
	my %spec_files = ();
	foreach my $dir ($options->directories) {
	    $dir = "$current_dir/$dir";
	    $dir =~ s%/\.$%%;
	    foreach my $spec_file (sort(keys(%{$$dir2spec_file{$dir}}))) {
		$spec_files{$spec_file}++;
	    }
	}
	@spec_files = sort(keys(%spec_files));
    }

    @$spec_files16 = ();
    @$spec_files32 = ();
    foreach my $spec_file (@spec_files) {
	(my $type, my $module) = get_spec_file_type("$wine_dir/$spec_file");

	$$spec_file2module{$spec_file} = $module;
	$$module2spec_file{$module} = $spec_file;

	if($type eq "win16") {
	    push @$spec_files16, $spec_file;
	} elsif($type eq "win32") {
	    push @$spec_files32, $spec_file;
	} else {
	    $output->write("$spec_file: unknown type '$type'\n");
	}
    }

    foreach my $spec_file (@spec_files) {
	if(!$$spec_file_found{$spec_file} && $spec_file !~ m%tests/[^/]+$%) {
	    $output->write("modules: $spec_file: exists but is not specified\n");
	}
    }
}

sub new($) {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $spec_file_found = $self->find_spec_files();
    $self->read_spec_files($spec_file_found);

    return $self;
}

sub all_modules($) {
    my $self = shift;

    my $module2spec_file = \%{$self->{MODULE2SPEC_FILE}};

    return sort(keys(%$module2spec_file));
}

sub is_allowed_module($$) {
    my $self = shift;

    my $module2spec_file = \%{$self->{MODULE2SPEC_FILE}};

    my $module = shift;

    return defined($$module2spec_file{$module});
}

sub is_allowed_module_in_file($$$) {
    my $self = shift;

    my $dir2spec_file = \%{$self->{DIR2SPEC_FILE}};
    my $spec_file2module = \%{$self->{SPEC_FILE2MODULE}};

    my $module = shift;
    my $file = shift;
    $file =~ s/^\.\///;

    my $dir = $file;
    $dir =~ s/\/[^\/]*$//;

    if($dir =~ m%^include%) {
	return 1;
    }

    foreach my $spec_file (sort(keys(%{$$dir2spec_file{$dir}}))) {
	if($$spec_file2module{$spec_file} eq $module) {
	    return 1;
	}
    }

    return 0;
}

sub allowed_modules_in_file($$) {
    my $self = shift;

    my $dir2spec_file = \%{$self->{DIR2SPEC_FILE}};
    my $spec_file2module = \%{$self->{SPEC_FILE2MODULE}};

    my $file = shift;
    $file =~ s/^\.\///;

    my $dir = $file;
    $dir =~ s/\/[^\/]*$//;

    my %allowed_modules = ();
    foreach my $spec_file (sort(keys(%{$$dir2spec_file{$dir}}))) {
	my $module = $$spec_file2module{$spec_file};
	$allowed_modules{$module}++;
    }

    my $module = join(" & ", sort(keys(%allowed_modules)));

    return $module;
}

sub allowed_dirs_for_module($$) {
   my $self = shift;

   my $module2spec_file = \%{$self->{MODULE2SPEC_FILE}};
   my $spec_file2dir = \%{$self->{SPEC_FILE2DIR}};

   my $module = shift;

   my $spec_file = $$module2spec_file{$module};

   return sort(keys(%{$$spec_file2dir{$spec_file}}));
}

sub allowed_spec_files16($) {
    my $self = shift;

    my $spec_files16 = \@{$self->{SPEC_FILES16}};

    return @$spec_files16;
}

sub allowed_spec_files32($) {
    my $self = shift;

    my $spec_files32 = \@{$self->{SPEC_FILES32}};

    return @$spec_files32;
}

sub found_module_in_dir($$$) {
    my $self = shift;

    my $module = shift;
    my $dir = shift;

    my $used_module_dirs = \%{$self->{USED_MODULE_DIRS}};

    $dir = "$current_dir/$dir";
    $dir =~ s%/\.$%%;

    $$used_module_dirs{$module}{$dir}++;
}

sub complete_modules($$) {
    my $self = shift;

    my $c_files = shift;

    my %dirs;

    foreach my $file (@$c_files) {
	my $dir = file_directory("$current_dir/$file");
	$dirs{$dir}++;
    }

    my @c_files = get_c_files("winelib");
    @c_files = files_skip(@c_files);
    foreach my $file (@c_files) {
	my $dir = file_directory($file);
	if(exists($dirs{$dir})) {
	    $dirs{$dir}--;
	}
    }

    my @complete_modules = ();
    foreach my $module ($self->all_modules) {
	my $index = -1;
	my @dirs = $self->allowed_dirs_for_module($module);
	foreach my $dir (@dirs) {
	    if(exists($dirs{$dir}) && $dirs{$dir} == 0) {
		$index++;
	    }
	}
	if($index == $#dirs) {
	    push @complete_modules, $module;
	}
    }

    return @complete_modules;
}

sub global_report($) {
    my $self = shift;

    my $module2spec_file = \%{$self->{MODULE2SPEC_FILE}};
    my $used_module_dirs = \%{$self->{USED_MODULE_DIRS}};

    my @messages;
    foreach my $dir ($options->directories) {
	$dir = "$current_dir/$dir";
	$dir =~ s%/\.$%%;
	foreach my $module ($self->all_modules) {
	    if(!$$used_module_dirs{$module}{$dir}) {
		my $spec_file = $$module2spec_file{$module};
		push @messages, "modules: $spec_file: directory ($dir) is not used\n";
	    }
	}
    }

    foreach my $message (sort(@messages)) {
	$output->write($message);
    }
}

1;
