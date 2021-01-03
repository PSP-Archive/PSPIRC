/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Based on the source code of PelDet written by Danzel A.
*/

#include <pspkernel.h>
#include <psppower.h>
#include <pspctrl.h>
#include <pspwlan.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL.h>

#include "psp_global.h"
#include "psp_sdl.h"
#include "psp_danzeff.h"
#include "psp_menu.h"
#include "psp_fmgr.h"
#include "psp_irc.h"
#include "irc_global.h"
#include "irc_thread.h"

#include "psp_console.h"
#include "psp_login.h"

# define false      0
# define true       1

#define CONSOLE_DEFAULT_TOP_Y     0
#define CONSOLE_DEFAULT_BOTTOM_Y  (CONSOLE_SCREEN_HEIGHT-1)

# define BRIGHTNESS_NORMAL  0
# define BRIGHTNESS_BRIGHT  1
# define BRIGHTNESS_DIM     2

# define FONT_DEFAULT       0
# define FONT_LINEDRAW      1

# define CONSOLE_SCREEN_TOP_Y  (CONSOLE_MAX_LINE - CONSOLE_SCREEN_HEIGHT)

  static console_tab_t *loc_tab_list = NULL;
  static console_tab_t *loc_curr_tab = NULL;

  volatile SceUID consoleSema;

  int console_colors[CONSOLE_MAX_COLOR];

  char* console_colors_name[CONSOLE_MAX_COLOR] = {
     "white" ,
     "black" ,
     "dark blue" ,
     "green" ,
     "red" ,
     "brown" ,
     "magenta" ,
     "orange" ,
     "yellow" ,
     "bright green" ,
     "cyan" ,
     "bright blue" ,
     "blue" ,
     "pink" ,
     "gray" ,
     "bright gray" ,
     "alpha"
  };

static void
psp_console_clear_line(console_tab_t* my_tab, int y)
{
  int x;
  for (x = 0; x < CONSOLE_SCREEN_WIDTH; x++) {
    console_elem_t *elem = &my_tab->screen[CONSOLE_SCREEN_TOP_Y + y][x];
    elem->m_char = ' ';
    elem->fg_color = COLOR_WHITE;
    elem->bg_color = COLOR_ALPHA;
  }
}


static void
psp_console_shift_up(console_tab_t* my_tab)
{
  int y;
  if (my_tab->top_y == 0) {  //if we're scrolling off the top of the screen then scroll the back buffer
    for (y = 0; y <= (CONSOLE_MAX_LINE-1-CONSOLE_SCREEN_HEIGHT); y++) {
      console_elem_t *scan_line1 = &my_tab->screen[y    ][0];
      console_elem_t *scan_line2 = &my_tab->screen[y + 1][0];
      memcpy(scan_line1, scan_line2, sizeof(console_elem_t) * CONSOLE_MAX_WIDTH);
    }
  }
  //now scroll the scroll area
  for (y = my_tab->top_y; y < my_tab->bottom_y; y++) {
    console_elem_t *scan_line1 = &my_tab->screen[CONSOLE_SCREEN_TOP_Y + y    ][0];
    console_elem_t *scan_line2 = &my_tab->screen[CONSOLE_SCREEN_TOP_Y + y + 1][0];
    memcpy(scan_line1, scan_line2, sizeof(console_elem_t) * CONSOLE_MAX_WIDTH);
  }
  psp_console_clear_line(my_tab, y);
  my_tab->curr_y--;
}

static void
psp_console_shift_up_if_needed(console_tab_t* my_tab)
{
  if (my_tab->curr_y > my_tab->bottom_y) {
    psp_console_shift_up(my_tab);
  }
}

static void
psp_console_clear_tab(console_tab_t* my_tab)
{
  int x;
  int y;
  for (y = 0; y < CONSOLE_MAX_LINE; y++) {
    for (x = 0; x < CONSOLE_MAX_WIDTH; x++) {
      console_elem_t *elem = &my_tab->screen[y][x];
      elem->m_char = ' ';
      elem->fg_color = COLOR_WHITE;
      elem->bg_color = COLOR_ALPHA;
    }
  }
  my_tab->curr_x = 0;
  my_tab->curr_y = 0;

  my_tab->display_from_y = CONSOLE_SCREEN_TOP_Y;
  my_tab->top_y          = CONSOLE_DEFAULT_TOP_Y;
  my_tab->bottom_y       = CONSOLE_DEFAULT_BOTTOM_Y;
  my_tab->curr_history   = CONSOLE_MAX_HISTORY;

  /* we want to start at bottom */
  if (PSPIRC.irc_start_bottom) {
    my_tab->curr_y = my_tab->bottom_y;
  }
}

