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

#include <pspkernel.h>
#include <psppower.h>
#include <pspctrl.h>
#include <pspwlan.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include <SDL.h>

#include "psp_global.h"
#include "psp_sdl.h"
#include "psp_menu.h"
#include "psp_menu_chantab.h"
#include "psp_menu_tab.h"
#include "psp_menu_user.h"
#include "psp_danzeff.h"
#include "psp_fmgr.h"
#include "psp_irc.h"
#include "psp_irkeyb.h"
#include "irc_thread.h"
#include "irc_global.h"

static int volatile loc_is_running = 0;
static irc_chan_elem* loc_chanelem_list = 0;

int 
psp_irc_chanlist_is_running()
{
  return loc_is_running;
}

static void
psp_irc_freeall_chanelem()
{
  irc_chan_elem* del_chan;

  while (loc_chanelem_list) {
    del_chan = loc_chanelem_list;
    loc_chanelem_list = loc_chanelem_list->next;
    if (del_chan->name) free(del_chan->name);
    if (del_chan->topic) free(del_chan->topic);
    free(del_chan);
  }
}

void
psp_irc_chanlist_refresh()
{
  psp_irc_freeall_chanelem();

  loc_is_running = 1;
  irc_thread_send_message("", "/LIST");
  int index = 0;
  while (index < 10) {
    sceKernelDelayThread(1000000);
    if (! loc_is_running) break;
    index++;
  }
  loc_is_running = 0;
}

void 
psp_irc_chanlist_begin()
{
  if (loc_is_running) {
  }
}

static int     loc_chanlist_muser = 0;
static regex_t loc_chanlist_regexpr;
static int     loc_chanlist_do_regexpr = 0;

static int
psp_irc_is_match(char *chan_name, int number_user, char* topic) 
{
  if (number_user >= loc_chanlist_muser) {
    if (! loc_chanlist_do_regexpr) {
      return 1;
    }
    if (! regexec(&loc_chanlist_regexpr, chan_name, 1, NULL, REG_NOTBOL)) {
      return 1;
    }
    if (! regexec(&loc_chanlist_regexpr, topic    , 1, NULL, REG_NOTBOL)) {
      return 1;
    }
  }
  return 0;
}

void
psp_irc_set_chanlist_match(char *match_string)
{
  loc_chanlist_do_regexpr = 0;
  if ((match_string) && (match_string[0])) {
    regfree(&loc_chanlist_regexpr);
    if (! regcomp(&loc_chanlist_regexpr, match_string, REG_ICASE|REG_EXTENDED|REG_NOSUB)) {
      loc_chanlist_do_regexpr = 1;
    }
  }
}

void
psp_irc_set_chanlist_min_user(int min_user)
{
  loc_chanlist_muser = min_user;
}

irc_chan_elem* 
psp_irc_add_chanlist(const char *new_line)
{
  char buffer[256];
  char *scan_str_1;
  char *scan_str_2;
  int   chan_number_user = 0;
  char *chan_name;
  char *chan_topic;

  if (loc_is_running) {
    strncpy(buffer, new_line, 256);
    scan_str_1 = strchr(buffer, ' ');

    if (scan_str_1) {
      *scan_str_1 = 0;
      scan_str_1++;
      chan_name = buffer;
      scan_str_2 = strchr(scan_str_1, ':');
      if (scan_str_2) {
        *scan_str_2 = 0;
        chan_number_user = atoi(scan_str_1);
        chan_topic = scan_str_2 + 1;
      } else {
        chan_topic = "none";
      }

      if (psp_irc_is_match(chan_name, chan_number_user, chan_topic)) {

        irc_chan_elem* new_chan = (irc_chan_elem*)malloc( sizeof( irc_chan_elem ) );
        memset(new_chan, 0, sizeof( irc_chan_elem ));
        new_chan->next = loc_chanelem_list;
        loc_chanelem_list = new_chan;

        new_chan->name = strdup(chan_name);
        new_chan->number_user = chan_number_user;
        new_chan->topic = strdup(chan_topic);

        return new_chan;
      }
    }
  }
  return 0;
}

void 
psp_irc_chanlist_end()
{
  loc_is_running = 0;
}

irc_chan_list*
psp_irc_get_chanlist()
{
  irc_chan_list* chan_list = (irc_chan_list* )malloc( sizeof(irc_chan_list) );
  memset(chan_list, 0, sizeof(irc_chan_list));

  irc_chan_elem* scan_chan;
  int number_chan = 0;
  for (scan_chan = loc_chanelem_list; scan_chan != 0; scan_chan = scan_chan->next) {
    number_chan++;
  }
  chan_list->number_chan = number_chan;

  if (number_chan) {
    chan_list->chan_array = (irc_chan_elem** )malloc( sizeof( irc_chan_elem* ) * number_chan );
    int index = 0;
    for (scan_chan = loc_chanelem_list; scan_chan != 0; scan_chan = scan_chan->next) {
      chan_list->chan_array[index] = scan_chan;
      index++;
      if (index >= number_chan) break;
    }
  }
  return chan_list;
}

