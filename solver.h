#ifndef SOLVER_H
#define SOLVER_H

#include <stdlib.h>


#define MAX_DIRECTION    (4)


extern int debug;

typedef unsigned long long ZOBRIST_KEY;

typedef struct{
	unsigned long long bitstring;
	int	pusher;
	int	g; //number of push so far
	int	h; //the heuristic function
	int	z; //zobrist value
	struct SNODE *qnext;
	struct SNODE *znext;
	struct SNODE *parent;


}SNODE;

int zobrist_init(ZOBRIST_KEY *zob,int boardsize);
void heuristic_init(int *heu, char *lvl, char *goal, int wh, int w);

////functions for conversion 
unsigned long long level_2_bitstring(char *level, int w, int h);
ZOBRIST_KEY	   level_2_zobrist(char *level, int wh, ZOBRIST_KEY *zob);
int                level_2_pusher(char *level, int w, int h);
ZOBRIST_KEY        bitstring_2_zobrist(unsigned long long bitstring, ZOBRIST_KEY *zobrist);
void               bitstring_2_level(unsigned long long bitstring, char *flvl, int wh, char *tlvl);
void		i2bi_init(char *lvl, int w, int h, int *i2bi);

//basic search functions
void 		level_find_reachable_area(char *level, int w, int h, int pusher);
void		level_do_push(char *wlvl, char *nlvl, int wh, int w, int box, int direction);
int 		level_compare(char *lvl,   int w, int pa, int pb, int *t, unsigned int *timestamp);
int		level_check(char *lvl, char *g, int wh);
char		level_find_push_direction(char *nlvl, char *olvl, int wh, int w,int pusher);
void 		level_find_corner(char *lvl, char * g, char *corner, int wh, int w);
int		level_find_heuristic(char *lvl, int *heu, int wh);
int		level_check_freeze_deadlock(char *lvl, char *corner,   int w, int box);

////
unsigned long long sokoban_find_zobrist_key(SOKOBAN *skb, ZOBRIST_KEY *zob, int boardsize);
int sokoban_check_boardsize(SOKOBAN *skb, int boardsize);
int sokoban_check_boardsize_2(SOKOBAN *skb); //return the number of squares
int sokoban_solve(SOKOBAN *skb);
int sokoban_solve_2(SOKOBAN *skb); //for compare purpose


#endif


