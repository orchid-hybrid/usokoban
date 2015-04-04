#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <stdlib.h>

#include "sokoban.h"
#include "settings.h"
#include "solver.h"
#include "solution.h"


int debug=0;

char *paste=NULL;

GError *error = NULL;
GdkPixbuf *skin;

GtkWidget *window;
GtkWidget *menubar, *draw_area, *status;
GtkWidget *toolbar;

GtkClipboard *clipboard;

SOKOBAN *skb;
char *level_filename=NULL;
char *level_folder=NULL;

char skin_filename[1024];
char skin_files[4][1024];

int screen_width=600;
int screen_height=400;
int screen_maximized=0;

SOKOBAN *level_set[MAX_LEVELS];
int n_levels=0;
int c_level; //index of current level
int thread_flag=0;
int fullscreen=0;
//gchar data_path[1024]="/usr/share/games/usokoban.";
//do something before exit
/* Another callback */

static void window_state_callback( GtkWidget *widget,   GdkEventWindowState *event,        gpointer   data )
{
	if ((event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED)==0) {
		sokoban_resize_topwindow(skb, window, draw_area,skin);
		//gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
		screen_maximized=0;
	} else screen_maximized=1;
	return;
}

static void restart_level( GtkWidget *widget,   gpointer   data ){
	if(thread_flag==0){
		sokoban_restart(skb);sokoban_clear_hint(skb);
		sokoban_show(skb, draw_area, skin, SS_UPDATE); sokoban_show_status(skb,status); 
	}
	return;

}

static void toggle_fullscreen( GtkWidget *widget,   gpointer   data ){
	if(fullscreen)
		gtk_window_unfullscreen (GTK_WINDOW(window));
	else gtk_window_fullscreen (GTK_WINDOW(window));
	fullscreen=1-fullscreen;
	return;

}

static void undo_level( GtkWidget *widget,   gpointer   data ){
	if(thread_flag==0 && sokoban_undo(skb)){
		if(sokoban_clear_hint(skb)) sokoban_show(skb, draw_area, skin, SS_UPDATE); 
		else sokoban_show(skb, draw_area, skin, SS_FAST_UPDATE); 
		sokoban_show_status(skb,status); 
	}
	return;

}

static void redo_level( GtkWidget *widget,   gpointer   data ){
	if(thread_flag==0 && sokoban_redo(skb)) {
		sokoban_check(skb); 
		if(sokoban_clear_hint(skb)) sokoban_show(skb, draw_area, skin, SS_UPDATE); 
		else sokoban_show(skb, draw_area, skin, SS_FAST_UPDATE); 
		sokoban_show_status(skb,status); 
	}
	return;

}

static void prev_level( GtkWidget *widget,   gpointer   data ){
	if(thread_flag==0 && n_levels>1){
		if (c_level>0) c_level--; else c_level=n_levels-1;
		if(skb->mode==SKB_SOLVED) gtk_clipboard_set_text (clipboard, skb->solution, skb->solution_head);
		sokoban_sleep(skb);
		skb=level_set[c_level];
		sokoban_wakeup(skb);
		sokoban_resize_topwindow(skb, window, draw_area,skin);
		sokoban_show(skb, draw_area, skin, SS_NEW);	 
		sokoban_show_status(skb,status); 
		sokoban_show_title(skb,window,level_filename, n_levels,c_level);
	}
	return;
}

static void next_level( GtkWidget *widget,   gpointer   data ){
	if(thread_flag==0 && n_levels>1){
		if (c_level<n_levels-1) c_level++; else c_level=0;

		if(skb->mode==SKB_SOLVED) gtk_clipboard_set_text (clipboard, skb->solution, skb->solution_head);
		sokoban_sleep(skb);
		skb=level_set[c_level];
		sokoban_wakeup(skb);
		sokoban_resize_topwindow(skb, window, draw_area,skin);
		sokoban_show(skb, draw_area, skin, SS_NEW);	 
		sokoban_show_status(skb,status); 
		sokoban_show_title(skb,window,level_filename, n_levels,c_level);
	}
	return;
}


