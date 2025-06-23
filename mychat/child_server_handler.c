#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //pid_t 
#include <signal.h>
#include <sys/socket.h>
#include "resource.h"
#include "debug.h"

static int socket_fd;
static int pipe_to_child_fd;
static int pipe_to_parent_fd;
static pid_t parent_pid;

char buf[BUFSIZE];

/*child_server_handler는 client한테 받은 메세지가 있으면 부모한테 보내고
부모한테 받아서 다시 client한테 쏴준다가 컨셉임*/

// SIGUSR2(부모->자식) 수신 시 : 부모가 보낸 메시지를 pipe로 read해서 클라이언트에게 전송
void handle_sigusr2 (int sig){
    memset(buf, 0, BUFSIZE);
    dprint("recv buf 초기화 했음, 이제 문제 없을거에용~!");
    int n = read(pipe_to_child_fd, buf, BUFSIZE);
    if (n>0){ //읽을 게 있으면
        buf[n] = '\0'; //문자열 끝 처리 해주고
        send (socket_fd, buf, n, 0); //client한테 쏴준다
    }
}

//자식 프로세스 전용 handler 실행 함수
void run_child_handler (int client_socket_fd, int pipe_from_parent, int pipe_to_parent){
    socket_fd = client_socket_fd;
    pipe_to_child_fd = pipe_from_parent; 
    pipe_to_parent_fd = pipe_to_parent;
    parent_pid = getppid(); //부모 pid 얻어옴

    signal (SIGUSR2, handle_sigusr2); //시그널 등록 
    
    while (1) {
        memset (buf, 0, BUFSIZE); //buf초기화
        int n = recv (socket_fd, buf, BUFSIZE-1, 0);
        dprint ("buf : %s\n", buf); 
        if (n<=0) {
            eprint("recv는 했는데 n<=0이라서 먼가 읽을게 없음. . . . .");
            break;
        }
        buf[n] = '\0';
        //부모에게 메시지 전달
        write (pipe_to_parent_fd, buf, n); //부모한테 쓰고
        kill(parent_pid, SIGUSR1); //부모 pid에 sigusr1 발생
    }

    close (socket_fd);
    exit(0);
}