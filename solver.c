#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>



#include "sokoban.h"
#include "solver.h"


void i2bi_init(char *lvl, int w, int h, int *i2bi){

	int i,bi=0;
	for(i=0;i<w*h;i++){
		switch(lvl[i]){
			case '-':
			case '$':
			case '@':
				i2bi[i]=bi; bi++; break;
			default: i2bi[i]=99; break;
		}

	}
	/*
	for(i=0;i<w*h;i++){
		if(i%w==0) printf("\n");
		printf("%d,",i2bi[i]);
	} */
}

//return 1 if freeze deadlock detected
//only detect freeze deadlock of the pattern
//
//  $$
//  ##
//
// and at least one of the two boxes are not in the goal position
int	 level_check_freeze_deadlock(char *lvl, char *corner,  int w, int box){

	
	if( lvl[box+1]=='$' && (corner[box]=='e' || corner[box+1]=='e') ){   
		if(lvl[box-w]=='#' && lvl[box-w+1]=='#') return 1;
		if(lvl[box+w]=='#' && lvl[box+w+1]=='#') return 1;
	}
	if( lvl[box-1]=='$' && (corner[box]=='e' || corner[box-1]=='e') ){
		if(lvl[box-w]=='#' && lvl[box-w-1]=='#') return 1;
		if(lvl[box+w]=='#' && lvl[box+w-1]=='#') return 1;
	}
	if( lvl[box+w]=='$' && (corner[box]=='e' || corner[box+w]=='e') ){
		if(lvl[box+1]=='#' && lvl[box+w+1]=='#') return 1;
		if(lvl[box-1]=='#' && lvl[box+w-1]=='#') return 1;
	}
	if( lvl[box-w]=='$' && (corner[box]=='e' || corner[box-w]=='e') ){
		if(lvl[box+1]=='#' && lvl[box-w+1]=='#') return 1;
		if(lvl[box-1]=='#' && lvl[box-w-1]=='#') return 1;
	}		
 

	return 0;

}

int		level_find_heuristic(char *lvl, int *heu, int wh){

	int i;
	int h=0;
	for(i=0;i<wh;i++){
		if(lvl[i]=='$') h+=heu[i];

	}
	return h;

}

//find the minimum push number for each square to a nearest target
void heuristic_init(int *heu, char *lvl, char *goal, int wh, int w){

	int i;
	int expand=1;
	int k=0;

	//init
	for(i=0;i<wh;i++){
		switch(lvl[i]){
			case '#':
				heu[i]=1111;break;
			case '@':
			case '-':
			case '$':
				heu[i]=1000;break;
			default: heu[i]=9999; break;
		}
		if(goal[i]=='.') heu[i]=0; //target squares are init to be 0
	}

	// loop for expanding 
	while(expand==1){
		expand=0;

		for(i=0;i<wh;i++){
			if(heu[i]==k){ // expand from a k-step square
				
				if( heu[i-1]==1000 && heu[i-2]<1001) {heu[i-1]=k+1; expand=1;}
				if( heu[i+1]==1000 && heu[i+2]<1001) {heu[i+1]=k+1; expand=1;}
				if( heu[i+w]==1000 && heu[i+2*w]<1001) {heu[i+w]=k+1; expand=1;} 
				if( heu[i-w]==1000 && heu[i-2*w]<1001) {heu[i-w]=k+1; expand=1;}
			}
		}

		k++;
	}

}

