#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define TCP_PORT 5100
#define MAX_EVENT 32
#define MAX_CLIENT 5 // 최대 클라이언트 수 제한

int client_fds[MAX_CLIENT]; // 클라이언트 소켓들을 저장하는 배열
int client_count = 0;       // 현재 접속한 클라이언트 수

// 소켓을 논블로킹 모드로 설정하는 함수
void setnonblocking(int fd) {
    int opts = fcntl(fd, F_GETFL);     // 기존 플래그 읽기
    opts |= O_NONBLOCK;               // 논블로킹 플래그 추가
    fcntl(fd, F_SETFL, opts);         // 새로운 플래그 설정
}

int main() {
    int ssock, csock;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clen;
    int n, epfd, nfd;
    struct epoll_event ev, events[MAX_EVENT];
    char mesg[BUFSIZ];

    // 서버 소켓 생성
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }

    // 서버 소켓을 논블로킹으로 설정
    setnonblocking(ssock);

    // 서버 주소 구조체 초기화 및 설정
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TCP_PORT);

    // 서버 소켓에 주소 바인딩
    if (bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind()");
        return -1;
    }

    // 클라이언트 접속 대기 큐 생성
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

    // 서버 소켓을 epoll에 등록 (읽기 감지)
    ev.events = EPOLLIN;
    ev.data.fd = ssock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, ssock, &ev);

    // 메인 이벤트 루프 시작
    while (1) {
        // epoll 이벤트 대기 (최대 MAX_EVENT개, 500ms 타임아웃)
        nfd = epoll_wait(epfd, events, MAX_EVENT, 500);

        for (int i = 0; i < nfd; i++) {
            int fd = events[i].data.fd; // 이벤트 발생한 파일 디스크립터

            // 새로운 클라이언트 연결 처리
            if (fd == ssock) {
                clen = sizeof(struct sockaddr_in);
                csock = accept(ssock, (struct sockaddr *)&cliaddr, &clen);
                if (csock <= 0) continue;

                // 클라이언트 수가 초과되면 거절
                if (client_count >= MAX_CLIENT) {
                    printf("Maximum client limit reached. Rejecting new connection.\n");
                    close(csock);
                    continue;
                }

                // 클라이언트 소켓 논블로킹 설정 및 epoll 등록
                setnonblocking(csock);
                ev.events = EPOLLIN | EPOLLET; // Edge Triggered 모드
                ev.data.fd = csock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, csock, &ev);

                // 클라이언트 배열에 저장
                client_fds[client_count++] = csock;
                printf("Client connected: FD %d\n", csock);
                continue;
            }

            // 클라이언트로부터 메시지 수신 처리
            if (events[i].events & EPOLLIN) {
                memset(mesg, 0, BUFSIZ); // 메시지 버퍼 초기화
                n = read(fd, mesg, BUFSIZ); // 메시지 읽기

                if (n > 0) {
                    // 메시지를 받은 경우: 다른 클라이언트에게 브로드캐스트
                    printf("[FD %d] %s\n", fd, mesg);
                    for (int j = 0; j < client_count; j++) {
                        if (client_fds[j] != fd) {
                            send(client_fds[j], mesg, n, 0);
                        }
                    }
                } else if (n == 0) {
                    // 클라이언트 연결 종료 감지 (EOF)
                    printf("Client disconnected: FD %d\n", fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    // 배열에서 해당 소켓 제거
                    for (int j = 0; j < client_count; j++) {
                        if (client_fds[j] == fd) {
                            client_fds[j] = client_fds[--client_count];
                            break;
                        }
                    }
                }
            }
        }
    }

    // 서버 소켓 종료
    close(ssock);
    return 0;
}

