/*
 *  Copyright (C) 2006 Ludovic Jacomme (ludovic.jacomme@gmail.com)
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>

#include <pspctrl.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspiofilemgr.h>

#include "psp_global.h"
#include "psp_danzeff.h"
#include "psp_sdl.h"
#include "psp_fmgr.h"
#include "psp_menu.h"
#include "psp_battery.h"
#include "psp_help.h"
#include "psp_irkeyb.h"
#include "psp_console.h"
#include "psp_irc.h"
#include "psp_menu_user.h"
#include "irc_thread.h"

# define MENU_CHANLIST_MUSER  0
# define MENU_CHANLIST_MATCH  1
# define MAX_MENU_ITEM (MENU_CHANLIST_MATCH+ 1)

  static menu_item_t menu_list[] =
  { 
    { "Min users :" },
    { "Match     :" }
  };

# define MAX_MATCH_LENGTH 40
# define MAX_MUSER_LENGTH  4

  static int cur_menu_id  = MENU_CHANLIST_MUSER;
  static int cur_chan_id  = 0;
  static int top_chan_id  = 0;
  static int number_chan  = 0;
  static int channel_menu = 0;

  static irc_chan_list* loc_chan_list = 0;

  static char loc_chanlist_muser[MAX_MUSER_LENGTH];
  static char loc_chanlist_match[MAX_MATCH_LENGTH];

static void
psp_menu_chanlist_validate()
{
  int min_user = atoi(loc_chanlist_muser);
  sprintf(loc_chanlist_muser, "%d", min_user);

  psp_irc_set_chanlist_min_user(min_user);
  psp_irc_set_chanlist_match(loc_chanlist_match);
}

static void
psp_menu_chanlist_put_key(int key)
{
  char *scan_str   = NULL;
  int   length     = 0;
  int   max_length = 0;

  switch (cur_menu_id) 
  {
    case MENU_CHANLIST_MUSER : scan_str = loc_chanlist_muser;
                               max_length = MAX_MUSER_LENGTH;
    break;
    case MENU_CHANLIST_MATCH : scan_str = loc_chanlist_match;
                               max_length = MAX_MATCH_LENGTH;
    break;
  }

  if (key == DANZEFF_CLEAR) {
    scan_str[0] = '\0';
    return;
  }
 
  length = strlen(scan_str);
  if ((key == DANZEFF_DEL ) || 
      (key == DANZEFF_LEFT)) {
    if (length) {
      length--;
      scan_str[length] = '\0';
    }
  } else {
    if (cur_menu_id == MENU_CHANLIST_MUSER) {
      if ((key < '0') || (key > '9')) return;
    }
    if ((length + 1) >= max_length) return;
    scan_str[length] = key;
    length++;
    scan_str[length] = 0;
  }
}


static void
psp_display_screen_menu()
{
  char buffer[60];
  char chan_name[15];

  psp_sdl_blit_menu();

  psp_sdl_draw_rectangle(10,10,459,249,PSP_MENU_BORDER_COLOR,0);
  psp_sdl_draw_rectangle(11,11,457,247,PSP_MENU_BORDER_COLOR,0);

  psp_sdl_back_print(  30, 6, " Channel list ", PSP_MENU_NOTE_COLOR);
  psp_sdl_back_print( 310, 6, " Tri: Toggle ", PSP_MENU_BORDER_COLOR );

  psp_display_screen_menu_battery();

  psp_sdl_back_print(30, 254, " O: Refresh  X: Join  Sel/[]: Back ", 
                     PSP_MENU_BORDER_COLOR);

  psp_sdl_back_print(370, 254, " By Zx-81 ",
                     PSP_MENU_AUTHOR_COLOR);

  int x      = 20;
  int y      = 25;
  int y_step = 10;
  int menu_id = 0;

  int index;
  int color;

  for (menu_id = 0; menu_id < MAX_MENU_ITEM; menu_id++) {
    color = PSP_MENU_TEXT_COLOR;
    if (channel_menu) {
      if (cur_menu_id == menu_id) color = PSP_MENU_SEL_COLOR;
    }

    psp_sdl_back_print(x, y, menu_list[menu_id].title, color);

    if (menu_id == MENU_CHANLIST_MUSER) {
      strcpy(buffer, loc_chanlist_muser);
      if (menu_id == cur_menu_id) strcat(buffer, "_");
      string_fill_with_space(buffer, MAX_MUSER_LENGTH);
      psp_sdl_back_print(120, y, buffer, color);
    } else
    if (menu_id == MENU_CHANLIST_MATCH) {
      strcpy(buffer, loc_chanlist_match);
      if (menu_id == cur_menu_id) strcat(buffer, "_");
      string_fill_with_space(buffer, MAX_MATCH_LENGTH);
      psp_sdl_back_print(120, y, buffer, color);
    }
    y += y_step;
  }
  y += y_step;

  for (index = top_chan_id; index < number_chan; index++) {
    color = PSP_MENU_TEXT_COLOR;
    if (index == cur_chan_id) {
      if (! channel_menu) {
        color = PSP_MENU_SEL_COLOR;
      }
      buffer[0] = '>';
    } else {
      buffer[0] = ' ';
    }
    strncpy(chan_name, loc_chan_list->chan_array[index]->name, 12);
    snprintf(buffer + 1, 56, "%-12s |%3d| %s ", 
       loc_chan_list->chan_array[index]->name, 
       loc_chan_list->chan_array[index]->number_user,
       loc_chan_list->chan_array[index]->topic
    );

    psp_sdl_back_print(x, y, buffer, color);

    y += y_step;
    if (y > 240) break;
  }

}

static void
psp_display_print_refresh()
{
  psp_display_screen_menu();
  psp_sdl_back_print(150, 120,   "Refresh channel list ...", PSP_MENU_NOTE_COLOR);
  psp_sdl_flip();
}

static void
psp_menu_chanlist_update()
{
  psp_display_print_refresh();
  psp_irc_chanlist_refresh();

  psp_irc_free_chanlist(loc_chan_list);
  loc_chan_list = psp_irc_get_chanlist();
  number_chan = loc_chan_list->number_chan;
}

int 
psp_irc_chanlist_menu(void)
{
  char buffer[60];

  SceCtrlData c;
  long        new_pad;
  long        old_pad;

  int         last_time;
  int         end_menu;

  psp_menu_chanlist_validate();

  loc_chan_list = psp_irc_get_chanlist();
  number_chan = loc_chan_list->number_chan;

  channel_menu = 1;

  psp_kbd_wait_no_button();

  old_pad      = 0;
  last_time    = 0;
  end_menu     = 0;

  int max_lines    = 19;
  int  danzeff_mode = 0;
  int  danzeff_key;

# ifdef USE_PSP_IRKEYB
  int irkeyb_key = PSP_IRKEYB_EMPTY;
# endif

  while (! end_menu) {

    psp_display_screen_menu();

    if (channel_menu) {

      if (danzeff_mode) {
        danzeff_moveTo(-50, -50);
        danzeff_render();
      }
    }
    psp_sdl_flip();

    while (1) {

      myCtrlPeekBufferPositive(&c, 1);
      c.Buttons &= PSP_ALL_BUTTON_MASK;

# ifdef USE_PSP_IRKEYB
      irkeyb_key = psp_irkeyb_set_psp_key(&c);
# endif
      new_pad = c.Buttons;

      if ((old_pad != new_pad) || ((c.TimeStamp - last_time) > PSP_MENU_MIN_TIME)) {
        last_time = c.TimeStamp;
        old_pad = new_pad;
        break;
      }
# ifdef USE_PSP_IRKEYB
      if (irkeyb_key != PSP_IRKEYB_EMPTY) break;
# endif
    }

    if (channel_menu) {
# ifdef USE_PSP_IRKEYB
      irkeyb_key = psp_irkeyb_read_key();

      if (irkeyb_key > 0) {
        if (irkeyb_key > ' ') {
          psp_menu_chanlist_put_key(irkeyb_key);
        } else
        if (irkeyb_key == 0x8) {
          psp_menu_chanlist_put_key(DANZEFF_DEL);
        } else
        if (irkeyb_key == 0xd) {
          new_pad |= PSP_CTRL_CROSS;
        } else /* ctrl L */
        if (irkeyb_key == 0xc) {
          psp_menu_chanlist_put_key(DANZEFF_CLEAR);
        }
        irkeyb_key = PSP_IRKEYB_EMPTY;
      }
