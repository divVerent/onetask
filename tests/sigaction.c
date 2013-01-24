#define _GNU_SOURCE
#include <unistd.h>
#include <signal.h>
void dummy(int signo)
{
	write(2, "This line should be seen ONCE...\n", sizeof("This line should be seen ONCE...\n"));
}
int main()
{
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = dummy;
	sa.sa_flags = SA_RESETHAND;
	sigaction(SIGABRT, &sa, NULL);
	raise(SIGABRT);
	raise(SIGABRT);
	return 1;
}
