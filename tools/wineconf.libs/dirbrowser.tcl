proc TkW:dirbrowse {pattern patternF mode} {
	# input arg: initial pattern  mode (File or Folder
	# returns selected folders as a global SelectFolder
	# or selected file as a global: SelectFile
	# variable: SelectFolder

	# You can use WAITbrowse in the calling app to wait
	# for dirbrowse to exit (set to 0 on exit, or -1 on CANCEL)
	global WAITbrowse
	global SelectFolder
	global SelectFile
	if {![info exists SelectFile]} {set SelectFile ""}
	if {![info exists SelectFolder]} {set SelectFolder [pwd]}
	global ndirs nfiles;set ndirs 0;set nfiles 0
	global ModeBrowse Restore;set Restore $SelectFolder
	global FTPcd
	if {$mode == "local"} {set FTPcd 1} {set FTPcd 0}
	set ModeBrowse $mode
	global DIRBROWSE WinBrowse
    ###################
    # CREATING WIDGETS
    ###################
	if {$mode != "local" } {
    		toplevel .dirbrowse -class Toplevel \
        		-background #feffff 
    		wm focusmodel .dirbrowse passive
    		wm geometry .dirbrowse 464x296
    		wm maxsize .dirbrowse 1265 994
    		wm minsize .dirbrowse 1 1
    		wm overrideredirect .dirbrowse 0
    		wm resizable .dirbrowse 1 1
    		wm deiconify .dirbrowse
    		wm title .dirbrowse "TkWine: $DIRBROWSE"
		set WinBrowse dirbrowse
	} else {
		set WinBrowse ftp.browsers.local
    		#frame .$WinBrowse -background #feffff -height 300 -width 464
	}

# Lang globals:
	global FolderSelection OK CANCEL FILTERFOLD FILTERFILES
	global RESCAN VIEW HELP
# Other globals:
	global BrowsePattern;set BrowsePattern $pattern 
	global BrowsePatternF;set BrowsePatternF $patternF 
	global dirscroll MouseY


# label and entrybox for selection:
    label .$WinBrowse.foldselLab \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text $FolderSelection 
    entry .$WinBrowse.foldsel \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
	-textvariable SelectFolder
	if {$ModeBrowse == "File"} {
		.$WinBrowse.foldsel  configure -textvar SelectFile
	}
# frame for display of currently available folders:
    frame .$WinBrowse.dirframe \
        -borderwidth 1 -height 30 -relief sunken -width 30 \
	-background #dddddd
    scrollbar  .$WinBrowse.dirframe_scr  -width 10
    frame .$WinBrowse.fileframe \
        -borderwidth 1 -height 30 -relief sunken -width 30 
    scrollbar  .$WinBrowse.fileframe_scr  -width 10
# label/entry for pattern
    label .$WinBrowse.patternLab \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text $FILTERFOLD
    entry .$WinBrowse.patternF \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
	-textvariable BrowsePatternF
    label .$WinBrowse.patternLabF \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text $FILTERFILES
    entry .$WinBrowse.pattern \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
	-textvariable BrowsePattern
    frame .$WinBrowse.fra25 \
        -borderwidth 1 -height 30 -relief sunken -width 30 
    menubutton .$WinBrowse.fra25.men26 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -menu .$WinBrowse.fra25.men26.m -padx 4 -pady 3 -relief ridge -text $VIEW 
    menu .$WinBrowse.fra25.men26.m \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* 
    button .$WinBrowse.fra25.but28 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -relief ridge -text $HELP 
    button .$WinBrowse.butOK \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text $OK  \
	-command {destroy .$WinBrowse
		set WAITbrowse 0
		return "$SelectFolder $SelectFile"
	}
    button .$WinBrowse.butCANCEL \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text $CANCEL \
	-command {destroy .$WinBrowse
		set WAITbrowse -1
		return "$SelectFolder $SelectFile"
	}
    button .$WinBrowse.butRESCAN \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text $RESCAN -command {TkW:DisplayCurrentDir $BrowsePattern}
    ###################
    # SETTING GEOMETRY
    ###################
global YHeight;set YHeight 170
if {$mode !="local" && $mode != "remote"} {
    place .$WinBrowse.foldselLab \
        -x 47 -y 270 -width 78 -height 20 -anchor nw -bordermode ignore 
    place .$WinBrowse.foldsel \
        -x 148 -y 268 -width 290 -height 24 -anchor nw -bordermode ignore 
    place .$WinBrowse.dirframe \
        -x 5 -y 90 -width 145 -height $YHeight -anchor nw -bordermode ignore 
    place  .$WinBrowse.dirframe_scr -x 150 -y 90 -width 17 -height $YHeight
    place .$WinBrowse.fileframe \
        -x 180 -y 90 -width 175 -height 170 -anchor nw -bordermode ignore 
    place  .$WinBrowse.fileframe_scr -x 355 -y 90 -width 17 -height $YHeight
    place .$WinBrowse.patternLab \
        -x 27 -y 60 -width 143 -height 20 -anchor nw -bordermode ignore 
    place .$WinBrowse.pattern \
        -x 190 -y 58 -width 150 -height 24 -anchor nw -bordermode ignore 
    place .$WinBrowse.patternLabF \
        -x 27 -y 35 -width 143 -height 20 -anchor nw -bordermode ignore 
    place .$WinBrowse.patternF \
        -x 190 -y 32 -width 150 -height 24 -anchor nw -bordermode ignore 
    place .$WinBrowse.fra25 \
        -x 0 -y 1 -width 455 -height 25 -anchor nw -bordermode ignore 
    place .$WinBrowse.fra25.men26 \
        -x -1 -y 0 -width 67 -height 24 -anchor nw -bordermode ignore 
    place .$WinBrowse.fra25.but28 \
        -x 397 -y -3 -width 60 -height 28 -anchor nw -bordermode ignore 
    place .$WinBrowse.butOK \
        -x 380 -y 115 -width 77 -height 28 -anchor nw -bordermode ignore 
    place .$WinBrowse.butCANCEL \
        -x 380 -y 155 -width 77 -height 28 -anchor nw -bordermode ignore 
    place .$WinBrowse.butRESCAN \
        -x 380 -y 195 -width 77 -height 28 -anchor nw -bordermode ignore 
} else {
# case of ftp filebrowser ....

	bind .$WinBrowse.pattern <Return> {.$WinBrowse.butRESCAN invoke}
       bind .$WinBrowse.patternF <Return> {.$WinBrowse.butRESCAN invoke}

       place .$WinBrowse.pattern  -x 0 -y 3 -width 145 -height 25
       place .$WinBrowse.patternF  -x 163 -y 3 -width 145 -height 25

       place .$WinBrowse.dirframe  -x 0 -y 28 -width 145 -height $YHeight
       place .$WinBrowse.dirframe_scr -x 145 -y 28 -width 17 -height $YHeight
       place .$WinBrowse.fileframe -x 163 -y 28 -width 145 -height $YHeight
       place .$WinBrowse.fileframe_scr -x 308 -y 28 -width 17 -height $YHeight

}

# binding the scrollbar:
	bind .$WinBrowse.dirframe_scr <ButtonPress> { TkW:ScrollDirs %x %y }
	bind .$WinBrowse.dirframe_scr <ButtonRelease> { set dirscroll no; grab release .$WinBrowse.dirframe_scr }
	bind .$WinBrowse.dirframe_scr <Motion> { set MouseY %y }

	bind .$WinBrowse.fileframe_scr <ButtonPress> { TkW:ScrollFiles %x %y }
	bind .$WinBrowse.fileframe_scr <ButtonRelease> { set filescroll no; grab release .$WinBrowse.fileframe_scr }
	bind .$WinBrowse.fileframe_scr <Motion> { set MouseY %y }

global TKW
	image create photo "fold" -file \
		$TKW/shared/icons/smalls/small_folder_yellow.gif
	image create photo "unknown" -file \
		$TKW/shared/icons/smalls/unknown.gif
	image create photo "execfile" -file \
		$TKW/shared/icons/smalls/exec.gif
	image create photo "imgfile" -file \
		$TKW/shared/icons/smalls/xpaint.gif
	image create photo "htmlfile" -file \
		$TKW/shared/icons/smalls/html.gif
	#image create photo "up" -file $TKW/shared/icons/up.gif
	TkW:DisplayCurrentDir $BrowsePattern
}

