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
#include <pspwlan.h>
#include <pspctrl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "psp_global.h"
#include "psp_sdl.h"
#include "psp_menu.h"
#include "psp_irkeyb.h"

#define MAX_PICK 5
#define MAX_PICK_TITLE 80
#define MAX_PICK_MAINSTR 13
#define MAX_PICK_FINEPRINT 45

typedef struct
{
    char szBig[MAX_PICK_MAINSTR+1]; // big font
    char szFinePrint[MAX_PICK_FINEPRINT+1]; // small font
    u32 userData;
} PICKER_INFO;

typedef struct
{
    int         pick_count;
    PICKER_INFO picks[MAX_PICK];
    int         pick_start; // -1 for none, 0->pick_count-1 for selection

} PICKER;

void 
my_initpicker(PICKER* pickerP)
{
  pickerP->pick_count = 0;
  pickerP->pick_start = -1;
}

int 
my_addpick(PICKER* pickerP, const char* szBig, const char* szFinePrint, u32 userData)
{
    PICKER_INFO* pickP;
    if (pickerP->pick_count >= MAX_PICK)
        return 0; // no room
        // REVIEW: in future provide a smaller font version with more slots
    pickP = &(pickerP->picks[pickerP->pick_count++]);

    strncpy(pickP->szBig, szBig, MAX_PICK_MAINSTR);
    pickP->szBig[MAX_PICK_MAINSTR] = '\0';
    if (szFinePrint == NULL)
        szFinePrint = "";
    strncpy(pickP->szFinePrint, szFinePrint, MAX_PICK_FINEPRINT);
    pickP->szFinePrint[MAX_PICK_FINEPRINT] = '\0';
    pickP->userData = userData;
    return 1;
}

static void
psp_wifi_display_screen()
{
  psp_sdl_blit_wifi();

  psp_sdl_draw_rectangle(10,10,459,249,PSP_MENU_BORDER_COLOR,0);
  psp_sdl_draw_rectangle(11,11,457,247,PSP_MENU_BORDER_COLOR,0);

  psp_sdl_back_print( 30, 6, " Wifi connection ", PSP_MENU_NOTE_COLOR);

  char buffer[32];
  sprintf(buffer, " %s ", PSPIRC_VERSION);
  int len = strlen(buffer);
  psp_sdl_back_print(460 - (8 * len), 6, buffer, PSP_MENU_BORDER_COLOR);

  psp_display_screen_menu_battery();

  psp_sdl_back_print(30, 254, " []: Exit  X: Valid  Tri: Cancel ", 
                     PSP_MENU_BORDER_COLOR);

  psp_sdl_back_print(370, 254, " By Zx-81 ",
                     PSP_MENU_AUTHOR_COLOR);
}


#define Y_PICK(i)    (4+3*(i))
#define X_PICK       (8)
#define STR_PICK "=>"

void
my_psp_sdl_back_print(int x, int y, char *text, int color)
{
  psp_sdl_back_print(x * 8, y * 8, text, color);
}

int
my_picker(const PICKER* pickerP)
{
  SceCtrlData c;
  long        new_pad = 0;
  long        old_pad = 0;

  int         last_time = 0;
  int         irkeyb_key;

    int iPick = pickerP->pick_start;
    int bRedraw = 1;

    if (pickerP->pick_count == 0) {
        return -1; // nothing to pick
    }

# ifdef USE_PSP_IRKEYB
    irkeyb_key = PSP_IRKEYB_EMPTY;
# endif

    while (1)
    {
      int i;

# if 0 //LUDO:
      if (pickerP->pick_count == 1) {
        // auto-pick if only one
        iPick = 0;
        break;
      }
# endif

      if (bRedraw) {
        psp_wifi_display_screen();

        for (i = 0; i < pickerP->pick_count; i++)
        {
          PICKER_INFO const* pickP = &pickerP->picks[i];
          int text_color = PSP_MENU_TEXT_COLOR;
          if (i == iPick)
          {
            text_color = PSP_MENU_SEL_COLOR;
            my_psp_sdl_back_print(4, Y_PICK(i), STR_PICK, text_color);
          }
          my_psp_sdl_back_print(X_PICK, Y_PICK(i), pickP->szBig, text_color);
          my_psp_sdl_back_print(X_PICK + 2, Y_PICK(i) + 1, pickP->szFinePrint, text_color);
        }
        bRedraw = 0;
        psp_sdl_flip();
      }

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
      if (irkeyb_key == 0xd) new_pad |= PSP_CTRL_CROSS;
# endif

      if (new_pad & PSP_CTRL_DOWN) {
        iPick++;
        if (iPick >= pickerP->pick_count)
            iPick = pickerP->pick_count-1;
        bRedraw = 1;
      }
      else if (new_pad & PSP_CTRL_UP) {
        iPick--;
        if (iPick < 0) iPick = 0;
        bRedraw = 1;
      }
      else if (new_pad & PSP_CTRL_CROSS) {
        if (iPick >= 0 && iPick < pickerP->pick_count) {
              break; // picked !
        }
      }
      else if (new_pad & PSP_CTRL_TRIANGLE)
      {
        iPick = -1; // cancel
        break;
      }
      else if (new_pad & PSP_CTRL_SQUARE)
      {
        iPick = -2; // exit
        break;
      }
    }
    return iPick;
}

