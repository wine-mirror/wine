proc TkW:wineconf {DefaultConfFile} {
	global WC_banner WC_driveSect WC_path WC_type WC_label  WC_fstype 
	global EDIT SAVE REMOVE ADD WC_wineSect WC_windows  WC_system WC_temp
	global WC_Symbols  WC_winepath WC_serialSect CLEAR WC_parallelSect
	global WC_logSect WC_exclude WC_wineLook
	global windowspath varpath syspath tmppath symbols winelook
	global ndrives drive drivepath drivetype drivefstype drivelabel
	global drivedevice driveserial
	global resolution defaultfont 
	global readport writeport TKW WAITconf
	global exclude default_progs startup allocsyscolors winelook wine_logfile
	global com lpt spool nspool alias nalias
	global WC_helpondrives WC_helponwine WC_helponserial WC_helponPP
	global WC_helponmisc WAIT WC_quit WC_quit
	global WC_quitreally WC_noquit
 	if {[winfo exist .choosemethod] } {destroy .choosemethod}
 	if {[winfo exist .install] } {destroy .install}
    set base .wineconf
    if {[winfo exists .wineconf]} {
        wm deiconify .wineconf; return
    }
    ###################
    # CREATING WIDGETS
    ###################
    toplevel .wineconf -class Toplevel \
        -background #3c1cfe 
    wm focusmodel .wineconf passive
    wm geometry .wineconf 462x600+148+153
    wm maxsize .wineconf 1265 994
    wm minsize .wineconf 1 1
    wm overrideredirect .wineconf 0
    wm resizable .wineconf 1 1
    wm deiconify .wineconf
    wm title .wineconf "TkWineSetup Configuring Wine"
    #wm iconbitmap .wineconf @$TKW/setupfiles/images/setup2.xbm
    #wm iconmask   .wineconf @$TKW/setupfiles/images/setupmask2.xbm

    label .wineconf.lab18 \
        -background #fcfefe \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text label -textvariable WC_banner 
    canvas .wineconf.can01 \
        -background #fee0b4 -borderwidth 2 -height 207 -relief ridge \
        -width 296 
    label .wineconf.can01.lab22 \
        -background #fefefe \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text label -textvariable WC_drivesSect 
    listbox .wineconf.can01.lis23 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
	-yscrollcommand {.wineconf.can01.scrDrives set }
    scrollbar .wineconf.can01.scrDrives \
        -borderwidth 1 -orient vert -width 10 \
	-command { .wineconf.can01.lis23 yview }
    listbox .wineconf.can01.lis24 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* 
    listbox .wineconf.can01.lis25 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* 
    listbox .wineconf.can01.lis26 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* 
    button .wineconf.can01.but28 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text button -textvariable ADD \
	-command {TkW:drives new}
    button .wineconf.can01.but29 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -state disabled -text button -textvariable REMOVE \
	-command {TkW:delDrive $sel_drive}
    button .wineconf.can01.but30 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -state disabled -text button -textvariable EDIT \
	-command {TkW:drives $sel_drive}
    label .wineconf.can01.lab31 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text label -textvariable WC_path 
    label .wineconf.can01.lab32 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text label -textvariable WC_type 
    label .wineconf.can01.lab33 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text label -textvariable WC_label 
    label .wineconf.can01.lab34 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text label -textvariable WC_fstype 
    button .wineconf.can01.but51 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text button -textvariable HELP \
	-command {TkW:message $WC_helpondrives}


#section wine

    canvas .wineconf.can37 \
        -background #fedcdc -borderwidth 2 -height 207 -relief ridge \
        -width 296 
    label .wineconf.can37.lab39 \
        -background #fefefe \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text label -textvariable WC_wineSect 
    label .wineconf.can37.lab40 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text label -textvariable WC_windows 
    label .wineconf.can37.lab41 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text label -textvariable WC_system 
    button .wineconf.can37.but42 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -textvariable WC_winepath \
	-command TkW:editWinePath

    button .wineconf.can37.but43 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text button -textvariable HELP \
	-command {TkW:message $WC_helponwine}
    entry .wineconf.can37.ent45 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -textvariable windowspath 
    entry .wineconf.can37.ent46 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -textvariable syspath 
    label .wineconf.can37.lab47 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text label -textvariable WC_temp 
    label .wineconf.can37.lab48 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text label -textvariable WC_Symbols 
    entry .wineconf.can37.ent49 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -textvariable tmppath 
    entry .wineconf.can37.ent50 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -textvariable symbols

# section serialports
    canvas .wineconf.can53 \
        -background #fedcdc -borderwidth 2 -relief ridge 
    label .wineconf.can53.lab57 \
        -background #fefefe \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text label -textvariable WC_serialSect
    listbox .wineconf.can53.lis58 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* 
    listbox .wineconf.can53.lis59 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
	-yscrollcommand {.wineconf.can53.scr60 set }
    scrollbar .wineconf.can53.scr60 \
        -borderwidth 1 -orient vert -width 10 \
	-command { .wineconf.can53.lis59 yview }
    button .wineconf.can53.but63 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text button -textvariable EDIT \
	-command {TkW:editports com}
    button .wineconf.can53.but64 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text button -textvariable CLEAR  -state disabled \
	-command {
		set com([expr $sel_com + 1]) " "
		TkW:setboxes 
		.wineconf.can53.but64 configure -state disabled
	}
     button .wineconf.can53.help \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text button -textvariable HELP\
	-command {TkW:message $WC_helponserial}

#section parallelports
    canvas .wineconf.can54 \
        -background #fedcdc -borderwidth 2 -relief ridge 
    label .wineconf.can54.lab65 \
        -background #fefefe \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text label -textvariable WC_parallelSect 
    listbox .wineconf.can54.lis67 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
	-yscrollcommand {.wineconf.can54.scr71 set }
    listbox .wineconf.can54.lis68 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* 
    button .wineconf.can54.but69 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text button -textvariable EDIT \
	-command {TkW:editports lpt}
    button .wineconf.can54.but70 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text button -textvariable CLEAR -state disabled \
	-command {
		set lpt([expr $sel_port + 1]) " "
		TkW:setboxes 
		.wineconf.can54.but70 configure -state disabled
	}
    scrollbar .wineconf.can54.scr71 \
        -borderwidth 1 -orient vert -width 10 \
	-command {.wineconf.can54.lis67 yview}
     button .wineconf.can54.help \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text button -textvariable HELP\
	-command {TkW:message $WC_helponPP}

#sections spy et Tweak.Layout
    canvas .wineconf.can55 \
        -background #fedcdc -borderwidth 2 -height 207 -relief ridge \
        -width 296 
    label .wineconf.can55.lab72 \
        -background #fefefe \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text label -textvariable WC_logSect 
    entry .wineconf.can55.ent74 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
	-textvariable wine_logfile
    button .wineconf.can55.but75 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text button  -textvariable WC_exclude
    label .wineconf.can55.lab76 \
        -background #fefefe \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -relief groove -text label -textvariable WC_wineLook
    menubutton .wineconf.can55.men78 \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        -menu .wineconf.can55.men78.m -padx 4 -pady 3 -text menu \
        -textvariable winelook 
    menu .wineconf.can55.men78.m \
        -cursor {} -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* 
    .wineconf.can55.men78.m add command \
        -command {set winelook "Win31"} -label {Win 3.1} 
    .wineconf.can55.men78.m add command \
        -command {set winelook "Win95"} -label win95/98 
    .wineconf.can55.men78.m add command \
        -command {set winelook "Win98"} -label {Win 98} 
    button .wineconf.can55.help \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text button  -textvariable HELP \
	-command {TkW:message $WC_helponmisc}

#main window buttons:

    button .wineconf.but80 \
        -command {
		set WAIT wait
		TkW:message2 $WC_quit $WC_quitreally $WC_noquit 
		while {$WAIT == "wait"} {update}
		if {$WAIT == "opt1"} { if {[winfo exists .desktop]} {
						destroy .wineconf
						set WAITconf 0
						return
					} else {
						exit
					}
				}
	} \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text button -textvariable QUIT 
    button .wineconf.save \
        -command TkW:writewinerc \
        -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* -padx 9 \
        -pady 3 -text button -textvariable SAVE 
    ###################
    # SETTING GEOMETRY
    ###################
    place .wineconf.lab18 \
        -x 3 -y 5 -width 453 -height 20 -anchor nw -bordermode ignore 

#placing drives sctions widgets:
    place .wineconf.can01 \
        -x 5 -y 35 -width 466 -height 180 -anchor nw -bordermode ignore 
    place .wineconf.can01.lab22 \
        -x 0 -y 0 -width 443 -height 25 -anchor nw -bordermode ignore 
    place .wineconf.can01.lis23 \
        -x 10 -y 56 -width 130 -height 113 -anchor nw -bordermode ignore 
    place .wineconf.can01.lis24 \
        -x 139 -y 55 -width 85 -height 113 -anchor nw -bordermode ignore 
    place .wineconf.can01.lis25 \
        -x 223 -y 55 -width 65 -height 113 -anchor nw -bordermode ignore 
    place .wineconf.can01.lis26 \
        -x 288 -y 55 -width 65 -height 113 -anchor nw -bordermode ignore 
    place .wineconf.can01.scrDrives \
        -x 353 -y 55 -width 16 -height 113 -anchor nw -bordermode ignore 
# bind lis23-26
	bind  .wineconf.can01.lis23 <ButtonRelease> {
		set sel_drive [ .wineconf.can01.lis23 nearest %y]
		#enable related button
		.wineconf.can01.but30 configure -state normal
		.wineconf.can01.but29 configure -state normal
		# disable the tow CLEAR buttons of ports:
		.wineconf.can54.but70 configure -state disabled
		.wineconf.can53.but64 configure -state disabled
	}
	bind  .wineconf.can01.lis24 <ButtonRelease> {
		set sel_drive [ .wineconf.can01.lis23 nearest %y]
		 .wineconf.can01.lis23 selection set $sel_drive
		.wineconf.can01.but30 configure -state normal
		.wineconf.can01.but29 configure -state normal
		# disable the tow CLEAR buttons of ports:
		.wineconf.can54.but70 configure -state disabled
		.wineconf.can53.but64 configure -state disabled
	}
	bind  .wineconf.can01.lis25 <ButtonRelease> {
		set sel_drive [ .wineconf.can01.lis23 nearest %y]
		 .wineconf.can01.lis23 selection set $sel_drive
		.wineconf.can01.but30 configure -state normal
		.wineconf.can01.but29 configure -state normal
		# disble the tow CLEAR buttons of ports:
		.wineconf.can54.but70 configure -state disabled
		.wineconf.can53.but64 configure -state disabled
	}
	bind  .wineconf.can01.lis26 <ButtonRelease> {
		set sel_drive [ .wineconf.can01.lis23 nearest %y]
		 .wineconf.can01.lis23 selection set $sel_drive
		.wineconf.can01.but30 configure -state normal
		.wineconf.can01.but29 configure -state normal
		# disble the tow CLEAR buttons of ports:
		.wineconf.can54.but70 configure -state disabled
		.wineconf.can53.but64 configure -state disabled
	}
    place .wineconf.can01.but28 \
        -x 375 -y 44 -width 60 -height 28 -anchor nw -bordermode ignore 
    place .wineconf.can01.but29 \
        -x 375 -y 76 -width 60 -height 28 -anchor nw -bordermode ignore 
    place .wineconf.can01.but30 \
        -x 375 -y 110 -width 60 -height 28 -anchor nw -bordermode ignore 
    place .wineconf.can01.lab31 \
        -x 13 -y 36 -width 128 -height 20 -anchor nw -bordermode ignore 
    place .wineconf.can01.lab32 \
        -x 140 -y 36 -width 83 -height 20 -anchor nw -bordermode ignore 
    place .wineconf.can01.lab33 \
        -x 223 -y 36 -width 68 -height 20 -anchor nw -bordermode ignore 
    place .wineconf.can01.lab34 \
        -x 290 -y 36 -width 68 -height 20 -anchor nw -bordermode ignore 
    place .wineconf.can01.but51 \
        -x 375 -y 141 -width 60 -height 28 -anchor nw -bordermode ignore 

#placing wine sctions widgets:

    place .wineconf.can37 \
        -x 5 -y 230 -width 449 -height 115 -anchor nw -bordermode ignore 
    place .wineconf.can37.lab39 \
        -x 3 -y 1 -width 443 -height 20 -anchor nw -bordermode ignore 
    place .wineconf.can37.lab40 \
        -x 7 -y 29 -width 58 -height 20 -anchor nw -bordermode ignore 
    place .wineconf.can37.lab41 \
        -x 7 -y 55 -width 58 -height 20 -anchor nw -bordermode ignore 
    place .wineconf.can37.but42 \
        -x 7 -y 84 -width 248 -height 20 -anchor nw -bordermode ignore 
    place .wineconf.can37.but43 \
        -x 375 -y 80 -width 60 -height 28 -anchor nw -bordermode ignore 
    place .wineconf.can37.ent45 \
        -x 74 -y 26 -width 150 -height 24 -anchor nw -bordermode ignore 
    place .wineconf.can37.ent46 \
        -x 74 -y 52 -width 150 -height 24 -anchor nw -bordermode ignore 
    place .wineconf.can37.lab47 \
        -x 239 -y 29 -width 58 -height 20 -anchor nw -bordermode ignore 
    place .wineconf.can37.lab48 \
        -x 239 -y 54 -width 58 -height 20 -anchor nw -bordermode ignore 
    place .wineconf.can37.ent49 \
        -x 304 -y 26 -width 135 -height 24 -anchor nw -bordermode ignore 
    place .wineconf.can37.ent50 \
        -x 304 -y 52 -width 135 -height 24 -anchor nw -bordermode ignore 

#placing serialports sections widgets:

    place .wineconf.can53 \
        -x 9 -y 356 -width 144 -height 180 -anchor nw -bordermode ignore 
    place .wineconf.can53.lab57 \
        -x 3 -y 3 -width 138 -height 20 -anchor nw -bordermode ignore 
    place .wineconf.can53.lis58 \
        -x 3 -y 25 -width 40 -height 83 -anchor nw -bordermode ignore 
    place .wineconf.can53.lis59 \
        -x 44 -y 25 -width 85 -height 83 -anchor nw -bordermode ignore 
# bind lis59 to select lis58 at the same time...
	bind  .wineconf.can53.lis58 <ButtonRelease> {
		set sel_com [ .wineconf.can53.lis58 nearest %y]
		 .wineconf.can53.lis58 selection set $sel_com
		#disbale other list related buttons:
		.wineconf.can01.but30 configure -state disabled
		.wineconf.can01.but29 configure -state disabled
		# disable/enable the tow CLEAR buttons of ports:
		.wineconf.can54.but70 configure -state disabled
		.wineconf.can53.but64 configure -state normal
	}
# bind lis58 to select lis59 at the same time...
	bind  .wineconf.can53.lis59 <ButtonRelease> {
		set sel_com [ .wineconf.can53.lis59 nearest %y]
		 .wineconf.can53.lis58 selection set $sel_com
		#disbale other list related buttons:
		.wineconf.can01.but30 configure -state disabled
		.wineconf.can01.but29 configure -state disabled
		# disable/enable the tow CLEAR buttons of ports:
		.wineconf.can54.but70 configure -state disabled
		.wineconf.can53.but64 configure -state normal
	}
    place .wineconf.can53.scr60 \
        -x 123 -y 26 -width 16 -height 80 -anchor nw -bordermode ignore 
    place .wineconf.can53.but63 \
        -x 10 -y 115 -width 60 -height 28 -anchor nw -bordermode ignore 
    place .wineconf.can53.but64 \
        -x 78 -y 115 -width 60 -height 28 -anchor nw -bordermode ignore 
    place .wineconf.can53.help \
        -x 10 -y 145 -width 130 -height 28 -anchor nw -bordermode ignore 

#placing parallelports sections widgets:

    place .wineconf.can54 \
        -x 160 -y 356 -width 144 -height 180 -anchor nw -bordermode ignore 
    place .wineconf.can54.lab65 \
        -x 1 -y 3 -width 143 -height 20 -anchor nw -bordermode ignore 
    place .wineconf.can54.lis67 \
        -x 5 -y 26 -width 45 -height 83 -anchor nw -bordermode ignore 
    place .wineconf.can54.lis68 \
        -x 50 -y 25 -width 75 -height 83 -anchor nw -bordermode ignore 
# bind lis67 
	bind  .wineconf.can54.lis68 <ButtonRelease> {
		set sel_port [ .wineconf.can53.lis59 nearest %y]
		#disbale other list related buttons:
		.wineconf.can01.but30 configure -state disabled
		.wineconf.can01.but29 configure -state disabled
		# disable/enable the tow CLEAR buttons of ports:
		.wineconf.can54.but70 configure -state normal
		.wineconf.can53.but64 configure -state disabled
	}
# bind lis68 to select lis67 at the same time...
	bind  .wineconf.can54.lis67 <ButtonRelease> {
		set sel_port [ .wineconf.can53.lis59 nearest %y]
		 .wineconf.can54.lis67 selection set $sel_port
		#disbale other list related buttons:
		.wineconf.can01.but30 configure -state disabled
		.wineconf.can01.but29 configure -state disabled
		# disable/enable the tow CLEAR buttons of ports:
		.wineconf.can54.but70 configure -state normal
		.wineconf.can53.but64 configure -state disabled
	}
    place .wineconf.can54.but69 \
        -x 10 -y 115 -width 60 -height 28 -anchor nw -bordermode ignore 
    place .wineconf.can54.but70 \
        -x 78 -y 115 -width 60 -height 28 -anchor nw -bordermode ignore 
    place .wineconf.can54.scr71 \
        -x 123 -y 26 -width 16 -height 80 -anchor nw -bordermode ignore 
    place .wineconf.can54.help \
        -x 10 -y 145 -width 130 -height 28 -anchor nw -bordermode ignore 

#placing spy and Tweak.Layout sections widgets:

    place .wineconf.can55 \
        -x 313 -y 357 -width 139 -height 180 -anchor nw -bordermode ignore 
    place .wineconf.can55.lab72 \
        -x 3 -y 3 -width 133 -height 20 -anchor nw -bordermode ignore 
    place .wineconf.can55.ent74 \
        -x 5 -y 25 -width 130 -height 24 -anchor nw -bordermode ignore 
    place .wineconf.can55.but75 \
        -x 5 -y 50 -width 130 -height 28 -anchor nw -bordermode ignore 
    place .wineconf.can55.lab76 \
        -x 3 -y 83 -width 133 -height 20 -anchor nw -bordermode ignore 
    place .wineconf.can55.men78 \
        -x 10 -y 110 -width 122 -height 24 -anchor nw -bordermode ignore 
    place .wineconf.can55.help \
        -x 5 -y 145 -width 130 -height 28 -anchor nw -bordermode ignore 

#Main window buttons:

    pack .wineconf.but80 -padx 5 -pady 1 -side bottom -fill x
    pack .wineconf.save -padx 5 -side bottom -fill x

# initialise the arrays
	TkW:initarrays
# Now start reading the .winerc (or the default wine.ini)
	TkW:readwinerc $DefaultConfFile
# Set the listboxes:
	TkW:setboxes
#updates the scollings
	set WAITconf 1
	while {$WAITconf == 1} { TkW:scrollwindows }
}

