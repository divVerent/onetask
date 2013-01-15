/*
 *        DO WHAT THE FORK YOU WANT TO PUBLIC LICENSE 
 *                    Version 2, December 2004
 *
 * Program Copyright (C) 2013 Rudolf Polzer <divVeremt@xonotic.org>
 * Original License Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>
 *
 * Everyone is permitted to copy and distribute verbatim or modified
 * copies of this license document, and changing it is allowed as long
 * as the name is changed.
 *
 *            DO WHAT THE FORK YOU WANT TO PUBLIC LICENSE
 *   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 *
 *  0. You just DO WHAT THE FORK YOU WANT TO.
 */

/*
 * Usage:
 *
 * [rpolzer@grawp onetask]$ gcc -shared -fPIC -o onetask.so onetask.c
 * [rpolzer@grawp onetask]$ ulimit -u 0; exec env -i dash
 * $ ls
 * dash: 1: Cannot fork
 * $ echo *
 * hello hello.c onetask.c onetask.so
 * $ LD_PRELOAD=./onetask.so SHELL=/bin/dash exec ls
 * [onetask] initialized
 * hello  hello.c  onetask.c  onetask.so
 * [onetask] back to shell
 * $
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define WRITE(fd, str) write(fd, str, sizeof(str) - 1)

static pid_t me;

static void init(void) __attribute__((constructor));
static void init(void)
{
	WRITE(2, "[onetask] initialized\n");
	putenv("LD_PRELOAD=");
	me = getpid();
}

static void restore_tty(int fd, int flag)
{
	if(isatty(fd))
		return;
	if(errno == EINVAL) // not a tty, good too
		return;
	// likely cause: EBADF - not a FD
	int new_fd = open("/dev/tty", flag);
	if(new_fd < 0)
		return;
	if(new_fd != fd)
	{
		dup2(new_fd, fd);
		close(new_fd);
	}
}

static void done(void) __attribute__((destructor));
static void done(void)
{
	if(getpid() != me)
		return;
	restore_tty(0, O_RDONLY);
	restore_tty(1, O_WRONLY);
	restore_tty(2, O_WRONLY);
	WRITE(2, "[onetask] back to shell\n");
	const char *shell = getenv("SHELL");
	if(!shell || !*shell)
		shell = "/bin/sh";
	execl(shell, shell, NULL);
}
