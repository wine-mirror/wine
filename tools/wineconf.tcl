#!/bin/sh
#
# Copyright 1999 Jean-Louis Thirot
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
