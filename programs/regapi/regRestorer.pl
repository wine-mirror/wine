#!/usr/bin/perl

# This script takes as STDIN an output from the regFixer.pl
# and reformat the file as if it would have been exported from the registry
# in the "REGEDIT4" format.
#
# Copyright 1999 Sylvain St-Germain
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

# I do not validate the integrity of the file, I am assuming that 
# the input file comes from the output of regFixer.pl, therefore things 
# should be ok, if they are not, submit a bug

my $curr_key = "";
my $key;
my $value;
my $rest;
my $s;

print "REGEDIT4\n";

LINE: while($s = <>) {
  chomp($s);			    # get rid of new line

  next LINE if($s =~ /^$/);         # this is an empty line

  if ($s =~ /\]$/)                  # only key name on the line
  {
      ($key) = ($s =~ /^\[(.*)\]$/);
      unless ($key)
      {
	  die "Unrecognized string $s";
      }
      if ($key ne $curr_key)
      {
	  $curr_key = $key;
	  print "\n[$key]\n";
      }
  }
  else
  {
      ($key, $value) = ($s =~ /^\[(.*?)\](.+)$/);
      if (!defined($key))
      {
	  die "Unrecognized string $s";
      }
      if ($key ne $curr_key)	   #curr_key might got chopped from regSet.sh
      {
          print "\n[$key]\n";
      }
      print "$value\n"
  }
}
