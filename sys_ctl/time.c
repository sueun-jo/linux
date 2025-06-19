#include <stdio.h>
#include <time.h>
#include <sys/time.h> //getimeofday()
#include <stdlib.h> 


int main(int argc, char **argv)
{

	int i, j;
	time_t rawtime;
	struct tm *tm;
	char buf[BUFSIZ];
	struct timeval mytime;

	time(&rawtime); //현재 시간
	printf("time: %u\n", (unsigned) rawtime); //현재 시간 화면 출력
	gettimeofday(&mytime, NULL); //현재 시간
	printf("gettimeofday : %ld/%d\n", mytime.tv_sec, mytime.tv_usec);

	printf("ctime: %s", ctime(&rawtime)); //현재 시간 문자열로 출력

	putenv("TZ=PST3PDT"); //환경 변수 설정
	tzset(); //TZ변수 설정
	tm = localtime(&rawtime); //tm=gmtime(&rawtime);
	printf("asctime: %s\n", asctime(tm)); //현재시간을 tm구조체로 출력

	strftime(buf, sizeof(buf), "%a %b %e %H:%M %S %Y",tm);
	printf("strftime: %s", buf);


	return 0;
}