proc TkW:readwinerc {DefaultConfFile} {
	global sect ndrives drive n rcout line wine_logfile winelook
	global nspool nalias srcdest TKW

TkW:debug "Entering readwinerc -----------------------------"

# OPEN THE FILE Read-Only:
TkW:fixme "TkW:wineconf::\
Detection of wine.ini will fail when ~/.winerc ~/wine.conf not there"
	if {[file exist $DefaultConfFile] } then {
		set winerc [open $DefaultConfFile r]
	} else {
		TkW:message "No file $DefaultConfFile... This is a TkWine error"
		return
	}
# Adds a fe setting in case they're missing (actually, they are in the std file) in the wine.ini
set winelook Win95
set wine_logfile /tmp/winelog

# OPEN /tmp/tkwwinerc for writting
	set rcout [open /tmp/tkwinerc w]

	set ndrives -1
	set nspool 0
	set nalias -1
	global NDllOver NDllPairs DllPairs DllOver
	set NDllOver 0
	set NDllPairs 0

# Read line by line
	set charread 0
	while {$charread != -1} {
		
		set charread [gets $winerc line]
		set line [string trim $line]
		set n [expr $charread - 1]
		set idok 0
		TkW:debug "READ:$line-$charread"
		if {[string match ";*" $line]} {
			#puts $rcout $line
			TkW:debug Comment; set idok 1}
 # The two lines below SHOULD work... they don't
		#if {[string match "\[*\]" $line]} \
		#{TkW:debug Section;set idok 1}
 ## a ugly replacement:
		if {"[string range $line 0 0][string range $line $n $n]"\
			 == "\[\]"} {
			TkW:sections
			set idok 1
		}
		if {$idok != 1 && [string match "*=*" $line]} {
			TkW:assign
			TkW:debug Assignment
			set idok 1
		}
		if {!$idok && $charread > 0} {
			TkW:debug "Error::Unknow line in .winerc"
			puts "************Warning: Error in config \
				file ****************"
			puts "Line $line incomprehensible"
		}
	}
}

