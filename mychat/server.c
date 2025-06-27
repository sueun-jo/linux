#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <signal.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server.h"
#include "debug.h"

#define SERVER_PORT 5432
#define BUFSIZE 1024

int from_child_to_parent[2];
int from_parent_to_child[2];
int client_socket;

/* zombie 방지 시그널 핸들러 */
void sig_child (){
    int status;
    pid_t pid;
    while ( (pid = waitpid(-1, &status, WNOHANG)) > 0){ //자원 회수 : pid에 종료된 자식 pid 담김
        int idx = find_user_idx_by_pid(pid);

        if (idx>=0) { //idx번째 구조체 값 초기화로 release 
            memset(&users[idx], 0, sizeof(ClientInfo));
            dprint("%d번째 cient 해제\n", idx);
        } else {eprint("no child pid %d\n", pid);}
    }
}

// void sig_usr1() { //부모 server do
//     dprint("parent server do sig_usr1\n");
//     char buf[BUFSIZE];
//     memset(buf, 0, BUFSIZE);

//     // 먼저 어떤 자식이 메시지를 보냈는지 찾아야 함
//     int sender_idx = -1;

//     for (int i = 0; i < MAX_CLIENT; i++) {
//         if (users[i].is_activated) {
//             int n = read(users[i].from_child_to_parent[PIPE_READ], buf, BUFSIZE-1);
//             if (n > 0) { //읽을 게 있으면
//                 buf[n] = '\0';
//                 sender_idx = i;
//                 dprint("recv msg from child[%d] : %s\n", i, buf);
//                 break; // 메시지 하나만 받고, broadcast는 나중에 따로
//             }
//         }
//     }

//     // broadcast to everyone except sender
//     if (sender_idx != -1) {
//         for (int i = 0; i < MAX_CLIENT; i++) {
//             if (users[i].is_activated && i != sender_idx) { //활성화가 돼 있고 sender_idx가 아니
//                 write(users[i].from_parent_to_child[PIPE_WRITE], buf, strlen(buf));
//                 kill(users[i].pid, SIGUSR2);
//                 dprint("broadcast to user[%d]\n", i);
//             }
//         }
//     }
// }

void sig_usr1(int signo, siginfo_t *info, void *context) {
    dprint("parent server do sig_usr1\n");

    pid_t sender_pid = info->si_pid;
    int sender_idx = find_user_idx_by_pid(sender_pid);
    if (sender_idx < 0) {
        eprint("cannot find sender\n");
        return;
    }

    char buf[BUFSIZE];
    memset(buf, 0, BUFSIZE);
    int n = read(users[sender_idx].from_child_to_parent[PIPE_READ], buf, BUFSIZE-1);
    if (n > 0) {
        buf[n] = '\0';

        // broadcast: 나 빼고 모두에게
        for (int i = 0; i < MAX_CLIENT; i++) {
            if (users[i].is_activated && i != sender_idx) {
                write(users[i].from_parent_to_child[PIPE_WRITE], buf, strlen(buf));
                kill(users[i].pid, SIGUSR2);
            }
        }
    } else {
        eprint("no message from pipe\n");
    }
}

void sig_usr2(){ // 자식 server do
    dprint("client server do sig_usr2\n");
    char buf[BUFSIZE];
    memset(buf, 0, BUFSIZE);
    dprint("now user_idx %d\n", user_idx);
    int n = read (from_parent_to_child[PIPE_READ], buf, BUFSIZE-1);
    if (n<=0){
        eprint("read erorr\n");
    } else {
        int n = send (client_socket, buf, strlen(buf), 0);
        if (n<0){
            perror("send error");
        } else {
            dprint("send to client : success\n");
        }
    }
}

int main (int argc, char **argv){
    int listen_socket; //listen_socket은 연결대기용, client_socket(전역)은 client와 통신용
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
        
        user_idx = find_empty_user_slot();

        if ( user_idx == -1 ) {
            printf("[Info] Connection Rejected : Too many clients\n");
            close (client_socket);
            continue;
        }
        
        //pipe 생성
        pipe (from_child_to_parent); // 자식 -> 부모 : 자식 write, 부모 read
        pipe (from_parent_to_child); // 부모 -> 자식 : 부모 write, 자식 read

        pid = fork(); // 부모 - 자식 나눠짐

        if (pid == 0){ //자식 프로세스
            signal (SIGUSR2, sig_usr2); // sig_usr2는 자식do
            close (listen_socket); //자식은 listen_socket 필요 없음 : accept은 부모만 한다
    
            //자식은 부모한테 write만 하면 됨, read 필요 없음
            close (from_child_to_parent[PIPE_READ]); // 자식은 부모한테 write only, read 필요X
            close (from_parent_to_child[PIPE_WRITE]); // 부모한테 read only, write 필요X

            while(1) {
                memset(buf, 0, BUFSIZE);
                int n = recv (client_socket, buf, BUFSIZE-1, 0);
                if (n<=0) {
                    eprint("recv error : nothing to read\n");
                    break;
                } else {
                    buf[n] = '\0'; //문자열 처리
                    dprint("recv msg from client\nmsg : %s\n", buf);
                    write (from_child_to_parent[PIPE_WRITE], buf, strlen(buf));
                    dprint("send msg from child server to parent server\n");
                    kill (getppid(), SIGUSR1); //쓰고 나서 부모한테 SIGUSR1 전달 
                }
            }
            exit(0);
        } else if (pid > 0){ // 부모 프로세스

            // sigaction 등록
            struct sigaction sa;
            sa.sa_sigaction = sig_usr1;
            sa.sa_flags = SA_SIGINFO;
            sigemptyset(&sa.sa_mask);
            sigaction(SIGUSR1, &sa, NULL);

            signal (SIGCHLD, sig_child); 
            
            close (from_parent_to_child[PIPE_READ]); //부모는 자식한테 write, read필요 없음
            close (from_child_to_parent[PIPE_WRITE]); //부모는 자식으로부터 read only, write 필요 X
            
            /* UserInfo 구조체 users setter */
            users[user_idx].pid = pid; //자식pid를 부모가 갖고 있다
            dprint("users[%d] = %d\n", user_idx, pid);
            users[user_idx].client_socket_fd = client_socket;
            users[user_idx].from_parent_to_child[PIPE_WRITE] = from_parent_to_child[PIPE_WRITE];
            users[user_idx].from_child_to_parent[PIPE_READ] = from_child_to_parent[PIPE_READ];
            users[user_idx].is_activated = 1; 
            users[user_idx].room_number = -1; //rid -1로 초기화
            
            printf("[INFO] New Client [# %d] is Connected.\n", user_idx);
            dprint("PID : %d, idx : %d\n", users[user_idx].pid, user_idx);
            
        } else {
            perror ("fork error : ");
            close (client_socket);
        }
    }

    close (listen_socket);
    return 0;
}