void
psp_irc_free_chanlist(irc_chan_list* chan_list)
{
  if (chan_list->chan_array) {
    free(chan_list->chan_array);
  }
  free(chan_list);
}


void
psp_irc_display_screen()
{
  psp_console_display_screen();
}

void
psp_irc_clear_screen()
{
  psp_irc_display_screen();
}

# define ANALOG_THRESHOLD 60

void 
psp_irc_analog_direction(int Analog_x, int Analog_y, int *x, int *y)
{
  int DeltaX = 255;
  int DeltaY = 255;
  int DirX   = 0;
  int DirY   = 0;

  *x = 0;
  *y = 0;

  if (Analog_x <=        ANALOG_THRESHOLD)  { DeltaX = Analog_x; DirX = -1; }
  else 
  if (Analog_x >= (255 - ANALOG_THRESHOLD)) { DeltaX = 255 - Analog_x; DirX = 1; }

  if (Analog_y <=        ANALOG_THRESHOLD)  { DeltaY = Analog_y; DirY = -1; }
  else 
  if (Analog_y >= (255 - ANALOG_THRESHOLD)) { DeltaY = 255 - Analog_y; DirY = 1; }

  *x = DirX;
  *y = DirY;
}

//#define PSP_IRC_MIN_TIME 200000
#define PSP_IRC_MIN_TIME 2000000

