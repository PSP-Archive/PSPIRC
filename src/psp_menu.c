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
#include "psp_fmgr.h"
#include "psp_console.h"
#include "psp_menu_chanlist.h"
#include "psp_menu_user.h"
#include "psp_menu_tab.h"
#include "psp_menu_chantab.h"
#include "psp_menu_config.h"
#include "irc_thread.h"

# define MENU_SCREENSHOT   0
# define MENU_SAVE_LOG     1
# define MENU_HELP         2
# define MENU_COMMAND      3
# define MENU_WORD         4
# define MENU_COLOR        5
# define MENU_EDIT_COMMAND 6
# define MENU_EDIT_WORD    7
# define MENU_SEARCH_CHAN  8
# define MENU_USER         9
# define MENU_CHAN_TAB    10
# define MENU_CLOSE_TAB   11
# define MENU_SCRIPT      12
# define MENU_SETTINGS    13
# define MENU_BACK        14
# define MENU_DISCONNECT  15

# define MAX_MENU_ITEM (MENU_DISCONNECT+ 1)

  static menu_item_t menu_list[] =
  { 
    { "Save Screenshot :" },
    { "Save Log        :" },

    { "Help" },

    { "Paste Command   :" },
    { "Paste Word      :" },
    { "Paste Color     :" },

    { "Edit Command" },
    { "Edit Word" },
    { "Search Channels" },
    { "Users" },
    { "Channel tabs" },
    { "Close tab" },
    { "Run script" },

    { "Settings" },

    { "Back"  },

    { "Disconnect" }
  };

# define MAX_COMMAND_LINE      64

  static char* psp_command[MAX_COMMAND_LINE];
  static int   psp_command_size    = -1;
  static int   psp_command_current = 0;

# define MAX_WORD_LINE         512

  static char* psp_word[MAX_WORD_LINE];
  static int   psp_word_size     = -1;
  static int   psp_word_current  = 0;

  static int   psp_color_current = 0;

  static int cur_menu_id = MENU_COMMAND;

void
string_fill_with_space(char *buffer, int size)
{
  int length = strlen(buffer);
  int index;

  for (index = length; index < size; index++) {
    buffer[index] = ' ';
  }
  buffer[size] = 0;
}

void
psp_display_screen_menu_battery(void)
{
  char buffer[64];

  int Length;
  int color;

  snprintf(buffer, 50, " [%s] ", psp_get_battery_string());
  Length = strlen(buffer);

  if (psp_is_low_battery()) color = PSP_MENU_RED_COLOR;
  else                      color = PSP_MENU_BORDER_COLOR;

  psp_sdl_back_print(240 - ((8*Length) / 2), 6, buffer, color);
}

void 
psp_irc_menu_banlist(char *chan_name)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "/MODE %s +b", chan_name);
  irc_thread_send_message(chan_name, buffer);
  psp_console_tab_first();
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
    psp_sdl_blit_menu();

    psp_sdl_draw_rectangle(10,10,459,249,PSP_MENU_BORDER_COLOR,0);
    psp_sdl_draw_rectangle(11,11,457,247,PSP_MENU_BORDER_COLOR,0);

    psp_sdl_back_print(  30, 6, " Console ", PSP_MENU_NOTE_COLOR);

    psp_display_screen_menu_battery();

    psp_sdl_back_print(30, 254, " O/X: Valid  Sel: Back  Start: Keyboard ", 
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
    if (menu_id == MENU_DISCONNECT) color = PSP_MENU_WARNING_COLOR;
    else
    if (menu_id == MENU_HELP) color = PSP_MENU_GREEN_COLOR;

    psp_sdl_back_print(x, y, menu_list[menu_id].title, color);

    if (menu_id == MENU_SCREENSHOT) {
      sprintf(buffer,"%d", PSPIRC.psp_screenshot_id);
      string_fill_with_space(buffer, 4);
      psp_sdl_back_print(160, y, buffer, color);
    } else
    if (menu_id == MENU_SAVE_LOG) {
      sprintf(buffer,"%d", PSPIRC.psp_log_id);
      string_fill_with_space(buffer, 4);
      psp_sdl_back_print(160, y, buffer, color);
      y += y_step;
    } else
    if (menu_id == MENU_HELP) {
      y += y_step;
    } else
    if (menu_id == MENU_SCRIPT) {
      y += y_step;
    } else
    if (menu_id == MENU_SETTINGS) {
      y += y_step;
    } else
    if (menu_id == MENU_COMMAND) {
      strncpy(buffer, psp_command[psp_command_current], 32);
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(160, y, buffer, color);
    } else
    if (menu_id == MENU_WORD) {
      strncpy(buffer, psp_word[psp_word_current], 32);
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(160, y, buffer, color);
    } else
    if (menu_id == MENU_COLOR) {
      sprintf(buffer,"%s", console_colors_name[psp_color_current]);
      string_fill_with_space(buffer, 32);
      color = console_colors[psp_color_current];
      psp_sdl_back_print(160, y, buffer, color);
      y += y_step;
    } else
    if (menu_id == MENU_BACK) {
      y += y_step;
    }

    y += y_step;
  }
}