proc TkW:sections {} {
	global sect ndrives drive n rcout line
	global drivepath drivelabel drivetype drivefstype driveserial drivedevice
	set sect [string tolower [string range $line 1 [expr $n - 1]]]
	if {[string tolower [string range $sect 0 4]] == "drive"} {
		set ndrives [expr $ndrives + 1]
		set drive($ndrives) [string range $sect 6 6]
		set drivepath($ndrives) ""
		set drivelabel($ndrives) ""
		set drivetype($ndrives) ""
		set drivefstype($ndrives) ""
		set driveserial($ndrives) ""
		set drivedevice($ndrives) ""
		set sect drive
		TkW:debug "Section $sect $drive($ndrives)"
		#puts $rcout "\[$sect $drive($ndrives)\]"
	} else {
		TkW:debug "Section $sect"
		#puts $rcout "\[$sect]"
	}
}

proc TkW:assign {} {
	global sect ndrives drive n rcout line

	global windowspath varpath syspath tmppath symbols winelook
	global ndrives drive drivepath drivetype drivefstype drivelabel
	global drivedevice driveserial
	global resolution defaultfont 
	global readport writeport
	global exclude default_progs startup allocsyscolors winelook wine_logfile
	global com lpt spool nspool alias nalias
	global NDllOver NDllPairs
	global DllOver DllPairs 

	set equalis  [string first "=" $line ]
	set varname [string tolower [string trim [string range $line 0 [expr $equalis - 1]]]]
	set value [string trim [string range $line [expr $equalis + 1] $n]]
	if {$varname == "default"} {set varname Default}
	switch $varname {
		{path} {
			 switch  $sect  {
				{drive} {set drivepath($ndrives) $value}
				{wine} {set varpath $value;update}
				{default} {TkW:conferror $sect $varname $value}
			 }
		       }
		{type} {
				if {$sect != "drive"} then {
					TkW:conferror $sect $varname $value
				} else {
					set drivetype($ndrives) $value
				}
			}
		{label} {
				if {$sect != "drive"} then {
                                	TkW:conferror $sect $varname $value
                        	} else {
                                	set drivelabel($ndrives) $value
                        	} 
			}
		{serial} {
				if {$sect != "drive"} then {
                                	TkW:conferror $sect $varname $value
                        	} else {
                                	set driveserial($ndrives) $value
                        	} 
			}
		{filesystem} {
				if {$sect != "drive"} then {
                                	TkW:conferror $sect $varname $value
                        	} else {
                                	set drivefstype($ndrives) $value
                        	} 
			     }
		{device} {
				if {$sect != "drive"} then {
                                	TkW:conferror $sect $varname $value
                        	} else {
                                	set drivedevice($ndrives) $value
                        	} 
			}
		{windows} {
				if {$sect != "wine"} then {
                               	 TkW:conferror $sect $varname $value
                        	} else {
                               	 set windowspath $value
                        	} 
			}
		{system} {
				if {$sect != "wine"} then {
                                	TkW:conferror $sect $varname $value
                        	} else {
                                	set syspath $value
                        	} 
			}
		{temp} {
				if {$sect != "wine"} then {
                                	TkW:conferror $sect $varname $value
                        	} else {
                                	set tmppath $value
                        	} 
			}
		{symboltablefile} {
					if {$sect != "wine"} then {
                                		TkW:conferror $sect\
							 $varname $value
                        		} else {
                                		set symbols $value
                        		} 
				}
		{resolution} {
				if {$sect != "fonts"} then {
                                	TkW:conferror $sect $varname $value
                        	} else {
                                	set resolution $value
                        	}
			     }
		{Default} {
				switch $sect {
					{fonts} { set defaultfont $value}
					{programs} {set default_progs $value}
                        		{default} { TkW:conferror \
							$sect $varname $value }
				}
			}
		{read} {
				if {$sect != "ports"} then {
                               		TkW:conferror $sect $varname $value
                        	} else {
                                	set readport $value
                        	}    
			}
		{write} {
				if {$sect != "ports"} then {
                                	TkW:conferror $sect $varname $value
                        	} else {
                                	set writeport $value
                        	}    
			}
		{file} {
				if {$sect != "spy"} then {
                                	TkW:conferror $sect $varname $value
                        	} else {
                                	set wine_logfile $value
                        	}    
			}
		{exclude} {
				if {$sect != "spy"} then {
                                	TkW:conferror $sect $varname $value
                        	} else {
                                	set Exclude $value
                        	}    
			}
		{allocsystemcolors} {
					if {$sect != "options"} then {
                                		TkW:conferror $sect $varname\
									 $value
                        		} else {
                                		set allocsyscolors $value
                        	   	} 
				   }
		{winelook} {
					if {$sect != "tweak.layout"} then {
                                		TkW:conferror $sect $varname\
									 $value
                        		} else {
                                		set winelook $value
                        	   	} 
				   }
		{startup} {
					if {$sect != "programs"} then {
                                		TkW:conferror $sect $varname\
									 $value
                        		} else {
                                		set startup $value
                        	   	} 
				   }
		{printer} {
					if {$sect != "wine"} then {
                                		TkW:conferror $sect $varname\
									 $value
                        		} else {
                                		set printer $value
                        	   	} 
				   }
		{default} {
		    	if {$sect == "dllpairs"} {
				set NDllPairs [expr $NDllPairs + 1]
				set DllPairs($NDllPairs) "$varname 	= $value"
				return
		    	} 
		    	if {$sect == "dlloverrides"} {
				set NDllOver [expr $NDllOver + 1]
				set DllOver($NDllOver) "$varname 	= $value"
				return
		    	} 
			if {[string tolower [string trim [string range $line 0 2]]] == "lpt" &&  [string range $line 4 4 ] == ":" } {
				if {$sect != "spooler"} then {
                                           TkW:conferror $sect $varname $value
                                 } else {
					set nspool [expr $nspool + 1]
 		                        set spool($nspool) $value
       	                         }
				return
			}

			set varname2 [string tolower [string trim \
				[string range $line 0 [expr $equalis - 2]]]]
			set letter [string tolower [string range $line [expr $equalis - 1]\
				 [expr $equalis - 1] ]]
			#set varname2 [string tolower $varname2]
			switch $varname2 {
	
				{com} {
					if {$sect != "serialports"} then {
                                                TkW:conferror $sect $varname\
                                                                         $value
                                        } else {
                                                set com($letter) $value
                                        }
                                      }        
				{lpt} {
					if {$sect != "parallelports"} then {
                                                TkW:conferror $sect $varname\
                                                                         $value
                                        } else {
                                                set lpt($letter) $value
                                        }
                                      }        

				{alias} {
					if {$sect != "fonts"} then {
                                                TkW:conferror $sect $varname\
                                                                         $value
                                        } else {
						set nalias [expr $nalias + 1]
                                                set alias($nalias) $value
                                        }
                                      }        
				{default} {
					TkW:conferror $sect $varname $value
				}
					

			}
			
		
		}
	}
}

