proc TkW:autoconf {outfile} {

# This files scans the fstab, then creates a wine.conf file
# parsed to wineconf for further editings....

# MS DOS drives are sorted following the rules explained in tools/wineconf

	global PATH
	global TMPLetter WINLetter SYSLetter

	global OUTF
	set OUTF [open $outfile w]

	TkW:InitNewConfig
	set err [TkW:readFStab]

	TkW:SortDosDrives
	TkW:addRootDrives
	
	set TMP [TkW:SearchTmp]
	TkW:ShowDrives


	set Windows [TkW:FindWindows]
	set System "$Windows/system"
	set Windows [TkW:Unix2Dos $Windows]
	set System [TkW:Unix2Dos $System]
	TkW:MiscEndConf $TMP 
	puts $OUTF "\[Wine\]\nwindows=$Windows"
	puts $OUTF "system=$System"
	puts $OUTF "Temp=[TkW:Unix2Dos $TMP]"
	puts $OUTF "Path= $Windows\;$System\;$PATH"

	#Quick and dirty... I'm tired now!
	#puts $OUTF "SymbolTableFile=[exec find / -name wine.sym -print]"
	puts $OUTF "printer=on"
	puts $OUTF ""

	puts $OUTF "\[DllDefaults\]"
	puts $OUTF "\;EXTRA_LD_LIBRARY_PATH=\$\{HOME\}/wine/cvs/lib"
	puts $OUTF ""
	puts $OUTF "\[DllPairs\]"
	puts $OUTF "kernel  = kernel32"
	puts $OUTF "gdi     = gdi32"
        puts $OUTF "user    = user32"
        puts $OUTF "commdlg = comdlg32"
        puts $OUTF "commctrl= comctl32"
        puts $OUTF "ver     = version"
        puts $OUTF "shell   = shell32"
        puts $OUTF "lzexpand= lz32"
        puts $OUTF "mmsystem= winmm"
        puts $OUTF "msvideo = msvfw32"
        puts $OUTF "winsock = wsock32"
        puts $OUTF ""

	 puts $OUTF "\[DllOverrides\]"
        puts $OUTF "kernel32, gdi32, user32 = builtin"
        puts $OUTF "kernel, gdi, user       = builtin"
        puts $OUTF "toolhelp                = builtin"
        puts $OUTF "comdlg32, commdlg       = elfdll, builtin, native"
        puts $OUTF "version, ver            = elfdll, builtin, native"
        puts $OUTF "shell32, shell          = builtin, native"
        puts $OUTF "lz32, lzexpand          = builtin, native"
        puts $OUTF "commctrl, comctl32      = builtin, native"
        puts $OUTF "sock32, winsock        = builtin"
        puts $OUTF "advapi32, crtdll, ntdll = builtin, native"
        puts $OUTF "mpr, winspool           = builtin, native"
        puts $OUTF "ddraw, dinput, dsound   = builtin, native"
        puts $OUTF "winmm, mmsystem         = builtin"
        puts $OUTF "msvideo, msvfw32        = builtin, native"
        puts $OUTF "w32skrnl                = builtin"
        puts $OUTF "wnaspi32, wow32         = builtin"
        puts $OUTF "system, display, wprocs = builtin"
        puts $OUTF "wineps                  = builtin"
	puts $OUTF ""

 	puts $OUTF "\[options\]"
        puts $OUTF "AllocSystemColors=100"
        puts $OUTF ""

        puts $OUTF "\[fonts\]"
        puts $OUTF "Resolution = 96 "
        puts $OUTF "Default = -adobe-times-"
        puts $OUTF ""

        puts $OUTF "\[serialports\]"
        puts $OUTF "Com1=/dev/ttyS0"
        puts $OUTF "Com2=/dev/ttyS1"
        puts $OUTF "Com3=/dev/modem,38400"
        puts $OUTF "Com4=/dev/modem"
	puts $OUTF ""

	puts $OUTF "\[parallelports\]"
        puts $OUTF "Lpt1=/dev/lp0"
        puts $OUTF ""

        puts $OUTF "\[spooler\]"
	puts $OUTF "LPT1:=|lpr"
        puts $OUTF "LPT2:=|gs -sDEVICE=bj200 -sOutputFile=/tmp/fred -q -"
        puts $OUTF "LPT3:=/dev/lp3"
        puts $OUTF ""

        puts $OUTF "\[ports\]"
        puts $OUTF "\;read=0x779,0x379,0x280-0x2a0"
        puts $OUTF "\;write=0x779,0x379,0x280-0x2a0"
        puts $OUTF ""

	 puts $OUTF "\[spy\]"
        puts $OUTF "Exclude=WM_SIZE" 
        puts $OUTF ";WM_TIMER"
	puts $OUTF ""
	
	puts $OUTF "\[Tweak.Layout\]"
        puts $OUTF ";WineLook=xxx	(supported styles are \'Win31\'(default),"
	puts $OUTF ";\'Win95\', \'Win98\')"
	puts $OUTF "WineLook=Win31"
        puts $OUTF ""

        puts $OUTF "\[programs\]"
        puts $OUTF "Default=]"
        puts $OUTF "Startup="

	close $OUTF
}

