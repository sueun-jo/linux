#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

sem_t *sem; //전역변수 for semaphore
static int cnt = 0; //semaphore에서 사용할 임계 구역 변수

void p(){ //세마포어 p연산
	sem_post(sem);
}

void v(){ //세마포어 v연산
	sem_wait(sem);
}

void *pthreadV (void *arg) //v연산을 수행하기 위한 함수
{
 	for(int i=0; i<10; i++){
		if(cnt>=7) usleep(100); // 7이상이면 100ms 대기
		v();
		cnt++;
		printf("increase : %d\n", cnt);
		fflush(NULL);
	}

	return NULL;
}


void *pthreadP (void *arg) //p연산을 수행하기 위한 함수
{
 	for(int i=0; i<10; i++){
		p();  //세마포어가 0이 되면 blocked
		cnt--;
		printf("decrease : %d\n", cnt);
		fflush(NULL);
		usleep(100); //100ms 대기
	}

	return NULL;
}

int main (int argc, char **argv)
{
	pthread_t ptV, ptP; //스레드를 위한 자료형

	const char* name = "posix_sem";
	unsigned int value = 7; //세마포어 값

	//세마포어 open
	sem = sem_open(name, O_CREAT, S_IRUSR | S_IWUSR, value);

	pthread_create(&ptV, NULL, pthreadV, NULL); //스레드 생성
	pthread_create(&ptP, NULL, pthreadP, NULL);
	pthread_join(ptV, NULL); //스레드가 종료될 때까지 대기
	pthread_join(ptP, NULL);

	//다 쓴 세마포어 닫고 정리
	sem_close(sem);
	printf("sem_destroy() : %d\n", sem_destroy(sem));

	//세마포어 삭제
	sem_unlink(name);

	return 0;
}