proc TkW:conferror {sect varname value} {

	puts "ERROR IN CONFIG FILE, FOUND $varname=$value in section $sect"
}

proc TkW:setboxes {} {
	
	global ndrives drive drivepath drivetype drivefstype drivelabel
	global drivedevice driveserial
	global com lpt

	.wineconf.can01.lis23 delete 0 end
	.wineconf.can01.lis24 delete 0 end
	.wineconf.can01.lis25 delete 0 end
	.wineconf.can01.lis26 delete 0 end

	for {set i 0} {$i <= $ndrives } {set i [expr $i + 1]} {

		TkW:debug "Drive $i : $drive($i) $drivepath($i) $drivetype($i)\
					$drivelabel($i)"
		TkW:debug "           $drivefstype($i) $drivedevice($i) $driveserial($i)"
		
		set yes " "
		if {![file isdirectory $drivepath($i)]} {set yes "*"}
		.wineconf.can01.lis23 insert end \
			"$yes\($drive($i):\\) $drivepath($i) "
		.wineconf.can01.lis24 insert end \
			"$yes$drivetype($i)"
		.wineconf.can01.lis25 insert end \
			"$yes$drivelabel($i)"
		.wineconf.can01.lis26 insert end \
			"$yes$drivefstype($i)"
	}

	.wineconf.can53.lis59 delete 0 end
	.wineconf.can53.lis58 delete 0 end
	for {set i 1} {$i <= 8 } {set i [expr $i + 1]} {
		.wineconf.can53.lis58 insert end "com$i"
		.wineconf.can53.lis59 insert end $com($i)
	}
	.wineconf.can54.lis67 delete 0 end
	.wineconf.can54.lis68 delete 0 end
	for {set i 1} {$i <= 8 } {set i [expr $i + 1]} {
		.wineconf.can54.lis67 insert end "lpt$i"
		.wineconf.can54.lis68 insert end $lpt($i)
	}

}

