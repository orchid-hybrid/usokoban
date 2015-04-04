#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>



#include "sokoban.h"
#include "savitch.h"
#include "solver.h"


//corners are mark 'c' in the corner stirng
// g also points to level, rather than goal
void 		level_find_corner_2(char *lvl, char * g, char *corner, int wh, int w){

	int i,j,k;

	//init
	for(i=0;i<wh;i++) {
		switch(lvl[i]){
			case '#':
			case '_':
				corner[i]=lvl[i];break;
			default: corner[i]='-'; break;
		}
	}

	//first mark
	for(i=0;i<wh;i++){
		if(corner[i]=='-'){
			if( (corner[i-1]=='#' && corner[i-w]=='#') || 
				(corner[i-1]=='#' && corner[i+w]=='#') ||
				(corner[i+1]=='#' && corner[i-w]=='#') ||
				(corner[i+1]=='#' && corner[i+w]=='#') ){
				
				if(g[i]!='$') corner[i]='c'; //mark corner squares

			}
			else if( corner[i-1]=='#' || corner[i+1]=='#' ||
				corner[i-w]=='#' || corner[i+w]=='#' ){

				if(g[i]!='$') corner[i]='e'; //mark edge squares
			}

		}
		
	}

	//second mark
	for(i=0;i<wh;i++){
		if(corner[i]=='e'){
			//horizontal check
			if(corner[i-1]=='c'){
				for(j=i; corner[j]=='e';j++){

				}
				if(corner[j]=='c'){
					for(k=i;k<j;k++) corner[k]='c'; //mark a line of edge squares
				}
			}
			//vertical check
			if(corner[i-w]=='c'){
				for(j=i; corner[j]=='e'; j+=w){}
				if(corner[j]=='c'){
					for(k=i;k<j;k+=w) corner[k]='c'; //mark a vertical edge
				}

			}			
			

		}

	}
	 
}

//return 1 is done
//return 0 if the current configuration is the last configuration
int box_next_config(int *box, int nbox, int nsquare){
	int i=1,j;	
	if(box[nbox-1]!=nsquare-1) {
		(box[nbox-1])++;
		return 1;
	}
	for(; box[nbox-i]==nsquare-i && i<=nbox; i++){}

	if(i== (nbox+1) ) return 0;

	box[nbox-i]++;
	for(j=nbox-i+1; j<nbox;j++){
		box[j]=box[j-1]+1;
	}
	return 1;
}

void box_init_config(int *box, int nbox){
	int i;
	for(i=0;i<nbox;i++){
		box[i]=i;
	}
	

}

void box_printf(int *box, int nbox){
	int i;
	for(i=0;i<nbox;i++){
		printf("%d,",box[i]);
	}
	printf("\n");
}

//return the number of different boxes
//the index [in lvl1] of the last box is return by 'box'
int level_compare_box(char *lvl1, char *lvl2, int wh, int *box){
	int i,n=0;
	for(i=0;i<wh;i++){
		if(lvl1[i]=='$'){
			if(lvl2[i]!='$') {*box=i; n++;}
		}

	}

	return n;
}