proc TkW:DisplayCurrentDir {BrowsePattern} {

	global ndirs nfiles Ybase dirlist filelist FYbase specialwhere 
	global FYbase BrowsePatternF ModeBrowse
	DestroyDirs
	set ndirs 0
	set dirlist ""
	set filelist ""

	# First we get the directory list:

	if {$ModeBrowse != "remote"} {
	    set fileList ""
	    set fileList [glob -nocomplain [pwd]/$BrowsePattern]
	    set fileList "$fileList [glob -nocomplain [pwd]/.$BrowsePattern]"
	    set UpLevel 0
	    foreach i $fileList {
		    if {[file tail $i] == ".."} {set UpLevel 1}
		    if { [file isdir $i] } { set dirlist "$dirlist \"$i\"" }
	    }
	    set fileList ""
	    set fileList [glob -nocomplain [pwd]/$BrowsePatternF]
	    set fileList "$fileList [glob -nocomplain [pwd]/.$BrowsePatternF]"
	    foreach i $fileList {
		if { ![file isdir $i] } { set filelist "$filelist \"$i\"" }
	    }
	    if {$UpLevel == 0} {set dirlist ".. $dirlist"}
	    set dirlist [lsort $dirlist]
	    set filelist [lsort $filelist]
	} else {set filelist ""; set dirlist ".."}
	set Ybase 5
	set FYbase 5
	TkW:ShowDirs
	TkW:ShowFiles
}

