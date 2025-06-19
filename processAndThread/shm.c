#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>

#define SHM_KEY 0x12345 //shm을 위한 key

int main (int argc, char **argv)
{
	int pid, shmid;
	int *cVal;
	void *shmmem = (void *)0;
	
	if ( (pid=fork()) == 0 ) //자식 프로세스를 위한 설정
	{
		//공유 메모리 공간을 가져온다
		shmid = shmget( (key_t)SHM_KEY, sizeof(int), 0);
		if (shmid == -1) {
			perror("shmget()");
			return -1;
		}

		//shm를 사용하기 위해 프로세스 메모리에 붙임
		shmmem = shmat(shmid, (void *)0, 0666 | IPC_CREAT);
		if (shmmem == (void *)-1)
		{
			perror("shmat()");
			return -1;
		}

		cVal = (int *)shmmem;
		*cVal = 1;
		for (int i=0; i<3; i++){
			*cVal += 1;
			printf("Child(%d) : %d\n", i, *cVal);
			sleep(1);
		}
	}
	else if (pid > 0) // 부모 프로세스로 공유 메모리의 내용을 표시 
	{
		//공유 메모리 공간을만든다
		shmid = shmget( (key_t)SHM_KEY, sizeof(int), 0666 | IPC_CREAT);
		if (shmid == -1) {
			perror("shmget()");
			return -1;
		}

		//shm를 사용하기 위해 프로세스 메모리에 붙임
		shmmem = shmat(shmid, (void *)0, 0);
		if (shmmem == (void *)-1)
		{
			perror("shmat()");
			return -1;
		}

		cVal = (int *) shmmem;
		for (int i=0; i<3; i++){
			sleep(1);
			printf("Parent(%d) : %d\n", i, *cVal);
		}
	}

	shmctl(shmid, IPC_RMID, 0);

	return 0;
}