void
psp_console_clear_screen()
{
  int x;
  int y;

  sceKernelWaitSema(consoleSema, 1, 0);

  for (y = 0; y < CONSOLE_SCREEN_HEIGHT; y++) {
    for (x = 0; x < CONSOLE_SCREEN_WIDTH; x++) {
      console_elem_t *elem = &loc_curr_tab->screen[CONSOLE_SCREEN_TOP_Y + y][x];
      elem->m_char = ' ';
      elem->fg_color = COLOR_WHITE;
      elem->bg_color = COLOR_ALPHA;
    }
  }
  loc_curr_tab->curr_x = 0;
  loc_curr_tab->curr_y = 0;

  sceKernelSignalSema(consoleSema, 1);
}

#define HASH_MULT 314159
#define HASH_PRIME 516595003

static int
psp_console_get_nick_color(const char* buffer)
{
  int color = COLOR_BRIGHT_GRAY;
  const char *nick_end = strchr(buffer, ':');
  if (! nick_end) return color;

  const char *scan = buffer;
  while (scan != nick_end) {
    color += (color ^ (color >> 1)) + HASH_MULT * (unsigned char)*scan++; \
    while (color >= HASH_PRIME) {
      color -= HASH_PRIME;
    }
  }
  color %= (CONSOLE_MAX_COLOR-1);
  return color;
}

static void 
psp_console_write(console_tab_t* my_tab, const char* buffer, int color)
{
  sceKernelWaitSema(consoleSema, 1, 0);

  int color_mode = 0;
  int color_digit  = 0;
  int utf8_mode = 0;
  int index;

  if (color == COLOR_USER) {
    if (PSPIRC.irc_user_color != COLOR_NONE) color = PSPIRC.irc_user_color;
    else                                     color = COLOR_WHITE;
  } else 
  if (color == COLOR_OTHER) {
    if (PSPIRC.irc_other_color != COLOR_NONE) color = PSPIRC.irc_other_color;
    else
    if (! PSPIRC.irc_multi_color)             color = COLOR_BRIGHT_GRAY;
    else {
      color = psp_console_get_nick_color(buffer);
    }
  }

  if (my_tab != loc_curr_tab) {
    my_tab->unread = 1;
  } else {
    my_tab->unread = 0;
  }

  int user_color_spec[2] = { 0, 0 };
  int user_color[2];

  for (index = 0; buffer[index] != '\0'; index++) {
    u8 c = buffer[index];

    if (color_mode) {
      if ((c >= '0') && (c <= '9')) {

        if (color_digit == 0) {
          user_color[color_mode-1] = (c - '0');
        } else {
          user_color[color_mode-1] = (user_color[color_mode-1] * 10) + (c - '0');
        }
        user_color[color_mode-1] = (user_color[color_mode-1] % (CONSOLE_MAX_COLOR-1));
        user_color_spec[color_mode-1] = 1;
        color_digit++;

      } else
      if (c == ',') {
        color_mode  = 2;
        color_digit = 0;
      } else {
        color_mode = 0;
      }
      if (color_digit > 2) {
        color_mode = 0;
      }
      if (color_mode) continue;
    } else 
    if (utf8_mode) {
      c = psp_convert_utf8_to_iso_8859_1(utf8_mode, c); 
      utf8_mode = 0;

    } else
    if ((c == 0xc2) || (c == 0xc3)) {
      u8 next_c = buffer[index+1];
      if ((next_c >= 0xa0) && (next_c <= 0xbf)) {
        /* Hack for simple utf8 char convertion to iso_8859-1 */
        utf8_mode = c;
        continue;
      }
    }

    if (c == '\n') {
      my_tab->curr_y++;
      my_tab->curr_x = 0;
      psp_console_shift_up_if_needed(my_tab);
      color_mode = 0;
      user_color_spec[0] = 0;
      user_color_spec[1] = 0;
    } else 
    if (c == '\r') {
      my_tab->curr_x = 0;
      color_mode = 0;
      user_color_spec[0] = 0;
      user_color_spec[1] = 0;
    } else 
    if (c == '\003') {
      color_mode = 1;
      color_digit = 0;
      user_color[0] = color;
      user_color[1] = COLOR_ALPHA;
      user_color_spec[0] = 0;
      user_color_spec[1] = 0;
    } else {
      if (my_tab->curr_x >= CONSOLE_SCREEN_WIDTH) {
        my_tab->curr_y++;
        my_tab->curr_x = 0;
        psp_console_shift_up_if_needed(my_tab);
      }
      console_elem_t *elem = &my_tab->screen[CONSOLE_SCREEN_TOP_Y + my_tab->curr_y][my_tab->curr_x];
      elem->m_char = c;
      if (user_color_spec[0]) {
        elem->fg_color = user_color[0];
      } else {
        elem->fg_color = color;
      }
      if (user_color_spec[1]) {
        elem->bg_color = user_color[1];
      } else {
        elem->bg_color = COLOR_ALPHA;
      }
      my_tab->curr_x++;
    }
  }

  sceKernelSignalSema(consoleSema, 1);
}

