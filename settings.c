#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "settings.h"


void save_settings(GtkWidget *top){

	FILE *pf;
	char settings_file[1024];

	int i;
	const gchar *home_path=g_get_home_dir();

	strcpy(settings_file, home_path);
	strcat(settings_file,"/.usokoban");

	//g_message("%s",home_path);

	pf=fopen(settings_file,"w");

	//g_message("%s",home_path);

	fprintf(pf,"levelset=%s\n",level_filename);
	fprintf(pf,"no=%d\n",c_level+1);
	for(i=0;i<4;i++) fprintf(pf,"skin%d=%s\n",i+1,skin_files[i]);

	
	fprintf(pf,"skin.default=%s\n",skin_filename);

	fprintf(pf,"window.width=%d\n",top->allocation.width);
	fprintf(pf,"window.height=%d\n",top->allocation.height);
	fprintf(pf,"window.maximized=%d\n",screen_maximized);

	fprintf(pf,"levelfolder=%s\n", level_folder);

	fprintf(pf,"\n");
	fclose(pf);
	return;
}

void init_settings(){

	level_filename=g_malloc(1024);  level_folder=g_malloc(1024); 

	strcpy(level_filename, "/usr/share/games/usokoban"); strcat(level_filename,"/alphabet.txt");
	c_level=0;
	strcpy(skin_files[0],"/usr/share/games/usokoban/skin12.png");
	strcpy(skin_files[1],"/usr/share/games/usokoban/skin18.png");
	strcpy(skin_files[2],"/usr/share/games/usokoban/skin36.png");
	strcpy(skin_files[3],"/usr/share/games/usokoban/skin54.png");
	strcpy(skin_filename,"/usr/share/games/usokoban/skin36.png");
	strcpy(level_folder, "/usr/share/games/usokoban");
				
	return;

}

//return 0 if settings file not exist
int load_settings(){

	char settings_file[1024];
	FILE *pf;
	char buffer[1024];
	//int line=0;
	int i,j=0;

	const gchar *home_path=g_get_home_dir();

	
	
	strcpy(settings_file, home_path);
	strcat(settings_file,"/.usokoban");

	pf=fopen(settings_file,"r");

	if(pf==NULL) return 0;

	while(fgets(buffer,1024,pf)!=NULL){
 
			if(strncmp("levelset=", buffer, 9)==0) {
				//level_filename=g_malloc(1024); /* already alloc'd in init_settings()
				//strcpy(level_filename, home_path); strcat(level_filename,"/");
				strcpy(level_filename, buffer+9);
				if(level_filename[strlen(level_filename)-1]=='\n')
					level_filename[strlen(level_filename)-1]='\0';
				//g_message("%s",level_filename);
			}
			else if(strncmp("no=", buffer, 3)==0) {
				c_level=atoi(buffer+3)-1;
				//g_message("c_level=%d",c_level);
			}
			else if(strncmp("skin.default=", buffer, 13)==0) {
				strcpy(skin_filename, buffer+13);
				//g_message("skin_filename=%s",skin_filename);
			}
			else if(strncmp("skin", buffer, 4)==0) {
				if (strlen(buffer) > 6 && buffer[5] == '='
				    && isdigit(buffer[4])) {
					int sk;
					sk = atoi(buffer+4);
					if (sk >= 1 && sk <= 4)
						strcpy(skin_files[sk-1],buffer+6);
					//g_message("skin[%d]=%s",sk-1,skin_files[sk-1]);
				}
			}
			else if(strncmp("window.width=", buffer, 13)==0) {
				screen_width=atoi(buffer+13);
				//g_message("width=%d",screen_width);
			}
			else if(strncmp("window.height=", buffer, 14)==0) {
				screen_height=atoi(buffer+14);
				//g_message("height=%d",screen_height);
			}
			else if(strncmp("window.maximized=", buffer, 17)==0) {
				screen_maximized=atoi(buffer+17);
				//g_message("max=%d",screen_maximized);
			}
			else if(strncmp("levelfolder=", buffer, 12)==0) {
				level_folder=g_malloc(1024);
				strcpy(level_folder,buffer+12);
				//g_message("level_folder=%s",level_folder);
		}
		
		//g_message("%s",buffer);
		

	} //end of file (while)

	for(i=0;i<4;i++){
		if(skin_files[i][strlen(skin_files[i])-1]=='\n') skin_files[i][strlen(skin_files[i])-1]='\0';
		 //g_message("%s",skin_files[i]);

	}	
 	if(skin_filename[strlen(skin_filename)-1]=='\n')
 		skin_filename[strlen(skin_filename)-1]='\0';

 	if(level_folder[strlen(level_folder)-1]=='\n')
 		level_folder[strlen(level_folder)-1]='\0';

	fclose(pf);

	return 1;
}