void
psp_irc_main_loop()
{
  SceCtrlData c;

  long new_pad = 0;
  long old_pad = 0;
  int  new_X   = 0;
  int  old_X   = 0;
  int  new_Y   = 0;
  int  old_Y   = 0;

  int  danzeff_mode = 0;
  int  danzeff_key;
# ifdef USE_PSP_IRKEYB
  int  irkeyb_mode = 0;
  int  irkeyb_key = 0;
# endif

  int last_time  = 0;

  while (! irc_thread_session_is_terminated())  {

    psp_irc_display_screen();

    if (danzeff_mode) {
      danzeff_moveTo(0, -122);
      danzeff_render();
    }
    psp_sdl_flip();

    if (PSPIRC.psp_screenshot_mode) {
      PSPIRC.psp_screenshot_mode--;
      if (PSPIRC.psp_screenshot_mode <= 0) {
        psp_sdl_save_screenshot();
        PSPIRC.psp_screenshot_mode = 0;
      }
    }
    if (PSPIRC.psp_save_log_mode) {
      PSPIRC.psp_save_log_mode--;
      if (PSPIRC.psp_save_log_mode <= 0) {
        psp_console_save_log();
        PSPIRC.psp_save_log_mode = 0;
      }
    }

# ifdef USE_PSP_IRKEYB
    irkeyb_mode = 0;
# endif
    sceDisplayWaitVblankStart();
 
    while (1) {

      myCtrlPeekBufferPositive(&c, 1);
      c.Buttons &= PSP_ALL_BUTTON_MASK;

# ifdef USE_PSP_IRKEYB
      irkeyb_key = psp_irkeyb_set_psp_key(&c);
# endif
      new_pad = c.Buttons;

      psp_irc_analog_direction(c.Lx, c.Ly, &new_X, &new_Y);

      if ((new_X != old_X) ||
          (new_Y != old_Y)) {
        old_X = new_X;
        old_Y = new_Y;
        break;
      }

      if ((c.TimeStamp - last_time) > PSP_IRC_MIN_TIME) {
        last_time = c.TimeStamp;
        //Force schedule
        sceKernelDelayThread(1);
        break;
      }

      if (old_pad != new_pad) {
        old_pad = new_pad;
        break;
      }

# ifdef USE_PSP_IRKEYB
      irkeyb_key = psp_irkeyb_read_key();
      if (irkeyb_key != PSP_IRKEYB_EMPTY) {
        if (irkeyb_key >= ' ') {
          psp_console_put_key(irkeyb_key);
        } else 
        if (irkeyb_key == 0x8) {
          psp_console_put_key(DANZEFF_DEL);
        } else
        if (irkeyb_key == 0xd) {
          psp_console_put_key(DANZEFF_ENTER);
        } else /* ctrl L */
        if (irkeyb_key == 0xc) {
          psp_console_put_key(DANZEFF_CLEAR);
        } else 
        if (irkeyb_key == PSP_IRKEYB_HOME) {
          psp_console_put_key(DANZEFF_HOME);
        } else
        if (irkeyb_key == PSP_IRKEYB_END) {
          psp_console_put_key(DANZEFF_END);
        }
        irkeyb_mode = 1;
        break;
      }
# endif
    }

# ifdef USE_PSP_IRKEYB
    if (danzeff_mode && !irkeyb_mode) 
# else
    if (danzeff_mode)
# endif
    {
      danzeff_key = danzeff_readInput(c);

      /* Hack for utf8 chars */
      if (((danzeff_key >> 8) == 0xc2) ||
          ((danzeff_key >> 8) == 0xc3)) {
        uchar c1 = (danzeff_key >> 8) & 0xff;
        uchar c2 = danzeff_key & 0xff;
        danzeff_key = psp_convert_utf8_to_iso_8859_1(c1, c2);
        psp_console_put_key(danzeff_key);

      } else
      if (danzeff_key > DANZEFF_START) {
        if (danzeff_key >= ' ') {
          psp_console_put_key(danzeff_key);
        } else
        if (danzeff_key == DANZEFF_COLOR) {
          psp_console_put_key(danzeff_key);
        } else
        if (danzeff_key == DANZEFF_DEL ) {
          psp_console_put_key(DANZEFF_DEL);
        } else
        if (danzeff_key == DANZEFF_LEFT ) {
          psp_console_put_key(DANZEFF_LEFT);
        } else
        if (danzeff_key == DANZEFF_RIGHT ) {
          psp_console_put_key(DANZEFF_RIGHT);
        } else
        if (danzeff_key == DANZEFF_CLEAR) {
          psp_console_put_key(DANZEFF_CLEAR);
        } else
        if (danzeff_key == DANZEFF_ENTER) {
          psp_console_put_key(DANZEFF_ENTER);
        } else 
        if (danzeff_key == DANZEFF_DOWN ) {
          psp_console_put_key(DANZEFF_ENTER);
        } else
        if (danzeff_key == DANZEFF_UP) {
          psp_console_put_key(DANZEFF_BEGEND);
        } else
        if (danzeff_key == DANZEFF_LIST) {
          psp_console_put_string("/LIST ");
        } else 
        if (danzeff_key == DANZEFF_JOIN) {
          psp_console_put_string("/JOIN ");
        } else 
        if (danzeff_key == DANZEFF_PART) {
          psp_console_put_string("/PART ");
        } else 
        if (danzeff_key == DANZEFF_NICK) {
          psp_console_put_string("/NICK ");
        } else 
        if (danzeff_key == DANZEFF_QUIT) {
          psp_console_put_string("/QUIT ");
        } else 
        if (danzeff_key == DANZEFF_WHOIS) {
          psp_console_put_string("/WHOIS ");
        } else 
        if (danzeff_key == DANZEFF_WHO) {
          psp_console_put_string("/WHO ");
        } else 
        if (danzeff_key == DANZEFF_INVITE) {
          psp_console_put_string("/INVITE ");
        } else
        if (danzeff_key == DANZEFF_KICK) {
          psp_console_put_string("/KICK ");
        } else
        if (danzeff_key == DANZEFF_BAN) {
          psp_console_put_string("/BAN ");
        } else
        if (danzeff_key == DANZEFF_OPER) {
          psp_console_put_string("/OPER ");
        } else
        if (danzeff_key == DANZEFF_MODE) {
          psp_console_put_string("/MODE ");
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

    int new_Lx = 0;
    int new_Ly = 0;
    psp_irc_analog_direction(c.Lx, c.Ly, &new_Lx, &new_Ly);
    if (new_Ly > 0) {
      psp_console_screen_scroll_down();
    } else 
    if (new_Ly < 0) {
      psp_console_screen_scroll_up();
    }
    if (new_Lx > 0) {
      psp_console_put_key(DANZEFF_RIGHT);
    } else 
    if (new_Lx < 0) {
      psp_console_put_key(DANZEFF_LEFT);
    }

    if ((new_pad & (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) ==
        (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) {
      /* Exit ! */
      psp_sdl_exit(0);
    } else
    if(new_pad & PSP_CTRL_SELECT) {
      /* Back to Main Menu */
      if (psp_irc_main_menu()) break;

    } else
    if(new_pad & PSP_CTRL_SQUARE) {
      /* Enter in channel list Menu */
      psp_irc_chanlist_menu();
    } else
    if(new_pad & PSP_CTRL_CIRCLE) {
      /* Enter in User list Menu */
      psp_irc_users_menu();
    } else
    if(new_pad & PSP_CTRL_TRIANGLE) {
      psp_irc_tabs_menu();
    } else
    if(new_pad & PSP_CTRL_START) {
      danzeff_mode = 1;
    } else
    if (new_pad & PSP_CTRL_LEFT) {
      if (new_pad & PSP_CTRL_RTRIGGER) {
        psp_console_put_key(DANZEFF_HOME);
      } else {
        psp_console_put_key(DANZEFF_LEFT);
      }
    } else
    if (new_pad & PSP_CTRL_RIGHT) {
      if (new_pad & PSP_CTRL_RTRIGGER) {
        psp_console_put_key(DANZEFF_END);
      } else {
        psp_console_put_key(DANZEFF_RIGHT);
      }
    } else
    if(new_pad & PSP_CTRL_LTRIGGER) {
      psp_console_tab_left();
    } else
    if(new_pad & PSP_CTRL_RTRIGGER) {
      psp_console_tab_right();
    } else 
    if (new_pad & PSP_CTRL_DOWN) {
      psp_console_put_key(DANZEFF_DOWN);
    } else 
    if (new_pad & PSP_CTRL_CROSS) {
      psp_console_put_key(DANZEFF_ENTER);
    } else
    if (new_pad & PSP_CTRL_UP) {
      psp_console_put_key(DANZEFF_UP);
    }
  }

  psp_irc_freeall_chanelem();
}

