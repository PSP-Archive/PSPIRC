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
#include <zlib.h>
#include "SDL.h"

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspsdk.h>
#include <pspctrl.h>
#include <pspthreadman.h>

#include <pspwlan.h>
#include <pspkernel.h>
#include <psppower.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "psp_global.h"
#include "psp_console.h"

  PSPIRC_t PSPIRC;

  int psp_exit_now = 0;

void
myCtrlPeekBufferPositive( SceCtrlData* pc, int count )
{
  if (psp_exit_now) psp_sdl_exit(0);
  sceCtrlPeekBufferPositive( pc, count );
}

void
psp_global_init()
{
  memset(&PSPIRC, 0, sizeof(PSPIRC));
  getcwd(PSPIRC.psp_homedir, sizeof(PSPIRC.psp_homedir));
  PSPIRC.psp_screenshot_id = 0;
  PSPIRC.psp_log_id = 0;
  PSPIRC.psp_screenshot_mode = 0;
  PSPIRC.psp_save_log_mode = 0;

  PSPIRC.irc_user_color = COLOR_NONE;
  PSPIRC.irc_other_color = COLOR_NONE;
  PSPIRC.irc_multi_color = 0;
  PSPIRC.irc_start_bottom = 1;
  PSPIRC.irc_time_stamp = 0;

  psp_global_load_config();
}

int
psp_global_save_config()
{
  char  FileName[MAX_PATH+1];
  FILE* FileDesc;
  int   error;

  snprintf(FileName, MAX_PATH, "%s/pspirc.cfg", PSPIRC.psp_homedir);
  error = 0;
  FileDesc = fopen(FileName, "w");
  if (FileDesc != (FILE *)0 ) {
    fprintf(FileDesc, "irc_user_color=%d\n"  , PSPIRC.irc_user_color);
    fprintf(FileDesc, "irc_other_color=%d\n" , PSPIRC.irc_other_color);
    fprintf(FileDesc, "irc_multi_color=%d\n" , PSPIRC.irc_multi_color);
    fprintf(FileDesc, "irc_start_bottom=%d\n", PSPIRC.irc_start_bottom);
    fprintf(FileDesc, "irc_time_stamp=%d\n"  , PSPIRC.irc_time_stamp);
    fclose(FileDesc);
  } else {
    error = 1;
  }

  return error;
}

unsigned char
psp_convert_utf8_to_iso_8859_1(unsigned char c1, unsigned char c2)
{
  unsigned char res = 0;
  if (c1 == 0xc2) res = c2;
  else
  if (c1 == 0xc3) res = c2 | 0x40;
  return res;
}


int
psp_global_load_config()
{
  char  Buffer[512];
  char  FileName[MAX_PATH+1];
  char *Scan;
  FILE* FileDesc;
  int   error;
  unsigned int Value;

  snprintf(FileName, MAX_PATH, "%s/pspirc.cfg", PSPIRC.psp_homedir);
  error = 0;
  FileDesc = fopen(FileName, "r");
  if (FileDesc == (FILE *)0 ) return 1;

  while (fgets(Buffer,512, FileDesc) != (char *)0) {

    Scan = strchr(Buffer,'\n');
    if (Scan) *Scan = '\0';
    /* For this #@$% of windows ! */
    Scan = strchr(Buffer,'\r');
    if (Scan) *Scan = '\0';
    if (Buffer[0] == '#') continue;

    Scan = strchr(Buffer,'=');
    if (! Scan) continue;

    *Scan = '\0';
    Value = atoi(Scan+1);
    if (!strcasecmp(Buffer,"irc_user_color")) PSPIRC.irc_user_color = Value;
    else
    if (!strcasecmp(Buffer,"irc_other_color")) PSPIRC.irc_other_color = Value;
    else
    if (!strcasecmp(Buffer,"irc_multi_color")) PSPIRC.irc_multi_color = Value;
    else
    if (!strcasecmp(Buffer,"irc_start_bottom")) PSPIRC.irc_start_bottom = Value;
    else
    if (!strcasecmp(Buffer,"irc_time_stamp")) PSPIRC.irc_time_stamp = Value;
  }

  fclose(FileDesc);

  return 0;
}

