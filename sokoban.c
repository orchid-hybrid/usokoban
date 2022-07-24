#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <pcre.h>

#include "sokoban.h"
#include "solver.h"
#include "solution.h"

#define OVECCOUNT 30 /* for PCRE, should be a multiple of 3 */

//A string-to-string version of fgets; return the number of bytes used.
int sgets(char *dest, int size, char *source, int offset) {
	int i;
	int slen = strlen(source) - offset;
	char *sptr = source + offset;
	char *dptr = dest;

	if (slen <= 0) return 0;
	if (slen > size-1) slen=size-1; // so we will only look at min(slen,size-1) chars

	for (i=0; i<slen && *sptr !='\n' && *sptr; i++)
		*dptr++ = *sptr++;
	*dptr='\0';

	if (*sptr == '\n') return i+1;
	if (*sptr == '\r' && *++sptr == '\n') return i+2;
	return i;
}

//return CC_LEVEL or CC_SOLUTION
//if more than 90% of the first 100 characters are from [UuLlRrDd0123456789] we decide it is a CC_SOLUTION, otherwise a CC_LEVEL.
int check_clipboard(char *p)
{
	int i,m=0;
	for(i=0;i<100;i++){
		if(p[i]=='\0') break;
		switch(p[i]){
			case 'U':
			case 'u':
			case 'L':
			case 'l':
			case 'R':
			case 'r':
			case 'D':
			case 'd':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				m++; break;
		}
	}
	//g_message("%f",1.0 * m / i );
	if(1.0 * m / i >.9) return CC_SOLUTION; else return CC_LEVEL;


}

//if unselect is actually taken place , return 1, otherwise, return 0
int sokoban_unselect_box(SOKOBAN *skb)
{
	if(skb->bi != 0 || skb->bj!=0) {skb->bi=0; skb->bj=0; return 1;}
	else return 0;
}

//select/unselect box
//write path to skb->solution if available
//return 1 if path has been written, 0 otherwise
int sokoban_click(SOKOBAN *skb, int row, int col)
{
	switch(skb->level[row* skb->w +col]){
		case '$': 
			if(skb->bi==row && skb->bj==col) {skb->bi=0; skb->bj=0;}
			else {skb->bi=row; skb->bj=col; sokoban_box_push_to_hint(skb);} 
			skb->wall_click=0;
			return 0;
		break;
		case '-':
		case '@':
			if(skb->bi!=0 || skb->bj!=0){
				if(sokoban_box_push_to(skb,row,col)){ skb->bi=0; skb->bj=0; return 1;}
			}
			else{
				if(sokoban_man_move_to(skb,row,col)){skb->wall_click=0; return 1;}				
			}
		break;
		case '#':
			skb->wall_click=1-skb->wall_click;
			if(skb->wall_click==1){
				skb->bi=0; skb->bj=0;
				sokoban_reachable_box_hint(skb);
			}
			return 0;
		break;
	}
}