static void
psp_console_write_stamp(console_tab_t* my_tab, int color)
{
  if (my_tab->curr_x == 0) {
    time_t my_time;
    struct tm *my_tm;
    char my_time_str[32];
    time(&my_time);
    my_tm = localtime(&my_time);
    strftime(my_time_str, sizeof(my_time_str), "[%H:%M] ", my_tm);
    psp_console_write(my_tab, my_time_str, color);
  }
}

void 
psp_console_write_current(const char* buffer, int color)
{
  if (loc_curr_tab) {
    if (PSPIRC.irc_time_stamp) {
      psp_console_write_stamp(loc_curr_tab, color);
    }
    psp_console_write(loc_curr_tab, buffer, color);
  }
}

console_tab_t*
psp_console_get_tab(const char* tab)
{
  console_tab_t* scan_tab;
  for (scan_tab = loc_tab_list; scan_tab != 0; scan_tab = scan_tab->next) {
    if (! strcmp(scan_tab->name, tab)) return scan_tab;
  }
  return 0;
}

void
psp_console_write_tab(const char* tab, const char* buffer, int color)
{
  console_tab_t* my_tab = psp_console_get_tab(tab);
  if (my_tab) {
    if (PSPIRC.irc_time_stamp) {
      psp_console_write_stamp(my_tab, color);
    }
    psp_console_write(my_tab, buffer, color);
  }
}

void
psp_console_write_channel(const char* channel, const char* buffer, int color)
{
  console_tab_t* my_tab = psp_console_get_tab(channel);
  if (my_tab) {
    if (my_tab->type == CONSOLE_TYPE_CHANNEL) {
      if (PSPIRC.irc_time_stamp) {
        psp_console_write_stamp(my_tab, color);
      }
      psp_console_write(my_tab, buffer, color);
    }
  }
}

static console_tab_t* 
loc_psp_console_add_tab(const char *tab, int type)
{
  console_tab_t* new_tab = (console_tab_t *)malloc( sizeof(console_tab_t) );
  memset(new_tab, 0, sizeof( console_tab_t ) );

  psp_console_clear_tab(new_tab);

  console_tab_t*  scan_tab;
  console_tab_t*  last_tab = NULL;
  console_tab_t** prev_tab = &loc_tab_list;

  for (scan_tab = loc_tab_list; scan_tab != 0; scan_tab = scan_tab->next) {
    prev_tab = &scan_tab->next;
    last_tab = scan_tab;
  }
  *prev_tab = new_tab;
  new_tab->prev = last_tab;

  new_tab->type = type;
  new_tab->name = strdup(tab);

  if (loc_curr_tab) {
    loc_curr_tab->unread = 0;
  }
  loc_curr_tab = new_tab;

  return new_tab;
}


void
psp_console_write_private(const char* who, const char* buffer, int color)
{
  console_tab_t* private_tab = psp_console_get_tab(who);
  if (! private_tab) {
    private_tab = psp_console_add_tab(who, CONSOLE_TYPE_PRIVATE);
  }
  if (private_tab) {
    if (private_tab->type == CONSOLE_TYPE_PRIVATE) {
      if (PSPIRC.irc_time_stamp) {
        psp_console_write_stamp(private_tab, color);
      }
      psp_console_write(private_tab, buffer, color);
    }
  }
}

void
psp_console_open_private(const char* who)
{
  if (loc_curr_tab) {
    loc_curr_tab->unread = 0;
  }
  loc_curr_tab = psp_console_get_tab(who);
  if (! loc_curr_tab) {
    loc_curr_tab = psp_console_add_tab(who, CONSOLE_TYPE_PRIVATE);
  }
}

