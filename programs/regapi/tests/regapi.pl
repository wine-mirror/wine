#!/usr/bin/perl -w
# This script tests regapi functionality
#
# Copyright 2002 Andriy Palamarchuk
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

use strict;
use diagnostics;

test_diff();
#TODO!!!
#test_regedit();

#removes all test output files
sub clear_output
{
    unlink './tests/after.reg.toAdd';
}

#tests scripts which implement "diff" functionality for registry
sub test_diff
{
    my $generated = './tests/after.reg.toAdd';
    my $orig =  './tests/orig.reg';
    my $s;

    $s = './regSet.sh ./tests/before.reg ./tests/after.reg > /dev/null';
    qx/$s/;

    #files must be the same
    if (-z($generated) || (-s($generated) != -s($orig))) {
        die "Original and generated registry files ($orig and $generated) " .
            "are different";
    }
    clear_output();
}

#tests compatibility with regedit
sub test_regedit
{
    my $orig = './tests/orig.reg';
    my $regedit = 'regapi';
    my $delete_cmd;
    my $insert_cmd = "$regedit setValue < $orig";
    my $export_cmd;

    qx/$insert_cmd/;
    print "Insert: $insert_cmd\n";
    clear_output();
}
