#include <stdio.h>
#include <unistd.h>

static int g_var = 1; //data영역의 초기화된 변수
char str[] = "PID";

int main (int argc, char ** argv){
	int var; //stack 영역의 지역 변수
	pid_t pid;
	var = 92;
	
	printf("mainprocess %s :%d\n ", str, getpid());
	if ( (pid = fork()) < 0 ) //포크 함수 에러시 처리
		perror("[ERROR] : fork()"); 
	else if (pid == 0){
		g_var++;
		var++;
		printf("Parent %s from Child Process(%d) : %d\n",
				str, getpid(), getppid());
	}
	else {
		printf("Child %s from Parrent Procees(%d) : %d\n",
				str, getpid(), pid);
		sleep (1);
	}

	//변수 내용 출력
	printf ("pid = %d, Global var = %d, var = %d\n",
			getpid(), g_var, var);


	return 0;
}
