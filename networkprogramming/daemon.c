#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/resource.h>

int main (int argc, char **argv)
{
	struct sigaction sa; //시그널 처리를 위한 시그널 액션
	struct rlimit rl; 
	int fd0, fd1, fd2, i;
	pid_t pid;


	if (argc<2)
	{
		printf("usage : %s command\n", argv[0]);
		return -1;
	}
	
	//파일생성을 위한 마스크를 0으로 설정 
	umask(0);

	//사용할 수 있는 최대 파일 디스크립터 수 얻기
	if (getrlimit (RLIMIT_NOFILE, &rl) < 0 ) 
		perror ("error()");
	else if (pid!=0) //부모 프로세스 종료 
		return 0;

	//터미널 제어할 수 있도록 세션 리더 만들기
	setsid();
	
	//터미널 제어와 관련된 시그널 무시
	sa.sa_handler = SIG_IGN; 
	sigemptyset(&sa.sa_mask); 
	sa.sa_flags = 0;
	if (sigaction(SIGHUP, &sa, NULL) < 0)
		perror ("sigaction() : Can't ignore SIGHUP");
	
	//프로세스 워킹 디렉토리를 '/'로 설정
	if (chdir("/") < 0)
		perror ("cd()");

	//프로세스의 모든 파일 디스크립터를 닫는다 
	if (rl.rlim_max == RLIM_INFINITY) 
		rl.rlim_max = 1024;

	for (i=0; i< rl.rlim_max; i++)
		close (i);

	//파일 디스크립터 0,1,2를 /dev/null로 연결 
	fd0 = open("/dev/null", O_RDWR); 
	fd1 = dup(0);
	fd2 = dup(0);

	//로그 출력을 위한 파일 로그 열기 
	openlog(argv[1], LOG_CONS, LOG_DAEMON);
	if (fd0 != 0 || fd1 != 1 || fd2 != 2){
		syslog(LOG_ERR, "unexpected file descriptors %d %d %d", fd0, fd1, fd2);
		return -1;
	}

	//로그 파일에 정보 수준 로그 출력 
	syslog (LOG_INFO, "Daemon Process");

	while (1){
		//데몬 프로세스로 해야할 일 반복 수행
	}
	
	//시스템 로그 닫음 
	closelog();


	return 0;
}