proc TkW:readFStab {} {

	global AlternateFstab OK CANCEL

	if {![file exist /etc/fstab]} {
		TkW:GetFileName $AlternateFstab $OK $CANCEL File
		TkW:FStabError 1
		return 1
	} else {
		set FSTAB "/etc/fstab"
	}
	if {![file readable $FSTAB]} {TkW:FStabError 2;return 1}

	set FStab [open /etc/fstab]
	while {![eof $FStab]} {

		gets $FStab line
		set line [string trim $line]
		if {[string range $line 0 0] != "#" && $line != ""} {
			#we have an entry
			switch [lindex $line 2] {

				swap {}
				proc {}
				minix {TkW:addUnixDrive $line}
				ext2 {TkW:addUnixDrive $line}
				xiafs {TkW:addUnixDrive $line}
				vfat {TkW:addDosDrive $line}
				msdos {TkW:addDosDrive msdos}
				hpfs {TkW:addDosDrive $line}
				iso9660 {TkW:addCdRomDrive $line}
				nfs {TkW:addNetDrive $line}
				auto {TkW:addAutoDrive $line}
				default {TkW:unknownDeviceType $line}
				
			}
		}
	}
}

proc TkW:addRootDrives {} {

	global RootDrives1 RootDrives2 OK CANCEL WAIT lineroot ADD REMOVE
	global NunixDrives UnixPath UnixDevice UnixLabel UnixType UnixSerial UnixFS
	global exclude  All
	
	set drivelist ""
	set drivelist "$drivelist [glob -nocomplain /*]"
	set List ""
	set excludes "$exclude /boot /lost+found /lib /etc /var /sbin /initrd\
			/cdrom /floppy /include /man /dev /proc /bin /mnt /tmp"
	set n 0
	foreach i $drivelist {
		if {[file isdir $i]} {
			set skip 0
			
			foreach k $excludes {
				if {$k  == $i} {
					set skip 1
				}
			}
			 if {$skip == 0} {set List "$List$i "}
		}
	}
	set List [lsort $List]
	set n 0
	foreach i $List {
		set All($n) $i;set n [expr $n + 1]
	}
	set dev  [lindex $lineroot 0]
	toplevel .rootDr -background #ffffff 
	wm title .rootDr  "Wine Autoconfigurator"
	label .rootDr.banner -backgr #0000ff -foregr #ffffff \
		-text "$RootDrives1$dev $RootDrives2" \
		-wraplength 300
	pack .rootDr.banner -fill x
	frame .rootDr.lists -backgr #ffffff
	pack .rootDr.lists -padx 10 -pady 10
	listbox .rootDr.lists.left -width 20 -height 10 
	pack .rootDr.lists.left -padx 10 -pady 0  -side left

	frame .rootDr.lists.buttons -backgr #ffffff
	pack .rootDr.lists.buttons -padx 10 -pady 0  -side left

	button .rootDr.lists.buttons.add -text "$ADD >>" -width 8\
		-command {TkW:AddRootDrInList [.rootDr.lists.left cursel]}
	pack .rootDr.lists.buttons.add -padx 10 -pady 0  -side top

	button .rootDr.lists.buttons.space -text "" -backgr #ffffff \
		-border 0 -highlightb  #ffffff -relief flat -width 8 -state disabled
	pack .rootDr.lists.buttons.space -padx 10 -pady 0  -side top

	button .rootDr.lists.buttons.rem -text "<< $REMOVE" -width 8\
		-command {TkW:RemRootDrInList [.rootDr.lists.right cursel]}
	pack .rootDr.lists.buttons.rem -padx 10 -pady 0  -side top

	listbox .rootDr.lists.right -width 20 -height 10 
	pack .rootDr.lists.right -padx 10 -pady 0 

	frame .rootDr.cmd -background #ffffff
	pack .rootDr.cmd -side bottom -anchor c -pady 5
	
	button .rootDr.cmd.ok -text OK -width 8 \
		-command {destroy .rootDr;set WAIT 0}
	pack .rootDr.cmd.ok -side left

	foreach i $List {
		.rootDr.lists.left insert end $i
	}

	global ToAddList;set ToAddList ""

	bind .rootDr.lists.left <Double-ButtonPress-1> {
		TkW:AddRootDrInList [.rootDr.lists.left nearest %y]}
	bind .rootDr.lists.right <Double-ButtonPress-1> {
		TkW:RemRootDrInList [.rootDr.lists.right nearest %y]}
	set WAIT wait
	while {$WAIT == "wait" } {update}
	foreach i $ToAddList {
		set NunixDrives [expr $NunixDrives + 1]
		set UnixPath($NunixDrives) $i
		set exclude "$exclude$UnixPath($NunixDrives) "
		set UnixLabel($NunixDrives) $i
		set UnixType($NunixDrives) hd
		set UnixFS($NunixDrives) win95
	}
}