//return 1 if success, 0 otherwise
//the path is written to skb->solution
int sokoban_box_push_to(SOKOBAN *skb, int row, int col)
{
	SOKOBAN *skb_temp=sokoban_new();
	SOKOBAN *skb_temp_2=sokoban_new(); // use for tagging new position only, so no need to wake it up
	
	char *queue_box_i=malloc(SBPT_MAX_SEARCH_POSITION); //should use a structure to do this next time 
	char *queue_box_j=malloc(SBPT_MAX_SEARCH_POSITION);
	char *queue_man_i=malloc(SBPT_MAX_SEARCH_POSITION);
	char *queue_man_j=malloc(SBPT_MAX_SEARCH_POSITION);
	int qhead=0,qtail=0,found=0; // searching queue

	char path[SBPT_MAX_SEARCH_POSITION][SBPT_MAX_STEP_LENGTH]; //for storing partial path in each searching step
	int link_back[SBPT_MAX_SEARCH_POSITION]; 
	char path_buffer[SBPT_MAX_STEP_LENGTH];

	char *path_all=malloc(SOLUTION_LENGTH); //temp buffer for the desired path ( in reversed order)
	int path_all_len =SOLUTION_LENGTH;

	char tag[LEVEL_SIZE][4]; //tags to make positions

	int i,j,len,w=skb->w,h=skb->h,n; // n is used in two different places
	int di,dj;

	sokoban_wakeup(skb_temp);// must wake it up before use

	link_back[qhead]=-1;

	sokoban_copy(skb_temp, skb); sokoban_copy(skb_temp_2,skb);

	//init queue
	queue_box_i[0]=skb->bi;
	queue_box_j[0]=skb->bj;
	queue_man_i[0]=skb->pi;
	queue_man_j[0]=skb->pj;

	

	//init tags
	for(i=0;i<LEVEL_SIZE;i++){
		for(j=0;j<4;j++) tag[i][j]=0;
	}

	//reducing searching nodes (i.e. tag the useless nodes)
	for(i=1;i<skb->h-1;i++){
		for(j=1;j<skb->w-1;j++){
			if( (skb->level[i*skb->w+j]=='-' || skb->level[i*skb->w+j]=='@') && skb->target[i*skb->w+j]!='.' && i!=row && j!=col){
				
				if( (skb->level[(i-1)*skb->w+j]=='#' && skb->level[i*skb->w+j-1]=='#' ) ||\
					(skb->level[(i-1)*skb->w+j]=='#' && skb->level[i*skb->w+j+1]=='#')||\
					(skb->level[(i+1)*skb->w+j]=='#' && skb->level[i*skb->w+j-1]=='#')||\
					(skb->level[(i+1)*skb->w+j]=='#' && skb->level[i*skb->w+j+1]=='#') ) {
					for(n=0;n<4;n++) tag[i*skb->w+j][n]=1; //g_message("%s","done!");

				}
			
			}
	
		}

	}

	if(sokoban_man_move_to_2(skb_temp, skb_temp->bi-1,skb_temp->bj)) tag[skb_temp->bi* w+ skb_temp->bj][0]=1; //up
	if(sokoban_man_move_to_2(skb_temp, skb_temp->bi+1,skb_temp->bj)) tag[skb_temp->bi* w+ skb_temp->bj][1]=1; //down
	if(sokoban_man_move_to_2(skb_temp, skb_temp->bi,skb_temp->bj-1)) tag[skb_temp->bi* w+ skb_temp->bj][2]=1; //left
	if(sokoban_man_move_to_2(skb_temp, skb_temp->bi,skb_temp->bj+1)) tag[skb_temp->bi* w+ skb_temp->bj][3]=1; //right
	
	//g_message("%d,%d,%d,%d",tag[skb->bi*w+skb->bj][0],tag[skb->bi*w+skb->bj][1],tag[skb->bi*w+skb->bj][2],tag[skb->bi*w+skb->bj][3]);

	

	sbpt_unload( skb_temp , skb_temp -> bi, skb_temp ->bj, skb_temp ->pi, skb_temp ->pj);
	sbpt_unload( skb_temp_2 , skb_temp_2 -> bi, skb_temp_2 ->bj, skb_temp_2 ->pi, skb_temp_2 ->pj);

	while(qtail<=qhead){
		//sokoban_copy_with_change(skb_temp,skb,queue_box_i[qtail], queue_box_j[qtail] ,queue_man_i[qtail] ,queue_man_j[qtail]); 
		sbpt_load( skb_temp ,queue_box_i[qtail], queue_box_j[qtail] ,queue_man_i[qtail] ,queue_man_j[qtail]); 

		if(sokoban_man_move_to_2(skb_temp,skb_temp->bi+1, skb_temp->bj) ){ //try push up
			if(( skb_temp->level[ (skb_temp->bi-1)*w + skb_temp->bj] =='-' 
				|| skb_temp->level[ (skb_temp->bi-1)*w + skb_temp->bj] =='@')
				&& tag[(skb_temp->bi-1)*w + skb_temp->bj][1]==0  ){// new unvisited position found
				qhead++;
				strcpy(path[qhead],"U");
				/*for(i=1,j=skb_temp->solution_head - skb_temp->solution_tail; i<=j; i++){
					path[qhead][i]= skb_temp->solution[j-i];
				} 
				path[qhead][i]='\0';*/
				link_back[qhead]=qtail; //save the path (in reversed order)

				queue_box_i[qhead]=skb_temp->bi-1;
				queue_box_j[qhead]=skb_temp->bj;
				queue_man_i[qhead]=skb_temp->bi;
				queue_man_j[qhead]=skb_temp->bj; // new position in queue

				//sokoban_copy_with_change(skb_temp_2,skb,queue_box_i[qhead], queue_box_j[qhead] ,queue_man_i[qhead] ,queue_man_j[qhead]); 
				sbpt_load( skb_temp_2 ,queue_box_i[qhead], queue_box_j[qhead] ,queue_man_i[qhead] ,queue_man_j[qhead]);
				if(sokoban_man_move_to_2(skb_temp_2, skb_temp_2->bi-1,skb_temp_2->bj)) tag[skb_temp_2->bi* w+ skb_temp_2->bj][0]=1; //up
				tag[skb_temp_2->bi* w+ skb_temp_2->bj][1]=1; //down
				if(sokoban_man_move_to_2(skb_temp_2, skb_temp_2->bi,skb_temp_2->bj-1)) tag[skb_temp_2->bi* w+ skb_temp_2->bj][2]=1; //left
				if(sokoban_man_move_to_2(skb_temp_2, skb_temp_2->bi,skb_temp_2->bj+1)) tag[skb_temp_2->bi* w+ skb_temp_2->bj][3]=1; //right(tag new pos)
				sbpt_unload( skb_temp_2 ,queue_box_i[qhead], queue_box_j[qhead] ,queue_man_i[qhead] ,queue_man_j[qhead]);

				if(queue_box_i[qhead]==row && queue_box_j[qhead]==col){ found=qhead; break;}

				//g_message("%d:%s",qhead,path[qhead]);
				//g_message("%d,%d,%d,%d",tag[skb_temp->bi*w+skb_temp->bj][0],tag[skb_temp->bi*w+skb_temp->bj][1],
							//tag[skb_temp->bi*w+skb_temp->bj][2],tag[skb_temp->bi*w+skb_temp->bj][3]);

			}
		}

		//sokoban_copy_with_change(skb_temp,skb,queue_box_i[qtail], queue_box_j[qtail] ,queue_man_i[qtail] ,queue_man_j[qtail]); 

		if(sokoban_man_move_to_2(skb_temp,skb_temp->bi-1, skb_temp->bj) ){ //try push down
			if(( skb_temp->level[ (skb_temp->bi+1)*w + skb_temp->bj] =='-' 
				|| skb_temp->level[ (skb_temp->bi+1)*w + skb_temp->bj] =='@')
				&& tag[(skb_temp->bi+1)*w + skb_temp->bj][0]==0  ){// new unvisited position found
				qhead++;
				strcpy(path[qhead],"D");
				/*for(i=1,j=skb_temp->solution_head - skb_temp->solution_tail; i<=j; i++){
					path[qhead][i]= skb_temp->solution[j-i];
				}
				path[qhead][i]='\0'; */
				link_back[qhead]=qtail; //save the path

				queue_box_i[qhead]=skb_temp->bi+1;
				queue_box_j[qhead]=skb_temp->bj;
				queue_man_i[qhead]=skb_temp->bi;
				queue_man_j[qhead]=skb_temp->bj; // new position in queue

				//sokoban_copy_with_change(skb_temp_2,skb,queue_box_i[qhead], queue_box_j[qhead] ,queue_man_i[qhead] ,queue_man_j[qhead]); 
				sbpt_load( skb_temp_2 ,queue_box_i[qhead], queue_box_j[qhead] ,queue_man_i[qhead] ,queue_man_j[qhead]);
				tag[skb_temp_2->bi* w+ skb_temp_2->bj][0]=1; //up
				if(sokoban_man_move_to_2(skb_temp_2, skb_temp_2->bi+1,skb_temp_2->bj)) tag[skb_temp_2->bi* w+ skb_temp_2->bj][1]=1; //down
				if(sokoban_man_move_to_2(skb_temp_2, skb_temp_2->bi,skb_temp_2->bj-1)) tag[skb_temp_2->bi* w+ skb_temp_2->bj][2]=1; //left
				if(sokoban_man_move_to_2(skb_temp_2, skb_temp_2->bi,skb_temp_2->bj+1)) tag[skb_temp_2->bi* w+ skb_temp_2->bj][3]=1; //right(tag new pos)
				sbpt_unload( skb_temp_2 ,queue_box_i[qhead], queue_box_j[qhead] ,queue_man_i[qhead] ,queue_man_j[qhead]);

				if(queue_box_i[qhead]==row && queue_box_j[qhead]==col){ found=qhead; break;}
				
				//g_message("%d:%s",qhead,path[qhead]);
				//g_message("%d,%d,%d,%d",tag[skb_temp->bi*w+skb_temp->bj][0],tag[skb_temp->bi*w+skb_temp->bj][1],
							//tag[skb_temp->bi*w+skb_temp->bj][2],tag[skb_temp->bi*w+skb_temp->bj][3]);

			}
		}

		//sokoban_copy_with_change(skb_temp,skb,queue_box_i[qtail], queue_box_j[qtail] ,queue_man_i[qtail] ,queue_man_j[qtail]); 

		if(sokoban_man_move_to_2(skb_temp,skb_temp->bi, skb_temp->bj+1) ){ //try push left
			if(( skb_temp->level[ (skb_temp->bi)*w + skb_temp->bj-1] =='-' 
				|| skb_temp->level[ (skb_temp->bi)*w + skb_temp->bj-1] =='@')
				&& tag[(skb_temp->bi)*w + skb_temp->bj-1][3]==0  ){// new unvisited position found
				qhead++;
				strcpy(path[qhead],"L");
				/*for(i=1,j=skb_temp->solution_head - skb_temp->solution_tail; i<=j; i++){
					path[qhead][i]= skb_temp->solution[j-i];
				}
				path[qhead][i]='\0'; */
				link_back[qhead]=qtail; //save the path

				queue_box_i[qhead]=skb_temp->bi;
				queue_box_j[qhead]=skb_temp->bj-1;
				queue_man_i[qhead]=skb_temp->bi;
				queue_man_j[qhead]=skb_temp->bj; // new position in queue

				//sokoban_copy_with_change(skb_temp_2,skb,queue_box_i[qhead], queue_box_j[qhead] ,queue_man_i[qhead] ,queue_man_j[qhead]); 
				sbpt_load( skb_temp_2 ,queue_box_i[qhead], queue_box_j[qhead] ,queue_man_i[qhead] ,queue_man_j[qhead]);
				if(sokoban_man_move_to_2(skb_temp_2, skb_temp_2->bi-1,skb_temp_2->bj)) tag[skb_temp_2->bi* w+ skb_temp_2->bj][0]=1; //up
				if(sokoban_man_move_to_2(skb_temp_2, skb_temp_2->bi+1,skb_temp_2->bj)) tag[skb_temp_2->bi* w+ skb_temp_2->bj][1]=1; //down
				if(sokoban_man_move_to_2(skb_temp_2, skb_temp_2->bi,skb_temp_2->bj-1)) tag[skb_temp_2->bi* w+ skb_temp_2->bj][2]=1; //left
				tag[skb_temp_2->bi* w+ skb_temp_2->bj][3]=1; //right(tag new pos)
				sbpt_unload( skb_temp_2 ,queue_box_i[qhead], queue_box_j[qhead] ,queue_man_i[qhead] ,queue_man_j[qhead]);

				if(queue_box_i[qhead]==row && queue_box_j[qhead]==col){ found=qhead; break;}
				
				//g_message("%d:%s",qhead,path[qhead]);
				//g_message("%d,%d,%d,%d",tag[skb_temp->bi*w+skb_temp->bj][0],tag[skb_temp->bi*w+skb_temp->bj][1],
							//tag[skb_temp->bi*w+skb_temp->bj][2],tag[skb_temp->bi*w+skb_temp->bj][3]);

			}
		}

		//sokoban_copy_with_change(skb_temp,skb,queue_box_i[qtail], queue_box_j[qtail] ,queue_man_i[qtail] ,queue_man_j[qtail]); 

		if(sokoban_man_move_to_2(skb_temp,skb_temp->bi, skb_temp->bj-1) ){ //try push right
			if(( skb_temp->level[ (skb_temp->bi)*w + skb_temp->bj+1] =='-' 
				|| skb_temp->level[ (skb_temp->bi)*w + skb_temp->bj+1] =='@')
				&& tag[(skb_temp->bi)*w + skb_temp->bj+1][2]==0  ){// new unvisited position found
				qhead++;
				strcpy(path[qhead],"R");
				/*for(i=1,j=skb_temp->solution_head - skb_temp->solution_tail; i<=j; i++){
					path[qhead][i]= skb_temp->solution[j-i];
				}
				path[qhead][i]='\0'; */
				link_back[qhead]=qtail; //save the path

				queue_box_i[qhead]=skb_temp->bi;
				queue_box_j[qhead]=skb_temp->bj+1;
				queue_man_i[qhead]=skb_temp->bi;
				queue_man_j[qhead]=skb_temp->bj; // new position in queue

				//sokoban_copy_with_change(skb_temp_2,skb,queue_box_i[qhead], queue_box_j[qhead] ,queue_man_i[qhead] ,queue_man_j[qhead]); 
				sbpt_load( skb_temp_2 ,queue_box_i[qhead], queue_box_j[qhead] ,queue_man_i[qhead] ,queue_man_j[qhead]);
				if(sokoban_man_move_to_2(skb_temp_2, skb_temp_2->bi-1,skb_temp_2->bj)) tag[skb_temp_2->bi* w+ skb_temp_2->bj][0]=1; //up
				if(sokoban_man_move_to_2(skb_temp_2, skb_temp_2->bi+1,skb_temp_2->bj)) tag[skb_temp_2->bi* w+ skb_temp_2->bj][1]=1; //down
				tag[skb_temp_2->bi* w+ skb_temp_2->bj][2]=1; //left
				if(sokoban_man_move_to_2(skb_temp_2, skb_temp_2->bi,skb_temp_2->bj+1)) tag[skb_temp_2->bi* w+ skb_temp_2->bj][3]=1; //right(tag new pos)
				sbpt_unload( skb_temp_2 ,queue_box_i[qhead], queue_box_j[qhead] ,queue_man_i[qhead] ,queue_man_j[qhead]);

				if(queue_box_i[qhead]==row && queue_box_j[qhead]==col){ found=qhead; break;}
				
				//g_message("%d:%s",qhead,path[qhead]);
				//g_message("%d,%d,%d,%d",tag[skb_temp->bi*w+skb_temp->bj][0],tag[skb_temp->bi*w+skb_temp->bj][1],
							//tag[skb_temp->bi*w+skb_temp->bj][2],tag[skb_temp->bi*w+skb_temp->bj][3]);

			}
		}

		sbpt_unload( skb_temp ,queue_box_i[qtail], queue_box_j[qtail] ,queue_man_i[qtail] ,queue_man_j[qtail]); 
		qtail++;

		//if(qtail==2) break; //to be deleted
	}

	//put together the final path if it is found
	if(found){
		
		strcpy(path_all,"");
		while(qhead!=0){
			di=0;dj=0;
			switch(path[qhead][0]){
				case 'U': di=1; break;
				case 'D': di=-1; break;
				case 'L': dj=1; break;
				case 'R': dj=-1; break;
				default: break;
			}
			sbpt_load( skb_temp ,queue_box_i[link_back[qhead]], queue_box_j[link_back[qhead]] ,
								queue_man_i[link_back[qhead]] ,queue_man_j[link_back[qhead]]); 
			sokoban_man_move_to(skb_temp, queue_man_i[qhead]+di ,queue_man_j[qhead]+dj);

			for(i=1,j=skb_temp->solution_head - skb_temp->solution_tail; i<=j; i++){
				path[qhead][i]= skb_temp->solution[j-i];
			}
			path[qhead][i]='\0';
			sbpt_unload( skb_temp ,queue_box_i[link_back[qhead]], queue_box_j[link_back[qhead]] ,
								queue_man_i[link_back[qhead]] ,queue_man_j[link_back[qhead]]); //retrieve the path

			if(strlen(path[qhead]) + strlen(path_all) >= path_all_len-1 ){ //ask for more memory if needed
				path_all_len += SOLUTION_LENGTH ; // note: this increase may not be enough in extreme case
								      //i.e. the path to be added itself may not more than twice of SOLUTION_LENGTH
                                                                       // in this case, it must be a extreme big level
				path_all=realloc(path_all, path_all_len);
			}
			strcat(path_all,path[qhead] );
			qhead=link_back[qhead];
		}

		len=strlen(path_all); skb->solution_head=skb->solution_tail+len;

		n=(skb->solution_head / SOLUTION_LENGTH) - (skb->solution_len / SOLUTION_LENGTH); //ask for more memory if needed
		if(skb->solution_len % SOLUTION_LENGTH ==0) n++;
		if( n >0 ){
			skb->solution_len += (n* SOLUTION_LENGTH);
			skb->solution=realloc(skb->solution, skb->solution_len);
		}

		//g_message("%d",skb->solution_len);

		for(i=0;i<len;i++){
			skb->solution[skb->solution_tail+i]=path_all[len-1-i];
		}

		//g_message("found=%d",found);

	}

	


	free(skb_temp); free(skb_temp_2);  free(queue_box_i); free(queue_box_j); free(queue_man_i); free(queue_man_j);  free(path_all);

	if(found) return 1; else return 0;
}


