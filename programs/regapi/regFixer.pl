#!/usr/bin/perl

# This script takes as STDIN an output from the Registry 
# (export from regedit.exe) and prefixes every subkey-value 
# pair by their hkey,key data member
#
# Copyright 1999 Sylvain St-Germain
# 

${prefix} = "";
${line}   = "";   

LINE: while(<>) {
  chomp;                    # Get rid of 0x0a
  chop;                     # Get rid of 0x0d

  next LINE if(/^$/);       # This is an empty line

  if( /^\[/ ) {
    ${prefix} = ${_};       # assign the prefix for the forthcomming section
    next LINE;
  }
  s/\\\\/\\/g;              # Still some more substitutions... To fix paths...

  s/^  //;                  # Get rid of the stupid two spaces at the begining
                            # they are there in the case of a multi-line thing

  if (/\\$/) {              # The line ends with '\', it means it is a multi
    s/\\$//;                # line thing, remove it.
    
    ${line} = "${line}${_}";# Add the current line to the line to output
    next LINE;              # process the next line
  }

  ${line} = "${line}${_}";  # Set line to the multi line thing+the current line

  print "${prefix}${line}\n";  
  ${line} = "";             # start over...
}



