#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void printSigset(sigset_t *set); //sigset_t에 설정된 시그널 표시
static void sigHandler(int); //시그널 처리용 핸들러

int main (int argc, char **argv)
{
	sigset_t pset; //블록할 시그널을 등록할 sigset_t형
	sigemptyset(&pset); //모두 0으로 설정
	sigaddset(&pset, SIGQUIT);
	sigaddset(&pset, SIGRTMIN);
	sigprocmask(SIG_BLOCK, &pset, NULL); //현재의 시그널 마스크에 추가

	printSigset(&pset); //현재 설정된 sigset_t를 화면으로 출력

	if (signal(SIGINT, sigHandler) == SIG_ERR)
	{
		perror("signal() : SIGINT");
		return -1;
	}

	if (signal (SIGUSR1, sigHandler) == SIG_ERR) {
		perror("signal() : SIGUSR1");
		return -1;
	}

	
	if (signal(SIGUSR2, sigHandler) == SIG_ERR)
	{
		perror("signal() : SIGUSR2");
		return -1;
	}
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
	{
		perror("signal() : SIGPIPE");
		return -1;
	}
	while(1) pause(); //시그널 처리를 위해 대기

	return 0;
}

static void sigHandler(int signo) //시그널 번호를 인자로 받는다
{
	if (signo == SIGINT) {
		printf("SIGINT is catched : %d\n", signo);
		exit(0);
	}

	else if (signo == SIGUSR1) {
		printf("SIGUSR1 is catched\n");
	}
	else if (signo == SIGUSR2) {
		printf("SIGUSR2 is catched\n");
	}
	else if (signo == SIGQUIT) {
		printf("SIGQUIT is catched\n");
		sigset_t uset;
		sigemptyset(&uset);
		sigaddset(&uset, SIGINT);
		sigprocmask(SIG_UNBLOCK, &uset, NULL);
	}
	else {
		fprintf(stderr, "Catched signal : %s\n", strsignal(signo));
	}
}

static void printSigset(sigset_t *set)
{
	for (int i=1; i< NSIG; i++){
		printf ((sigismember(set, i))? "1" : "0");
	}

	putchar('\n');
}
