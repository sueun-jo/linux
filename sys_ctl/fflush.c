#include <stdio.h>
#include <unistd.h>

int main(){
	printf("Hello, ");
	printf("World!");
	//fflush(stdout);
	fflush(NULL);
	_exit(1);
}
