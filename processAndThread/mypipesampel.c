#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h> //waitpid()함수를 위해 사용
#include <string.h>

int main (int argc, char **argv) {


    pid_t pid;
    int pfd[2]; //파일디스크립터 담을거니까용
    
    char line [BUFSIZ]; // 자식 -> 부모로 갈 메시지 담을 변수
    int status; 

    if (pipe(pfd) < 0){ //파이프 오류 터지면
        perror("problems : pipe()"); 
        return -1;
    }

    if ( (pid = fork()) < 0 ) //fork 오류 터지면
    {
        perror ("problem : fork()");
        return -1;
    }

    else if ( pid == 0 ) //자식이면
    {
        // pfd[2] = {3,4} pfd[0]은 read / pfd[1]는 write
        close (pfd[0]); // 자식은 쓰기만 할거니까 read를 닫음
        
        dup2 (pfd[1], 1); // 표준출력(1)을 pfd[1]에 연결 -> "표준출력이 파이프로 흘러간다"
        const char* msg = "Hello from child";
        write(pfd[1], msg, strlen(msg)); //쓰기 빼먹으면 안됨
        close (pfd[1]); //다 썼으니까 write도 닫음
        _exit(127);
    }

    else //부모인 경우
    {
        close (pfd[1]); //write을 닫음
        if (read (pfd[0], line, BUFSIZ) < 0 )//읽을게 없으면
        {
            perror ("problem : read()");
            return -1;
        }

        printf("%s", line); //pfd[0]에 담긴 내용을 읽은 line을 출력
        close (pfd[0]); //read를 닫음

        waitpid (pid, &status, 0); //자식 프로세스 종료 기다리기
    }

    return 0;
}