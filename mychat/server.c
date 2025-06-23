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
#include "resource.h"
#include "parent_server_handler.h"
#include "child_server_handler.h"
#include "debug.h"

#define SERVER_PORT 5432
#define BUFSIZE 1024

// void run_child_handler(int client_socket_fd, int pipe_from_parent, int pipe_to_parent);

UserInfo users[MAX_CLIENT]; //user 배열
RoomInfo rooms[MAX_ROOM]; //room 배열

/* zombie 방지 시그널 핸들러 */
void handle_child (int sig){
    int status;
    pid_t pid;
    while (waitpid(-1, &status, WNOHANG) > 0){
        int idx = find_user_idx_by_pid(pid);
        if (idx != -1){
            users[idx].is_activated = 0;
            printf("[server Info] Released slot for PID %d", pid);
        }
    }
}

/* 사용하지 않는 user slot 찾기 */
int find_emtpy_user_slot(){
    for (int i=0; i<MAX_CLIENT; i++){
        if (!users[i].is_activated) return i;
    }
    return -1;
}

int main (int argc, char **argv){

    int listen_socket, client_socket; //listen_socket은 연결대기용, client_socket은 client와 통신용
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);
    char buf [BUFSIZE];
    pid_t pid;
    

    signal (SIGCHLD, handle_child); //자식 죽으면 handle_child로 자원회수하겠다는 소리
    signal (SIGUSR1, handle_sigusr1); //SIGUSR1 발생시 handle_sigusr1 하겠다는 소리
    signal (SIGUSR2, handle_sigusr2); //SIGUSR2 발생시 hadnle_sigusr2 하겠다는 소리

    listen_socket = socket (AF_INET, SOCK_STREAM, 0); //tcp socket 통신 하겠습니다
    if (listen_socket < 0){
        perror ("socket error : ");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /* bind로 주소 설정 */
    if (bind (listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error\n");
        return -1;
    }

    /* 동시 접속 클라이언트 처리 대기 큐 listen */
    listen (listen_socket, MAX_CLIENT); 
    printf("[Server Info] Server is Listening on Port : %d . . . \n", SERVER_PORT);
    
    while (1) {
    
        /* accept : 클라이언트가 요청해서 생긴 socket이기에 client_socket이라 명명 */
        /* accept하면 서버:클라이언트 1:1 관계를 만들어야 하니까 fork() 해야됨 */
        client_socket = accept (listen_socket, (struct sockaddr*)&client_addr, &addrlen);
        if (client_socket < 0){                   
            perror ("accept error : ");
            continue;
        }
        
        int user_idx = find_emtpy_user_slot();

        if (user_idx == -1) {
            printf("[Info] Connection Rejected : Too many clients\n");
            close (client_socket);
            continue;
        }
        
        //pipe 생성
        int pipe_to_parent[2];
        int pipe_to_child[2];
        pipe (pipe_to_parent); // 자식 -> 부모 : 자식 write, 부모 read
        pipe (pipe_to_child); // 부모 -> 자식 : 부모 write, 자식 read

        pid = fork(); // 부모 - 자식 나눠짐

        if (pid == 0){ //자식 프로세스
            close (listen_socket);
            //자식은 listen_socket 필요 없음 : accept은 부모만 한다
            
            //자식은 부모한테 write만 하면 됨, read 필요 없음
            close (pipe_to_parent[PIPE_READ]);
            //자식은 부모로부터 read만 하면 됨, write 필요 없음
            close (pipe_to_child[PIPE_WRITE]);

            //run_child_hadler() 등으로 분기 예정
            run_child_handler (client_socket, pipe_to_child[PIPE_WRITE], pipe_to_parent[PIPE_READ]);

            exit(0);
        }

        else if (pid > 0){ // 부모 프로세스

            //부모는 자식한테 write만 하면 됨, read 필요 없음
            close (pipe_to_child[PIPE_READ]);
            //부모는 자식으로부터 read만 하면 됨, write 필요 없음
            close (pipe_to_parent[PIPE_WRITE]); 

            users[user_idx].pid = pid; 
            users[user_idx].rid = -1; //rid -1로 초기화
            users[user_idx].client_socket_fd = client_socket;
            users[user_idx].pipe_to_child[PIPE_WRITE] = pipe_to_child[PIPE_WRITE];
            users[user_idx].pipe_to_parent[PIPE_READ] = pipe_to_parent[PIPE_READ];
            users[user_idx].is_activated = 1;
            
            printf("[INFO] New Client(no.%d)is Connected.\n", user_idx);
            dprint("PID : %d, idx : %d", pid, user_idx);
            run_parent_handler();
        }
        else {
            perror ("fork error : ");
            close (client_socket);
        }
    }

    close (listen_socket);
    return 0;
}