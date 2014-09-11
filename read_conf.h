#include<stdlib.h>
#include<stdio.h>
#include<string.h>

struct s_origin_card
{
	int ID;
	char name[10];
	int atk;
	int life;
}

struct s_origin_card *card_array[11];

int read_conf()
{
	printf("loading the config file...\n");

	char *ptr=NULL;
	FILE *fp=NULL;
	int i=0;

	if((fp=fopen("card.conf","r"))==NULL)
		{
		fprintf(stderr,"open error:%s\n",strerror(errno));
		return 1;
		}

	fgets(buffer,BUFSIZE,fp);
	while((fgets(buffer,BUFSIZE,fp))!=NULL)
		{
		ptr=strtok(buffer," ");
		card_array[i]->ID=atoi(ptr);
		ptr=strtok(NULL," ");
		strcpy(card_array[i]->name,ptr)
		ptr=strtok(NULL," ");
		card_array[i]->atk=atoi(ptr);
		ptr=strtok(NULL," ");
		card_array[i]->life=atoi(ptr);
		ptr=strtok(NULL," ");
		card_array[i++]->cost=atoi(ptr);
		}
}


	