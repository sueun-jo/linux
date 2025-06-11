#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(void){
	int str [BUFSIZ];
	scanf("%s", str);
	fprintf(stderr, "%s : %s\n", str, strerror(errno));
	return 0;
}
