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
#include "irc_thread.h"
#include "psp_menu_user.h"
#include "psp_menu_mode.h"
#include "irc_thread.h"

#define MENU_MODE_GIVE_VOICE  0
#define MENU_MODE_TAKE_VOICE  1
#define MENU_MODE_GIVE_HOPS   2
#define MENU_MODE_TAKE_HOPS   3
#define MENU_MODE_GIVE_OPS    4
#define MENU_MODE_TAKE_OPS    5
#define MENU_MODE_HELP        6
#define MENU_MODE_KICK        7
#define MENU_MODE_BAN         8
#define MENU_MODE_KICKBAN     9
#define MENU_MODE_WHO        10
#define MENU_MODE_WHOIS      11
#define MENU_MODE_PM         12
#define MENU_MODE_BACK       13

# define MAX_MENU_ITEM (MENU_MODE_BACK+1)

  static menu_item_t menu_list[] =
  { 
    { "Give voice" },
    { "Take voice" },
    { "Give half-oper" },
    { "Take half-oper" },
    { "Give oper" },
    { "Take oper" },

    { "Help" },

    { "Kick" },
    { "Ban" },
    { "KickBan" },

    { "Who" },
    { "WhoIs" },
    { "Send PM" },


    { "Back"  }
  };

  static int cur_menu_id = MENU_MODE_GIVE_VOICE;

static void 
psp_display_screen_menu(char *nick_name)
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

    psp_sdl_back_print(  30, 6, " Modes ", PSP_MENU_NOTE_COLOR);

    psp_display_screen_menu_battery();

    psp_sdl_back_print(30, 254, " O/X: Valid  []/Sel: Back ", 
                       PSP_MENU_BORDER_COLOR);

    psp_sdl_back_print(370, 254, " By Zx-81 ",
                       PSP_MENU_AUTHOR_COLOR);
  }

  
  x      = 20;
  y      = 25;
  y_step = 10;

  /* Display user name */
  psp_sdl_back_print(x, y, "User : ", PSP_MENU_TEXT_COLOR);
  strncpy(buffer, nick_name, 50);
  psp_sdl_back_print(70, y, buffer, PSP_MENU_TEXT_COLOR);

  y += 2*y_step;
  
  for (menu_id = 0; menu_id < MAX_MENU_ITEM; menu_id++) {
    color = PSP_MENU_TEXT_COLOR;
    if (cur_menu_id == menu_id) color = PSP_MENU_SEL_COLOR;
    else 
    if (menu_id == MENU_MODE_BAN) color = PSP_MENU_WARNING_COLOR;
    else
    if (menu_id == MENU_MODE_KICKBAN) color = PSP_MENU_WARNING_COLOR;
    else
    if (menu_id == MENU_MODE_HELP) color = PSP_MENU_GREEN_COLOR;

    psp_sdl_back_print(x, y, menu_list[menu_id].title, color);

    if (menu_id == MENU_MODE_HELP) {
      y += y_step;
    } else
    if (menu_id == MENU_MODE_TAKE_OPS) {
      y += y_step;
    } else
    if (menu_id == MENU_MODE_KICKBAN) {
      y += y_step;
    } else
    if (menu_id == MENU_MODE_PM) {
      y += y_step;
    } else
    if (menu_id == MENU_MODE_BACK) {
      y += y_step;
    }

    y += y_step;
  }
}

static void 
psp_irc_modes_menu_change(char *chan_name, char *nick_name, char *mode)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "/MODE %s %s %s", chan_name, mode, nick_name);
  irc_thread_send_message("", buffer);
}

static void 
psp_irc_modes_menu_kick(char *chan_name, char *nick_name)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "/KICK %s %s", chan_name, nick_name);
  irc_thread_send_message("", buffer);
}

static void 
psp_irc_modes_menu_ban(char *chan_name, irc_user_elem* user_elem)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "/MODE %s +b *!%s@%s", chan_name, user_elem->nick, user_elem->host);
  irc_thread_send_message(chan_name, buffer);
}

static void 
psp_irc_modes_menu_kickban(char *chan_name, irc_user_elem* user_elem)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "/MODE %s +b *!%s@%s", chan_name, user_elem->nick, user_elem->host);
  irc_thread_send_message(chan_name, buffer);

  snprintf(buffer, sizeof(buffer), "/KICK %s %s", chan_name, user_elem->nick);
  irc_thread_send_message(chan_name, buffer);
}

static void 
psp_irc_modes_menu_who(char *chan_name, char *nick_name)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "/WHO %s", nick_name);
  irc_thread_send_message(chan_name, buffer);
  psp_console_tab_first();
}

static void 
psp_irc_modes_menu_whois(char *chan_name, char *nick_name)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "/WHOIS %s", nick_name);
  irc_thread_send_message(chan_name, buffer);
  psp_console_tab_first();
}

int 
psp_irc_modes_menu(char *chan_name, irc_user_elem *user_elem)
{
  char buffer[60];

  SceCtrlData c;
  long        new_pad;
  long        old_pad;
  char       *nick_name;

  int         last_time;
  int         end_menu;

  psp_kbd_wait_no_button();

  old_pad      = 0;
  last_time    = 0;
  end_menu     = 0;

  int cur_user_id  = 0;
  int top_user_id  = 0;

  nick_name = user_elem->nick;

# ifdef USE_PSP_IRKEYB
  int irkeyb_key = PSP_IRKEYB_EMPTY;
# endif

  while (! end_menu) {

    psp_display_screen_menu(nick_name);
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
        case MENU_MODE_HELP       : psp_help_menu();
                                    old_pad = new_pad = 0;
        break;              
        case MENU_MODE_GIVE_VOICE : psp_irc_modes_menu_change( chan_name, nick_name, "+v");
                                    end_menu = 2;
        break;
        case MENU_MODE_TAKE_VOICE : psp_irc_modes_menu_change( chan_name, nick_name, "-v");
                                    end_menu = 2;
        break;
        case MENU_MODE_GIVE_HOPS  : psp_irc_modes_menu_change( chan_name, nick_name, "+h");
                                    end_menu = 2;
        break;
        case MENU_MODE_TAKE_HOPS  : psp_irc_modes_menu_change( chan_name, nick_name, "-h");
                                    end_menu = 2;
        break;
        case MENU_MODE_GIVE_OPS   : psp_irc_modes_menu_change( chan_name, nick_name, "+o");
                                    end_menu = 2;
        break;
        case MENU_MODE_TAKE_OPS   : psp_irc_modes_menu_change( chan_name, nick_name, "-o");
                                    end_menu = 2;
        break;
        case MENU_MODE_KICK       : psp_irc_modes_menu_kick( chan_name, nick_name );
                                    end_menu = 2;
        break;
        case MENU_MODE_BAN        : psp_irc_modes_menu_ban( chan_name, user_elem );
                                    end_menu = 2;
        break;
        case MENU_MODE_KICKBAN    : psp_irc_modes_menu_kickban( chan_name, user_elem );
                                    end_menu = 2;
        break;
        case MENU_MODE_WHO        : psp_irc_modes_menu_who( chan_name, nick_name );
                                    end_menu = 2;
        break;
        case MENU_MODE_WHOIS      : psp_irc_modes_menu_whois( chan_name, nick_name );
                                    end_menu = 2;
        break;
        case MENU_MODE_PM         : psp_console_open_private( nick_name );
                                    end_menu = 2;
        break;
        case MENU_MODE_BACK       : end_menu = 1;
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
    }
  }
 
  psp_kbd_wait_no_button();
  
  return end_menu;
}

