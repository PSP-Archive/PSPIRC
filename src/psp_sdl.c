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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_image.h>
#include <pspctrl.h>
#include <psptypes.h>
#include <png.h>

#include "psp_sdl.h"
#include "psp_global.h"
#include "psp_danzeff.h"

  extern unsigned char psp_font_8x8[];
  extern unsigned char psp_font_6x10[];

  unsigned char *psp_font;
  int            psp_font_width  = 8; 
  int            psp_font_height = 8; 

  SDL_Surface *screen_surface;
  SDL_Surface *back_surface;

  SDL_Surface *menu_surface;
  SDL_Surface *console_surface;
  SDL_Surface *wifi_surface;
  SDL_Surface *login_surface;
  SDL_Surface *splash_surface;

uint
psp_sdl_rgb(uchar R, uchar G, uchar B)
{
  return SDL_MapRGB(screen_surface->format, R,G,B);
}

ushort *
psp_sdl_get_vram_addr(uint x, uint y)
{
  ushort *vram = (ushort *)screen_surface->pixels;
  return vram + x + (y*PSP_LINE_SIZE);
}

void
loc_psp_debug(char *file, int line, char *message)
{
  static int current_line = 0;
  static int current_col  = 10;

  char buffer[128];
  current_line += 10;
  if (current_line > 250)
  {
    if (current_col == 200) {
      psp_sdl_clear_screen(psp_sdl_rgb(0, 0, 0xff));
      current_col = 10;
    } else {
      current_col = 200;
    }
    
    current_line = 10;
  }
  sprintf(buffer,"%s:%d %s", file, line, message);
  psp_sdl_print(current_col, current_line, buffer, psp_sdl_rgb(0xff,0xff,0xff) );
}

void 
psp_sdl_print(int x,int y, char *str, int color)
{
  int index;
  int x0 = x;

  for (index = 0; str[index] != '\0'; index++) {
    psp_sdl_put_char(x, y, color, 0, str[index], 1, 0);
    x += psp_font_width;
    if (x >= (PSP_SDL_SCREEN_WIDTH - psp_font_width)) {
      x = x0; y++;
    }
    if (y >= (PSP_SDL_SCREEN_HEIGHT - psp_font_width)) break;
  }
}

void
psp_sdl_clear_screen(int color)
{
  int x; int y;
  ushort *vram = psp_sdl_get_vram_addr(0,0);
  
  for (y = 0; y < PSP_SDL_SCREEN_HEIGHT; y++) {
    for (x = 0; x < PSP_SDL_SCREEN_WIDTH; x++) {
      vram[x + (y*PSP_LINE_SIZE)] = color;
    }
  }
  psp_sdl_flush();
}


void 
psp_sdl_draw_rectangle(int x, int y, int w, int h, int border, int mode) 
{
  ushort *vram = (ushort *)psp_sdl_get_vram_addr(x, y);
  int xo, yo;
  if (mode == PSP_SDL_XOR) {
    for (xo = 0; xo < w; xo++) {
      vram[xo] ^=  border;
      vram[xo + h * PSP_LINE_SIZE] ^=  border;
    }
    for (yo = 0; yo < h; yo++) {
      vram[yo * PSP_LINE_SIZE] ^=  border;
      vram[w + yo * PSP_LINE_SIZE] ^=  border;
    }
    vram[w + h * PSP_LINE_SIZE] ^=  border;
  } else {
    for (xo = 0; xo < w; xo++) {
      vram[xo] =  border;
      vram[xo + h * PSP_LINE_SIZE] =  border;
    }
    for (yo = 0; yo < h; yo++) {
      vram[yo * PSP_LINE_SIZE] =  border;
      vram[w + yo * PSP_LINE_SIZE] =  border;
    }
    vram[w + h * PSP_LINE_SIZE] =  border;
  }
}