proc TkW:ShowDirs {} {
	global dirlist ndirs 
	DestroyDirs
	set ndirs 0
	set showedIn 0
	set showedbelow 0
	set showedabove 0
	foreach i $dirlist {
		set inout [TkW:showdir $i]
		if {$inout == 0} {set showedIn [expr $showedIn + 1]}
		if {$inout == -1} {set showedabove [expr $showedabove + 1]}
		if {$inout == 1} {set showedbelow [expr $showedbelow + 1]}
	}
	# Scrollbars management:
	TkW:DirBrowseScroll [expr (100.*$showedabove/$ndirs)/100.]  \
				[expr 1. - (100.*$showedbelow/$ndirs)/100.]
	update
	
}
proc TkW:DirBrowseScroll { y1 y2} {
	global SliderY1 SliderY2 WinBrowse
	set SliderY1 $y1
	set SliderY2 $y2
	.$WinBrowse.dirframe_scr set  $y1 $y2
}
proc TkW:FileBrowseScroll { y1 y2} {
	global FSliderY1 FSliderY2 WinBrowse
	set FSliderY1 $y1
	set FSliderY2 $y2
	.$WinBrowse.fileframe_scr set  $y1 $y2
}
proc TkW:hilightFold {y} {

	global YDIRS ndirs DIRS SelectNum WinBrowse
	global SelectFolder  BrowsePattern FTPcd
	
	if { $y > $YDIRS($ndirs)} {set num $ndirs} else {
		set i 1
		set ok 0
		while {$ok == 0} {
			if {
				$YDIRS($i) < $y &&\
					 $y <= $YDIRS([expr $i + 1])}  {
				set num $i
				set ok 1
			} else { set i [expr $i + 1] }
		}
	}


	if {$SelectFolder == $DIRS($num)} {
		cd $SelectFolder
		if {$FTPcd == 1} {FTP_Lcd $SelectFolder}
		set SelectFolder [pwd]
		TkW:DisplayCurrentDir $BrowsePattern
		unset SelectNum 
	} else {
		.$WinBrowse.dirframe.label$num configure -background #ffffff
		set SelectFolder $DIRS($num)
		if {[info exists SelectNum]} {
			.$WinBrowse.dirframe.label$SelectNum configure \
				-background #dddddd
		}
		set SelectNum $num
	}

	update
}

