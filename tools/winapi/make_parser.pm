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

package make_parser;

use strict;
use warnings 'all';

use setup qw($current_dir $wine_dir $winapi_dir);

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw($directory $tool $file $line $message);

use vars qw($directory $tool $file $line $message);

use output qw($output);
use options qw($options);


#sub command($);
#sub gcc_output($$);
#sub ld_output($$);
#sub make_output($$);
#sub winebuild_output($$);
#sub wmc_output($$);
#sub wrc_output($$);


########################################################################
# global
########################################################################

my $current;
my $function;

########################################################################
# error
########################################################################

sub error($) {
    my $where = shift;

    if(!defined($where)) {
	$where = "";
    }

    my $context;
    if($tool) {
	$context = "$tool";
	if($where) {
	    $context .= "<$where>";
	}
    } else {
	if($where) {
	    $context = "<$where>";
	} else {
	    $context = "<>";
	}
    }

    if(defined($tool)) {
	$output->write("$directory: $context: can't parse output: '$current'\n");
    } else {
	$output->write("$directory: $context: can't parse output: '$current'\n");
    }
    exit 1;
}

########################################################################
# make_output
########################################################################

sub make_output($$) {
    my $level = shift;
    local $_ = shift;

    $file = "";
    $message = "";

    if(/^\*\*\* \[(.*?)\] Error (\d+)$/) {
	# Nothing
    } elsif(/^\*\*\* Error code (\d+)$/) {
	# Nothing
    } elsif(/^\*\*\* Warning:\s+/) { #
	if(/^File \`(.+?)\' has modification time in the future \((.+?) > \(.+?\)\)$/) {
	    # Nothing
	} else {
	    error("make_output");
	}
    } elsif(/^\`(.*?)\' is up to date.$/) {
	# Nothing
    } elsif(/^\[(.*?)\] Error (\d+) \(ignored\)$/) {
	# Nothing
    } elsif(/^don\'t know how to make (.*?)\. Stop$/) {
	$message = "$_";
    } elsif(/^(Entering|Leaving) directory \`(.*?)\'$/) {
	if($1 eq "Entering") {
	    $directory = $2;
	} else {
	    $directory = "";
	}

	my @components;
	foreach my $component (split(/\//, $directory)) {
	    if($component eq "wine") {
		@components = ();
	    } else {
		push @components, $component;
	    }
	}
	$directory = join("/", @components);
    } elsif(/^(.*?) is older than (.*?), please rerun (.*?)\$/) {
	# Nothing
    } elsif(/^Nothing to be done for \`(.*?)\'\.$/) {
	# Nothing
    } elsif(s/^warning:\s+//) {
	if(/^Clock skew detected.  Your build may be incomplete.$/) {
	    # Nothing
	} else {
	    error("make_output");
	}
    } elsif(/^Stop in (.*?)\.$/) {
	# Nothing
    } elsif(/^\s*$/) {
	# Nothing
    } else {
	error("make_output");
    }

}

########################################################################
# ar_command
########################################################################

sub ar_command($) {
    local $_ = shift;

    my $read_files;
    my $write_files;

    if(/rc\s+(\S+)(\s+\S+)+$/) {
	$write_files = [$1];
	$read_files = $2;
	$read_files =~ s/^\s*//;
	$read_files = [split(/\s+/, $read_files)];
    } else {
	error("ar_command");
    }

    return ($read_files, $write_files);
}

########################################################################
# as_command
########################################################################

sub as_command($) {
    local $_ = shift;

    my $read_files;
    my $write_files;

    if(/-o\s+(\S+)\s+(\S+)$/) {
	$write_files = [$1];
	$read_files = [$2];
    } else {
	error("as_command");
    }

    return ($read_files, $write_files);
}

########################################################################
# bision_command
########################################################################

sub bison_command($) {
    local $_ = shift;

    return ([], []);
}

########################################################################
# cd_command
########################################################################

sub cd_command($) {
    local $_ = shift;

    return ([], []);
}

########################################################################
# cd_output
########################################################################

sub cd_output($) {
    local $_ = shift;

    if(/^(.*?): No such file or directory/) {
	$message = "directory '$1' doesn't exist";
    }
}

########################################################################
# flex_command
########################################################################

sub flex_command($) {
    local $_ = shift;

    return ([], []);
}

########################################################################
# for_command
########################################################################

sub for_command($) {
    local $_ = shift;

    return ([], []);
}

########################################################################
# gcc_command
########################################################################

sub gcc_command($) {
    my $read_files;
    my $write_files;

    if(/-o\s+(\S+)\s+(\S+)$/) {
	my $write_file = $1;
	my $read_file = $2;

	$write_file =~ s%^\./%%;
	$read_file =~ s%^\./%%;

	$write_files = [$write_file];
	$read_files = [$read_file];
    } elsif(/-o\s+(\S+)/) {
	my $write_file = $1;

	$write_file =~ s%^\./%%;

	$write_files = [$write_file];
	$read_files = ["<???>"];
    } elsif(/^-shared.*?-o\s+(\S+)/) {
	my $write_file = $1;

	$write_file =~ s%^\./%%;

	$write_files = [$write_file];
	$read_files = ["<???>"];
    } else {
	error("gcc_command");
    }

    return ($read_files, $write_files);
}

########################################################################
# gcc_output
########################################################################

sub gcc_output($$) {
    $file = shift;
    local $_ = shift;

    if(s/^(\d+):\s+//) {
	$line = $1;
	if(s/^warning:\s+//) {
	    my $suppress = 0;

	    if(/^((?:signed |unsigned )?(?:int|long)) format, (different type|\S+) arg \(arg (\d+)\)$/) {
		my $type = $2;
		if($type =~ /^(?:
		   HACCEL|HACMDRIVER|HANDLE|HBITMAP|HBRUSH|HCALL|HCURSOR|HDC|HDRVR|HDESK|HDRAWDIB
		   HGDIOBJ|HKL|HGLOBAL|HIMC|HINSTANCE|HKEY|HLOCAL|
		   HMENU|HMIDISTRM|HMIDIIN|HMIDIOUT|HMIXER|HMIXEROBJ|HMMIO|HMODULE|
		   HLINE|HPEN|HPHONE|HPHONEAPP|
		   HRASCONN|HRGN|HRSRC|HWAVEIN|HWAVEOUT|HWINSTA|HWND|
		   SC_HANDLE|WSAEVENT|handle_t|pointer)$/x)
		{
		    $suppress = 1;
		} else {
		    $suppress = 0;
		}
	    } elsif(/^\(near initialization for \`(.*?)\'\)$/) {
		$suppress = 0;
	    } elsif(/^\`(.*?)\' defined but not used$/) {
		$suppress = 0;
	    } elsif(/^\`(.*?)\' is not at beginning of declaration$/) {
		$suppress = 0;
	    } elsif(/^\`%x\' yields only last 2 digits of year in some locales$/) {
		$suppress = 1;
	    } elsif(/^assignment makes integer from pointer without a cast$/) {
		$suppress = 0;
	    } elsif(/^assignment makes pointer from integer without a cast$/) {
		$suppress = 0;
	    } elsif(/^assignment from incompatible pointer type$/) {
		$suppress = 0;
	    } elsif(/^cast from pointer to integer of different size$/) {
		$suppress = 0;
	    } elsif(/^comparison between pointer and integer$/) {
		$suppress = 0;
	    } elsif(/^comparison between signed and unsigned$/) {
		$suppress = 0;
	    } elsif(/^comparison of unsigned expression < 0 is always false$/) {
		$suppress = 0;
	    } elsif(/^comparison of unsigned expression >= 0 is always true$/) {
		$suppress = 0;
	    } elsif(/^conflicting types for built-in function \`(.*?)\'$/) {
		$suppress = 0;
	    } elsif(/^empty body in an if-statement$/) {
		$suppress = 0;
	    } elsif(/^empty body in an else-statement$/) {
		$suppress = 0;
	    } elsif(/^implicit declaration of function \`(.*?)\'$/) {
		$suppress = 0;
	    } elsif(/^initialization from incompatible pointer type$/) {
		$suppress = 0;
	    } elsif(/^initialization makes pointer from integer without a cast$/) {
		$suppress = 0;
	    } elsif(/^missing initializer$/) {
		$suppress = 0;
	    } elsif(/^ordered comparison of pointer with integer zero$/) {
		$suppress = 0;
	    } elsif(/^passing arg (\d+) of (?:pointer to function|\`(\S+)\') from incompatible pointer type$/) {
		$suppress = 0;
	    } elsif(/^passing arg (\d+) of (?:pointer to function|\`(\S+)\') makes integer from pointer without a cast$/) {
		$suppress = 0;
	    } elsif(/^passing arg (\d+) of (?:pointer to function|\`(\S+)\') makes pointer from integer without a cast$/) {
		$suppress = 0;
	    } elsif(/^return makes integer from pointer without a cast$/) {
		$suppress = 0;
	    } elsif(/^return makes pointer from integer without a cast$/) {
		$suppress = 0;
	    } elsif(/^type of \`(.*?)\' defaults to \`(.*?)\'$/) {
		$suppress = 0;
	    } elsif(/^unused variable \`(.*?)\'$/) {
		$suppress = 0;
	    } elsif(!$options->pedantic) {
		$suppress = 0;
	    } else {
		error("gcc_output");
	    }

	    if(!$suppress) {
		if($function) {
		    $message = "function $function: warning: $_";
		} else {
		    $message = "warning: $_";
		}
	    } else {
		$message = "";
	    }
	} elsif(/^\`(.*?)\' undeclared \(first use in this function\)$/) {
	    $message = "$_";
	} elsif(/^\(Each undeclared identifier is reported only once$/) {
	    $message = "$_";
	} elsif(/^conflicting types for \`(.*?)\'$/) {
	    $message = "$_";
	} elsif(/^for each function it appears in.\)$/) {
	    $message = "$_";
	} elsif(/^too many arguments to function$/) {
	    $message = "$_";
        } elsif(/^previous declaration of \`(.*?)\'$/) {
	    $message = "$_";
	} elsif(/^parse error before `(.*?)'$/) {
	    $message = "$_";
	} elsif(!$options->pedantic) {
	    $message = "$_";
	} else {
	    error("gcc_output");
	}
    } elsif(/^In function \`(.*?)\':$/) {
	$function = $1;
    } elsif(/^At top level:$/) {
	$function = "";
    } else {
	error("gcc_output");
    }
}

########################################################################
# install_command
########################################################################

sub install_command($) {
    local $_ = shift;

    return ([], []);
}

########################################################################
# ld_command
########################################################################

sub ld_command($) {
    local $_ = shift;

    my $read_files;
    my $write_files;

    if(/-r\s+(.*?)\s+-o\s+(\S+)$/) {
	$write_files = [$2];
	$read_files = [split(/\s+/, $1)];
    } else {
	error("ld_command");
    }

    return ($read_files, $write_files);
}

########################################################################
# ld_output
########################################################################

sub ld_output($$) {
    $file = shift;
    local $_ = shift;

    if(/^In function \`(.*?)\':$/) {
	$function = $1;
    } elsif(/^more undefined references to \`(.*?)\' follow$/) {
	# Nothing
    } elsif(/^the use of \`(.+?)\' is dangerous, better use \`(.+?)\'$/) {
	# Nothing
    } elsif(/^undefined reference to \`(.*?)\'$/) {
	# Nothing
    } elsif(/^warning: (.*?)\(\) possibly used unsafely; consider using (.*?)\(\)$/) {
	# Nothing
    } elsif(/^warning: type and size of dynamic symbol \`(.*?)\' are not defined$/) {
	$message = "$_";
    } else {
	$message = "$_";
    }
}

########################################################################
# ldconfig_command
########################################################################

sub ldconfig_command($) {
    local $_ = shift;

    return ([], []);
}

########################################################################
# makedep_command
########################################################################

sub makedep_command($) {
    local $_ = shift;

    return ([], []);
}

########################################################################
# mkdir_command
########################################################################

sub mkdir_command($) {
    local $_ = shift;

    return ([], []);
}

########################################################################
# ranlib_command
########################################################################

sub ranlib_command($) {
    local $_ = shift;

    my $read_files;
    my $write_files;

    $read_files = [split(/\s+/)];
    $write_files = [];

    return ($read_files, $write_files);
}

########################################################################
# rm_command
########################################################################

sub rm_command($) {
    local $_ = shift;
    s/^-f\s*//;
    return ([], [], [split(/\s+/, $_)]);
}

########################################################################
# sed_command
########################################################################

sub sed_command($) {
    local $_ = shift;

    return ([], []);
}

########################################################################
# strip_command
########################################################################

sub strip_command($) {
    local $_ = shift;

    return ([], []);
}

########################################################################
# winebuild_command
########################################################################

sub winebuild_command($) {
    local $_ = shift;

    return ([], []);
}

########################################################################
# winebuild_output
########################################################################

sub winebuild_output($$) {
    $file = shift;
    local $_ = shift;

    $message = $_;
}

########################################################################
# wmc_command
########################################################################

sub wmc_command($) {
    local $_ = shift;

    my $read_files;
    my $write_files;

    if(/\s+(\S+)$/) {
	my $mc_file = $1;

	my $rc_file = $mc_file;
	$rc_file =~ s/\.mc$/.rc/;

	$write_files = [$rc_file];
	$read_files = [$mc_file];
    } else {
	error("wmc_command");
    }

    return ($read_files, $write_files);
}

########################################################################
# wmc_output
########################################################################

sub wmc_output($$) {
    $file = shift;
    local $_ = shift;
}

########################################################################
# wrc_command
########################################################################

sub wrc_command($) {
    local $_ = shift;

    my $read_files;
    my $write_files;

    if(/\s+(\S+)$/) {
	my $rc_file = $1;

	my $o_file = $rc_file;
	$o_file =~ s/\.rc$/.o/;

	$write_files = [$o_file];
	$read_files = [$rc_file];
    } else {
	error("wrc_command");
    }

    return ($read_files, $write_files);
}

########################################################################
# wrc_output
########################################################################

sub wrc_output($$) {
    $file = shift;
    local $_ = shift;
}

########################################################################
# command
########################################################################

sub command($) {
    local $_ = shift;

    my $tool;
    my $file;
    my $read_files = ["<???>"];
    my $write_files = ["<???>"];
    my $remove_files = [];

    s/^\s*(.*?)\s*$/$1/;

    if(s/^\[\s+-d\s+(.*?)\s+\]\s+\|\|\s+//) {
	# Nothing
    }

    if(s/^ar\s+//) {
	$tool = "ar";
	($read_files, $write_files) = ar_command($_);
    } elsif(s/^as\s+//) {
	$tool = "as";
	($read_files, $write_files) = as_command($_);
    } elsif(s/^bison\s+//) {
	$tool = "bison";
	($read_files, $write_files) = bison_command($_);
    } elsif(s/^cd\s+//) {
	$tool = "cd";
	($read_files, $write_files) = cd_command($_);
    } elsif(s/^flex\s+//) {
	$tool = "flex";
	($read_files, $write_files) = flex_command($_);
    } elsif(s/^for\s+//) {
	$tool = "for";
	($read_files, $write_files) = for_command($_);
    } elsif(s/^\/usr\/bin\/install\s+//) {
	$tool = "install";
	($read_files, $write_files) = install_command($_);
    } elsif(s/^ld\s+//) {
	$tool = "ld";
	($read_files, $write_files) = ld_command($_);
    } elsif(s/^\/sbin\/ldconfig\s+//) {
	$tool = "ldconfig";
	($read_files, $write_files) = ldconfig_command();
    } elsif(s/^gcc\s+//) {
	$tool = "gcc";
	($read_files, $write_files) = gcc_command($_);
    } elsif(s/^(?:(?:\.\.\/)+|\.\/)tools\/makedep\s+//) {
	$tool = "makedep";
	($read_files, $write_files) = makedep_command($_);
    } elsif(s/^mkdir\s+//) {
	$tool = "mkdir";
	($read_files, $write_files) = mkdir_command($_);
    } elsif(s/^ranlib\s+//) {
	$tool = "ranlib";
	($read_files, $write_files) = ranlib_command($_);
    } elsif(s/^rm\s+//) {
	$tool = "rm";
	($read_files, $write_files, $remove_files) = rm_command($_);
    } elsif(s/^sed\s+//) {
	$tool = "sed";
	($read_files, $write_files) = sed_command($_);
    } elsif(s/^strip\s+//) {
	$tool = "sed";
	($read_files, $write_files) = strip_command($_);
    } elsif(s/^LD_LIBRARY_PATH="(?:(?:\.\.\/)*unicode)?:\$LD_LIBRARY_PATH"\s+(?:\.\.\/)*tools\/winebuild\/winebuild\s+//) {
	$tool = "winebuild";
	($read_files, $write_files) = winebuild_command($_);
    } elsif(s/^LD_LIBRARY_PATH="(?:(?:\.\.\/)*unicode)?:\$LD_LIBRARY_PATH"\s+(?:\.\.\/)*tools\/wmc\/wmc\s+//) {
	$tool = "wmc";
	($read_files, $write_files) = wmc_command($_);
    } elsif(s/^LD_LIBRARY_PATH="(?:(?:\.\.\/)*unicode)?:\$LD_LIBRARY_PATH"\s+(?:\.\.\/)*tools\/wrc\/wrc\s+//) {
	$tool = "wrc";
	($read_files, $write_files) = wrc_command($_);
    }

    return ($tool, $read_files, $write_files, $remove_files);
}

########################################################################
# line
########################################################################

sub line($) {
    local $_ = shift;

    $file = "";
    $line = "";
    $message = "";

    $current = $_;

    my ($new_tool, $read_files, $write_files, $remove_files) = command($_);
    if(defined($new_tool)) {
	$tool = $new_tool;

	$function = "";

	my $progress = "";
	if($directory && $directory ne ".") {
	    $progress .= "$directory: ";
	}
	if($tool) {
	    $progress .= "$tool: ";
	}

	if($tool =~ /^(?:cd|make)$/) {
	    # Nothing
	} elsif($tool eq "ld"/) {
	    foreach my $file (@{$read_files}) {
		$output->lazy_progress("${progress}reading '$file'");
	    }
	    my $file = $$write_files[0];
	    $output->progress("$progress: writing '$file'");
	} elsif($tool eq "rm") {
	    foreach my $file (@{$remove_files}) {
		$output->lazy_progress("${progress}removing '$file'");
	    }
	} else {
	    if($#$read_files >= 0) {
		$progress .= "read[" . join(" ", @{$read_files}) . "]";
	    }
	    if($#$write_files >= 0) {
		if($#$read_files >= 0) {
		    $progress .= ", ";
		}
		$progress .= "write[" . join(" ", @{$write_files}) . "]";
	    }
	    if($#$remove_files >= 0) {
		if($#$read_files >= 0 || $#$write_files >= 0) {
		    $progress .= ", ";
		}
		$progress .= "remove[" . join(" ", @{$remove_files}) . "]";
	    }

	    $output->progress($progress);
	}

	return 0;
    }

    my $make = $options->make;

    if(/^Wine build complete\.$/) {
	# Nothing
    } elsif(/^(.*?) is newer than (.*?), please rerun (.*?)\!$/) {
	$message = "$_";
    } elsif(/^(.*?) is older than (.*?), please rerun (.*?)$/) {
	$message = "$_";
    } elsif(/^\`(.*?)\' is up to date.$/) {
	$tool = "make";
	make_output($1, $_);
    } elsif(s/^$make(?:\[(\d+)\])?:\s*//) {
	$tool = "make";
	make_output($1, $_);
    } elsif(!defined($tool)) {
	error("line");
    } elsif($tool eq "make") {
	make_output($1, $_);
    } elsif($tool eq "bison" && /^conflicts:\s+\d+\s+shift\/reduce$/) {
	# Nothing
    } elsif($tool eq "gcc" && /^(?:In file included |\s*)from (.+?):(\d+)[,:]$/) {
	# Nothing
    } elsif($tool =~ /^(?:gcc|ld)$/ && s/^(.+?\.s?o)(?:\(.*?\))?:\s*//) {
	$tool = "ld";
	ld_output($1, $_)
    } elsif($tool =~ /^(?:gcc|ld)$/ && s/^(.*?)ld:\s*//) {
	$tool = "ld";
	ld_output("", $_)
    } elsif($tool =~ /^(?:gcc|ld)$/ && s/^collect2:\s*//) {
	$tool = "ld";
	ld_output("collect2", $_);
    } elsif($tool eq "gcc" && s/^(.+?\.[chly]):\s*//) {
	gcc_output($1, $_);
    } elsif($tool eq "ld" && s/^(.+?\.c):(?:\d+:)?\s*//) {
	ld_output($1, $_);
    } elsif($tool eq "winebuild" && s/^(.+?\.spec):\s*//) {
	winebuild_output($1, $_);
    } elsif($tool eq "wmc" && s/^(.+?\.mc):\s*//) {
        wmc_output($1, $_);
    } elsif($tool eq "wrc" && s/^(.+?\.rc):\s*//) {
	wrc_output($1, $_);
    } elsif($tool eq "cd" && s/^\/bin\/sh:\s*cd:\s*//) {
	parse_cd_output($_);
    } elsif(/^\s*$/) {
	# Nothing
    } else {
	error("line");
    }

    $file =~ s/^\.\///;

    return 1;
}

1;
