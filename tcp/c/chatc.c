#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <ctype.h>
#include "chat.h"

#define	MAX_BUF		256

pthread_t recv_t, send_t;

int		Sockfd;

// standard input에서 들어오는 입력값 받아 소켓으로 전송하는 함수 
void Send(int* fd){
		
	int sockfd = *fd;
	char buf[MAX_BUF];		
	int n;
	while(1){
		//Standard Input에서 값이 들어오면
		fgets(buf, MAX_BUF, stdin);
		// 그 값을 소켓에 보낸다. 
		if ((n = send(sockfd, buf, strlen(buf)+1, 0)) < 0){
			perror("send");
			exit(1);	
		
		}
	}
}

// 소켓에서 들어오는 값 받아서 콘솔에 출력하는 함수
void Receive(int* fd){

	int sockfd = *fd;

	int n;
	char buf[MAX_BUF];
	
	while(1){
		//소켓에서 값 들어오면
		if (( n = recv(sockfd, buf, MAX_BUF,0)) < 0){
			perror("recv");
			exit(1);
		}
		//서버가 닫혔을 때 0들어오면 소켓닫고 프로세스 종료한다.
		if ( n == 0){
			fprintf(stderr, "Server terminated.....\n");
			close(sockfd);
			exit(1);
		}
		//채팅이 들어왔을 대는 채팅 출력한다.
		printf("%s", buf);
	}
}

// 채팅 클라이언트 함수
void ChatClient(void){
	char	buf[MAX_BUF];
	int		count, n;
	fd_set	fdset;
	// 채팅을 처음 시작했을 때 아이디를 받는다.
	printf("Enter ID: ");
	fflush(stdout);
	fgets(buf, MAX_BUF, stdin);
	*strchr(buf, '\n') = '\0';
	// 최초 입력한 아이디를 소켓에 전송한다.
	if (send(Sockfd, buf, strlen(buf)+1, 0) < 0)  {
		perror("send");
		exit(1);
	}
	printf("Press ^C to exit\n");

	// recv_t 스레드는 Receive함수 실행 -> 소켓에서 오는 값 받는다.
	// 함수 파라미터로 소켓디스크립터 값 넣어준다.
	pthread_create(&recv_t, NULL,(void *)Receive, (void *)&Sockfd);
	// send_t 스레드는 Send 함수 실행 -> 사용자가 입력하는 값 소켓으로 전송한다.
	pthread_create(&send_t, NULL, (void *)Send, (void *)&Sockfd);

	//두개 스레드가 모두 종료되면 함수 종료
	pthread_join(recv_t, NULL);
	pthread_join(send_t, NULL);

	exit(1);
}	

void CloseClient(int signo){
	//소켓을 닫고 프로세스를 종료한다.
	close(Sockfd);
	printf("\nChat client terminated.....\n");

	exit(0);
}


int main(int argc, char *argv[]){

	struct sockaddr_in	servAddr;
	struct hostent		*hp;
	//서버 주소가 입력되지 않으면 오류 발생
	if (argc != 2)  {
		fprintf(stderr, "Usage: %s ServerIPaddress\n", argv[0]);
		exit(1);
	}

	// TCP, IPv4 사용하는 소켓 오픈
	if ((Sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)  {
		perror("socket");
		exit(1);
	}

	bzero((char *)&servAddr, sizeof(servAddr));
	servAddr.sin_family = PF_INET;
	servAddr.sin_port = htons(SERV_TCP_PORT);
	
	// 숫자값으로 들어왔을 때
	if (isdigit(argv[1][0]))  {
		servAddr.sin_addr.s_addr = inet_addr(argv[1]);
	}
	else  {
		//문자값으로 서버 주소 들어오면 호스트 이름 찾아서 넣는다.
		if ((hp = gethostbyname(argv[1])) == NULL)  {
			fprintf(stderr, "Unknown host: %s\n", argv[1]);
			exit(1);
		}
		memcpy(&servAddr.sin_addr, hp->h_addr, hp->h_length);
	}
	// 서버와 소켓 연결
	if (connect(Sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)  {
		perror("connect");
		exit(1);
	}
	
	// ^C 입력들어오면 서버 닫는 함수 실행
	signal(SIGINT, CloseClient);

	// 채팅 관리 함수 실행
	ChatClient();
}
