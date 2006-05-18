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

package winapi_fixup_documentation;

use strict;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw(fixup_documentation);

use config qw($current_dir $wine_dir);
use modules qw($modules);
use options qw($options);
use output qw($output);
use winapi qw($win16api $win32api @winapis);

my %documentation_line_used;

sub fixup_documentation($$) {
    my $function = shift;
    my $editor = shift;

    my $file = $function->file;
    my $documentation_line = $function->documentation_line;
    my $documentation = $function->documentation;
    my $function_line = $function->function_line;
    my $linkage = $function->linkage;
    my $return_type = $function->return_type;
    my $calling_convention = $function->calling_convention;
    my $internal_name = $function->internal_name;
    my $statements = $function->statements;

    if($linkage eq "static" ||
       ($linkage eq "extern" && !defined($statements)) ||
       ($linkage eq "" && !defined($statements)))
    {
	return;
    }

    my @external_names = $function->external_names;
    if($#external_names < 0) {
	return;
    }

    if($documentation_line_used{$file}{documentation_line}) {
	$documentation = undef;
    }
    $documentation_line_used{$file}{$documentation_line}++;

    my @module_ordinal_entries = ();
    foreach my $entry2 ($function->get_all_module_ordinal) {
	(my $external_name2, my $module2, my $ordinal2) = @$entry2;
	if(($external_name2 eq "@" ||
	    ($win16api->is_module($module2) && !$win16api->is_function_stub_in_module($module2, $external_name2)) ||
	    ($win32api->is_module($module2) && !$win32api->is_function_stub_in_module($module2, $external_name2))) &&
	       $modules->is_allowed_module_in_file($module2, "$current_dir/$file"))
	{
	    push @module_ordinal_entries, $entry2;
	}
    }

    my $spec_modified = 0;

    if($options->stub && defined($documentation)) {
	my $calling_convention16 = $function->calling_convention16;
	my $calling_convention32 = $function->calling_convention32;

	foreach my $winapi (@winapis) {
	    my @entries = ();
	    my $module = $winapi->function_internal_module($internal_name);
	    my $ordinal = $winapi->function_internal_ordinal($internal_name);

	    if($winapi->is_function_stub_in_module($module, $internal_name)) {
		my $external_name = $internal_name;
		if($winapi->name eq "win16") {
		    $external_name =~ s/(?:_)?16([AW]?)$//;
		    if(defined($1)) {
			$external_name .= $1;
		    }
		}
		push @entries, [$external_name, $module, $ordinal];
	    }

	    foreach (split(/\n/, $documentation)) {
		if(/^\s*\*\s*(\S+)\s*[\(\[]\s*(\w+)\s*\.\s*([^\s\)\]]*)\s*[\)\]].*?$/) {
		    my $external_name = $1;
		    my $module = lc($2);
		    my $ordinal = $3;

		    if($external_name ne "@" &&
		       $winapi->is_module($module) &&
		       $winapi->is_function_stub_in_module($module, $external_name) &&
		       $internal_name !~ /^\U$module\E_\Q$external_name\E$/)
		    {
			push @entries, [$external_name, $module, $ordinal];
		    }
		}
	    }

	    foreach my $entry (@entries) {
		(my $external_name, my $module, my $ordinal) = @$entry;

		my $refargument_types = $function->argument_types;

		if(!defined($refargument_types)) {
		    next;
		}

		my $abort = 0;
		my $n;
		my @argument_kinds = map {
		    my $type = $_;
		    my $kind;
		    if($type ne "..." && !defined($kind = $winapi->translate_argument($type))) {
			$output->write("no win*.api translation defined: " . $type . "\n");
		    }

		    # FIXME: Kludge
		    if(defined($kind) && $kind eq "longlong") {
			$n += 2;
			("long", "long");
		    } elsif(defined($kind)) {
			$n++;
			$kind;
		    } elsif($type eq "...") {
			if($winapi->name eq "win16") {
			    $calling_convention16 = "pascal"; # FIXME: Is this correct?
			} else {
			    $calling_convention32 = "varargs";
			}
			();
		    } else {
			$abort = 1;
			$n++;
			"undef";
		    }
		} @$refargument_types;

		my $search = "^\\s*$ordinal\\s+stub\\s+$external_name\\s*(?:#.*?)?\$";
		my $replace;
		if($winapi->name eq "win16") {
		    $replace = "$ordinal $calling_convention16 $external_name(@argument_kinds) $internal_name";
		} else {
		    $replace = "$ordinal $calling_convention32 $external_name(@argument_kinds) $internal_name";
		}

		if(!$abort) {
		    $spec_modified = 1;
		    $editor->replace_spec_file($module, $search, $replace);
		}
	    }
	}
    }

    my %found_external_names;
    foreach my $external_name (@external_names) {
	$found_external_names{$external_name} = {};
    }

    my $documentation_modified = 0;

    if(!$spec_modified &&
       (defined($documentation) && !$documentation_modified) &&
       ($options->documentation_name || $options->documentation_ordinal ||
	$options->documentation_missing))
    {
	local $_;

	my $line3;
	my $search;
	my $replace;

	my $count = 0;
	my $line2 = $documentation_line - 1;
	foreach (split(/\n/, $documentation)) {
	    $line2++;
	    if(/^(\s*\*\s*(\S+)\s*)((?:\s*[\(\[]\s*\w+(?:\s*\.\s*[^\s\)\]]*\s*)?[\)\]])+)(.*?)$/) {
		my $part1 = $1;
		my $external_name = $2;
		my $part3 = $3;
		my $part4 = $4;

		$part4 =~ s/\s*$//;

		my @entries = ();
		while($part3 =~ s/^\s*([\(\[]\s*(\w+)(?:\s*\.\s*([^\s\)\]]*)\s*)?[\)\]])//) {
		    push @entries, [$1, $2, $3];
		}

		my $found = 0;
		foreach my $external_name2 (@external_names) {
		    if($external_name eq $external_name2) {
			foreach my $entry (@entries) {
			    (undef, my $module, undef) = @$entry;
			    $found_external_names{$external_name2}{$module} = 1;
			}
			$found = 1;
			last;
			}
		}

		my $replaced = 0;
		my $replace2 = "";
		foreach my $entry (@entries) {
		    my $part12 = $part1;
		    (my $part32, my $module, my $ordinal) = @$entry;

		    foreach my $entry2 (@module_ordinal_entries) {
			(my $external_name2, my $module2, my $ordinal2) = @$entry2;

			if($options->documentation_name && lc($module) eq $module2 &&
			   $external_name ne $external_name2)
			{
			    if(!$found && $part12 =~ s/\b\Q$external_name\E\b/$external_name2/) {
				$external_name = $external_name2;
				$replaced++;
			    }
			}

			if($options->documentation_ordinal &&
			   $external_name eq $external_name2 &&
			   lc($module) eq $module2 &&
			   ($#entries > 0 || !defined($ordinal) || ($ordinal ne $ordinal2)))
			{
			    if(defined($ordinal)) {
				if($part32 =~ s/\Q$module\E\s*.\s*\Q$ordinal\E/\U$module2\E.$ordinal2/ || $#entries > 0) {
				    $replaced++;
				}
			    } else {
				if($part32 =~ s/\Q$module\E/\U$module2\E.$ordinal2/ || $#entries > 0) {
				    $replaced++;
				}
			    }
			}
		    }
		    if($replace2) { $replace2 .= "\n"; }
		    $replace2 .= "$part12$part32$part4";
		}

		if($replaced > 0) {
		    $line3 = $line2;
		    $search = "^\Q$_\E\$";
		    $replace = $replace2;
		}
		$count++;
	    } elsif(/^(\s*\*\s*)([^\s\(]+)(?:\(\))?\s*$/) {
		my $part1 = $1;
		my $external_name = $2;

		if($internal_name =~ /^(?:\S+_)?\Q$external_name\E(?:16)?$/) {
		    foreach my $entry (@module_ordinal_entries) {
			(my $external_name2, my $module, my $ordinal) = @$entry;

			$line3 = $line2;
			$search = "^\Q$_\E\$";
			$replace = "$part1$external_name2 (\U$module\E.$ordinal)";
		    }
		    $count++;
		}
	    }
	}

	if(defined($line3) && defined($search) && defined($replace)) {
	    if($count > 1 || $#external_names >= 1) {
		$output->write("multiple entries (fixup not supported)\n");
		# $output->write("s/$search/$replace/\n");
		# $output->write("@external_names\n");
	    } else {
		$documentation_modified = 1;
		$editor->substitute_line($line3, $search, $replace);
	    }
	}
    }

    if(!$spec_modified && !$documentation_modified &&
       $options->documentation_missing && defined($documentation))
    {
	my $part1;
	my $part2;
	my $part3;
	my $part4;
	my $line3 = 0;

	my $line2 = $documentation_line - 1;
	foreach (split(/\n/, $documentation)) {
	    $line2++;
	    if(/^(\s*\*\s*)(\S+\s*)([\(\[])\s*\w+\s*\.\s*[^\s\)\]]*\s*([\)\]]).*?$/) {
		$part1 = $1;
		$part2 = $2;
		$part3 = $3;
		$part4 = $4;

		$part2 =~ s/\S/ /g;

		$line3 = $line2 + 1;
	    }
	}

	foreach my $entry2 (@module_ordinal_entries) {
	    (my $external_name2, my $module2, my $ordinal2) = @$entry2;

	    my $found = 0;
	    foreach my $external_name (keys(%found_external_names)) {
		foreach my $module3 (keys(%{$found_external_names{$external_name}})) {
		    if($external_name eq $external_name2 && uc($module2) eq $module3) {
			$found = 1;
		    }
		}
	    }
	    # FIXME: Not 100% correct
	    if(!$found &&
	       !$win16api->is_function_stub_in_module($module2, $internal_name) &&
	       !$win32api->is_function_stub_in_module($module2, $internal_name))
	    {
		if($line3 > 0) {
		    $documentation_modified = 1;
		    $part2 = $external_name2 . " " x (length($part2) - length($external_name2));
		    $editor->insert_line($line3, "$part1$part2$part3\U$module2\E.$ordinal2$part4\n");
		} else {
		    $output->write("$external_name2 (\U$module2\E.$ordinal2) missing (fixup not supported)\n");
		}
	    }
	}
    }

    if(!$documentation_modified &&
       defined($documentation) &&
       $options->documentation_wrong)
    {
	my $line2 = $documentation_line - 1;
	foreach (split(/\n/, $documentation)) {
	    $line2++;
	    if(/^\s*\*\s*(\S+)\s*[\(\[]\s*(\w+)\s*\.\s*([^\s\)\]]*)\s*[\)\]].*?$/) {
		my $external_name = $1;
		my $module = $2;
		my $ordinal = $3;

		my $found = 0;
		foreach my $entry2 (@module_ordinal_entries) {
		    (my $external_name2, my $module2, my $ordinal2) = @$entry2;

		    if($external_name eq $external_name2 &&
		       lc($module) eq $module2 &&
		       $ordinal eq $ordinal2)
		    {
			$found = 1;
		    }
		}
		if(!$found) {
		    if(1) {
			$documentation_modified = 1;

			$editor->delete_line($line2, "^\Q$_\E\$");
		    } else {
			$output->write("$external_name (\U$module\E.$ordinal) wrong (fixup not supported)\n");
		    };
		}
	    }
	}
    }

    if(!$spec_modified && !$documentation_modified && !defined($documentation))
    {
	my $insert = "";
	foreach my $winapi (@winapis) {
	    my $external_name = $winapi->function_external_name($internal_name);
	    my $module = $winapi->function_internal_module($internal_name);
	    my $ordinal = $winapi->function_internal_ordinal($internal_name);

	    if(defined($external_name) && defined($module) && defined($ordinal)) {
		$insert .= " *\t\t$external_name (\U$module\E.$ordinal)\n";
	    }
	}
	if($insert) {
	    $editor->insert_line($function_line,
				 "/" . "*" x 71 . "\n" .
				 "$insert" .
				 " */\n");
	}
    }
}

1;
