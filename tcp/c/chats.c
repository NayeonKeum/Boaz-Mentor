#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "chat.h"

#define	DEBUG

#define	MAX_CLIENT	5
#define	MAX_ID		32
#define	MAX_BUF		256

//여러개의 클라이언트와 연결했을 때 각각 클라이언트의 주소를 저장할 구조
typedef	struct  {
	int			sockfd;
	int			inUse;
	pthread_t	tid;
	char		uid[MAX_ID];
}
	ClientType;

int				Sockfd;
ClientType		Client[MAX_CLIENT];

//5개의 소켓을 연결 -> 비어 있는 것을 찾아서 리턴해주는 함수
int GetID(){
	int	i;

	for (i = 0 ; i < MAX_CLIENT ; i++)  {
		if (! Client[i].inUse)  {
			Client[i].inUse = 1;
			return i;
		}
	}
}

//채팅이 왔을 때 같이 채팅하고 있는 다른 참여자들에게 동일한 메시지 전송해주는 함수
void SendToOtherClients(int id, char *buf){
	int		i;
	char	msg[MAX_BUF+MAX_ID];
	// "참여자> 채팅메시지" 형태로 전송
	sprintf(msg, "%s> %s", Client[id].uid, buf);
#ifdef	DEBUG
	printf("%s", msg);
	fflush(stdout);
#endif

	for (i = 0 ; i < MAX_CLIENT ; i++)  {
		if (Client[i].inUse && (i != id))  {
			if (send(Client[i].sockfd, msg, strlen(msg)+1, 0) < 0)  {
				perror("send");
				exit(1);
			}
		}
	}
}
//SIGINT 핸들링하는 함수. 
void CloseServer(int signo){
	int		i;
	
	//서버 소켓 디스크립터 삭제 
	close(Sockfd);
	// 모든 클라이언트와 연결되어있는 소켓 닫고 함수 종료한다.
	for (i = 0 ; i < MAX_CLIENT ; i++)  {
		if (Client[i].inUse)  {
			close(Client[i].sockfd);
		}
	}

	printf("\nChat server terminated.....\n");

	exit(0);
}
	
int main(int argc, char *argv[]){

	int					newSockfd, cliAddrLen, id, one = 1;
	struct sockaddr_in	cliAddr, servAddr;
	fd_set fdset;
	
	// ^C들어오면 서버 종료하는 함수로 연결
	signal(SIGINT, CloseServer);
	
	// TCP, IPv4사용하는 소켓 오픈
	if ((Sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)  {
		perror("socket");
		exit(1);
	}
	//Sockfd에 대해서 SOL_SOCKET Level,이미 사용된 주소를 재사용하도록 한다. 
	if (setsockopt(Sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0)  {
		perror("setsockopt");
		exit(1);
	}
	//서버 주소 설정 -> 현재 랜카드에서 쓸 수 있는거 아무거나 
	bzero((char *)&servAddr, sizeof(servAddr));
	servAddr.sin_family = PF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(SERV_TCP_PORT);

	//소켓 주소 설정해서 바인딩한다.
	if (bind(Sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)  {
		perror("bind");
		exit(1);
	}

	listen(Sockfd, 5);

	printf("Chat server started.....\n");

	cliAddrLen = sizeof(cliAddr);
	int i, n, count;
	char buf[MAX_BUF];
	while(1){
		//while문 돌 때마다 fdset에 서버 소켓과 클라이언트에 연결되어있는 소켓 디스크립터의 값을 1로 변경
		FD_ZERO(&fdset);
		FD_SET(Sockfd, &fdset);
		for ( i = 0 ; i < MAX_CLIENT;i++){
			if (Client[i].inUse)
				FD_SET(Client[i].sockfd, &fdset);
		}
		// 변경된 소켓이 있는지 확인한다.
		if ((count = select(10, &fdset, (fd_set *)NULL, (fd_set *)NULL,
			(struct timeval *)NULL)) < 0)  {
			perror("select");
			exit(1);
		}	
		//변경된 소켓의 개수만큼 while문 돈다.
		while(count--){	
			//만약 서버의 소켓 변경 -> 새로운 소켓 연결 요청 들어왔다는 것
			if (FD_ISSET(Sockfd, &fdset)){
				//새로운 연결을 accept한다.
				newSockfd = accept(Sockfd, (struct sockaddr *)&cliAddr, &cliAddrLen);
				if (newSockfd < 0){
					perror("accpet");
					exit(1);
				}
				//id값을 가져와서 소켓디스크립터를 저장해준다.
				id = GetID();
				Client[id].sockfd = newSockfd;
				//처음에 연결되었을 유저 아이디를 받아온다.
				if ((n = recv(Client[id].sockfd, Client[id].uid, MAX_ID, 0)) < 0)  {
						perror("recv");
						exit(1);
					}
				printf("Client %d log-in(ID: %s).....\n", id, Client[id].uid);
				
			}	
			//변경된 소켓이 서버 소켓이 아니었을 때 -> 클라이언트와 연결된 소켓 중 하나다.
			// == 채팅이 들어왔다는 뜻이므로 온 소켓에서 메시지 받아 다른 소켓들에게 전송
			for ( i = 0 ; i<MAX_CLIENT;i++){
				if (FD_ISSET(Client[i].sockfd, &fdset)){
					//클라이언트 소켓에서 채팅 내용을 읽어 buf에 넣는다.
					if ((n = recv(Client[i].sockfd, buf, MAX_BUF, 0)) <0){
						perror("recv");
						exit(1);
					}
					// 제대로 복사 되었으면 다른 클라이언트들에게도 전송한다.
					if ( n > 0){
						SendToOtherClients(i, buf);
					}
					else if ( n == 0){
						//소켓이 닫혔을 때 메시지 출력하고 소켓을 닫는다.
						printf("Client %d log-out(ID: %s).....\n", id, Client[i].uid);
						close(Client[i].sockfd);
						Client[i].inUse = 0;

						strcpy(buf, "log-out.....\n");
			
						SendToOtherClients(i, buf);
					}
				}
			}
		}
	}
}
