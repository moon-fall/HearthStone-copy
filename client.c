#include<stdlib.h> 
#include<stdio.h> 
#include<unistd.h> 
#include<errno.h> 
#include<string.h> 
#include<netdb.h> 
#include<netinet/in.h> 
#include<sys/socket.h> 
#include<arpa/inet.h> 

#include"msg.h"

#define BUFSIZE 512

int sockfd;

static void bail(const char *on_what)
{ 
	fputs(strerror(errno),stderr); 
	fputs(": ",stderr); 
	fputs(on_what,stderr); 
	fputc('\n',stderr); 
	exit(1); 
}

void login()
{

	printf("login..\n");

	struct s_login_msg login_msg;

	login_msg.h.type=LOGIN_MSG;
	login_msg.h.len=sizeof(login_msg)-sizeof(struct s_app_hdr);

	printf("please input username:\n");
	gets((char*)&login_msg.username);
	printf("please input password:\n");
	gets((char*)&login_msg.password);

	puts((char*)&login_msg.username);
	puts((char*)&login_msg.password);


	write(sockfd,(char*)&login_msg,sizeof(login_msg));
}
	
void find_game()
{
	printf("find game...\n");
	struct s_find_game_msg find_game_msg;
	find_game_msg.h.type=FIND_GAME;
	find_game_msg.h.len=0;

	write(sockfd,(char*)&find_game_msg,sizeof(find_game_msg));
}

void use_card()
{
	printf("use card..\n");
	struct s_game_action_use_card_msg game_action_use_card_msg;
	game_action_use_card_msg.h.type=GAME_ACTION_USE_CARD;
	game_action_use_card_msg.h.len=sizeof(game_action_use_card_msg)-sizeof(struct s_app_hdr);
	printf("please input the position of the card you used:");
	scanf("%d",&game_action_use_card_msg.card_pos);
	
	write(sockfd,(char*)&game_action_use_card_msg,sizeof(game_action_use_card_msg));
}

void card_attack()
{
	printf("card attack..\n");
	struct s_game_action_field_card_attack_msg game_action_field_card_attack_msg;
	game_action_field_card_attack_msg.h.type=GAME_ACTION_FIELD_CARD_ATTACK;
	game_action_field_card_attack_msg.h.len=sizeof(game_action_field_card_attack_msg)-sizeof(struct s_app_hdr);
	printf("please input the position of the card attacking:");
	scanf("%d",&game_action_field_card_attack_msg.card_pos);
	
	write(sockfd,(char*)&game_action_field_card_attack_msg,sizeof(game_action_field_card_attack_msg));
}	


void quit()
{
	struct s_quit_msg quit_msg;
	
	quit_msg.h.type=QUIT;
	quit_msg.h.len=0;

	write(sockfd,(char*)&quit_msg,sizeof(quit_msg));
} 

int main(int argc,char **argv) 
{  
	char rcvBuf[BUFSIZE]; 
	char reqBuf[512];

	struct sockaddr_in server_addr;
	struct hostent *host;
	int port; 
	int z; 

	fd_set rfds,orfds;
	int ret,maxfd=-1; 
	if(argc!=3) 
	{ 
		fprintf(stderr,"Usage: %s hostip port\a\n",argv[0]); 
		exit(1); 
	} 

	if((host=gethostbyname(argv[1]))<0) 
	{ 
		fprintf(stderr,"Usage:%s hostip port\a\n",argv[0]); 
		exit(1); 
	}

	if((port=atoi(argv[2]))<0)
	{
		fprintf(stderr,"Usage: %s hostip port\a\n",argv[0]);
		exit(0);
	}
 
	if((sockfd=socket(PF_INET,SOCK_STREAM,0))==-1) 
		bail("socket()"); 
	
	memset(&server_addr,0,sizeof server_addr);
	server_addr.sin_family=PF_INET; 
	server_addr.sin_port=htons(port); 
	server_addr.sin_addr=*((struct in_addr*)host->h_addr); 

	if(connect(sockfd,(struct sockaddr*)(&server_addr),sizeof(server_addr))==-1)
		bail("connect()"); 

	FD_ZERO(&orfds);
	FD_SET(STDIN_FILENO,&orfds);
	maxfd=STDIN_FILENO; 

	FD_SET(sockfd,&orfds);
	if(sockfd>maxfd) 
		maxfd=sockfd; 
	printf(("connected to server: %s \n"),inet_ntoa(server_addr.sin_addr));
	for(;;) 
	{ 
		rfds=orfds;

		printf("\nplease input command:"); 
		fflush(stdout);

		ret=select(maxfd+1,&rfds,NULL,NULL,NULL);
		if(ret==-1)
		{ 
			printf("select: %s",strerror(errno)); 
			break; 
		}		 
		else
		{ 
			if(FD_ISSET(sockfd,&rfds))
			{ 
				if((z=read(sockfd,rcvBuf,sizeof(rcvBuf)))==-1)
					bail("read()"); 

				if(z==0) 
				{ 
					printf("\nserver has close th socket.\n"); 
					printf("press ENTER to exit...\n"); 
					getchar(); 
					break; 
				} 

				printf("\nresult from %s port %u :\n\t'%s'\n",inet_ntoa(server_addr.sin_addr),(unsigned)ntohs(server_addr.sin_port),rcvBuf); 
			} 
			else if(FD_ISSET(STDIN_FILENO,&rfds))
			{ 
				if(!fgets(reqBuf,BUFSIZE,stdin))
				{ 
					printf("\n"); 
					break;
				} 
				z=strlen(reqBuf);

				switch(reqBuf[0])
				{
				case 'l':
					login();
					break;

				case 'f':
					find_game();
					break;
					
				case 'u':
					use_card();
					break;

				case 'a':
					card_attack();
					break;

				case 'q':
					quit();
					break;
				}
			} 
		} 
	} 
	close(sockfd); 
	return 0; 
}
