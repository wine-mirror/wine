#!/usr/bin/perl -w
##Wine Quick Debug Report Maker Thingy (WQDRMK)
## Copyright (c) 1998-1999 Adam Sacarny jazz@cscweb.net ICQ: 19617831
##Do not say this is yours without my express permisson, or I will
##hunt you down and kill you like the savage animal I am.
##
## Improvements by Gerald Pfeifer <pfeifer@dbai.tuwien.ac.at>
## (c) 2000
##
## A few improovements and updates here and there
## Copyright 2003-2004 Ivan Leo Puoti
##
## This library is free software; you can redistribute it and/or
## modify it under the terms of the GNU Lesser General Public
## License as published by the Free Software Foundation; either
## version 2.1 of the License, or (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public
## License along with this library; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
##
##Changelog:
##August 29, 1999 - Work around for debugger exit (or lack thereof)
##                - Should now put debugging output in correct place
##April 19, 1999 - Much nicer way to select Wine's location
##               - Option to disable creation of a debugging output
##               - Now places debugging output where it was started
##April 4, 1999 - Sanity check for file locations/wine strippedness
##              - Various code cleanups/fixes
##March 21, 1999 - Bash 2.0 STDERR workaround (Thanks Ryan Cumming!)
##March 1, 1999 - Check for stripped build
##February 3, 1999 - Fix to chdir to the program's directory
##February 1, 1999 - Cleaned up code
##January 26, 1999 - Fixed various bugs...
##                 - Made newbie mode easier
##January 25, 1999 - Initial Release
use strict;
sub do_var($) {
	my $var=$_[0];
	$var =~ s/\t//g;
	return $var;
}
open STDERR, ">&SAVEERR"; open STDERR, ">&STDOUT";
$ENV{'SHELL'}="/bin/bash";
my $var0 = qq{
	What is your level of Wine expertise? 1-newbie 2-intermediate 3-advanced

	1 - Makes a debug report as defined in the Wine documentation. Best
	    for new Wine users. If you're not sure what WINEDEBUG is, then
	    use this mode.
	2 - Makes a debug report that is more customizable (Example: you can
	    choose what WINEDEBUG to use). You are asked more questions in
	    this mode. May intimidate newbies.
	3 - Just like 2, but not corner cutting. Assumes you know what you're
	    doing so it leaves out the long descriptions.
};
print do_var($var0)."\n";
my $debuglevel=0;
until ($debuglevel >= 1 and $debuglevel <= 3) {
	print "Enter your level of Wine expertise (1-3): ";
	$debuglevel=<STDIN>;
	chomp $debuglevel;
}

if ($debuglevel < 3) {
	my $var1 = qq{
	This program will make a debug report for Wine developers. It generates
	two files. The first one has everything asked for by the bugreports guide;
	the second has *all* of the debug output, which can go to thousands of
	lines.
	To (hopefully) get the bug fixed, report it to the project
	bug tracking system at http://bugs.winehq.org.
	Attach the first file to the bug description.
	Also include detailed description of the problem. The developers
	might ask you for "the last X lines from the report". If so, just
	provide the output of the following command:
	    gzip -d (output file) | tail -n (X) > outfile
	If you do not want to create one of the files, just specify "no file".
	};
	print do_var($var1);
} elsif ($debuglevel =~ 3) {
	my $var2 = qq{
	This program will output to two files:
	1. Formatted debug report you might want to post to the newsgroup
	2. File with ALL the debug output (It will later be compressed with
	gzip, so leave off the trailing .gz)
	If you do not want to create one of the files, just type in "no file"
	and I'll skip it.
	};
	print do_var($var2);
}

print "\nFilename for the formatted debug report: ";
my $outfile=<STDIN>;
chomp $outfile;
my $var23 = qq{
I don't think you typed in the right filename. Let's try again.
};
while ($outfile =~ /^(\s)*$/) {
	print do_var($var23);
	$outfile=<STDIN>;
	chomp $outfile;
}

