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
	else if (pid == 0){ //자식 프로세스이면
		g_var++;
		var++;
		printf("Parent %s from Child Process(%d) : %d\n",
				str, getpid(), getppid()); //자식pid는 자식 입장에서 getpid() // 내 부모는 getppid();
	}
	else { //부모 프로세스면
		printf("Child %s from Parrent Procees(%d) : %d\n",
				str, getpid(), pid); //getpid() 부모 입장에서 난 누구? -> 부모 pid 
		sleep (1);
		//sleep 안넣으면 어케되는데? 부모가 먼저 종료될 경우-> 자식은 init(pid 1번)을 부모로 갖게됨
	}

	//변수 내용 출력
	printf ("pid = %d, Global var = %d, var = %d\n",
			getpid(), g_var, var);

	return 0;
}