void 
psp_sdl_fill_rectangle(int x, int y, int w, int h, int color, int mode)
{
  ushort *vram  = (ushort *)psp_sdl_get_vram_addr(x, y);
  int xo, yo;
  if (mode == PSP_SDL_XOR) {
    for (xo = 0; xo <= w; xo++) {
      for (yo = 0; yo <= h; yo++) {
        if ( ((xo == 0) && ((yo == 0) || (yo == h))) ||
             ((xo == w) && ((yo == 0) || (yo == h))) ) {
          /* Skip corner */
        } else {
          vram[xo + yo * PSP_LINE_SIZE] ^=  color;
        }
      }
    }
  } else {
    for (xo = 0; xo <= w; xo++) {
      for (yo = 0; yo <= h; yo++) {
        vram[xo + yo * PSP_LINE_SIZE] =  color;
      }
    }
  }
}

static int
psp_sdl_get_back_color(int x, int y)
{
  uchar *back2 = (uchar *)back_surface->pixels;
  int bytes_per_pixels = back_surface->format->BytesPerPixel;
  int pitch            = back_surface->pitch;
  Uint8 r = back2[0 + (y * pitch) + (x * bytes_per_pixels)];
  Uint8 g = back2[1 + (y * pitch) + (x * bytes_per_pixels)];
  Uint8 b = back2[2 + (y * pitch) + (x * bytes_per_pixels)];
	int color = psp_sdl_rgb(r, g, b);

  return color;
}

void 
psp_sdl_back_rectangle(int x, int y, int w, int h)
{
  if (! back_surface) {
    psp_sdl_fill_rectangle(x, y, w, h, 0x0, 0);
    return;
  }

  ushort *vram  = (ushort *)psp_sdl_get_vram_addr(x, y);

  int xo, yo;
  for (xo = 0; xo <= w; xo++) {
    for (yo = 0; yo <= h; yo++) {
      vram[xo + yo * PSP_LINE_SIZE] = psp_sdl_get_back_color(x + xo, y + yo);
    }
  }
}

void 
psp_sdl_put_char(int x, int y, int color, int bgcolor, uchar c, int drawfg, int drawbg)
{
  int cx;
  int cy;
  int b;
  int index;

  ushort *vram = (ushort *)psp_sdl_get_vram_addr(x, y);
  index = ((ushort)c) * psp_font_height;

  for (cy=0; cy< psp_font_height; cy++) {
    b = 1 << (psp_font_width - 1);
    for (cx=0; cx< psp_font_width; cx++) {
      if (psp_font[index] & b) {
        if (drawfg) vram[cx + cy * PSP_LINE_SIZE] = color;
      } else {
        if (drawbg) vram[cx + cy * PSP_LINE_SIZE] = bgcolor;
      }
      b = b >> 1;
    }
    index++;
  }
}

void 
psp_sdl_put_underline(int x, int y, int color)
{
  int cx;
  ushort *vram = (ushort *)psp_sdl_get_vram_addr(x, y + (psp_font_height-1));
  for (cx=0; cx< psp_font_width; cx++) {
    vram[cx] = color;
  }
}

void 
psp_sdl_back_put_char(int x, int y, int color, uchar c)
{
  int cx;
  int cy;
  int bmask;
  int index;

  if (! back_surface) {
    psp_sdl_put_char(x, y, color, 0x0, c, 1, 1);
    return;
  }

  ushort *vram  = (ushort *)psp_sdl_get_vram_addr(x, y);

  index = ((ushort)c) * psp_font_height;

  for (cy=0; cy< psp_font_height; cy++) {
    bmask = 1 << (psp_font_width - 1);
    for (cx=0; cx< psp_font_width; cx++) {
      if (psp_font[index] & bmask) {
        vram[cx + cy * PSP_LINE_SIZE] = color;
      } else {
        vram[cx + cy * PSP_LINE_SIZE] = psp_sdl_get_back_color(x + cx, y + cy);
      }
      bmask = bmask >> 1;
    }
    index++;
  }
}