# endif

      if (danzeff_mode) {

        danzeff_key = danzeff_readInput(c);
        if (danzeff_key > DANZEFF_START) {
          /* Disable utf8 */
          danzeff_key &= 0x7f;
          if (danzeff_key > ' ') {
            psp_menu_chanlist_put_key(danzeff_key);
          } else
          if ((danzeff_key == DANZEFF_DEL ) ||
              (danzeff_key == DANZEFF_LEFT)) {
            psp_menu_chanlist_put_key(DANZEFF_DEL);
          } else
          if (danzeff_key == DANZEFF_CLEAR) {
            psp_menu_chanlist_put_key(DANZEFF_CLEAR);
          } else
          if ((danzeff_key == DANZEFF_ENTER) ||
              (danzeff_key == DANZEFF_DOWN )) {
            cur_menu_id = cur_menu_id + 1;
            if (cur_menu_id > MENU_CHANLIST_MATCH) cur_menu_id = MENU_CHANLIST_MUSER;
          } else
          if (danzeff_key == DANZEFF_UP) {
            cur_menu_id = cur_menu_id - 1;
            if (cur_menu_id < 0) cur_menu_id = MENU_CHANLIST_MATCH;
          }

        } else
        if ((danzeff_key == DANZEFF_START ) || 
            (danzeff_key == DANZEFF_SELECT)) 
        {
          danzeff_mode = 0;
          old_pad = new_pad = 0;

          psp_kbd_wait_no_button();
        }

        continue;
      }
    }

    if ((c.Buttons & (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) ==
        (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) {
      /* Exit ! */
      psp_sdl_exit(0);
    } else
    if (! channel_menu) {

      if (new_pad & PSP_CTRL_CIRCLE) {

        psp_menu_chanlist_validate();
        psp_menu_chanlist_update();
        if (! number_chan) channel_menu = 1;

      } else
      if (new_pad & PSP_CTRL_CROSS) {
        snprintf( buffer, 53, "/JOIN %s", loc_chan_list->chan_array[cur_chan_id]->name );
        irc_thread_send_message("", buffer);
        end_menu = 1;
      
      } else
      if(new_pad & PSP_CTRL_UP) {
        cur_chan_id--;
      } else
      if(new_pad & PSP_CTRL_DOWN) {
        cur_chan_id++;
      } else  
      if(new_pad & PSP_CTRL_LEFT) {
        cur_chan_id -= 10;
      } else
      if(new_pad & PSP_CTRL_RIGHT) {
        cur_chan_id += 10;
      } else
      if(new_pad & PSP_CTRL_SQUARE) {
        /* Cancel */
        end_menu = -1;
      } else 
      if(new_pad & PSP_CTRL_TRIANGLE) {
        channel_menu = 1;
        psp_kbd_wait_no_button();
      } else
      if(new_pad & PSP_CTRL_SELECT) {
        /* Back to Console */
        end_menu = 1;
      }

      if (cur_chan_id >= number_chan) cur_chan_id = number_chan - 1;
      if (cur_chan_id < 0           ) cur_chan_id = 0;

      if (cur_chan_id >= top_chan_id + max_lines) top_chan_id = cur_chan_id - max_lines + 1;
      if (cur_chan_id <  top_chan_id            ) top_chan_id = cur_chan_id;
  
      if (top_chan_id > number_chan - max_lines) top_chan_id = number_chan - max_lines;
      if (top_chan_id < 0                      ) top_chan_id = 0;

    } else {

      if(new_pad & PSP_CTRL_TRIANGLE) {
        if (number_chan) {
          channel_menu = 0;
          psp_menu_chanlist_validate();
          psp_kbd_wait_no_button();
        }

      } else
      if(new_pad & PSP_CTRL_START) {
        danzeff_mode = 1;
      } else
      if(new_pad & PSP_CTRL_CIRCLE) {

        psp_menu_chanlist_validate();
        psp_menu_chanlist_update();

      } else
      if(new_pad & PSP_CTRL_SQUARE) {
        /* Cancel */
        end_menu = -1;
      } else 
      if(new_pad & PSP_CTRL_SELECT) {
        /* Back to Console */
        end_menu = 1;
      } else
      if(new_pad & PSP_CTRL_UP) {
        if (cur_menu_id > 0) cur_menu_id--;
        else                 cur_menu_id = MAX_MENU_ITEM-1;

      } else
      if(new_pad & PSP_CTRL_DOWN) {
        if (cur_menu_id < (MAX_MENU_ITEM-1)) cur_menu_id++;
        else                                 cur_menu_id = 0;
      }
    }
  }
 
  psp_kbd_wait_no_button();

  psp_irc_free_chanlist(loc_chan_list);
  loc_chan_list = 0;
  
  return end_menu;
}

