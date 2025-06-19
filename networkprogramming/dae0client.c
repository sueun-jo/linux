#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <stdlib.h>


#define TCP_PORT 5100


int main(int argc, char **argv){
    int sockfd , n;
    socklen_t clisize;
    struct sockaddr_in servaddr, cliaddr;
    char mesg[BUFSIZ];
    char *address = "192.168.2.95";//argv[1]
    int port;
    if(argc == 2){
        port = atoi(argv[1]);
    }else if(argc == 1){
        port = 5101;
    }else if(argc == 3){
        address = argv[1];
        port = atoi(argv[2]);
    }else{
        printf("usage : %s <Port>\n", argv[0]);
        printf("usage : %s <IP address> <Port>\n", argv[0]);
        return -1;
    }

    // 소켓 생성
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){
        perror("socket()");
        return -1;
    }
    
    fd_set readfd;
    FD_ZERO(&readfd);
    // 소켓이 접속할 주소 지정
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;

    inet_pton(AF_INET, address, &(servaddr.sin_addr.s_addr));
    servaddr.sin_port = htons(port);
    printf("Connect Net = %s:%d\n",address,port);

    //지정한 주소로 접속
    if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))<0){
        perror("connect()");
        return -1;
    }
    do {
        FD_SET(sockfd, &readfd);
        FD_SET(0, &readfd);
        select(sockfd+1, &readfd, NULL, NULL, NULL);
        if(FD_ISSET(sockfd, &readfd)){
            memset(mesg, 0 ,BUFSIZ);
            if(recv(sockfd, mesg, BUFSIZ, 0) <=0){
                perror("recv()");
                return -1;
            }
            printf("%s\n", mesg);
            // fflush(stdout);
        }else if(FD_ISSET(0, &readfd)){
            fgets(mesg, BUFSIZ, stdin);
            int size = 0;
            for(int i=0; i<BUFSIZ ; i++){
                if(mesg[i]=='\n'){
                    size = i;
                    break;
                }
            }
            if(size==0){
                continue;
            }
            // printf("size = %d\n",size);
            if(send(sockfd, mesg, size, MSG_DONTWAIT)<=0){
                perror("send()");
                return -1;
            }
        }
        
    }while(strncmp(mesg, "@", 1));
    close(sockfd);
    return 0;
}
