#!/usr/bin/perl -w
#This script tests regapi functionality

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
