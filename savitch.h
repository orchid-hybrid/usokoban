#ifndef SAVITCH_H
#define SAVITCH_H

#include "sokoban.h"
#define MAX_BOX    10

typedef struct{

	char level[LEVEL_SIZE];
	int w;
	int h;
	int pusher; //pusher position
	


}RNODE;

//
void box_init_config(int *box, int nbox);
int box_next_config(int *box, int nbox, int nsquare);
void box_printf(int *box, int nbox);

//
int level_compare_box(char *lvl1, char *lvl2, int wh, int *box);
void level_find_corner_2(char *lvl, char * g, char *corner, int wh, int w);


//
int savitch(RNODE *r1, RNODE *r2, int n,int k);

//
int sokoban_savitch(SOKOBAN *skb, int n);


#endif

