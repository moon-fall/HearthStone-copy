typedef unsigned char u8;    //定义无符号8个二进制位的变量类型
typedef unsigned long u32;   //定义无符号32个二进制位的变量类型
typedef unsigned short u16;  //定义无符号16个二进制位的变量类型

#define   LOGIN_MSG                        1      //登录消息
#define   FIND_GAME       	               2      //寻找游戏
#define   GAME_ACTION_USE_CARD             4      //游戏动作 使用卡牌
#define   GAME_ACTION_FIELD_CARD_ATTACK    8      //游戏动作 在场仆从攻击
#define   GAME_ACTION_TURN_OVER			   16     //游戏动作 回合结束
#define   QUIT                             32     //退出

struct s_app_hdr
{
	u8 type;
	int len;
};

struct s_login_msg
{
	struct s_app_hdr h;
	u8 username[10];
	u8 password[10];
};

struct s_find_game_msg
{
	struct s_app_hdr h;
};

struct s_quit_msg
{
	struct s_app_hdr h;
};

struct s_game_action_use_card_msg
{
	struct s_app_hdr h;
	int card_pos;
};

struct s_game_action_field_card_attack_msg
{
	struct s_app_hdr h;
	int card_pos;
};


