#include <stdio.h>
#include <unistd.h>
#include <signal.h>

void sigHdl(int no){
	printf("%d\n", no);
}

int main(int argc, char **argv)
{
	signal (SIGINT, sigHdl);

	for (int i=0; ; i++){
		printf("%10d\r", i);
		fflush(NULL);
		sleep(1);
	}

	return 0;
}