static void destroy( GtkWidget *widget,
                     gpointer   data )
{
	save_settings(window);
	sokoban_close_solution_db(); //close the database
    //g_message("%s","destroyed");
    gtk_main_quit ();
}



//multi-purpose auto_play thread: (1) path-finding (3) ...
gpointer  auto_play_thread  (gpointer data)
{

	gpointer retval=NULL;
   	while(thread_flag==1 && sokoban_redo(skb)){
		sokoban_check(skb);

		gdk_threads_enter();
		sokoban_show(skb, draw_area, skin, SS_FAST_UPDATE); 
		sokoban_show_status(skb,status);
		gdk_threads_leave();

	} 
	thread_flag=0;
	//g_message("%s","thread ends!");
	return retval;
}

// (2) paste solution
gpointer  auto_play_thread_2  (gpointer data)
{

	gpointer retval=NULL;
	int i;   	
	for(i=0;thread_flag==1 && i<strlen(paste);i++){
		switch(paste[i]){
			case 'u':
			case 'U': sokoban_move(skb,SM_UP); break;
			case 'd':
			case 'D': sokoban_move(skb,SM_DOWN); break;
			case 'l':
			case 'L': sokoban_move(skb,SM_LEFT); break;
			case 'r':
			case 'R': sokoban_move(skb,SM_RIGHT); break;
				

		}
		sokoban_check(skb);
		gdk_threads_enter();
		sokoban_show(skb, draw_area, skin, SS_FAST_UPDATE); 
		sokoban_show_status(skb,status);
		gdk_threads_leave();

	} 
	free(paste); paste=NULL;
	thread_flag=0;
	//g_message("%s","thread ends!");
	return retval;
}


void           clipboard_callback    (GtkClipboard *clipboard,  const gchar *text,      gpointer data)
{
	 //g_message(text);
	//g_message("%s","thread_created!");
	int i;
	paste=malloc(strlen(text)+1);
	strcpy(paste,text); // reserve a copy of clipboard first
	
	if(check_clipboard(paste)==CC_SOLUTION){
		thread_flag=1;
		g_thread_new ("Auto Play Thread 2", auto_play_thread_2, NULL);
	}
	else{ //paste levels from clipboard
		//if(strcmp(level_filename, "Clipboard levels")!=0)
		g_free(level_filename);
		level_filename = g_malloc(20);
		strcpy(level_filename, "Clipboard levels");

		//g_message("%s",level_filename);

		for(i=0;i<n_levels;i++) sokoban_free(level_set[i]);

		n_levels=sokoban_open_from_clipboard(level_set,paste);	

		if(n_levels>0){	
			c_level=0; 
			skb=level_set[0];
		
			sokoban_wakeup(skb);
			sokoban_resize_topwindow(skb, window, draw_area,skin);
			sokoban_show(skb, draw_area, skin, SS_NEW);	 
			sokoban_show_status(skb,status); 
			sokoban_show_title(skb,window,level_filename, n_levels,c_level);
		}
		
		free(paste);

	}
	return;
}