//(used as sub-routine for sokoban_box_push_to()  )
void sbpt_load(SOKOBAN *skb , int nbi, int nbj, int npi, int npj)
{

	skb->pi=npi;
	skb->pj=npj;  
	skb->bi=nbi;
	skb->bj=nbj;
	skb->solution_tail=0;

	skb->level[nbi * skb->w + nbj]='$';
	skb->level[npi * skb->w + npj]='@';
	return;
}

//(used as sub-routine for sokoban_box_push_to()  )
void sbpt_unload(SOKOBAN *skb , int nbi, int nbj, int npi, int npj)
{
	skb->level[nbi * skb->w + nbj]='-';
	skb->level[npi * skb->w + npj]='-';
	return;
}

// (used as sub-routine for sokoban_box_push_to()  )
void sokoban_copy_with_change(SOKOBAN *skb1, SOKOBAN *skb2, int nbi, int nbj, int npi, int npj)
{
	skb1->w=skb2->w;
	skb1->h=skb2->h;
	skb1->pi=npi;
	skb1->pj=npj;  
	skb1->bi=nbi;
	skb1->bj=nbj;
	skb1->solution_tail=0;
	memcpy(skb1->level, skb2->level, skb2->w*skb2->h);
	skb1->level[skb2->bi * skb2->w + skb2->bj]='-';
	skb1->level[skb2->pi * skb2->w + skb2->pj]='-';
	skb1->level[nbi * skb2->w + nbj]='$';
	skb1->level[npi * skb2->w + npj]='@';
}

//copy the level maze, height, width and man position only (used as sub-routine for sokoban_box_push_to()  )
void sokoban_copy(SOKOBAN *skb1, SOKOBAN *skb2)
{
	skb1->w=skb2->w;
	skb1->h=skb2->h;
	skb1->pi=skb2->pi;
	skb1->pj=skb2->pj;  
	skb1->bi=skb2->bi;
	skb1->bj=skb2->bj;
	memcpy(skb1->level, skb2->level, skb2->w*skb2->h);
}

void sokoban_copy_all(SOKOBAN *skb1, SOKOBAN *skb2) // skb->hint is not copy yet
{
	skb1->w=skb2->w;
	skb1->h=skb2->h;
	skb1->pi=skb2->pi;
	skb1->pj=skb2->pj;  
	skb1->bi=skb2->bi;
	skb1->bj=skb2->bj;
	skb1->box_i=skb2->box_i;
	skb1->box_j=skb2->box_j;
	skb1->level_len=skb2->level_len;
	skb1->solution_len=skb2->solution_len;
	skb1->solution_tail=skb2->solution_tail;
	skb1->solution_head=skb2->solution_head;
	skb1->n_push=skb2->n_push;
	skb1->n_boxchange=skb2->n_boxchange;
	skb1->mode=skb2->mode;

	free(skb1->level);free(skb1->target); free(skb1->solution);
	skb1->level=malloc(skb1->level_len);
	skb1->target=malloc(skb1->level_len);
	skb1->solution=malloc(skb1->solution_len);
	memcpy(skb1->level, skb2->level, skb2->level_len);
	memcpy(skb1->target, skb2->target, skb2->level_len);
	memcpy(skb1->solution,skb2->solution, skb2->solution_len);
}


//return 1 if success, 0 otherwise ((used as sub-routine for sokoban_box_push_to()  only)
//does not save the path
int sokoban_man_move_to_2(SOKOBAN *skb, int row, int col)
{
	char *tag=malloc(skb->level_len);
	char *queue_r=malloc(skb->level_len);
	char *queue_c=malloc(skb->level_len);
	int qhead=0,qtail=0,found=0;

	int hr,hc; //used in both searching process and path-retrieving process 
	int dr,dc;

	int i,j;

	//init tag and queue
	for(i=0;i<skb->w * skb->h; i++) tag[i]=0;
	queue_r[0]=skb->pi; queue_c[0]=skb->pj;
	tag[ skb->pi * skb->w + skb->pj]='s';  // tag the pusher position as 's'

	if(skb->pi==row && skb->pj==col) {found=1; goto reach_goal;}// for faster return
	if(skb->level[row*skb->w+col]!='-' && skb->level[row*skb->w+col]!='@') {found=0; goto reach_goal;}

	while(qtail<=qhead){
		hr=queue_r[qtail]; hc=queue_c[qtail];

		for(dr=-1;dr<=1;dr++){
			for(dc=-1;dc<=1;dc++){
				if(abs(dr)+abs(dc)==1){
					if(skb->level[(hr+dr)*skb->w+hc+dc]=='-' && tag[(hr+dr)*skb->w+hc+dc]==0){

						if(hr+dr==row && hc+dc==col){found=1; goto reach_goal;} //break the loop if reachable
						tag[(hr+dr)*skb->w+hc+dc]=1;

						qhead++; queue_r[qhead]=hr+dr; queue_c[qhead]=hc+dc;
					}
				}
			}
		}//explore all 4 neighors
		qtail++;
	}//end while

reach_goal:	

	free(tag); free(queue_r); free(queue_c); 

	if(found) return 1; else return 0;
}

