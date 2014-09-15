#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<netdb.h>
#include<sys/types.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<time.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<assert.h>
#include<pthread.h>

#include"list.h"
#include"msg.h"
#include"player.h"
#include"game.h"

#define BUFSIZE		512
#define MAXCONN		200
#define	MAX_EVENTS	MAXCONN

typedef struct s_player S_PLAYER;		//为socket对象结构体定义新名字

int handler_msg(struct s_player*);		//处理消息函数
int create_conn(struct s_player*);		//创建连接函数
int init(int);				//初始化函数
void setnonblocking(int);

LIST_HEAD(head);
int epfd;
int num;		//fd_hash中的fd总数
struct epoll_event *events;

//find game and multithread
struct find_game_player
{
	struct s_player *player;
	struct list_head list;
};

void *find_game_run(void*);

pthread_t find_game_tid;
pthread_mutex_t fgqlock=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fgqlock_ready=PTHREAD_COND_INITIALIZER;

struct list_head find_game_hqueue[20];
LIST_HEAD(game_list);
LIST_HEAD(find_game_queue);

int find_game_hlist_num=0;
int find_game_list_num=0;


void *find_game_run(void *arg)
{
	struct list_head *pos;
	struct find_game_player *p_player[2];
	struct s_game *p_game;

	for(;;)
	{
		pthread_mutex_lock(&fgqlock);
		if(find_game_hlist_num<2)
			pthread_cond_wait(&fgqlock_ready,&fgqlock);
		int i,j;

		for(i=0;i<20;i++)	
			{		
			list_splice(&find_game_hqueue[i],&find_game_queue);	
			}
		find_game_list_num=find_game_hlist_num;
		find_game_hlist_num=0;

		pthread_mutex_unlock(&fgqlock);

		while(find_game_list_num>1)
			{
			for(j=0;j<2;j++)
				{
				pos=(&find_game_queue)->next;
				p_player[j]=list_entry(pos,struct find_game_player,list);
				list_del((&find_game_queue)->next);
				find_game_list_num--;
				}
	
			p_game=(struct s_game*)malloc(sizeof(struct s_game));
			p_game->player1=p_player[0]->player;
			p_game->player2=p_player[1]->player;
			list_add_tail(&p_game->list,&game_list);
		
			free(p_player[0]);
			free(p_player[1]);

			p_game->player1->game_info.game=p_game;
			p_game->player2->game_info.game=p_game;
	
			printf("create game: %s and %s at %d\n",p_game->player1->conn_info.username,p_game->player2->conn_info.username,p_game);
			fflush(stdout);
		
			//init game
			init_game(p_game);
			}
		
	}
}

//end



//业务相关
struct s_app_hdr app_hdr_buf;
char buf[100];
char *p_buf=buf;

void do_login(struct s_player *p_player)
{
	printf("player login!\n");
	printf("username: %s \n",((struct s_login_msg*)p_buf)->username);
	printf("password: %s \n",((struct s_login_msg*)p_buf)->password);
	strcpy(p_player->conn_info.username,((struct s_login_msg*)p_buf)->username);
	printf("username: %s \n",p_player->conn_info.username);
	p_player->game_info.state=main_menu;
	p_player->game_info.gold=0;
	p_player->game_info.dust=0;
	p_player->game_info.rank=10;
	
}

void do_begin_find_game(struct s_player *p_player)
{	
	struct find_game_player *p;
	p=(struct find_game_player*)malloc(sizeof(struct find_game_player));	
	p->player=p_player;

	pthread_mutex_lock(&fgqlock);
	list_add_tail(&p->list,&find_game_hqueue[p->player->game_info.rank]);
	find_game_hlist_num++;
	pthread_mutex_unlock(&fgqlock);
	if(find_game_hlist_num>1)
		pthread_cond_signal(&fgqlock_ready);

	printf("username: %s begin find game.. \n",p_player->conn_info.username);

}

void do_game_acton_use_card(struct s_player *p_player)
{
	use_card(p_player,((struct s_game_action_use_card_msg*)p_buf)->card_pos);
	show_game(p_player->game_info.game);
}

void do_game_acton_filed_card_attack(struct s_player *p_player)
{
	attack_hero(p_player,p_player->game_info.game->player2,((struct s_game_action_field_card_attack_msg*)p_buf)->card_pos);
	show_game(p_player->game_info.game);
	judge_game(p_player->game_info.game);
}
void do_turn_over(){}

