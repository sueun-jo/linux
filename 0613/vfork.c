#include <stdio.h>
#include <unistd.h>

static int g_var = 1; //data영역의 초기화된 변수
char str[] = "PID";

int main (int argc, char ** argv){
	int var; //stack 영역의 지역 변수
	pid_t pid;
	var = 88;
	
	printf("mainprocess %s :%d\n ", str, getpid());

	if ( (pid = vfork()) < 0 ) //포크 함수 에러시 처리
		perror("[ERROR] : vfork()"); 
	else if (pid == 0){ //자식 프로세스
		g_var++; //변수 값 변경
		var++;
		printf("Parent %s from Child Process(%d) : %d\n",
				str, getpid(), getppid());
		
		printf ("pid = %d, Global var = %d, var = %d\n",
			getpid(), g_var, var);
		_exit(0);
	}
	else {
		printf("Child %s from Parrent Procees(%d) : %d\n",
				str, getpid(), pid);
	
	}

	//변수 내용 출력
	printf ("pid = %d, Global var = %d, var = %d\n",
			getpid(), g_var, var);


	return 0;
}



