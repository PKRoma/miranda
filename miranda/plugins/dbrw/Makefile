SRC =   contacts.c \
        dbrw.c \
        events.c \
        settings.c \
        sql.c \
        utf8.c \
        utils.c \
        sqlite3/alter.c \
        sqlite3/analyze.c \
        sqlite3/attach.c \
        sqlite3/auth.c \
        sqlite3/btree.c \
        sqlite3/build.c \
        sqlite3/callback.c \
        sqlite3/complete.c \
        sqlite3/date.c \
        sqlite3/delete.c \
        sqlite3/expr.c \
        sqlite3/func.c \
        sqlite3/hash.c \
        sqlite3/insert.c \
        sqlite3/legacy.c \
        sqlite3/loadext.c \
        sqlite3/main.c \
        sqlite3/malloc.c \
        sqlite3/opcodes.c \
        sqlite3/os.c \
        sqlite3/os_os2.c \
        sqlite3/os_unix.c \
        sqlite3/os_win.c \
        sqlite3/pager.c \
        sqlite3/parse.c \
        sqlite3/pragma.c \
        sqlite3/prepare.c \
        sqlite3/printf.c \
        sqlite3/random.c \
        sqlite3/select.c \
        sqlite3/table.c \
        sqlite3/tokenize.c \
        sqlite3/trigger.c \
        sqlite3/update.c \
        sqlite3/utf.c \
        sqlite3/util.c \
        sqlite3/vacuum.c \
        sqlite3/vdbe.c \
        sqlite3/vdbeapi.c \
        sqlite3/vdbeaux.c \
        sqlite3/vdbeblob.c \
        sqlite3/vdbefifo.c \
        sqlite3/vdbemem.c \
        sqlite3/vtab.c \
        sqlite3/where.c
RS  = resource.rc

CC  = gcc
RC  = windres
RM  = rm
OBJ = $(SRC:.c=.o)
RES = $(RS:.rc=.res)

LIB     = 
LFLAGS  = -shared -s
RCFLAGS = --input-format rc --output-format coff -I../../include
SQLITEDEF = -DNO_TCL
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
