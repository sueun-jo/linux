#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(void){
	// close (1);
	//close (0);
	//int n;
	char str[BUFSIZ];
	int fd1, fd2;

	// scanf("%d", &:n);
	read (0, str, BUFSIZ); 
	printf("Hello, World(%s)\n!", str);
	
	return 0;
} 
