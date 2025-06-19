#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>//read, close, unlink()등의 syscall을 위한 헤더
#include <sys/stat.h>

#define FIFOFILE "fifo"

int main (int argc, char**argv){
	

	int n, fd;
	char buf [BUFSIZ];
	
	unlink(FIFOFILE); // 기존fifo 파일 삭제

	if (mkfifo(FIFOFILE, 0666) < 0) { // 새로운 fifo file 생성
		perror("mkfifo()"); 
		return -1;
	}

	if ((fd=open(FIFOFILE, O_RDONLY)) < 0) // fifo를 연다
	{
	  perror("open()");
	  return -1;
	}

	while ((n=read(fd, buf, sizeof(buf))) > 0 ) //fifo로부터 데이터 받음
	{	
		buf[n] = '\0'; 
		printf("%s", buf);
	}
	close (fd);
	return 0;
}
