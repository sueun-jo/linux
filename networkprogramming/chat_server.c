#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define TCP_PORT 5100 //서버 포트 번호

int main (int argc, char **argv)
{
	int ssock; //socket discriptor를 담을 ssock
	socklen_t clen;
	int n;
	struct sockaddr_in servaddr, cliaddr; //주소 구조체 정의
	char mesg[BUFSIZ];
	
	//fd_set은 file discriptor들(집합) 저장하는 자료형
	fd_set readfd; //읽기 이벤트를 감시할 디스크립터들을 담는 변수
	int maxfd, client_index, start_index; 
	int client_fd[5] = {0};

	//서버 소켓 생성
	if ((ssock = socket (AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket()"); 
		return -1;
	}

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

	FD_ZERO (&readfd); //fd_set형 readfd를 모두 0으로 초기화
	maxfd = ssock; //일단  최대의 파일디스크립터 번호는 ssock으로 설정


	client_index =0;
	do{	
		FD_SET(ssock, &readfd); 
		//readfd라는 감시 목록에 ssock(서버소켓)  추가하겠다

	for (start_index = 0; start_index < client_index; start_index++) {
		FD_SET(client_fd[start_index], &readfd);
		//각 클라이언트 소켓도 readfd에 추가하겠다

		if (client_fd[start_index] > maxfd)
			maxfd = client_fd[start_index]; //가장 큰 소켓 번호 저장 
	}
	
	
	//select 함수로 
	select (maxfd+1, &readfd, NULL, NULL, NULL); 
	if (FD_ISSET(ssock, &readfd)) { //읽기 가능한 소켓이 서버 소켓인 경우
		clen = sizeof(struct sockaddr_in);
		
		//클라이언트 요청 받아들이기 accept()
		int csock = accept(ssock, (struct sockaddr*)&cliaddr, &clen);
		
		if (csock < 0){
			perror("accept()");
			return -1;
		}

		else {
			//네트워크를 문자열로 변경
			inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
			printf("Client is connected : %s\n", mesg);

			//새로 접속한 클라이언트의 소켓 변화를 fd_set에 추가
			FD_SET(csock, &readfd);
			client_fd[client_index] = csock;
			client_index++;
			continue;
		}

		if (client_index == 5) break;
	}

	//읽기 가능했던 소켓이 클라이언트인 경우 
	for (start_index = 0; start_index < client_index; start_index++)
	{
		//for문으로 클라이언트들 모두 조사
		if (FD_ISSET(client_fd[start_index], &readfd)){
			memset(mesg, 0, sizeof(mesg));

			//해당 클라이언트에서 메세지를 읽고 다시 전송 (echo)
			if ((n = read(client_fd[start_index], mesg, sizeof(mesg)))>0)
			{
				printf("Recieved data : %s", mesg);
				write (client_fd[start_index], mesg, n);
				close (client_fd[start_index]); //클라이언트 소켓을 닫는다

				//클라이언트 소켓을 지운다
				FD_CLR (client_fd[start_index], &readfd);
				client_index--;
			}
		}
	}
} while ( strncmp(mesg, "q", 1));

	close (ssock); //서버 소켓 닫음 
	return 0;
}