//return 1 if r2 and be obtained from r1 in at most n pushes
// 0 otherwise
int savitch(RNODE *r1, RNODE *r2, int n, int k){
	RNODE rm;
	int i,j;	
	int box1,box2;
	int wh=r1->w * r1->h;
	int bi2i[LEVEL_SIZE * sizeof(int)];
	int box[LEVEL_SIZE * sizeof(int)];
	char corner[LEVEL_SIZE * sizeof(int)];
	int nbox=0;
	int nsquare=0;
	char clvl[LEVEL_SIZE]; // a copy of the level without boxes
	//char mlvl[LEVEL_SIZE];
	int t[LEVEL_SIZE * sizeof(int)];
	int timestamp=1;
	int direction;
	int n1=n/2;
	int n2=n/2 + n%2;
	int dead;
	k++;
	memset(t,0,LEVEL_SIZE *sizeof(int));

	//printf("n1,n2=%d,%d (k=%d)\n",n1,n2, k);
	//
	strncpy(clvl, r1->level, wh);
	for(i=0;i<wh;i++) {
		if(clvl[i]=='$') clvl[i]='-';
	}

	//find the number of boxes and squares
	for(i=0;i<wh;i++){
		switch(r1->level[i]){
			case '$':
				bi2i[nsquare]=i;
				nbox++;
				nsquare++;break;
			case '@':
			case '-':
				bi2i[nsquare]=i;
				nsquare++;break;
			default: bi2i[i]=0xFFFFFFFF;break;
		}		
	}
	//printf("#box=%d\n", nbox);
	//printf("#square=%d\n", nsquare);

	//find corner
	level_find_corner_2(r1->level, r2->level, corner, wh, r1->w);
	

	//check for 0 or 1 step
	switch(level_compare_box(r1->level, r2->level, wh, &box1)){
		case 0:
			if ( level_compare(r1->level,r1->w, r1->pusher, r2->pusher, t, &timestamp)) return 1;
			 
			break;
		case 1:
			level_compare_box(r2->level, r1->level, wh, &box2);
			direction=box2-box1;
			//printf("direction=%d\n",direction);
			if(direction==1 || direction==-1 || direction==r1->w || direction==r1->w * (-1) ){
				if ( level_compare(r1->level, r1->w, r1->pusher, box1-direction, t, &timestamp) && 
					level_compare(r2->level, r1->w, box1, r2->pusher, t, &timestamp)  ) return 1; // tricky
				 
			}
			break;
		default:
			break;
	}

	if(n<=1){ return 0;}

	//
	box_init_config(box, nbox);
	do{
		//box_printf(box,nbox);
		dead=0;		

		strncpy(rm.level,clvl,wh);
		for(i=0;i<nbox;i++){
			if( corner[ bi2i[box[i]] ]=='c') {dead=1;break;}
			rm.level[ bi2i[box[i]] ]='$';
		}

		if(dead) continue;//this is a dead configuration, try next

		rm.w=r1->w;
		rm.h=r1->h;
		rm.pusher=0;
		
		for(i=0;i<nsquare;i++){
			if( rm.level[ bi2i[i] ]!='$' ) { //assign pusher to each of the empty square
				if(rm.pusher!=0 && level_compare(rm.level, rm.w, rm.pusher, bi2i[i], t, &timestamp) ) continue; //next square	
				rm.pusher = bi2i[i];
				//printf("pusher=%d\n",rm.pusher);

				//recursive part
				if( savitch(r1, &rm, n1, k) && savitch(&rm, r2, n2, k) ) {
					
					for(j=0;j<wh;j++){
						if(j%r1->w==0) printf("\n");
						printf("%c", rm.level[j]);
					}printf(" (pusher=%d) [%d] \n",rm.pusher,k);					
					return 1;
				}

			}

		}
	}
	while(box_next_config(box, nbox, nsquare));
	
	return 0;
}

//the main function for SOKOBAN type
//return 1 if the level can be solved in n pushes,
//return 0 otherwise
int sokoban_savitch(SOKOBAN *skb, int n){
	RNODE r1;
	RNODE r2;
	int i,j,k=0;
	int wh=skb->w *skb->h;

	int t[LEVEL_SIZE * sizeof(int)];
	int timestamp=1;
	memset(t,0,LEVEL_SIZE *sizeof(int));
	
	//init r1
	strncpy(r1.level, skb->level, wh);
	r1.w=skb->w;
	r1.h=skb->h;
	r1.pusher=skb->w* skb->pi + skb->pj;
	r1.level[r1.pusher]='-';

	/*
	for(i=0;i<wh;i++){
		if(i%skb->w==0) printf("\n");
		printf("%c", r1.level[i]);
	}printf("\n");*/

	//init r2
	strncpy(r2.level, skb->level, wh);
	for(i=0;i<wh;i++){
		if(r2.level[i]=='$') r2.level[i]='-';
		if(skb->target[i]=='.') r2.level[i]='$';
	}
	r2.w=skb->w;
	r2.h=skb->h;
	r2.pusher=0;

	/*
	printf("pusher=%d\n",r2.pusher);

	for(i=0;i<wh;i++){
		if(i%skb->w==0) printf("\n");
		printf("%c", r2.level[i]);
	}printf("\n"); 
	
	printf("diff=%d\n", level_compare_box(r1.level, r2.level, wh, &i) ); */


	
	
	//as we have no idea where the pusher should be
	// in the solved configuration, we should check all possible positions
	for(i=0;i<wh;i++){
		if(r2.level[i]=='-'){ // a possible final position of the pusher
			if(r2.pusher!=0 && level_compare(r2.level, r2.w, r2.pusher, i, t, &timestamp) ) continue; //next square			
			r2.pusher=i;
			if( savitch(&r1, &r2, n, k) ){
				return 1;
			}
		}

	}
	

	

	return 0;
}



