void init_card_store(struct list_head *p)
{
	struct s_card *p_card;
	int i;
	for(i=0;i<30;i++)
	{
		p_card=(struct s_card*)malloc(sizeof(struct s_card));
		p_card->atk=rand()%10+1;
		p_card->life=rand()%10+1;
		
		list_add_tail(&p_card->list,p);
	}
}
	
void draw_card(struct s_player *player)
{
	struct list_head *pos;
	struct s_card *p_card;
	
	pos=(&player->game_info.running_game->card_store_list)->next;
	p_card=list_entry(pos,struct s_card,list);
	list_del((&player->game_info.running_game->card_store_list)->next);
	list_add_tail(&p_card->list,&player->game_info.running_game->hand_card_list);
}

void show_hand_card(struct s_player *player)
{
	struct s_card *p_card;
	list_for_each_entry(p_card,&player->game_info.running_game->hand_card_list,list)
	{
		printf("hand_card atk:%d life:%d\n",p_card->atk,p_card->life);
		fflush(stdout);
	}
}


void show_field_card(struct s_player *player)
{
	struct s_card *p_card;
	list_for_each_entry(p_card,&player->game_info.running_game->field_card_list,list)
	{
		printf("field_card atk:%d life:%d\n",p_card->atk,p_card->life);
		fflush(stdout);
	}
}

void show_game(struct s_game *game)
{
	printf("player1:%s\n",game->player1->conn_info.username);
	printf("hero life:%d\n",game->player1->game_info.running_game->hero_life);
	show_hand_card(game->player1);
	show_field_card(game->player1);
	
	printf("player2:%s\n",game->player2->conn_info.username);
	printf("hero life:%d\n",game->player2->game_info.running_game->hero_life);
	show_hand_card(game->player2);
	show_field_card(game->player2);
}
void init_game(struct s_game *game)
{
	game->player1->game_info.running_game=(struct s_running_game*)malloc(sizeof(struct s_running_game));
	game->player2->game_info.running_game=(struct s_running_game*)malloc(sizeof(struct s_running_game));
	game->player1->game_info.running_game->hero_life=30;
	game->player2->game_info.running_game->hero_life=30;
	
	INIT_LIST_HEAD(&game->player1->game_info.running_game->card_store_list);
	INIT_LIST_HEAD(&game->player1->game_info.running_game->hand_card_list);
	INIT_LIST_HEAD(&game->player1->game_info.running_game->field_card_list);
	INIT_LIST_HEAD(&game->player2->game_info.running_game->card_store_list);
	INIT_LIST_HEAD(&game->player2->game_info.running_game->hand_card_list);
	INIT_LIST_HEAD(&game->player2->game_info.running_game->field_card_list);

	init_card_store(&game->player1->game_info.running_game->card_store_list);
	init_card_store(&game->player2->game_info.running_game->card_store_list);

	int i;
	for(i=0;i<4;i++)
	{
		draw_card(game->player1);
		draw_card(game->player2);
	}

	show_game(game);
	
}

void destroy_game(struct s_game *game)
{
	printf("destroy game!\n");
	
	struct s_card *p_card;
	struct s_card *n;
	
	list_for_each_entry_safe(p_card,n,&game->player1->game_info.running_game->hand_card_list,list)
		{
		list_del(&p_card->list);
		free(p_card);
		}
		
	list_for_each_entry_safe(p_card,n,&game->player1->game_info.running_game->card_store_list,list)
		{
		list_del(&p_card->list);
		free(p_card);
		}

	list_for_each_entry_safe(p_card,n,&game->player1->game_info.running_game->field_card_list,list)
		{
		list_del(&p_card->list);
		free(p_card);
		}
	
	list_for_each_entry_safe(p_card,n,&game->player2->game_info.running_game->hand_card_list,list)
		{
		list_del(&p_card->list);
		free(p_card);
		}
	
	list_for_each_entry_safe(p_card,n,&game->player2->game_info.running_game->card_store_list,list)
		{
		list_del(&p_card->list);
		free(p_card);
		}

	list_for_each_entry_safe(p_card,n,&game->player2->game_info.running_game->field_card_list,list)
		{
		list_del(&p_card->list);
		free(p_card);
		}

	free(game->player1->game_info.running_game);
	free(game->player2->game_info.running_game);
	
	free(game);

}

void use_card(struct s_player *player,int card_num)
{
	printf("use card! %d",card_num);

	struct list_head *pos;
	struct s_card *p_card;

	pos=&player->game_info.running_game->hand_card_list;
	
	int i;
	for(i=0;i<card_num;i++)
	{
		pos=pos->next;
	}
	p_card=list_entry(pos,struct s_card,list);
	list_del(&p_card->list);
	list_add_tail(&p_card->list,&player->game_info.running_game->field_card_list);
}
	
void attack_card(struct s_player *player1,struct s_player *player2,int attacker_num,int attackeder_num)
{
	struct list_head *pos;
	struct s_card *p_card1;
	struct s_card *p_card2;

	int i;

	pos=&player1->game_info.running_game->field_card_list;
	for(i=0;i<attacker_num;i++)
	{
		pos=pos->next;
	}
	p_card1=list_entry(pos,struct s_card,list);

	pos=&player2->game_info.running_game->field_card_list;
	for(i=0;i<attackeder_num;i++)
	{
		pos=pos->next;
	}
	p_card2=list_entry(pos,struct s_card,list);

	p_card1->life=p_card1->life-p_card2->atk;
	if(p_card1->life<1)
	{
		list_del(&p_card1->list);
		free(p_card1);
	}

	p_card2->life=p_card2->life-p_card1->atk;
	if(p_card1->life<1)
	{
		list_del(&p_card2->list);
		free(p_card2);
	}
}
	
void attack_hero(struct s_player *player1,struct s_player *player2,int attacker_num)
{
	struct list_head *pos;
	struct s_card *p_card;

	int i;

	pos=&player1->game_info.running_game->field_card_list;
	for(i=0;i<attacker_num;i++)
	{
		pos=pos->next;
	}
	p_card=list_entry(pos,struct s_card,list);
		
	player2->game_info.running_game->hero_life=player2->game_info.running_game->hero_life-p_card->atk;

}

void judge_game(struct s_game *game)
{
	if(game->player1->game_info.running_game->hero_life<1)
		{
		printf("player1 win!\n");
		destroy_game(game);
		}
	else if(game->player2->game_info.running_game->hero_life<1)
		{
		printf("player2 win!\n");
		destroy_game(game);
		}
}
		