proc TkW:showdir {fulldirname} {
	global TKW ndirs Ybase YHeight YDIRS DIRS Ydecal ModeBrowse WinBrowse
	set Ydecal 20
	set dirname [file tail $fulldirname]
	if {$dirname == "."} return
	set Y [expr $Ybase + $ndirs * $Ydecal]
	set In 0
	if {$Y < 0 } {set In -1}
	if {$Y > [expr $YHeight - $Ydecal]} { set In 1 } 


	set ndirs [expr $ndirs + 1]
	set YDIRS($ndirs) $Y
	set DIRS($ndirs) $fulldirname
	label .$WinBrowse.dirframe.icon$ndirs -image fold
	label .$WinBrowse.dirframe.label$ndirs -text $dirname\
		 -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*
	
	if {$In == 0} {
		place .$WinBrowse.dirframe.icon$ndirs -x 5 -y $Y
		place .$WinBrowse.dirframe.label$ndirs -x 30 -y $Y
	}

	#dirframe bindings
	bind .$WinBrowse.dirframe.icon$ndirs <Button-1> {
		set Y [expr %Y - [winfo rooty .$WinBrowse.dirframe] -$Ybase]
		TkW:hilightFold $Y
	}
	#dirframe bindings
	bind .$WinBrowse.dirframe.label$ndirs <ButtonPress-1> {
		set Y [expr %Y - [winfo rooty .$WinBrowse.dirframe] -$Ybase]
		TkW:hilightFold $Y
	}
	return $In
}