proc TkW:AddRootDrInList {N} {

	global All ToAddList Nadd Add
	if {![info exists All($N)]} {return}
	foreach i $ToAddList {
		if {$i == $All($N)} {return}
	}
	set  ToAddList [lsort "$ToAddList $All($N)"]
	.rootDr.lists.right delete 0 end
	set Nadd 0
	foreach i $ToAddList {
		.rootDr.lists.right insert end $i
		set Add($Nadd) $i
		set Nadd [expr $Nadd + 1]
	}
}
proc TkW:RemRootDrInList {N} {
	global All ToAddList Nadd Add
	if {![info exists Add($N)]} {return}
	set NewList ""
	foreach i $ToAddList {
		if {$i != $Add($N)} {set NewList "$NewList$i "}
	}
	set ToAddList $NewList
	set Nadd 0
	.rootDr.lists.right delete 0 end
	foreach i $ToAddList {
		.rootDr.lists.right insert end $i
		set Add($Nadd) $i
		set Nadd [expr $Nadd + 1]
	}
}
proc TkW:addUnixDrive {line} {
	global lineroot
	#pre-processing for the case of the root:
	if {![file exists [lindex $line 1]]} {return}
	if {[lindex $line 1] != "/"} { TkW:addUnixDriveOK $line}  {set lineroot $line}
}
proc TkW:addUnixDriveOK {line} {

	# for now, I'll consider any unix file as ext2... 
	# may be this should improve a bit!

	global NunixDrives UnixPath UnixDevice UnixLabel UnixType UnixSerial UnixFS
	
	global TMPDRIVE HOMEDRIVE exclude

	set NunixDrives [expr $NunixDrives +1]

	set UnixDevice($NunixDrives) [lindex $line 0]
	set UnixType($NunixDrives) "hd"
	set UnixPath($NunixDrives)  [lindex $line 1] 
	set exclude "$exclude$UnixPath($NunixDrives) "
	set  UnixLabel($NunixDrives) "UNIX $NunixDrives"
	if {[string first "/tmp" $UnixPath($NunixDrives)] > 0 && $TMPDRIVE == 0} {
		set  UnixLabel($NunixDrives) "TEMP"
	}
	if {[string first "/home" $UnixDevice($NunixDrives)] > 0 && $HOMEDRIVE == 0} {
		set  UnixLabel($NunixDrives) "HOME"
	}
	set UnixDevice($NunixDrives) ""
	set UnixSerial($NunixDrives) ""
	set UnixFS($NunixDrives) "win95"
	set UnixType($NunixDrives) "hd"
}
proc TkW:addDosDrive {line} {

	# for now, I'll consider any dos file as ext2... 
	# may be this should improve a bit!

	global NdosDrives DosPath DosDevice DosLabel DosType DosSerial DosFS
	
	global TMPDRIVE HOMEDRIVE exclude

	if {![file exists [lindex $line 1]]} {return}

	if { [string first "/dev/fd" [lindex $line 0]] == 0  } {
		TkW:addFloppy $line
		return
	}

	set NdosDrives [expr $NdosDrives +1]

	set DosDevice($NdosDrives) [lindex $line 0]
	set DosPath($NdosDrives)  [lindex $line 1] 
	set exclude "$exclude$DosPath($NdosDrives) "
	set  DosLabel($NdosDrives) ""
	set DosType($NdosDrives) "win95"
	if {[lindex $line 2] == "msods"} {
		set DosFS($NdosDrives) 
	} else {
		set DosFS($NdosDrives) "win95"
	}
	set DosSerial($NdosDrives) ""
}

