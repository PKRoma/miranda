CC=gcc
LD=gcc
CFLAGS=-Os -fomit-frame-pointer -fno-exceptions -s -pipe -shared -D_WIN32 -Wall -mms-bitfields
#CFLAGS=-pipe -shared -D_WIN32 -Wall -mms-bitfields -g
#LDFLAGS=-shared -s
LDFLAGS=-shared
LIBFAIMA=../oscar.a
BINARY=oscar.dll

SRC=oscar.c itr.c oscar_svc.c svc.c oscar_opt.c ackqueue.c
OBJ=$(SRC:.c=.o)

.PHONY: all

all: $(OBJ) resource.o
	$(LD) $(LDFLAGS) $(OBJ) resource.o $(LIBFAIMA) -lwsock32 -o $(BINARY)
	-cp $(BINARY) ../../../bin/release/plugins
	
resource.o: resource.rc
	windres -i resource.rc -o resource.o
	
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -I . -I ../. -I c:\\m\\miranda\\include -c $(SRC)
	
clean:
	-rm -rf *.o
	