void 
psp_sdl_fill_print(int x,int y,const char *str, int color, int bgcolor)
{
  int index;
  int x0 = x;

  for (index = 0; str[index] != '\0'; index++) {
    uchar c = str[index];
    if ((c == 0xc2) || (c == 0xc3)) {
      uchar new_c = psp_convert_utf8_to_iso_8859_1(c, str[index+1]);
      if (new_c) { c = new_c; index++; }
    }
    psp_sdl_put_char(x, y, color, bgcolor, c, 1, 1);
    x += psp_font_width;
    if (x >= (PSP_SDL_SCREEN_WIDTH - psp_font_width)) {
      x = x0; y++;
    }
    if (y >= (PSP_SDL_SCREEN_HEIGHT - psp_font_width)) break;
  }
}

void
psp_sdl_back_print(int x,int y,const char *str, int color)
{
  int index;
  int x0 = x;

  for (index = 0; str[index] != '\0'; index++) {
    uchar c = str[index];
    if ((c == 0xc2) || (c == 0xc3)) {
      uchar new_c = psp_convert_utf8_to_iso_8859_1(c, str[index+1]);
      if (new_c) { c = new_c; index++; }
    }
    psp_sdl_back_put_char(x, y, color, c);
    x += psp_font_width;
    if (x >= (PSP_SDL_SCREEN_WIDTH - psp_font_width)) {
      x = x0; y++;
    }
    if (y >= (PSP_SDL_SCREEN_HEIGHT - psp_font_width)) break;
  }
}

void
psp_sdl_wait_vn(uint count)
{
  for (; count>0; --count) {
    sceDisplayWaitVblankStart();
  }
}

void
psp_sdl_wait_vblank(void)
{
  sceDisplayWaitVblankStart();
}

void
psp_sdl_flush(void)
{
  sceKernelDcacheWritebackAll();
}

void
psp_sdl_lock(void)
{
  SDL_LockSurface(screen_surface);
}

void
psp_sdl_load_background()
{
  menu_surface = IMG_Load("./menu.png");
  console_surface = IMG_Load("./console.png");
  wifi_surface = IMG_Load("./wifi.png");
  login_surface = IMG_Load("./login.png");
  splash_surface = IMG_Load("./splash.png");

  if (!menu_surface || 
      !console_surface || 
      !wifi_surface || 
      !splash_surface || 
      !login_surface) {
    psp_sdl_exit(1);
  }
}

void
psp_sdl_blit_console()
{
	SDL_BlitSurface(console_surface, NULL, screen_surface, NULL);
  back_surface = console_surface;
}

void
psp_sdl_blit_menu()
{
	SDL_BlitSurface(menu_surface, NULL, screen_surface, NULL);
  back_surface = menu_surface;
}

void
psp_sdl_blit_wifi()
{
	SDL_BlitSurface(wifi_surface, NULL, screen_surface, NULL);
  back_surface = wifi_surface;
}

void
psp_sdl_blit_login()
{
	SDL_BlitSurface(login_surface, NULL, screen_surface, NULL);
  back_surface = login_surface;
}

void
psp_sdl_blit_splash()
{
	SDL_BlitSurface(splash_surface, NULL, screen_surface, NULL);
  back_surface = splash_surface;
}

void
psp_sdl_unlock(void)
{
  SDL_UnlockSurface(screen_surface);
}

void
psp_sdl_flip(void)
{
  SDL_Flip(screen_surface);
}

#define  systemRedShift      (screen_surface->format->Rshift)
#define  systemGreenShift    (screen_surface->format->Gshift)
#define  systemBlueShift     (screen_surface->format->Bshift)
#define  systemRedMask       (screen_surface->format->Rmask)
#define  systemGreenMask     (screen_surface->format->Gmask)
#define  systemBlueMask      (screen_surface->format->Bmask)

