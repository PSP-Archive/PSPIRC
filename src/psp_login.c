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
#include <zlib.h>
#include <SDL/SDL.h>

#include <pspctrl.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspiofilemgr.h>

#include "psp_global.h"
#include "psp_sdl.h"
#include "psp_danzeff.h"
#include "psp_fmgr.h"
#include "psp_menu.h"
#include "psp_login.h"
#include "psp_battery.h"
#include "psp_help.h"
#include "psp_irkeyb.h"
#include "psp_console.h"

# define MENU_LOGIN_HOST         0
# define MENU_LOGIN_PORT         1
# define MENU_LOGIN_USER         2
# define MENU_LOGIN_PASSWORD     3
# define MENU_LOGIN_NICKNAME     4
# define MENU_LOGIN_REALNAME     5
# define MENU_LOGIN_CHANNEL      6
# define MENU_LOGIN_SCRIPT       7

# define MENU_LOGIN_HELP        8
# define MENU_LOGIN_NEW         9
# define MENU_LOGIN_DELETE      10
# define MENU_LOGIN_DEFAULT     11

# define MENU_LOGIN_LOAD        12
# define MENU_LOGIN_SAVE        13

# define MENU_LOGIN_CONNECT     14
# define MENU_LOGIN_BACK        15

# define MENU_LOGIN_EXIT        16

# define MAX_MENU_LOGIN_ITEM (MENU_LOGIN_EXIT + 1)

  static menu_item_t menu_list[] =
  {
    { "Host      : " },
    { "Port      : " },
    { "User      : " },
    { "Password  : " },
    { "Nickname  : " },
    { "Realname  : " },
    { "Channel   : " },
    { "Script    : " },

    { "Help" },

    { "New profile" },
    { "Delete profile" },
    { "Set as default" },

    { "Load profiles" },
    { "Save profiles" },

    { "Connect" },
    { "Back to Wifi" },

    { "Exit" }
  };

  static int cur_menu_id = MENU_LOGIN_CONNECT;

  static psplogin_list_t *loc_head_login_list = NULL;

  psplogin_list_t *psp_login_current  = NULL;

static void
psp_del_all_login_list()
{
  psplogin_list_t *del_list;
  while (loc_head_login_list != (psplogin_list_t *)0) 
  {
    del_list = loc_head_login_list;
    loc_head_login_list = del_list->next;
    free(del_list);
  }
  psp_login_current  = NULL;
}

static int
psp_save_login_list()
{
  gzFile          *file_desc;
  psplogin_list_t *scan_list;
  char             buffer[sizeof(psplogin_t)];
  int              error = 0;
  int              index = 0;

  char  FileName[MAX_PATH+1];

  snprintf(FileName, MAX_PATH, "%s/login.dat", PSPIRC.psp_homedir);
  file_desc = gzopen(FileName, "w");

  if (! file_desc) {
    return 1;
  }
  for (scan_list  = loc_head_login_list;
       scan_list != (psplogin_list_t *)0;
       scan_list  = scan_list->next) {

    /* Encode data */
    memcpy(buffer, &scan_list->login, sizeof(psplogin_t));
    for (index = 0; index < sizeof(psplogin_t); index++) {
      unsigned char encode = buffer[index];
      encode = (((encode >> 4) & 0x0F) | ((encode << 4) & 0xF0));
      buffer[index] = encode;
    }
    if (gzwrite(file_desc, buffer, sizeof(psplogin_t)) != sizeof(psplogin_t)) {
      error = 1;
    }
  }
  gzclose(file_desc);

  return error;
}

static void
psp_default_login_list()
{
  psplogin_list_t *new_list;

  new_list = malloc(sizeof(psplogin_list_t));
  memset(new_list, 0, sizeof(psplogin_list_t));
  new_list->next = loc_head_login_list;
  loc_head_login_list = new_list;
  strcpy(new_list->login.user, "");
  strcpy(new_list->login.password, "");
  strcpy(new_list->login.nickname, "");
  strcpy(new_list->login.host, "irc.noobz.eu");
  strcpy(new_list->login.port, "6667");
  strcpy(new_list->login.channel, "#psp");
  strcpy(new_list->login.script, "");
  psp_login_current = new_list;
}

