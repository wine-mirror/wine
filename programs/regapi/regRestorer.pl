#!/usr/bin/perl

# This script takes as STDIN an output from the regFixer.pl
# and reformat the file as if it would have been exported from the registry
# in the "REGEDIT4" format.
#
# Copyright 1999 Sylvain St-Germain
# 

${newkey} = "";
${key}    = "";   
${data}   = "";   

# I do not validate the integrity of the file, I am assuming that 
# the input file comes from the output of regFixer.pl, therefore things 
# should be ok, if they are not, investigate and send me an email...

print "REGEDIT4\n";

LINE: while(<>) {
  chomp;                             # get rid of new line

  next LINE if(/^$/);                # This is an empty line

  (${key}, ${data}, ${rest})= split /]/,$_ ; # Extract the values from the line

  ${key}  = "${key}]";               # put the ']' back there...

  if (${rest} ne "")                 # concat we had more than 1 ']' 
  {                                  # on the line
    ${data} = "${data}]${rest}"; 
  }

  if (${key} eq ${newkey})
  {
    print "${data}\n";
  }
  else
  {
    print "\n";
    print "$key\n";
    my $s = $data;
    $s =~ s/^\s+//;
    $s =~ s/\s+$//;  
    if ($s ne "")
    {
        $newkey = $key;              # assign it
        print "$data\n";
    }
  }
}
