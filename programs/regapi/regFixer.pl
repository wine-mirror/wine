#!/usr/bin/perl

# This script takes as STDIN an output from the Registry 
# (export from regedit.exe) and prefixes every subkey-value 
# pair by their hkey,key data member
#
# Copyright 1999 Sylvain St-Germain
# 

${prefix} = "";

LINE: while(<>) {
  chomp;
  s/\r$//;                  # Get rid of 0x0a

  next LINE if(/^$/);       # This is an empty line

  if( /^\[/ ) {
    ${prefix} = ${_};       # assign the prefix for the forthcomming section
    next LINE;
  }
  s/\\\\/\\/g;              # Still some more substitutions... To fix paths...

  print "${prefix}$_\n";  
}