static int
psp_load_login_list()
{
  gzFile          *file_desc;
  psplogin_list_t *new_list;
  psplogin_list_t *scan_list;
  psplogin_list_t *prev_list;
  psplogin_list_t *next_list;
  char             buffer[sizeof(psplogin_t)];
  int              error = 0;
  int              index = 0;

  psp_del_all_login_list();

  psp_login_current = NULL;
  file_desc = gzopen("./login.dat", "r");

  if (file_desc) {

    while (gzread(file_desc, &buffer, sizeof(psplogin_t)) > 0) {
      new_list = malloc(sizeof(psplogin_list_t));
      memset(new_list, 0, sizeof(psplogin_list_t));

      /* Decode data */
      for (index = 0; index < sizeof(psplogin_t); index++) {
        unsigned char encode = buffer[index];
        encode = (((encode >> 4) & 0x0F) | ((encode << 4) & 0xF0));
        buffer[index] = encode;
      }
      memcpy(&new_list->login, buffer, sizeof(psplogin_t));
      new_list->next = loc_head_login_list;
      loc_head_login_list = new_list;
    }
    gzclose(file_desc);
  }

  /* Reverse list */
  if (loc_head_login_list) {
    scan_list = loc_head_login_list;
    next_list = scan_list->next;
    prev_list = NULL;
    while (next_list != NULL) {
      scan_list->next = prev_list;
      prev_list = scan_list;
      scan_list = next_list;
      next_list = scan_list->next;
    }
    scan_list->next = prev_list;
    loc_head_login_list = scan_list;
  }

  psp_login_current = loc_head_login_list;

  if (! psp_login_current) {
    psp_default_login_list();
  }

  return error;
}

static void 
psp_display_screen_menu(void)
{
  char buffer[64];
  int menu_id = 0;
  int color   = 0;
  int x       = 0;
  int y       = 0;
  int y_step  = 0;

  {
    psp_sdl_blit_login();

    psp_sdl_draw_rectangle(10,10,459,249,PSP_MENU_BORDER_COLOR,0);
    psp_sdl_draw_rectangle(11,11,457,247,PSP_MENU_BORDER_COLOR,0);

    psp_sdl_back_print( 30, 6, " Login ", PSP_MENU_NOTE_COLOR);
    psp_sdl_back_print( 310, 6, " <- Profile -> ",  PSP_MENU_BORDER_COLOR);

    psp_display_screen_menu_battery();

    psp_sdl_back_print(30, 254, " O/X: Valid  Start: Keyboard ", 
                       PSP_MENU_BORDER_COLOR);

    psp_sdl_back_print(370, 254, " By Zx-81 ",
                       PSP_MENU_AUTHOR_COLOR);
  }
  
  x      = 20;
  y      = 25;
  y_step = 10;
  
  for (menu_id = 0; menu_id < MAX_MENU_LOGIN_ITEM; menu_id++) {
    color = PSP_MENU_TEXT_COLOR;
    if (cur_menu_id == menu_id) color = PSP_MENU_SEL_COLOR;
    else 
    if (menu_id == MENU_LOGIN_EXIT) color = PSP_MENU_WARNING_COLOR;
    else
    if (menu_id == MENU_LOGIN_HELP) color = PSP_MENU_GREEN_COLOR;

    psp_sdl_back_print(x, y, menu_list[menu_id].title, color);

    if (menu_id == MENU_LOGIN_HOST) {
      sprintf(buffer, "%s", psp_login_current->login.host);
      if (menu_id == cur_menu_id) strcat(buffer, "_");
      string_fill_with_space(buffer, MAX_HOST_LENGTH);
      psp_sdl_back_print(120, y, buffer, color);
    } else
    if (menu_id == MENU_LOGIN_USER) {
      sprintf(buffer, "%s", psp_login_current->login.user);
      if (menu_id == cur_menu_id) strcat(buffer, "_");
      string_fill_with_space(buffer, MAX_USER_LENGTH);
      psp_sdl_back_print(120, y, buffer, color);
    } else
    if (menu_id == MENU_LOGIN_PASSWORD) {
      int length = strlen(psp_login_current->login.password);
      memset(buffer, '*', length);
      buffer[length] = 0;
      if (menu_id == cur_menu_id) strcat(buffer, "_");
      string_fill_with_space(buffer, MAX_PASSWORD_LENGTH);
      psp_sdl_back_print(120, y, buffer, color);
    } else
    if (menu_id == MENU_LOGIN_NICKNAME) {
      sprintf(buffer, "%s", psp_login_current->login.nickname);
      if (menu_id == cur_menu_id) strcat(buffer, "_");
      string_fill_with_space(buffer, MAX_USER_LENGTH);
      psp_sdl_back_print(120, y, buffer, color);
    } else
    if (menu_id == MENU_LOGIN_REALNAME) {
      sprintf(buffer, "%s", psp_login_current->login.realname);
      if (menu_id == cur_menu_id) strcat(buffer, "_");
      string_fill_with_space(buffer, MAX_USER_LENGTH);
      psp_sdl_back_print(120, y, buffer, color);
    } else
    if (menu_id == MENU_LOGIN_PORT) {
      sprintf(buffer, "%s", psp_login_current->login.port);
      if (menu_id == cur_menu_id) strcat(buffer, "_");
      string_fill_with_space(buffer, MAX_PORT_LENGTH);
      psp_sdl_back_print(120, y, buffer, color);
    } else
    if (menu_id == MENU_LOGIN_CHANNEL) {
      sprintf(buffer, "%s", psp_login_current->login.channel);
      if (menu_id == cur_menu_id) strcat(buffer, "_");
      string_fill_with_space(buffer, MAX_CHANNEL_LENGTH);
      psp_sdl_back_print(120, y, buffer, color);
    } else
    if (menu_id == MENU_LOGIN_SCRIPT) {
      sprintf(buffer, "%s", psp_login_current->login.script);
      string_fill_with_space(buffer, MAX_SCRIPT_LENGTH);
      psp_sdl_back_print(120, y, buffer, color);
      y += y_step;
    } else
    if (menu_id == MENU_LOGIN_HELP) {
      y += y_step;
    } else
    if (menu_id == MENU_LOGIN_DELETE) {
      y += y_step;
    } else
    if (menu_id == MENU_LOGIN_SAVE) {
      y += y_step;
    } else
    if (menu_id == MENU_LOGIN_CONNECT) {
      y += y_step;
    }

    y += y_step;
  }
}

