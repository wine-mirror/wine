#!/usr/bin/perl
##Wine Quick Debug Report Maker Thingy (WQDRMK)
##By Adam the Jazz Guy
##(c) 1998
##Do not say this is yours without my express permisson, or I will
##hunt you down and kill you like the savage animal I am.
##Released under the WINE licence
##Changelog: 
##March 21, 1999 - Bash 2.0 STDERR workaround (Thanks Ryan Cumming!)
##March 1, 1999 - Check for stripped build
##February 3, 1999 - Fix to chdir to the program's directory
##February 1, 1999 - Cleaned up code
##January 26, 1999 - Fixed various bugs...
##                 - Made newbie mode easier
##January 25, 1999 - Initial Release
## -------------------------------------------
##| IRCNET/UNDERNET: jazzfan AOL: Jazzrock12  |
##| E-MAIL: magicbox@bestweb.net ICQ: 19617831|
##|   Utah Jazz Page @ http://www.gojazz.net  |
##|  Wine Builds @ http://www.gojazz.net/wine |
## -------------------------------------------
sub do_var {
	$var=$_[0];
	$var =~ s/\t//g;
	return $var;
}
open STDERR, ">&SAVEERR"; open STDERR, ">&STDOUT"; 
$ENV{'SHELL'}="/bin/bash";
$var0 = qq{
	Enter your level of WINE expertise: 1-newbie 2-intermediate 3-advanced
	
	1 - Makes a debug report as defined in the WINE documentation. Best
	for new WINE users. If you're not sure what -debugmsg is, then use
	this mode.
	2 - Makes a debug report that is more customizable (Example: you can
	choose what -debugmsg 's to use). You are asked more
	questions in this mode. May intimidate newbies.
	3 - Just like 2, but not corner cutting. Assumes you know what you're
	doing so it leaves out the long descriptions.
};
print do_var($var0);
$debuglevel=<STDIN>;
chomp $debuglevel;
until ($debuglevel < 4) {
	print "Enter a number from 1-3!\n"; 
	$debuglevel=<STDIN>;
	chomp $debuglevel;
} 
if ($debuglevel < 3) {
	$var1 = qq{
	This program will make a debug report for WINE developers. It does this
	in two files. The first one has everything asked for by the bugreports
	guide. The second has *all* of the debug output (This can go to
	thousands of lines). To (hopefully) get the bug fixed, attach the first
	file to a messsage sent to the comp.emulators.ms-windows.wine newsgroup.
	The developers might ask you for "the last XX number of lines from the
	report". If so, post the second file (It will be compressed with gzip 
	later, so leave off the .gz). If you feel like it, post both files at the
	same time. I don't care. 
	};
	print do_var($var1);
} elsif ($debuglevel =~ 3) {
	$var2 = qq{
	This program will output to two files:
	1. Formatted debug report you might want to post to the newsgroup
	2. File with ALL the debug output (It will later be compressed with
	gzip, so leave off the trailing .gz)
	};
	print do_var($var2);
}
print "Enter the filename for this debug report (The first file):\n";
$outfile=<STDIN>;
chomp $outfile;
print "Enter the file for the debug output (The second file):\n";
$dbgoutfile=<STDIN>;
chomp $dbgoutfile;
if ($debuglevel =~ 1) {
	print "Looking for wine...\n";
	$wineloc=`which wine`;
	chomp $wineloc;
	if ($wineloc =~ "") {
		print "Couldn't find wine...\n";
		$var3 = qq{
		Enter the full path to wine. The path should look like
		/path/to/wine/wine. Get it? It's the directories leading up to the
		wine file, and then the actual wine file (Example: /home/wine/wine):
		};
		print do_var($var3);
	$wineloc=<STDIN>;
	chomp $wineloc; 
	} else {
		print "Found wine: $wineloc\n"
	} 
}
if ($debuglevel > 1) {
	if ($debuglevel =~ 2) {
		$var4 = qq{
		Enter the full path to wine. The path should look like
		/path/to/wine/wine. Get it? It's the directories leading up to the
		wine file, and then the actual wine file (Example: /home/wine/wine):
		};
		print do_var($var4);
	} elsif ($debuglevel =~ 3) {
		print "Enter the full path to wine (Example: /home/wine/wine):\n";
	}
	$wineloc=<STDIN>;
	chomp $wineloc; 
}
print "Checking if $wineloc is stripped...\n";
$ifstrip = `nm $wineloc 2>&1`;
if ($ifstrip =~ /no symbols/) {
	print "Your wine is stripped! Please re-download an unstripped version!\n";
	exit;
}
else {
	print "$wineloc is unstripped\n";
}
$var5 = qq{
What version of windows are you using with wine? 0-None, 1-Win3.x,
2-Win95, 3-Win98, 4-WinNT3.5x, 5-WinNT4.x, 6-WinNT5.x, 7-Other (Enter
0-7):
};
print do_var($var5);
$winver=<STDIN>; 
until ($winver < 7) {
	$var6 = qq{
	No! Enter a number from 0 to 7 that corresponds to your windows version! 
	};
	print do_var($var6);
	$winver=<STDIN>; 
}
chomp $winver; 
if ($winver =~ 0) { 
	$winver="None Installed"; 
} elsif ($winver =~ 1) { 
	$winver="Windows 3.x"; 
} elsif ($winver =~ 2) { 
	$winver="Windows 95"; 
} elsif ($winver =~ 3) { 
	$winver="Windows 98"; 
} elsif ($winver =~ 4) { 
	$winver="Windows NT 3.5x"; 
} elsif ($winver =~ 5) { 
	$winver="Windows NT 4.x"; 
} elsif ($winver =~ 6) { 
	$winver="Windows NT 5.x"; 
} elsif ($winver =~ 7) { 
	print "OK. What version of Windows are you using?\n";
	$winver=<STDIN>; 
	chomp $winver; 
}
if ($debuglevel < 3) {
	$var7 = qq{
	Enter the full path to the program you want to run. Remember what you
	were told before - a full path is the directories leading up to the
	program and then the program's name, like /dos/windows/sol.exe, not
	sol.exe:
	};
	print do_var($var7);
}
if ($debuglevel =~ 3) {
	$var8 = qq{
	Enter the full path to the program you want to run (Example: 
	/dos/windows/sol.exe, NOT sol.exe): 
	};
	print do_var($var8);
} 
$program=<STDIN>;
chomp $program;
$var9 = qq{
Enter the name, version, and manufacturer of the program (Example:
Netscape Navigator 4.5):
};
print do_var($var9);
$progname=<STDIN>;
chomp $progname;
$var10 = qq{
Enter 0 if your program is 16 bit (Windows 3.x), 1 if your program is 32
bit (Windows 9x, NT3.x and up), or 2 if you are unsure:
};
print do_var($var10);
$progbits=<STDIN>;
chomp $progbits;
until ($progbits < 3) {
	print "You must enter 0, 1 or 2!\n";
	$progbits=<STDIN>;
	chomp $progbits 
}
if ($progbits =~ 0) { 
	$progbits=Win16
} elsif ($progbits =~ 1) { 
	$progbits=Win32 
} else { 
	$progbits = "Unsure" 
} 
if ($debuglevel > 1) {
	if ($debuglevel =~ 2) {
		$var11 = qq{
		Enter any extra debug options. Default is +relay - If you don't
		know what options to use, just hit enter, and I'll use those (Example, the
		developer tells you to re-run with -debugmsg +dosfs,+module you would type
		in +dosfs,+module). Hit enter if you're not sure what to do:
		};
		print do_var($var11);
	} elsif ($debuglevel =~ 3) {
		$var12 = qq{
		Enter any debug options you would like to use. Just enter parts after
		-debugmsg. Default is +relay:
		};
		print do_var($var12);
	}
	$debugopts=<STDIN>;
	chomp $debugopts;
	if ($debugopts=~/-debugmsg /) {
		($crap, $debugopts) = / /,$debugopts; 
	}
	if ($debugopts=~/^\s*$/) { 
		$debugopts="+relay"; 
	} 
} elsif ($debuglevel =~ 1) {
	$debugopts = "+relay"; 
} 
if ($debuglevel > 1) {
	if ($debuglevel =~ 2) {
		$var13 = qq{
		How many trailing lines of debugging info do you want to include in the report
		you're going to submit (First file)? If a developer asks you to include
		the last 200 lines, enter 200 here. Default is 100, which is reached by
		pressing enter. (If you're not sure, just hit enter):
		};
		print do_var($var13);
	} elsif ($debuglevel =~ 3) {
		$var14 = qq{
		Enter how many lines of trailing debugging output you want in your nice
		formatted report. Default is 100:
		};
		print do_var($var14);
	}
	$lastnlines=<STDIN>;
	chomp $lastnlines;
	if ($lastnlines=~/^\s*$/) { 
	$lastnlines=100; 
	} 
} elsif ($debuglevel =~ 1) {
	$lastnlines=100; 
}
if ($debuglevel > 1) { 
	$var15 = qq{
	Enter any extra options you want to pass to WINE. Strongly recommended you
	include -managed:
	};
	print do_var($var15);
	$extraops=<STDIN>;
	chomp $extraops;
} elsif ($debuglevel =~ 1) {
	$extraops="-managed";
}
print "Enter your distribution name (Example: Redhat 5.0):\n";
$dist=<STDIN>;
chomp $dist;
if ($debuglevel > 1) {
	if ($debuglevel =~ 2) {
		$var16 = qq{
		When you ran ./configure to build wine, were there any special options
		you used to do so (Example: --enable-dll)? If you didn't use any special
		options or didn't compile WINE on your own, just hit enter:
		};
		print do_var($var16);
	} elsif ($debuglevel =~ 3) {
		$var17 = qq{
		Enter any special options you used when running ./configure for WINE
		(Default is none, use if you didn't compile wine yourself):
		};
		print do_var($var17);
	}
	$configopts=<STDIN>;
	chomp $configopts;
	if ($configopts=~/\s*/) { 
	$configopts="None"; 
	} 
} elsif ($debuglevel =~ 1) {
	$configopts="None"; 
}
if ($debuglevel > 1) {
	if ($debuglevel =~ 2) {
		$var18 = qq{
		Is your wine version CVS or from a .tar.gz file? As in... did you download it
		off a website/ftpsite or did you/have you run cvs on it to update it?
		For CVS: YYMMDD, where YY is the year (99), MM is the month (01), and DD
		is the day (14), that you last updated it (Example: 990114). 
		For tar.gz: Just hit enter and I'll figure out the version for you:
		};
		print do_var($var18);
	} elsif ($debuglevel =~ 3) {
		$var19 = qq{
		Is your wine from CVS? Enter the last CVS update date for it here, in
		YYMMDD form (If it's from a tarball, just hit enter):
		};
		print do_var($var19);
	}
	$winever=<STDIN>;
	chomp $winever;
	$winever=~s/ //g;
	if ($winever=~/[0-9]+/) {  
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
$gccver=`gcc -v 2>&1`;
($leftover,$gccver) = split /\n/,$gccver;
chomp $gccver;
$cpu=`uname -m`;
chomp $cpu;
$kernelver=`uname -r`;
chomp $kernelver;
$ostype=`uname -s`;
chomp $ostype;
$wineneeds=`ldd $wineloc`;
if ($debuglevel < 3) {
	$var20 = qq{
	OK, now I'm going to run WINE. I will close it for you once the wine
	debugger comes up. NOTE: You won't see ANY debug messages. Don't
	worry, they are being output to a file. Since there are so many, it's
	not a good idea to have them all output to a terminal (Speed slowdown 
	mainly).
	WINE will still run much slower than normal, because there will be so
	many debug messages being output to file. 
	};
	print do_var($var20);
} elsif ($debuglevel =~ 3) {
	$var21 = qq{
	OK, now it's time to run WINE. I will close down WINE for you after
	the debugger is finished doing its thing.
	};
	print do_var($var21);
}
$bashver=qw("/bin/bash -version");
if ($bashver =~ /2\./) { $outflags = "2>" }
else { $outflags = ">&" }
print "Hit enter to start wine!\n";
$blank=<STDIN>;
$dir=$program;
$dir=~m#(.*)/#;
$dir=$1;
chdir($dir);
system("echo quit|$wineloc -debugmsg $debugopts $extraops \"$program\" $outflags $dbgoutfile");
$lastlines=`tail -n $lastnlines $dbgoutfile`;
system("gzip $dbgoutfile");
open(OUTFILE,">$outfile");
print OUTFILE <<EOM;
Auto-generated debug report by Wine Quick Debug Report Maker Thingy:
WINE Version:                $winever
Windows Version:             $winver
Distribution:                $dist
Kernel Version:              $kernelver
OS Type:                     $ostype
CPU:                         $cpu
GCC Version:                 $gccver
Program:                     $progname
Program Type:                $progbits
Debug Options:               -debugmsg $debugopts
Other Extra Commands Passed: $extraops
Extra ./configure Commands:  $configopts
Wine Dependencies: 
$wineneeds
Last $lastnlines lines of debug output follows:
$lastlines
I have a copy of the full debug report, if it is needed.
Thank you!
EOM
$var22 = qq{
Great! We're finished making the debug report. Do whatever with it. The
filename for it is:
$outfile
The filename for the compressed full debug is:
$dbgoutfile.gz
Note that it is $dbgoutfile.gz, since I compressed it with gzip for you.
C Ya!
Adam the Jazz Guy
};
print do_var($var22);


