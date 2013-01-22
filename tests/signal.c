#define _GNU_SOURCE
#include <unistd.h>
#include <signal.h>
int main()
{
	signal(SIGABRT, SIG_IGN);
	raise(SIGABRT);
	write(2, "This line should be seen...\n", sizeof("This line should be seen...\n"));
	signal(SIGABRT, SIG_DFL);
	raise(SIGABRT);
	return 1;
}