void
psp_console_write_console(const char* buffer, int color)
{
  if (loc_tab_list) {
    if (PSPIRC.irc_time_stamp) {
      psp_console_write_stamp(loc_tab_list, color);
    }
    psp_console_write(loc_tab_list, buffer, color);
  }
}

void
psp_console_screen_scroll_up()
{
  sceKernelWaitSema(consoleSema, 1, 0);

  loc_curr_tab->display_from_y -= CONSOLE_SCREEN_HEIGHT / 2;
  if (loc_curr_tab->display_from_y < 0) loc_curr_tab->display_from_y = 0;

  sceKernelSignalSema(consoleSema, 1);
}

void
psp_console_screen_scroll_down()
{
  sceKernelWaitSema(consoleSema, 1, 0);

  loc_curr_tab->display_from_y += CONSOLE_SCREEN_HEIGHT / 2;
  if (loc_curr_tab->display_from_y > CONSOLE_SCREEN_TOP_Y) loc_curr_tab->display_from_y = CONSOLE_SCREEN_TOP_Y;

  sceKernelSignalSema(consoleSema, 1);
}

void
psp_console_display_screen()
{
  sceKernelWaitSema(consoleSema, 1, 0);

  psp_sdl_blit_console();

# if 0 //LUDO: FOR_DEBUG
  {
    char buffer[32];
    sprintf(buffer, "(%d %d)", loc_curr_tab->curr_x, loc_curr_tab->curr_y);
    int color = psp_sdl_rgb(0xff, 0xff, 0xff);
    psp_sdl_back_print(40, 6, buffer, color);
  }
# endif

  //psp_sdl_select_font_8x8();
  psp_sdl_select_font_6x10();

  int line = 0;
  int y_pix = 0;
  int x_pix = 0;

  int first_line = loc_curr_tab->display_from_y;
  int last_line  = loc_curr_tab->display_from_y + CONSOLE_SCREEN_HEIGHT;

  if (last_line >= CONSOLE_MAX_LINE) {
    first_line = CONSOLE_SCREEN_TOP_Y;
    last_line  = CONSOLE_SCREEN_TOP_Y + CONSOLE_SCREEN_HEIGHT - 1;
  }

  for (line = first_line; line <= last_line; line++) {
    console_elem_t *scan_line = &loc_curr_tab->screen[line][0];
    x_pix = 0;
    int x;
    for (x = 0; x < CONSOLE_SCREEN_WIDTH; x++) {
      int fg_color = console_colors[scan_line[x].fg_color];
      u8 m_char = scan_line[x].m_char;
      if (m_char < ' ') m_char = ' ';
      if (scan_line[x].bg_color == COLOR_ALPHA) {
        psp_sdl_back_put_char(x_pix, y_pix, fg_color, m_char);
      } else {
        int bg_color = console_colors[scan_line[x].bg_color];
        psp_sdl_put_char(x_pix, y_pix, fg_color, bg_color, m_char, 1, 1);
      }
      x_pix += psp_font_width;
    }
    y_pix += psp_font_height;
  }

  y_pix = psp_font_height * CONSOLE_SCREEN_HEIGHT;
  x_pix = 0;
  int x = 0;
  char *scan_str = loc_curr_tab->input_buffer;
# if 0
  int length = strlen(scan_str);
  int first = 0;
  if (length > (CONSOLE_SCREEN_WIDTH-1)) {
    first = length - (CONSOLE_SCREEN_WIDTH-1);
  }
# endif
  int user_color = COLOR_WHITE;
  if (PSPIRC.irc_user_color != COLOR_NONE) {
    user_color = PSPIRC.irc_user_color;
  }
  for (x = 0; x < CONSOLE_SCREEN_WIDTH; x++) {
    u8 m_char;
    if ((loc_curr_tab->input_first + x) >= loc_curr_tab->input_len) m_char = ' ';
    else  m_char = scan_str[loc_curr_tab->input_first + x];

    psp_sdl_back_put_char(x_pix, y_pix, console_colors[user_color], m_char);
    x_pix += psp_font_width;
  }

  /* Display input cursor */
  x_pix = psp_font_width  * (loc_curr_tab->input_x - loc_curr_tab->input_first);
  psp_sdl_fill_rectangle(x_pix, y_pix, 0, psp_font_height,  console_colors[user_color], 0);

  psp_sdl_select_font_6x10();

  /* Display tab name */
  y_pix = 272 - psp_font_height;
  x_pix = 0;
  if (loc_curr_tab) {
    char* chan_name = loc_curr_tab->name;
    if (loc_curr_tab == loc_tab_list) {
      chan_name = "console";
    }
    int color = COLOR_CYAN;
    if (loc_curr_tab->unread) color = COLOR_RED;
    psp_sdl_back_print(x_pix, y_pix, chan_name, console_colors[color]);
  }

  /* Display small help on hotkeys */
  psp_sdl_back_print(150, y_pix, "<-Lt Rt-> Sel-Menu []-Chan X-Enter O-Users Tr-Tabs ", console_colors[COLOR_CYAN]);

  psp_sdl_select_font_8x8();

  sceKernelSignalSema(consoleSema, 1);
}