//callback function for mouse button release 
gboolean    mouse_callback    (GtkWidget   *widget,  GdkEventButton *event,   gpointer     user_data)
{
	int row,col;

	if(thread_flag==1) return 0;//during auto replay, no response to other movements

	if(skb->mode==SKB_SOLVED){
		if(n_levels>0){
			if (c_level<n_levels-1) c_level++; else c_level=0;

			gtk_clipboard_set_text (clipboard, skb->solution, skb->solution_head);//save solution to clipboard before leave
			sokoban_sleep(skb);
			skb=level_set[c_level];
			sokoban_wakeup(skb);
			sokoban_resize_topwindow(skb, window, draw_area,skin);
			sokoban_show(skb, draw_area, skin, SS_NEW);	 
			sokoban_show_status(skb,status); 
			sokoban_show_title(skb,window,level_filename, n_levels,c_level);
		}
	}

	//g_message("cooridnate: %d,%d", (int) event->x,(int) event->y);
	if(calculate_row_col(skb, widget, skin, (int) event->x,(int) event->y,&row,&col)){

		if(sokoban_click(skb,row,col)) {
			thread_flag=1;
			//g_message("%s","thread_created!");
			sokoban_show(skb, draw_area, skin, SS_UPDATE); 
			//g_thread_create (auto_play_thread,NULL,FALSE, NULL);
			g_thread_new ("Auto Play Thread", auto_play_thread,NULL);
		}
		else {
			//if(skb->bi!=0 ){
				//sokoban_box_push_to_hint(skb);
			//}

			sokoban_show(skb, draw_area, skin, SS_UPDATE); 

		}
		//skb->bi=4; skb->bj=6; if (sokoban_box_push_to(skb,row,col)) g_thread_create (auto_play_thread,NULL,FALSE, NULL);	
		/*if(sokoban_man_move_to(skb,row,col)){
			thread_flag=1;
			g_thread_create (auto_play_thread,NULL,FALSE, NULL);	
		} //*/
	}
	return 1;
}


