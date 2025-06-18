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

package winapi;

use strict;
use warnings 'all';

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw($win16api $win32api @winapis);

use vars qw($win16api $win32api @winapis);

use config qw($current_dir $wine_dir $winapi_dir);
use options qw($options);
use output qw($output);

use vars qw($modules);

sub found_shared_internal_function($$);
sub function_external_calling_convention_in_module($$$);
sub function_internal_module($$);
sub is_function_stub_in_module($$$);
sub new($$$);
sub parse_api_file($$);
sub parse_spec_file($$);

sub import(@) {
    $Exporter::ExportLevel++;
    Exporter::import(@_);
    $Exporter::ExportLevel--;

    if (defined($modules) && defined($win16api) && defined($win32api)) {
	return;
    }

    require modules;
    import modules qw($modules);

    my @spec_files16 = $modules->allowed_spec_files16;
    $win16api = 'winapi'->new("win16", \@spec_files16);

    my @spec_files32 = $modules->allowed_spec_files32;
    $win32api = 'winapi'->new("win32", \@spec_files32);

    @winapis = ($win16api, $win32api);

    for my $internal_name ($win32api->all_internal_functions) {
	my $module16 = $win16api->function_internal_module($internal_name);
	my $module32 = $win16api->function_internal_module($internal_name);
	if(defined($module16) &&
	   !$win16api->is_function_stub_in_module($module16, $internal_name) &&
	   !$win32api->is_function_stub_in_module($module32, $internal_name))
	{
	    $win16api->found_shared_internal_function($internal_name);
	    $win32api->found_shared_internal_function($internal_name);
	}
    }
}


sub new($$$) {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $name = \${$self->{NAME}};
    my $function_forward = \%{$self->{FUNCTION_FORWARD}};
    my $function_internal_name = \%{$self->{FUNCTION_INTERNAL_NAME}};
    my $function_module = \%{$self->{FUNCTION_MODULE}};

    $$name = shift;
    my $refspec_files = shift;

    foreach my $file (@$refspec_files) {
	$self->parse_spec_file("$wine_dir/$file");
    }

    $self->parse_api_file("$$name.api");

    foreach my $module (sort(keys(%$function_forward))) {
	foreach my $external_name (sort(keys(%{$$function_forward{$module}}))) {
	    (my $forward_module, my $forward_external_name) = @{$$function_forward{$module}{$external_name}};
	    my $forward_internal_name = $$function_internal_name{$forward_external_name};
	    if(defined($forward_internal_name)) {
		$$function_module{$forward_internal_name} .= " & $module";
	    }
	}
    }

    return $self;
}

sub win16api() {
    return $win16api;
}

sub win32api() {
    return $win32api;
}

