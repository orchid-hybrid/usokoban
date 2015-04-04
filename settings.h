#ifndef SETTINGS_H
#define SETTINGS_H

#include <gtk/gtk.h>

extern char skin_filename[1024];
extern char skin_files[4][1024];
extern char *level_filename;
extern int c_level;
extern gchar *run_time_path;
extern int screen_width;
extern int screen_height;
extern int screen_maximized;
extern char *level_folder;

void save_settings(GtkWidget *top);
int load_settings();
void init_settings();

#endif