//the draw_area Widget is pass to this function by 'user_data'
gboolean       key_press_callback(GtkWidget   *widget,  GdkEventKey *event,   gpointer     user_data)   
{


	GError *error = NULL;
		
	switch(event->keyval){
		case GDK_F7:

			sokoban_load_solution(skb, LS_MOVE);
			sokoban_show(skb, draw_area, skin, SS_NEW);	 
			sokoban_show_status(skb,status); 
		break;		
		case GDK_F8:

			sokoban_load_solution(skb, LS_PUSH);
			sokoban_show(skb, draw_area, skin, SS_NEW);
			sokoban_show_status(skb,status); 
		break;
		case GDK_F9: //solver

			sokoban_solve(skb);
			sokoban_show_solving_status(skb,status);
		break; 
		case GDK_F10:
			//if(sokoban_savitch(skb, 8)) printf("Solvable!\n");
			//else printf("Unsolvable!\n");
			//sokoban_levelset_to_js();
			//test2(level_set, n_levels);
			//g_message("%d\n", sokoban_check_boardsize_2(skb) );
			//test(skb);
			//if(sokoban_solve(skb)==1){
			//}
		break;


		////////////////////END//////////////////

		case GDK_Up:
			if(thread_flag==0 && skb->mode==SKB_UNSOLVE){			
				sokoban_move(skb,SM_UP);
				sokoban_check(skb);
				if(sokoban_clear_hint(skb)) sokoban_show(skb, user_data, skin, SS_UPDATE); 
				else sokoban_show(skb, user_data, skin, SS_FAST_UPDATE); 
				sokoban_show_status(skb,status); 
			}
		break;
		case GDK_Down: 
			if(thread_flag==0 && skb->mode==SKB_UNSOLVE){			
				sokoban_move(skb,SM_DOWN);
				sokoban_check(skb);
				if(sokoban_clear_hint(skb)) sokoban_show(skb, user_data, skin, SS_UPDATE); 
				else sokoban_show(skb, user_data, skin, SS_FAST_UPDATE); 
				sokoban_show_status(skb,status); 
			}
		break;
		case GDK_Left: 
			if((event->state & GDK_CONTROL_MASK) ){ 
				if(thread_flag==0 && n_levels>1){
					if (c_level>0) c_level--; else c_level=n_levels-1;
					if(skb->mode==SKB_SOLVED) gtk_clipboard_set_text (clipboard, skb->solution, skb->solution_head);
					sokoban_sleep(skb);
					skb=level_set[c_level];
					sokoban_wakeup(skb);
					sokoban_resize_topwindow(skb, window, draw_area,skin);
					sokoban_show(skb, draw_area, skin, SS_NEW);	 
					sokoban_show_status(skb,status); 
					sokoban_show_title(skb,window,level_filename, n_levels,c_level);
				}

			}
			else if(thread_flag==0 && skb->mode==SKB_UNSOLVE){			
				sokoban_move(skb,SM_LEFT);
				sokoban_check(skb);
				if(sokoban_clear_hint(skb)) sokoban_show(skb, user_data, skin, SS_UPDATE); 
				else sokoban_show(skb, user_data, skin, SS_FAST_UPDATE); 
				sokoban_show_status(skb,status); 
			}
		break;
		case GDK_Right: 
			if((event->state & GDK_CONTROL_MASK)){ //Ctrl+Right
				if(thread_flag==0 && n_levels>1){
					if (c_level<n_levels-1) c_level++; else c_level=0;

					if(skb->mode==SKB_SOLVED) gtk_clipboard_set_text (clipboard, skb->solution, skb->solution_head);
					sokoban_sleep(skb);
					skb=level_set[c_level];
					sokoban_wakeup(skb);
					sokoban_resize_topwindow(skb, window, draw_area,skin);
					sokoban_show(skb, draw_area, skin, SS_NEW);	 
					sokoban_show_status(skb,status); 
					sokoban_show_title(skb,window,level_filename, n_levels,c_level);
				}
			}
			else if(thread_flag==0 && skb->mode==SKB_UNSOLVE){			
				sokoban_move(skb,SM_RIGHT);
				sokoban_check(skb);
				if(sokoban_clear_hint(skb)) sokoban_show(skb, user_data, skin, SS_UPDATE); 
				else sokoban_show(skb, user_data, skin, SS_FAST_UPDATE); 
				sokoban_show_status(skb,status); 
			}
		break;

		case GDK_BackSpace: if(thread_flag==0 && sokoban_undo(skb)){
						if(sokoban_clear_hint(skb)) sokoban_show(skb, user_data, skin, SS_UPDATE); 
						else sokoban_show(skb, user_data, skin, SS_FAST_UPDATE); 
						sokoban_show_status(skb,status); 
					}
		break;
		case GDK_space: if(thread_flag==0 && sokoban_redo(skb)) {
						sokoban_check(skb); 
						if(sokoban_clear_hint(skb)) sokoban_show(skb, user_data, skin, SS_UPDATE); 
						else sokoban_show(skb, user_data, skin, SS_FAST_UPDATE); 
						sokoban_show_status(skb,status); 
					}
		break;
		case GDK_Escape: if(thread_flag==0){
				sokoban_restart(skb);sokoban_clear_hint(skb);
				 sokoban_show(skb, user_data, skin, SS_UPDATE); sokoban_show_status(skb,status); 
			}
		break;

		case GDK_F1: 
			strcpy(skin_filename,skin_files[0]);
			skin=gdk_pixbuf_new_from_file (skin_filename, &error); 
			sokoban_resize_topwindow(skb, window, draw_area,skin);
			sokoban_show(skb, user_data, skin, SS_NEW); break;
		case GDK_F2: 
			strcpy(skin_filename,skin_files[1]);
			skin=gdk_pixbuf_new_from_file (skin_filename, &error); 
			sokoban_resize_topwindow(skb, window, draw_area,skin);
			sokoban_show(skb, user_data, skin, SS_NEW); break;

		case GDK_F3: 
			strcpy(skin_filename,skin_files[2]);
			skin=gdk_pixbuf_new_from_file (skin_filename, &error); 
			sokoban_resize_topwindow(skb, window, draw_area,skin);
			sokoban_show(skb, user_data, skin, SS_NEW); break;
		case GDK_F4: 
			strcpy(skin_filename,skin_files[3]);
			skin=gdk_pixbuf_new_from_file (skin_filename, &error); 
			sokoban_resize_topwindow(skb, window, draw_area,skin);
			sokoban_show(skb, user_data, skin, SS_NEW); break;
		case GDK_F11:
			if(fullscreen)
				gtk_window_unfullscreen (GTK_WINDOW(window));
			else gtk_window_fullscreen (GTK_WINDOW(window));
			fullscreen=1-fullscreen;
			break;

		//case (GDK_Control_L | GDK_Right): g_message("ctrl+right"); break;
		case GDK_v:  //(ctrl+v) for pasting (partial) solutions
			if((event->state & GDK_CONTROL_MASK) && thread_flag==0){
				
				gtk_clipboard_request_text( clipboard,  clipboard_callback, NULL); 
			} 
		break;
		case GDK_c: // (ctrl+c) copy solution to clipboard
			if((event->state & GDK_CONTROL_MASK) && thread_flag==0){
				gtk_clipboard_set_text (clipboard, skb->solution, skb->solution_tail);
			}
		break;
		case GDK_p: // (ctrl+p) stop the auto replaying
			if((event->state & GDK_CONTROL_MASK) && thread_flag==1){
				thread_flag=0;
			}
		break;
		case GDK_l: // (ctrl+l) copy level
			if((event->state & GDK_CONTROL_MASK) && thread_flag==0){
				sokoban_copy_level_to_clipboard(skb,clipboard, SCLTC_WITH_SOLUTION);
			}
		break;
		case GDK_m: // (ctrl+m) copy level in mf8 format
			if((event->state & GDK_CONTROL_MASK) && thread_flag==0){
				sokoban_copy_level_to_clipboard(skb,clipboard, SCLTC_MF8);
			}
		break;
		case GDK_j: // (ctrl+m) copy level in mf8 format
			if((event->state & GDK_CONTROL_MASK) && thread_flag==0){
				sokoban_copy_level_to_clipboard(skb,clipboard, SCLTC_SOKOJAVA);
			}
		break;
		case GDK_n: // (ctrl+n) normalized level
			if((event->state & GDK_CONTROL_MASK) && thread_flag==0){
				sokoban_normalize(skb);
				sokoban_show(skb, user_data, skin, SS_NEW);
			}
		break;
		
		


	}


	return 1;
}