//return 1 if success, 0 otherwise
//the path is written to skb->solution
int sokoban_man_move_to(SOKOBAN *skb, int row, int col)
{
	char *tag=malloc(skb->level_len);
	char *queue_r=malloc(skb->level_len);
	char *queue_c=malloc(skb->level_len);
	char *path=malloc(skb->level_len+1); // need an extra '\0'
	int qhead=0,qtail=0,found=0;

	int hr,hc; //used in both searching process and path-retrieving process 
	int dr,dc;

	int i,j;

	//init tag and queue
	for(i=0;i<skb->w * skb->h; i++) tag[i]=0;
	queue_r[0]=skb->pi; queue_c[0]=skb->pj;
	tag[ skb->pi * skb->w + skb->pj]='s';  // tag the pusher position as 's'

	while(qtail<=qhead){
		hr=queue_r[qtail]; hc=queue_c[qtail];

		for(dr=-1;dr<=1;dr++){
			for(dc=-1;dc<=1;dc++){
				if(abs(dr)+abs(dc)==1){
					if(skb->level[(hr+dr)*skb->w+hc+dc]=='-' && tag[(hr+dr)*skb->w+hc+dc]==0){
						if(dr==-1) tag[(hr+dr)*skb->w+hc+dc]='u';
						else if(dr==1) tag[(hr+dr)*skb->w+hc+dc]='d';
						else if (dc==-1) tag[(hr+dr)*skb->w+hc+dc]='l';
						else tag[(hr+dr)*skb->w+hc+dc]='r';
						qhead++; queue_r[qhead]=hr+dr; queue_c[qhead]=hc+dc;
					}
				}
			}
		}//explore all 4 neighors
		qtail++;
	}//end while

	if(tag[row*skb->w+col]!=0){ //path found!
		path[0]='\0'; hr=row; hc=col;
		while(tag[hr*skb->w +hc]!='s'){
			switch(tag[hr*skb->w +hc]){
				case 'u': strcat(path,"u"); hr++; break;
				case 'd': strcat(path,"d"); hr--; break;
				case 'l': strcat(path,"l"); hc++; break;
				case 'r': strcat(path,"r"); hc--; break;

			}		
		}//path retrieved

		//write path to solution in reversed order
		j=skb->solution_tail;
		for(i=strlen(path)-1;i>=0;i--,j++){
			
			if(j==skb->solution_len){  //ask for more memory if needed
				skb->solution_len+=SOLUTION_LENGTH;
				skb->solution=realloc(skb->solution, skb->solution_len);
			}
			skb->solution[j]=path[i];
		}
		skb->solution_head=j;
		
		found=1;
	} 

	free(tag); free(queue_r); free(queue_c); free(path);

	if(found) return 1; else return 0;
}

//to calculate the row and colomn to which the cursor is pointing 
// return 0 if the cursor poiting outside the maze, 1 otherwise
int calculate_row_col(SOKOBAN *skb, GtkWidget *widget, GdkPixbuf *skin, int px, int py, int *row, int *col)
{
	int tile_size=gdk_pixbuf_get_width(skin)/4;
	int ox=0,oy=0;

	if(widget->allocation.width> tile_size * skb->w) ox= (widget->allocation.width - tile_size * skb->w)/2;
	if(widget->allocation.height> tile_size * skb->h) oy= (widget->allocation.height - tile_size * skb->h)/2;

	*row= (py-oy)/tile_size;
	*col= (px-ox)/tile_size;
	if(*row<0 || *col<0 || *row>=skb->h || *col>=skb->w) return 0;
	else return 1; 

}

// pad '-' to the end of each line so that the level maze will be a rectangle
void pad_level(SOKOBAN *skb)
{
	char temp[LEVEL_SIZE];
	int i,j;
	int width;

	//for(i=0;i<strlen(skb->level);i++) temp[i]=skb->level[i];
	//temp[i]='\0';

	//g_message("%s", skb->level);
	strcpy(temp, skb->level);
	skb->level[0]='\0';


	for(i=0;i<strlen(temp);i++){
		width=0;
		while(temp[i]!='|' && temp[i]!='\0'){
			switch(temp[i]){
				case '-': strcat(skb->level,"-"); break;
				case '@': strcat(skb->level,"@"); break;
				case '+': strcat(skb->level,"+"); break;
				case '$': strcat(skb->level,"$"); break;
				case '*': strcat(skb->level,"*"); break;
				case '#': strcat(skb->level,"#"); break;
				case '.': strcat(skb->level,"."); break;
			}
			i++; width++;
		}
		for(j=0;j<skb->w-width;j++) strcat(skb->level,"-");//padding '-'

	} // finish padding

	//separate goals from maze
	for(i=0;i<skb->h*skb->w;i++){
		switch(skb->level[i]){
			case '*': skb->level[i]='$'; skb->target[i]='.'; break;
			case '.': skb->level[i]='-'; skb->target[i]='.'; break;
			case '+': skb->level[i]='@'; skb->target[i]='.'; break;
		}
	}

	//find coordinate of man
	for(i=0;i<skb->h*skb->w;i++){
		if(skb->level[i]=='@' || skb->level[i]=='+'){
			skb->pi=i/skb->w; skb->pj=i%skb->w; 
			break;
		}
	}
	
}

//
int line_check(char *line)
{
	static initialized=0;
	static pcre *re[LC_OTHER];  // PCRE variables
	const char *error;
	int erroffset;
	int rc;
	int ovector[OVECCOUNT];  /* to keep PCRE happy */
	int ret = LC_OTHER;
	int i;

	const char *restring[LC_OTHER] = {
		"^\\s*$",                     /* LC_BLANK */
		"^[-_\\s]*#+[-_\\s]*$",               /* LC_SOLIDBOUNDARY */
		"^[-_\\s]*(?:#+[-_\\s]+)+#+[-_\\s]*$",    /* LC_BOUNDARY */
		"^[-_\\s]*#[-#@+$* _.]+#[-_\\s]*$",   /* LC_LEVEL */
		"^\\s*(?:;|Author:|Email:|Title:|Website:)",  /* LC_HEADER */
		"^\\s*Comment:",              /* LC_COMMENT_START */
		"^\\s*Comment-[Ee]nd:" };     /* LC_COMMENT_END */

	if (! initialized) {
		for (i=LC_BLANK; i<LC_OTHER; i++) {
			re[i] = pcre_compile(restring[i], PCRE_NEWLINE_ANY,
					     &error, &erroffset, NULL);
			if (re[i] == NULL) {
				g_error("PCRE (LC number %d) compilation failed at offset %d: %s", i, erroffset, error);
			}
		}
		initialized = 1;
	}

	for (i=LC_BLANK; i<LC_OTHER; i++) {
		rc = pcre_exec(re[i], NULL, line, strlen(line), 0, 0,
			       ovector, OVECCOUNT);
		if (rc >= 0) {
			ret=i;
			break;
		}
	}

	return ret;
}


// append a line to skb->level, change the value of skb->w and skb->h
// also count the number of players, boxes/balls and targets
void append_line(SOKOBAN *skb, char *buffer, int *p, int *b, int *t)
{
	int j;
	int len, maxline;

	len=strlen(skb->level);
	if (len==skb->level_len) {
		// We're full up - oops!  Give huge values for width and height to indicate this
		skb->w = skb->h = skb->level_len + 1;
		return;
	}
	maxline = skb->level_len - len - 2;
	// -2 because we're going to put '|\0' at the end of the buffer
	if (maxline>LINE_BUFFER) maxline=LINE_BUFFER;

	for(j=0;j<maxline;j++)
	{
		if(buffer[j]!='\n' && buffer[j]!='\r' && buffer[j]!='\0' )
		{
			switch(buffer[j]){
				case '-':
				case ' ':
				case '_':
					strcat(skb->level,"-");
					break;
				case '#':
					strcat(skb->level,"#");
					break;
				case '@':
					strcat(skb->level,"@");
					(*p)++; break;
				case '$':
					strcat(skb->level,"$");
					(*b)++; break;
				case '.':
					strcat(skb->level,".");
					(*t)++; break;
				case '+':
					strcat(skb->level,"+");
					(*t)++; (*p)++; break;
				case '*':
					strcat(skb->level,"*");
					(*t)++; (*b)++; break;
				// Nothing else should have got through
				default:
					g_warning("Unexpected character %c in append_line()", buffer[j]);
			}
		}
		else{
			strcat(skb->level,"|");
			if(j>skb->w) skb->w=j;
			skb->h++;
			break;
		}
	}//end for
}



