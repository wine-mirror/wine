#!/usr/bin/wish
#############################################################################
# Visual Tcl v1.07 Project
#

#################################
# GLOBAL VARIABLES
#
global widget; 
#################################

proc TkW:message2 {message opt1 opt2} {
global OK MSG_title WAIT TKW

    set base .msg
    if {[winfo exists .msg]} {
        wm deiconify .msg; return
    }
    ###################
    # CREATING WIDGETS
    ###################
    toplevel .msg -class Toplevel \
        -background #ffffff 
    wm focusmodel .msg passive
    wm maxsize .msg 1265 994
    wm minsize .msg 1 1
    wm overrideredirect .msg 0
    wm resizable .msg 1 1
    wm deiconify .msg
     wm geometry .msg +100+100
    wm title .msg $MSG_title
    #wm iconbitmap .msg @$TKW/shared/images/setup2.xbm 
    #wm iconmask   .msg @$TKW/shared/images/setupmask2.xbm 

#creates the message widget:
    message .msg.msg -justify center -text $message -background #dddddd -aspect 300


    button .msg.but1 \
        -background #dddddd \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text $opt1 -width 8 \
	-command {destroy .msg; set WAIT opt1; return}
    button .msg.but2 \
        -background #dddddd \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text $opt2  -width 8\
	-command {destroy .msg; set WAIT opt2; return}
    ###################
    # SETTING GEOMETRY
    ###################
#set H2 [expr $H +10]
    pack .msg.msg -padx 5 -pady 5 
         #-anchor nw -bordermode ignore  
    pack .msg.but1 -padx 5 -pady 5 -side left
    pack .msg.but2 -padx 5 -pady 5 -fill x
        #-x 110 -y $H2 -width 100 -height 28 -anchor nw -bordermode ignore 
}
