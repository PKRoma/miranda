####################################################################
# AIM TOC2 Protocol Plugin
# Makefile for Mingw
####################################################################
#
# TARGETS:
# all       Compiles dll into debug directory (create dir first)
# depend    Generate dependancy file (Makefile.dep)
# format    Formats the source files using GNU Indent
# clean     Cleans object files
#
#
# NOTES:
# To use this makefile you need to download the latest Mingw from
# http://mingw.org/.  I have only tested on MinGW Version 2.0.0
# (using the latest package updates).  Older versions make work as
# well.  You also need to checkout the SDK module alongside this
# module.  You will need a copy of rm.exe for the 'make clean' to
# work (http://unxutils.sourceforge.net/).  Make sure you create a
# 'Bin\debug\plugins' directory or 'Bin\release\plugins' in the parent 
# directory of this cvs module or the compile will fail.  To format
# the code (format target) you will need GNU Indent from:
# http://gnuwin32.sourceforge.net/.  Indent.exe will need to be in
# the path or in the same directory as the source.
####################################################################
SRC =	aim.cpp \
      	commands.cpp \
 	connection.cpp \
	md5.cpp \
	packets.cpp \
	services.cpp \
	snac.cpp \
	thread.cpp \
	utility.cpp \
	proxy.cpp	\
	direct_connect.cpp \
	file.cpp 
OBJ =	$(SRC:.cpp=.o)
RES = resource.res
HDR =	aim.h \
      	commands.h \
 	connection.h \
	md5.h \
	packets.h \
	services.h \
	snac.h \
	thread.h \
	utility.h \
	proxy.h	\
	direct_connect.ch \
	file.h \
	resource.h

LIB = -lwsock32
CC = g++
RC = windres
RM = rm
ID = indent

# Install location
ifdef DEBUG
BIN = ..\..\bin\debug\plugins\AimOSCAR.dll
else
BIN = ..\..\bin\release\plugins\AimOSCAR.dll
endif

# Defines
DEFINES = -DWIN32 -D__SEH_NOOP
ifdef DEBUG
DEFINES := $(DEFINES) -D_DEBUG
endif

# Flags
RCFLAGS = --use-temp-file -O coff
ifdef DEBUG
CFLAGS  = -g -g3 $(DEFINES) -I../../include
LFLAGS  = -shared
else
CFLAGS  = -O1 $(DEFINES) -I../../include
LFLAGS  = -shared -s -pedantic -W
endif

# Targets
all : $(OBJ) $(RES)
	$(CC) $(LFLAGS) $(CFLAGS) -o $(BIN) $(OBJ) $(RES) $(LIB) -Wl

$(OBJ) :
	$(CC) -c $(SRC) $(CFLAGS)

$(RES) : aim.rc $(RES:.res=.h) Makefile
	$(RC) $(RCFLAGS) -o $(RES) -i aim.rc

depend : 
	$(CC) -MM $(CFLAGS) $(SRC)>Makefile.dep

format :
	$(ID) -gnu -l150 -nce -br -npsl -nut -cbi0 -cli4 -i4 -npcs $(SRC) $(HDR)
	$(RM) *.*~

clean :
	$(RM) -fr $(OBJ) $(RES) *.ncb *.suo Release Debug

-include Makefile.dep