//return value: the number of levels in the levelset
int sokoban_open(SOKOBAN *level_set[MAX_LEVELS], char *filename)
{
	FILE *pf;
	char buffer[LINE_BUFFER];
	int i, j=0;
	int level_count=0;
	int lc, line=0, comment_line;
	int status = STATUS_NONLEVEL;
	SOKOBAN *skb;  /* to hold the current level */
	int players=0, boxes=0, targets=0;

	// These are invalid filenames
	if (strcmp(filename,"Clipboard levels")==0) return 0;
	if (strcmp(filename,"built-in")==0) return 0;

	pf=fopen(filename,"r");

	if(pf==NULL) return 0;

	while(fgets(buffer,LINE_BUFFER,pf)!=NULL){
		line++;
		//if it is the begining of a level
		if(status == STATUS_NONLEVEL) {
			lc=line_check(buffer);
			if (lc==LC_LEVEL) { // bad!
				g_warning("Failed to read file at line %d: a mid-level row without a preceding boundary!", line);
				status = STATUS_ERROR;
				break;
			}

			if (lc==LC_BLANK || lc==LC_HEADER || lc==LC_OTHER)
				continue;

			if (lc==LC_COMMENT_START) {
				status=STATUS_COMMENT;
				comment_line=line;
				while(fgets(buffer,LINE_BUFFER,pf)!=NULL){
					line++;
					lc=line_check(buffer);
					if (lc==LC_COMMENT_END) {
						status=STATUS_NONLEVEL;
						break;
					}
				}
				continue;
			}

			if (lc!=LC_SOLIDBOUNDARY && lc!=LC_BOUNDARY) {
				g_warning("Failed to understand the output of line_check() at line %d: output was %d!", line, lc);
				status = STATUS_ERROR;
				break;
			}

			skb=sokoban_new();
			append_line(skb, buffer, &players, &boxes, &targets);//read the first line of the level
			status=STATUS_TOP_BOUNDARY;

			while(fgets(buffer,LINE_BUFFER,pf)!=NULL){
				line++;
				lc=line_check(buffer);
				if (lc==LC_LEVEL) {
					if (status == STATUS_TOP_BOUNDARY || status == STATUS_POSSIBLE_BOUNDARY || status == STATUS_INLEVEL) {
						append_line(skb, buffer, &players, &boxes, &targets);
						status = STATUS_INLEVEL;
					} else {
						// invalid line, so we bail out
						g_warning("Unexpected status (%d) at line %d!", status, line);
						status = STATUS_ERROR;
						break;
					}
				} else if (lc==LC_SOLIDBOUNDARY) {
					if (status == STATUS_TOP_BOUNDARY) {
						// two boundaries in a row for aesthetic reasons
						append_line(skb, buffer, &players, &boxes, &targets);
					} else {
						// could be two bottom boundaries in a row for aesthetic reasons
						append_line(skb, buffer, &players, &boxes, &targets);
						status = STATUS_BOTTOM_BOUNDARY;
					}
				} else if (lc==LC_BOUNDARY) {
					// we must have status == STATUS_{TOP,POSSIBLE,BOTTOM}_BOUNDARY or STATUS_INLEVEL
					append_line(skb, buffer, &players, &boxes, &targets);
					if (status == STATUS_INLEVEL) status = STATUS_POSSIBLE_BOUNDARY;
				} else if ((status == STATUS_POSSIBLE_BOUNDARY || status == STATUS_BOTTOM_BOUNDARY) &&
					   (lc == LC_HEADER || lc == LC_OTHER || lc == LC_BLANK)) {
					// non-level line, so we've finished a level
					status = STATUS_NONLEVEL;
					break;
				} else if ((status == STATUS_POSSIBLE_BOUNDARY || status == STATUS_BOTTOM_BOUNDARY) &&
					   lc == LC_COMMENT_START) {
					// non-level line, so we've finished a level, but must handle this comment
					status=STATUS_COMMENT;
					comment_line=line;
					while(fgets(buffer,LINE_BUFFER,pf)!=NULL){
						line++;
						lc=line_check(buffer);
						if (lc==LC_COMMENT_END) {
							status=STATUS_NONLEVEL;
							break;
						}
					}
					break;
				} else if ((status == STATUS_POSSIBLE_BOUNDARY || status == STATUS_BOTTOM_BOUNDARY) &&
					   lc == LC_COMMENT_END) {
					// weird non-level line, so we've finished a level, but must make a warning
					g_warning("Unexpected Comment-End line at line %d", line);
					status = STATUS_NONLEVEL;
					break;
				} else if (lc != LC_COMMENT_START && lc != LC_COMMENT_END && lc != LC_OTHER && lc != LC_BLANK) {
					g_warning("Unrecognised line_check() output (%d) at file line %d!", lc, line);
					status = STATUS_ERROR;
					break;
				} else {
					// invalid line, so we bail out
					g_warning("Failed to read file at line %d: unexpected end of level!", line);
					status = STATUS_ERROR;
					break;
				}

			} // read the remaining lines of the puzzle

			if (status == STATUS_ERROR || status == STATUS_TOP_BOUNDARY || status == STATUS_INLEVEL) {
				// The latter two can happen if the file ends mid-level
				sokoban_free(skb);
				g_warning("File reading ended mid-level, so ignoring this final level.");
				status == STATUS_ERROR;
				break; // break out of outer file-reading loop
			}

			// We have a successful level!
			// Does the level fit into the allocated space?
			if (skb->w * skb->h <= skb->level_len) {
				if (players != 1) {
					g_warning("Need precisely one player per level!  Ignoring level finishing on line %d", line);
					sokoban_free(skb);
				} else if (targets != boxes) {
					g_warning("Need the same number of targets and boxes/balls!  Ignoring level finishing on line %d", line);
					sokoban_free(skb);
				} else {
					level_set[level_count] = skb;
					level_count++;
					if (level_count == MAX_LEVELS) {
						g_warning("Maximum number of levels (%d) reached.", MAX_LEVELS);
						break;
					}
				}
				players = boxes = targets = 0;
			} else {
				sokoban_free(skb);
				g_warning("The level which finished on or just before line %d was too big; ignoring it.", line);
			}
		} else {
			g_warning("I'm confused: my status is %d at line %d and I shouldn't be here!!", status, line);
			break;
		}

	} //end of file (while)

	if (status == STATUS_ERROR)
		g_warning("Giving up reading the file at this point.");

	if (status == STATUS_COMMENT)
		g_warning("File finished mid-comment; comment started on line %d.", comment_line);

	for(i=0;i<level_count;i++) {
		pad_level(level_set[i]);
		sokoban_normalize(level_set[i]);
		level_set[i]->key =sokoban_find_zobrist_key(level_set[i], zob3k, 3000);
		level_set[i]->key2 =sokoban_find_zobrist_key_2(level_set[i], zob3k, 3000);
	
	}
	
	fclose(pf);
	return level_count;
}

