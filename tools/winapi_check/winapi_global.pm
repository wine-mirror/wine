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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

package winapi_global;

use strict;

use nativeapi qw($nativeapi);
use options qw($options);
use output qw($output);
use winapi qw($win16api $win32api @winapis);

sub check {
    my $type_found = shift;

    if($options->win16) {
	_check($win16api, $type_found);
    }

    if($options->win32) {
	_check($win32api, $type_found);
    }
}

sub _check {
    my $winapi = shift;
    my $type_found = shift;

    my $winver = $winapi->name;

    if($options->argument) {
	foreach my $type ($winapi->all_declared_types) {
	    if(!$$type_found{$type} && !$winapi->is_limited_type($type) && $type ne "CONTEXT86 *") {
		$output->write("*.c: $winver: ");
		$output->write("type ($type) not used\n");
	    }
	}
    }

    if($options->argument && $options->argument_forbidden) {
	my $types_used = $winapi->types_unlimited_used_in_modules;

	foreach my $type (sort(keys(%$types_used))) {
	    $output->write("*.c: type ($type) only used in module[s] (");
	    my $count = 0;
	    foreach my $module (sort(keys(%{$$types_used{$type}}))) {
		if($count++) { $output->write(", "); }
		$output->write("$module");
	    }
	    $output->write(")\n");
	}
    }
}

1;

