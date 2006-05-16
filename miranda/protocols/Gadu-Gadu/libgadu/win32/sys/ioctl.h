/*
 * <sys/ioctl.h>
 *
 * zast±pienie ioctl() przez ioctlsocket(). zwyk³y #define
 * to za ma³o ze wzglêdu na niezgodno¶æ typu ostatniego argumentu obu
 * funkcji.
 *
 * $Id$
 */

#ifndef COMPAT_SYS_IOCTL_H
#define COMPAT_SYS_IOCTL_H

#include <io.h>
#include <winsock2.h>
#include <stdarg.h>

#ifndef __WINSOCKIOCTL__
#define __WINSOCKIOCTL__
#ifdef _MSC_VER
static __inline int ioctl(int fd, int request, ...)
#else
static inline int ioctl(int fd, int request, ...)
#endif
{
	va_list ap;
	unsigned long *foo;

	va_start(ap, request);
	foo = va_arg(ap, unsigned long*);
	va_end(ap);

	return ioctlsocket(fd, request, foo);

}
#endif

#endif /* COMPAT_SYS_IOCTL_H */