//return value: the number of levels in the levelset
int sokoban_open_from_clipboard(SOKOBAN *level_set[MAX_LEVELS], char *paste)
{
	char buffer[LINE_BUFFER];
	int i, offset=0, used;
	int level_count=0;
	int lc, line=0, comment_line;
	int status = STATUS_NONLEVEL;
	SOKOBAN *skb;  /* to hold the current level */
	int players=0, boxes=0, targets=0;

	while((used = sgets(buffer,LINE_BUFFER,paste,offset)) > 0){
		offset += used;
		line++;
		//if it is the begining of a level
		if(status == STATUS_NONLEVEL) {
			lc=line_check(buffer);
			if (lc==LC_LEVEL) { // bad!
				g_warning("Failed to read clipboard at line %d: a mid-level row without a preceding boundary!", line);
				status = STATUS_ERROR;
				break;
			}

			if (lc==LC_BLANK || lc==LC_HEADER || lc==LC_OTHER)
				continue;

			if (lc==LC_COMMENT_START) {
				status=STATUS_COMMENT;
				comment_line=line;
				while((used=sgets(buffer,LINE_BUFFER,paste,offset)) > 0){
					offset += used;
					line++;
					lc=line_check(buffer);
					if (lc==LC_COMMENT_END) {
						status=STATUS_NONLEVEL;
						break;
					}
				}
				continue;
			}

			if (lc!=LC_SOLIDBOUNDARY && lc!=LC_BOUNDARY) {
				g_warning("Failed to understand the output of line_check() at line %d: output was %d!", line, lc);
				status = STATUS_ERROR;
				break;
			}

			skb=sokoban_new();
			append_line(skb, buffer, &players, &boxes, &targets);//read the first line of the level
			status=STATUS_TOP_BOUNDARY;

			while((used=sgets(buffer,LINE_BUFFER,paste,offset)) > 0){
				offset += used;
				line++;
				lc=line_check(buffer);
				if (lc==LC_LEVEL) {
					if (status == STATUS_TOP_BOUNDARY || status == STATUS_POSSIBLE_BOUNDARY || status == STATUS_INLEVEL) {
						append_line(skb, buffer, &players, &boxes, &targets);
						status = STATUS_INLEVEL;
					} else {
						// invalid line, so we bail out
						g_warning("Unexpected status (%d) at line %d!", status, line);
						status = STATUS_ERROR;
						break;
					}
				} else if (lc==LC_SOLIDBOUNDARY) {
					if (status == STATUS_TOP_BOUNDARY) {
						// two boundaries in a row for aesthetic reasons
						append_line(skb, buffer, &players, &boxes, &targets);
					} else {
						// could be two boundaries in a row for aesthetic reasons
						append_line(skb, buffer, &players, &boxes, &targets);
						status = STATUS_BOTTOM_BOUNDARY;
					}
				} else if (lc==LC_BOUNDARY) {
					// we must have status == STATUS_{TOP,POSSIBLE,BOTTOM}_BOUNDARY or STATUS_INLEVEL
					append_line(skb, buffer, &players, &boxes, &targets);
					if (status == STATUS_INLEVEL) status = STATUS_POSSIBLE_BOUNDARY;
				} else if ((status == STATUS_POSSIBLE_BOUNDARY || status == STATUS_BOTTOM_BOUNDARY) &&
					   (lc == LC_HEADER || lc == LC_OTHER || lc == LC_BLANK)) {
					// non-level line, so we've finished a level
					status = STATUS_NONLEVEL;
					break;
				} else if ((status == STATUS_POSSIBLE_BOUNDARY || status == STATUS_BOTTOM_BOUNDARY) &&
					   lc == LC_COMMENT_START) {
					// non-level line, so we've finished a level, but must handle this comment
					status=STATUS_COMMENT;
					comment_line=line;
					while((used=sgets(buffer,LINE_BUFFER,paste,offset)) > 0){
						offset += used;
						line++;
						lc=line_check(buffer);
						if (lc==LC_COMMENT_END) {
							status=STATUS_NONLEVEL;
							break;
						}
					}
					break;
				} else if ((status == STATUS_POSSIBLE_BOUNDARY || status == STATUS_BOTTOM_BOUNDARY) &&
					   lc == LC_COMMENT_END) {
					// weird non-level line, so we've finished a level, but must make a warning
					g_warning("Unexpected Comment-End line at line %d", line);
					status = STATUS_NONLEVEL;
					break;
				} else if (lc != LC_COMMENT_START && lc != LC_COMMENT_END && lc != LC_OTHER && lc != LC_BLANK) {
					g_warning("Unrecognised line_check() output (%d) at clipboard line %d!", lc, line);
					status = STATUS_ERROR;
					break;
				} else {
					// invalid line, so we bail out
					g_warning("Failed to read clipboard at line %d: unexpected end of level!", line);
					status = STATUS_ERROR;
					break;
				}

			} // read the remaining lines of the puzzle

			if (status == STATUS_ERROR || status == STATUS_TOP_BOUNDARY || status == STATUS_INLEVEL) {
				// The latter two can happen if the clipboard ends mid-level
				sokoban_free(skb);
				g_warning("Clipboard reading ended mid-level, so ignoring this final level.");
				status == STATUS_ERROR;
				break; // break out of outer clipboard-reading loop
			}

			// We have a successful level!
			// Does the level fit into the allocated space?
			if (skb->w * skb->h <= skb->level_len) {
				if (players != 1) {
					g_warning("Need precisely one player per level!  Ignoring level finishing on line %d", line);
					sokoban_free(skb);
				} else if (targets != boxes) {
					g_warning("Need the same number of targets and boxes/balls!  Ignoring level finishing on line %d", line);
					sokoban_free(skb);
				} else {
					level_set[level_count] = skb;
					level_count++;
					if (level_count == MAX_LEVELS) {
						g_warning("Maximum number of levels (%d) reached.", MAX_LEVELS);
						break;
					}
				}
				players = boxes = targets = 0;
			} else {
				sokoban_free(skb);
				g_warning("The level which finished on or just before line %d was too big; ignoring it.", line);
			}
		} else {
			g_warning("I'm confused: my status is %d at line %d and I shouldn't be here!!", status, line);
			break;
		}

	} //end of clipboard (while)

	if (status == STATUS_ERROR)
		g_warning("Giving up reading the clipboard at this point.");

	if (status == STATUS_COMMENT)
		g_warning("Clipboard finished mid-comment; comment started on line %d.", comment_line);

	for(i=0;i<level_count;i++) {
		pad_level(level_set[i]);
		sokoban_normalize(level_set[i]);
		level_set[i]->key =sokoban_find_zobrist_key(level_set[i], zob3k, 3000);
		level_set[i]->key2 =sokoban_find_zobrist_key_2(level_set[i], zob3k, 3000);

	}
	
	return level_count;
}

void sokoban_restart(SOKOBAN *skb)
{	
	while(skb->solution_tail!=0) sokoban_undo(skb);

	skb->mode=SKB_UNSOLVE;

}


/////
/* return 1 if undo take place, 0 otherwise */
int sokoban_redo(SOKOBAN *skb)
{
	int head;	

	if(skb->solution_tail==skb->solution_head) return 0;

	head=skb->solution_head;

	sokoban_move(skb, skb->solution[skb->solution_tail]);
	skb->solution_head=head;
	
	return 1;
}



/* return 1 if undo take place, 0 otherwise */
int sokoban_undo(SOKOBAN *skb)
{
	int di=0,dj=0,i,done;

	if(skb->solution_tail==0) return 0; //already return to the begining

	skb->solution_tail--;

	switch(skb->solution[skb->solution_tail])
	{
		case SM_UP: case SM_PUSH_UP: di=-1; break;
		case SM_DOWN: case SM_PUSH_DOWN: di=1; break;
		case SM_LEFT: case SM_PUSH_LEFT: dj=-1; break;
		case SM_RIGHT: case SM_PUSH_RIGHT: dj=1; break;
	}
		
	skb->level[(skb->pi+(-1)*di)*skb->w+skb->pj+(-1)*dj]='@';

	switch(skb->solution[skb->solution_tail]){
		case SM_UP:
		case SM_DOWN:
		case SM_LEFT:
		case SM_RIGHT:
			skb->level[(skb->pi)*skb->w+skb->pj]='-';
		break;
		default: //i.e. push
			skb->level[(skb->pi)*skb->w+skb->pj]='$';
			skb->level[(skb->pi+di)*skb->w+skb->pj+dj]='-';
			--skb->n_push;

			// find last box which has been touched previously.
			// needed for updating box_i, box_j, n_boxchange
			i = skb->solution_tail;
			skb->box_i = (skb->pi)-di; // start with the position of man after reverting the last move
			skb->box_j = (skb->pj)-dj;
			done = 0;
			while (!done) {
				if (i==0) {
					// reverted push was the first push
					skb->box_i = -1;
					skb->box_j = -1;
					done = 1;
				} else {
					--i;
						switch(skb->solution[i]) {
						// as long as man was moving: trace the position of the man
						case SM_UP: ++skb->box_i; break;
						case SM_DOWN: --skb->box_i; break;
						case SM_LEFT: ++skb->box_j; break;
						case SM_RIGHT: --skb->box_j; break;

						// found the last push: now switch to the old box position
						case SM_PUSH_UP: --skb->box_i; done = 1;break;
						case SM_PUSH_DOWN: ++skb->box_i; done = 1;break;
						case SM_PUSH_LEFT: --skb->box_j; done = 1;break;
						case SM_PUSH_RIGHT: ++skb->box_j; done = 1;break;
					}
				}
			}
			// compare old box position with latest man position (before reverting last move),
			// which is the position of the moved box before the last push which gets reverted.
			if ((skb->box_i != skb->pi) || (skb->box_j != skb->pj)) {
				--skb->n_boxchange;
			}
		break;
	}

	skb->pi-=di;
	skb->pj-=dj;
	
	return 1;
}

