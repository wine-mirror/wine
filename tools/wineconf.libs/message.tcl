#!/usr/bin/wish
#############################################################################
# Visual Tcl v1.07 Project
#

#################################
# GLOBAL VARIABLES
#
global widget; 
#################################

proc TkW:message {message} {
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
        #-background #4c92fe 
    wm focusmodel .msg passive
    wm maxsize .msg 1265 994
    wm minsize .msg 200 1
    wm overrideredirect .msg 0
    wm resizable .msg 1 1
    wm deiconify .msg
     wm geometry .msg +100+100
    wm title .msg $MSG_title
    #wm iconbitmap .msg @$TKW/shared/images/setup2.xbm
    #wm iconmask   .msg @$TKW/shared/images/setupmask2.xbm

#creates the message widget:
    message .msg.msg -justify center -text $message -background #dddddd -aspect 300


    button .msg.but18 \
        -background #dddddd \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text button -textvariable $OK -width 8\
	-command {
		destroy .msg
		set WAIT done 
	}   
    ###################
    # SETTING GEOMETRY
    ###################
#set H2 [expr $H +10]
    pack .msg.msg -padx 5 -pady 5  
         #-anchor nw -bordermode ignore  
    pack .msg.but18 -side bottom -padx 5 -pady 5  
        #-x 110 -y $H2 -width 100 -height 28 -anchor nw -bordermode ignore 
    #pack .msg.lab19 -width 50 -height 30 -padx 10 -pady 10
    #pack .msg.but18
}