void
psp_initialize_command()
{
  char  FileName[MAX_PATH+1];

  char  Buffer[512];
  char *Scan;
  FILE* FileDesc;

  /* Already done ? */
  if (psp_command_size > 0) return; 

  getcwd(FileName, MAX_PATH);
  strcat(FileName, "/command.txt");
  FileDesc = fopen(FileName, "r");

  psp_command_current = 0;
  psp_command_size = 0;

  if (FileDesc != (FILE *)0 ) {

    while (fgets(Buffer,512, FileDesc) != (char *)0) {

      Scan = strchr(Buffer,'\n');
      if (Scan) *Scan = '\0';
      /* For this #@$% of windows ! */
      Scan = strchr(Buffer,'\r');
      if (Scan) *Scan = '\0';

      psp_command[psp_command_size++] = strdup(Buffer);
      if (psp_command_size >= MAX_COMMAND_LINE) break;
    }
    fclose(FileDesc);
  }

  if (psp_command_size == 0) {
    psp_command[0] = strdup( "ls");
    psp_command[1] = strdup( "clear");
    psp_command_size = 2;
  }
}

void
psp_command_go_right()
{
  psp_initialize_command();

  psp_command_current++;
  if (psp_command_current >= psp_command_size) psp_command_current = 0;
}

void
psp_command_go_fast_right()
{
  psp_command_current += 10;
  if (psp_command_current >= psp_command_size) psp_command_current = psp_command_size-1;
}

void
psp_command_go_left()
{
  psp_initialize_command();

  psp_command_current--;
  if (psp_command_current < 0) psp_command_current = psp_command_size-1;
}

void
psp_command_go_fast_left()
{
  psp_command_current -= 10;
  if (psp_command_current < 0) psp_command_current = 0;
}

char*
psp_get_current_command()
{
  psp_initialize_command();
  return psp_command[psp_command_current];
}

void
psp_irc_menu_command()
{
  char buffer[256];
  buffer[0] = '/';
  strcpy(buffer + 1, psp_command[psp_command_current]);
  psp_console_put_string(buffer);
}

static int
psp_find_first_command(char first_char, int check_last)
{
static char last_first_char  = 0;
static int  last_first_index = -1;

  int index  = 0;
  if ((first_char >= 'a') && (first_char <= 'z')) {
    first_char = (first_char - 'a') + 'A';
  }
  if (check_last && (last_first_index >= 0 )) {
    if (first_char == last_first_char) {
      last_first_index++;
      if (last_first_index < psp_command_size) { 
        char test_char = toupper(psp_command[last_first_index][0]);
        if (test_char == first_char) index = last_first_index;
      } else {
        last_first_index = -1;
      }
    }
  }
  while (index < psp_command_size) {
    char test_char = toupper(psp_command[index][0]);
    if (test_char == first_char) {
      last_first_index = index;
      last_first_char  = first_char;
      return index;
    }
    index++;
  }
  last_first_index = -1;
  return -1;
}

void
psp_irc_menu_find_command(char key)
{
  int index = psp_find_first_command( key, 1 );
  if (index >= 0) psp_command_current = index;
}

void
psp_initialize_word()
{
  char  FileName[MAX_PATH+1];

  char  Buffer[512];
  char *Scan;
  FILE* FileDesc;

  /* Already done ? */
  if (psp_word_size > 0) return; 

  getcwd(FileName, MAX_PATH);
  strcat(FileName, "/word.txt");
  FileDesc = fopen(FileName, "r");

  psp_word_current = 0;
  psp_word_size = 0;

  if (FileDesc != (FILE *)0 ) {

    while (fgets(Buffer,512, FileDesc) != (char *)0) {
      Scan = strchr(Buffer,'\n');
      if (Scan) *Scan = '\0';
      /* For this #@$% of windows ! */
      Scan = strchr(Buffer,'\r');
      if (Scan) *Scan = '\0';

      psp_word[psp_word_size++] = strdup(Buffer);
      if (psp_word_size >= MAX_WORD_LINE) break;
    }
    fclose(FileDesc);
  }

  if (psp_word_size == 0) {
    psp_word[0] = strdup( "hello");
    psp_word[1] = strdup( "bye");
    psp_word_size = 2;
  }
}

