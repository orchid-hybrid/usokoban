#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "sokoban.h"

extern int n_levels;
extern SOKOBAN *level_set[MAX_LEVELS];

void sokoban_levelset_to_js(){

	FILE *js;
	int i,j,k;

	js = fopen( "out.js","w");



	fprintf(js, "function sokoLoadLevelset(){\n\n");

	fprintf(js, "levelset=new Array(\n");
	for(i=0;i<n_levels;i++){

		fprintf(js, "\"");
		//fprintf(js, "%s", level_set[i]->level);
		for(j=0;j<level_set[i]->h;j++){
			for(k=0;k<level_set[i]->w; k++){

				switch(level_set[i]->level[ j* level_set[i]->w +k   ]){
					case '@':
						if (     level_set[i]->target[ j* level_set[i]->w +k   ]=='.'          )
							fprintf(js,  "+");
						else fprintf(js, "%c", level_set[i]->level[ j* level_set[i]->w +k   ]);
						break;
					case '$':
						if (     level_set[i]->target[ j* level_set[i]->w +k   ]=='.'          )
							fprintf(js,  "*");
						else fprintf(js, "%c", level_set[i]->level[ j* level_set[i]->w +k   ]);
						break;
					case '-':
						if (     level_set[i]->target[ j* level_set[i]->w +k   ]=='.'          )
							fprintf(js,  ".");
						else fprintf(js, "%c", level_set[i]->level[ j* level_set[i]->w +k   ]);
						break;
					
					default:
						fprintf(js, "%c", level_set[i]->level[ j* level_set[i]->w +k   ]);break;
				}
			}
			if (j!=level_set[i]->h-1 )fprintf(js, "%c", '|');
		}
		if(i!= n_levels-1 )fprintf(js, "\",\n"); else fprintf(js, "\"\n");

	}
	fprintf(js, ");\n\n");

	fprintf(js, "total=%d;\n\n", n_levels);
	fprintf(js, "current=0;\n\n");

	fprintf(js, "title=new Array(");
	for(i=0;i<n_levels;i++){
		if(i % 50 == 0) fprintf(js,"\n");
		if(i != n_levels-1) fprintf(js,"\"\","); else fprintf(js,"\"\");\n\n");
	}

	fprintf(js, "author=\"\";\n\n");
	fprintf(js, "author_array=0;\n\n");

	fprintf(js, "};\n\n");


	fclose(js);
}

int sokoban_clear_hint(SOKOBAN *skb){
	if(skb->bi!=0 || skb->wall_click!=0){
		skb->bi=0; skb->bj=0;
		skb->wall_click=0;
		return 1;
	}

	return 0;
}

void sokoban_reachable_box_hint(SOKOBAN *skb){
	
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
	tag[ skb->pi * skb->w + skb->pj]=1;  // tag the pusher position as 1

	while(qtail<=qhead){
		hr=queue_r[qtail]; hc=queue_c[qtail];

		for(dr=-1;dr<=1;dr++){
			for(dc=-1;dc<=1;dc++){
				if(abs(dr)+abs(dc)==1){
					if(skb->level[(hr+dr)*skb->w+hc+dc]=='-' && tag[(hr+dr)*skb->w+hc+dc]==0){

						
						tag[(hr+dr)*skb->w+hc+dc]=1;

						qhead++; queue_r[qhead]=hr+dr; queue_c[qhead]=hc+dc;
					}
				}
			}
		}//explore all 4 neighors
		qtail++;
	}//end while

	for(i=0;i<skb->level_len;i++){
		if(skb->level[i]=='$'){
			if(tag[i-1]==1 || tag[i+1]==1 || tag[i+skb->w]==1 || tag[i-skb->w]==1) 
				skb->hint[i]=1;
			else skb->hint[i]=0;
		}
		else skb->hint[i]=0;
	}
	

	free(tag); free(queue_r); free(queue_c); 

	

	return;
}