static void
psp_login_menu_save()
{
  int error;

  error = psp_save_login_list();

  if (! error) /* save OK */
  {
    psp_display_screen_menu();
    psp_sdl_back_print(270, 180, "File saved !", PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  else 
  {
    psp_display_screen_menu();
    psp_sdl_back_print(270, 180, "Can't save file !", 
                       PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
}

static void
psp_login_menu_load()
{
  int error;

  error = psp_load_login_list();

  if (! error) /* save OK */
  {
    psp_display_screen_menu();
    psp_sdl_back_print(270, 180, "File loaded !", PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  else 
  {
    psp_display_screen_menu();
    psp_sdl_back_print(270, 180, "Can't load file !", 
                       PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
}

static void
psp_login_menu_delete()
{
  psplogin_list_t  *scan_list;
  psplogin_list_t **prev_list;

  scan_list = loc_head_login_list;
  prev_list = &loc_head_login_list;

  while (scan_list != 0) {

    if (scan_list == psp_login_current) {
      *prev_list = scan_list->next;
      psp_login_current = scan_list->next;
      if (psp_login_current == NULL) {
        if (loc_head_login_list != NULL) {
          psp_login_current = loc_head_login_list;
        } else {
          psp_default_login_list();
        }
      }
      free(scan_list);
      break;
    }
    prev_list = &scan_list->next;
    scan_list = scan_list->next;
  }

}

static int
psp_login_menu_set_as_default()
{
  psplogin_list_t  *scan_list;
  psplogin_list_t **prev_list;

  scan_list = loc_head_login_list;
  prev_list = &loc_head_login_list;

  while (scan_list != 0) {

    if (scan_list == psp_login_current) {
      *prev_list = scan_list->next;
      scan_list->next = loc_head_login_list;
      loc_head_login_list = scan_list;
      break;
    }
    prev_list = &scan_list->next;
    scan_list = scan_list->next;
  }
  return 0;
}


static void
psp_login_menu_new()
{
  psplogin_list_t *new_list;

  new_list = malloc(sizeof(psplogin_list_t));
  memset(new_list, 0, sizeof(psplogin_list_t));
  new_list->next = loc_head_login_list;
  loc_head_login_list = new_list;
  strcpy(new_list->login.user, "");
  strcpy(new_list->login.password, "");
  strcpy(new_list->login.nickname, "");
  strcpy(new_list->login.host, "");
  strcpy(new_list->login.port, "6667");
  strcpy(new_list->login.channel, "#");
  strcpy(new_list->login.script, "");
  psp_login_current = new_list;

  cur_menu_id = MENU_LOGIN_HOST;
}

int
psp_login_menu_exit(void)
{
  SceCtrlData c;

  psp_display_screen_menu();
  psp_sdl_back_print(270, 180, "press X to confirm !", PSP_MENU_WARNING_COLOR);
  psp_sdl_flip();

  psp_kbd_wait_no_button();

  do
  {
    sceCtrlReadBufferPositive(&c, 1);
    c.Buttons &= PSP_ALL_BUTTON_MASK;

    if (c.Buttons & PSP_CTRL_CROSS) {
      psp_sdl_exit(0);
    }

  } while (c.Buttons == 0);

  psp_kbd_wait_no_button();

  return 0;
}

void
psp_login_menu_connect()
{
  if (!psp_login_current->login.nickname[0]) {
    strcpy(psp_login_current->login.nickname, psp_login_current->login.user);
  }
  if (!psp_login_current->login.realname[0]) {
    strcpy(psp_login_current->login.realname, psp_login_current->login.user);
  }

  if ( (!psp_login_current->login.user[0]) ||
       (!psp_login_current->login.nickname[0]) ||
       (!psp_login_current->login.realname[0]) ||
       (!psp_login_current->login.host[0]) ||
       (!psp_login_current->login.port[0])) {

    psp_display_screen_menu();
    psp_sdl_back_print(250, 180, "Invalid parameters !", PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
    return;
  }

  /* Initialize console */
  psp_console_init();
  psp_console_clear_screen();

  psp_display_screen_menu();
  psp_sdl_back_print(250, 180, "starting irc client ...", PSP_MENU_NOTE_COLOR);
  psp_sdl_flip();

  irc_thread_start_proto_thread();
  sceKernelDelayThread(100000);

  psp_irc_main_loop();

  psp_display_screen_menu();
  psp_sdl_back_print(250, 180, "closing irc client ...", PSP_MENU_NOTE_COLOR);
  psp_sdl_flip();

  irc_thread_stop_proto_thread();

  sceKernelDelayThread(1000000);

  psp_console_destroy();
}

void
psp_login_put_key(int key)
{
  char *scan_str   = NULL;
  int   length     = 0;
  int   max_length = 0;

  switch (cur_menu_id) 
  {
    case MENU_LOGIN_HOST     : scan_str = psp_login_current->login.host;
                               max_length = MAX_HOST_LENGTH;
    break;
    case MENU_LOGIN_USER     : scan_str = psp_login_current->login.user;
                               max_length = MAX_USER_LENGTH;
    break;
    case MENU_LOGIN_PASSWORD : scan_str = psp_login_current->login.password;
                               max_length = MAX_PASSWORD_LENGTH;
    break;
    case MENU_LOGIN_NICKNAME : scan_str = psp_login_current->login.nickname;
                               max_length = MAX_NICKNAME_LENGTH;
    break;
    case MENU_LOGIN_REALNAME : scan_str = psp_login_current->login.realname;
                               max_length = MAX_REALNAME_LENGTH;
    break;
    case MENU_LOGIN_PORT     : scan_str = psp_login_current->login.port;
                               max_length = MAX_PORT_LENGTH;
    break;
    case MENU_LOGIN_CHANNEL  : scan_str = psp_login_current->login.channel;
                               max_length = MAX_CHANNEL_LENGTH;
    break;
  }

  if (! scan_str) return;

  if (key == DANZEFF_CLEAR) {
    if (cur_menu_id != MENU_LOGIN_CHANNEL) {
      scan_str[0] = '\0';
    } else {
      length = 1;
      scan_str[0] = '#';
      scan_str[1] = 0;
    }
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
  
    if (cur_menu_id == MENU_LOGIN_PORT) {
      if ((key < '0') || (key > '9')) return;
    }
    if ((length + 1) >= max_length) return;
    scan_str[length] = key;
    length++;
    scan_str[length] = 0;
  }
  if (cur_menu_id == MENU_LOGIN_CHANNEL) {
    if (length <= 0) {
      length = 1;
      scan_str[0] = '#';
      scan_str[1] = 0;
    }
  }
}

int
psp_login_menu_script(void)
{
  char dir_name[MAX_PATH];
  snprintf(dir_name, MAX_PATH, "%s/run/", PSPIRC.psp_homedir);
  char* script_filename = psp_fmgr_menu(dir_name, 0);
  char* scan = 0;
  if (script_filename) {
    scan = strrchr(script_filename, '/');
  }
  if (scan) {
    strcpy(psp_login_current->login.script, scan + 1);
  } else {
    psp_login_current->login.script[0] = 0;
  }
}


static void
psp_login_go_right()
{
  psp_login_current = psp_login_current->next;
  if (psp_login_current == NULL) psp_login_current = loc_head_login_list;
}

static void
psp_login_go_left()
{
  int              found = 0;
  psplogin_list_t *scan_list;

  for (scan_list        = loc_head_login_list;
       scan_list->next != (psplogin_list_t *)0;
       scan_list        = scan_list->next) {
    if (scan_list->next == psp_login_current) {
      psp_login_current = scan_list;
      found = 1;
      break;
    }
  }
  if (! found) {
    psp_login_current = scan_list;
  }
}

# define PSP_ALL_BUTTON_MASK 0xFFFF

int 
psp_login_menu(void)
{
  int  danzeff_mode;
  int  danzeff_key;
  int  irkeyb_key;

  SceCtrlData c;
  long        new_pad;
  long        old_pad;
  int         last_time;
  int         end_menu;

  psp_load_login_list();

  psp_kbd_wait_no_button();

  old_pad      = 0;
  last_time    = 0;
  end_menu     = 0;
  danzeff_mode = 0;

# ifdef USE_PSP_IRKEYB
  irkeyb_key = PSP_IRKEYB_EMPTY;
# endif

  while (! end_menu)
  {
    psp_display_screen_menu();

    if (danzeff_mode) {
      danzeff_moveTo(-50, -50);
      danzeff_render();
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

# ifdef USE_PSP_IRKEYB
    irkeyb_key = psp_irkeyb_read_key();

    if (irkeyb_key > 0) {
      if (irkeyb_key > ' ') {
        psp_login_put_key(irkeyb_key);
      } else
      if (irkeyb_key == 0x8) {
        psp_login_put_key(DANZEFF_DEL);
      } else
      if (irkeyb_key == 0xd) {
        new_pad |= PSP_CTRL_CROSS;
      } else /* ctrl L */
      if (irkeyb_key == 0xc) {
        psp_login_put_key(DANZEFF_CLEAR);
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
          psp_login_put_key(danzeff_key);
        } else
        if ((danzeff_key == DANZEFF_DEL ) ||
            (danzeff_key == DANZEFF_LEFT)) {
          psp_login_put_key(DANZEFF_DEL);
        } else
        if (danzeff_key == DANZEFF_CLEAR) {
          psp_login_put_key(DANZEFF_CLEAR);
        } else
        if ((danzeff_key == DANZEFF_ENTER) ||
            (danzeff_key == DANZEFF_DOWN )) {
          cur_menu_id = cur_menu_id + 1;
          if (cur_menu_id > MENU_LOGIN_CHANNEL) cur_menu_id = MENU_LOGIN_HOST;
        } else
        if (danzeff_key == DANZEFF_UP) {
          cur_menu_id = cur_menu_id - 1;
          if (cur_menu_id < 0) cur_menu_id = MENU_LOGIN_CHANNEL;
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

    if ((c.Buttons & (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) ==
        (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) {
      /* Exit ! */
      psp_sdl_exit(0);
    } else
    if(new_pad & PSP_CTRL_START) {

      danzeff_mode = 1;
      if (cur_menu_id >= MENU_LOGIN_SCRIPT) {
        cur_menu_id = MENU_LOGIN_HOST;
      }

    } else
    if ((new_pad & PSP_CTRL_CROSS ) || 
        (new_pad & PSP_CTRL_CIRCLE))
    {
      switch (cur_menu_id ) 
      {
        case MENU_LOGIN_NEW       : psp_login_menu_new();
        break;

        case MENU_LOGIN_DELETE    : psp_login_menu_delete();
        break;

        case MENU_LOGIN_DEFAULT   : psp_login_menu_set_as_default();
        break;

        case MENU_LOGIN_CONNECT   : psp_login_menu_connect();
        break;

        case MENU_LOGIN_BACK      : end_menu = 1;
        break;

        case MENU_LOGIN_SAVE      : psp_login_menu_save();
        break;

        case MENU_LOGIN_LOAD      : psp_login_menu_load();
        break;

        case MENU_LOGIN_EXIT      : psp_login_menu_exit();
        break;

        case MENU_LOGIN_SCRIPT    : psp_login_menu_script();
        break;

        case MENU_LOGIN_HELP      : psp_help_menu();
                                    old_pad = new_pad = 0;
        break;              
      }

    } else
    if(new_pad & PSP_CTRL_UP) {

      if (cur_menu_id > 0) cur_menu_id--;
      else                 cur_menu_id = MAX_MENU_LOGIN_ITEM-1;

    } else
    if(new_pad & PSP_CTRL_DOWN) {

      if (cur_menu_id < (MAX_MENU_LOGIN_ITEM-1)) cur_menu_id++;
      else                                       cur_menu_id = 0;

    } else
    if(new_pad & PSP_CTRL_RIGHT) {
      psp_login_go_right();
    } else
    if(new_pad & PSP_CTRL_LEFT) {
      psp_login_go_left();
    } else
    if(new_pad & PSP_CTRL_RTRIGGER) {
      psp_login_go_right();
    } else
    if(new_pad & PSP_CTRL_LTRIGGER) {
      psp_login_go_left();
    }
  }
 
  psp_kbd_wait_no_button();

  return 1;
}

