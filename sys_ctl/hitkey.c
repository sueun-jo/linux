#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

//키보드 입력을 처리하기 위한 함수

int kbhit(void){

	struct termios oldt, newt;
	int ch, oldf;

	tcgetattr(0, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(0, TCSANOW, &newt);
	oldf = fcntl(0, F_GETFL, 0);
	fcntl(0, F_SETFL, oldf | O_NONBLOCK); // 입력을 논블로킹 모드
	ch = getchar();

	tcsetattr(0, TCSANOW, &newt);
	fcntl(0, F_SETFL, oldf);

	if (ch != EOF) {
		ungetc(ch, stdin); //앞에서 읽으며 꺼낸 문자 다시 넣기
		return 1;
	}

	return 0;
}

int main (int argc, char **argv) {
	int i=0;
	for (i=0; ; i++){
		if (kbhit()) {
			switch (getchar()) {
				case 'q':
					goto END; //읽은 문자가 q이면 종료
					break;
			};
		}

		printf("%20d\t\t\r", i);
		usleep(100);
	}
END:
	printf("Good Bye!\n");
	return 0;
}