static void expose_callback( GtkWidget *widget,
                     gpointer   data )
{

	sokoban_show(skb, widget, skin, SS_NEW);

}


//
static void get_level_filename ()
//( GtkWidget *w,                        gpointer   data )
{
  
	GtkWidget *dialog;
	int i;
	dialog = gtk_file_chooser_dialog_new ("Open File",
					      NULL,   // can this be NULL ?
					      GTK_FILE_CHOOSER_ACTION_OPEN,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					      NULL);
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),level_folder);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT){
		g_free(level_filename);g_free(level_folder);
		
		level_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		level_folder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
	

		//g_message("%s\n",level_folder);

		for(i=0;i<n_levels;i++) sokoban_free(level_set[i]);

		n_levels=sokoban_open(level_set,level_filename);	

		if(n_levels>0){	
			c_level=0; 
			skb=level_set[0];
		
		}	
		else {	//if read level from file failed, load built-in level
			level_set[0]=sokoban_new_from_built_in();
			skb=level_set[0];
			strcpy(level_filename,"built-in"); 
			n_levels=1; c_level=0;

		}
		sokoban_wakeup(skb);
		sokoban_resize_topwindow(skb, window, draw_area,skin);
		sokoban_show(skb, draw_area, skin, SS_NEW);	 
		sokoban_show_status(skb,status); 
		sokoban_show_title(skb,window,level_filename, n_levels,c_level);

		

	}
	gtk_widget_destroy (dialog);

}