//corners are mark 'c' in the corner stirng
void 		level_find_corner(char *lvl, char * g, char *corner, int wh, int w){

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
				
				if(g[i]!='.') corner[i]='c'; //mark corner squares

			}
			else if( corner[i-1]=='#' || corner[i+1]=='#' ||
				corner[i-w]=='#' || corner[i+w]=='#' ){

				if(g[i]!='.') corner[i]='e'; //mark edge squares
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

//return push direction L,U,R or D
char level_find_push_direction(char *nlvl, char *olvl, int wh, int w, int pusher){
	int i,di;
	for(i=0;i<wh;i++){
		if(nlvl[i]=='$'){
			if(olvl[i]!='$'){
				di=i-pusher;
				if(di ==1) return 'R';
				else if (di ==-1) return 'L';
				else if (di ==w) return 'D';
				else if (di ==(-1)*w) return 'U';
				else   return 'F'; 

				
			}
		}
	}

}

//return 1 is sovle, 0 otherwise
int		level_check(char *lvl, char *g, int wh){

	int i;
	for(i=0;i<wh;i++){
		if(lvl[i]=='$'){
			if(g[i]!='.') return 0;

		}

	}

	return 1;
}

//do one push, new position is written to nlvl
void		level_do_push(char *wlvl, char *nlvl, int wh, int w, int box, int direction){
	int i;
	for(i=0;i<wh;i++){

		switch(wlvl[i]){
			case '@':
				nlvl[i]='-';
			break;
			default:
				nlvl[i]=wlvl[i];
			break;
		}
	}
	/*
	switch(direction){
		case 'u':
			nlvl[box-w]='$';break;
		case 'd':
			nlvl[box+w]='$';break;
		case 'l': nlvl[box-1]='$';break;
		case 'r': nlvl[box+1]='$';break;
	}*/
	nlvl[box+direction]='$';
	nlvl[box]='-';
}

//return 1 if two positions are the same,
// i.e. the pusher can move from position 'pa' to position 'pb'
// without disturbing the boxes.
//return 0 otherwise
int 		level_compare(char *lvl, int w, int pa, int pb, int *t, unsigned int *timestamp){
	int q[65];
	int tail=0, head=0;
	
	 

	//there used to be a bug here, 65 is replaced by 255
	//	
	(*timestamp)++;
	if(*timestamp==0xFFFFFFFF){
		memset(t,0,255*sizeof(int)); *timestamp=1;
	}
	//printf("timestamp=%d\n",*timestamp);	 

	
	t[pa]=*timestamp;
	q[tail]=pa;

	if(pa==pb) { return 1;}

	 

	while(tail<=head){
		if(lvl[q[tail]+1]=='-' && t[q[tail]+1] <*timestamp ){
			if(q[tail]+1==pb) {  return 1;}
			head++;
			q[head]=q[tail]+1;
			t[q[head]]=*timestamp;

		}
		if(lvl[q[tail]-1]=='-' && t[q[tail]-1] <*timestamp ){
			if(q[tail]-1==pb) {  return 1;}
			head++;
			q[head]=q[tail]-1;
			t[q[head]]=*timestamp;
		}
		if(lvl[q[tail]+w]=='-' && t[q[tail]+w] <*timestamp ){
			if(q[tail]+w==pb) {  return 1;}
			head++;
			q[head]=q[tail]+w;
			t[q[head]]=*timestamp;
		}
		if(lvl[q[tail]-w]=='-' && t[q[tail]-w] <*timestamp ){
			if(q[tail]-w==pb) {  return 1;}
			head++;
			q[head]=q[tail]-w;
			t[q[head]]=*timestamp;
		}

		tail++;

	}

	
	return 0;
}

//all reachable square are replaced by '@':
void 		level_find_reachable_area(char *level, int w, int h, int pusher){
	int q[65];
	int tail=0, head=0;

	q[tail]=pusher;
	while(tail<=head){
		if(level[q[tail]+1]=='-'){
			head++;
			q[head]=q[tail]+1;
			level[q[tail]+1]='@';
		}
		if(level[q[tail]-1]=='-'){
			head++;
			q[head]=q[tail]-1;
			level[q[tail]-1]='@';
		}
		if(level[q[tail]+w]=='-'){
			head++;
			q[head]=q[tail]+w;
			level[q[tail]+w]='@';
		}
		if(level[q[tail]-w]=='-'){
			head++;
			q[head]=q[tail]-w;
			level[q[tail]-w]='@';
		}
		tail++;

	}

}

void               bitstring_2_level(unsigned long long bitstring, char *flvl, int wh, char *tlvl){

	int i,bi=0;

	for(i=0;i<wh;i++){
		switch(flvl[i]){
			case '@':
			case '-':
			case '$':
				if( (bitstring) & ((unsigned long long) 1<< bi) ) tlvl[i]='$';
				else tlvl[i]='-';
				bi++;
			break;
			default:
				tlvl[i]=flvl[i];
			break;

		}

	}

	//for(i=0;i<wh;i++) printf("%c", tlvl[i]); printf("\n");
}

ZOBRIST_KEY     bitstring_2_zobrist(unsigned long long bitstring, ZOBRIST_KEY *zob){

	int i;
	ZOBRIST_KEY zkey=0;
	for(i=0;i<64;i++){
		if( bitstring & (((unsigned long long)1)<<i) ){

			zkey^=zob[i];
		}


	}
	//printf("zkey=%llX \n",zkey );
	return zkey;

}

//return -1 if pusher not found, else return the index of the pusher in the string 'level'
int level_2_pusher(char *level, int w, int h){
	int i;

	for(i=0;i<w*h;i++){
		if(level[i]=='@') {   return i;}

	}

	return -1;

}

unsigned long long level_2_bitstring(char *level, int w, int h){
	unsigned long long bitstring=0;
	int i,bi=0;

	for(i=0;i<w*h;i++){
		switch(level[i]){
			case '$':
				bitstring+= ((unsigned long long) 0x1)<<bi;
				bi++; break;
			case '@':
			case '-':
				bi++;break;
			default: break;

		}
		
	}
	//printf("bitstring=%llX \n", bitstring);
	return bitstring;

}

int zobrist_init(ZOBRIST_KEY *zob, int boardsize){

	int i;

	srand(time(NULL));
	for(i=0;i<boardsize;i++){
		zob[i]=( ((ZOBRIST_KEY)rand())<<48)+(((ZOBRIST_KEY)rand())<<32)+(((ZOBRIST_KEY)rand())<<16)+(((ZOBRIST_KEY)rand())<<8)+(ZOBRIST_KEY)rand();

		//printf("%d=%llX\n",i, zob[i]);

	}

	//printf("size=%d\n", sizeof(ZOBRIST_KEY));
	//printf("SNODE size=%d\n", sizeof(SNODE));
	return 1;
}

ZOBRIST_KEY	   level_2_zobrist(char *level, int wh, ZOBRIST_KEY *zob){

	ZOBRIST_KEY zkey=0;
	int i, bi=0;

	for(i=0;i< wh;i++){
		switch(level[i]){
			case '$':
				zkey^=zob[bi];
				bi++;
			break;
			case '@':
			case '-':
				bi++;
			break;
			default: 
			break;
		

		}
		//if (bi>64) return 0;
	}
	//printf("zkey=%llX,hash_value= %llu [%u], (%u)\n",zkey,  (zkey % (1024*1024) ),(int) (zkey % (1024*1024)), bi );

	//level_2_bitstring(skb->level, skb->w, skb->h);
	return zkey;

}

unsigned long long sokoban_find_zobrist_key(SOKOBAN *skb, ZOBRIST_KEY *zob, int boardsize){
	ZOBRIST_KEY zkey=0;

	int i,bi=0;

	for(i=0;i<skb->w*skb->h;i++){
		switch(skb->level[i]){
			case '$':
				zkey^=zob[bi];
				bi++;
			break;
			case '@':
			case '-':
				bi++;
			break;
			default: 
			break;
		

		}
		if (bi>=boardsize) return zkey;
	}
	//printf("zkey=%llX,hash_value= %llu [%u], (%u)\n",zkey,  (zkey % (1024*1024) ),(int) (zkey % (1024*1024)), bi );

	//level_2_bitstring(skb->level, skb->w, skb->h);
	return zkey;


}

//return 1 if the number of real squares <= boardsize
int sokoban_check_boardsize(SOKOBAN *skb, int boardsize){

	int i,bi=0;

	for(i=0;i<skb->w*skb->h;i++){
		switch(skb->level[i]){
			case '-':
			case '@':
			case '$': bi++; break;

		}

	}

	if(bi>boardsize) return 0; else return 1;

}

//return the number of reachable squares
int sokoban_check_boardsize_2(SOKOBAN *skb){

	int i,bi=0;

	for(i=0;i<skb->w*skb->h;i++){
		switch(skb->level[i]){
			case '-':
			case '@':
			case '$': bi++; break;

		}

	}

	return bi;
}

//return 1 if solution is found
//and solution is append to skb->solution
int sokoban_solve(SOKOBAN *skb){

	SNODE **bucket;
	SNODE *tail,*head, *q0;//used for the searching queue
	SNODE **heub;
	SNODE *pnode;

	char *wlvl; // the working level, i.e. the current position being expanded
	char *nlvl; // new level expanded from the working level
	char *corner;

	ZOBRIST_KEY     zobrist[64];
	int 	zobrist_value;
	unsigned long long bitstring;

	int direction[MAX_DIRECTION]={-1,1,0,0};

	int *heu;

	int i,j;
	int wh;
	int new;
	int found=0;
	int count=1;
	int *i2bi;

	int *t;
	unsigned int timestamp=0; //timestamp method for compare

	char push;
	char path[3000]="";
	int di; //for retrieving path
	SOKOBAN *skb_temp=sokoban_new();
	sokoban_copy(skb_temp, skb);
	sokoban_wakeup(skb_temp);

	
	
	//for compare
	t=calloc(255, sizeof(int)); 
		

	if(sokoban_check_boardsize(skb, 64)==0){
		printf("I can't solve this level because the number of squares are more than 64.\n");
		printf("#squares=%d\n", sokoban_check_boardsize_2(skb));
		return 0;

	}

	bucket=  calloc(1024*1024, sizeof(SNODE *));
	heub = calloc (512, sizeof(SNODE *));
	wh=skb->w* skb->h;
	wlvl=malloc(wh * sizeof(char));
	nlvl=malloc(wh *sizeof(char));
	i2bi=malloc(wh *sizeof(int));
	
	//init i2bi
	i2bi_init(skb->level, skb->w, skb->h, i2bi);

	//corner stuff	
	corner=malloc(wh*sizeof(char));
	level_find_corner(skb->level, skb->target, corner, wh, skb->w);

	//init zobrist keys
	zobrist_init(zobrist, 64);

	//init heuristic
	heu=malloc(wh*sizeof(int));
	heuristic_init(heu,skb->level,skb->target, wh, skb->w);	

	//init searching
	tail=(SNODE *)malloc(sizeof(SNODE));
	q0=tail; // q0 is for debug purpose only
	head=tail;
	tail->bitstring=level_2_bitstring(skb->level,skb->w, skb->h);
	tail->pusher=level_2_pusher(skb->level, skb->w, skb->h);
	tail->g=0;
	tail->h=level_find_heuristic(skb->level, heu, wh);
	tail->znext=NULL;
	tail->qnext=NULL;
	tail->parent=NULL;

	//put starting position to heu bucket
	if(tail->g + tail->h<512) heub[tail->g + tail->h]=tail; 
	else return 0; //if the heuristic function for the initial position is bigger than or equal to 512, the solver give up.
			//though I don't think this case will ever happen
	
	//put starting position to the bucket
	level_2_zobrist(skb->level,wh, zobrist);
	zobrist_value= bitstring_2_zobrist(tail->bitstring, zobrist) % (1024*1024);
	tail->z=zobrist_value;
	tail->znext= (struct SNODE *) bucket [ zobrist_value];
	bucket[zobrist_value]=tail;

	
	//init directions
	direction[2]=skb->w* (-1);
	direction[3]=skb->w ;

	//the main searching loop
	while(1 ){

		//find a node with minimum heuristic function 
		for(i=0;i<512;i++){
			if( heub[i]!=NULL) { //if a node is found, remove it from the heuristic bucket 
				tail=heub[i];
				heub[i]=(SNODE *)heub[i]->qnext;
				break;
			}
		}

		if(i==512) goto BFS_end;
		 
		
		//restore tail from the transiposition table (also the BFS-queue)
		bitstring_2_level(tail->bitstring, skb->level, wh, wlvl);
		wlvl[tail->pusher]='@';
		//for(i=0;i<wh;i++) printf("%c", wlvl[i]); printf("\n");


		//find all pushable boxes
		level_find_reachable_area(wlvl, skb->w, skb->h, tail->pusher);
		//for(i=0;i<wh;i++) printf("%c", wlvl[i]); printf("\n");

		//
		for(i=0;i<wh;i++){
			if(wlvl[i]=='$'){//for each of the box, check that if it can be pushed

				for(j=0;j<MAX_DIRECTION;j++){//for each direction
					//pushed left?
					if(wlvl[i-direction[j] ]=='@' && (wlvl[i+direction[j]]=='@' || wlvl[i+direction[j]]=='-') 
 												&& corner[i+direction[j]]!='c' ){
						level_do_push(wlvl, nlvl, wh, skb->w, i, direction[j] );
						//for(j=0;j<wh;j++) printf("%c", nlvl[j]); printf("\n");
					

						//find zobrist value
						//zobrist_value= level_2_zobrist(nlvl,wh, zobrist) (1024*1024);						
						zobrist_value= ( ((ZOBRIST_KEY) tail->z) ^ zobrist[i2bi[i]] ^ zobrist[i2bi[i+direction[j]]] ) % (1024*1024);
						

						//check if the position is new
						new=1;
						bitstring = level_2_bitstring(nlvl, skb->w, skb->h);

						pnode=bucket[zobrist_value];
						while(pnode != NULL){
						
							if( pnode-> bitstring == bitstring &&
								level_compare(nlvl, skb->w, i, pnode->pusher, t, &timestamp) ){// position is not new
								//printf(".\n");
								new=0; break;
							
							} 
							//printf("+");
							pnode =(SNODE *) pnode->znext;
						}

						//add the new position to transposition table and BFS queue if it's new
						if(new && level_check_freeze_deadlock(nlvl,corner,  skb->w, i+direction[j] )!=1 ){
							count++;
							pnode=malloc(sizeof(SNODE));

							pnode->bitstring=bitstring;
							pnode->pusher=i;
							pnode->g=tail->g+1;
							pnode->h=tail->h+heu[i+direction[j] ]-heu[i];
							pnode->z=zobrist_value;
							//inserted to bucket:
							pnode->znext=(struct SNODE *)bucket[zobrist_value]; bucket[zobrist_value]=pnode;
							//pnode->qnext=NULL; head->qnext=(struct SNODE *) pnode; head=pnode; // append to BFS queue
							if(pnode->g+pnode->h< 512) {
								pnode->qnext=(struct SNODE *) heub[pnode->g+pnode->h]; 
								heub[pnode->g+pnode->h]=pnode; 
							} //inserted to heuristic bucket
							pnode->parent=(struct SNODE *) tail; // for retrieving the path
							if (count%1000==0) printf("node=%d\n", count);

							if( pnode->h==0 ) {found=1; goto BFS_end;}


						}

					
					

					}// End of one direction
				}//End of 4 directions

			}

		}//End of all boxes


		//tail=(SNODE *) tail->qnext;
	}//end while

BFS_end:

/*	//print the bucket for debug purpose
	for(i=0;i<1024*1024;i++){
		pnode=bucket[i];
		while(pnode!=NULL){
			printf("pointer[%d] address=%p\n",i, pnode);

			bitstring_2_level(pnode->bitstring, skb->level, wh, wlvl);
			wlvl[pnode->pusher]='@';
			for(j=0;j<wh;j++) {
				if(j%skb->w==0) printf("\n"); 
				printf("%c", wlvl[j]);
				
			}
			printf("\n"); 

			pnode =(SNODE *) pnode->znext;

		}

	}*/
	
	//print the same thing from the viewpoint of BFS queue
	/*printf("BFS Queue:\n");
	i=0;
	while(q0!=NULL){
		printf("queue[%d] address=%p, g=%d, h=%d\n",i,q0,q0->g,q0->h);
		i++;
		q0 =(SNODE *) q0->qnext;
	} */


	//found?
	if(found==0) return 0;
	printf("Solution found! (total nodes: %d)\n",count);
	printf("#squares = %d\n",sokoban_check_boardsize_2(skb) );
	printf("timestamp = %d\n", timestamp);
	//for(j=0;j<wh;j++) printf("%c", nlvl[j]); printf("\n");
	//for(j=0;j<wh;j++) printf("%c", skb->target[j]); printf("\n");


	//find path
	//pnode=head;
	i=0;

	while(pnode->parent!=NULL){

		

		bitstring_2_level(pnode->bitstring,skb->level,wh, nlvl);
		bitstring_2_level( ((SNODE *) (pnode->parent))->bitstring,skb->level,wh, wlvl);

		push=level_find_push_direction(nlvl,wlvl,  wh, skb->w,pnode->pusher);
		//printf("%c", push );
		path[i]=push; i++;
		switch(push){
			case 'L': di=1;break;
			case 'R': di=-1;break;
			case 'U': di=skb->w;break;
			case 'D': di=skb->w * (-1); break;
			default: di=0;break;
		}

		strncpy(skb_temp->level, wlvl, wh);
		skb_temp->pi= ((SNODE *) (pnode->parent))->pusher/skb->w;//row
		skb_temp->pj= ((SNODE *) (pnode->parent))->pusher % skb->w; //colomn
		skb_temp->solution_head=skb_temp->solution_tail=0;
		sokoban_man_move_to(skb_temp,(pnode->pusher +di) /skb->w ,(pnode->pusher +di) % skb->w );

		while(skb_temp->solution_tail<skb_temp->solution_head){

			skb_temp->solution_head--;
			//printf("%c", skb_temp->solution[skb_temp->solution_head]);
			path[i]=skb_temp->solution[skb_temp->solution_head]; i++;
			
		}

		pnode=(SNODE *) pnode->parent;
	}

	
	//printf("\n");

	

	//path[i]='\n'; printf("%s",path); // i= the length of the 'lurd' solution string

	for(j=0;j<i;j++){
		skb->solution[skb->solution_tail+j]=path[i-1-j];
	}
	skb->solution_head=skb->solution_tail+i;

	/*

	//test
	for(i=0;i<wh;i++){
		if( i%skb->w==0) printf("\n");
		printf("%c", corner[i]);
	} printf("\n");

	//test 2
	//printf("%d,%d,%d\n",'-','_','#');
	for(i=0;i<wh;i++){
		if( i%skb->w==0) printf("\n");
		printf("%d,", heu[i]);
	} printf("\n");

	*/

	//free memory
	free(wlvl); free(nlvl); free(corner); free(heu); 

	debug=0;
	new=0;
	for(i=0;i<1024*1024;i++){
		pnode=bucket[i];
		j=0;
		while(pnode!=NULL){
			q0=pnode;
			j++;
			pnode =(SNODE *) pnode->znext;
			free(q0);
		}
		if(j>debug)debug=j;new+=j;

	}
	printf("max_hash=%d [total=%d]\n",debug,new);

	free(bucket); free(heub); free(t); free(i2bi);

	return 1;
}