sub parse_api_file($$) {
    my $self = shift;

    my $allowed_kind = \%{$self->{ALLOWED_KIND}};
    my $allowed_modules = \%{$self->{ALLOWED_MODULES}};
    my $allowed_modules_limited = \%{$self->{ALLOWED_MODULES_LIMITED}};
    my $allowed_modules_unlimited = \%{$self->{ALLOWED_MODULES_UNLIMITED}};
    my $translate_argument = \%{$self->{TRANSLATE_ARGUMENT}};
    my $type_format = \%{$self->{TYPE_FORMAT}};

    my $file = shift;

    my $module;
    my $kind;
    my $format;
    my $forbidden = 0;

    $output->lazy_progress("$file");

    open(IN, "< $winapi_dir/$file") || die "$winapi_dir/$file: $!\n";
    $/ = "\n";
    my $linenum=0;
    while(<IN>) {
        $linenum++;
	s/^\s*?(.*?)\s*$/$1/; # remove whitespace at begin and end of line
	s/^(.*?)\s*#.*$/$1/;  # remove comments
	/^$/ && next;         # skip empty lines

	if(/^%%(\S+)$/) {
	    $module = $1;
	    $module =~ s/\.dll$//; # FIXME: Kludge
	} elsif(!$modules->is_allowed_module($module)) {
	    # Nothing
	} elsif(s/^%(\S+)\s*//) {
	    $kind = $1;
	    $format = undef;
	    $forbidden = 0;

	    $$allowed_kind{$kind} = 1;
	    if(/^--forbidden/) {
		$forbidden = 1;
	    } elsif(/^--format=(\".*?\"|\S*)/) {
		$format = $1;
		$format =~ s/^\"(.*?)\"$/$1/;
	    }

	    if(!defined($format)) {
		if($kind eq "long") {
		    $format  = "%d|%u|%x|%X|";
		    $format .= "%hd|%hu|%hx|%hX|";
		    $format .= "%ld|%lu|%lx|%lX|";
		    $format .= "%04x|%04X|0x%04x|0x%04X|";
		    $format .= "%08x|%08X|0x%08x|0x%08X|";
                    $format .= "%08lx|%08lX|0x%08lx|0x%08lX";
		} elsif($kind eq "longlong") {
		    $format = "%lld";
		} elsif($kind eq "ptr") {
		    $format = "%p";
		} elsif($kind eq "segptr") {
		    $format = "%p";
		} elsif($kind eq "str") {
		    $format = "%p|%s";
		} elsif($kind eq "wstr") {
		    $format = "%p|%s";
		} elsif($kind eq "word") {
		    $format  = "%d|%u|%x|%X|";
		    $format .= "%hd|%hu|%hx|%hX|";
		    $format .= "%04x|%04X|0x%04x|0x%04X";
		} else {
		    $format = "<unknown>";
		}
	    }
	} elsif(defined($kind)) {
	    my $type = $_;
            if ($type =~ /\blong\b/)
            {
                $output->write("$file:$linenum: type ($type) is not Win64 compatible\n");
            }
	    if(!$forbidden) {
		if(defined($module)) {
		    if($$allowed_modules_unlimited{$type}) {
			$output->write("$file:$linenum: type ($type) already specified as an unlimited type\n");
		    } elsif(!$$allowed_modules{$type}{$module}) {
			$$allowed_modules{$type}{$module} = 1;
			$$allowed_modules_limited{$type} = 1;
		    } else {
			$output->write("$file:$linenum: type ($type) already specified\n");
		    }
		} else {
		    $$allowed_modules_unlimited{$type} = 1;
		}
	    } else {
		$$allowed_modules_limited{$type} = 1;
	    }
	    if(defined($$translate_argument{$type}) && $$translate_argument{$type} ne $kind) {
		$output->write("$file:$linenum: type ($type) respecified as different kind ($kind != $$translate_argument{$type})\n");
	    } else {
		$$translate_argument{$type} = $kind;
	    }

	    $$type_format{$module}{$type} = $format;
	} else {
	    $output->write("$file:$linenum: file must begin with %<type> statement\n");
	    exit 1;
	}
    }
    close(IN);
}

sub parse_spec_file($$) {
    my $self = shift;

    my $function_internal_arguments = \%{$self->{FUNCTION_INTERNAL_ARGUMENTS}};
    my $function_external_arguments = \%{$self->{FUNCTION_EXTERNAL_ARGUMENTS}};
    my $function_internal_ordinal = \%{$self->{FUNCTION_INTERNAL_ORDINAL}};
    my $function_external_ordinal = \%{$self->{FUNCTION_EXTERNAL_ORDINAL}};
    my $function_internal_calling_convention = \%{$self->{FUNCTION_INTERNAL_CALLING_CONVENTION}};
    my $function_external_calling_convention = \%{$self->{FUNCTION_EXTERNAL_CALLING_CONVENTION}};
    my $function_internal_name = \%{$self->{FUNCTION_INTERNAL_NAME}};
    my $function_external_name = \%{$self->{FUNCTION_EXTERNAL_NAME}};
    my $function_forward = \%{$self->{FUNCTION_FORWARD}};
    my $function_internal_module = \%{$self->{FUNCTION_INTERNAL_MODULE}};
    my $function_external_module = \%{$self->{FUNCTION_EXTERNAL_MODULE}};
    my $function_wine_extension = \%{$self->{FUNCTION_WINE_EXTENSION}};
    my $modules = \%{$self->{MODULES}};
    my $module_files = \%{$self->{MODULE_FILES}};
    my $module_external_calling_convention = \%{$self->{MODULE_EXTERNAL_CALLING_CONVENTION}};

    my $file = shift;
    $file =~ s%^\./%%;

    my %ordinals;
    my $module;
    my $wine_extension = 0;

    $output->lazy_progress("$file");

    $module = $file;
    $module =~ s/^.*?([^\/]*)\.spec$/$1/;

    open(IN, "< $file") || die "$file: $!\n";
    $/ = "\n";
    my $header = 1;
    my $lookahead = 0;
    while($lookahead || defined($_ = <IN>)) {
	$lookahead = 0;
	s/^\s*(.*?)\s*$/$1/;
	if(s/^(.*?)\s*\#\s*(.*)\s*$/$1/) {
	    my $comment = $2;
	    if ($comment =~ /^Wine/i) { # FIXME: Kludge
		$wine_extension = 1;
	    }
	}
	/^$/ && next;

	if($header)  {
	    if(/^\d+|@/) { $header = 0; $lookahead = 1; }
	    next;
	}

	my $ordinal;
	my $ARCHES="arm|arm64|i386|powerpc|win32|win64|x86_64";
	if(/^(\d+|@)\s+
	   (cdecl|pascal|stdcall|varargs|thiscall)\s+
	   ((?:(?:-arch=!?(?:$ARCHES)(?:,(?:$ARCHES))*|-import|-noname|-norelay|-ordinal|-i386|-ret16|-ret64|-fastcall|-register|-interrupt|-private)\s+)*)(\S+)\s*\(\s*(.*?)\s*\)\s*(\S*)$/x)
	{
	    my $calling_convention = $2;
	    my $flags = $3;
	    my $external_name = $4;
	    my $arguments = $5;
	    my $internal_name = $6;

	    $ordinal = $1;

	    $flags =~ s/\s+/ /g;

            if (!$internal_name)
            {
                $internal_name = ($flags =~ /-register/ ? "__regs_" : "") . $external_name;
            }

	    if($flags =~ /-noname/) {
		# $external_name = "@";
	    }

	    if($flags =~ /(?:-register|-interrupt)/) {
		if($arguments) { $arguments .= " "; }
		$arguments .= "ptr";
		$calling_convention .= " -register";
	    }
	    if($flags =~ /(?:-i386)/) {
		$calling_convention .= " -i386";
	    }

	    if ($internal_name =~ /^(.*?)\.(.*?)$/) {
		my $forward_module = lc($1);
		my $forward_name = $2;
		$calling_convention = "forward";
		$$function_forward{$module}{$external_name} = [$forward_module, $forward_name];
	    }

            if($external_name ne "@") {
                $$module_external_calling_convention{$module}{$external_name} = $calling_convention;
            } else {
                $$module_external_calling_convention{$module}{"\@$ordinal"} = $calling_convention;
            }
	    if(!$$function_internal_name{$external_name}) {
		$$function_internal_name{$external_name} = $internal_name;
	    } else {
		$$function_internal_name{$external_name} .= " & $internal_name";
	    }
	    if(!$$function_external_name{$internal_name}) {
		$$function_external_name{$internal_name} = $external_name;
	    } else {
		$$function_external_name{$internal_name} .= " & $external_name";
	    }
	    $$function_internal_arguments{$internal_name} = $arguments;
	    $$function_external_arguments{$external_name} = $arguments;
	    if(!$$function_internal_ordinal{$internal_name}) {
		$$function_internal_ordinal{$internal_name} = $ordinal;
	    } else {
		$$function_internal_ordinal{$internal_name} .= " & $ordinal";
	    }
	    if(!$$function_external_ordinal{$external_name}) {
		$$function_external_ordinal{$external_name} = $ordinal;
	    } else {
		$$function_external_ordinal{$external_name} .= " & $ordinal";
	    }
	    $$function_internal_calling_convention{$internal_name} = $calling_convention;
	    $$function_external_calling_convention{$external_name} = $calling_convention;
	    if(!$$function_internal_module{$internal_name}) {
		$$function_internal_module{$internal_name} = "$module";
	    } else {
		$$function_internal_module{$internal_name} .= " & $module";
	    }
	    if(!$$function_external_module{$external_name}) {
		$$function_external_module{$external_name} = "$module";
	    } else {
		$$function_external_module{$external_name} .= " & $module";
	    }
	    $$function_wine_extension{$module}{$external_name} = $wine_extension;

	    if(0 && $options->spec_mismatch) {
		if($external_name eq "@") {
		    if($internal_name !~ /^\U$module\E_$ordinal$/) {
			$output->write("$file: $external_name: the internal name ($internal_name) mismatch\n");
		    }
		} else {
		    my $name = $external_name;

		    my $name1 = $name;
		    $name1 =~ s/^Zw/Nt/;

		    my $name2 = $name;
		    $name2 =~ s/^(?:_|Rtl|k32|K32)//;

		    my $name3 = $name;
		    $name3 =~ s/^INT_Int[0-9a-f]{2}Handler$/BUILTIN_DefaultIntHandler/;

		    my $name4 = $name;
		    $name4 =~ s/^(VxDCall)\d$/$1/;

		    # FIXME: This special case is because of a very ugly kludge that should be fixed IMHO
		    my $name5 = $name;
		    $name5 =~ s/^(.*?16)_(.*?)$/$1_fn$2/;

		    if(uc($internal_name) ne uc($external_name) &&
		       $internal_name !~ /(\Q$name\E|\Q$name1\E|\Q$name2\E|\Q$name3\E|\Q$name4\E|\Q$name5\E)/)
		    {
			$output->write("$file: $external_name: internal name ($internal_name) mismatch\n");
		    }
		}
	    }
	} elsif(/^(\d+|@)\s+stub(?:\s+(-arch=(?:$ARCHES)(?:,(?:$ARCHES))*|-noname|-norelay|-ordinal|-i386|-ret16|-ret64|-private))*\s+(\S+?)\s*(\(\s*(.*?)\s*\))?$/) {
	    $ordinal = $1;

	    my $flags = $2;
	    my $external_name = $3;

	    $flags = "" if !defined($flags);

	    if($flags =~ /-noname/) {
		# $external_name = "@";
	    }

	    my $internal_name = $external_name;

            if ($external_name ne "@") {
                $$module_external_calling_convention{$module}{$external_name} = "stub";
            } else {
                $$module_external_calling_convention{$module}{"\@$ordinal"} = "stub";
            }
	    if(!$$function_internal_name{$external_name}) {
		$$function_internal_name{$external_name} = $internal_name;
	    } else {
		$$function_internal_name{$external_name} .= " & $internal_name";
	    }
	    if(!$$function_external_name{$internal_name}) {
		$$function_external_name{$internal_name} = $external_name;
	    } else {
		$$function_external_name{$internal_name} .= " & $external_name";
	    }
	    if(!$$function_internal_ordinal{$internal_name}) {
		$$function_internal_ordinal{$internal_name} = $ordinal;
	    } else {
		$$function_internal_ordinal{$internal_name} .= " & $ordinal";
	    }
	    if(!$$function_external_ordinal{$external_name}) {
		$$function_external_ordinal{$external_name} = $ordinal;
	    } else {
		$$function_external_ordinal{$external_name} .= " & $ordinal";
	    }
	    if(!$$function_internal_module{$internal_name}) {
		$$function_internal_module{$internal_name} = "$module";
	    } else { # if($$function_internal_module{$internal_name} !~ /$module/) {
		$$function_internal_module{$internal_name} .= " & $module";
	    }
	    if(!$$function_external_module{$external_name}) {
		$$function_external_module{$external_name} = "$module";
	    } else { # if($$function_external_module{$external_name} !~ /$module/) {
		$$function_external_module{$external_name} .= " & $module";
	    }
	} elsif(/^(\d+|@)\s+extern(?:\s+(?:-arch=(?:$ARCHES)(?:,(?:$ARCHES))*|-noname|-norelay|-ordinal|-i386|-ret16|-ret64))*\s+(\S+)\s*(\S*)$/) {
	    $ordinal = $1;

	    my $external_name = $2;
	    my $internal_name = $3;

	    $internal_name = $external_name if !$internal_name;

            if ($external_name ne "@") {
                $$module_external_calling_convention{$module}{$external_name} = "extern";
            } else {
                $$module_external_calling_convention{$module}{"\@$ordinal"} = "extern";
            }
	} elsif(/^(?:\d+|@)\s+(?:equate|variable)/) {
	    # ignore
	} else {
	    my $next_line = <IN>;
	    if(!defined($next_line) || $next_line =~ /^\s*\d|@/) {
		die "$file: $.: syntax error: '$_'\n";
	    } else {
		$_ .= $next_line;
		$lookahead = 1;
	    }
	}

	if(defined($ordinal)) {
	    if($ordinal ne "@" && $ordinals{$ordinal}) {
		$output->write("$file: ordinal redefined: $_\n");
	    }
	    $ordinals{$ordinal}++;
	}
    }
    close(IN);

    $$modules{$module}++;

    $$module_files{$module} = $file;
}

sub name($) {
    my $self = shift;
    my $name = \${$self->{NAME}};

    return $$name;
}

sub is_allowed_kind($$) {
    my $self = shift;
    my $allowed_kind = \%{$self->{ALLOWED_KIND}};

    my $kind = shift;
    if(defined($kind)) {
	return $$allowed_kind{$kind};
    } else {
	return 0;
    }

}

sub allow_kind($$) {
    my $self = shift;
    my $allowed_kind = \%{$self->{ALLOWED_KIND}};

    my $kind = shift;

    $$allowed_kind{$kind}++;
}

sub is_limited_type($$) {
    my $self = shift;
    my $allowed_modules_limited = \%{$self->{ALLOWED_MODULES_LIMITED}};

    my $type = shift;

    return $$allowed_modules_limited{$type};
}

sub is_allowed_type_in_module($$) {
    my $self = shift;
    my $allowed_modules = \%{$self->{ALLOWED_MODULES}};
    my $allowed_modules_limited = \%{$self->{ALLOWED_MODULES_LIMITED}};

    my $type = shift;
    my @modules = split(/ \& /, shift);

    if(!$$allowed_modules_limited{$type}) { return 1; }

    foreach my $module (@modules) {
	if($$allowed_modules{$type}{$module}) { return 1; }
    }

    return 0;
}

sub allow_type_in_module($$) {
    my $self = shift;
    my $allowed_modules = \%{$self->{ALLOWED_MODULES}};

    my $type = shift;
    my @modules = split(/ \& /, shift);

    foreach my $module (@modules) {
	$$allowed_modules{$type}{$module}++;
    }
}

sub type_used_in_module($$) {
    my $self = shift;
    my $used_modules = \%{$self->{USED_MODULES}};

    my $type = shift;
    my @modules = split(/ \& /, shift);

    foreach my $module (@modules) {
	$$used_modules{$type}{$module} = 1;
    }

    return ();
}

sub types_not_used($) {
    my $self = shift;
    my $used_modules = \%{$self->{USED_MODULES}};
    my $allowed_modules = \%{$self->{ALLOWED_MODULES}};

    my $not_used;
    foreach my $type (sort(keys(%$allowed_modules))) {
	foreach my $module (sort(keys(%{$$allowed_modules{$type}}))) {
	    if(!$$used_modules{$type}{$module}) {
		$$not_used{$module}{$type} = 1;
	    }
	}
    }
    return $not_used;
}

sub types_unlimited_used_in_modules($) {
    my $self = shift;

    my $used_modules = \%{$self->{USED_MODULES}};
    my $allowed_modules = \%{$self->{ALLOWED_MODULES}};
    my $allowed_modules_unlimited = \%{$self->{ALLOWED_MODULES_UNLIMITED}};

    my $used_types;
    foreach my $type (sort(keys(%$allowed_modules_unlimited))) {
	my $count = 0;
	my @modules = ();
	foreach my $module (sort(keys(%{$$used_modules{$type}}))) {
	    $count++;
	    push @modules, $module;
	}
        if($count) {
	    foreach my $module (@modules) {
	      $$used_types{$type}{$module} = 1;
	    }
	}
    }
    return $used_types;
}

sub translate_argument($$) {
    my $self = shift;
    my $translate_argument = \%{$self->{TRANSLATE_ARGUMENT}};

    my $type = shift;

    return $$translate_argument{$type};
}

sub declare_argument($$$) {
    my $self = shift;
    my $translate_argument = \%{$self->{TRANSLATE_ARGUMENT}};

    my $type = shift;
    my $kind = shift;

    $$translate_argument{$type} = $kind;
}

sub all_declared_types($) {
    my $self = shift;
    my $translate_argument = \%{$self->{TRANSLATE_ARGUMENT}};

    return sort(keys(%$translate_argument));
}

sub is_allowed_type_format($$$$) {
    my $self = shift;
    my $type_format = \%{$self->{TYPE_FORMAT}};

    my $module = shift;
    my $type = shift;
    my $format = shift;

    my $formats;

    if(defined($module) && defined($type)) {
	local $_;
	foreach (split(/ & /, $module)) {
	    if(defined($formats)) {
		$formats .= "|";
	    } else {
		$formats = "";
	    }
	    if(defined($$type_format{$_}{$type})) {
		$formats .= $$type_format{$_}{$type};
	    }
	}
    }

    if(defined($formats)) {
	local $_;
	foreach (split(/\|/, $formats)) {
	    if($_ eq $format) {
		return 1;
	    }
	}
    }

    return 0;
}

sub all_modules($) {
    my $self = shift;
    my $modules = \%{$self->{MODULES}};

    return sort(keys(%$modules));
}

sub is_module($$) {
    my $self = shift;
    my $modules = \%{$self->{MODULES}};

    my $name = shift;

    return $$modules{$name};
}

sub module_file($$) {
    my $self = shift;

    my $module = shift;

    my $module_files = \%{$self->{MODULE_FILES}};

    return $$module_files{$module};
}

sub all_internal_functions($) {
    my $self = shift;
    my $function_internal_calling_convention = \%{$self->{FUNCTION_INTERNAL_CALLING_CONVENTION}};

    return sort(keys(%$function_internal_calling_convention));
}

sub all_internal_functions_in_module($$) {
    my $self = shift;
    my $function_internal_calling_convention = \%{$self->{FUNCTION_INTERNAL_CALLING_CONVENTION}};
    my $function_internal_module = \%{$self->{FUNCTION_INTERNAL_MODULE}};

    my $module = shift;

    my @names;
    foreach my $name (keys(%$function_internal_calling_convention)) {
	if($$function_internal_module{$name} eq $module) {
	    push @names, $name;
	}
    }

    return sort(@names);
}

sub all_external_functions($) {
    my $self = shift;
    my $function_external_name = \%{$self->{FUNCTION_EXTERNAL_NAME}};

    return sort(keys(%$function_external_name));
}

sub all_external_functions_in_module($$) {
    my $self = shift;
    my $function_external_name = \%{$self->{FUNCTION_EXTERNAL_NAME}};
    my $function_external_module = \%{$self->{FUNCTION_EXTERNAL_MODULE}};

    my $module = shift;

    my @names;
    foreach my $name (keys(%$function_external_name)) {
	if($$function_external_module{$name} eq $module) {
	    push @names, $name;
	}
    }

    return sort(@names);
}

sub all_functions_in_module($$) {
    my $self = shift;
    my $module_external_calling_convention = \%{$self->{MODULE_EXTERNAL_CALLING_CONVENTION}};

    my $module = shift;

    return sort(keys(%{$$module_external_calling_convention{$module}}));
}

sub all_broken_forwards($) {
    my $self = shift;
    my $function_forward = \%{$self->{FUNCTION_FORWARD}};

    my @broken_forwards = ();
    foreach my $module (sort(keys(%$function_forward))) {
	foreach my $external_name (sort(keys(%{$$function_forward{$module}}))) {
	    (my $forward_module, my $forward_external_name) = @{$$function_forward{$module}{$external_name}};

	    my $forward_external_calling_convention =
		$self->function_external_calling_convention_in_module($forward_module, $forward_external_name);

	    if(!defined($forward_external_calling_convention)) {
		push @broken_forwards, [$module, $external_name, $forward_module, $forward_external_name];
	    }
	}
    }
    return @broken_forwards;
}


sub function_internal_ordinal($$) {
    my $self = shift;
    my $function_internal_ordinal = \%{$self->{FUNCTION_INTERNAL_ORDINAL}};

    my $name = shift;

    return $$function_internal_ordinal{$name};
}

sub function_external_ordinal($$) {
    my $self = shift;
    my $function_external_ordinal = \%{$self->{FUNCTION_EXTERNAL_ORDINAL}};

    my $name = shift;

    return $$function_external_ordinal{$name};
}

sub function_internal_calling_convention($$) {
    my $self = shift;
    my $function_internal_calling_convention = \%{$self->{FUNCTION_INTERNAL_CALLING_CONVENTION}};

    my $name = shift;

    return $$function_internal_calling_convention{$name};
}

sub function_external_calling_convention($$) {
    my $self = shift;
    my $function_external_calling_convention = \%{$self->{FUNCTION_EXTERNAL_CALLING_CONVENTION}};

    my $name = shift;

    return $$function_external_calling_convention{$name};
}

sub function_external_calling_convention_in_module($$$) {
    my $self = shift;
    my $module_external_calling_convention = \%{$self->{MODULE_EXTERNAL_CALLING_CONVENTION}};

    my $module = shift;
    my $name = shift;

    return $$module_external_calling_convention{$module}{$name};
}

sub function_internal_name($$) {
    my $self = shift;
    my $function_internal_name = \%{$self->{FUNCTION_INTERNAL_NAME}};

    my $name = shift;

    return $$function_internal_name{$name};
}

sub function_external_name($$) {
    my $self = shift;
    my $function_external_name = \%{$self->{FUNCTION_EXTERNAL_NAME}};

    my $name = shift;

    return $$function_external_name{$name};
}

sub function_forward_final_destination($$$) {
    my $self = shift;

    my $function_forward = \%{$self->{FUNCTION_FORWARD}};

    my $module = shift;
    my $name = shift;

    my $forward_module = $module;
    my $forward_name = $name;
    while(defined(my $forward = $$function_forward{$forward_module}{$forward_name})) {
	($forward_module, $forward_name) = @$forward;
    }

    return ($forward_module, $forward_name);
}

sub is_function($$) {
    my $self = shift;
    my $function_internal_calling_convention = \%{$self->{FUNCTION_INTERNAL_CALLING_CONVENTION}};

    my $name = shift;

    return $$function_internal_calling_convention{$name};
}

sub all_shared_internal_functions($$) {
    my $self = shift;
    my $function_shared = \%{$self->{FUNCTION_SHARED}};

    return sort(keys(%$function_shared));
}

sub is_shared_internal_function($$) {
    my $self = shift;
    my $function_shared = \%{$self->{FUNCTION_SHARED}};

    my $name = shift;

    return $$function_shared{$name};
}

sub found_shared_internal_function($$) {
    my $self = shift;
    my $function_shared = \%{$self->{FUNCTION_SHARED}};

    my $name = shift;

    $$function_shared{$name} = 1;
}

sub function_internal_arguments($$) {
    my $self = shift;
    my $function_internal_arguments = \%{$self->{FUNCTION_INTERNAL_ARGUMENTS}};

    my $name = shift;

    return $$function_internal_arguments{$name};
}

sub function_external_arguments($$) {
    my $self = shift;
    my $function_external_arguments = \%{$self->{FUNCTION_EXTERNAL_ARGUMENTS}};

    my $name = shift;

    return $$function_external_arguments{$name};
}

sub function_internal_module($$) {
    my $self = shift;
    my $function_internal_module = \%{$self->{FUNCTION_INTERNAL_MODULE}};

    my $name = shift;

    return $$function_internal_module{$name};
}

sub function_external_module($$) {
    my $self = shift;
    my $function_external_module = \%{$self->{FUNCTION_EXTERNAL_MODULE}};

    my $name = shift;

    return $$function_external_module{$name};
}

sub function_wine_extension($$$) {
    my $self = shift;
    my $function_wine_extension = \%{$self->{FUNCTION_WINE_EXTENSION}};

    my $module = shift;
    my $name = shift;

    return $$function_wine_extension{$module}{$name};
}

sub is_function_stub($$$) {
    my $self = shift;
    my $module_external_calling_convention = \%{$self->{MODULE_EXTERNAL_CALLING_CONVENTION}};

    my $module = shift;
    my $name = shift;

    if($$module_external_calling_convention{$module}{$name} eq "stub") {
	return 1;
    }
    return 0;
}

sub is_function_stub_in_module($$$) {
    my $self = shift;
    my $module_external_calling_convention = \%{$self->{MODULE_EXTERNAL_CALLING_CONVENTION}};

    my $module = shift;
    my $name = shift;

    if(!defined($$module_external_calling_convention{$module}{$name})) {
	return 0;
    }
    return $$module_external_calling_convention{$module}{$name} eq "stub";
}

########################################################################
# class methods
#

sub _get_all_module_internal_ordinal($$) {
    my $winapi = shift;
    my $internal_name = shift;

    my @entries = ();

    my @name = (); {
	my $name = $winapi->function_external_name($internal_name);
	if(defined($name)) {
	    @name = split(/ & /, $name);
	}
    }

    my @module = (); {
	my $module = $winapi->function_internal_module($internal_name);
	if(defined($module)) {
	    @module = split(/ & /, $module);
	}
    }

    my @ordinal = (); {
	my $ordinal = $winapi->function_internal_ordinal($internal_name);
	if(defined($ordinal)) {
	    @ordinal = split(/ & /, $ordinal);
	}
    }

    my $name;
    my $module;
    my $ordinal;
    while(defined($name = shift @name) &&
	  defined($module = shift @module) &&
	  defined($ordinal = shift @ordinal))
    {
	push @entries, [$name, $module, $ordinal];
    }

    return @entries;
}

sub get_all_module_internal_ordinal16($) {
    return _get_all_module_internal_ordinal($win16api, $_[0]);
}

sub get_all_module_internal_ordinal32($) {
    return _get_all_module_internal_ordinal($win32api, $_[0]);
}

sub get_all_module_internal_ordinal($) {
    my @entries = ();
    foreach my $winapi (@winapis) {
	push @entries, _get_all_module_internal_ordinal($winapi, $_[0]);
    }

    return @entries;
}

sub _get_all_module_external_ordinal($$) {
    my $winapi = shift;
    my $external_name = shift;

    my @entries = ();

    my @name = (); {
	my $name = $winapi->function_internal_name($external_name);
	if(defined($name)) {
	    @name = split(/ & /, $name);
	}
    }

    my @module = (); {
	my $module = $winapi->function_external_module($external_name);
	if(defined($module)) {
	    @module = split(/ & /, $module);
	}
    }

    my @ordinal = (); {
	my $ordinal = $winapi->function_external_ordinal($external_name);
	if(defined($ordinal)) {
	    @ordinal = split(/ & /, $ordinal);
	}
    }

    my $name;
    my $module;
    my $ordinal;
    while(defined($name = shift @name) &&
	  defined($module = shift @module) &&
	  defined($ordinal = shift @ordinal))
    {
	push @entries, [$name, $module, $ordinal];
    }

    return @entries;
}

sub get_all_module_external_ordinal16($) {
    return _get_all_module_external_ordinal($win16api, $_[0]);
}

sub get_all_module_external_ordinal32($) {
    return _get_all_module_external_ordinal($win32api, $_[0]);
}

sub get_all_module_external_ordinal($) {
    my @entries = ();
    foreach my $winapi (@winapis) {
	push @entries, _get_all_module_external_ordinal($winapi, $_[0]);
    }

    return @entries;
}

1;
