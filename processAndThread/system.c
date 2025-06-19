#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

int system(const char *cmd) //fork(), exec(), waitpid() 함수 사용
{
	pid_t pid;
	int status;

	if ( (pid=fork())<0) { //fork() 수행 시 에러가 발생했을 때 처리
		status = -1;
	}
	else if (pid==0){ // 자식 프로세스 처리
		execl("/bin/sh", "sh", "-c", cmd, (char *)0);
		_exit(127); //excel 함수 에러 사항
	}
	else {
		while (waitpid(pid, &status, 0) < 0) // 자식 프로세스 종료 대기
			if (errno != EINTR) { // waitpid() 함수에서 EINTR 이 아닌 경우
				status = -1;
				break;
			}
	}

	return status;
}

int main (int argc, char **argv, char **envp)
{
	while (*envp)
		printf("%s\n", *envp++);

	system ("who"); //who utility 수행
	system ("nocommand"); // 오류 사항의 수행
	system ("cal"); //cal utility 수행

	return 0;
}
