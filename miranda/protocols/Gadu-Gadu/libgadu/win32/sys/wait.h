/*
 * <sys/wait.h>
 *
 * funkcje wait() i waitpid() zwracaj� -1. tyle wystarczy, �eby skompilowa�
 * obs�ug� synchronicznych po��cze�.
 *
 * $Id$
 */

#ifndef COMPAT_SYS_WAIT_H
#define COMPAT_SYS_WAIT_H

#define WHOHANG 0

static int wait(int *x)
{
	return -1;
};

static int waitpid(int x, int *y, int z)
{
	return -1;
}

#endif /* COMPAT_SYS_WAIT_H */
