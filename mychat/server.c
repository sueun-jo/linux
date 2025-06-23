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
#include "cm.h" //to manager client

#define SERVER_PORT 5432
#define BUFSIZE 1024

/* 시그널 핸들러 */
void handle_child (int sig){
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0);
}

int main (int argc, char **argv){

    int listen_socket, client_socket; //listen_socket은 연결대기용, client_socket은 client와 통신용
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);
    char buf [BUFSIZE];
    pid_t pid;
    

    signal (SIGCHLD, handle_child);

    listen_socket = socket (AF_INET, SOCK_STREAM, 0); //tcp socket 통신 하겠습니다
    if (listen_socket < 0){
        perror ("socket error\n");
        return 1;
    }

    memset (&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /* bind로 주소 설정 */
    if (bind (listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error\n");
        return 1;
    }

    /* 동시 접속 클라이언트 처리 대기 큐 listen */
    listen (listen_socket, 10);
    printf("Server is Listening on Port : %d . . . \n", SERVER_PORT);
    
    while (1) {
    
        /* accept : 클라이언트가 요청해서 생긴 socket이기에 client_socket이라 명명 */
        /* accept하면 서버:클라이언트 1:1 관계를 만들어야 하니까 fork() 해야됨 */
        client_socket = accept (listen_socket, (struct sockaddr*)&client_addr, &addrlen);
        if (client_socket < 0){                   
            perror ("accept error : ");
            continue;
        }

        printf ("[Info] Client connected.\n");

        pid = fork();

        // //pipe 생성 드가자
        // int pipeToParent[2];
        // int pipeToChild[2];
        // pipe (pipeToParent); // 자식 -> 부모 : 자식 write, 부모 read
        // pipe (pipeToChild); // 부모 -> 자식 : 부모 write, 자식 read

        

        if (pid == 0){ //자식 프로세스
            close (listen_socket); //자식은 listen_socket 필요 없음 : accept은 부모만 한다

            int my_idx = -1;
            for (int i=0; i<client_cnt; i++){
                if (clients[i].pid == getpid());
                my_idx = i;
                break;
            }

            if (my_idx == -1){
                fprintf();
            }
            //자식은 부모한테 write만 하면 됨, read 필요 없음
            close (pipeToParent[0]);
            //자식은 부모로부터 read만 하면 됨, write 필요 없음
            close (pipeToChild[1]);
            

            while (1){
                int len = recv(client_socket, buf, BUFSIZE, 0); //recv from client
                if (len <= 0) break; 
                printf("debug / [CHILD %d] recv: %s", getpid(), buf);

                write (pipeToParent[1], buf, len); //부모 server에게 전달
                printf("debug / [CHILD %d] wrote to parent\n", getpid());

                len = read (pipeToChild[0], buf, BUFSIZE); //부모가 write한 걸 읽어야함
                if (len <= 0) {
                    printf("debug / [CHILD %d] read from parent failed\n", getpid());
                    break;
                }
                printf("debug / [CHILD %d] read from parent: %s", getpid(), buf);
                send (client_socket, buf, len, 0); //클라이언트 소켓에 보내기
            }
            
            close (client_socket);
            exit(0);
        }
        else { // 부모 프로세스

            //부모는 자식한테 write만 하면 됨, read 필요 없음
            close (pipeToChild[0]);
            //부모는 자식으로부터 read만 하면 됨, write 필요 없음
            close (pipeToParent[1]); 

            while(1) {

                int len = read(pipeToParent[0], buf, BUFSIZE); //자식이 쓴 거 읽어와야함
                if (len <= 0) {
                    printf("[PARENT] read from child failed\n");
                    break;
                }
                printf("[PARENT] got msg: %s", buf);

                //받은 메세지를 그대로 다시 자식에게 전달 (echo)
                write (pipeToChild[1], buf, len);
                printf("[PARENT] wrote msg back to child\n");
            }

            // close(client_socket);
        }
    }
    close (listen_socket);
    return 0;
}