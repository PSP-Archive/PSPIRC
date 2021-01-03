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
#include "psp_sdl.h"
#include "psp_fmgr.h"
#include "psp_menu.h"
#include "psp_battery.h"
#include "psp_help.h"
#include "psp_irkeyb.h"
#include "psp_console.h"

# define MENU_CONFIG_USER_COLOR   0
# define MENU_CONFIG_OTHER_COLOR  1
# define MENU_CONFIG_MULTI_COLOR  2
# define MENU_CONFIG_BOTTOM       3
# define MENU_CONFIG_STAMP        4
# define MENU_CONFIG_HELP         5
# define MENU_CONFIG_LOAD_CONFIG  6
# define MENU_CONFIG_SAVE_CONFIG  7
# define MENU_CONFIG_BACK         8

# define MAX_MENU_ITEM (MENU_CONFIG_BACK+ 1)

  static menu_item_t menu_list[] =
  { 
    { "Your color   :" },
    { "Other color  :" },
    { "Multi color  :" },
    { "Display text :" },
    { "Time stamp   :" },

    { "Help" },

    { "Load settings" },
    { "Save settings" },

    { "Back"  }
  };

  static int cur_menu_id = MENU_CONFIG_USER_COLOR;

static void 
psp_display_screen_menu_config(void)
{
  char buffer[64];
  int menu_id = 0;
  int color   = 0;
  int x       = 0;
  int y       = 0;
  int y_step  = 0;

  {
    psp_sdl_blit_menu();

    psp_sdl_draw_rectangle(10,10,459,249,PSP_MENU_BORDER_COLOR,0);
    psp_sdl_draw_rectangle(11,11,457,247,PSP_MENU_BORDER_COLOR,0);

    psp_sdl_back_print(  30, 6, " Settings ", PSP_MENU_NOTE_COLOR);

    psp_display_screen_menu_battery();

    psp_sdl_back_print(30, 254, " O/X: Valid  Sel: Back ", 
                       PSP_MENU_BORDER_COLOR);

    psp_sdl_back_print(370, 254, " By Zx-81 ",
                       PSP_MENU_AUTHOR_COLOR);
  }
  
  x      = 20;
  y      = 25;
  y_step = 10;
  
  for (menu_id = 0; menu_id < MAX_MENU_ITEM; menu_id++) {
    color = PSP_MENU_TEXT_COLOR;
    if (cur_menu_id == menu_id) color = PSP_MENU_SEL_COLOR;
    else 
    if (menu_id == MENU_CONFIG_HELP) color = PSP_MENU_GREEN_COLOR;

    psp_sdl_back_print(x, y, menu_list[menu_id].title, color);

    if (menu_id == MENU_CONFIG_HELP) {
      y += y_step;
    } else
    if (menu_id == MENU_CONFIG_SAVE_CONFIG) {
      y += y_step;
    } else
    if (menu_id == MENU_CONFIG_USER_COLOR) {
      if (PSPIRC.irc_user_color == COLOR_NONE) {
        strcpy(buffer, "default");
      } else {
        sprintf(buffer,"%s", console_colors_name[PSPIRC.irc_user_color]);
        color = console_colors[PSPIRC.irc_user_color];
      }
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(140, y, buffer, color);
    } else
    if (menu_id == MENU_CONFIG_OTHER_COLOR) {
      if (PSPIRC.irc_other_color == COLOR_NONE) {
        strcpy(buffer, "default");
      } else {
        sprintf(buffer,"%s", console_colors_name[PSPIRC.irc_other_color]);
        color = console_colors[PSPIRC.irc_other_color];
      }
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(140, y, buffer, color);
    } else 
    if (menu_id == MENU_CONFIG_BOTTOM) {
      if (PSPIRC.irc_start_bottom) strcpy(buffer, "bottom");
      else                         strcpy(buffer, "top");
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(140, y, buffer, color);
    } else
    if (menu_id == MENU_CONFIG_MULTI_COLOR) {
      if (PSPIRC.irc_multi_color) strcpy(buffer, "yes");
      else                        strcpy(buffer, "no");
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(140, y, buffer, color);
    } else
    if (menu_id == MENU_CONFIG_STAMP) {
      if (PSPIRC.irc_time_stamp) strcpy(buffer, "yes");
      else                       strcpy(buffer, "no");
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(140, y, buffer, color);
      y += y_step;
    } else
    if (menu_id == MENU_CONFIG_BACK) {
      y += y_step;
    }

    y += y_step;
  }
}

static void
psp_user_go_right()
{
  PSPIRC.irc_user_color++;
  if (PSPIRC.irc_user_color >= (CONSOLE_MAX_COLOR-1)) {
    PSPIRC.irc_user_color = COLOR_NONE;
  }
}

static void
psp_user_go_left()
{
  PSPIRC.irc_user_color--;
  if (PSPIRC.irc_user_color < COLOR_NONE) {
    PSPIRC.irc_user_color = CONSOLE_MAX_COLOR-2;
  }
}