proc TkW:ScrollDirs {X Y} {

	global Ybase Ydecal ndirs dirscroll global nextLoop MouseY WinBrowse
	global SliderY1 SliderY2 YHeight Nabove Nbelow

	set MinBase [expr -$Ydecal * ($ndirs - [expr $YHeight/$Ydecal])]
	set Slider1 $SliderY1
	set Slider2 $SliderY2
	set MaxSlide1 [expr 1-($SliderY2-$SliderY1)]
	set MinSlide2 [expr $SliderY2-$SliderY1]
	switch [.$WinBrowse.dirframe_scr identify $X $Y] {

		{arrow1} { set dirscroll UP; set modeUP 1}
		{arrow2} { set dirscroll DOWN;set modeDOWN 1}
		{slider} {set dirscroll SLIDER}
		{trough1} {set dirscroll UP; set modeUP page}
		{trough2} {set dirscroll DOWN;set modeDOWN page}
	}
	while {$dirscroll == "UP"} {
		if {$modeUP == "1"} {set XUP 1} {
			set XUP [expr ($YHeight/$Ydecal ) -1]
		}
		set Ybase [expr $Ybase + $XUP * $Ydecal]
		if {$Ybase > $Ydecal} {
			set Ybase 5
			set dirscroll no
			set Slider1 0
			set Slider2 $MinSlide2
		}
		TkW:ReShowDirs
		TkW:DirBrowseScroll [expr (100.*$Nabove/$ndirs)/100.]  \
			[expr 1. - (100.*$Nbelow/$ndirs)/100.]
		set nextLoop 0
                after 30 {set nextLoop 1}
                while {$nextLoop == 0} {update}
		set Slider1 $SliderY1
		set Slider2 $SliderY2
	}
	while {$dirscroll == "DOWN"} {
		if {$modeDOWN == "1"} {set XDOWN 1} {
			set XDOWN [expr ($YHeight/$Ydecal ) -1]
		}
		set Ybase [expr $Ybase - $XDOWN * $Ydecal]
		if {$Ybase < $MinBase} {
			set Ybase $MinBase
			set dirscroll no
			set Slider2 1
			set Slider1 $MaxSlide1
		}
		TkW:ReShowDirs 
		TkW:DirBrowseScroll [expr (100.*$Nabove/$ndirs)/100.]  \
			[expr 1. - (100.*$Nbelow/$ndirs)/100.]
		set nextLoop 0
		after 30 {set nextLoop 1}
		while {$nextLoop == 0} {update}
		set Slider1 $SliderY1
		set Slider2 $SliderY2
	}
	while {$dirscroll == "SLIDER"} {
		set nextLoop 0
		after 20 {set nextLoop 1}
		while {$nextLoop == 0} {
			set Delta [expr (100.*($MouseY - $Y)/$YHeight)/100. ]
			set Slider1 [expr $SliderY1 +$Delta]
			set Slider2 [expr $SliderY2 +$Delta]
			if {$Slider2 > 1} {
				set Slider2 1
				set Slider1 $MaxSlide1
			}
			if {$Slider1 < 0} {
				set Slider1 0
				set Slider2 $MinSlide2
			}
			.$WinBrowse.dirframe_scr set  $Slider1 $Slider2
			set Ybase [expr 5 - $Slider1 * $ndirs *$Ydecal]
			TkW:ReShowDirs
			update
		}
	}
	set SliderY1 $Slider1
	set SliderY2 $Slider2
}
proc TkW:ScrollFiles {X Y} {

	global FYbase Ydecal nfiles filescroll nextLoop MouseY WinBrowse
	global FSliderY1 FSliderY2 YHeight NFabove NFbelow

	set MinBase [expr -$Ydecal * ($nfiles - [expr $YHeight/$Ydecal])]
	set FSlider1 $FSliderY1
	set FSlider2 $FSliderY2
	set MaxSlide1 [expr 1-($FSliderY2-$FSliderY1)]
	set MinSlide2 [expr $FSliderY2-$FSliderY1]
	switch [.$WinBrowse.fileframe_scr identify $X $Y] {

		{arrow1} { set filescroll UP; set modeUP 1}
		{arrow2} { set filescroll DOWN;set modeDOWN 1}
		{slider} {set filescroll SLIDER}
		{trough1} {set filescroll UP; set modeUP page}
		{trough2} {set filescroll DOWN;set modeDOWN page}
	}
	while {$filescroll == "UP"} {
		if {$modeUP == "1"} {set XUP 1} {
			set XUP [expr ($YHeight/$Ydecal ) -1]
		}
		set FYbase [expr $FYbase + $XUP * $Ydecal]
		if {$FYbase > $Ydecal} {
			set FYbase 5
			set filescroll no
			set FSlider1 0
			set FSlider2 $MinSlide2
		}
		TkW:ReShowFiles
		if {$nfiles != 0} {
			TkW:FileBrowseScroll [expr (100.*$NFabove/$nfiles)/100.]  \
				[expr 1. - (100.*$NFbelow/$nfiles)/100.]
		} else {
			TkW:FileBrowseScroll 0 1
		}
		set nextLoop 0
                after 30 {set nextLoop 1}
                while {$nextLoop == 0} {update}
		set FSlider1 $FSliderY1
		set FSlider2 $FSliderY2
	}
	while {$filescroll == "DOWN"} {
		if {$modeDOWN == "1"} {set XDOWN 1} {
			set XDOWN [expr ($YHeight/$Ydecal ) -1]
		}
		set FYbase [expr $FYbase - $XDOWN * $Ydecal]
		if {$FYbase < $MinBase} {
			set FYbase $MinBase
			set filescroll no
			set FSlider2 1
			set FSlider1 $MaxSlide1
		}
		TkW:ReShowFiles 
		if {$nfiles != 0} {
			TkW:FileBrowseScroll [expr (100.*$NFabove/$nfiles)/100.]  \
				[expr 1. - (100.*$NFbelow/$nfiles)/100.]
		} else {
			TkW:FileBrowseScroll 0 1
		}
		set nextLoop 0
		after 30 {set nextLoop 1}
		while {$nextLoop == 0} {update}
		set FSlider1 $FSliderY1
		set FSlider2 $FSliderY2
	}
	while {$filescroll == "SLIDER"} {
		set nextLoop 0
		after 20 {set nextLoop 1}
		while {$nextLoop == 0} {
			set Delta [expr (100.*($MouseY - $Y)/$YHeight)/100. ]
			set FSlider1 [expr $FSliderY1 +$Delta]
			set FSlider2 [expr $FSliderY2 +$Delta]
			if {$FSlider2 > 1} {
				set FSlider2 1
				set FSlider1 $MaxSlide1
			}
			if {$FSlider1 < 0} {
				set FSlider1 0
				set FSlider2 $MinSlide2
			}
			.$WinBrowse.fileframe_scr set  $FSlider1 $FSlider2
			set FYbase [expr 5 - $FSlider1 * $nfiles *$Ydecal]
			TkW:ReShowFiles
			update
		}
	}
	set FSliderY1 $FSlider1
	set FSliderY2 $FSlider2
}

