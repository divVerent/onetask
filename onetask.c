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
 * [rpolzer@grawp onetask]$ gcc -shared -fPIC -std=c11 -o onetask.so onetask.c -ldl
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
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>

#define WRITE(fd, str) write(fd, str, sizeof(str) - 1)

static pid_t me = 0;
static const char *shell;

extern char **environ;
static const char *findenv(const char *name)
{
	size_t l = strlen(name);
	char **p = environ;
	if(!p)
		return NULL;
	while(*p)
	{
		if(!strncmp(*p, name, l))
			if((*p)[l] == '=')
				return &(*p)[l+1];
		++p;
	}
	return NULL;
}
static void killenv(const char *name)
{
	size_t l = strlen(name);
	char **p = environ;
	if(!p)
		return;
	while(*p)
	{
		if(!strncmp(*p, name, l))
			if((*p)[l] == '=')
				(*p)[l+1] = 0;
		++p;
	}
}

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

static void new_shell(void) __attribute__((noreturn));
static void new_shell(void)
{
	me = 0; // don't call a second time
	restore_tty(0, O_RDONLY);
	restore_tty(1, O_WRONLY);
	restore_tty(2, O_WRONLY);
	WRITE(2, "[onetask] back to shell\n");
	killenv("LD_PRELOAD");
	if(!shell)
		shell = findenv("SHELL");
	if(shell && *shell)
		execle(shell, shell, (char *) NULL, environ);
	execle("/bin/sh", "/bin/sh", (char *) NULL, environ);
	WRITE(2, "[onetask] cannot get a shell - BAD BAD BAD\n");
	// TODO loop asking for binary to spawn
	for(;;)
	{
	}
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

struct signalinfo
{
	int sig;
	void (*handler) (int);
};
struct signalinfo allsignals[] = {
	// first all standard fatal signals
	{SIGHUP, signal_handler},
	{SIGINT, signal_handler},
	{SIGQUIT, signal_handler},
	{SIGILL, signal_handler},
	{SIGABRT, signal_handler},
	{SIGFPE, signal_handler},
	{SIGSEGV, signal_handler},
	{SIGPIPE, signal_handler},
	{SIGALRM, signal_handler},
	{SIGTERM, signal_handler},
	{SIGUSR1, signal_handler},
	{SIGUSR2, signal_handler},

	// then all optional fatal signals
#ifdef SIGBUS
	{SIGBUS, signal_handler},
#endif
#ifdef SIGPOLL
	{SIGPOLL, signal_handler},
#endif
#ifdef SIGPROF
	{SIGPROF, signal_handler},
#endif
#ifdef SIGSYS
	{SIGSYS, signal_handler},
#endif
#ifdef SIGTRAP
	{SIGTRAP, signal_handler},
#endif
#ifdef SIGVTALRM
	{SIGVTALRM, signal_handler},
#endif
#ifdef SIGXCPU
	{SIGXCPU, signal_handler},
#endif
#ifdef SIGXFSZ
	{SIGXFSZ, signal_handler},
#endif
#ifdef SIGIOT
	{SIGIOT, signal_handler},
#endif
#ifdef SIGEMT
	{SIGEMT, signal_handler},
#endif
#ifdef SIGSTKFLT
	{SIGSTKFLT, signal_handler},
#endif
#ifdef SIGIO
	{SIGIO, signal_handler},
#endif
#ifdef SIGPWR
	{SIGPWR, signal_handler},
#endif
#ifdef SIGLOST
	{SIGLOST, signal_handler},
#endif
#ifdef SIGUNUSED
	{SIGUNUSED, signal_handler},
#endif
#ifdef SIGTHR
	{SIGTHR, signal_handler},
#endif

	// and then ignore all stopping signals
	{SIGTSTP, SIG_IGN},
	{SIGTTIN, SIG_IGN},
	{SIGTTOU, SIG_IGN},

	// end of list
	{0, NULL}
};

static int is_default_sigaction(const struct sigaction *sa,
                                void (*redir_to) (int))
{
	if(sa->sa_flags & SA_SIGINFO)
		return 0;
	if(sa->sa_handler == SIG_DFL)
		return 1;
	if(sa->sa_handler == redir_to)
		return 1;
	return 0;
}

static void catch_ALL_the_signals(void)
{
	static int (*real_sigaction) (int, const struct sigaction *restrict,
	                              struct sigaction *restrict) = NULL;
	struct signalinfo *inf;
	struct sigaction sa, osa;

	if(!real_sigaction)
		real_sigaction = dlsym(RTLD_NEXT, "sigaction");

	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0;

	for(inf = allsignals; inf->sig; ++inf)
	{
		sa.sa_handler = inf->handler;
		real_sigaction(inf->sig, NULL, &osa);
		// don't override already set handlers
		if(is_default_sigaction(&osa, SIG_DFL))
			real_sigaction(inf->sig, &sa, NULL);
	}
#ifdef SIGRTMIN
	{
		int i;
		for(i = SIGRTMIN; i <= SIGRTMAX; ++i)
		{
			sa.sa_handler = inf->handler;
			real_sigaction(i, NULL, &osa);
			// don't override already set handlers
			if(is_default_sigaction(&osa, SIG_DFL))
				real_sigaction(i, &sa, NULL);
		}
	}
#endif
}

int sigaction(int sig, const struct sigaction *restrict act,
                     struct sigaction *restrict oact)
{
	static int (*real_sigaction) (int, const struct sigaction *restrict,
	                              struct sigaction *restrict) = NULL;
	int ret;
	struct sigaction myact;
	struct signalinfo *inf;
	void (*redir_to) (int);
	int do_redir;

	if(!real_sigaction)
		real_sigaction = dlsym(RTLD_NEXT, "sigaction");

	redir_to = NULL;
	for(inf = allsignals; inf->sig; ++inf)
		if(sig == inf->sig)
			redir_to = inf->handler;
	if(sig >= SIGRTMIN && sig <= SIGRTMAX)
		redir_to = signal_handler;

	if(act && redir_to && is_default_sigaction(act, redir_to))
	{
		myact = *act;
		myact.sa_handler = redir_to;
		act = &myact;
	}

	// supporting this flag would be quite complex, so we just try living
	// without it and seeing what breaks; this MIGHT cause endless loops
	// in code relying on it, but if this happens, just use ^C (or ^\) to
	// send a SIGINT (or SIGQUIT) to cancel the cycle
	if(act->sa_flags & SA_RESETHAND)
	{
		WRITE(2, "[onetask] SA_RESETHAND is not supported\n");
		myact = *act;
		myact.sa_flags &= ~SA_RESETHAND;
		act = &myact;
	}

	ret = real_sigaction(sig, act, oact);

	if(!ret && oact && redir_to && is_default_sigaction(oact, redir_to))
		oact->sa_handler = SIG_DFL;

	return ret;
}

void (*bsd_signal (int sig, void (*handler) (int))) (int)
{
	struct sigaction sa;
	struct sigaction osa;
	int ret;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = handler;
	ret = sigaction(sig, &sa, &osa);
	if(ret)
		return SIG_ERR;
	return osa.sa_handler;
}

void (*__sysv_signal (int sig, void (*handler) (int))) (int)
{
	struct sigaction sa;
	struct sigaction osa;
	int ret;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESETHAND;
	sa.sa_handler = handler;
	ret = sigaction(sig, &sa, &osa);
	if(ret)
		return SIG_ERR;
	return osa.sa_handler;
}

// glibc defaults to BSD semantics, so do we
void (*signal (int sig, void (*handler) (int))) (int)
{
	return bsd_signal(sig, handler);
}

void _exit(int ret)
{
	static int (*real__exit) (int) = NULL;
	if(!real__exit)
		real__exit = dlsym(RTLD_NEXT, "_exit");
	if(getpid() != me)
		real__exit(ret);
	WRITE(2, "[onetask] caught _exit\n");
	new_shell();
}

void _Exit(int ret)
{
	static int (*real__Exit) (int) = NULL;
	if(!real__Exit)
		real__Exit = dlsym(RTLD_NEXT, "_Exit");
	if(getpid() != me)
		real__Exit(ret);
	WRITE(2, "[onetask] caught _Exit\n");
	new_shell();
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
	killenv("LD_PRELOAD");
	shell = findenv("SHELL");
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