/* return 1 if a move has been made, return 0 otherwise */
int sokoban_move(SOKOBAN *skb, char direction)
{
	int di=0,dj=0;

	switch(direction)
	{
		case SM_UP: case SM_PUSH_UP: di=-1; break;
		case SM_DOWN: case SM_PUSH_DOWN:  di=1; break;
		case SM_LEFT: case SM_PUSH_LEFT:  dj=-1; break;
		case SM_RIGHT: case SM_PUSH_RIGHT: dj=1; break;
	}

	//g_print("invoked!");

	if(skb->level[(skb->pi+di)*skb->w+skb->pj+dj]=='-'){
		// move
		skb->level[(skb->pi+di)*skb->w+skb->pj+dj]='@';
		skb->level[(skb->pi)*skb->w+skb->pj]='-';
		skb->pi+=di;
		skb->pj+=dj;

		if(skb->solution_tail==skb->solution_len){
			skb->solution_len+=SOLUTION_LENGTH;
			skb->solution=realloc(skb->solution, skb->solution_len);
		} // ask for more memory for storing solution
		skb->solution[skb->solution_tail]=direction;
		skb->solution_tail++;
		skb->solution_head=skb->solution_tail;
		//g_print("invoked!");
		return 1;
	}
	else if(skb->level[(skb->pi+di)*skb->w+skb->pj+dj]=='$' && skb->level[(skb->pi+2*di)*skb->w+skb->pj+2*dj]=='-'){
		// push
		skb->level[(skb->pi+2*di)*skb->w+skb->pj+2*dj]='$';
		skb->level[(skb->pi+di)*skb->w+skb->pj+dj]='@';
		skb->level[(skb->pi)*skb->w+skb->pj]='-';
		skb->pi+=di;
		skb->pj+=dj;
		if(skb->solution_tail==skb->solution_len){
			skb->solution_len+=SOLUTION_LENGTH;
			skb->solution=realloc(skb->solution, skb->solution_len);
		} // ask for more memory for storing solution
		skb->solution[skb->solution_tail]=toupper(direction);
		skb->solution_tail++;
		skb->solution_head=skb->solution_tail;
		++skb->n_push;
		if ((skb->box_i != skb->pi) || (skb->box_j != skb->pj)) {
				++skb->n_boxchange;
		}
		skb->box_i = skb->pi+di;
		skb->box_j = skb->pj+dj;
		return 1;
	}

	return 0;
}


SOKOBAN *sokoban_new_from_built_in()
{

	SOKOBAN *skb;

	skb=malloc(sizeof(SOKOBAN));

	int i;

	skb->level=malloc(15*9);
	skb->target=malloc(15*9);
	memset(skb->target, ' ', sizeof(char)*15*9);
	memcpy(skb->level,"\
###############\
##----##------#\
##$$.-##-$***-#\
#-$.-*-##$.---#\
#--*..-*--..*-#\
#--*$.-#-$$$.-#\
##$.$*-##-.**-#\
##-@#--##--#--#\
###############\
	", 15*9);
	
	skb->level_len=15*9;	

	skb->w=15;
	skb->h=9;

	for(i=0;i<skb->h*skb->w;i++){
		switch(skb->level[i]){
			case '*': skb->level[i]='$'; skb->target[i]='.'; break;
			case '.': skb->level[i]='-'; skb->target[i]='.'; break;
			case '+': skb->level[i]='@'; skb->target[i]='.'; break;
		}
	}

	for(i=0;i<skb->h*skb->w;i++){
		if(skb->level[i]=='@' || skb->level[i]=='+'){
			skb->pi=i/skb->w; skb->pj=i%skb->w; 
			break;
		}
	}

	skb->solution=malloc(SOLUTION_LENGTH);
	skb->solution_len=SOLUTION_LENGTH;
	skb->solution_head=0;
	skb->solution_tail=0;
	skb->bi=0; skb->bj=0;

	skb->n_push=0;
	skb->n_boxchange=0;
	skb->box_i=-1;
	skb->box_j=-1;
	skb->mode=SKB_UNSOLVE;

	skb->key =sokoban_find_zobrist_key(skb, zob3k, 3000);
	skb->key2 =sokoban_find_zobrist_key_2(skb, zob3k, 3000);

	return skb;
}

//return 1 is solved, 0 otherwise
int sokoban_check(SOKOBAN *skb)
{
	int i;

	if(skb->mode==SKB_SOLVED) return 1;

	for(i=0;i<skb->w*skb->h;i++){
		if(skb->target[i]=='.'){
			if(skb->level[i]!='$') return 0;
		}

	}
	skb->mode=SKB_SOLVED;
	sokoban_save_solution(skb);

	return 1;
}

gint max(gint a, gint b) { return a > b ? a : b; }

void sokoban_resize_topwindow(SOKOBAN *skb, GtkWidget *top,GtkWidget *draw_area,   GdkPixbuf *skin)
{
	int tile_size=gdk_pixbuf_get_width(skin)/4;
	int dh=top->allocation.height - draw_area->allocation.height;
	GdkScreen *screen=gdk_screen_get_default();
	int w=gdk_screen_get_width(screen)-1; 
	
	if(tile_size*skb->w <w) w=tile_size*skb->w;
	//g_message("%d,%d",gdk_screen_get_width(screen),gdk_screen_get_height(screen));
	
	//gtk_window_set_default_size (GTK_WINDOW(top),w, tile_size*skb->h+ dh );
	gint oldw, oldh;
	gtk_window_get_size(GTK_WINDOW(top), &oldw, &oldh);
	oldw = max(oldw, w);
	oldh = max(oldh, tile_size*skb->h+ dh);
	gtk_window_resize (GTK_WINDOW(top), oldw, oldh); 

	//gtk_window_set_position(GTK_WINDOW(top), GTK_WIN_POS_CENTER);

	return;
}

