########################################################
#
#   hyperoid makefile
#
#   Copyright (C) 1988-1990 Lantern Corp.
#
########################################################

# This is just a teaser to show how little you have to do to make projects
# under Lantern Corp.'s software engineering system. To make Hyperoid on
# your system, make sure <winext.h> is in your include path and set up your
# makefile like you normally do for your own stuff.
# I'll (hopefully) post the 100k of stuff you need for this makefile to
# be usefull. I'll have to talk to my boss first...
# P.S. The system only works for the Mircosoft C6.0 compiler, but its easy
# enough to change.
# P.P.S. I hate every integrated environment I've run across so far, thus
# the system is run from a command prompt.

PROJ = hyperoid
OBJS = hyperoid.obj roidsupp.obj
RESS = hyperoid.res
LIBS =

!include <windows.mk>

