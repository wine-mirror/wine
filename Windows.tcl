# Windows Tcl/Tk emulation scripts
# Initial implementation by Peter MacDonald pmacdona@sanjuan.uvic.ca

proc CreateWindow { f t x y h w } {
	global baseframe
	set baseframe $f
	wm title . "$t"
	frame .$f
	pack append . .$f {top}
	canvas .$f.canvas1 -scrollregion " $x $y $h $w " -width 15c -height 10c
	pack append .$f .$f.canvas1 {top}
}

proc CreateMenuEntry { fn t x } {
	global baseframe
	menubutton .$fn -text "$t" -underline $x -menu .$fn.m
	pack append .$baseframe .$fn left
	menu .$fn.m
}

proc CreateMenuBar { f } {
	global allmenus
	global baseframe
	set allmenus ""
	frame .$f -relief raised -borderwidth 1
	pack before .$baseframe .$f {top fillx}
}

proc AppendMenu { a b c d x } {
	global allmenus
	global baseframe
	if { ($b == 0x10) } {
		.$c configure -text "$d" -underline "$x"
		pack append .$a .$c left
		set allmenus "$allmenus $c"
		tk_menuBar .$a $allmenus
		tk_bindForTraversal .$baseframe.canvas1
	} else { if { ($b == 0x0800) } {
		.$a.m add separator
	} else {
		.$a.m add command -label "$d" -command "wincallback menu $a $b $c $d" -underline $x
	}}
}

####################################################################
# Misc unimplemented stuff
####################################################################

proc LoadIcon { wind name } {
	echo "LoadIcon"
}

proc LoadBitmap { wind name } {
	echo "LoadBitmap"
}

proc LoadCursor { wind name } {
	echo "LoadCursor"
}

proc GetObject { obj count ptr } {
	echo "GetObject $obj $count $ptr"
}

proc GetStockObject { wind } {
	echo "GetStockObject $wind"
}

proc DefWindowProc { a b c d } {
	echo "DefWindowProc $a $b $c $d"
}

proc GetMenu { a } {
	echo "GetMenu $a"
}

proc SetMenu { a b } {
	echo "SetMenu $a $b"
}

proc MessageBeep {a } {
	echo "MessageBeep $a"
}

proc MessageBox { wind msg title type } {
	echo "MessageBox '$msg'"
}

proc DrawText { f t top left right bottom } {
	.$f.canvas1 create text $top $left -text "$t" -anchor n
}
