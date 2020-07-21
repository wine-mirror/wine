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

package winapi_documentation;

use strict;
use warnings 'all';

use config qw($current_dir $wine_dir);
use modules qw($modules);
use nativeapi qw($nativeapi);
use options qw($options);
use output qw($output);
use winapi qw($win16api $win32api @winapis);

my %comment_width;
my %comment_indent;
my %comment_spacing;

sub check_documentation($) {
    local $_;

    my $function = shift;

    my $file = $function->file;
    my $external_name16 = $function->external_name16;
    my $external_name32 = $function->external_name32;
    my $internal_name = $function->internal_name;
    my $module16 = $function->module16;
    my $module32 = $function->module32;
    my $ordinal16 = $function->ordinal16;
    my $ordinal32 = $function->ordinal32;
    my $documentation = $function->documentation;
    my $documentation_line = $function->documentation_line;

    my $documentation_error = 0;
    my $documentation_warning = 0;
    if($options->documentation_name ||
       $options->documentation_ordinal ||
       $options->documentation_pedantic)
    {
	my @winapis = ($win16api, $win32api);
	my @modules = ($module16, $module32);
	my @external_names = ($external_name16, $external_name32);
	my @ordinals = ($ordinal16, $ordinal32);
	while(
	      defined(my $winapi = shift @winapis) &&
	      defined(my $external_name = shift @external_names) &&
	      defined(my $module = shift @modules) &&
	      defined(my $ordinal = shift @ordinals))
	{
	    if($winapi->is_function_stub_in_module($module, $internal_name)) { next; }

	    my @external_name = split(/\s*\&\s*/, $external_name);
	    my @modules = split(/\s*\&\s*/, $module);
	    my @ordinals = split(/\s*\&\s*/, $ordinal);

	    my $pedantic_failed = 0;
	    while(defined(my $external_name = shift @external_name) &&
		  defined(my $module = shift @modules) &&
		  defined(my $ordinal = shift @ordinals))
	    {
		my $found_name = 0;
		my $found_ordinal = 0;

		$module = "kernel" if $module eq "krnl386"; # FIXME: Kludge

		foreach (split(/\n/, $documentation)) {
		    if(/^(\s*)\*(\s*)(\@|\S+)(\s*)([\(\[])(\w+(?:\.\w+)?)\.(\@|\d+)([\)\]])/) {
			my $external_name2 = $3;
			my $module2 = $6;
			my $ordinal2 = $7;

			if ($winapi->function_wine_extension(lc($module2), $external_name2)) {
			    # $output->write("documentation: $external_name2 (\U$module2\E.$ordinal2) is a Wine extension \\\n$documentation\n");
			}

			if(length($1) != 1 || length($2) < 1 ||
			   length($4) < 1 || $5 ne "(" || $8 ne ")")
			{
			    $pedantic_failed = 1;
			}

			if($external_name eq $external_name2) {
			    $found_name = 1;
			    if("\U$module\E" eq $module2 &&
			       $ordinal eq $ordinal2)
			    {
				$found_ordinal = 1;
			    }
			}
		    }
		}
		if((($options->documentation_name && !$found_name) ||
		   ($options->documentation_ordinal && !$found_ordinal)) &&
		   !$winapi->is_function_stub($module, $external_name) &&
		   !$winapi->function_wine_extension($module, $external_name))
		{
		    $documentation_error = 1;
		    $output->write("documentation: expected $external_name (\U$module\E.$ordinal): \\\n$documentation\n");
		}

	    }
	    if($options->documentation_pedantic && $pedantic_failed) {
		$documentation_warning = 1;
		$output->write("documentation: pedantic failed: \\\n$documentation\n");
	    }
	}
    }

    if(!$documentation_error && $options->documentation_wrong) {
	foreach (split(/\n/, $documentation)) {
	    if (/^\s*\*\s*(\S+)\s*[\(\[]\s*(\w+(?:\.(?:DRV|EXE|OCX|VXD))?)\s*\.\s*([^\s\)\]]*)\s*[\)\]].*?$/) {
		my $external_name = $1;
		my $module = $2;
		my $ordinal = $3;

		if ($ordinal eq "@") {
		    # Nothing
		} elsif ($ordinal =~ /^\d+$/) {
		    $ordinal = int($ordinal);
		} elsif ($ordinal eq "init") {
		    $ordinal = 0;
		} else {
		    $output->write("documentation: invalid ordinal for $external_name (\U$module\E.$ordinal)\n");
		    next;
		}

		my $found = 0;
		foreach my $entry2 (winapi::get_all_module_internal_ordinal($internal_name)) {
		    (my $external_name2, my $module2, my $ordinal2) = @$entry2;

		    my $_module2 = $module2;
		    $_module2 =~ s/\.(acm|dll|drv|exe|ocx)$//; # FIXME: Kludge
		    $_module2 = "kernel" if $_module2 eq "krnl386"; # FIXME: Kludge

		    if($external_name eq $external_name2 &&
		       lc($module) eq $_module2 &&
		       $ordinal eq $ordinal2 &&
		       ($external_name2 eq "@" ||
			($win16api->is_module($module2) && !$win16api->is_function_stub_in_module($module2, $external_name2)) ||
			($win32api->is_module($module2) && !$win32api->is_function_stub_in_module($module2, $external_name2))) ||
			$modules->is_allowed_module_in_file($module2, "$current_dir/$file"))
		    {
			$found = 1;
			last;
		    }


		}

		if (!$found && $external_name ne "DllMain" && $ordinal ne "0") {
		    $output->write("documentation: $external_name (\U$module\E.$ordinal) not declared in the spec file\n");
		}
	    }
	}
    }

    if($options->documentation_comment_indent) {
	foreach (split(/\n/, $documentation)) {
	    if(/^\s*\*(\s*)\S+(\s*)[\(\[]\s*\w+\s*\.\s*[^\s\)\]]*\s*[\)\]].*?$/) {
		my $indent = $1;
		my $spacing = $2;

		$indent =~ s/\t/        /g;
		$indent = length($indent);

		$spacing =~ s/\t/        /g;
		$spacing = length($spacing);

		$comment_indent{$indent}++;
		if($indent >= 20) {
		    $output->write("documentation: comment indent is $indent\n");
		}
		$comment_spacing{$spacing}++;
	    }
	}
    }

    if($options->documentation_comment_width) {
	if($documentation =~ /(^\/\*\*+)/) {
	    my $width = length($1);

	    $comment_width{$width}++;
	    if($width <= 65 || $width >= 81) {
		$output->write("comment is $width columns wide\n");
	    }
	}
    }

    if($options->documentation_arguments) {
	my $refargument_documentations = $function->argument_documentations;

	if(defined($refargument_documentations)) {
	    my $n = 0;
	    for my $argument_documentation (@$refargument_documentations) {
		$n++;
		if($argument_documentation ne "") {
		    if($argument_documentation !~ /^\/\*\s+\[(?:in|out|in\/out|\?\?\?|I|O|I\/O)\].*?\*\/$/s) {
			$output->write("argument $n documentation: \\\n$argument_documentation\n");
		    }
		}
	    }
	}
    }
}

sub report_documentation() {
    if($options->documentation_comment_indent) {
	foreach my $indent (sort(keys(%comment_indent))) {
	    my $count = $comment_indent{$indent};
	    $output->write("*.c: $count functions have comment that is indented $indent\n");
	}
    }

    if($options->documentation_comment_width) {
	foreach my $width (sort(keys(%comment_width))) {
	    my $count = $comment_width{$width};
	    $output->write("*.c: $count functions have comments of width $width\n");
	}
    }
}

1;
