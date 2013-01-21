/*
 *        DO WHAT THE FORK YOU WANT TO PUBLIC LICENSE 
 *                    Version 2, December 2004
 *
 * Program Copyright (C) 2013 Rudolf Polzer <divVerent@xonotic.org>
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
 * [rpolzer@grawp onetask]$ gcc -shared -fPIC -std=c11 -o onetask.so onetask.c
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
#include <signal.h>

#define WRITE(fd, str) write(fd, str, sizeof(str) - 1)

static pid_t me = 0;
static const char *shell;

static void restore_tty(int fd, int flag)
{
	if(!fsync(fd))
		return;
	if(errno != EBADF)
		return;
	// if we get here, we know fd is an invalid fd
	int new_fd = open("/dev/tty", flag);
	if(new_fd < 0)
		return;
	if(new_fd != fd)
	{
		dup2(new_fd, fd);
		close(new_fd);
	}
}

static void new_shell(void)
{
	me = 0; // don't call a second time
	restore_tty(0, O_RDONLY);
	restore_tty(1, O_WRONLY);
	restore_tty(2, O_WRONLY);
	WRITE(2, "[onetask] back to shell\n");
	unsetenv("LD_PRELOAD");
	if(!shell)
		shell = getenv("SHELL");
	if(shell && *shell)
		execle(shell, shell, NULL, environ);
	execle("/bin/sh", "/bin/sh", NULL, environ);
	WRITE(2, "[onetask] cannot get a shell - BAD BAD BAD\n");
	// TODO loop asking for binary to spawn
	_exit(42);
}

static void signal_handler(int signo)
{
	char x;
	if(getpid() != me)
		return;
	WRITE(2, "[onetask] caught signal ");
	if(signo >= 100)
	{
		x = '0' + (signo / 100);
		write(2, &x, 1);
	}
	if(signo >= 10)
	{
		x = '0' + ((signo % 100) / 10);
		write(2, &x, 1);
	}
	x = '0' + (signo % 10);
	write(2, &x, 1);
	WRITE(2, "\n");
	new_shell();
}

static void catch_ALL_the_signals(void)
{
	int i;
	struct sigaction sa;
	sa.sa_handler = signal_handler;
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0;

	// first all fatal signals

	// standard
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);
	sigaction(SIGALRM, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);

	// extensions
#ifdef SIGBUS
	sigaction(SIGBUS, &sa, NULL);
#endif
#ifdef SIGPOLL
	sigaction(SIGPOLL, &sa, NULL);
#endif
#ifdef SIGPROF
	sigaction(SIGPROF, &sa, NULL);
#endif
#ifdef SIGSYS
	sigaction(SIGSYS, &sa, NULL);
#endif
#ifdef SIGTRAP
	sigaction(SIGTRAP, &sa, NULL);
#endif
#ifdef SIGVTALRM
	sigaction(SIGVTALRM, &sa, NULL);
#endif
#ifdef SIGXCPU
	sigaction(SIGXCPU, &sa, NULL);
#endif
#ifdef SIGXFSZ
	sigaction(SIGXFSZ, &sa, NULL);
#endif
#ifdef SIGIOT
	sigaction(SIGIOT, &sa, NULL);
#endif
#ifdef SIGEMT
	sigaction(SIGEMT, &sa, NULL);
#endif
#ifdef SIGSTKFLT
	sigaction(SIGSTKFLT, &sa, NULL);
#endif
#ifdef SIGIO
	sigaction(SIGIO, &sa, NULL);
#endif
#ifdef SIGPWR
	sigaction(SIGPWR, &sa, NULL);
#endif
#ifdef SIGLOST
	sigaction(SIGLOST, &sa, NULL);
#endif
#ifdef SIGUNUSED
	sigaction(SIGUNUSED, &sa, NULL);
#endif
#ifdef SIGTHR
	sigaction(SIGTHR, &sa, NULL);
#endif
#ifdef SIGRTMIN
	for(i = SIGRTMIN; i <= SIGRTMAX; ++i)
		sigaction(i, &sa, NULL);
#endif

	// and then ignore all stopping signals
	sa.sa_handler = SIG_IGN;
	sigaction(SIGTSTP, &sa, NULL);
	sigaction(SIGTTIN, &sa, NULL);
	sigaction(SIGTTOU, &sa, NULL);
}

#if __STDC_VERSION__ >= 201112L
static void quick_exit_handler(void)
{
	if(getpid() != me)
		return;
	WRITE(2, "[onetask] caught quick_exit\n");
	new_shell();
}
#endif

static void init(void) __attribute__((constructor));
static void init(void)
{
	WRITE(2, "[onetask] initialized\n");
	unsetenv("LD_PRELOAD");
	shell = getenv("SHELL");
	me = getpid();
#if __STDC_VERSION__ >= 201112L
	at_quick_exit(quick_exit_handler);
#else
#	warning Not compiled using C11, quick_exit cannot be caught
#endif
	catch_ALL_the_signals();
}

static void done(void) __attribute__((destructor));
static void done(void)
{
	if(getpid() != me)
		return;
	WRITE(2, "[onetask] caught exit\n");
	new_shell();
}
