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
#pragma link "Ws2_32.lib"  /* W Borlandzie linkuje biblioteke z socketami */

#define ASSIGN_SOCKETS_TO_THREADS /* gg_connect bedzie zapisywal nr socketa na liscie,
				     tak zeby mozna go bylo zamknac w polaczeniach synchronicznych (z innego watku) */

//#define socket(af,type,protocol) WSASocket(af,type,protocol,0,0,WSA_FLAG_OVERLAPPED)
#define write(handle,buf,len) send(handle,(void *)buf,len,0)
#define read(handle,buf,len) recv(handle,(void *)buf,len,0)
#define close(handle) closesocket(handle)

#undef EINPROGRESS
#undef ENOTCONN
#undef EINTR
#define EINPROGRESS WSAEINPROGRESS
#define ENOTCONN WSAENOTCONN
#define EINTR WSAEINTR
#define vsnprintf _vsnprintf
#define snprintf _snprintf

// win32 WinSocket wrapper
// #undef errno
// #define errno (errno = WSAGetLastError())
#define strerror(x) ws_strerror(x)

//#define socket(x,y,z) 

static int fork()
{
	return -1;
}

static int pipe(int filedes[])
{
	return -1;
}

#endif /* COMPAT_SYS_SOCKET_H */
