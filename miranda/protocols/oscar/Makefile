#
# this makefile isnt what comes with libfaim, but it generates a static library
#

CC=gcc
#CFLAGS=-Os -pipe -fomit-frame-pointer -fno-exceptions -s
CFLAGS=-g -Os -pipe
ARFLAGS=-rcs

SRC=adverts.c md5.c translate.c invite.c stats.c bart.c admin.c odir.c popups.c \
search.c misc.c buddylist.c auth.c bstream.c bos.c chat.c chatnav.c email.c \
locate.c rxhandlers.c snac.c msgcookie.c rxqueue.c icq.c ssi.c \
txqueue.c ft.c service.c tlv.c util.c conn.c im.c libc_interface.c

OBJ=$(SRC:.c=.o)

.PHONY: all

define generate_library
$(AR) $(ARFLAGS) oscar.a $(OBJ)
endef

all: $(OBJ)
	$(generate_library)
	
%.o: %.c
	$(CC) $(CFLAGS) -I . -D_WIN32 -c $<
	
clean:
	rm -rf $(OBJ) *.a