#
# makefile for shlext 1.0.6.6
#
# This makefile is to be used with FPC's MAKE utility which is
# actually GNU make, not fpcmake which generates make files.
#

TARGET=shlext.dll
CC=fpc
OBJ=shlcom.ow shlipc.ow shlext.ow
PPW=$(OBJ:.ow=.ppw)
INC=../../SDK/headers_pascal
RELOCADDR=B49ad0000

CCFLAGS=-RINTEL -Sd -Sg -WG -S2 -W $(RELOCADDR) -Fi$(INC) -Fu$(INC)

#verbose levels
CCFLAGS+=-v0 -vi

#ifndef DEBUG
CCFLAGS+=-OG -Or -O3 -Ur -Xs
#else
#endif

all: $(TARGET)
#
	
$(TARGET): shlext.dpr

shlext.dpr: shlcom.pas shlipc.pas
	windres -i shldlgs.rc -o shldlgs.res
	$(CC) $(CCFLAGS) $@
	copy shlext.dll d:\miranda0300\bin\release\plugins
	copy shlext.dll d:\miranda0300\bin\debug\plugins

clean:
	rm -f $(TARGET)
	rm -f $(OBJ)
	rm -f $(PPW)
	rm -f shlext.owr