void do_quit(struct s_player *p_player)
{
	
	//此fd代表的客户端关闭了连接，因此该fd将自动从epfd中删除，于是我们仅需将其从散列表中删除
	
	printf("connection from %s: %d quit",inet_ntoa(p_player->conn_info.address.sin_addr),ntohs(p_player->conn_info.address.sin_port));
	fflush(stdout);
	close(p_player->conn_info.fd);		//关闭此套接字
	list_del(&p_player->list);			//套接字选项链表中删除当前选项p_player
	free(p_player);				//释放指向该套接字对象的指针
}

//业务相关

static void bail(const char *on_what)		//错误报告函数
{
	fputs(strerror(errno),stderr);
	fputs(": ",stderr);
	fputs(on_what,stderr);
	fputs("\n",stderr);
	exit(1);
}

int main(int argc,char* argv[])			//主函数为带命令行参数的函数，用于输入段端口信息
{
	int listen_fd;				//监听套接字
	struct sockaddr_in srvaddr;	//监听套接字地址
	int port;					//服务器监听端口
	int nev;					//epoll_wait返回的文件描述符个数
	struct s_player *n;
	struct s_player *p_player;

	if(argc!=2)				//若命令行参数个数不等于2，输出端口信息错误
	{
		fprintf(stderr,"Usage: %s port\a\n",argv[0]);
		exit(1);
	}

	if((port=atoi(argv[1]))<0)		//若命令行的的二个参数经过字符到整型的转换小于0，输出端口信息错误
	{
		fprintf(stderr,"Usage: %s port\a\n",argv[0]);
		exit(1);
	}

	epfd=epoll_create(MAXCONN);		//创建epoll上下文环境

	if((listen_fd=socket(PF_INET,SOCK_STREAM,0))==-1)//创建监听套接字
	{
		fprintf(stderr,"Socket error: %s\a\n",strerror(errno));
		exit(1);
	}
	
	setnonblocking(listen_fd);		//设置为费阻塞模式

	//设置服务器套接字地址
	memset(&srvaddr,0,sizeof(srvaddr));
	srvaddr.sin_family=PF_INET;
	srvaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	srvaddr.sin_port=htons(port);

	if((bind(listen_fd,(struct sockaddr*)(&srvaddr),sizeof(srvaddr)))==-1)//绑定监听套接字
	{
		fprintf(stderr,"Bind error: %s\a\n",strerror(errno));
		exit(1);
	}
	
	if(listen(listen_fd,5)==-1)	//开始监听
		bail("listen()");

	if(init(listen_fd))		//初始化监听套接字
		bail("init");
	
	events=(struct epoll_event*)malloc(sizeof(struct epoll_event)*MAX_EVENTS);		//这里直接申请最大事件数量乘以epoll_event结构体大小的空间，并将受地址返回个赋值给之前声明的结构体指针
	if(!events)
		bail("malloc");

	//run find game thread
	int i;
	pthread_create(&find_game_tid,NULL,find_game_run,NULL);
	for(i=0;i<20;i++)	
		{	
		INIT_LIST_HEAD(&find_game_hqueue[i]);	
		}


	printf("Server is waiting for acceptance of new client\n");
	for(;;)		//循环接受客户端
	{
		//等待注册事件的发生，
		nev=epoll_wait(epfd,events,MAX_EVENTS,-1);
		//该函数的原型为 int epoll_wait(int epfd,struct epoll_event *event,int maxevents,int timeout)
		//调用epoll_wait()后，进程将等待时间的发生，直到timeout参数设定的超时值到时为止
		//返回值为准备好的文件描述符个数，第一个参数指明在之前创建成功的epfd文件描述符上进行操作，
		//当epoll_wait成功返回后，返回值为发生了所监视事件的文件描述符个数，并且第二个参数，
		//一个指向 struct epoll_event的指针将会指向被epoll上下文返回的事件,
		//本次epool_wait调用最多可已返回的时间个数由第三个参数确定，因此可已通过遍历的方式逐个处理发生的时间的那些文件描述符
		//注：若timeout参数为0，则函数立即返回，及时没有任何事件发生，若timeout参数为-1，则epoll_wait不返回，直到发生事件为止
		
		if(nev<0)			//epoll_wait错误信息
		{
			free(events);
			bail("epoll_wait");
		}
		
		int i;
		for(i=0;i<nev;i++)
		{
			list_for_each_entry_safe(p_player,n,&head,list)
			{
				if(p_player->conn_info.fd==events[i].data.fd)
					p_player->do_task(p_player);
			}
		}
	}

	return 0;
}

