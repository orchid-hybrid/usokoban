#ifndef SOLUTION_H
#define SOLUTION_H


#include "sokoban.h"
#include "solver.h" //use the zobrist key functions

extern ZOBRIST_KEY	zob3k[3000];

//#define SLT_PUSH 0
//#define SLT_MOVE 1
//#define SLT_BOTH 2

//int test();
//int test2();

//constant 2nd arguments for sokoban_load_solution()
#define LS_PUSH 1
#define LS_MOVE 0

void sokoban_open_solution_db();
void sokoban_close_solution_db(); 

void sokoban_load_solution(SOKOBAN *skb, int t);
int sokoban_lookup_solution(SOKOBAN *skb, int *m4);
void sokoban_save_solution(SOKOBAN *skb);

//note that the sokoban_find_zobrist_key() function is in the file solver.c
unsigned long long sokoban_find_zobrist_key_2(SOKOBAN *skb, ZOBRIST_KEY *zob, int boardsize);


#endif