void
psp_sdl_save_png(char *filename)
{
  int w = 480;
  int h = 272;
  u8* pix = (u8*)psp_sdl_get_vram_addr(0,0);
  u8 writeBuffer[512 * 3];

  FILE *fp = fopen(filename,"wb");

  if(!fp) return;
  
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                NULL,
                                                NULL,
                                                NULL);
  if(!png_ptr) {
    fclose(fp);
    return;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);

  if(!info_ptr) {
    png_destroy_write_struct(&png_ptr,NULL);
    fclose(fp);
    return;
  }

  png_init_io(png_ptr,fp);

  png_set_IHDR(png_ptr,
               info_ptr,
               w,
               h,
               8,
               PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png_ptr,info_ptr);

  u8 *b = writeBuffer;

  int sizeX = w;
  int sizeY = h;
  int y;
  int x;

  u16 *p = (u16 *)pix;
  for(y = 0; y < sizeY; y++) {
     for(x = 0; x < sizeX; x++) {
      u16 v = p[x];

      *b++ = ((v & systemRedMask  ) >> systemRedShift  ) << 3; // R
      *b++ = ((v & systemGreenMask) >> systemGreenShift) << 2; // G 
      *b++ = ((v & systemBlueMask ) >> systemBlueShift ) << 3; // B
    }
    p += 512;
    png_write_row(png_ptr,writeBuffer);
     
    b = writeBuffer;
  }

  png_write_end(png_ptr, info_ptr);

  png_destroy_write_struct(&png_ptr, &info_ptr);

  fclose(fp);
}

void
psp_sdl_save_screenshot(void)
{
  char TmpFileName[MAX_PATH];

  sprintf(TmpFileName,"%s/scr/screenshot_%d.png", PSPIRC.psp_homedir, PSPIRC.psp_screenshot_id++);
  if (PSPIRC.psp_screenshot_id >= 10) PSPIRC.psp_screenshot_id = 0;
  psp_sdl_save_png(TmpFileName);
}

void
psp_sdl_display_splash()
{
  int index = 0;
  SceCtrlData c;

  psp_sdl_blit_splash();
  psp_sdl_flush();
  psp_sdl_flip();

  psp_sdl_blit_splash();
  psp_sdl_flush();
  psp_sdl_flip();
  while (index < 50) {
    myCtrlPeekBufferPositive(&c, 1);
    c.Buttons &= PSP_ALL_BUTTON_MASK;

    if (c.Buttons & (PSP_CTRL_START|PSP_CTRL_CROSS)) break;
    index++;
  }
}

int
psp_sdl_init(void)
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
     return 0;
  }

  if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
    return 0;
  }

  psp_sdl_select_font_8x8();

  screen_surface=SDL_SetVideoMode(PSP_SDL_SCREEN_WIDTH,PSP_SDL_SCREEN_HEIGHT, 16, 
                                  SDL_ANYFORMAT|SDL_DOUBLEBUF|SDL_HWSURFACE|SDL_HWPALETTE);

  if ( !screen_surface) {
    return 0;
  }

  psp_sdl_load_background();

  SDL_FillRect(screen_surface,NULL,SDL_MapRGB(screen_surface->format,0x0,0x0,0x0));
  SDL_Flip(screen_surface);
  SDL_FillRect(screen_surface,NULL,SDL_MapRGB(screen_surface->format,0x0,0x0,0x0));
  SDL_Flip(screen_surface);

  SDL_ShowCursor(SDL_DISABLE);

  psp_sdl_display_splash();

  /* Danzeff Keyboard */
  danzeff_load();
  danzeff_set_screen(screen_surface);

  return 1;
}

void
psp_sdl_exit(int status)
{
  SDL_Quit();
  exit(status);
}

void
psp_sdl_select_font_8x8()
{
  psp_font = psp_font_8x8;
  psp_font_height = 8;
  psp_font_width  = 8;
}

void
psp_sdl_select_font_6x10()
{
  psp_font = psp_font_6x10;
  psp_font_height = 10;
  psp_font_width  = 6;
}

int 
SDL_main(int argc,char *argv[])
{
  psp_sdl_init();

  psp_wifi_connect();

  psp_sdl_exit(0);

  return 0;
}