void sokoban_show(SOKOBAN *skb, GtkWidget *widget, GdkPixbuf *skin, int type)
{
	int tile_size=gdk_pixbuf_get_width(skin)/4;
	int bi=0,bj=0,ei=skb->h,ej=skb->w;
	int i,j,oi=0,oj=0; //oi,oj for offeset
	int center;//center

	int wall_x=0,wall_y=0;

	if(type==SS_NEW) {
		for(i=0;i<=widget->allocation.height /tile_size;i++)
		for(j=0;j<=widget->allocation.width/tile_size;j++)
			gdk_draw_pixbuf(widget->window, NULL, skin, tile_size*3,tile_size*2, tile_size*j+oj,tile_size*i+oi, 
                               tile_size, tile_size,GDK_RGB_DITHER_NONE,0,0);
	}
	else if(type==SS_FAST_UPDATE){
		if(skb->pi-2 > bi) bi=skb->pi-2; if(skb->pi+3<ei) ei=skb->pi+3;
		if(skb->pj-2 > bj) bj=skb->pj-2; if(skb->pj+3<ej) ej=skb->pj+3;
	}
	

	//calculate offset, in order to center the level
	if(widget->allocation.width> tile_size * skb->w) oj= (widget->allocation.width - tile_size * skb->w)/2;
	if(widget->allocation.height> tile_size * skb->h) oi= (widget->allocation.height - tile_size * skb->h)/2;
	center= (tile_size-4)/2;

	for(i=bi;i<ei;i++)
	{
		for(j=bj;j<ej;j++)
		{
			switch(skb->level[i*skb->w+j])
			{
				case '-':
					if(skb->target[i*skb->w+j]=='.')
						gdk_draw_pixbuf(widget->window, NULL, skin, tile_size*0,tile_size, tile_size*j+oj,tile_size*i+oi, tile_size, tile_size,GDK_RGB_DITHER_NONE,0,0);
					else
					gdk_draw_pixbuf(widget->window, NULL, skin, 0,0, tile_size*j+oj,tile_size*i+oi, tile_size, tile_size,GDK_RGB_DITHER_NONE,0,0);
					if(skb->bi!=0 && skb->hint[i*skb->w+j]==1) 
						gdk_draw_pixbuf(widget->window, NULL, skin,tile_size* 2,tile_size*3, tile_size*j+oj+center,tile_size*i+oi+center, 4, 4,GDK_RGB_DITHER_NONE,0,0);
				break;
				case '@':
					if(skb->target[i*skb->w+j]=='.')
						gdk_draw_pixbuf(widget->window, NULL, skin, tile_size*1,tile_size*1, tile_size*j+oj,tile_size*i+oi, tile_size, tile_size,GDK_RGB_DITHER_NONE,0,0);
					else
						gdk_draw_pixbuf(widget->window, NULL, skin, tile_size,0, tile_size*j+oj,tile_size*i+oi, tile_size, tile_size,GDK_RGB_DITHER_NONE,0,0);
					if(skb->bi!=0 && skb->hint[i*skb->w+j]==1) 
						gdk_draw_pixbuf(widget->window, NULL, skin,tile_size* 2,tile_size*3, tile_size*j+oj+center,tile_size*i+oi+center, 4, 4,GDK_RGB_DITHER_NONE,0,0);
				break;
				case '$':
					if(skb->target[i*skb->w+j]=='.'){
						if( (i==skb->bi && j==skb->bj) || (skb->wall_click==1 && skb->hint[i*skb->w+j]==1) )
							gdk_draw_pixbuf(widget->window, NULL, skin, tile_size*3,tile_size, tile_size*j+oj,tile_size*i+oi, 
							tile_size, tile_size,GDK_RGB_DITHER_NONE,0,0);
						else
							gdk_draw_pixbuf(widget->window, NULL, skin, tile_size*2,tile_size, tile_size*j+oj,tile_size*i+oi, 
							tile_size, tile_size,GDK_RGB_DITHER_NONE,0,0);
					}
					else{
						if((i==skb->bi && j==skb->bj) || (skb->wall_click==1 && skb->hint[i*skb->w+j]==1) )
							gdk_draw_pixbuf(widget->window, NULL, skin, tile_size*3,0, tile_size*j+oj,tile_size*i+oi, 
							tile_size, tile_size,GDK_RGB_DITHER_NONE,0,0);
						else
							gdk_draw_pixbuf(widget->window, NULL, skin, tile_size*2,0, tile_size*j+oj,tile_size*i+oi, 
							tile_size, tile_size,GDK_RGB_DITHER_NONE,0,0);
					}
					//if(skb->wall_click==1 && skb->hint[i*skb->w+j]==1) 
						//gdk_draw_pixbuf(widget->window, NULL, skin,tile_size* 2,tile_size*3, tile_size*j+oj+center,tile_size*i+oi+center, 4, 4,GDK_RGB_DITHER_NONE,0,0);
				break;
				case '#':
					if(type==SS_UPDATE) break;
					//upperleft corner:
					wall_x=1; wall_y=3;
					if(j!=0 && skb->level[i*skb->w+j-1]=='#') wall_y--;
					if(i!=0 && skb->level[(i-1)*skb->w+j]=='#') wall_x--;
					if(wall_x==0 && wall_y==2 && skb->level[(i-1)*skb->w+j-1]=='#') wall_x=2;

					gdk_draw_pixbuf(widget->window, NULL, skin, tile_size*wall_x, tile_size*wall_y, 
                                           tile_size*j+oj,tile_size*i+oi, tile_size/2, tile_size/2,GDK_RGB_DITHER_NONE,0,0);

					//upper right  corner:
					wall_x=1; wall_y=3;
					if(j!=skb->w-1 && skb->level[i*skb->w+j+1]=='#') wall_y--;
					if(i!=0 && skb->level[(i-1)*skb->w+j]=='#') wall_x--;
					if(wall_x==0 && wall_y==2 && skb->level[(i-1)*skb->w+j+1]=='#') wall_x=2;

					gdk_draw_pixbuf(widget->window, NULL, skin, tile_size*wall_x+tile_size/2, tile_size*wall_y,  
						tile_size*j+oj+tile_size/2,tile_size*i+oi, tile_size/2, tile_size/2,GDK_RGB_DITHER_NONE,0,0);

					//lower left corner:
					wall_x=1; wall_y=3;
					if(j!=0 && skb->level[i*skb->w+j-1]=='#') wall_y--;
					if(i!=skb->h-1 && skb->level[(i+1)*skb->w+j]=='#') wall_x--;
					if(wall_x==0 && wall_y==2 && skb->level[(i+1)*skb->w+j-1]=='#') wall_x=2;

					gdk_draw_pixbuf(widget->window, NULL, skin, tile_size*wall_x, tile_size*wall_y+tile_size/2, 
						tile_size*j+oj,tile_size*i+oi+tile_size/2, tile_size/2, tile_size/2,GDK_RGB_DITHER_NONE,0,0);

					//lower left corner:
					wall_x=1; wall_y=3;
					if(j!=skb->w-1 && skb->level[i*skb->w+j+1]=='#') wall_y--;
					if(i!=skb->h-1 && skb->level[(i+1)*skb->w+j]=='#') wall_x--;
					if(wall_x==0 && wall_y==2 && skb->level[(i+1)*skb->w+j+1]=='#') wall_x=2;

					gdk_draw_pixbuf(widget->window, NULL, skin, tile_size*wall_x+tile_size/2, tile_size*wall_y+tile_size/2, 
						tile_size*j+oj+tile_size/2,tile_size*i+oi+tile_size/2, tile_size/2, tile_size/2,GDK_RGB_DITHER_NONE,0,0);

					
				break;
				case '_':
					gdk_draw_pixbuf(widget->window, NULL, skin, tile_size*3,tile_size*2, tile_size*j+oj,tile_size*i+oi, tile_size, tile_size,GDK_RGB_DITHER_NONE,0,0);
				break;


			}

		}

	}


}


void sokoban_show_status(SOKOBAN *skb, GtkWidget *widget)
{

	gint context_id;
	gchar *buff;

	context_id=gtk_statusbar_get_context_id(GTK_STATUSBAR (widget), "USokoban");

	
	if(skb->mode==SKB_UNSOLVE)
		buff = g_strdup_printf ("Moves: %d/%d  Pushes: %d  Box changes: %d", skb->solution_tail,skb->solution_head,skb->n_push,skb->n_boxchange);
	else
		buff = g_strdup_printf ("Moves: %d/%d  Pushes: %d  Box changes: %d     (Solved)", skb->solution_tail,skb->solution_head,skb->n_push,skb->n_boxchange);
	gtk_statusbar_push (GTK_STATUSBAR (widget), context_id, buff);

	g_free (buff);
}

void sokoban_show_solving_status(SOKOBAN *skb, GtkWidget *widget)
{

	gint context_id;
	gchar *buff;

	context_id=gtk_statusbar_get_context_id(GTK_STATUSBAR (widget), "USokoban");
	

	buff = g_strdup_printf ("Moves: %d/%d[*]  Pushes: %d", skb->solution_tail,skb->solution_head,skb->n_push);

	gtk_statusbar_push (GTK_STATUSBAR (widget), context_id, buff);

	g_free (buff);
}

void sokoban_show_title(SOKOBAN *skb, GtkWidget *window , char *filename,int total, int active_id)
{
	int m4[4];
	gchar *buff;

	if(!sokoban_lookup_solution(skb,m4))
		buff=g_strdup_printf("[%d/%d] %s - USokoban", active_id+1, total, filename);
	else
		buff=g_strdup_printf("[%d/%d] (%d/%d,%d/%d) %s - USokoban", active_id+1, total ,m4[0],m4[1],m4[2],m4[3], filename);
	gtk_window_set_title (GTK_WINDOW(window), buff);
	g_free(buff);
}

void sokoban_free(SOKOBAN *skb)
{
	free(skb->level);
	free(skb->target);
	free(skb->solution);
	free(skb->hint);
	free(skb);

}

SOKOBAN  *sokoban_new()
{
	SOKOBAN *skb=malloc(sizeof(SOKOBAN));
	skb->level=malloc(LEVEL_SIZE); skb->level[0]='\0';
	skb->target=malloc(LEVEL_SIZE); skb->target[0]='\0';
	skb->solution=NULL; skb->hint=NULL;
	memset(skb->target, '-', sizeof(char)*LEVEL_SIZE);
	skb->level_len = LEVEL_SIZE;

	skb->w=0; skb->h=0;

	skb->n_push=0;
	skb->n_boxchange=0;
	skb->box_i=-1;
	skb->box_j=-1;
	skb->mode=SKB_UNSOLVE;
	skb->bi=0; skb->bj=0;
	skb->wall_click=0;
	return skb;

}

void sokoban_wakeup(SOKOBAN *skb)
{
	skb->solution=malloc(SOLUTION_LENGTH); skb->solution[0]='\0';	
	skb->solution_len=SOLUTION_LENGTH;
	skb->hint=malloc(skb->level_len); skb->bi=0; skb->bj=0;
	skb->solution_tail=0; skb->solution_head=0;

	skb->wall_click=0;
}

void sokoban_sleep(SOKOBAN *skb)
{
	sokoban_restart(skb);
	free(skb->solution); skb->solution=NULL; 
	free(skb->hint); skb->hint=NULL;
}