proc TkW:scrollwindows {} {

	global OldPos 

	if {![info exists OldPos]} {set OldPos 0}

# Serial
	set lis58pos [.wineconf.can53.lis59 yview]; set a 1
	foreach i $lis58pos { if {$a} {set pos $i};  set a 0 }
	.wineconf.can53.lis58 yview moveto $pos
# // ports
	set lis67pos [.wineconf.can54.lis67 yview]; set a 1
	foreach i $lis67pos { if {$a} {set pos $i};  set a 0 }
	.wineconf.can54.lis68 yview moveto $pos
# drives:
	set lis23pos [.wineconf.can01.lis23 yview]; set a 1
	foreach i $lis23pos { if {$a} {set pos $i};  set a 0 }
	if {$lis23pos != $OldPos} {
		TkW:debug  "listbox23 has move from $OldPos to $lis23pos"
		TkW:debug  "listbox24 goes to $pos "
	}
	set OldPos $lis23pos
	.wineconf.can01.lis24 yview moveto $pos
	.wineconf.can01.lis25 yview moveto $pos
	.wineconf.can01.lis26 yview moveto $pos

	update
}

proc TkW:initarrays {} {

	global com lpt spool

	for {set i 1 } {$i <= 8 } {set i [expr $i + 1]} {
		set com($i) ""
		set lpt($i) ""
	}
	for {set i 1 } {$i <= 8 } {set i [expr $i + 1]} {
		set spool($i) ""
	}
}

