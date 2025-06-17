#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>
#include <sys/socket.h>

/*socketpair() 함수를 이용해서
 두 프로세스 사이에 데이터를 교환하는 코드를 작성해보자.
 기본적인 코드는 앞 장의 pipe를 이용하는 것과 비슷하다.*/
int main (int argc, char **argv){
	
	int ret, sock_fd[2]; 
	int status;
	char buf[] = "Hello, World!", line[BUFSIZ];
	pid_t pid;
	
	//한 쌍의 소켓 생성
	ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, sock_fd);
	if (ret == -1){
		perror("socketpair()");
		return -1;
	}

	printf("sorket 1: %d\n", sock_fd[0]);
	printf("sorket 2: %d\n", sock_fd[1]);

	if ((pid = fork()) < 0) { //fork() 함수 실행 에러 시 처리
		perror ("fork()");
	}
	else if (pid==0){ //자식ps 처리
		write (sock_fd[0], buf, strlen(buf)+1); //부모ps로 data 보내기
		printf("Data send : %s\n", buf);

		close (sock_fd[0]); //소켓 닫기
	}
	else { //부모 ps처리
		wait(&status); // 자식ps 종료 대기
		read(sock_fd[1], line, BUFSIZ); //자식ps에서 온 데이터 읽기
		printf("Received data : %s\n", line);

		close (sock_fd[1]); //소켓 닫기
	
	}
	return 0;
}
