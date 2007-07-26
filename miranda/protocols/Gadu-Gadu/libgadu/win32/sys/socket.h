/*
 * <sys/socket.h>
 *
 * wrapper wielu funkcji i sta³ych niezbêdnyc do obs³ugi socketów.
 *
 * $Id$
 */

#ifndef COMPAT_SYS_SOCKET_H
#define COMPAT_SYS_SOCKET_H

#include <winsock2.h>
#ifndef _MSC_VER
#pragma link "Ws2_32.lib"  /* W Borlandzie linkuje biblioteke z socketami */
#endif


/* Socket function is already declared in winsock2.h */
/* #define socket(af, type, protocol) WSASocket(af, type, protocol, 0, 0, WSA_FLAG_OVERLAPPED) */
#define write(handle, buf, len)	send(handle, (void *)(buf), len, 0)
#define read(handle, buf, len)	recv(handle, (void *)(buf), len, 0)

#undef ASSIGN_SOCKETS_TO_THREADS /* gg_connect bedzie zapisywal nr socketa na liscie,
				     tak zeby mozna go bylo zamknac w polaczeniach synchronicznych (z innego watku) */
#ifdef ASSIGN_SOCKETS_TO_THREADS
#  define close(handle)			gg_win32_thread_socket(-1,handle)
#else
#define close(handle)			closesocket(handle)
#endif

/* Some Visual C++ overrides having no problems with MinGW */
#ifdef _MSC_VER
#define	S_IWUSR		0x0080
/* Make sure we included errno before that */
#include <errno.h>
#endif

#ifdef EINPROGRESS
# undef EINPROGRESS
#endif
#ifdef ENOTCONN
# undef ENOTCONN
#endif
#ifdef EINTR
# undef EINTR
#endif
#define EINPROGRESS WSAEINPROGRESS
#define ENOTCONN    WSAENOTCONN
#define EINTR       WSAEINTR
#define ECONNRESET  WSAECONNRESET
#define ETIMEDOUT   WSAETIMEDOUT

#define WNOHANG     WHOHANG
#define SHUT_RDWR 2

#define vsnprintf _vsnprintf
#define snprintf _snprintf

// win32 WinSocket wrapper
// #undef errno
// #define errno (errno = WSAGetLastError())

// Defined in gg.c custom error reporting function
char *ws_strerror(int code);
#define strerror(x) ws_strerror(x)

// #define socket(x,y,z)

static int fork()
{
	return -1;
}

static int pipe(int filedes[])
{
	return -1;
}

#endif /* COMPAT_SYS_SOCKET_H */