proc TkW:editports {comlpt} {
	
	global OK CANCEL com portch lpt lptcom win banner TKW
	global WC_confserial WC_confparallel  WC_PPbanner WC_serialbanner

	.wineconf.can54.but69 configure -state disabled
	.wineconf.can53.but63 configure -state disabled

# this is a ugly trick as buttons command need a global variable, not an arg
	set lptcom $comlpt

	if {$comlpt == "com"} then {
		set win .comedit
		set title $WC_confserial
		set banner $WC_serialbanner
	} else {
		set win .lptedit
		set title $WC_confparallel
		set banner $WC_PPbanner
	}
    toplevel $win -class Toplevel \
        -background #3c1cfe 
    wm focusmodel $win passive
    wm geometry $win 205x365+448+253
    wm maxsize $win 1265 994
    wm minsize $win 1 1
    wm overrideredirect $win 0
    wm resizable $win 1 1
    wm deiconify $win
    wm title $win $title
    #wm iconbitmap $win @$TKW/setupfiles/images/setup2.xbm
    #wm iconmask   $win @$TKW/setupfiles/images/setupmask2.xbm 

# creating the Banner
	
	label "$win.banner" \
        	-background #dc96dc \
        	-font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
        	-relief groove -text $banner

# creating the labels and entries

	for {set i 1} {$i <= 8} {set i [expr $i + 1 ]} {

		set portch($i) $com($i)
		if {$comlpt == "com"} then {
			set portch($i) $com($i) 
		} else {
			set portch($i) $lpt($i) 
		}

    		label "$win.lab$i" \
        	     -background #dc96dc \
        	     -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
        	     -relief groove -text "Com$i"
    		entry "$win.ent$i" \
        	     -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
		     -textvariable portch($i)
		set Y [expr 55 + $i * 30]
    		place  "$win.lab$i"\
        		-x 5 -y $Y -width 45 -height 28 \
			-anchor nw -bordermode ignore 
    		place  "$win.ent$i"\
        		-x 55 -y $Y -width 145 -height 28 \
			-anchor nw -bordermode ignore 
	}

    	button $win.but80 \
        	-command {
			for {set i 1} {$i <= 8} {set i [expr $i + 1]} {
				if {$lptcom == "com"} then {
					set com($i) $portch($i)
				} else {
					set lpt($i) $portch($i)
				}
			.wineconf.can54.but69 configure -state normal
			.wineconf.can53.but63 configure -state normal
			}
			destroy $win
			TkW:setboxes
		} \
        	-font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
		 -padx 9  -pady 3 -text button -textvariable OK 
	place $win.banner -x 5 -y 5 -width 195 -height 70

	place $win.but80 \
		-x 5 -y 335 -width 100 -height 28 \
		-anchor nw -bordermode ignore 
    	button $win.but81 \
        	-command {destroy $win
			.wineconf.can54.but69 configure -state normal
			.wineconf.can53.but63 configure -state normal
		}\
        	-font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
		 -padx 9  -pady 3 -text button -textvariable CANCEL \

	place $win.but81 \
		-x 105 -y 335 -width 100 -height 28 \
		-anchor nw -bordermode ignore 

}