//
void sokoban_box_push_to_hint(SOKOBAN *skb)
{
	SOKOBAN *skb_temp=sokoban_new();
	SOKOBAN *skb_temp_2=sokoban_new(); // use for tagging new position only, so no need to wake it up
	
	char *queue_box_i=malloc(SBPT_MAX_SEARCH_POSITION); //should use a structure to do this next time 
	char *queue_box_j=malloc(SBPT_MAX_SEARCH_POSITION);
	char *queue_man_i=malloc(SBPT_MAX_SEARCH_POSITION);
	char *queue_man_j=malloc(SBPT_MAX_SEARCH_POSITION);
	int qhead=0,qtail=0,found=0; // searching queue


	char tag[LEVEL_SIZE][4]; //tags to make positions

	int i,j,len,w=skb->w,h=skb->h,n; // n is used in two different places
	int di,dj;


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

			
				
				//g_message("%d:%s",qhead,path[qhead]);
				//g_message("%d,%d,%d,%d",tag[skb_temp->bi*w+skb_temp->bj][0],tag[skb_temp->bi*w+skb_temp->bj][1],
							//tag[skb_temp->bi*w+skb_temp->bj][2],tag[skb_temp->bi*w+skb_temp->bj][3]);

			}
		}

		sbpt_unload( skb_temp ,queue_box_i[qtail], queue_box_j[qtail] ,queue_man_i[qtail] ,queue_man_j[qtail]); 
		qtail++;

		//if(qtail==2) break; //to be deleted
	}

	for(i=0;i<skb->level_len;i++){
		if(tag[i][0]==1 || tag[i][1]==1|| tag[i][2]==1|| tag[i][3]==1) skb->hint[i]=1;
		else skb->hint[i]=0;
			
	}



	free(skb_temp); free(skb_temp_2); free(queue_box_i); free(queue_box_j); free(queue_man_i); free(queue_man_j); 

	return;
} // end box_push_to_hint()


