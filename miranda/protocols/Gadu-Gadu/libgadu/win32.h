/* Windows wrappers for missing POSIX functions */

#ifndef __GG_WIN32_H
#define __GG_WIN32_H

#include <winsock2.h>
#include <io.h>

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

/* Defined in gg.c custom error reporting function */
#ifdef GG_CONFIG_MIRANDA
char *ws_strerror(int code);
#define strerror(x) ws_strerror(x)
#endif

#define fork()			(-1)
#define pipe(filedes)	(-1)
#define wait(x)			(-1)
#define waitpid(x,y,z)	(-1)
#define ioctl(fd,request,val) ioctlsocket(fd,request,(unsigned long *)val)

#endif /* __GG_WIN32_H */