proc TkW:drives {n} {

	global ndrives drive drivepath drivetype drivefstype drivelabel
	global drivedevice driveserial TKW
	global WC_confdrives HELP WCHLP WC_driveSetBanner
	global chdrives1 chdrives2 chdrives3 chdrives4 
	global chdrives5 chdrives6 chdrives7 
	global win i num ch drOK

	set chdrives(1) $chdrives1
	set chdrives(2) $chdrives2
	set chdrives(3) $chdrives3
	set chdrives(4) $chdrives4
	set chdrives(5) $chdrives5
	set chdrives(6) $chdrives6
	set chdrives(7) $chdrives7


	if {$n == "new"} then {
		set num [expr $ndrives + 1]
		set ch(1) ""
		set ch(2) ""
		set ch(3) ""
		set ch(4) ""
		set ch(5) ""
		set ch(6) ""
		set ch(7) ""
	} else {
		set num $n
		set ch(1) $drive($num)
		set ch(2) $drivepath($num)
		set ch(3) $drivetype($num)
		set ch(4) $drivefstype($num)
		set ch(5) $drivelabel($num)
		set ch(6) $drivedevice($num)
		set ch(7) $driveserial($num)
	}

	TkW:debug "Entering TkW:drives - drive number: $num"

	.wineconf.can01.but28 configure -state disabled
	.wineconf.can01.but29 configure -state disabled
	.wineconf.can01.but30 configure -state disabled
	# re-enable the tow EDIT buttons of ports:
	.wineconf.can54.but69 configure -state disabled
	.wineconf.can53.but63 configure -state disabled


	set win .drivedit
	set title $WC_confdrives

    toplevel $win -class Toplevel \
        -background #3c1cfe 
    wm focusmodel $win passive
    wm geometry $win 305x325+448+253
    wm maxsize $win 1265 994
    wm minsize $win 1 1
    wm overrideredirect $win 0
    wm resizable $win 1 1
    wm deiconify $win
    wm title $win $title
    #wm iconbitmap $win @$TKW/setupfiles/images/setup2.xbm
    #wm iconmask   $win @$TKW/setupfiles/images/setupmask2.xbm

# setting the banner:

    		label "$win.banner" \
        	     -background #dc96dc \
        	     -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
        	     -relief groove -text $WC_driveSetBanner -height 35


	
# creating the labels and entries

# setting HELP buttons
		button $win.hlp1\
        		-font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
 			-padx 9 -pady 3 -text button -textvariable HELP \
			-command {TkW:message $WCHLP(1)}
		button $win.hlp2\
        		-font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
 			-padx 9 -pady 3 -text button -textvariable HELP \
			-command {TkW:message $WCHLP(2)}
		button $win.hlp3\
        		-font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
 			-padx 9 -pady 3 -text button -textvariable HELP \
			-command {TkW:message $WCHLP(3)}
		button $win.hlp4\
        		-font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
 			-padx 9 -pady 3 -text button -textvariable HELP \
			-command {TkW:message $WCHLP(4)}
		button $win.hlp5\
        		-font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
 			-padx 9 -pady 3 -text button -textvariable HELP \
			-command {TkW:message $WCHLP(5)}
		button $win.hlp6\
        		-font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
 			-padx 9 -pady 3 -text button -textvariable HELP \
			-command {TkW:message $WCHLP(6)}
		button $win.hlp7\
        		-font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
 			-padx 9 -pady 3 -text button -textvariable HELP \
			-command {TkW:message $WCHLP(7)}


	for {set i 1} {$i <= 7} {set i [expr $i + 1 ]} {


    		label "$win.lab$i" \
        	     -background #dc96dc \
        	     -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
        	     -relief groove -text $chdrives($i)
		if {$i < 3 || $i > 4} then {
    		   entry "$win.ent$i" \
        	     -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
		     -text text -textvariable ch($i)
		} else {
		    menubutton $win.ent$i \
        		-font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-* \
        		-menu $win.ent$i.m -padx 4 -pady 3 \
        		-text text -textvariable ch($i)
		}
# settings the menus:
		if {$i == 3} then {
    			menu $win.ent$i.m \
        			-cursor {} \
				-font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*
    			$win.ent$i.m add command \
        			-command {set ch(3) "hd"} -label {Hard Drive}
    			$win.ent$i.m add command \
        			-command {set ch(3) "floppy"} -label {Floppy}
    			$win.ent$i.m add command \
        			-command {set ch(3) "cdrom"} -label {CDrom} 
    			$win.ent$i.m add command \
        			-command {set ch(3) "network"} \
				-label {network} 
		}
		if {$i == 4} then {
    			menu $win.ent$i.m \
        			-cursor {} \
				-font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*
    			$win.ent$i.m add command \
        			-command {set ch(4) "msdos"} -label {msdos}
    			$win.ent$i.m add command \
        			-command {set ch(4) "win95"} -label {Win95}
    			$win.ent$i.m add command \
        			-command {set ch(4) "Unix"} -label {Unix} 
		}


		set Y [expr 40 + $i * 30]
    		place  "$win.lab$i"\
        		-x 5 -y $Y -width 145 -height 28 \
			-anchor nw -bordermode ignore 
    		place  "$win.ent$i"\
        		-x 150 -y $Y -width 85 -height 28 \
			-anchor nw -bordermode ignore 
    		place  "$win.hlp$i"\
        		-x 235 -y $Y -width 55 -height 28 \
			-anchor nw -bordermode ignore 
		
	}


    	button $win.but80 \
        	-command {
			TkW:checkdrives
			if {$drOK} {
				set drive($num) $ch(1)
				set drivepath($num) $ch(2)
				set drivetype($num) $ch(3)
				set drivefstype($num) $ch(4)
				set drivelabel($num) $ch(5)
				set drivedevice($num) $ch(6)
				set driveserial($num) $ch(7) 
				destroy $win
				if {$num > $ndrives} {set  ndrives $num}
				TkW:setboxes
				.wineconf.can01.but28 configure -state normal
				# re-enable the tow EDIT buttons of ports:
				.wineconf.can54.but69 configure -state normal
				.wineconf.can53.but63 configure -state normal
			}
		} \
        	-font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
		 -padx 9  -pady 3 -text button -textvariable OK \

	place $win.but80 \
		-x 5 -y 290 -width 145 -height 28 \
		-anchor nw -bordermode ignore 
    	button $win.but81 \
        	-command {
			destroy $win
			.wineconf.can01.but28 configure -state normal
			#.wineconf.can01.but29 configure -state normal
			#.wineconf.can01.but30 configure -state normal
			# re-enable the tow EDIT buttons of ports:
			#.wineconf.can54.but69 configure -state normal
			#.wineconf.can53.but63 configure -state normal
		}\
        	-font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
		 -padx 9  -pady 3 -text button -textvariable CANCEL \

	place $win.but81 \
		-x 150 -y 290 -width 145 -height 28 \
		-anchor nw -bordermode ignore 
	place $win.banner -width 290 -height 35 -x 5 -y 5 

}
proc TkW:checkdrives {} {
	global ndrives ch num drOK drive
	global WC_givepath WC_oneletter WC_usedletter WC_11letters

		if {[string length $ch(1)] != 1} {
			TkW:message "$ch(1) $WC_oneletter"
			set drOK 0
			return
		}
		if {[string length $ch(5)] > 11} {
			TkW:message "$ch(5) $WC_11letters"
			set drOK 0
			return
		}
		for {set i 0} {$i <=$ndrives} {set i [expr $i + 1]} {
			if {$i != $num  && [string tolower $ch(1)]\
					 == [string tolower $drive($i)] } {
 				TkW:message "$ch(1)$WC_usedletter" 
				set drOK 0
 				return
			}
		}
		if { $ch(2) == ""} {TkW:message $WC_givepath; set drOK 0;return}
		if { $ch(3) == ""} {set ch(3) hd}
		if { $ch(4) == ""} {set ch(4) unix}
		if { $ch(5) == ""} {set ch(5) "Drive $ch(1)"}
		set drOK 1
}

proc TkW:editWinePath {} {
	global OK varpath WC_editPathBanner WC_pathtitle TKW
	TkW:debug "Entering TkW:editWinePath with path set to $varpath"
	global ndrives dri
	
	# Create the toplevel
    toplevel .editpath -class Toplevel \
        -background #3c1cfe 
    wm focusmodel .editpath passive
    wm geometry .editpath 410x100+448+253
    wm maxsize .editpath 1265 994
    wm minsize .editpath 1 1
    wm overrideredirect .editpath 0
    wm resizable .editpath 1 1
    wm deiconify .editpath
    wm title .editpath $WC_pathtitle 
    #wm iconbitmap .editpath @$TKW/setupfiles/images/setup2.xbm
    #wm iconmask   .editpath @$TKW/setupfiles/images/setupmask2.xbm

	#Creates the Banner
    		label .editpath.banner \
        	     -background #dc96dc \
        	     -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
        	     -relief groove -text $WC_editPathBanner
 	#Creates the entry box
		entry .editpath.ent \
        	     -background #dc96dc \
        	     -font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
        	     -relief groove -textvariable varpath
	#Creates OK button
		button .editpath.ok\
		     	-background #dc96dc \
			-font -Adobe-Helvetica-Medium-R-Normal-*-*-120-*-*-*-*-*-*\
			-relief groove -text $OK -command {destroy .editpath}

	place .editpath.banner -x 5 -y 5 -width 400 -height 30
	place .editpath.ent -x 5 -y 40 -width 400 -height 30
	place .editpath.ok -x 5 -y 70 -width 400 -height 30
			
}