print "Filename for full debug output: ";
my $dbgoutfile=<STDIN>;
chomp $dbgoutfile;
while ($dbgoutfile =~ /^(\s)*$/) {
	print do_var($var23);
	$dbgoutfile=<STDIN>;
	chomp $dbgoutfile;
}

my $var31 = qq{
Since you will only be creating the formatted report, I will need a
temporary place to put the full output.
You may not enter "no file" for this.
Enter the filename for the temporary file:
};
my $tmpoutfile;
if ($outfile ne "no file" and $dbgoutfile eq "no file") {
	print do_var($var31);
	$tmpoutfile=<STDIN>;
	chomp $tmpoutfile;
	while (($tmpoutfile =~ /^(\s)*$/) or ($tmpoutfile eq "no file")) {
		print do_var($var23);
		$tmpoutfile=<STDIN>;
		chomp $tmpoutfile;
	}
}

my $whereis=`whereis wine`;
chomp $whereis;
print "\nWhere is your copy of Wine located?\n\n";
$whereis =~ s/^wine\: //;
my @locations = split(/\s/,$whereis);
print "1 - Unlisted (I'll prompt you for a new location\n";
print "2 - Unsure (I'll use #3, that's probably it)\n";
my $i=2;
foreach my $location (@locations) {
	$i++;
	print "$i - $location\n";
}
print "\n";
sub select_wineloc() {
	my $wineloc;
	do
		{
		print "Enter the number that corresponds to Wine's location: ";
		$wineloc=<STDIN>;
		chomp $wineloc;
		}
	while ( ! ( $wineloc >=1 and $wineloc <= 2+@locations ) );
	if ($wineloc == 1) {
		my $var25 = qq{
		Enter the full path to wine (Example: /usr/bin/wine):
		};
		my $var26 = qq{
		Please enter the full path to wine. A full path is the
		directories leading up to a program's location, and then the
		program. For example, if you had the program "wine" in the
		directory "/usr/bin", you would type in "/usr/bin/wine". Now
		try:
		};
		print do_var($var25) if $debuglevel == 3;
		print do_var($var26) if $debuglevel < 3;
		$wineloc=<STDIN>;
		chomp $wineloc;
		while ($wineloc =~ /^(\s)*$/) {
			print do_var($var23);
			$wineloc=<STDIN>;
			chomp $wineloc;
		}
	}
	elsif ($wineloc == 2) {
		$wineloc=$locations[0];
	}
	else {
		$wineloc=$locations[$wineloc-3];
	}
	return $wineloc;
}
my $wineloc=select_wineloc();
print "Checking if $wineloc is stripped...\n";
my $ifstrip = `nm $wineloc 2>&1`;
while ($ifstrip =~ /no symbols/) {
	my $var24 = qq{
	Your wine is stripped! Stripped versions make useless debug reports
	If you have another location of wine that may be used, enter it now.
	Otherwise, hit control-c and download an unstripped (Debug) version, then re-run
	this script.
	};
	print do_var($var24);
	$wineloc=select_wineloc();
	$ifstrip = `nm $wineloc 2>&1`;
}
while ($ifstrip =~ /not recognized/) {
	my $var26 = qq{
	Looks like you gave me something that isn't a Wine binary (It could be a
	text file). Try again.
	};
	print do_var($var26);
	$wineloc=select_wineloc();
	print "Checking if $wineloc is stripped...\n";
	$ifstrip = `nm $wineloc 2>&1`;
}

print "\nWhat version of Windows are you using with Wine?\n\n".
      "0 - None\n".
      "1 - Windows 3.x\n".
      "2 - Windows 95\n".
      "3 - Windows 98\n".
      "4 - Windows ME\n".
      "5 - Windows NT 3.5x\n".
      "6 - Windows NT4.x\n".
      "7 - Windows 2000\n".
      "8 - Windows XP\n".
      "9 - Windows Server 2003\n".
      "10 - Other\n\n";
my $winver;
do
	{
	print "Enter the number that corresponds to your Windows version: ";
	$winver=<STDIN>;
	chomp $winver;
	}