void sokoban_normalize(SOKOBAN *skb)
{
	char *tag=malloc(skb->level_len);
	char *queue_r=malloc(skb->level_len);
	char *queue_c=malloc(skb->level_len);
	int qhead=0,qtail=0;

	int hr,hc; //used in both searching process and path-retrieving process 
	int dr,dc;

	int i,j,k;

	//init tag and queue
	for(i=0;i<skb->w * skb->h; i++) tag[i]=0;
	queue_r[0]=skb->pi; queue_c[0]=skb->pj;
	tag[ skb->pi * skb->w + skb->pj]=1;  // tag the pusher position as 1

	while(qtail<=qhead){ // tag reachable tiles as 1
		hr=queue_r[qtail]; hc=queue_c[qtail];

		for(dr=-1;dr<=1;dr++){
			for(dc=-1;dc<=1;dc++){
				if(abs(dr)+abs(dc)==1){
					if((skb->level[(hr+dr)*skb->w+hc+dc]=='-'|| skb->level[(hr+dr)*skb->w+hc+dc]=='$') && tag[(hr+dr)*skb->w+hc+dc]==0){

						
						tag[(hr+dr)*skb->w+hc+dc]=1;

						qhead++; queue_r[qhead]=hr+dr; queue_c[qhead]=hc+dc;
					}
				}
			}
		}//explore all 4 neighors
		qtail++;
	}//end while

	//tag walls as 2
	for(i=0;i<skb->h;i++){
		for(j=0;j<skb->w;j++){
			if(tag[i*skb->w+j]==1){
				if(tag[i*skb->w+j-1]!=1) tag[i*skb->w+j-1]=2;
				if(tag[i*skb->w+j+1]!=1) tag[i*skb->w+j+1]=2;
				if(tag[(i+1)*skb->w+j]!=1) tag[(i+1)*skb->w+j]=2;
				if(tag[(i-1)*skb->w+j]!=1) tag[(i-1)*skb->w+j]=2;
				if(tag[(i+1)*skb->w+j+1]!=1) tag[(i+1)*skb->w+j+1]=2;
				if(tag[(i-1)*skb->w+j+1]!=1) tag[(i-1)*skb->w+j+1]=2;
				if(tag[(i+1)*skb->w+j-1]!=1) tag[(i+1)*skb->w+j-1]=2;
				if(tag[(i-1)*skb->w+j-1]!=1) tag[(i-1)*skb->w+j-1]=2;

			}
		}
	}

	//
	for(i=0;i<skb->h*skb->w;i++){
		switch(tag[i]){
			case 0:
				skb->level[i]='_';
				skb->target[i]='_'; //bug fixed, have'nt normalized the target map before...
				break;
			case 2: skb->level[i]='#'; break;
			case 1: break;
		}
	}

	free(tag); free(queue_r); free(queue_c);

	//delete empty column
	k=0;
	while(1){

		for(i=0;i<skb->h;i++){
			//g_message("%c\n",skb->level[i*skb->h+k]);
			if(skb->level[i*skb->w+k]!='_') goto handle_empty_column;
		}
		k++;
		
	}
handle_empty_column:
	if(k){
		for(i=0;i<skb->h;i++){
			for(j=k;j<skb->w;j++){
				skb->level[i*(skb->w - k)+ j-k ] = skb->level[i*skb->w+j];
				skb->target[i*(skb->w - k)+ j-k ] = skb->target[i*skb->w+j];

			}		
		}
		skb->w -= k;
		skb->pj -=k;
	}
	

	//delete empty column 2
	k=0;
	while(1){

		for(i=0;i<skb->h;i++){
			if(skb->level[i*skb->w+skb->w-1-k]!='_') goto handle_empty_column_2;
		}
		k++;
	}
handle_empty_column_2:
	if(k){
		for(i=0;i<skb->h;i++){
			for(j=0;j<skb->w - k ;j++){
				skb->level[i*(skb->w - k)+ j ] = skb->level[i*skb->w+j];
				skb->target[i*(skb->w - k)+ j ] = skb->target[i*skb->w+j];

			}		
		}
		skb->w -= k;
	}

	//delete row 
	k=0;
	while(1){

		for(i=0;i<skb->w;i++){
			if(skb->level[k*skb->w+i]!='_') goto handle_empty_row;
		}
		k++;
	}
handle_empty_row:
	if(k){
		for(i=k;i<skb->h;i++){
			for(j=0;j<skb->w  ;j++){
				skb->level[(i-k)*(skb->w )+ j ] = skb->level[i*skb->w+j];
				skb->target[(i-k)*(skb->w )+ j] = skb->target[i*skb->w+j];

			}		
		}
		skb->h -= k;
		skb->pi -=k;
	}

	//delete row 2
	k=0;
	while(1){

		for(i=0;i<skb->w;i++){
			if(skb->level[(skb->h -1-k)*skb->w+i]!='_') goto handle_empty_row_2;
		}
		k++;
	}
handle_empty_row_2:
	if(k){
		for(i=0;i<skb->h-k;i++){
			for(j=0;j<skb->w  ;j++){
				skb->level[(i)*(skb->w )+ j ] = skb->level[i*skb->w+j];
				skb->target[(i)*(skb->w )+ j] = skb->target[i*skb->w+j];

			}		
		}
		skb->h -= k;

	}




	return;
}

