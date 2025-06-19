#include <stdio.h>
#include <signal.h>

void myHandler(int signum) {
    printf("시그널 %d 발생!\n", signum);
}

int main() {
    signal(SIGINT, myHandler);  // Ctrl+C 입력 시 myHandler 호출
    while (1);  // 무한 루프로 대기
    return 0;
}
