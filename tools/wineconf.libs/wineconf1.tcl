proc TkW:debug {msg} {
}
proc TkW:fixme {msg} {
}
proc TkW:wineconf1 {} {
	
# propmt the user for a choice of the default config file:
# can be:
# 	Existing {--> menu ~/.winerc or /etc/wineconf or custom}
# 	autogenarated with tools/wineconf
#	built-in the script

# test of existing is preformed first; test of existing wineconf also
# 

	set ExistEtc  [TkW:ListConfig /etc/wine.conf]
	set ExistHome  [TkW:ListConfig "~/.winerc"]

	TkW:AskUserConfMethod $ExistEtc $ExistHome
	tkwait vis .askConf
	while [winfo exists .askConf] {update}
}

proc TkW:ListConfig {fileConf} {

	global HOME

	set ExistConf 0
	if [file writa $fileConf] {return 1} {return 0}
}

proc TkW:AskUserConfMethod {ExistEtc ExistHome} {
	
	global USER ChDefautConf DejaConf HOME OK CANCEL WAITfilename
	global DefaultType EtcState HomeState GiveCustomDefault Default
	global GetFileName GenereConf TkWBuiltConf

	toplevel .askConf -backgr #ffffff
	label .askConf.banner -text $ChDefautConf -backgr #0000ff \
		-foregr #ffffff -height 2
	pack .askConf.banner -side top -fill x
	
	if {$USER == "root" && $ExistEtc == 1} {
		set Default /etc/wine.conf
	} else {
		if {$ExistHome == 1} {set Default "~/.winerc"} \
			else  {set Default Custom}
	}
	if {[file writable /etc/wine.conf]} {set EtcState normal} {set EtcState disabled}
	if {$ExistHome == 1} {set HomeState normal} {set HomeState disabled}
	set DefaultType 0
	frame .askConf.deja -width 200 -backgr #ffffff
	radiobutton .askConf.deja.rad -backgr #dddddd -text $DejaConf\
		-anchor nw -value 0 -variable DefaultType \
		-indicatoron 1 -relief raised -selectcolor #00ff00
	pack  .askConf.deja.rad -side left -padx 5
	menubutton .askConf.deja.men -backgr #dddddd \
		-menu .askConf.deja.men.m -width 50 -text $Default\
		-relief raised

	menu .askConf.deja.men.m -cursor {} -tearoff 0
	.askConf.deja.men.m add command \
		-command  {
			set Default "$HOME/.winerc"
			.askConf.deja.men configure -text $Default
		} -label "~/.winerc" -state $HomeState
	.askConf.deja.men.m add command \
		-command  {
			set Default "/etc/wine.conf"
			.askConf.deja.men configure -text $Default
		} -state $EtcState \
		-label "/etc/wine.conf (System wide configuration)"
	.askConf.deja.men.m add command \
		-command  {
			TkW:GetFileName $GiveCustomDefault $OK $CANCEL File
			#set WAITfilename wait
			#while {$WAITfilename == "wait"} {update}
			if {$WAITfilename == "opt1"} {
				set Default $GetFileName
			}
			.askConf.deja.men configure -text $Default
			update
		} \
		-label "Custom (Give your own configuration file)"
	pack .askConf.deja  -padx 5 -pady 10
	pack .askConf.deja.men

	label .askConf.img -image wine_half -backgr #ffffff -relief flat
	pack .askConf.img -side right -anchor se -padx 5 -pady 5
	
# Next radiobutton: tools/wineconf:
	
	frame .askConf.radio -backgr #ffffff -relief flat
	radiobutton .askConf.radio.generate -text $GenereConf\
		-anchor nw -value 1 -variable DefaultType \
		-selectcolor #00ff00 -relief raised
	pack .askConf.radio.generate -padx 5 -pady 5 -anchor w -fill x

# Next radiobutton :  TkWine Built-in

	radiobutton .askConf.radio.tkwbuilt -text $TkWBuiltConf\
		-anchor nw -value 2 -variable DefaultType \
		-selectcolor #00ff00 -relief raised
	pack .askConf.radio.tkwbuilt -padx 5 -pady 5  -anchor w -fill x
	pack .askConf.radio -padx 0 -pady 0  -anchor w

# now OK or CANCEL....
	frame .askConf.buttons -backgr #ffffff -relief flat -height 25
	button .askConf.buttons.ok -width 8 -text $OK -command {
		destroy .askConf
		TkW:CallWineConf 
	}
	button .askConf.buttons.cancel -width 8  -text $CANCEL -command {
		destroy .askConf
	}
	pack .askConf.buttons.ok -side left  -padx 25 -pady 5
	pack .askConf.buttons.cancel -side left -padx 5 -pady 5
	pack .askConf.buttons -anchor w -padx 5

}
proc TkW:CallWineConf {} {

	global Default DefaultType TKW
	switch $DefaultType {
		0 {TkW:wineconf $Default}
		1 {
			set FoundWineConf 0
			set FoundWineConf [TkW:autoconf /tmp/auto_generated_wineconf]
			while {$FoundWineConf == 0} update
			TkW:wineconf /tmp/auto_generated_wineconf

		}
		2 {TkW:wineconf $TKW/setupfiles/wine.ini}
	}

}

proc TkW:GenereTmpConf {wineconfTool} {
	global specialwhere
	set wherearewe ConfTool
	set Pipe [open "| $wineconfTool " r+]
	exec rm -f /tmp/auto_generated_wineconf
	set TmpConf [open  /tmp/auto_generated_wineconf w]
	while {![eof $Pipe]} {
		gets $Pipe line
		puts $TmpConf $line
	}
	close $TmpConf
	set specialwhere CloseWineAutoConf
	close $Pipe
}