proc TkW:unknownDeviceType {line} {

	TkW:message "Error\n\
[lindex $line 0] (mounted on [lindex $line 1]) : Unknown device type. Please, mail thirot@univ-brest.fr for me to add this filesystem to the list of detected FS.\n\
\n		Thank you.\
\n			Jean-Louis Thirot"
	global WAIT
	set WAIT wait
	while {$WAIT == "wait"} {update}

}

proc TkW:InitNewConfig {} {
	
	global NunixDrives UnixPath UnixDevice UnixLabel UnixType UnixSerial UnixFS
	global NdosDrives DosPath DosDevice DosLabel DosType DosSerial DosFS
	global Ncdroms CDPath CDDevice CDLabel CDType CDSerial CDFS
	global Nfloppys FLOPPYPath FLOPPYDevice FLOPPYLabel FLOPPYType FLOPPYSerial FLOPPYFS
	global NnetDrives
	global TMPDRIVE HOMEDRIVE
	global exclude; set exclude ""

	set TMPDRIVE 0
	set HOMEDRIVE 0

	set NunixDrives 0
	set NdosDrives 0
	set Ncdroms 0
	set Nfloppys 0
	set NnetDrives 0
}

proc TkW:ShowDrives {} {

	global NunixDrives UnixPath UnixDevice UnixLabel UnixType UnixSerial UnixFS
        global NdosDrives DosPath DosDevice DosLabel DosType DosSerial DosFS       
	global Ncdroms CDPath CDDevice CDLabel CDType CDSerial CDFS
	global NnetDrives NetPath NetLabel 
	global Nfloppys FLOPPYPath
	global DriveRank DRIVEUNIXPATH DRIVEDOSPATH NDRIVES
	global OUTF
	
	set NDRIVES 0
	set letters {let: C D E F G H I J K L M N O P Q R S T U V W X Y Z}
	set lettersF {let: A B C D E F G H I J K L M N O P Q R S T U V W X Y Z}
	for  {set i 1} {$i <= $NdosDrives} {set i [expr $i + 1]} {
		set NDRIVES [expr $NDRIVES + 1]
		set rank $DriveRank($i)
		puts $OUTF "\[Drive [lindex $letters $i]\]"
		set Letter($NDRIVES)  [lindex $letters $i]
		set DRIVEUNIXPATH($NDRIVES) $DosPath($rank)
		set DRIVEDOSPATH($NDRIVES) "[lindex $letters $i]:\\"
		puts $OUTF "Path=$DosPath($rank)"
		puts $OUTF "Type=hd"
		puts $OUTF "Label=MS-DOS $rank"
		puts $OUTF "Filesystem=$DosFS($rank)"
		puts $OUTF ""
	}
	for  {set i 1} {$i <= $Nfloppys} {set i [expr $i + 1]} {
		if {$i > 2} {
			set NDRIVES [expr $NDRIVES + 1]
			set NLet $NDRIVES
			set DRIVEDOSPATH($NDRIVES) "[lindex $lettersF $NLet]:\\"
			set DRIVEUNIXPATH($NDRIVES) $FLOPPYPath($i)
		} else {
			set NLet $i
			#set DRIVEDOSPATH($i) "[lindex $lettersF $NLet]:\\"
			#set DRIVEUNIXPATH($i) $FLOPPYPath($i)
		}
		puts $OUTF "\[Drive [lindex $lettersF $NLet]\]"
		puts $OUTF "Path=$FLOPPYPath($i)"
		puts $OUTF "Type=floppy"
		puts $OUTF "Label=Floppy $i"
		puts $OUTF "Serial=87654321"
		puts $OUTF ""
	}
	for  {set i 1} {$i <= $Ncdroms} {set i [expr $i + 1]} {
		set NDRIVES [expr $NDRIVES + 1]
		puts $OUTF "\[Drive [lindex $letters $NDRIVES]\]"
		puts $OUTF "Path=$CDPath($i)"
		set DRIVEDOSPATH($NDRIVES) "[lindex $letters $NDRIVES]:\\"
		set DRIVEUNIXPATH($NDRIVES) $CDPath($i)
		puts $OUTF "Type=cdrom"
		puts $OUTF "Label=CDROM $i"
		puts $OUTF "Filesystem=$UnixFS($i)"
		if {$CDDevice($i) != ""} {puts $OUTF "Device=$CDDevice($i)"}
		puts $OUTF ""
	}
	for  {set i 1} {$i <= $NunixDrives} {set i [expr $i + 1]} {
		set NDRIVES [expr $NDRIVES + 1]
		puts $OUTF "\[Drive [lindex $letters $NDRIVES]\]"
		set DRIVEDOSPATH($NDRIVES) "[lindex $letters $NDRIVES]:\\"
		set DRIVEUNIXPATH($NDRIVES) $UnixPath($i)
		puts $OUTF "Path=$UnixPath($i)"
		puts $OUTF "Type=hd"
		puts $OUTF "Label=$UnixLabel($i)"
		puts $OUTF "Filesystem=$UnixFS($i)"
		puts $OUTF ""
	}
	for  {set i 1} {$i <= $NnetDrives} {set i [expr $i + 1]} {
		set NDRIVES [expr $NDRIVES + 1]
		puts $OUTF "\[Drive [lindex $letters $NDRIVES]\]"
		set DRIVEDOSPATH($NDRIVES) "[lindex $letters $NDRIVES]:\\"
		set DRIVEUNIXPATH($NDRIVES) $NetPath($i)
		puts $OUTF "Path=$NetPath($i)"
		puts $OUTF "Type=network"
		puts $OUTF "Label=$NetLabel($i)"
		puts $OUTF "Filesystem=win95"
		puts $OUTF ""
	}
}

