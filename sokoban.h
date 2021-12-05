#ifndef SOKOBAN_H
#define SOKOBAN_H

#include <gtk/gtk.h>

#define LEVEL_SIZE          (5000)
#define SOLUTION_LENGTH     (10000)  // the size of a basic block of memory for solution
#define LINE_BUFFER         (1000)

#define MAX_LEVELS          (1000)   //support single level file containing at most 1000 levels

#define SBPT_MAX_SEARCH_POSITION   (4000)  //enough for levels upto size 63x63?
#define SBPT_MAX_STEP_LENGTH        (2000) // a step length of 2000 may be sufficient for level of size upto 75x75


#define SM_UP		('u')
#define SM_DOWN		('d')
#define SM_LEFT		('l')
#define SM_RIGHT   	('r')
#define SM_PUSH_UP		('U')
#define SM_PUSH_DOWN		('D')
#define SM_PUSH_LEFT		('L')
#define SM_PUSH_RIGHT   	('R')

/* These MUST be in this order, otherwise line_check will fail! */
enum { LC_BLANK, LC_SOLIDBOUNDARY, LC_BOUNDARY, LC_LEVEL,
       LC_HEADER, LC_COMMENT_START, LC_COMMENT_END, LC_OTHER };

enum { STATUS_NONLEVEL, STATUS_COMMENT, STATUS_TOP_BOUNDARY, STATUS_INLEVEL,
       STATUS_BOTTOM_BOUNDARY, STATUS_POSSIBLE_BOUNDARY, STATUS_ERROR };

#define SS_NEW              (1)
#define SS_UPDATE           (2)
#define SS_FAST_UPDATE            (3)

#define SKB_UNSOLVE             (1)
#define SKB_SOLVED              (2)

#define CC_LEVEL               (1)
#define CC_SOLUTION            (2)

#define SCLTC_LEVEL_ONLY		(0)
#define SCLTC_WITH_SOLUTION        (1)
#define SCLTC_MF8                  (2)
#define SCLTC_SOKOJAVA                  (3)

typedef struct
{
	char		*level; /* stores the level compoments @ - $ (man, empty square, box) */
	char		*target; /* stores the level component . (target square) */
	char 		*hint;
	int            level_len;
	int              w; /* width of the level [squares] */
	int              h; /* height of the level [squares] */


	int			pi;  /* row index of the pusher  */
	int			pj;  /* column index of the pusher */

	int	bi; /* row index for selected box */
	int    bj; /* column index for selected box */

	int	box_i; /* row index of last touched box */
	int	box_j; /* column index of last touched box */

	char		*solution; /* move sequence as a string of [urdlURDL] */
	int		solution_len;
	int			solution_head;  
	int			solution_tail;  // this is also the number of moves

	int              n_push;  // number of pushes
	int		n_boxchange; // number of box changes

	int              mode; //solved? unsolve?
	int		wall_click;
	unsigned long long   key; /* Zobrist key over the positions of the boxes. Pusher position, targets, walls and the general layout are ignored. */
	unsigned long long key2; /* Zobrist key over the positiions of the targets. Pusher position, boxes, walls and the general layout are ignored. */

}SOKOBAN;



SOKOBAN *sokoban_new();
void sokoban_free(SOKOBAN *skb);
SOKOBAN *sokoban_new_from_built_in();  
void sokoban_show(SOKOBAN *skb, GtkWidget *widget, GdkPixbuf *skin,  int type);
void sokoban_resize_topwindow(SOKOBAN *skb, GtkWidget *top,GtkWidget *draw_area,   GdkPixbuf *skin);
void sokoban_show_status(SOKOBAN *skb, GtkWidget *widget);
void sokoban_show_solving_status(SOKOBAN *skb, GtkWidget *widget);
void sokoban_show_title(SOKOBAN *skb, GtkWidget *window, char *filename,int total, int active_id);
int sokoban_move(SOKOBAN *skb, char direction);
int sokoban_undo(SOKOBAN *skb);
int sokoban_redo(SOKOBAN *skb);
int sokoban_open(SOKOBAN *level_set[MAX_LEVELS], char *filename);
int sokoban_open_from_clipboard(SOKOBAN *level_set[MAX_LEVELS], char *paste);
int sokoban_check(SOKOBAN *skb);
void sokoban_restart(SOKOBAN *skb);
int sokoban_man_move_to(SOKOBAN *skb, int row, int col);
int sokoban_man_move_to_2(SOKOBAN *skb, int row, int col);
int sokoban_box_push_to(SOKOBAN *skb, int row, int col);
void sokoban_copy(SOKOBAN *skb1, SOKOBAN *skb2);
void sokoban_copy_all(SOKOBAN *skb1, SOKOBAN *skb2); 
void sokoban_copy_with_change(SOKOBAN *skb1, SOKOBAN *skb2, int nbi, int nbj, int npi, int npj);
int sokoban_click(SOKOBAN *skb, int row, int col);
void sokoban_sleep(SOKOBAN *skb);
void sokoban_wakeup(SOKOBAN *skb);
int sokoban_unselect_box(SOKOBAN *skb);
void sokoban_box_push_to_hint(SOKOBAN *skb);
void sokoban_reachable_box_hint(SOKOBAN *skb);
int sokoban_clear_hint(SOKOBAN *skb);
void sokoban_levelset_to_js();

//sokoban2.c
void sokoban_normalize(SOKOBAN *skb); 
void sokoban_copy_level_to_clipboard(SOKOBAN *skb, GtkClipboard *clipboard, int type);


//------------------------
//faster copy function for sokoban_box_push_to()
void sbpt_load(SOKOBAN *skb , int nbi, int nbj, int npi, int npj);
void sbpt_unload(SOKOBAN *skb , int nbi, int nbj, int npi, int npj);

//three sub-routines for sokoban_open(...)
int line_check(char *line);
void append_line(SOKOBAN *skb, char *buffer, int *p, int *b, int *t);
void pad_level(SOKOBAN *skb);

//sub-routins for mouse control
int calculate_row_col(SOKOBAN *skb, GtkWidget *widget, GdkPixbuf *skin, int px, int py, int *row, int *col);

//
int check_clipboard(char *p);


#endif /* SOKOBAN_H */
