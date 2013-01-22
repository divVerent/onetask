#define _POSIX_SOURCE
#include <unistd.h>
#include <signal.h>
int main()
{
	kill(getpid(), SIGABRT);
	sleep(1);
	sleep(1);
	return 1;
}
