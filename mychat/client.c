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
#include "resource.h"

#define SERVER_PORT 5432
#define BUFSIZE 1024

static int my_socket, client_pipe[2];
char nicknames[MAX_CLIENT][MAX_NAME_LEN] = {0}; //char nicknames[20][50];

/* 시그널 핸들러 모음*/
void handle_exit (int sig){ //종료할 때의 시그널 핸들러
    const char *quitmsg = "[Info] Good Bye!";
    write (1, quitmsg, strlen(quitmsg));
    _exit (0);
}

void handle_child (int sig){  //SIGCHLD 시그널 발생 시 시그널 핸들러
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0);
} 

void sig_usr1 (int sig){ //자식->부모한테 쓰고 알리는 시그널 :  부모는 읽어야 됨
    char buf[BUFSIZE];
    memset (buf, 0, BUFSIZE);
    int n =  read (client_pipe[0], buf, BUFSIZE-1); 
    if (n<=0) {
        dprint("read erorr");
    }
    else { //읽었으면
        buf[n] = '\0'; //문자열 처리 : 개행 제거
        send(my_socket, buf, strlen(buf), 0); // 읽은 내용을 서버로 보내기
        dprint("send by usr1");
    }
}

int assign_user_idx(){
    for (int i=0; i<MAX_CLIENT; i++){
        if (nicknames[i][0]=='\0') return i; // 비어있으면 return i
    }
    return -1; //빈 자리 없으면
}

int main (int argc, char **argv){
    
    struct sockaddr_in server_addr;
    char send_buf[BUFSIZE] = {0}; // from child -> to server로 send 할 내용
    char recv_buf[BUFSIZE] = {0}; // form server로부터 recv 내용

    pid_t pid;

    /* ./client.out 127.0.0.1 입력 예외 처리 */
    if (argc != 2){
        printf("[Info] Your Command Input Something Wrong");
        return 1;
    }

    /* socket 생성 */
    if ( (my_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        perror ("socket error : ");
        return 1;
    }
    dprint("my_socket 생성 완료");
    
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
    dprint("connect request");
    
    printf("Connected to server. Type /quit to exit. \n"); 

    pipe(client_pipe);//cleint_pipe는 자식->부모로 쓰기만 할거임
        
    pid = fork(); // fork로 부모 - 자식 분기, 자식: input-> pipe로 부모한테 내용 전달 / 부모 : read pipe, interact with server

    /* 부모 프로세스와 자식 프로세스는 따로, 동시에 돌아간다 */
    if (pid < 0) perror ("fork error: ");

    else if (pid == 0){  //자식 프로세스 : 사용자 입력 -> 부모 클라로 보냄
        close (client_pipe[0]); //read안할거니까 닫음


        int user_idx = 0;
        while (1) {
            user_idx = assign_user_idx();
            dprint("client idx is %d", user_idx);
            printf("Hello, World! plz input your nickname : "); //닉네임 입력받기
            fflush(stdout);
            
            char nickname[MAX_NAME_LEN];
            memset (nickname, 0, BUFSIZE); //닉네임 초기화
            
            int  n = read (0, nickname, MAX_NAME_LEN-1); //키보드로 닉네임 입력받음 : 49바이트까지 읽겠다
            if (n>1){
                nickname[n-1] = '\0'; //문자열 끝 처리 (개행문자 제거)
                strcpy (nicknames[user_idx], nickname); //복사해서 집어넣음
                dprint("nicknames[user_idx] = %s\n", nicknames[user_idx]);
                printf("H")
                break; //닉네임 입력받고 집어 넣었으면 빠져나감
            }
            else {
                perror ("read error");
                eprint("nothing to read");
            }
        }

        
        while (1){ 
            memset (send_buf, 0, BUFSIZE); // 초기화
            char msg_with_nick[BUFSIZE];
            memset(msg_with_nick, 0, BUFSIZE); //초기화
            fgets(send_buf, BUFSIZE, stdin); //send_buf에 입력받음, blocking function

            snprintf(msg_with_nick, BUFSIZE, "[%s] %s", nicknames[user_idx], send_buf);
            dprint("send msg to parent'%s\n", msg_with_nick); 
            write(client_pipe[1], msg_with_nick, strlen(msg_with_nick));
            kill (getppid(), SIGUSR1); //부모한테 signal 보냄
        }
    }

    else{ // (pid > 0) 부모 프로세스 : 서버와 전적으로 통신 (send/recv)
        close (client_pipe[1]); // 부모는read만 하니까 write필요 없으니까 닫음
        signal(SIGUSR1, sig_usr1); //SIGUSR1 받으면 sig_usr1() 수행 -> 여기서 send함
        
        
        while (1){
            memset (recv_buf, 0, BUFSIZE);
            int n = recv (my_socket, recv_buf, BUFSIZE-1, 0);

            if (n == 0) {
                printf("[Info] Server Disconnected.\n");
                break;
            }
            if (n < 0) {
                eprint("읽을 게 없음");
                break;
            }
            else { // n>0
            recv_buf[n] = '\0';
            printf("[server echo]: %s", recv_buf);
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