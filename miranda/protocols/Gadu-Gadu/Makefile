####################################################################
# Gadu-Gadu Protocol Plugin
# Makefile for Mingw
####################################################################
#
# TARGETS:
# all		Compiles dll into debug directory (create dir first)
# depend	Generate dependancy file (Makefile.dep)
# format	Formats the source files using GNU Indent
# clean 	Cleans object files
#
####################################################################
#
# OPTIONS:
#
# Example Usage: 'make DEBUG=1'
#
# DEBUG 	Compiles with debug options on
# INC		Miranda SDK headers
#
####################################################################
#
# NOTES:
# To use this makefile you need to download the latest Mingw from
# http://mingw.org/.  I have only tested on MinGW Version 2.0.0
# (using the latest package updates).  Older versions make work as
# well.  You will also need a copy of rm.exe for the make clean to
# work (http://unxutils.sourceforge.net/).	Make sure you create a
# 'debug\plugins' directory in the root directory of this cvs
# module or the compile will fail.	To format the code (format
# target) you will need GNU Indent from:
# http://gnuwin32.sourceforge.net/.  Indent.exe will need to be in
# the path or in the same directory as the source.
####################################################################
CFILES =	gg.c \
			keepalive.c \
			dialogs.c \
			pthread.c \
			services.c \
			import.c \
			dynstuff.c \
			ownerinfo.c \
			userutils.c \
			filetransfer.c \
			ssl.c \
			core.c \
			groupchat.c
CPPFILES =	token.cpp \
			image.cpp

OBJ = $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
RES = resource.res
HDR = gg.h pthread.h resource.h toc.h ssl.h
LIB = libgadu/libgadu.a -lws2_32 -lgdi32 -lcomdlg32 -lcomctl32 -lole32 -loleaut32 -lolepro32 -luuid -lstdc++

# Set here miranda SDK headers
INC = -I../../include -Iinclude

# Output dll and debug additions (report gen)
ifdef DEBUG
BIN = GGdebug.dll
else
BIN = GG.dll
endif

UPX = upx --best -q --compress-icons=0
CC = gcc
CPP = g++
RC = windres
RM = rm
DW = dllwrap
ID = indent

# Defines
DEFINES = -DWIN32
ifdef DEBUG
DEFINES := $(DEFINES) -D__DEBUG__
endif

# Flags
#LFLAGS  = --no-export-all-symbols --add-stdcall-alias
LFLAGS	= -shared
RCFLAGS = --input-format rc --output-format coff
ifdef DEBUG
CFLAGS = -g $(DEFINES) $(INC)
CPPFLAGS = -g $(DEFINES) $(INC)
else
CFLAGS = -Os $(DEFINES) $(INC)
CPPFLAGS = -Os $(DEFINES) $(INC)
LFLAGS := $(LFLAGS) -Wl,-s
endif

# Targets
all : libgadu/libgadu.a $(OBJ) $(RES)
	$(CC) $(LFLAGS) $(CFLAGS) -o $(BIN) $(OBJ) $(RES) $(LIB)
ifdef UPX
ifndef DEBUG
	$(UPX) $(BIN)
endif
endif

libgadu/libgadu.a:
	make -C libgadu

$(RES) : $(RES:.res=.rc) $(RES:.res=.h) Makefile
	$(RC) $(RCFLAGS) -o $(RES) -i $(RES:.res=.rc)

depend :
	$(CC) -MM $(CFLAGS) $(SRC)>Makefile.dep

format :
	$(ID) -gnu -l150 -nce -br -npsl -nut -cbi0 -cli4 -i4 -npcs $(SRC) $(HDR)
	$(RM) *.*~

clean :
	$(RM) -f *.o *.a $(RES)

-include Makefile.dep
