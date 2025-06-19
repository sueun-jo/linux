#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define UDP_PORT 5100
/*소켓을 이용하여 UDP로 서로 통신하는 코드를 작성해보자.
 먼저 소켓을 이용해 UDP 통신을 위한 서버를 작성한다.*/
int main (int argc, char **argv)
{
	int sockfd, n;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t len;
	char mesg[1000];

	sockfd = socket(AF_INET, SOCK_DGRAM, 0); //UDP를 위한 소켓 생성

	//접속되는 클라이언트를 위한 주소 설정 후 OS에 서비스 등록 
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(UDP_PORT);
	bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	//클라이언트로부터 메시지를 받아서 다시 클라이언트 전송
	do {
		len = sizeof(cliaddr);
		n = recvfrom(sockfd, mesg, 1000, 0,
				(struct sockaddr *)&cliaddr, &len);
		sendto(sockfd, mesg, n, 0,
				(struct sockaddr *)&cliaddr, sizeof(cliaddr));

		mesg[n] = '\0';
		printf("Received data : %s\n", mesg);
	}while (strncmp(mesg, "q", 1));

	close (sockfd); //사용 끝난 후 소켓 닫기
	return 0;
}
