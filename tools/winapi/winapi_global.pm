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

package winapi_global;

use strict;
use warnings 'all';

use modules qw($modules);
use nativeapi qw($nativeapi);
use options qw($options);
use output qw($output);
use winapi qw(@winapis);

sub check_modules($$) {
    my $complete_module = shift;
    my $module2functions = shift;

    my @complete_modules = sort(keys(%$complete_module));

    if($options->declared) {
	foreach my $module (@complete_modules) {
	    foreach my $winapi (@winapis) {
		if(!$winapi->is_module($module)) { next; }
		my $functions = $$module2functions{$module};
		foreach my $internal_name ($winapi->all_internal_functions_in_module($module)) {
		    next if $internal_name =~ /\./;
		    my $function = $functions->{$internal_name};
		    if(!defined($function) && !$nativeapi->is_function($internal_name))
		    {
			$output->write("*.c: $module: $internal_name: " .
				       "function declared but not implemented or declared external\n");
		    }
		}
	    }
	}
    }

    if($options->argument && $options->argument_forbidden) {
	foreach my $winapi (@winapis) {
	    my $types_not_used = $winapi->types_not_used;
	    foreach my $module (sort(keys(%$types_not_used))) {
		if(!$$complete_module{$module}) { next; }
		foreach my $type (sort(keys(%{$$types_not_used{$module}}))) {
		    $output->write("*.c: $module: type ($type) not used\n");
		}
	    }
	}
    }
}

sub check_all_modules($) {
    my $include2info = shift;

    winapi_documentation::report_documentation();

    if($options->headers_unused && $options->include) {
	foreach my $name (sort(keys(%$include2info))) {
	    if(!$$include2info{$name}{used}) {
		$output->write("*.c: $name: include file is never used\n");
	    }
	}
    }
    
    $modules->global_report;
    $nativeapi->global_report;
}

1;
