#! /bin/bash

# default settings
TMPDIR=/tmp/fconv.$$;
TFILE=`tempfile`;

# Where the fnt2bdf utility resides 
FC=$HOME""/wine/tools/fnt2bdf;
# which OEM_CHARSET to use
CHARSET="winsys";
TARGET=/usr/X11R6/lib/X11/fonts/misc;
BDFTOPCF=/usr/X11R6/bin/bdftopcf;
PAT="*.fon";
Q="";
OLDPWD=`pwd`; 

usage () {
    echo "usage: "`basename $0`" [-q] [-c charset] [-t fontdir] [-b bdftopcf] [-f fnt2bdf]"
    echo "       [-p pattern] windir"
    echo 
    echo "this utility scans a directory and its subdirectories for bitmap-fonts"
    echo "in Windows format, converts them to PCF-fons and installs them. If X"
    echo "is running, the X fontpath is re-adjusted."
    echo 
    echo "options:"
    echo " -q          quit operation."
    echo " -c charset  charset name for OEM_CHARSET fonts, default: $CHARSET"
    echo " -t fontdir  directory to install the converted fonts in. This"
    echo "             directory should be a known fontdirectory to X, default:"
    echo "             $TARGET";
    echo " -b bdftopcf name of the program to call for bdf to pcf conversion,"
    echo "             default: $BDFTOPCF";
    echo " -f fnt2bdf  name of the program to call for winfont to bdf conversion,"
    echo "             default: $FC"
    echo " -p pattern  Shell-Pattern of the filenames to look for. By default, the"
    echo "             utility will look for the pattern "$PAT" (case insensitive)."
    echo " windir      base directory to search."
    exit 1;
}


while [ "$1" ]; do
    case $1 in
	-c ) shift; if [ "$1" ]; then CHARSET=$1; shift; else usage; fi; ;;
	-t ) shift; if [ "$1" ]; then TARGET=$1; shift; else usage; fi; ;;
	-b ) shift; if [ "$1" ]; then BDFTOPCF=$1; shift; else usage; fi; ;;
	-f ) shift; if [ "$1" ]; then FC=$1; shift; else usage; fi; ;;
	-p ) shift; if [ "$1" ]; then PAT=$1; shift; else usage; fi; ;;
        -q ) shift; Q=":"; ;;
	-* ) usage; ;;
	* ) if [ "$WIND" ]; then usage; else WIND=$1; shift; fi; ;;  
    esac;
done;

if [ ! "$WIND" ]; then usage; fi;
if [ ! -d "$WIND" ]; then $Q echo "$WIND is not a directory"; exit 1; fi;
if [ ! -d "$TARGET" ]; then $Q echo "$TARGET is not a directory"; exit 1; fi;
type -p $BDFTOPCF 1>/dev/null || { $Q echo "Can 't execute $BDFTOPCF"; exit 1; }
type -p $FC 1>/dev/null || { $Q echo "Can't execute $FC"; exit 1; }

$Q echo -n "looking for bitmap fonts... "
FONTS=`find "$WIND" -iname $PAT 1>$TFILE 2>/dev/null`;
if [ $? -ne 0 ]; then 
    $Q echo "$PAT is a invalid sarch expression"; exit 1;
fi;
i=0;
{ while read dummy; do FONTS[$i]="$dummy"; i=$[$i+1]; done; } < $TFILE
rm $TFILE;
$Q echo "done."

if [ -z "$FONTS" ]; then $Q echo "Can't find any fonts in $WIND"; exit 1; fi; 

mkdir $TMPDIR;
cd $TMPDIR; 

for i in "${FONTS[@]}"; do 
    FNT=`basename "$i"`; FNT=${FNT%.???};
    $Q echo "converting $i";
    if [ "$Q" ]; then 
	$FC -c $CHARSET -f $FNT "$i" 2>/dev/null;
    else
	$FC -c $CHARSET -f $FNT "$i";
    fi;
done;

for i in *.bdf; do
    if [ "$i" == "*.bdf" ]; then echo "No fonts extracted"; exit 0; fi; 
    bdftopcf -o "${i%.???}.pcf" "$i";
    $Q echo "installing ${i%.???}.pcf";
    mv "${i%.???}.pcf" $TARGET 2>/dev/null
    if [ $? -ne 0 ]; then 
	$Q echo "Can't install fonts to $TARGET. Are your root?"; cd $OLDPWD; rm -rf $TMPDIR; exit 1; fi;
    rm "$i";
done;

cd $TARGET;
$Q echo "running mkfontdir"; 
if [ "$Q" ]; then
    mkfontdir 1>/dev/null 2>/dev/null;
else
    mkfontdir
fi;
rmdir $TMPDIR;

if [ "$DISPLAY" ]; then $Q echo "adjusting X font database"; xset fp rehash; fi;