void sokoban_copy_level_to_clipboard(SOKOBAN *skb, GtkClipboard *clipboard, int type)
{
	char *buffer;
	char s_buffer[200]="";
	int i,j;
	SOKOBAN *skb_temp=sokoban_new();

	sokoban_copy_all(skb_temp,skb);
	
	while(sokoban_undo(skb_temp));

	switch(type){
		case SCLTC_LEVEL_ONLY:
			buffer=malloc(skb_temp->level_len+2*skb_temp->h);break;
		case SCLTC_WITH_SOLUTION:
			buffer=malloc(skb_temp->level_len+2*skb_temp->h+skb_temp->solution_head+100);break;
		case SCLTC_MF8:
			buffer=malloc(2*skb_temp->level_len+3*skb_temp->h+200);break;
		case SCLTC_SOKOJAVA:
			buffer=malloc(2*skb_temp->level_len+3*skb_temp->h+300);break;
	}

	strcpy(buffer,"");
	for(i=0;i<skb_temp->h;i++){
		for(j=0;j<skb_temp->w;j++){
			switch(skb_temp->level[i*skb_temp->w+j]){
				case '#':
					strcat(buffer,"#");break;
				case '-':
					if(skb_temp->target[i*skb_temp->w+j]=='.') strcat(buffer,".");
					else strcat(buffer,"-");
					break;
				case '$':
					if(skb_temp->target[i*skb_temp->w+j]=='.') strcat(buffer,"*");
					else strcat(buffer,"$");
					break;
				case '@':
					if(skb_temp->target[i*skb_temp->w+j]=='.') strcat(buffer,"+");
					else strcat(buffer,"@");
					break;
				case '_':
					strcat(buffer,"_");break;
			}
		}
		strcat(buffer,"\n");

	}

	

	if(type==SCLTC_WITH_SOLUTION){
		skb_temp->solution[skb_temp->solution_head]='\0';
		strcat(buffer,"\nSolution:\n");
		strcat(buffer,skb_temp->solution); strcat(buffer,"\n");
	}

	if(type==SCLTC_MF8){

		strcat(buffer,"\n[soko=0,0]\n");
		for(i=0;i<skb_temp->h;i++){
			for(j=0;j<skb_temp->w;j++){
				switch(skb_temp->level[i*skb_temp->w+j]){
					case '#':
						strcat(buffer,"H");break;
					case '-':
					case '_':
						if(skb_temp->target[i*skb_temp->w+j]=='.') strcat(buffer,".");
						else strcat(buffer,"_");
						break;
					case '$':
						if(skb_temp->target[i*skb_temp->w+j]=='.') strcat(buffer,"*");
						else strcat(buffer,"$");
						break;
					case '@':
						if(skb_temp->target[i*skb_temp->w+j]=='.') strcat(buffer,"x");
						else strcat(buffer,"a");
						break;
				}
			}
			strcat(buffer,"\n");

		}
		strcat(buffer,"[/soko]\n");
		
	}

	if(type==SCLTC_SOKOJAVA ){
		sprintf(s_buffer,"\n[sokojava=%d,%d]\n[param='level']",skb_temp->w*16, (skb_temp->h+1)*16);
		 
		//g_message("debug%d",100);
		strcat(buffer,s_buffer);
		for(i=0;i<skb_temp->h;i++){
			for(j=0;j<skb_temp->w;j++){
				switch(skb_temp->level[i*skb_temp->w+j]){
					case '#':
						strcat(buffer,"#");break;
					case '-':
					case '_':
						if(skb_temp->target[i*skb_temp->w+j]=='.') strcat(buffer,".");
						else strcat(buffer,"-");
						break;
					case '$':
						if(skb_temp->target[i*skb_temp->w+j]=='.') strcat(buffer,"*");
						else strcat(buffer,"$");
						break;
					case '@':
						if(skb_temp->target[i*skb_temp->w+j]=='.') strcat(buffer,"+");
						else strcat(buffer,"@");
						break;
				}
			}
			if(i!=skb_temp->h-1) strcat(buffer,"|");

		}
		strcat(buffer,"[/param]\n[/sokojava]\n");
	}

	//g_message("buffer: %d: %s",strlen(buffer),buffer);


	gtk_clipboard_set_text (clipboard, buffer, strlen(buffer));

	free(buffer); sokoban_free(skb_temp);
	return;
}