int handler_msg(struct s_player *p_player)		//发送回应函数
{
	char reqBuf[BUFSIZE];		//接受缓存
	int z;

   	memset((char*)&app_hdr_buf,0,sizeof(app_hdr_buf));
	memset(buf,0,100);

	read(p_player->conn_info.fd,(char*)&app_hdr_buf,sizeof(app_hdr_buf));
	read(p_player->conn_info.fd,buf+sizeof(app_hdr_buf),app_hdr_buf.len);
	switch(app_hdr_buf.type)
	{
	case LOGIN_MSG:
		do_login(p_player);
		break;

	case FIND_GAME:
		do_begin_find_game(p_player);
		break;

	case GAME_ACTION_USE_CARD:
		do_game_acton_use_card(p_player);
		break;

	case GAME_ACTION_FIELD_CARD_ATTACK:
		do_game_acton_filed_card_attack(p_player);
		break;

	case GAME_ACTION_TURN_OVER:
		do_turn_over();
		break;

	case QUIT:
		do_quit(p_player);
		break;

	default:
		return;
	}
	return 0;
}

int create_conn(struct s_player *p_player)		//创建连接函数，参数是一个指向套接字文件描述符结构体的指针
{
	struct sockaddr_in cliaddr;				//客户端internet地址
	int conn_fd;					//客户端连接套接字
	socklen_t sin_size;				//客户端地址长度
	struct epoll_event ev;
	int ret;

	sin_size=sizeof(struct sockaddr_in);	//长度赋值

	if((conn_fd=accept(p_player->conn_info.fd,(struct sockaddr*)(&cliaddr),&sin_size))==-1 )		//调用accept函数接受连接返回一个新的套接字赋给conn_fd
	{
		fprintf(stderr,"Accept error: %s\a\n",strerror(errno));
		exit(1);
	}

	setnonblocking(conn_fd);
	fprintf(stdout,"Server got connection from %s: %d\n",inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));

	if((p_player=(S_PLAYER*)malloc(sizeof(S_PLAYER)))==NULL)	//创建储存该套接字对象的空间
	{
		perror("malloc");
		return -1;
	}

	p_player->conn_info.fd=conn_fd;	//对该套接字对象的链表节点赋值
	p_player->do_task=handler_msg;	//函数指针赋值
	p_player->conn_info.address=cliaddr;
	p_player->game_info.state=login;

	list_add_tail(&p_player->list,&head);	//加入链表节点到链表
	num++;

	ev.data.fd=conn_fd;		//将来epoll会返回此fd给应用
	ev.events=EPOLLIN;		//监视可读事件
	ret=epoll_ctl(epfd,EPOLL_CTL_ADD,conn_fd,&ev);
	//该函数的原型为 int epoll_ctl(int epfd,int op,int fd,struct epoll_event *event)
	//返回值为0表示成功，第一个参数epfd指明在之前创建成功的epfd文件描述符上进行操作，第二个参数op规定了将对第三个参数文件描述符fd的操作方式，
	//event参数进一步向epoll操作提供必要的数据。
	//这里表示对epfd上下文进行添加文件描述符conn_fd操作，
	if(ret)
	{
		perror("epoll_ctl");
		return -1;
	}
	return 0;
}


int init(int fd)	//参数为套接字
{
	struct s_player *p_player;		//处理每个socket描述符结构体指针
	struct epoll_event ev;			//
	int ret;

	assert(list_empty(&head));
	num=0;				//设定哈希表中元素个数为零
	if((p_player=(S_PLAYER*)malloc(sizeof(S_PLAYER)))==NULL) //创建socket结构体的空间
	{
		perror("malloc");
		return -1;
	}

	p_player->do_task=create_conn;			//设定监听套接字的回调函数为create_conn函数
	p_player->conn_info.fd=fd;					//将套接字参数赋值给结构体中的套接字成员

	list_add_tail(&p_player->list,&head);	//将监听套接字描述符加入到哈希表
	//向epoll上下文注册此fd
	ev.data.fd=fd;
	ev.events=EPOLLIN;

	//添加此fd
	ret=epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&ev);
	if(ret)
		bail("epoll_ctl");
	num++;

	return 0;
}



void setnonblocking(int sock)		//将套接字设置为非阻塞函数
{
	int opts;						

	opts=fcntl(sock,F_GETFL);		//取套接字的原有工作模式
	if(opts<0)
		bail("fcntl");
	opts=opts|O_NONBLOCK;			//原有工作模式与非阻塞模式进行或运算，在原有工作模式的基础上加上非阻塞工作模式
	if(fcntl(sock,F_SETFL,opts))	//设置新的套接字工作模式
		bail("fcntl");
}