proc TkW:writewinerc {} {

	global windowspath varpath syspath tmppath symbols winelook
	global ndrives drive drivepath drivetype drivefstype drivelabel
	global drivedevice driveserial
	global resolution defaultfont HOME
	global readport writeport
	global exclude default_progs startup allocsyscolors winelook wine_logfile
	global com lpt spool nspool alias nalias
	global NDllPairs NDllOver DllPairs DllOver

# open file for writing:;;

# move the old file:
	if {[file exists ~/.winerc]} {
		set free 1
		while {[file exists ~/.winerc.$free]} {
			set free [expr $free + 1]
		}
		exec mv -f $HOME/.winerc $HOME/.winerc.$free
	}
	set out [open ~/.winerc w]

# writes drives
# Puts the usual comments:
	puts $out ";;"
	puts $out ";; MS-DOS drives configuration "
	puts $out ";;"
	puts $out ";; Each section has the following format:"
	puts $out ";; \[Drive X\]"
	puts $out ";; Path=xxx       (Unix path for drive root)"
	puts $out ";; Type=xxx       (supported types are 'floppy', 'hd', 'cdrom' and 'network')"
	puts $out ";; Label=xxx      (drive label, at most 11 characters)"
	puts $out ";; Serial=xxx     (serial number, 8 characters hexadecimal number)"
	puts $out ";; Filesystem=xxx (supported types are 'msdos','win95','unix')"
	puts $out ";; Device=/dev/xx (only if you want to allow raw device access)"
	puts $out ";;    "
	for {set i 0} {$i <= $ndrives} {set i [expr $i + 1]} {
		puts $out "\[Drive $drive($i)\]"
		puts $out "Path=$drivepath($i)"
		puts $out "Type=$drivetype($i)"
		puts $out "Label=$drivelabel($i)"
		if {$drivefstype($i) == ""} {set drivefstype($i) unix}
		puts $out "Filesystem=$drivefstype($i)"
		if {$driveserial($i) != "" } {puts $out "Serial=$driveserial($i)"}
		if {$drivedevice($i) != "" } {puts $out "Device=$drivedevice($i)"}
		puts $out ""

	}
#write wine section
	puts $out "\[wine\]"
	puts $out "Windows=$windowspath"
	puts $out "System=$syspath"
	puts $out "Temp=$tmppath"
	puts $out "Path=$varpath"
	puts $out "SymbolTableFile=$symbols"
	puts $out ""

# write options
	TkW:fixme "Option not supported in wineconf, write de default"
	puts $out "\[options\]"
	puts $out "AllocSystemColors=$allocsyscolors"
	puts $out ""

# DllPairs:
	puts $out "\[DllPairs\]"
	for {set i 1} {$i <= $NDllPairs} {set i [expr $i + 1]} {
		puts $out $DllPairs($i)
	}
	puts $out
# DllOverrides:
	puts $out "\[DllOverrides\]"
	for {set i 1} {$i <= $NDllOver} {set i [expr $i + 1]} {
		puts $out $DllOver($i)
	}
	puts $out
# write fonts section
	TkW:fixme "fonts support to be added to wineconf...."
	puts $out "\[fonts\]"
	puts $out ";Read documentation/fonts before adding aliases"
	puts $out "Resolution = $resolution"
	puts $out "Default = $defaultfont"
	puts $out ""
# write serial
	puts $out "\[serialports\]"
	for {set i 1} {$i <= 8} {set i [expr $i + 1]} {
		if {$com($i) != ""} {puts $out "Com$i=$com($i)"}
	}
	puts $out ""
# write parallel
	puts $out "\[parallelports\]"
	for {set i 1} {$i <= 8} {set i [expr $i + 1]} {
		if {$lpt($i) != ""} {puts $out "Lpt$i=$lpt($i)"}
	}
	puts $out ""
# write spooler
	puts $out "\[spooler\]"
	for {set i 1} {$i <= 8} {set i [expr $i + 1]} {
		if {$spool($i) != ""} {puts $out "LPT$i:=$spool($i)"}
	}
	puts $out ""
# write ports
	puts $out "\[ports]"
	if {[info exists readport]} {puts $out $readport} {puts $out ";read=0x779,0x379,0x280-0x2a0"}
	if {[info exists writeport]} {puts $out $writeport} {puts $out ";write=0x779,0x379,0x280-0x2a0"}
	puts $out ""
#write spy
	puts $out "\[spy\]"
	puts $out "file=$wine_logfile"
	if {[info exists exclude]} {puts $out "Exclude=$exclude"} {puts $out "Exclude=WM_SIZE;WM_TIMER;"}
	puts $out ""
#write Tweak.Layout
	puts $out "\[Tweak.Layout\]"
	puts $out ";; WineLook=xxx  (supported styles are \'Win31\'(default), \'Win95\', \'Win98\')"
	puts $out "WineLook=Win95"
}
proc TkW:delDrive {n} {

	global ndrives drive drivepath drivetype drivefstype drivelabel
	global drivedevice driveserial

	TkW:debug "Remove drive [expr  $n + 1]"
	for {set i  $n} {$i<$ndrives} {set i [expr $i +1]} {
		set i1 [expr $i + 1]
		set drive($i) $drive($i1)
		set drivepath($i)  $drivepath($i1)
		set drivelabel($i) $drivelabel($i1)
		set drivetype($i) $drivetype($i1)
		set drivefstype($i) $drivefstype($i1)
		set driveserial($i) $driveserial($i1)
	}
	set ndrives [expr $ndrives - 1]
	TkW:setboxes
}
