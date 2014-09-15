enum e_player_state 
{
	login,
	main_menu,
	find_game,
	gaming,
	game_over
};

struct s_game;

struct s_conn_info
{
	int fd;
	struct sockaddr_in address;
	char username[10];
};

struct s_running_game
{
	struct list_head hand_card_list;
	struct list_head card_store_list;
	struct list_head field_card_list;

	int hero_life;
};

struct s_game_info
{
	enum e_player_state state;
	int rank;
	int gold;
	int dust;
	struct s_game *game;
	struct s_running_game *running_game;
};

struct s_player		//socket对象结构体
{
	struct s_conn_info conn_info;
	struct s_game_info game_info;
	int (*do_task)(struct s_player *p_player);//回调函数指针
	struct list_head list;		//内嵌链表节点
};

struct s_game
{
	struct s_player *player1;
	struct s_player *player2;
	struct list_head list;
};

struct s_card
{
	int atk;
	int life;
	struct list_head list;
};