void
psp_word_go_right()
{
  psp_word_current++;
  if (psp_word_current >= psp_word_size) psp_word_current = 0;
}

void
psp_word_go_fast_right()
{
  psp_word_current += 10;
  if (psp_word_current >= psp_word_size) psp_word_current = psp_word_size-1;
}

void
psp_word_go_left()
{
  psp_word_current--;
  if (psp_word_current < 0) psp_word_current = psp_word_size-1;
}

void
psp_word_go_fast_left()
{
  psp_word_current -= 10;
  if (psp_word_current < 0) psp_word_current = 0;
}

void
psp_irc_menu_edit_command()
{
  char TmpFileName[MAX_PATH];
  sprintf(TmpFileName, "%s/command.txt", PSPIRC.psp_homedir);
  psp_editor_menu(TmpFileName);
  psp_command_size = 0;
  psp_initialize_command();
}

void
psp_irc_menu_edit_word()
{
  char TmpFileName[MAX_PATH];
  sprintf(TmpFileName, "%s/word.txt", PSPIRC.psp_homedir);
  psp_editor_menu(TmpFileName);
  psp_word_size = 0;
  psp_initialize_word();
}

static int
psp_find_first_word(char first_char, int check_last)
{
static char last_first_char  = 0;
static int  last_first_index = -1;

  int index  = 0;
  if ((first_char >= 'a') && (first_char <= 'z')) {
    first_char = (first_char - 'a') + 'A';
  }
  if (check_last && (last_first_index >= 0 )) {
    if (first_char == last_first_char) {
      last_first_index++;
      if (last_first_index < psp_word_size) { 
        char test_char = toupper(psp_word[last_first_index][0]);
        if (test_char == first_char) index = last_first_index;
      } else {
        last_first_index = -1;
      }
    }
  }
  while (index < psp_word_size) {
    char test_char = toupper(psp_word[index][0]);
    if (test_char == first_char) {
      last_first_index = index;
      last_first_char  = first_char;
      return index;
    }
    index++;
  }
  last_first_index = -1;
  return -1;
}

void
psp_irc_menu_find_word(char key)
{
  int index = psp_find_first_word( key, 1 );
  if (index >= 0) psp_word_current = index;
}


void
psp_irc_menu_word()
{
  psp_console_put_string(psp_word[psp_word_current]);
}

void
psp_color_go_right()
{
  psp_color_current++;
  if (psp_color_current >= (CONSOLE_MAX_COLOR-1)) psp_color_current = 0;
}

void
psp_color_go_left()
{
  psp_color_current--;
  if (psp_color_current < 0) psp_color_current = CONSOLE_MAX_COLOR-2;
}

void
psp_irc_menu_color()
{
  char buffer[12];
  sprintf(buffer, "%c%d", DANZEFF_COLOR, psp_color_current);
  psp_console_put_string(buffer);
}

void
psp_irc_menu_screenshot()
{
  PSPIRC.psp_screenshot_mode = 10;
}

void
psp_irc_menu_save_log()
{
  PSPIRC.psp_save_log_mode = 10;
}

void
psp_irc_menu_close_connection()
{
  psp_display_screen_menu();
  psp_sdl_back_print(250,  80,   "Closing connection ... ", PSP_MENU_NOTE_COLOR);
  psp_sdl_flip();

  psp_console_close_all_tabs();
}

int
psp_irc_menu_script()
{
  char dir_name[MAX_PATH];
  snprintf(dir_name, MAX_PATH, "%s/run/", PSPIRC.psp_homedir);
  char* script_filename = psp_fmgr_menu(dir_name, 0);
  if (script_filename) {
    irc_thread_run_script(script_filename);
    return 1;
  }
  return 0;
}