console_tab_t* 
psp_console_add_tab(const char *tab, int type)
{
  sceKernelWaitSema(consoleSema, 1, 0);

  console_tab_t* new_tab = loc_psp_console_add_tab(tab, type);

  sceKernelSignalSema(consoleSema, 1);

  return new_tab;
}

static void
loc_psp_console_del_tab(const char* tab)
{
  console_tab_t*  scan_tab;
  console_tab_t** prev_tab;

  scan_tab  = loc_tab_list;
  prev_tab = &loc_tab_list;

  while (scan_tab && (strcmp(tab, scan_tab->name))) {
    prev_tab = &scan_tab->next;
    scan_tab = scan_tab->next;
  }
  if (! scan_tab) return;

  if (loc_curr_tab == scan_tab) {
    if (scan_tab->next) {
      loc_curr_tab = scan_tab->next;
    } else 
    if (scan_tab->prev) {
      loc_curr_tab = scan_tab->prev;
    } else {
      loc_curr_tab = 0;
    }
  }

  *prev_tab = scan_tab->next;
  if (scan_tab->next) {
    scan_tab->next->prev = scan_tab->prev;
  }

  free(scan_tab->name);
  free(scan_tab);
}

void
psp_console_del_tab(const char* tab)
{
  sceKernelWaitSema(consoleSema, 1, 0);

  loc_psp_console_del_tab(tab);

  sceKernelSignalSema(consoleSema, 1);
}

void
psp_console_close_tab(const char* tab_name)
{
  console_tab_t* del_tab = psp_console_get_tab( tab_name );
  if (del_tab) {
    if (del_tab->type == CONSOLE_TYPE_PRIVATE) {
      psp_console_del_tab( tab_name );
    } else 
    if (del_tab->type == CONSOLE_TYPE_CHANNEL) {
      char buffer[256];
      sprintf(buffer, "/PART %s", tab_name);
      irc_thread_send_message(tab_name, buffer);
    }
  }
}

void
psp_console_close_all_tabs()
{
  console_tab_t* scan_tab = loc_tab_list->next;

  while (scan_tab != NULL) {
    if (scan_tab->type == CONSOLE_TYPE_CHANNEL) {
      char buffer[256];
      sprintf(buffer, "/PART %s", scan_tab->name);
      irc_thread_send_message(scan_tab->name, buffer);
    }
    scan_tab = scan_tab->next;
  }
}

void
psp_console_tab_left()
{
  sceKernelWaitSema(consoleSema, 1, 0);

  if (loc_curr_tab) {
    loc_curr_tab->unread = 0;
  }
  loc_curr_tab = loc_curr_tab->prev;
  if (loc_curr_tab == NULL) {
    loc_curr_tab = loc_tab_list;
  }

  sceKernelSignalSema(consoleSema, 1);
}

void
psp_console_tab_right()
{
  sceKernelWaitSema(consoleSema, 1, 0);

  if (loc_curr_tab) {
    loc_curr_tab->unread = 0;
  }
  if (loc_curr_tab->next != NULL) {
    loc_curr_tab = loc_curr_tab->next;
  }

  sceKernelSignalSema(consoleSema, 1);
}

void
psp_console_tab_first()
{
  sceKernelWaitSema(consoleSema, 1, 0);

  if (loc_curr_tab) {
    loc_curr_tab->unread = 0;
  }
  loc_curr_tab = loc_tab_list;

  sceKernelSignalSema(consoleSema, 1);
  
}

void
psp_console_set_current_tab(const char *tab)
{
  console_tab_t* scan_tab = psp_console_get_tab(tab);
  if (scan_tab) {
    if (loc_curr_tab) {
      loc_curr_tab->unread = 0;
    }
    loc_curr_tab = scan_tab;
  }
}

