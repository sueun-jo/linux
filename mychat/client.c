#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include "debug.h"

#define MAX_CLIENT 24
#define MAX_NAME_LEN 64

#define SERVER_PORT 5432
#define BUFSIZE 1024

int my_socket, client_pipe[2];

/* 시그널 핸들러 모음*/
void handle_exit (){ //종료할 때의 시그널 핸들러
    const char *quitmsg = "[Info] Good Bye!";
    write (1, quitmsg, strlen(quitmsg));
    _exit (0);
}

void handle_child (int sig){  //SIGCHLD 시그널 발생 시 시그널 핸들러
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0);
} 

void sig_usr1 (int signo){ //부모 do
    dprint("parent client do : sigusr1\n");
    char buf[BUFSIZE];
    memset (buf, 0, BUFSIZE);
    int n =  read (client_pipe[0], buf, BUFSIZE-1);
    if (n<=0) {
        dprint("read erorr\n");
    }
    else { //읽었으면
        buf[n] = '\0'; //문자열 처리 : 개행 제거
        send(my_socket, buf, strlen(buf), 0); // 읽은 내용을 서버로 보내기
        dprint("send msg to server\n");
    }
}

int main (int argc, char **argv){
    char nickname[MAX_NAME_LEN];
    struct sockaddr_in server_addr;
    char send_buf[BUFSIZE] = {0}; // from child -> to server로 send 할 내용
    char recv_buf[BUFSIZE] = {0}; // form server로부터 recv 내용

    pid_t pid;

    /* ./client.out 127.0.0.1 입력 예외 처리 */
    if (argc != 2){
        printf("[Info] Your Command Input Something Wrong\n");
        return 1;
    }

    /* socket 생성 */
    if ((my_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror ("socket error : ");
        return 1;
    }
    dprint("my socket is established\n");
    
    /* 서버 주소 생성 */
    memset (&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons (SERVER_PORT);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);
    
    /* 서버에 connect 요청 */
    if (connect (my_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror ("connect error : ");
        return 1;
    }
    dprint("connect request\n");
    
    printf("Connected to server. Type /quit to exit.\n"); 

    pipe(client_pipe); //cleint_pipe는 자식->부모로 쓰기만 할거임
        
    pid = fork(); // fork로 부모 - 자식 분기, 자식: input-> pipe로 부모한테 내용 전달 / 부모 : read pipe, interact with server

    /* 부모 프로세스와 자식 프로세스는 따로, 동시에 돌아간다 */
    if (pid < 0) {
        perror ("fork error: ");
    } else if (pid == 0) { // 자식 프로세스 : 사용자 입력 -> 부모 클라로 보냄
        close (client_pipe[0]); // read안할거니까 닫음

        while (1) {        
            printf("Hello, World! Plz input your nickname : "); // 닉네임 입력받기
            fflush(stdout);

            memset (nickname, 0, BUFSIZE); // 닉네임 초기화            
            fgets(nickname, MAX_NAME_LEN, stdin); //닉네임 입력받음
            int x = strlen(nickname);
            nickname[x - 1] = '\0';
            
            if (strchr(nickname, ' ') != NULL ){
                printf("Cannot include ' '(blank) in your nickname\n");
                continue;
            }

            write (client_pipe[1], nickname, strlen(nickname));
            kill (getppid(), SIGUSR1); //부모한테 signal 보냄
            printf("Welcome, %s! You're ready to chat.\n", nickname);
            break; //빠져나옴
        }

        while (1){ 
            memset (send_buf, 0, BUFSIZE); // 초기화
            fgets(send_buf, BUFSIZE, stdin); //send_buf에 입력받음, blocking function
            int len = strlen(send_buf);

            if (send_buf[len-1] =='\n'){
                send_buf[len-1] = '\0';
            }
            write(client_pipe[1], send_buf, strlen(send_buf));
            kill (getppid(), SIGUSR1); //부모한테 signal 보냄
            dprint("send msg to parent(%d) client\n", getppid()); 
        }
    } else { // (pid > 0) 부모 프로세스 : 서버와 전적으로 통신 (send/recv)
        close (client_pipe[1]); // 부모는read만 하니까 write필요 없으니까 닫음
        signal(SIGUSR1, sig_usr1); //SIGUSR1 받으면 sig_usr1() 수행 -> 여기서 send함
        dprint("signal 등록%d\n", getpid());
        
        while (1){
            memset (recv_buf, 0, BUFSIZE);
            int n = recv (my_socket, recv_buf, BUFSIZE-1, 0);

            if (n == 0) {
                printf("[Info] Server Disconnected.\n");
                break;
            }
            if (n < 0) {
                eprint("nothing to read\n");
                break;
            }
            else { // n>0 읽을 게 있으면
            recv_buf[n] = '\0';
            printf("[from server]: %s\n", recv_buf);
            }
        }
        
        kill(pid, SIGTERM); //자식 죽으라고 함
        waitpid(pid, NULL, 0); // 자원 회수 : 좀비 방지
    }

    /* shutdown 종료 선언 : 더 이상 송신하지(write) 않겠다고 알려줌 
    server쪽에서 읽으면 EOF(0리턴)을 받아서 클라이언트가 종료됐다고 알려주는 역할임 */
    shutdown (my_socket, SHUT_WR);
    close (my_socket);

    return 0; 
}