proc TkW:MiscEndConf {temp} {

	#Create an acceptable PATH variable, and
	# gets the TMP WINDOWS and SYSTEM drive letters:
	global PATH NDRIVES Letter DRIVEPATH
	global TMPLetter 
	set PATH ""
	for {set i 1} {$i<= $NDRIVES} {set i [expr $i + 1]} {
		if {[info exists Letter($i)]} {
			if {[info exists DRIVEPATH($i)]} {
			  	if {$DRIVEPATH($i) == $temp} {
					set TMPLetter $Letter($i)
			   	}
			    	set PATH "$PATH\;$Letter($i):\\"
			}
		}
	}
	return 
}

proc TkW:Unix2Dos {unixpath} {

	global NDRIVES DRIVEDOSPATH DRIVEUNIXPATH

	set dospath ""

# First, search the corresponding drive:

	for {set i 1} {$i<=$NDRIVES} {set i [expr $i + 1]} {
		set n [string first $DRIVEUNIXPATH($i) $unixpath] 
		if {$n == 0} {
			set dospath $DRIVEDOSPATH($i)
			set N [expr [string length  $DRIVEUNIXPATH($i)] + 1]
			break
		}
	}
	
# Now, add the remaining part of the path

	for {set i $N} {$i <= [string length $unixpath]} {set i [expr $i + 1]} {

		set newchar [string range $unixpath $i $i] 
		if {$newchar  != "/"} {
			set dospath "$dospath$newchar"
		} else {
			set dospath "$dospath\\"
		}
	}
	return $dospath
}

