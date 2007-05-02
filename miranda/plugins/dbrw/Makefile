SRC =   contacts.c \
        dbrw.c \
        events.c \
        settings.c \
        sql.c \
        utf8.c \
        utils.c \
        sqlite3/sqlite3.c
RS  = resource.rc

CC  = gcc
RC  = windres
RM  = rm
OBJ = $(SRC:.c=.o)
RES = $(RS:.rc=.res)

LIB     = 
LFLAGS  = -shared -s
RCFLAGS = --input-format rc --output-format coff -I../../include
SQLITEDEF = -DNO_TCL \
            -DSQLITE_OMIT_UTF16 \
            -DSQLITE_OMIT_LOAD_EXTENSION \
            -DSQLITE_OMIT_PROGRESS_CALLBACK \
            -DSQLITE_OMIT_AUTHORIZATION \
            -DSQLITE_OMIT_ALTERTABLE \
            -DSQLITE_OMIT_AUTOVACUUM 
DEFINES = -D__SEH_NOOP $(SQLITEDEF) -DDBRW_OLD_PLUGINAPI_SUPPORT
ifdef DEBUG
BIN = ../../bin/debug/plugins/dbx_rw.dll
DEFINES += -DDBRW_LOGGING -DDBRW_DEBUG
CFLAGS  = -g $(DEFINES) -I../../include -Wall -Wconversion
else
BIN = ../../bin/release/plugins/dbx_rw.dll
CFLAGS  = -O3 $(DEFINES) -I../../include -Wall
endif

all : $(OBJ) $(RES)
	$(CC) $(LFLAGS) $(CFLAGS) -o $(BIN) $(OBJ) $(RES) $(LIB) -Wl

$(RES) : $(RS) Makefile
	$(RC) $(RCFLAGS) -o $(RES) -i $(RS)
    
depend : 
	$(CC) -MM $(CFLAGS) $(SRC)>Makefile.dep

clean :
	$(RM) -f $(OBJ) $(RES)

dist-clean : clean
	$(RM) -f $(BIN) $(RES)

-include Makefile.dep
