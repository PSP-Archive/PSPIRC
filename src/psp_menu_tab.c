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
#include "psp_menu_tab.h"
#include "irc_thread.h"

int 
psp_irc_tabs_menu(void)
{
  char buffer[60];

  SceCtrlData c;
  long        new_pad;
  long        old_pad;

  int         last_time;
  int         end_menu;

  console_tab_list* tab_list = psp_console_get_tabs();
  int number_tab = tab_list->number_tab;

  if (! number_tab) return 0;
  
  psp_kbd_wait_no_button();

  old_pad      = 0;
  last_time    = 0;
  end_menu     = 0;

  int cur_tab_id  = 0;
  int top_tab_id  = 0;
  int max_lines   = 22;

# ifdef USE_PSP_IRKEYB
  int irkeyb_key = PSP_IRKEYB_EMPTY;
# endif

  while (! end_menu) {

    psp_sdl_blit_menu();

    psp_sdl_draw_rectangle(10,10,459,249,PSP_MENU_BORDER_COLOR,0);
    psp_sdl_draw_rectangle(11,11,457,247,PSP_MENU_BORDER_COLOR,0);

    psp_sdl_back_print(  30, 6, " Tabs ", PSP_MENU_NOTE_COLOR);
    psp_sdl_back_print( 310, 6, " Tri: Close ", PSP_MENU_BORDER_COLOR );

    psp_display_screen_menu_battery();

    psp_sdl_back_print(30, 254, " O: Paste  X: Active  Sel/[]: Back ", 
                       PSP_MENU_BORDER_COLOR);

    psp_sdl_back_print(370, 254, " By Zx-81 ",
                       PSP_MENU_AUTHOR_COLOR);

    int x      = 20;
    int y      = 25;
    int y_step = 10;

    int index;

    for (index = top_tab_id; index < number_tab; index++) {
      int color = PSP_MENU_TEXT_COLOR;
      if (index == cur_tab_id) {
        color = PSP_MENU_SEL_COLOR;
        buffer[0] = '>';
      } else {
        buffer[0] = ' ';
      }
      char* tab_name;
      if (index == 0) tab_name = "console";
      else            tab_name = tab_list->tab_array[index]->name;
      char unread_char = tab_list->tab_array[index]->unread ? '+' : '-';
      snprintf(buffer + 1, 53, " [%2d] %c %s", index, unread_char, tab_name);

      psp_sdl_back_print(x, y, buffer, color);

      y += y_step;
      if (y > 260) break;
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

    if ((c.Buttons & (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) ==
        (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) {
      /* Exit ! */
      psp_sdl_exit(0);
    } else
    if (new_pad & PSP_CTRL_CIRCLE) {
      snprintf( buffer, 60, "%s ", tab_list->tab_array[cur_tab_id]->name);
      psp_console_put_string(buffer);
      end_menu = 1;
    } else
    if (new_pad & PSP_CTRL_TRIANGLE) {
      psp_console_close_tab(tab_list->tab_array[cur_tab_id]->name);
      end_menu = 1;
    } else
    if (new_pad & PSP_CTRL_CROSS) {
      psp_console_set_current_tab(tab_list->tab_array[cur_tab_id]->name);
      end_menu = 1;
    } else
    if(new_pad & PSP_CTRL_UP) {
      cur_tab_id--;
    } else
    if(new_pad & PSP_CTRL_DOWN) {
      cur_tab_id++;
    } else  
    if(new_pad & PSP_CTRL_LEFT) {
      cur_tab_id -= 10;
    } else
    if(new_pad & PSP_CTRL_RIGHT) {
      cur_tab_id += 10;
    } else
    if(new_pad & PSP_CTRL_SQUARE) {
      /* Cancel */
      end_menu = -1;
    } else 
    if(new_pad & PSP_CTRL_SELECT) {
      /* Back to Console */
      end_menu = 1;
    }

    if (cur_tab_id >= number_tab) cur_tab_id = number_tab - 1;
    if (cur_tab_id < 0           ) cur_tab_id = 0;

    if (cur_tab_id >= top_tab_id + max_lines) top_tab_id = cur_tab_id - max_lines + 1;
    if (cur_tab_id <  top_tab_id            ) top_tab_id = cur_tab_id;

    if (top_tab_id > number_tab - max_lines) top_tab_id = number_tab - max_lines;
    if (top_tab_id < 0                      ) top_tab_id = 0;
  }
 
  psp_kbd_wait_no_button();

  psp_console_free_tabs(tab_list);
  
  return end_menu;
}