int 
psp_irc_main_menu(void)
{
  SceCtrlData c;
  long        new_pad;
  long        old_pad;

  int         last_time;
  int         end_menu;

  int         danzeff_mode = 0;
  int         danzeff_key;
  int         irkeyb_key;

  psp_initialize_command();
  psp_initialize_word();

  psp_kbd_wait_no_button();

  old_pad   = 0;
  last_time = 0;
  end_menu  = 0;

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
        if (cur_menu_id == MENU_COMMAND) {
          psp_irc_menu_find_command(irkeyb_key);
        } else 
        if (cur_menu_id == MENU_WORD) {
          psp_irc_menu_find_word(irkeyb_key);
        }
      } else
      if (irkeyb_key == 0xd) {
        if (cur_menu_id == MENU_COMMAND) {
          psp_irc_menu_command();
        } else 
        if (cur_menu_id == MENU_WORD) {
          psp_irc_menu_word();
        } else
        if (cur_menu_id == MENU_COLOR) {
          psp_irc_menu_color();
        }
        end_menu  = 1;
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
          if (cur_menu_id == MENU_COMMAND) {
            psp_irc_menu_find_command(danzeff_key);
          } else
          if (cur_menu_id == MENU_WORD) {
            psp_irc_menu_find_word(danzeff_key);
          }
        } else
        if ((danzeff_key == DANZEFF_ENTER) ||
            (danzeff_key == DANZEFF_DOWN )) {
          if (cur_menu_id == MENU_COMMAND) {
            psp_irc_menu_command();
          } else 
          if (cur_menu_id == MENU_WORD) {
            psp_irc_menu_word();
          } else 
          if (cur_menu_id == MENU_COLOR) {
            psp_irc_menu_color();
          }
          end_menu  = 1;

        } else
        if (danzeff_key == DANZEFF_LEFT) { /* Left */
          if (cur_menu_id == MENU_COMMAND) {
            psp_command_go_left();
          } else
          if (cur_menu_id == MENU_WORD) {
            psp_word_go_left();
          } else
          if (cur_menu_id == MENU_COLOR) {
            psp_color_go_left();
          }

        } else
        if (danzeff_key == DANZEFF_RIGHT) { /* Right */
          if (cur_menu_id == MENU_COMMAND) {
            psp_command_go_right();
          } else
          if (cur_menu_id == MENU_WORD) {
            psp_word_go_right();
          } else
          if (cur_menu_id == MENU_COLOR) {
            psp_color_go_right();
          }
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
    if ((new_pad & PSP_CTRL_CROSS ) || 
        (new_pad & PSP_CTRL_CIRCLE))
    {
      switch (cur_menu_id ) 
      {
        case MENU_COMMAND   : psp_irc_menu_command();
                              end_menu = 1;
        break;

        case MENU_WORD      : psp_irc_menu_word();
                              end_menu = 1;
        break;

        case MENU_COLOR     : psp_irc_menu_color();
                              end_menu = 1;
        break;

        case MENU_EDIT_COMMAND : psp_irc_menu_edit_command();
        break;
        case MENU_EDIT_WORD    : psp_irc_menu_edit_word();
        break;


        case MENU_SEARCH_CHAN : if (psp_irc_chanlist_menu() == 1) end_menu = 1;
        break;

        case MENU_USER      : if (psp_irc_users_menu() == 1) end_menu = 1;
        break;

        case MENU_CHAN_TAB  : if (psp_irc_chantabs_menu() == 1) end_menu = 1;
        break;

        case MENU_CLOSE_TAB : if (psp_irc_tabs_menu() == 1) end_menu = 1;
        break;

        case MENU_SETTINGS  : psp_irc_config_menu();
        break;

        case MENU_SCREENSHOT : psp_irc_menu_screenshot();
                               end_menu = 1;
        break;              

        case MENU_SAVE_LOG   : psp_irc_menu_save_log();
                               end_menu = 1;
        break;              

        case MENU_SCRIPT    : if (psp_irc_menu_script()) end_menu = 1;
        break;              

        case MENU_BACK      : end_menu = 1;
        break;

        case MENU_DISCONNECT : psp_irc_menu_close_connection();
                               end_menu = 2;
        break;

        case MENU_HELP      : psp_help_menu();
                              old_pad = new_pad = 0;
        break;              
      }

    } else
    if(new_pad & PSP_CTRL_START) {
      danzeff_mode = 1;
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
      if (cur_menu_id == MENU_COMMAND) {
        psp_command_go_right();
      } else
      if (cur_menu_id == MENU_WORD) {
        psp_word_go_right();
      } else
      if (cur_menu_id == MENU_COLOR) {
        psp_color_go_right();
      }

    } else
    if(new_pad & PSP_CTRL_LEFT) {
      if (cur_menu_id == MENU_COMMAND) {
        psp_command_go_left();
      } else
      if (cur_menu_id == MENU_WORD) {
        psp_word_go_left();
      } else
      if (cur_menu_id == MENU_COLOR) {
        psp_color_go_left();
      }
    } else
    if(new_pad & PSP_CTRL_RTRIGGER) {
      if (cur_menu_id == MENU_COMMAND) {
        psp_command_go_fast_right();
      } else 
      if (cur_menu_id == MENU_WORD) {
        psp_word_go_fast_right();
      }

    } else
    if(new_pad & PSP_CTRL_LTRIGGER) {
      if (cur_menu_id == MENU_COMMAND) {
        psp_command_go_fast_left();
      } else 
      if (cur_menu_id == MENU_WORD) {
        psp_word_go_fast_left();
      }
    }
  }
 
  psp_kbd_wait_no_button();

  return (end_menu == 2);
}

