#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define TCP_PORT 5100 //서버 포트 번호
#define MAX_EVENT 32

// 파일 디스크립터를 논블로킹 모드로 설정
void setnonblocking(int fd)
{
    int opts = fcntl(fd, F_GETFL);
    opts |= O_NONBLOCK;
    fcntl(fd, F_SETFL, opts);
}

int main(int argc, char **argv)
{
    int ssock, csock;
    socklen_t clen;
    int n, epfd, nfd; // epoll_wait 리턴값 nfd로
    struct sockaddr_in servaddr, cliaddr;
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENT];
    char mesg[BUFSIZ];

    // 서버 소켓 생성
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }

    setnonblocking(ssock); // 논블로킹 모드로 설정

    // 주소 구조체 설정
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TCP_PORT);

    // 소켓 주소 bind
    if (bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind()");
        return -1;
    }

    // 클라이언트 접속 대기
    if (listen(ssock, 8) < 0) {
        perror("listen()");
        return -1;
    }

    // epoll 인스턴스 생성
    epfd = epoll_create(MAX_EVENT);
    if (epfd == -1) {
        perror("epoll_create()");
        return 1;
    }

    // 서버 소켓을 감시 대상으로 등록
    ev.events = EPOLLIN;
    ev.data.fd = ssock;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, ssock, &ev) == -1) {
        perror("epoll_ctl()");
        return 1;
    }

    // 메인 루프
    while (1) {
        nfd = epoll_wait(epfd, events, MAX_EVENT, 500); // 이벤트 발생한 fd 수
        for (int i = 0; i < nfd; i++) {
            if (events[i].data.fd == ssock) {
               
				// 서버 소켓에서 연결 수락
                clen = sizeof(struct sockaddr_in);
                csock = accept(ssock, (struct sockaddr *)&cliaddr, &clen);
                if (csock > 0) {
                    inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
                    printf("Client connected: %s\n", mesg);

                    setnonblocking(csock);

                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = csock;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, csock, &ev);
                }
            } else if (events[i].events & EPOLLIN) {
               
				// 클라이언트가 보낸 데이터 읽기
                memset(mesg, 0, sizeof(mesg));
                n = read(events[i].data.fd, mesg, sizeof(mesg));
                if (n > 0) {
                    printf("Received from Client: %s\n", mesg);
                    write(events[i].data.fd, mesg, n);

                    // "q"가 입력되면 해당 클라이언트 종료
                    if (strncmp(mesg, "q", 1) == 0) {
                        close(events[i].data.fd);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    }
                } else if (n == 0) {
                    // 클라이언트가 연결 종료
                    printf("Client disconnected\n");
                    close(events[i].data.fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                }
            }
        }
    }

    close(ssock);
    return 0;
}

