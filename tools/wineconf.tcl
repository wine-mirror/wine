#!/bin/sh
# the next line restarts using wish \
exec wish $0 ${1+"$@"} 
proc load_libs {} {
	global TKW
	set libLIST [glob $TKW/wineconf.libs/*.tcl]
	foreach i $libLIST {
		uplevel #0 [list source $i]
	}
}

proc load_lang { lang} {
	global TKW
	set langLIST [glob $TKW/wineconf.libs/*.$lang]
	foreach i $langLIST {
		uplevel #0 [list source $i]
	}
}
global TKW
if {![info exists TKW]} {
	set TKW .
	wm geom . 0x0
	load_libs
	load_lang eng
	#global TKW;set TKW [pwd]
	global USER; set USER [exec whoami]
	global env HOME; set HOME $env(HOME)
	image create photo wine_half -file wineconf.libs/wine-half.gif
	TkW:wineconf1
	TkW:autoconf
	exit
} else {
	load_libs
        load_lang eng
}