until ($winver >= 0 and $winver <= 10);
if ($winver =~ 0) {
	$winver="None Installed";
} elsif ($winver =~ 1) {
	$winver="Windows 3.x";
} elsif ($winver =~ 2) {
	$winver="Windows 95";
} elsif ($winver =~ 3) {
	$winver="Windows 98";
} elsif ($winver =~ 4) {
	$winver="Windows ME";
} elsif ($winver =~ 5) {
	$winver="Windows NT 3.5x";
} elsif ($winver =~ 6) {
	$winver="Windows NT 4.x";
} elsif ($winver =~ 7) {
	$winver="Windows 2000";
} elsif ($winver =~ 8) {
	$winver="Windows XP";
} elsif ($winver =~ 9) {
	$winver="Windows Server 2003";
} elsif ($winver =~ 10) {
	print "What version of Windows are you using? ";
	$winver=<STDIN>;
	chomp $winver;
}
if ($debuglevel < 3) {
	my $var7 = qq{
	Enter the full path to the program you want to run. Remember what you
	were told before - a full path is the directories leading up to the
	program and then the program's name, like /dos/windows/sol.exe, not
	sol.exe:
	};
	print do_var($var7);
}
if ($debuglevel =~ 3) {
	my $var8 = qq{
	Enter the full path to the program you want to run (Example:
	/dos/windows/sol.exe, NOT sol.exe):
	};
	print do_var($var8);
}
my $program=<STDIN>;
chomp $program;
while ($program =~ /^(\s)*$/) {
	print do_var($var23);
	$program=<STDIN>;
	chomp $program;
}
$program =~ s/\"//g;
my $var9 = qq{
Enter the name, version, and manufacturer of the program (Example:
Netscape Navigator 4.5):
};
print do_var($var9);
my $progname=<STDIN>;
chomp $progname;
my $var10 = qq{
Enter 1 if your program is 16 bit (Windows 3.x), 2 if your program is 32
bit (Windows 95, NT3.x and up), or 3 if you are unsure:
};
print do_var($var10);
my $progbits=<STDIN>;
chomp $progbits;
until ($progbits == 1 or $progbits == 2 or $progbits == 3) {
	print "You must enter 1, 2 or 3!\n";
	$progbits=<STDIN>;
	chomp $progbits
}
if ($progbits =~ 1) {
	$progbits = "Win16";
} elsif ($progbits =~ 2) {
	$progbits = "Win32";
} else {
	$progbits = "Unsure";
}
my $debugopts;
if ($debuglevel > 1) {
	if ($debuglevel =~ 2) {
		my $var11 = qq{
		Enter any extra debug options. Default is +relay - If you don't
		know what options to use, just hit enter, and I'll use those (Example, the
		developer tells you to re-run with WINEDEBUG=+dosfs,+module you would type
		in +dosfs,+module). Hit enter if you're not sure what to do:
		};
		print do_var($var11);
	} elsif ($debuglevel =~ 3) {
		my $var12 = qq{
		Enter any debug options you would like to use. Just enter parts after
		WINEDEBUG. Default is +relay:
		};
		print do_var($var12);
	}
	$debugopts=<STDIN>;
	chomp $debugopts;
	if ($debugopts =~ /--debugmsg /) {
		$debugopts = (split / /,$debugopts)[1];
	}
	if ($debugopts =~ /WINEDEBUG= /) {
		$debugopts = (split / /,$debugopts)[1];
	}
	if ($debugopts =~ /^\s*$/) {
		$debugopts="+relay";
	}
} elsif ($debuglevel =~ 1) {
	$debugopts = "+relay";
}
my $lastnlines;
if ($debuglevel > 1) {
	if ($debuglevel =~ 2) {
		my $var13 = qq{
		How many trailing lines of debugging info do you want to include in the report
		you're going to submit (First file)? If a developer asks you to include
		the last 15000 lines, enter 15000 here. Default is 3000, which is reached by
		pressing enter. (If you're not sure, just hit enter):
		};
		print do_var($var13);
	} elsif ($debuglevel =~ 3) {
		my $var14 = qq{
		Enter how many lines of trailing debugging output you want in your nice
		formatted report. Default is 3000:
		};
		print do_var($var14);
	}
	$lastnlines=<STDIN>;
	chomp $lastnlines;
	if ($lastnlines =~ /^\s*$/) {
	$lastnlines=3000;
	}
} elsif ($debuglevel =~ 1) {
	$lastnlines=3000;
}
my $extraops;
if ($debuglevel > 1) {
	my $var15 = qq{
	Enter any extra options you want to pass to Wine.
	};
	print do_var($var15);
	$extraops=<STDIN>;
	chomp $extraops;
} elsif ($debuglevel =~ 1) {
	$extraops=" ";
}

print "\nEnter the name of your distribution (Example: RedHat 9.0): ";
my $dist=<STDIN>;
chomp $dist;

my $configopts;
if ($debuglevel > 1) {
	if ($debuglevel =~ 2) {
		my $var16 = qq{
		When you ran ./configure to build wine, were there any special options
		you used to do so (Example: --enable-dll)? If you didn't use any special
		options or didn't compile Wine yourself, just hit enter:
		};
		print do_var($var16);
	} elsif ($debuglevel =~ 3) {
		my $var17 = qq{
		Enter any special options you used when running ./configure for Wine
		(Default is none, use if you didn't compile Wine yourself):
		};
		print do_var($var17);
	}
	$configopts=<STDIN>;
	chomp $configopts;
	if ($configopts =~ /^\s*$/) {
	$configopts="None";
	}
} elsif ($debuglevel =~ 1) {
	$configopts="None";
}
my $winever;
if ($debuglevel > 1) {
	if ($debuglevel =~ 2) {
		my $var18 = qq{
		Is your Wine version CVS or from a .tar.gz or RPM file? As in... did you download it
		off a website/ftpsite or did you/have you run cvs on it to update it?
		For CVS: YYYYMMDD, where YYYY is the year (2004), MM is the month (03), and DD
		is the day (09), that you last updated it (Example: 20040309).
		For tar.gz and RPM: Just hit enter and I'll figure out the version for you:
		};
		print do_var($var18);
	} elsif ($debuglevel =~ 3) {
		my $var19 = qq{
		Is your Wine from CVS? Enter the last CVS update date for it here, in
		YYYYMMDD form (If it's from a tarball or RPM, just hit enter):
		};
		print do_var($var19);
	}
	$winever=<STDIN>;
	chomp $winever;
	if ($winever =~ /[0-9]+/) {
		$winever .= " CVS";
	}
	else {
		$winever = `$wineloc -v 2>&1`;
		chomp $winever;
	}
} elsif ($debuglevel =~ 1) {
	$winever=`$wineloc -v 2>&1`;
	chomp $winever;
}
my $gccver=`gcc -v 2>&1`;
$gccver = (split /\n/,$gccver)[1];
chomp $gccver;
my $cpu=`uname -m`;
chomp $cpu;
my $kernelver=`uname -r`;
chomp $kernelver;
my $ostype=`uname -s`;
chomp $ostype;
my $wineneeds=`ldd $wineloc`;
if ($debuglevel < 3) {
	my $var20 = qq{
	OK, now I'm going to run Wine. I will close it for you once the Wine
	debugger comes up. NOTE: You won't see ANY debug messages. Don't
	worry, they are being output to a file. Since there are so many, it's
	not a good idea to have them all output to a terminal (Speed slowdown
	mainly).
	Wine will still run much slower than normal, because there will be so
	many debug messages being output to file.
	};
	print do_var($var20);
} elsif ($debuglevel =~ 3) {
	my $var21 = qq{
	OK, now it's time to run Wine. I will close down Wine for you after
	the debugger is finished doing its thing.
	};
	print do_var($var21);
}
print "Hit enter to start Wine!\n";
<STDIN>;
my $dir=$program;
$dir=~m#(.*)/#;
$dir=$1;
use Cwd;
my $nowdir=getcwd;
chdir($dir);
if (!($outfile =~ /\//) and $outfile ne "no file") {
	$outfile = "$nowdir/$outfile";
}
if (!($dbgoutfile =~ /\//) and $dbgoutfile ne "no file") {
	$dbgoutfile = "$nowdir/$dbgoutfile";
}
if (!($tmpoutfile =~ /\//)) {
	$tmpoutfile = "$nowdir/$tmpoutfile";
}
$SIG{CHLD}=$SIG{CLD}=sub { wait };
my $lastlines;
sub generate_outfile();
if ($dbgoutfile ne "no file") {
	unlink("$dbgoutfile");
	my $pid=fork();
	if ($pid) {
	}
	elsif (defined $pid) {
		close(0);close(1);close(2);
		exec "echo quit | WINEDEBUG=$debugopts $wineloc $extraops \"$program\" > $dbgoutfile 2>&1";
	}
	else {
		die "couldn't fork";
	}
	while (kill(0, $pid)) {
		sleep(5);
		my $last = `tail -n 5 $dbgoutfile | grep Wine-dbg`;
		if ($last =~ /Wine-dbg/) {
			kill "TERM", $pid;
			last;
		}
	}
	if ($outfile ne "no file") {
		$lastlines=`tail -n $lastnlines $dbgoutfile`;
		system("gzip $dbgoutfile");
		generate_outfile();
	}
	else {
		system("gzip $dbgoutfile");
	}
}
elsif ($outfile ne "no file" and $dbgoutfile eq "no file") {
	my $pid=fork();
	if ($pid) {
	}
	elsif (defined $pid) {
		close(0);close(1);close(2);
		exec "echo quit | WINEDEBUG=$debugopts $wineloc $extraops \"$program\" 2>&1| tee $tmpoutfile | tail -n $lastnlines > $outfile";
	}
	else {
		die "couldn't fork";
	}
	print "$outfile $tmpoutfile";
	while (kill(0, $pid)) {
		sleep(5);
		my $last = `tail -n 5 $tmpoutfile | grep Wine-dbg`;
		if ($last =~ /Wine-dbg/) {
			kill "TERM", $pid;
			last;
		}
	}
	unlink($tmpoutfile);
	open(OUTFILE, "$outfile");
	while (<OUTFILE>) {
		$lastlines .= $_;
	}
	close(OUTFILE);
	unlink($outfile);
	generate_outfile();
}
else {
	my $var27 = qq{
	I guess you don't want me to make any debugging output. I'll send
	it to your terminal. This will be a *lot* of output -- hit enter to
	continue, control-c to quit.
	Repeat: this will be a lot of output!
	};
	print do_var($var27);
	<STDIN>;
	system("$wineloc WINEDEBUG=$debugopts $extraops \"$program\"");
}
sub generate_outfile() {
open(OUTFILE,">$outfile");
print OUTFILE <<EOM;
Auto-generated debug report by Wine Quick Debug Report Maker Tool:
WINE Version:                $winever
Windows Version:             $winver
Distribution:                $dist
Kernel Version:              $kernelver
OS Type:                     $ostype
CPU:                         $cpu
GCC Version:                 $gccver
Program:                     $progname
Program Type:                $progbits
Debug Options:               WINEDEBUG=$debugopts
Other Extra Commands Passed: $extraops
Extra ./configure Commands:  $configopts
Wine Dependencies:
$wineneeds
Last $lastnlines lines of debug output follows:
$lastlines
I have a copy of the full debug report, if it is needed.
Thank you!
EOM
}
my $var22 = qq{
Great! We're finished making the debug report. Please go to http://bugs.winehq.org
and enter it as a new bug. Check that nobody has already reported the same bug!
};
my $var28 = qq{
The filename for the formatted report is:
$outfile
};
my $var29 = qq{
The filename for the compressed full debug is:
$dbgoutfile.gz
Note that it is $dbgoutfile.gz, since I compressed it with gzip for you.
};
my $var30 = qq{
If you have any problems with this bug reporting tool,
please submit a bug report to Wine bugtracking system at http://bugs.winehq.org
or tell the Wine newsgroup (comp.emulators.ms-windows.wine).
};
print do_var($var22);
print do_var($var28) if $outfile ne "no file";
print do_var($var29) if $dbgoutfile ne "no file";
print do_var($var30);
