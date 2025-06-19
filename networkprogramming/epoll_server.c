#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define TCP_PORT 5100 //서버 포트 번호
#define MAX_EVENT 32 

//파일 디스크립터를 논블로킹 모드로 설정
void setnonblocking(int fd)
{
	int opts = fcntl(fd, F_GETFL);
	opts |= O_NONBLOCK;
	fcntl(fd, F_SETFL, opts);
}


int main (int argc, char **argv)
{
	int ssock, csock;
	socklen_t clen;
	int n, epfd, nfds = 1; //기본 서버 추가
	struct sockaddr_in servaddr, cliaddr; //주소 구조체 정의
	struct epoll_event ev;
	struct epoll_event events[MAX_EVENT]; 
	char mesg[BUFSIZ];


	//서버 소켓디스크립터를 연다
	if ((ssock = socket (AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket()"); 
		return -1;
	}

	setnonblocking(ssock); //서버를 논블로킹 모드로 설정

	//주소 구조체에 주소 지정
	memset(&servaddr, 0, sizeof(servaddr)); //운영체제에 서비스 등록
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(TCP_PORT); //사용할 포트 지정


	//bind 함수를이용하여 서버의 소켓 주소 설정
	if (bind (ssock,(struct sockaddr *)&servaddr, sizeof(servaddr))<0)
	{
		perror("bind()");
		return -1;
	}

	// 동시에 접속하는 클라이언트의 처리를 위한 대기 큐를 설정
	if (listen(ssock, 8) < 0)
	{
		perror("listen()");
		return -1;
	}

	//epoll_create()를 이용해 커널에 등록
	epfd = epoll_create (MAX_EVENT); 
	if (epfd == -1 ){
		perror ("epoll_create()"); 
		return 1;
	}

	//epoll_ctl()을 통해 감시하고 싶은 서버 소켓 등록
	ev.events = EPOLLIN;
	ev.data.fd = ssock;

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, ssock, &ev) == -1)
	{
		perror  ("epoll_ctl()"); 
		return 1;
	}

	do{	
		epoll_wait(epfd, events, MAX_EVENT, 500);
		
		for (int i=0; i<nfds; i++){
			if (events[i].data.fd == ssock){ //읽기 가능한 소켓이 서버 소켓
				clen = sizeof(struct sockaddr_in);

				//클라이언트 요청 받아들이기
				csock = accept(ssock, (struct sockaddr*)&cliaddr, &clen);
				if (csock > 0){

					//네트워크 주소를 문자열로 변경
					inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
					printf("Client is connected : %s\n", mesg);

					setnonblocking(csock); //client를 nonblocking모드로

					//새로 접속한 클라이언트 소켓 번호를 fd_set에 추가
					ev.events = EPOLLIN | EPOLLET;
					ev.data.fd = csock;

					epoll_ctl(epfd, EPOLL_CTL_ADD, csock, &ev);
					nfds++;
					continue;
				}
			} else if (events[i].events & EPOLLIN) { // client 입력
				if (events[i].data.fd < 0) continue; //소켓 아닌 경우 처리

			memset(mesg, 0, sizeof(mesg));
				//해당 client에서 메시지를 읽고 다시 전송 (echo)

				if( (n = read(events[i].data.fd, mesg, sizeof(mesg)))>0)
				{
					printf("Received data : %s", mesg);	
					write (events[i].data.fd, mesg, n);
					close (events[i].data.fd); //클라이언트 소켓을 닫고 지움
					epoll_ctl (epfd, EPOLL_CTL_DEL,	events[i].data.fd, NULL);
					nfds--;
				}
			}
		}
	} while (strncmp(mesg, "q", 1));	
	close (ssock); //서버 소켓 닫음 
	return 0;
}
