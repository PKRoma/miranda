#makefile for icqwd

CC=gcc
LD=dllwrap
TARGET=icqwd.dll
SRC=icqwd.c
HDR=$(SRC:.c=.h)
OBJ=$(SRC:.c=.o)
#--pedantic 
CCFLAGS=-Wall -O2 -DWIN32 -momit-leaf-frame-pointer
ifdef DEBUG
	CCFLAGS+=-g -DDEBUG -D__DEBUG__
else
	CCFLAGS+=-O2 -DNDEBUG -D_WINDOWS
endif

LIB=-luser32
DEF=icqwd.def 
LDFLAGS=--enable-stdcall-fixup -O --relocateable --strip-all --discard-all -s --def $(DEF) 

# targets

all: $(TARGET)

$(TARGET): $(OBJ) $(HDR)
	$(LD) $(LIB) $(LDFLAGS) $(OBJ) -o $(TARGET)
	#cp $(TARGET) /d/miranda/plugins/

%.o: %.c
	$(CC) $(CCFLAGS) -c $<
	
clean:
	rm $(OBJ)
	rm $(TARGET)