proc TkW:ReShowDirs {} {
	global dirlist Ybase Ydecal ndirs YHeight WinBrowse
	global Nabove Nbelow
	set n 0
	set Nabove 0
	set Nbelow 0
	foreach i $dirlist {
		set dirname [file tail $i]
		if {$dirname != "."} {
			set Y0 [expr $Ybase + $n * $Ydecal]
			if {$Y0 < -5} {set Nabove [expr $Nabove + 1]}
			if {$Y0 > [expr $YHeight - $Ydecal]} { set Nbelow [expr $Nbelow + 1]}
			set n [expr $n + 1]
			place .$WinBrowse.dirframe.icon$n -x 5 -y $Y0
			place .$WinBrowse.dirframe.label$n -x 30 -y $Y0
		}
	}
}
proc TkW:ReShowFiles {} {
	global filelist FYbase Ydecal nfiles YHeight WinBrowse
	global NFabove NFbelow
	set n 0
	set NFabove 0
	set NFbelow 0
	foreach i $filelist {
		set filename [file tail $i]
		if {$filename != "."} {
			set Y0 [expr $FYbase + $n * $Ydecal]
			if {$Y0 < -5} {set NFabove [expr $NFabove + 1]}
			if {$Y0 > [expr $YHeight - $Ydecal]} { set NFbelow [expr $NFbelow + 1]}
			set n [expr $n + 1]
			place .$WinBrowse.fileframe.icon$n -x 5 -y $Y0
			place .$WinBrowse.fileframe.label$n -x 30 -y $Y0
		}
	}
}