/* about */
static void about ( gpointer   callback_data,
                            guint      callback_action,
                            GtkWidget *menu_item )
{
     gchar *auth[] = { "anian<anianwu@gmail.com>", "Chao Yang<yangchao0710@gmail.com>","Julian Gilbey <jdg@debian.org>", "laizhufu (Level Designer)", "stopheart (Level Designer)", NULL }; 

	gtk_show_about_dialog (NULL,
				"authors", auth,
				"comments", "Sokoban is a puzzle game invented by Hiroyuki Imabayashi in 1981. \
USokoban is a GTK+ version of this classic game.",
                       "program-name", "USokoban",
                       "title", "About USokoban",
			"version", "0.0.13",
                          "copyright" , "Copyright (c) 2010-2012 anian, Chao YANG",
                               "website","http://sokoban.ws/usokoban/usokoban.php",
                       NULL);
}


/* Our menu, an array of GtkItemFactoryEntry structures that defines each menu item */
static GtkItemFactoryEntry menu_items[] = {
  { "/_Game",         NULL,         NULL,           0, "<Branch>" },
  { "/Game/_Open",    NULL, get_level_filename,    0, "<Item>" },
  { "/Game/_Quit",    NULL, destroy,            0, "<Item>" },
  { "/_Help",         NULL,         NULL,           0, "<Branch>" },
  { "/Help/_About",   NULL,         about,           0, "<Item>" },
};

static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

/* Returns a menubar widget made from the above menu */
static GtkWidget *get_menubar_menu( GtkWidget  *window )
{
  GtkItemFactory *item_factory;
  GtkAccelGroup *accel_group;

  /* Make an accelerator group (shortcut keys) */
  accel_group = gtk_accel_group_new ();

  /* Make an ItemFactory (that makes a menubar) */
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>",
                                       accel_group);

  /* This function generates the menu items. Pass the item factory,
     the number of items in the array, the array itself, and any
     callback data for the the menu items. */
  gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);

  /* Attach the new accelerator group to the window. */
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  /* Finally, return the actual menu bar created by the item factory. */
  return gtk_item_factory_get_widget (item_factory, "<main>");
}