proc TkW:addCdRomDrive {line} {
	global Ncdroms CDPath CDDevice CDLabel CDType CDSerial CDFS

	global IsCDwrite YES NO WAIT

	if {![file exists [lindex $line 1]]} {return}
	set Ncdroms [expr $Ncdroms + 1]
	set  CDFS($Ncdroms) "iso9660"
	set CDPath($Ncdroms) [lindex $line 1]
	set dev [lindex $line 0]
	TkW:message2 "Device $dev $IsCDwrite" $YES $NO
	set WAIT wait
	while {$WAIT == "wait"} {update}
	if {$WAIT == "opt1"} {set CDDevice($Ncdroms) $dev} {set CDDevice($Ncdroms) ""}
	set CDLabel($Ncdroms) "CDrom"
}
proc TkW:addNetDrive {line} {
	global NnetDrives NetPath NetLabel 
	set NnetDrives [expr $NnetDrives + 1]
	set NetPath($NnetDrives)  [lindex $line 1]
	set NetLabel($NnetDrives) [file tail $NetPath($NnetDrives)]
}
proc TkW:addFloppy {line} {
	global Nfloppys FLOPPYPath FLOPPYDevice FLOPPYLabel FLOPPYType FLOPPYSerial FLOPPYFS
	if {![file exists [lindex $line 1]]} {return}
	set Nfloppys [expr $Nfloppys + 1]
	set FLOPPYPath($Nfloppys) [lindex $line 1]
	set FLOPPYDevice($Nfloppys) [lindex $line 0]
}

proc TkW:SortDosDrives {} {
	global NdosDrives DosDevice DriveRank

	set devlist ""
	for  {set i 1} {$i <= $NdosDrives} {set i [expr $i + 1]} { 
		set  devlist "$devlist $DosDevice($i):TKW:$i"
	}
	set devlist [lsort $devlist]

	set i 1
	foreach dev $devlist {
		set n1 [expr [string first ":TKW:" $dev] + 5]
		set n2 [string length $dev]
		set DriveRank([string range $dev $n1 $n2]) $i
		set i [expr $i + 1]
	}
}

proc TkW:SearchTmp {} {
	global exclude
	global NunixDrives UnixPath UnixLabel UnixType UnixFS
	global NdosDrives DosPath DosLabel DosType DosFS
	global WAITfilename GetFileName NDRIVES DRIVEUNIXPATH DRIVEDOSPATH
	global WhereTmp OK CANCEL

# Check if it /tmp is alreaddy mounted (when it has it's own partition)
	foreach i $exclude {
		if {$i == "/tmp"} {return /tmp}
	}
# check that we have a /tmp:
	if {[file exists /tmp] } {
		set NunixDrives [expr $NunixDrives + 1]
		set UnixPath($NunixDrives) /tmp
		set exclude "$exclude$UnixPath($NunixDrives) "
		set UnixLabel($NunixDrives) "TEMP"
		set UnixType($NunixDrives) hd
		set UnixFS($NunixDrives) win95
		return /tmp
	} else {
#Could add here a search for c:\TEMP or c:|windows\temp
		set WAITfilename wait
		TkW:GetFileName $WhereTmp $OK $CANCEL Folder

# check if this folder is on a drive that has an entry in
# wineconf (Must be a unix or dos drive)
		for {set i 1} {$i<=$NdosDrives} {set i [expr $i +1]} {
			if {[string first $DosPath($i) $GetFileName] == 0} {
				return $GetFileName
			}
		}
		for {set i 1} {$i<=$NunixDrives} {set i [expr $i +1]} {
			if {[string first $UnixPath($i) $GetFileName] == 0} {
				return $GetFileName
			}
		}
# I'll consider this new one as unix, althought if it's win fs that's ok
		set NunixDrives [expr $NunixDrives + 1]
		set UnixPath($NunixDrives) $GetFileName
		set exclude "$exclude$UnixPath($NunixDrives) "
		set UnixLabel($NunixDrives) "TEMP"
		set UnixType($NunixDrives) hd
		set UnixFS($NunixDrives) win95
		return $GetFileName
	}
}

proc TkW:FindWindows {} {

	global NdosDrives DosPath WINLetter SYSLetter Letter

	#first attempt: C:\windows (would avoid to search)

	if {[file exist $DosPath(1)/windows]} {
		return "$DosPath(1)/windows"
	}
	
	for  {set i 1} {$i<=$NdosDrives} {set i [expr $i + 1]} {
		set searchWin [open "| find $DosPath($i) -name win.ini -print" r+]
		while {![eof $searchWin]} {
			gets $searchWin Found
			close $searchWin
			set Found [file dirname $Found]
			return $Found
		}
	}
	close $searchWin
	
}