static void
psp_other_go_right()
{
  PSPIRC.irc_other_color++;
  if (PSPIRC.irc_other_color >= (CONSOLE_MAX_COLOR-1)) {
    PSPIRC.irc_other_color = COLOR_NONE;
  }
}

static void
psp_other_go_left()
{
  PSPIRC.irc_other_color--;
  if (PSPIRC.irc_other_color < COLOR_NONE) {
    PSPIRC.irc_other_color = CONSOLE_MAX_COLOR-1;
  }
}

void
psp_irc_config_menu_save()
{
  int error;

  error = psp_global_save_config();

  if (! error) /* save OK */
  {
    psp_display_screen_menu_config();
    psp_sdl_back_print(270, 80, "File saved !", PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  else 
  {
    psp_display_screen_menu_config();
    psp_sdl_back_print(270, 80, "Can't save file !", 
                       PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
}

static void
psp_irc_config_menu_load()
{
  int error;

  error = psp_global_load_config();

  if (! error) /* save OK */
  {
    psp_display_screen_menu_config();
    psp_sdl_back_print(270, 80, "File loaded !", PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  else 
  {
    psp_display_screen_menu_config();
    psp_sdl_back_print(270, 80, "Can't load file !", 
                       PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
}

int 
psp_irc_config_menu(void)
{
  SceCtrlData c;
  long        new_pad;
  long        old_pad;

  int         last_time;
  int         end_menu;
  int         irkeyb_key;

  psp_kbd_wait_no_button();

  old_pad   = 0;
  last_time = 0;
  end_menu  = 0;

# ifdef USE_PSP_IRKEYB
  irkeyb_key = PSP_IRKEYB_EMPTY;
# endif

  while (! end_menu)
  {
    psp_display_screen_menu_config();
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
    if ((new_pad & PSP_CTRL_CROSS ) || 
        (new_pad & PSP_CTRL_CIRCLE))
    {
      switch (cur_menu_id ) 
      {
        case MENU_CONFIG_LOAD_CONFIG : psp_irc_config_menu_load();
        break;

        case MENU_CONFIG_SAVE_CONFIG : psp_irc_config_menu_save();
        break;

        case MENU_CONFIG_BACK        : end_menu = 1;
        break;

        case MENU_CONFIG_HELP        : psp_help_menu();
                                       old_pad = new_pad = 0;
        break;              
      }

    } else
    if(new_pad & PSP_CTRL_UP) {

      if (cur_menu_id > 0) cur_menu_id--;
      else                 cur_menu_id = MAX_MENU_ITEM-1;

    } else
    if(new_pad & PSP_CTRL_DOWN) {

      if (cur_menu_id < (MAX_MENU_ITEM-1)) cur_menu_id++;
      else                                 cur_menu_id = 0;

    } else  
    if(new_pad & PSP_CTRL_SQUARE) {
      /* Cancel */
      end_menu = -1;
    } else 
    if(new_pad & PSP_CTRL_SELECT) {
      /* Back to Console */
      end_menu = 1;
    } else
    if(new_pad & PSP_CTRL_RIGHT) {
      if (cur_menu_id == MENU_CONFIG_USER_COLOR) {
        psp_user_go_right();
      } else 
      if (cur_menu_id == MENU_CONFIG_OTHER_COLOR) {
        psp_other_go_right();
      } else 
      if (cur_menu_id == MENU_CONFIG_BOTTOM) {
        PSPIRC.irc_start_bottom = ! PSPIRC.irc_start_bottom;
      } else
      if (cur_menu_id == MENU_CONFIG_STAMP) {
        PSPIRC.irc_time_stamp = ! PSPIRC.irc_time_stamp;
      } else
      if (cur_menu_id == MENU_CONFIG_MULTI_COLOR) {
        PSPIRC.irc_multi_color = ! PSPIRC.irc_multi_color;
      }

    } else
    if(new_pad & PSP_CTRL_LEFT) {
      if (cur_menu_id == MENU_CONFIG_USER_COLOR) {
        psp_user_go_left();
      } else
      if (cur_menu_id == MENU_CONFIG_OTHER_COLOR) {
        psp_other_go_left();
      } else
      if (cur_menu_id == MENU_CONFIG_BOTTOM) {
        PSPIRC.irc_start_bottom = ! PSPIRC.irc_start_bottom;
      } else
      if (cur_menu_id == MENU_CONFIG_STAMP) {
        PSPIRC.irc_time_stamp = ! PSPIRC.irc_time_stamp;
      } else
      if (cur_menu_id == MENU_CONFIG_MULTI_COLOR) {
        PSPIRC.irc_multi_color = ! PSPIRC.irc_multi_color;
      }
    }
  }
 
  psp_kbd_wait_no_button();

  return 0;
}

