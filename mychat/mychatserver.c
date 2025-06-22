#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <signal.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 5432
#define BUFSIZE 1024

/* 시그널 핸들러 */
void handle_child (int sig){
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) ;
}

int main (int argc, char **argv){

    int listenSocket, clientSocket; //listenSocket은 연결대기용, clientSocket은 client와 통신용
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);
    char buf [BUFSIZE];
    pid_t pid;

    int pipeToParent [2]; // 자식 -> 부모로 가는 파이프 fd
    int pipeToChild [2]; //부모 -> 자식으로 가는 파이프 fd
     
    signal (SIGCHLD, handle_child);

    listenSocket = socket (AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0){
        perror ("socket error\n");
        return 1;
    }

    memset (&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /* bind로 주소 설정 */
    if (bind (listenSocket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error\n");
        return 1;
    }

    /* 동시 접속 클라이언트 처리 대기 큐 listen */
    listen (listenSocket, 10);
    printf("Server is Listening on Port : %d . . . \n", SERVER_PORT);
    
    /* accept : 클라이언트가 요청해서 생긴 socket이기에 clinetSocket이라 명명 */
    clientSocket = accept (listenSocket, (struct sockaddr*)&client_addr, &addrlen);
    if (clientSocket < 0){
        perror ("accept error\n");
        return 1;
    }

    printf ("[Info] Client connected.\n");

    //pipe 생성 드가자
    pipe (pipeToParent); // 자식 -> 부모
    pipe (pipeToChild); // 부모 -> 자식

    pid = fork();

    if (pid == 0){ //자식 프로세스

    }

    else { // 부모 프로세스

    }

    return 0;
}