/* Entry Point of the main program */
int main( int argc,
          char *argv[] )
{	
	GtkWidget *main_vbox;
	
	

	//thread stuff
	//g_thread_init(NULL);
	gdk_threads_init();
	gdk_threads_enter();

	/* Initialize GTK */
	gtk_init (&argc, &argv);


	//load settings; always initialise first in case some settings are
	//not made in the settings file
	init_settings();
	load_settings();



	/* Make a window */
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	//gtk_window_set_title (GTK_WINDOW(window), "USokoban");
	gtk_window_set_default_size (GTK_WINDOW(window),screen_width,screen_height);
	//gtk_widget_set_size_request (GTK_WIDGET(window), 200, 200);

	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	if ( screen_maximized) gtk_window_maximize(GTK_WINDOW(window));

	/* Make a vbox to put the three menus in */
	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 0);
	gtk_container_add (GTK_CONTAINER (window), main_vbox);

	/* */
	menubar = get_menubar_menu (window);
	draw_area=gtk_drawing_area_new ();
	status=gtk_statusbar_new();

	//skin=gdk_pixbuf_new_from_file ("/home/sokoban/workspace/test/skin.png", &error);

	//run_time_path=g_get_current_dir();
	//g_message("test:%s",run_time_path);
	
	//skin=gdk_pixbuf_new_from_file ("./sokobox.gif", &error);
	

	//load skin from settings
	//strcpy(skin_filename,skin_files[2]);
	//strcat(skin_filename,"/skin36.png");
	//g_message("%s",skin_filename);
	skin=gdk_pixbuf_new_from_file (skin_filename, &error); //

	//load levels from settings
	n_levels=sokoban_open(level_set,level_filename);	

	if(n_levels!=0) skb=level_set[c_level]; //c_level is read from settings
	else {	//if read level from file failed, load built-in level
		level_set[0]=sokoban_new_from_built_in();
		skb=level_set[0];
		strcpy(level_filename,"built-in"); 
		n_levels=1; c_level=0;


	}

	sokoban_wakeup(skb);

	//open solution database
	sokoban_open_solution_db();

	//Connect callbacks for window and draw_area
	g_signal_connect (G_OBJECT (draw_area), "expose_event",
	      G_CALLBACK (expose_callback), NULL);
        g_signal_connect (G_OBJECT (draw_area), "button_release_event",
	      G_CALLBACK (mouse_callback), NULL);
	g_signal_connect (G_OBJECT (window), "key_press_event",
	      G_CALLBACK (key_press_callback), draw_area); 
	g_signal_connect (window, "destroy",
		      G_CALLBACK (destroy), NULL);
	g_signal_connect (window, "window_state_event",
		      G_CALLBACK (window_state_callback), NULL);


	//g_signal_connect (G_OBJECT (window), "key_press_event",  G_CALLBACK (key_press_callback), draw_area);
	

	gtk_widget_set_events (draw_area,  GDK_EXPOSURE_MASK  | GDK_LEAVE_NOTIFY_MASK   | GDK_BUTTON_RELEASE_MASK
		          | GDK_BUTTON_PRESS_MASK   | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);  

	//
	toolbar=gtk_toolbar_new ();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	//gtk_toolbar_insert(toolbar, gtk_tool_button_new (NULL, "Previous"), -1);

	GtkToolItem *tt_open, *tt_exit, *tt_undo, *tt_redo ,*tt_restart, *tt_prev, *tt_next , *tt_sep, *tt_full;

	tt_open = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
  	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tt_open, -1);

	tt_prev = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
  	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tt_prev, -1);

	tt_next = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
  	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tt_next, -1);

	tt_full = gtk_tool_button_new_from_stock(GTK_STOCK_FULLSCREEN);
  	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tt_full, -1);

	tt_sep = gtk_separator_tool_item_new();
 	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tt_sep, -1);

	tt_restart = gtk_tool_button_new_from_stock(GTK_STOCK_REFRESH);
  	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tt_restart, -1);
	
	tt_undo = gtk_tool_button_new_from_stock(GTK_STOCK_UNDO);
  	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tt_undo, -1);

	tt_redo = gtk_tool_button_new_from_stock(GTK_STOCK_REDO);
  	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tt_redo, -1);

	tt_sep = gtk_separator_tool_item_new();
 	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tt_sep, -1);

	tt_exit = gtk_tool_button_new_from_stock(GTK_STOCK_QUIT);
  	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tt_exit, -1);

	

	
	//connect toolbar buttons
	g_signal_connect(G_OBJECT(tt_open), "clicked",  G_CALLBACK(get_level_filename), NULL);
	g_signal_connect(G_OBJECT(tt_exit), "clicked",  G_CALLBACK(destroy), NULL);
	g_signal_connect(G_OBJECT(tt_prev), "clicked",  G_CALLBACK(prev_level), NULL);
	g_signal_connect(G_OBJECT(tt_next), "clicked",  G_CALLBACK(next_level), NULL);
	g_signal_connect(G_OBJECT(tt_restart), "clicked",  G_CALLBACK(restart_level), NULL);
	g_signal_connect(G_OBJECT(tt_undo), "clicked",  G_CALLBACK(undo_level), NULL);
	g_signal_connect(G_OBJECT(tt_redo), "clicked",  G_CALLBACK(redo_level), NULL);
	g_signal_connect(G_OBJECT(tt_full), "clicked",  G_CALLBACK(toggle_fullscreen), NULL);


	//gtk_widget_grab_focus(draw_area);

	/* Pack it all together */
	gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (main_vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (main_vbox), draw_area, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (main_vbox), status, FALSE, TRUE, 0);

	sokoban_show_status(skb,status);
	sokoban_show_title(skb,window,level_filename, n_levels,c_level);

	//set up clipboard
	clipboard=gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

	/* Show the widgets */
	gtk_widget_show_all (window);

	/* Finished! */
	gtk_main ();
	gdk_threads_leave();
	return 0;
}
