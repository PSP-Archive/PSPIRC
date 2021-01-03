/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

# ifndef _PSP_CONSOLE_H_
# define _PSP_CONSOLE_H_

#ifdef __cplusplus
extern "C" {
#endif

# define CONSOLE_SCREEN_HEIGHT   25
# define CONSOLE_SCREEN_WIDTH    80
# define CONSOLE_MAX_INPUT      256

# if 0 //LUDO: EGA
# define COLOR_BLACK              0
# define COLOR_BLUE               1
# define COLOR_GREEN              2
# define COLOR_CYAN               3
# define COLOR_RED                4
# define COLOR_MAGENTA            5
# define COLOR_BROWN              6
# define COLOR_GRAY               7
# define COLOR_DARK_GRAY          8
# define COLOR_BRIGHT_BLUE        9
# define COLOR_BRIGHT_GREEN      10
# define COLOR_BRIGHT_CYAN       11
# define COLOR_BRIGHT_RED        12
# define COLOR_BRIGHT_MAGENTA    13
# define COLOR_YELLOW            14
# define COLOR_WHITE             15
# define COLOR_ALPHA             16
# define CONSOLE_MAX_COLOR       17
# endif

# define COLOR_NONE              -1
# define COLOR_WHITE              0
# define COLOR_BLACK              1
# define COLOR_DARK_BLUE          2
# define COLOR_GREEN              3
# define COLOR_RED                4
# define COLOR_BROWN              5
# define COLOR_MAGENTA            6
# define COLOR_ORANGE             7
# define COLOR_YELLOW             8
# define COLOR_BRIGHT_GREEN       9
# define COLOR_CYAN              10
# define COLOR_BRIGHT_BLUE       11
# define COLOR_BLUE              12
# define COLOR_PINK              13
# define COLOR_GRAY              14
# define COLOR_BRIGHT_GRAY       15
# define COLOR_ALPHA             16
# define CONSOLE_MAX_COLOR       17

# define COLOR_USER              17
# define COLOR_OTHER             18

# define CONSOLE_MAX_LINE      1024
# define CONSOLE_MAX_WIDTH       80

# define CONSOLE_TYPE_CHANNEL 0
# define CONSOLE_TYPE_PRIVATE 1
# define CONSOLE_TYPE_LOG     2

# define CONSOLE_MAX_HISTORY  10

  typedef struct console_elem_t {
     u8    m_char;
     u8    fg_color;
     u8    bg_color;
  } console_elem_t;

  typedef struct console_tab_t {

    char                  *name;
    struct console_tab_t  *next;
    struct console_tab_t  *prev;
    console_elem_t         screen[CONSOLE_MAX_LINE][CONSOLE_MAX_WIDTH];
            char                   input_buffer[CONSOLE_MAX_INPUT];
            char                   last_buffer[CONSOLE_MAX_HISTORY+1][CONSOLE_MAX_INPUT];
    int                    curr_history;
    int                    display_from_y;
    int                    curr_x;
    int                    curr_y;
    int                    top_y;
    int                    bottom_y;
    int                    input_x;
    int                    input_len;
    int                    input_first;
    int                    type;
    int                    unread;

  } console_tab_t;

  typedef struct console_tab_list {
    int              number_tab;
    console_tab_t**  tab_array;

  } console_tab_list;

  extern char* console_colors_name[CONSOLE_MAX_COLOR];
  extern int   console_colors[CONSOLE_MAX_COLOR];

  extern void psp_console_clear_screen();
  extern void psp_console_display_screen();
  extern void psp_console_init();

  extern void psp_console_set_current_tab(const char *tab);
  extern console_tab_t* psp_console_get_tab(const char* tab);
  extern void psp_console_del_tab(const char* tab);
  extern console_tab_t* psp_console_add_tab(const char* tab, int type);

  extern void psp_console_write_console(const char* buffer, int color);
  extern void psp_console_write_tab(const char* tab, const char* buffer, int color);
  extern void psp_console_write_channel(const char* channel, const char* buffer, int color);
  extern void psp_console_write_private(const char* who, const char* buffer, int color);
  extern void psp_console_write_current(const char* buffer, int color);

  extern char* psp_console_get_current_tab();
  extern char* psp_console_get_current_channel();
  extern void psp_console_tab_left();
  extern void psp_console_tab_right();
  extern void psp_console_tab_first();

  extern console_tab_list* psp_console_get_tabs();
  extern void psp_console_free_tabs(console_tab_list* tab_list);
  extern void psp_console_put_string(char *buffer);
  extern void psp_console_put_key(int key);

  extern void psp_console_close_all_tabs();

#ifdef __cplusplus
}
#endif

# endif
