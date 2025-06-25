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
#include "debug.h"

#define SERVER_PORT 5432
#define BUFSIZE 1024


RoomInfo rooms[MAX_ROOM]; //room 배열

int from_child_to_parent[2];
int from_parent_to_child[2];

/* zombie 방지 시그널 핸들러 */
void handle_child (int sig){
    int status;
    pid_t pid;
    while (waitpid(-1, &status, WNOHANG) > 0){ //자원 회수
        // // int idx = find_user_idx_by_pid(pid); // 여기 뭔가 이상하다 다시봐야될듯?
        // if (idx != -1){
        //     users[idx].is_activated = 0;
        //     dprint("Released slot for PID %d", pid);
        // }
    }
}

void sig_usr1(int sig){ //부모는 자식이 쓴 거 읽어서 다시 자식server에 write 해줘야함
    char buf[BUFSIZE];
    memset (buf, 0, BUFSIZE);
    int n = read (from_child_to_parent[PIPE_READ], buf, BUFSIZE-1);
    if (n<=0){
        eprint("read error : nothing to read");
    }
    else{ //읽을 게 있으면
        buf[n] = '\0';
        write (from_parent_to_child[PIPE_WRITE], buf, strlen(buf));
        
       // kill (users[user_idx].pid, SIGUSR2); // <- 이부분이 지금 자식pid를 찾아야되는데 이거 어떻게 처리하지?
    }
}

void sig_usr2(int sig){

}

int main (int argc, char **argv){
    int listen_socket, client_socket; //listen_socket은 연결대기용, client_socket은 client와 통신용
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);

    char buf [BUFSIZE];
    pid_t pid;
    
    listen_socket = socket (AF_INET, SOCK_STREAM, 0); //tcp socket 통신 할게요
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
        perror("bind error : ");
        return -1;
    }

    /* 동시 접속 클라이언트 처리 대기 큐 listen */
    listen (listen_socket, MAX_CLIENT); 
    printf("[Server Info] Server is Listening on Port : %d ... \n", SERVER_PORT);
    
    while (1) {
        /* accept : 클라이언트가 요청해서 생긴 socket이기에 client_socket이라 명명 */
        /* accept하면 서버:클라이언트 1:1 관계를 만들어야 하니까 fork() 해야됨 */
        client_socket = accept (listen_socket, (struct sockaddr*)&client_addr, &addrlen);
        if (client_socket < 0){       
            perror ("accept error : ");
            continue;
        }
        
        int user_idx = find_empty_user_slot();

        if (user_idx == -1) {
            printf("[Info] Connection Rejected : Too many clients\n");
            close (client_socket);
            continue;
        }
        
        //pipe 생성
        pipe (from_child_to_parent); // 자식 -> 부모 : 자식 write, 부모 read
        pipe (from_parent_to_child); // 부모 -> 자식 : 부모 write, 자식 read

        pid = fork(); // 부모 - 자식 나눠짐

        if (pid == 0){ //자식 프로세스
            signal (SIGUSR2, sig_usr2); //signal등록 parent가 child한테 너 읽어야된다고 알려줌
            close (listen_socket); //자식은 listen_socket 필요 없음 : accept은 부모만 한다
    
            //자식은 부모한테 write만 하면 됨, read 필요 없음
            close (from_child_to_parent[PIPE_READ]); // 자식은 부모한테 write only, read 필요X
            close (from_parent_to_child[PIPE_WRITE]); // 부모한테 read only, write 필요X

            while(1) {
                memset(buf, 0, BUFSIZE);
                int n = recv (client_socket, buf, BUFSIZE-1, 0);
                if (n<=0) {
                    eprint("recv error : nothing to read");
                    break;
                }
                else {
                    buf[n] = '\0'; //문자열 처리
                    write (from_child_to_parent[PIPE_WRITE], buf, strlen(buf));
                    kill (getppid(), SIGUSR1); //쓰고 나서 SIGUSR1 발생시키고 sig_usr1 수행 
                }
            }
            exit(0);
        }

        else if (pid > 0){ // 부모 프로세스
            signal (SIGCHLD, handle_child); // 시그널 등록
            signal (SIGUSR1, sig_usr1); //자식이 부모한테 쓰고 알려줄거임 : sig_usr1 수행하는 건 부모쪽
            close (from_parent_to_child[PIPE_READ]); //부모는 자식한테 write, read필요 없음
            close (from_child_to_parent[PIPE_WRITE]); //부모는 자식으로부터 read only, write 필요 X
       
            /* UserInfo 구조체 users setter */
            users[user_idx].pid = pid; 
            dprint("users[%d] = %d", user_idx, pid);
            users[user_idx].client_socket_fd = client_socket;
            users[user_idx].from_parent_to_child[PIPE_WRITE] = from_parent_to_child[PIPE_WRITE];
            users[user_idx].from_child_to_parent[PIPE_READ] = from_child_to_parent[PIPE_READ];
            users[user_idx].is_activated = 1; 
            users[user_idx].rid = -1; //rid -1로 초기화
            
            printf("[INFO] New Client [# %d] is Connected.\n", user_idx);
            dprint("PID : %d, idx : %d", users[user_idx].pid, user_idx);
            
        }
        else {
            perror ("fork error : ");
            close (client_socket);
        }
    }

    close (listen_socket);
    return 0;
}