proc TkW:ShowFiles {} {
	global filelist nfiles
	DestroyFiles
	set nfiles 0
	set FshowedIn 0
	set NFbelow 0
	set NFabove 0
	foreach i $filelist {
		set inout [TkW:showfile $i]
		if {$inout == 0} {set FshowedIn [expr $FshowedIn + 1]}
		if {$inout == -1} {set NFabove [expr $NFabove + 1]}
		if {$inout == 1} {set NFbelow [expr $NFbelow + 1]}
	}
	# Scrollbar management:
	if {$nfiles != 0} {
		TkW:FileBrowseScroll [expr (100.*$NFabove/$nfiles)/100.]  \
			[expr 1. - (100.*$NFbelow/$nfiles)/100.]
	} else {
		TkW:FileBrowseScroll 0 1
	}
	
}
proc TkW:showfile {fullfilename} {
	global TKW nfiles FYbase YHeight YFILES Ydecal FILES
	set Y [expr $FYbase + $nfiles * $Ydecal]
	set In 0
	if {$Y < 0 } {set In -1}
	if {$Y > [expr $YHeight - $Ydecal]} { set In 1 } 


	set nfiles [expr $nfiles + 1]
	set YFILES($nfiles) $Y
	set FILES($nfiles) $fullfilename
	createPlaceFileIcon $nfiles $Y
	return $In
}
proc createPlaceFileIcon {n Y} {
	global FILES ModeBrowse WinBrowse
	set fullfilename $FILES($n)
	set icon unknown
	if {[file executable $fullfilename]} {set icon execfile}
	if {[file extension $fullfilename] == ".html"} {set icon htmlfile}
	if {[file extension $fullfilename] == ".htm"} {set icon htmlfile}
	if {[file extension $fullfilename] == ".gif"} {set icon imgfile}
	if {[file extension $fullfilename] == ".jpg"} {set icon imgfile}
	set filename [file tail $fullfilename]
	label .$WinBrowse.fileframe.icon$n -image $icon
	label .$WinBrowse.fileframe.label$n -text $filename\
	    -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
	    -background #dddddd

	place .$WinBrowse.fileframe.icon$n -x 5 -y $Y
	place .$WinBrowse.fileframe.label$n -x 30 -y $Y
	
	if {$ModeBrowse == "File"} {
		#fileframe bindings
		bind .$WinBrowse.fileframe.icon$n <Button-1> {
			set Y [expr %Y - [winfo rooty .$WinBrowse.fileframe] -$FYbase]
			TkW:hilightFile $Y
		}
		bind .$WinBrowse.fileframe.icon$n <Double-Button-1> {
			destroy .$WinBrowse
			set WAITbrowse 0
			return "$SelectFolder $SelectFile"
		}
		#fileframe bindings
		bind .$WinBrowse.fileframe.label$n <ButtonPress-1> {
			set Y [expr %Y - [winfo rooty .$WinBrowse.fileframe] -$FYbase]
			TkW:hilightFile $Y
		}
		bind .$WinBrowse.fileframe.label$n <Double-Button-1> {
			destroy .$WinBrowse
			set WAITbrowse 0
			return "$SelectFolder $SelectFile"
		}
	}
}

proc DestroyDirs {} {
	global ndirs WinBrowse

	for {set i 1} {$i <= $ndirs} {set i [expr $i + 1 ]} {
		if {[winfo exists .$WinBrowse.dirframe.icon$i]} {
			destroy .$WinBrowse.dirframe.icon$i
			destroy .$WinBrowse.dirframe.label$i
		}
	}
}
proc DestroyFiles {} {
	global nfiles WinBrowse

	for {set i 1} {$i <= $nfiles} {set i [expr $i + 1 ]} {
		if {[winfo exists .$WinBrowse.fileframe.icon$i]} {
			destroy .$WinBrowse.fileframe.icon$i
			destroy .$WinBrowse.fileframe.label$i
		}
	}
}

proc TkW:hilightFile {y} {

	global YFILES nfiles FILES SelectNumF WinBrowse
	global SelectFile  BrowsePattern
	
	if { $y > $YFILES($nfiles)} {set num $nfiles} else {
		set i 1
		set ok 0
		while {$ok == 0} {
			if {
				$YFILES($i) < $y &&\
					 $y <= $YFILES([expr $i + 1])}  {
				set num $i
				set ok 1
			} else { set i [expr $i + 1] }
		}
	}


	if {$SelectFile == $FILES($num)} {
		return
	} else {
		.$WinBrowse.fileframe.label$num configure -background #ffffff
		set SelectFile $FILES($num)
		if {[info exists SelectNumF]} {
			.$WinBrowse.fileframe.label$SelectNumF configure -background #dddddd
		}
		set SelectNumF $num
	}

	update
}