void 
psp_wifi_connect()
{
  int last_state;
  u32 err;
  char szMyIPAddr[32];
  int connectionConfig = -1;
  PICKER pickConn; // connection picker
  int iNetIndex;
  int iPick;
  int end_wifi = 0;

  while (! end_wifi) {

    psp_wifi_display_screen();
    psp_sdl_flip();

    err = pspSdkInetInit();

    if (err != 0) {
      fprintf(stdout, "nlhInit: %d !\n", err);
      pspSdkInetTerm();
      return;
    }

    while (sceWlanGetSwitchState() != 1)  //switch = off
    {
      psp_wifi_display_screen();
      my_psp_sdl_back_print(4, 4, "Turn the wifi switch on !", PSP_MENU_RED_COLOR);
      psp_sdl_flip();
      sceKernelDelayThread(1000000);
    }

    // enumerate connections
    my_initpicker(&pickConn);

    for (iNetIndex = 1; iNetIndex < 100; iNetIndex++) // skip the 0th connection
    {
      char data[128];
      char name[128];
      char detail[128];
      if (sceUtilityCheckNetParam(iNetIndex) != 0) continue;
      sceUtilityGetNetParam(iNetIndex, 0, name);

      sceUtilityGetNetParam(iNetIndex, 1, data);
      strcpy(detail, "SSID=");
      strcat(detail, data);

      sceUtilityGetNetParam(iNetIndex, 4, data);
      if (data[0]) {
        // not DHCP
        sceUtilityGetNetParam(iNetIndex, 5, data);
        strcat(detail, " IPADDR=");
        strcat(detail, data);
      } else {
        strcat(detail, " DHCP");
      }
      my_addpick(&pickConn, name, detail, (u32)iNetIndex);
      if (pickConn.pick_count >= MAX_PICK) break;  // no more
    }

    if (pickConn.pick_count == 0) {
      psp_wifi_display_screen();
      my_psp_sdl_back_print(4,4, "No wifi connection, please create one !", PSP_MENU_TEXT_COLOR);
      psp_sdl_flip();
      sceKernelDelayThread(3000000);
      goto close_net;
    }

    pickConn.pick_start = 0;
    iPick = my_picker(&pickConn);
    if (iPick == -2) end_wifi = 1;
    if (iPick <= -1) goto close_net;
    connectionConfig = (int)(pickConn.picks[iPick].userData);

connect_wifi:

    err = sceNetApctlConnect(connectionConfig);
    if (err != 0) {
      goto close_net;
    }

    {
      char buffer[128];
      psp_wifi_display_screen();
      strcpy(buffer, "Trying to connect to ");
      strcat(buffer, pickConn.picks[iPick].szBig);
      my_psp_sdl_back_print(4, 4, buffer, PSP_MENU_TEXT_COLOR);
      psp_sdl_flip();
    }

    // 4 connected with IP address
    last_state = -1;

    while (1) 
    {
      int state = 0;
      char buffer[128];

      sceKernelDelayThread(50*1000);
      err = sceNetApctlGetState(&state);

      if (err != 0)
      {
        psp_wifi_display_screen();
        my_psp_sdl_back_print(4, 4, "Connection failed !", PSP_MENU_TEXT_COLOR);
        psp_sdl_flip();

        goto close_connection;
      }

      {
        SceCtrlData c;
        myCtrlPeekBufferPositive(&c, 1);
        c.Buttons &= PSP_ALL_BUTTON_MASK;

        if (c.Buttons & PSP_CTRL_SQUARE) {
          end_wifi = 1; goto close_connection;
        }
        if (c.Buttons & PSP_CTRL_TRIANGLE) {
          goto close_connection;
        }
      }
  
      if (state != last_state) 
      {
        if (last_state == 2 && state == 0)
        {
          psp_wifi_display_screen();
          my_psp_sdl_back_print(4, 4, "Connecting to wifi Failed, Retrying...", PSP_MENU_TEXT_COLOR);
          psp_sdl_flip();

          sceKernelDelayThread(500*1000); // 500ms
          goto connect_wifi;
        }
     
        psp_wifi_display_screen();
        sprintf(buffer, "Connection state %d of 4", state);
        my_psp_sdl_back_print(4, 4, buffer, PSP_MENU_TEXT_COLOR);
        psp_sdl_flip();

        last_state = state;
      }
  
      // 0 - idle
      // 1,2 - starting up
      // 3 - waiting for dynamic IP
      // 4 - got IP - usable
      if (state == 4) break;  // connected with static IP
    }
  
    // get IP address
    if (sceNetApctlGetInfo(8, szMyIPAddr) != 0) {
      strcpy(szMyIPAddr, "unknown IP address");
    }
  
    /* Let's go to login menu now */
    psp_login_menu();
  
  close_connection:
    err = sceNetApctlDisconnect();
  
  close_net:
    psp_wifi_display_screen();
    my_psp_sdl_back_print(4, 4, "Closing connection ...", PSP_MENU_TEXT_COLOR);
    psp_sdl_flip();
    
    pspSdkInetTerm();
    sceKernelDelayThread(1000000); 
  }
}
