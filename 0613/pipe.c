#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main (int argc, char **argv)
{
	pid_t pid;
	int pfd[2]; 
	char line [BUFSIZ]; //stdio.h파일에 정의된 버퍼 크기로 설정
	int status;

	if (pipe(pfd) <0) { // pipe()함수를 이용해서 pipe 생성
		perror("fork()");
		return -1;
	}

	if ((pid = fork()) < 0) //fork 함수로 자식 프로세스 생성
	{
		perror("fork()");
		return -1;
	}
	else if (pid==0) //자식 프로세스인 경우
	{
		close(pfd[0]); // 읽기를 위한 파일 디스크립터 닫기
		dup2(pfd[1], 1); //표준 출력(1)을 쓰기 위한 파일 디스크립터 pfd[1]로변경

		execl("/bin/date", "date", NULL); //date 명령어 수행
		close(pfd[1]); //쓰기를 위한 파일 디스크립터 닫기
		_exit(127);
	}
	else
	{
		close(pfd[1]); //쓰기를 위한 파일 디스크립터 닫기
		if (read(pfd[0], line, BUFSIZ) < 0){
			perror("read()"); 
			return -1;
		}

		printf("%s", line); //파일 디스크립터로부터 읽은 내용 화면에 표시

		close (pfd[0]); //읽기를 위한 파일~ 닫기 
		waitpid(pid, &status, 0); //자식 프로세스 종료 기다리기
	}

	return 0;
}