char*
psp_console_get_current_tab()
{
  if (loc_curr_tab) {
    char* chan_name = loc_curr_tab->name;
    if (loc_curr_tab != loc_tab_list) {
      return chan_name;
    }
  }
  return 0;
}

char*
psp_console_get_current_channel()
{
  if (loc_curr_tab) {
    if (loc_curr_tab->type == CONSOLE_TYPE_CHANNEL) {
      char* chan_name = loc_curr_tab->name;
      return chan_name;
    }
  }
  return 0;
}

void
psp_console_init()
{
  console_colors[COLOR_WHITE         ] = psp_sdl_rgb(255, 255, 255);
  console_colors[COLOR_BLACK         ] = psp_sdl_rgb(0, 0, 0);
  console_colors[COLOR_DARK_BLUE     ] = psp_sdl_rgb(0, 0, 0x55);
  console_colors[COLOR_GREEN         ] = psp_sdl_rgb(0, 0xAA, 0);
  console_colors[COLOR_RED           ] = psp_sdl_rgb(0xAA, 0, 0);
  console_colors[COLOR_BROWN         ] = psp_sdl_rgb(0xAA, 0x55, 0);
  console_colors[COLOR_MAGENTA       ] = psp_sdl_rgb(0xAA, 0, 0xAA);
  console_colors[COLOR_ORANGE        ] = psp_sdl_rgb(255, 0xAA, 0);
  console_colors[COLOR_YELLOW        ] = psp_sdl_rgb(255, 255, 0x55);
  console_colors[COLOR_BRIGHT_GREEN  ] = psp_sdl_rgb(0, 255, 0);
  console_colors[COLOR_CYAN          ] = psp_sdl_rgb(0, 0xAA, 0xAA);
  console_colors[COLOR_BRIGHT_BLUE   ] = psp_sdl_rgb(0x55, 0x55, 255);
  console_colors[COLOR_BLUE          ] = psp_sdl_rgb(0, 0, 0xAA);
  console_colors[COLOR_PINK          ] = psp_sdl_rgb(255, 0, 255);
  console_colors[COLOR_GRAY          ] = psp_sdl_rgb(0x55, 0x55, 0x55);
  console_colors[COLOR_BRIGHT_GRAY   ] = psp_sdl_rgb(0xAA, 0xAA, 0xAA);

  loc_psp_console_add_tab("", CONSOLE_TYPE_LOG);

  //Semaphore
  if (! consoleSema) {
    consoleSema = sceKernelCreateSema("console", 0, 1, 1, NULL);
  }
}

void
psp_console_destroy()
{
  while (loc_tab_list != NULL) {
    psp_console_del_tab(loc_tab_list->name);
  }
}

void
psp_console_save_log(void)
{
  sceKernelWaitSema(consoleSema, 1, 0);

  FILE* File;
  char  TmpFileName[MAX_PATH];
  int   y = 0;
  int   x = 0;
  int   x_max = 0;
  u8    v = 0;

  sprintf(TmpFileName,"%s/log/log_%s_%d.txt", PSPIRC.psp_homedir, loc_curr_tab->name, PSPIRC.psp_log_id++);
  if (PSPIRC.psp_log_id >= 100) PSPIRC.psp_log_id = 0;
  File = fopen(TmpFileName, "w");
  if (File) {
    /* Find first non empty line */
    for (y = 0; y < CONSOLE_MAX_LINE; y++) {
      for (x = 0; x < CONSOLE_SCREEN_WIDTH; x++) {
        console_elem_t *elem = &loc_curr_tab->screen[y][x];
        if (elem->m_char > ' ') break;
      }
      if (x != CONSOLE_SCREEN_WIDTH) break;
    }
    while (y < CONSOLE_MAX_LINE) {
      /* Find last non empty column */
      x_max = CONSOLE_SCREEN_WIDTH;
      do {
        x_max--;
        v = loc_curr_tab->screen[y][x_max].m_char;
      } while ((x_max > 0) && (v <= ' '));
      
      for (x = 0; x <= x_max; x++) {
        v = loc_curr_tab->screen[y][x].m_char;
        if (v < ' ') v = ' ';
        fputc(v, File);
      }
      fputc('\n', File);
      y++;
    }
    fclose(File);
  }
  sceKernelSignalSema(consoleSema, 1);
}

