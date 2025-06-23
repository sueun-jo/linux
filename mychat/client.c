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

#define SERVER_PORT 5432
#define BUFSIZE 1024

/* 시그널 핸들러 모음*/
void handle_exit (int sig){ //종료할 때의 시그널 핸들러
    const char *quitmsg = "\n[Info] Good Bye!\n";
    write (1, quitmsg, strlen(quitmsg));
    _exit (0);
}

void handle_child (int sig){  //SIGCHLD 시그널 발생 시 시그널 핸들러
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0);
}

int main (int argc, char **argv){
    
    int my_socket; //socket
    struct sockaddr_in server_addr;
    char send_buf[BUFSIZE] = {0}; // 부모에서 server로 send 할 내용
    char recv_buf[BUFSIZE] = {0}; // 자식이 server로부터 recv 내용
    pid_t pid;

    /* 시그널 등록 */
    signal (SIGINT, handle_exit);
    signal (SIGCHLD, handle_child);
    
    if (argc != 2){
        printf("[Info] Your Command Input Something Wrong");
        return 1;
    }

    /* socket 생성 */
    if ( (my_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        perror ("socket error : ");
        return 1;
    }

    /* 서버 주소 생성 */
    memset (&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons (SERVER_PORT);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    /* 서버에 connect 요청 */
    if ( connect (my_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror ("connect error : ");
        return 1;
    }
    
    printf("Connected to server. Type /quit to exit. \n"); 

    /* fork로 입력 출력 분리 : 부모-입력 / 자식-받아와서 출력 */
    pid = fork();

    /* 부모 프로세스와 자식 프로세스는 따로, 동시에 돌아간다 */

    if (pid > 0){  //부모 프로세스 : 사용자 입력 -> 서버 전송
        while (1){
            fgets(send_buf, BUFSIZE, stdin);

            if (strncmp(send_buf, "/quit", 5) == 0) { // 사용자가 /quit 입력 시
                send(my_socket, send_buf, strlen(send_buf), 0); // 서버에도 알려주고
                kill (pid, SIGTERM); // 자식한테 죽으라고 함
                waitpid (pid, NULL, 0); //죽은 자식 자원 회수 : 좀비 방지
                break;
            }

            if ((send (my_socket, send_buf, strlen(send_buf), 0)) <= 0){ //socket 통신의 send : write 대신
                perror ("send error :"); 
                return -1;
            }
        }
    }

    else if (pid == 0){ //자식 프로세스 : 서버로부터 recv() -> 화면 출력
        while (1){

            int len = recv (my_socket, recv_buf, BUFSIZE-1, 0);
            if (len <= 0) break;
            recv_buf[len] = '\0';
            // 읽은 내용 출력 예정 printf(); 추후 수정
            printf("[server]: %s", recv_buf);

        }
        
        exit(0);
    }

    /* shutdown 종료 선언 : 더 이상 송신하지(write) 않겠다고 알려줌 
    server쪽에서 읽으면 EOF(0리턴)을 받아서 클라이언트가 종료됐다고 알려주는 역할임 */
    shutdown (my_socket, SHUT_WR);
    close (my_socket);

    return 0; 
}