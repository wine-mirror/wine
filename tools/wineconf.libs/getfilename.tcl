#!/usr/bin/wish
#############################################################################
# Visual Tcl v1.07 Project
#

#################################
# GLOBAL VARIABLES
#
global widget; 
#################################

proc TkW:GetFileName {message opt1 opt2 searchtype} {
global OK MSG_title WAITfilename TKW GetFileName BROWSE WAITbrowse

	global SearchType
	set SearchType $searchtype

    set base .getfilename
    if {[winfo exists .getfilename]} {
        wm deiconify .getfilename; return
    }
    ###################
    # CREATING WIDGETS
    ###################
    toplevel .getfilename -class Toplevel \
        -background #ffffff 
    wm focusmodel .getfilename passive
    wm maxsize .getfilename 1265 994
    wm minsize .getfilename 1 1
    wm overrideredirect .getfilename 0
    wm resizable .getfilename 1 1
    wm deiconify .getfilename
     wm geometry .getfilename +100+100
    wm title .getfilename $MSG_title
    #wm iconbitmap .getfilename @$TKW/shared/images/setup2.xbm 
    #wm iconmask   .getfilename @$TKW/shared/images/setupmask2.xbm 

#creates the message widget:
    message .getfilename.msg -justify center -text $message \
	-background #dddddd -aspect 300

# Creates the entry box:
	global GetFileName
	entry .getfilename.ent -textvar GetFileName -relief sunken -width 50

    button .getfilename.but1 \
        -background #dddddd \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text $opt1 -width 8 -state disabled \
	-command {
		destroy .getfilename
		set WAITfilename opt1
		return 
	}
    button .getfilename.but2 \
        -background #dddddd \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text $opt2  -width 8\
	-command {destroy .getfilename; set WAITfilename opt2; return}
    button .getfilename.but3 \
        -background #dddddd \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text $BROWSE  -width 8\
	-command {TkW:dirbrowse * * $SearchType
		set WAITbrowse 1
		 while {$WAITbrowse == 1} {update}
                if {$WAITbrowse == 0}   {
		   if {$SearchType == "File"} {
			 set GetFileName $SelectFile
		   } else {
			 set GetFileName $SelectFolder
		   }
		}
              update  
	}
    ###################
    ###################
    # SETTING GEOMETRY
    ###################
#set H2 [expr $H +10]
    pack .getfilename.msg -padx 5 -pady 5 
         #-anchor nw -bordermode ignore  
    pack .getfilename.ent -padx 5 -pady 5
    pack .getfilename.but1 -padx 30 -pady 10 -side left
    pack .getfilename.but3 -padx 0 -pady 10 -side left
    pack .getfilename.but2 -padx 30 -pady 10 -side left
        #-x 110 -y $H2 -width 100 -height 28 -anchor nw -bordermode ignore 

	#bind .getfilename.ent <KeyPress-Return> {.getfilename.but1 invoke}

    while {[winfo exists .getfilename]} {
	if {[file exists $GetFileName]} {
		.getfilename.but1 configure -state normal
	} else {
		.getfilename.but1 configure -state disabled
	}
	update
    }
}