void
psp_console_update_first()
{
  if (loc_curr_tab) {
    int max_width = (CONSOLE_SCREEN_WIDTH-1);
    if ((loc_curr_tab->input_x - loc_curr_tab->input_first) > max_width) {
       loc_curr_tab->input_first = loc_curr_tab->input_x - max_width;
    }
    if (loc_curr_tab->input_x < loc_curr_tab->input_first) {
      loc_curr_tab->input_first = loc_curr_tab->input_x - (max_width / 2);
      if (loc_curr_tab->input_first < 0) loc_curr_tab->input_first = 0;
    }
  }
}

void
psp_console_update_first_eol()
{
  if (loc_curr_tab) {
    int max_width = (CONSOLE_SCREEN_WIDTH-1);
    if (loc_curr_tab->input_len <= (max_width / 2)) {
      loc_curr_tab->input_first = 0;
    } else {
      loc_curr_tab->input_first = loc_curr_tab->input_len - (max_width / 2);
    }
  }
}

void
psp_console_put_key(int key)
{
  char *scan_str   = NULL;
  int   max_length = 0;
  int   index = 0;

  scan_str = loc_curr_tab->input_buffer;
  max_length = CONSOLE_MAX_INPUT - 1;

  if ((scan_str[0]           == '/') &&
      (loc_curr_tab->input_x ==   1)) {
    if (key == DANZEFF_LEFT) {
      psp_command_go_left();
    } else
    if (key == DANZEFF_RIGHT) {
      psp_command_go_right();
    }
    if ((key == DANZEFF_LEFT) || (key == DANZEFF_RIGHT)) {
      char *curr_command = psp_get_current_command();
      if (curr_command) {
        strcpy(scan_str + 1, curr_command);
        loc_curr_tab->input_len = strlen(scan_str);
        loc_curr_tab->input_x = 1;
        loc_curr_tab->input_first = 0;
        return;
      }
    } else 
    if (key == ' ') {
      loc_curr_tab->input_len = strlen(scan_str);
      loc_curr_tab->input_x = loc_curr_tab->input_len;
      psp_console_update_first();
      return;
    }
  }
  if (key == DANZEFF_BEGEND) {
    if (loc_curr_tab->input_x == 0) key = DANZEFF_END;
    else                            key = DANZEFF_HOME;
  }
  if (key == DANZEFF_HOME) {
    loc_curr_tab->input_x = 0;
    loc_curr_tab->input_first = 0;
  } else
  if (key == DANZEFF_END) {
    loc_curr_tab->input_x = loc_curr_tab->input_len;
    psp_console_update_first_eol();
  } else
  if ((key == DANZEFF_HISTORY) || (key == DANZEFF_UP)) {
    if (loc_curr_tab->curr_history > 0) {
      int length = strlen(loc_curr_tab->last_buffer[loc_curr_tab->curr_history - 1]);
      if (length) {
        if (loc_curr_tab->curr_history == CONSOLE_MAX_HISTORY) {
          strcpy(loc_curr_tab->last_buffer[CONSOLE_MAX_HISTORY], loc_curr_tab->input_buffer);
          loc_curr_tab->curr_history--;
        } else
        if (loc_curr_tab->curr_history) {
          loc_curr_tab->curr_history--;
        }
        strcpy(scan_str, loc_curr_tab->last_buffer[loc_curr_tab->curr_history]);
        loc_curr_tab->input_len = strlen(scan_str);
        loc_curr_tab->input_x = loc_curr_tab->input_len;
        psp_console_update_first_eol();
      }
    }

  } else
  if (key == DANZEFF_DOWN) {

    if (loc_curr_tab->curr_history < CONSOLE_MAX_HISTORY) {
      loc_curr_tab->curr_history++;
      strcpy(scan_str, loc_curr_tab->last_buffer[loc_curr_tab->curr_history]);
      loc_curr_tab->input_len = strlen(scan_str);
      loc_curr_tab->input_x = loc_curr_tab->input_len;
      psp_console_update_first_eol();
    }

  } else
  if (key == DANZEFF_CLEAR) {
    scan_str[0] = '\0';
    loc_curr_tab->input_len = 0;
    loc_curr_tab->input_x = 0;
    loc_curr_tab->input_first = 0;
    loc_curr_tab->curr_history = CONSOLE_MAX_HISTORY;

  } else
  if (key == DANZEFF_DEL) {
    if (loc_curr_tab->input_len > 0) {
      if (loc_curr_tab->input_x > 0) {
        for (index = loc_curr_tab->input_x - 1; index < loc_curr_tab->input_len; index++) {
           scan_str[index] = scan_str[index + 1];
        }
        loc_curr_tab->input_x--;
        loc_curr_tab->input_len--;
        scan_str[loc_curr_tab->input_len] = 0;
        psp_console_update_first();
      }
    }
    loc_curr_tab->curr_history = CONSOLE_MAX_HISTORY;

  } else 
  if (key == DANZEFF_LEFT) {
    if (loc_curr_tab->input_x > 0) loc_curr_tab->input_x--;
    psp_console_update_first();
    return;

  } else 
  if (key == DANZEFF_RIGHT) {

    if (loc_curr_tab->input_x < loc_curr_tab->input_len) {
      loc_curr_tab->input_x++;
    } else {

      if ((loc_curr_tab->input_len + 1) >= max_length) return;

      scan_str[loc_curr_tab->input_x] = ' ';
      loc_curr_tab->input_len++;
      scan_str[loc_curr_tab->input_len] = 0;
      loc_curr_tab->input_x++;
    }
    psp_console_update_first();

  }
  if ((key == DANZEFF_ENTER) || (key == '\n')) {
    /* Send the line to the server */
    if (loc_curr_tab->input_len > 0) {
      char buffer[300];
      if (scan_str[0] != '/') {
        if (PSPIRC.irc_user_color != COLOR_NONE) {
          sprintf(buffer, "\003%d", PSPIRC.irc_user_color);
          strcat(buffer, scan_str);
          scan_str = buffer;
        }
      }
      if (!strncasecmp(scan_str, "/CLEAR",6)) {
        psp_console_clear_tab(loc_curr_tab);
      } else
      if (loc_curr_tab->type == CONSOLE_TYPE_PRIVATE) {
        irc_thread_send_private(loc_curr_tab->name, scan_str);
      } else {
        irc_thread_send_message(loc_curr_tab->name, scan_str);
      }
      for (index = 1; index < CONSOLE_MAX_HISTORY; index++) {
         strcpy(loc_curr_tab->last_buffer[index - 1], loc_curr_tab->last_buffer[index]);
      }
      loc_curr_tab->curr_history = CONSOLE_MAX_HISTORY;
      strcpy(loc_curr_tab->last_buffer[CONSOLE_MAX_HISTORY - 1], loc_curr_tab->input_buffer);

      memset(loc_curr_tab->input_buffer, 0, max_length);
      loc_curr_tab->input_len = 0;
      loc_curr_tab->input_x = 0;
      loc_curr_tab->input_first = 0;
    }
    return;

  } else
  if ((key >= ' ') || (key == DANZEFF_COLOR)) {
  
    if ((loc_curr_tab->input_len + 1) >= max_length) return;

    for (index = loc_curr_tab->input_len; index >= loc_curr_tab->input_x; index--) {
      scan_str[index] = scan_str[index - 1];
    }

    scan_str[loc_curr_tab->input_x] = key;
    loc_curr_tab->input_x++;
    loc_curr_tab->input_len++;
    scan_str[loc_curr_tab->input_len] = 0;

    psp_console_update_first();
    loc_curr_tab->curr_history = CONSOLE_MAX_HISTORY;
  }
}

void
psp_console_put_string(char *buffer)
{
  int index;
  for (index = 0; buffer[index]; index++) {
    uchar c = buffer[index];
    psp_console_put_key(c);
  }
}

console_tab_list*
psp_console_get_tabs()
{
  console_tab_list* tab_list = (console_tab_list* )malloc( sizeof(console_tab_list) );
  memset(tab_list, 0, sizeof(console_tab_list));

  console_tab_t* scan_tab;
  int number_tab = 0;
  for (scan_tab = loc_tab_list; scan_tab != 0; scan_tab = scan_tab->next) {
    number_tab++;
  }
  tab_list->number_tab = number_tab;

  if (number_tab) {
    tab_list->tab_array = (console_tab_t** )malloc( sizeof( console_tab_t* ) * number_tab );
    int index = 0;
    for (scan_tab = loc_tab_list; scan_tab != 0; scan_tab = scan_tab->next) {
      tab_list->tab_array[index] = scan_tab;
      index++;
    }
  }
  return tab_list;
}

void
psp_console_free_tabs(console_tab_list* tab_list)
{
  if (tab_list->tab_array) {
    free(tab_list->tab_array);
  }
  free(tab_list);
}
