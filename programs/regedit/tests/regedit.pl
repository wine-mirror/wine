#!/usr/bin/perl -w
#
# This script tests regedit functionality
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
use winetest;

$main::orig_reg = './tests/orig.reg';

test_regedit();

# Imitation of test framework "ok".
# Uncomment when running on Windows without testing framework
#  sub ok($;$)
#  {
#      my ($condition, $message) = @_;
#      if (!$condition)
#      {
#  	die $message;
#      }
#  }

# Checks if the files are equal regardless of the end-of-line encoding.
# Returns 0 if the files are different, otherwise returns 1
# params - list of file names
sub files_are_equal
{
    my @file_names = @_;
    my @files = ();

    die "At least 2 file names expected" unless ($#file_names);

    #compare file contents
    foreach my $file_name (@file_names)
    {
        my $file;
        open($file, "<$file_name") || die "Error! can't open file $file_name";
        push(@files, $file);
    }

    my $first_file = shift(@files);
    my $line1;
    my $line2;
    while ($line1 = <$first_file>)
    {
        $line1 =~ s/\r//;
        chomp($line1);
        foreach my $file (@files)
        {
            $line2 = <$file>;
            $line2 =~ s/\r//;
            chomp($line2);
            return 0 if $line1 ne $line2;
        }
    }
    return 1;
}

#removes all test output files
sub clear_output
{
    unlink "${main::orig_reg}.exported";
    unlink "${main::orig_reg}.exported2";
}

#tests compatibility with regedit
sub test_regedit
{
    my $error_no_file_name = "regedit: No file name is specified";
    my $error_undefined_switch = "regedit: Undefined switch /";
    my $error_no_registry_key = "regedit: No registry key is specified";
    my $error_file_not_found = 'regedit: Can\'t open file "dummy_file_name"';
    my $error_bad_reg_class_name = 'regedit: Incorrect registry class specification in';
    my $error_dont_delete_class = 'regedit: Can\'t delete registry class';

    my $test_reg_key = 'HKEY_CURRENT_USER\Test Regedit';

    my $s;
    my $regedit = -e "./regedit.exe" ? ".\\regedit.exe" : "./regedit";

    #no parameters
    my $command = "$regedit 2>&1";
    $s = qx/$command/;
    ok($?, "regedit.exe return code check");
    ok($s =~ /$error_no_file_name/,
       'Should raise an error on missed file name');

    #ignored parameters
    $command = "$regedit /S /V /R:1.reg /L:ss_ss.reg 2>&1";
    $s = qx/$command/;
    ok($?, "regedit.exe return code check");
    ok($s =~ /$error_no_file_name/,
       'Should raise an error on missed file name');

    #incorrect form for /L, /R parameters
    for my $switch ('L', 'R')
    {
        $command = "$regedit /$switch 2>&1";
        $s = qx/$command/;
        ok($?, "regedit.exe return code check");
        ok($s =~ /$error_undefined_switch/, "Incorrect switch format check");
	#with ':'
        $command = "$regedit /$switch: 2>&1";
        $s = qx/$command/;
        ok($?, "regedit.exe return code check");
        ok($s =~ /$error_no_file_name/, "Incorrect switch format check");
    }

    #file does not exist
    $command = "$regedit dummy_file_name 2>&1";
    $s = qx/$command/;
    ok($?, "regedit.exe return code check");
    ok($s =~ /$error_file_not_found/, 'Incorrect processing of not-existing file');

    #incorrect registry class is specified
    $command = "$regedit /e ${main::orig_reg}.exported \"BAD_CLASS_HKEY\" 2>&1";
    $s = qx/$command/;
    ok($?, "regedit.exe return code check");
    ok($s =~ /$error_bad_reg_class_name/, 'Incorrect processing of not-existing file');

    #import registry file, export registry file, compare the files
    $command = "$regedit ${main::orig_reg} 2>&1";
    $s = qx/$command/;
    ok(!$?, "regedit.exe return code check");
    $command = "$regedit /e ${main::orig_reg}.exported \"$test_reg_key\" 2>&1";
    $s = qx/$command/;
    ok(!$?, "regedit.exe return code check");
    ok(files_are_equal("${main::orig_reg}.exported", $main::orig_reg),
       "Original and generated registry files " .
       "(${main::orig_reg}.exported and ${main::orig_reg}) " .
       "are different");
    clear_output();

    #export bare registry class (2 formats of command line parameter)
    #XXX works fine under wine, but commented out because does not handle all key types
    #existing on Windows and Windows registry is *very* big
#      $command = "$regedit /e ${main::orig_reg}.exported HKEY_CURRENT_USER 2>&1";
#      $s = qx/$command/;
#      print("DEBUG\t result: $s, return code - $?\n");
#      ok(!$?, "regedit.exe return code check");
#      $command = "$regedit /e ${main::orig_reg}.exported2 HKEY_CURRENT_USER 2>&1";
#      qx/$command/;
#      ok(!$?, "regedit.exe return code check");
#      ok(files_are_equal("${main::orig_reg}.exported", "${main::orig_reg}.exported2"),
#         "Original and generated registry files " .
#         "(${main::orig_reg}.exported and ${main::orig_reg}.exported2) " .
#         "are different");

    ##test removal

    #incorrect format
    $command = "$regedit /d 2>&1";
    $s = qx/$command/;
    ok($?, "regedit.exe return code check");
    ok($s =~ /$error_no_registry_key/,
       'No registry key name is specified for removal');

    #try to delete class
    $command = "$regedit /d HKEY_CURRENT_USER 2>&1";
    $s = qx/$command/;
    ok($?, "regedit.exe return code check");
    ok($s =~ /$error_dont_delete_class/, 'Try to remove registry class');

    #try to delete registry key with incorrect name
    $command = "$regedit /d BAD_HKEY 2>&1";
    $s = qx/$command/;
    ok($?, "regedit.exe return code check");

    #should not export anything after removal because the key does not exist
    clear_output();
    ok(!-e("${main::orig_reg}.exported"), "Be sure the file is deleted");
    $command = "$regedit /d \"$test_reg_key\" 2>&1";
    $s = qx/$command/;
    $command = "$regedit /e ${main::orig_reg}.exported \"$test_reg_key\" 2>&1";
    $s = qx/$command/;
    ok(!-e("${main::orig_reg}.exported"),
       "File ${main::orig_reg}.exported should not exist");
    ok($?, "regedit.exe return code check");

    clear_output();
}
