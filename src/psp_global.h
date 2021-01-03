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

# ifndef _PSP_GLOBAL_H_
# define _PSP_GLOBAL_H_

#include <pspctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

# define MAX_PATH    512
# define PSP_ALL_BUTTON_MASK 0xFFFF

  typedef struct PSPIRC_t {

    char psp_homedir[MAX_PATH];
    int  psp_screenshot_id;
    int  psp_log_id;
    int  psp_screenshot_mode;
    int  psp_save_log_mode;

    int  irc_user_color;
    int  irc_other_color;
    int  irc_multi_color;
    int  irc_start_bottom;
    int  irc_time_stamp;

  } PSPIRC_t;

  extern PSPIRC_t PSPIRC;

  extern int psp_global_save_config();
  extern int psp_global_load_config();

  extern unsigned char psp_convert_utf8_to_iso_8859_1(unsigned char c1, unsigned char c2);
  extern int psp_exit_now;
  extern void myCtrlPeekBufferPositive( SceCtrlData* pc, int count );

#ifdef __cplusplus
}
#endif

# endif
