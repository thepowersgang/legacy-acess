/*
 AcessOS v1 Stress Tester
 Fork Bomb
*/
#include "../syscalls.h"

int main(int argc, char *argv[])
{
	int	pid;
	while( !(pid = fork()) );
	kdebug("%i Created\n", pid);
	for(;;);
	return 0;
}
