#include <stdio.h>
#include <unistd.h>
#include <sys/sem.h>

static int semid;

void p(){
	struct sembuf pbuf;
	pbuf.sem_num = 0; 
	pbuf.sem_op = -1;
	pbuf.sem_flg = SEM_UNDO; 
	 
	if (semop(semid, &pbuf, 1) == -1) //세마포어의 감소 연산 수행
		perror("p : semop()");
}

void v(){
	struct sembuf vbuf;
	vbuf.sem_num = 0;
	vbuf.sem_op = 1;
	vbuf.sem_flg = SEM_UNDO;
	
	if (semop(semid, &vbuf, 1) == -1) //세마포어의 증가 연산을 수행
		perror ("v : semop()");
}

int main(int argc, char **argv)
{
	union semun //semun 공용체
	{
		int val;
		struct semid_ds *buf;
		unsigned short int *array;
	}arg;

	//세마포어에 대한 채널 얻기
	if ((semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666)) == -1)
	{
		perror ("semget()");
		return -1;
	}

	arg.val = 1; //세마포어 값을 1로 설정
	if (semctl(semid, 0, SETVAL, arg) == -1)
	{
		perror("semctl() : SETVAL");
		return -1;
	}
	
	int cnt = 0;

	while (1) {
		if (cnt >= 8) {
			cnt--; 
			p();
			printf("decrease : %d\n", cnt);
			break;
		}

		else {
			cnt++;
			v();
			printf("increase : %d\n", cnt);
			usleep(100);
		}
	}

	//세마포어에 대한 채널 삭제
	if (semctl(semid, 0, IPC_RMID, arg) == -1) 
	{
		perror("semctl() : IPC_RMID");
		return -1;
	}

	return 0;
}
