proc TkW:exit {} {
	global EXIT_title QUITSAVING QUITNOSAVE CANCEL TKW
    set base .saveRC
    if {![winfo exists .choosemethod]} {
	cd $TKW
        TkW:choosemethod
	return
    }

	TkW:CloseOtherWindows
 
	TkW:debug "Other should be detroyed now......"
    if {[winfo exists .saveRC]} {
        wm deiconify .saveRC; return
    }
	TkW:debug "Entering TkW:exit"
    ###################
    # CREATING WIDGETS
    ###################
    toplevel .saveRC -class Toplevel \
        -background #ffffff 
    wm focusmodel .saveRC passive
    wm geometry .saveRC 412x163+404+267
    wm maxsize .saveRC 1265 994
    wm minsize .saveRC 1 1
    wm overrideredirect .saveRC 0
    wm resizable .saveRC 1 1
    wm deiconify .saveRC
    wm title .saveRC "$EXIT_title"
    #wm iconbitmap .saveRC @$TKW/setupfiles/images/setup2.xbm
    #wm iconmask   .saveRC @$TKW/setupfiles/images/setupmask2.xbm

    label .saveRC.lab18 \
        -background #dddddd \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-160-*-*-*-*-*-* \
        -foreground #0000f6 -relief groove -text label \
        -textvariable CONFIRMQUIT  -wraplength 350
    button .saveRC.but19 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text button -textvariable QUITSAVING -command {TkW:saveRC;exit}
    button .saveRC.but20 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text button -textvariable QUITNOSAVE  -command {exit}
    button .saveRC.but21 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text button -textvariable CANCEL -command {destroy .saveRC}
    ###################
    # SETTING GEOMETRY
    ###################
    place .saveRC.lab18 \
        -x 5 -y 5 -width 403 -height 110 -anchor nw -bordermode ignore 
    place .saveRC.but19 \
        -x 5 -y 120 -width 135 -height 38 -anchor nw -bordermode ignore 
    place .saveRC.but20 \
        -x 138 -y 120 -width 135 -height 38 -anchor nw -bordermode ignore 
    place .saveRC.but21 \
        -x 273 -y 120 -width 135 -height 38 -anchor nw -bordermode ignore 
}

proc TkW:CloseOtherWindows {} {
	set Wins ".wineconf .compile .install .insbin"

	foreach i $Wins { if [winfo exists $i] {destroy $i}}

	return

}

proc TkW:saveRC {} {
# This proc saves the ~/.winesetuprc file
# all the last compile option, source dest, install dire,
# and the like are saved.
# Could be improved by adding a "default" button in some places so some
#changes could be ignore there....
# For now, it saves all the last selections.

# Opens the ~/.winesetuprc for writting:
set savein [open ~/.winesetuprc w]

# Puts the comments as well as the content:

puts $savein "#"
puts $savein "# System description:"
puts $savein "#"
global distrib; puts $savein "set distrib \"$distrib\""
global kernel; puts $savein "set kernel \"$kernel\""
global processor; puts $savein "set processor \"$processor\""
global FTPNUM bz2docdest bz2exedest
global BROWSER;puts $savein "set BROWSER \"$BROWSER\""
puts $savein "#"
puts $savein "# Method:"
puts $savein "#"
global choosenmethod; puts $savein "set choosenmethod \"$choosenmethod\""
puts $savein "#"
puts $savein "# Compile options"
puts $savein "#"
puts $savein "# where to search the archive"
global dir_search; puts $savein "set dir_search \"$dir_search\""
puts $savein "# where to put the executable"
global bindir; puts $savein "set bindir \"$bindir\""
puts $savein "# options"
global debug trace noemu dll nolib clean
puts $savein "set debug $debug ; set trace $trace ; set noemu $noemu ; set dll $dll ; set nolib $nolib ; set clean $clean"

puts $savein "# where to put the source tree"
global srcdest; puts $savein "set srcdest \"$srcdest\""
puts $savein "# where is the top of the source tree"
global winedir; puts $savein "set winedir \"$winedir\""
puts $savein "#"
puts $savein "# Number in the list of ftp site to use"
puts $savein "set FTPNUM $FTPNUM"

puts $savein "#"
puts $savein "# BZIP@ exe and doc dest"
puts $savein "set bz2exedest $bz2exedest"
puts $savein "set bz2docdest $bz2docdest"

puts $savein "# "
puts $savein "# memory from last start"
puts $savein "# "
global laststart ; puts $savein "set laststart \"$laststart\""

}
