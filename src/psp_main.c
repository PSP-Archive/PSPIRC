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

#include "psp_global.h"
#include "psp_irkeyb.h"

#define STDOUT_FILE  "stdout.txt"
#define STDERR_FILE  "stderr.txt"

static int setup_thid = -1;

#ifdef PSP_DEBUG
void sdl_psp_exception_handler(PspDebugRegBlock *regs)
{
	pspDebugScreenInit();

	pspDebugScreenSetBackColor(0x00FF0000);
	pspDebugScreenSetTextColor(0xFFFFFFFF);
	pspDebugScreenClear();

	pspDebugScreenPrintf("I regret to inform you your psp has just crashed\n\n");
	pspDebugScreenPrintf("Exception Details:\n");
	pspDebugDumpException(regs);
	pspDebugScreenPrintf("\nThe offending routine may be identified with:\n\n"
		"\tpsp-addr2line -e target.elf -f -C 0x%x 0x%x 0x%x\n",
		regs->epc, regs->badvaddr, regs->r[31]);

  sceKernelDelayThread( 1000 * 1000 * 30 );
}
# endif


/* If application's main() is redefined as SDL_main, and libSDLmain is
   linked, then this file will create the standard exit callback,
   define the PSP_* macros, add an exception handler, nullify device 
   checks and exit back to the browser when the program is finished. */

extern int SDL_main(int argc, char *argv[]);

static void cleanup_output(void);

# ifndef PSPFW30X
PSP_MODULE_INFO("PSPIRC", 0x1000, 1, 1);
# else
PSP_MODULE_INFO("PSPIRC", 0x0, 1, 1);
PSP_HEAP_SIZE_KB(14*1024);
# endif
PSP_MAIN_THREAD_ATTR(0);
PSP_MAIN_THREAD_STACK_SIZE_KB(64);

int 
sdl_psp_exit_callback(int arg1, int arg2, void *common)
{
  psp_exit_now = 1;
	return 0;
}

int sdl_psp_callback_thread(SceSize args, void *argp)
{
	int cbid = sceKernelCreateCallback("Exit Callback", sdl_psp_exit_callback, NULL);
	sceKernelRegisterExitCallback( cbid );

	sceKernelSleepThreadCB();
	return 0;
}

int sdl_psp_setup_callbacks(void)
{
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

	setup_thid = sceKernelCreateThread("update_thread", sdl_psp_callback_thread, 0x11, 0xFA0, 0, 0);
	if(setup_thid >= 0) sceKernelStartThread(setup_thid, 0, 0);
	return setup_thid;
}

/* Remove the output files if there was no output written */
static void cleanup_output(void)
{
#ifndef NO_STDIO_REDIRECT
  FILE *file;
  int empty;
#endif

  /* Flush the output in case anything is queued */
  fclose(stdout);
  fclose(stderr);

#ifndef NO_STDIO_REDIRECT
  /* See if the files have any output in them */
  file = fopen(STDOUT_FILE, "rb");
  if ( file ) {
    empty = (fgetc(file) == EOF) ? 1 : 0;
    fclose(file);
    if ( empty ) {
      remove(STDOUT_FILE);
    }
  }
  file = fopen(STDERR_FILE, "rb");
  if ( file ) {
    empty = (fgetc(file) == EOF) ? 1 : 0;
    fclose(file);
    if ( empty ) {
      remove(STDERR_FILE);
    }
  }
#endif
# ifdef USE_PSP_IRKEYB
  psp_irkeyb_exit();
# endif
}

#ifdef main
#undef main
#endif

#ifndef PSPFW30X
int
user_thread(SceSize args, void *argp)
{
	SDL_main(args, argp);
}
# endif

void
psp_main_exit()
{
  if (setup_thid >= 0) sceKernelTerminateThread(setup_thid);
  sceKernelExitGame();
}

int 
main(int argc, char *argv[])
{
	pspDebugScreenInit();

#ifdef PSP_DEBUG
	pspDebugInstallErrorHandler(sdl_psp_exception_handler);
#endif

	sdl_psp_setup_callbacks();

  psp_global_init();

#ifndef NO_STDIO_REDIRECT
  /* Redirect standard output and standard error. */
  /* TODO: Error checking. */
  freopen(STDOUT_FILE, "w", stdout);
  freopen(STDERR_FILE, "w", stderr);
  setvbuf(stdout, NULL, _IOLBF, BUFSIZ);  /* Line buffered */
  setbuf(stderr, NULL);          /* No buffering */
#endif /* NO_STDIO_REDIRECT */

  /* Functions registered with atexit() are called in reverse order, so make
     sure that we register sceKernelExitGame() first, so that it's called last. */
	atexit(psp_main_exit);

  atexit(cleanup_output);

# ifdef PSPFW30X
  sceUtilityLoadNetModule(1); /* common */
  sceUtilityLoadNetModule(3); /* inet */
# else
  if (pspSdkLoadInetModules() != 0) {
    pspDebugScreenPrintf("unable to load wifi drivers !");
    return 0;
  }
# endif

# ifdef USE_PSP_IRKEYB
  if (psp_irkeyb_init()) {
    fprintf(stdout, "failed to initialize IR keyboard !\n");
  }
# endif

# ifdef PSPFW30X
  SDL_main(argc, argv );
# else
  int user_thid = sceKernelCreateThread( "user_thread", 
     (SceKernelThreadEntry)user_thread, 0x16, 256*1024, PSP_THREAD_ATTR_USER, 0 );
  if(user_thid >= 0) {
    sceKernelStartThread(user_thid, 0, 0);
    sceKernelWaitThreadEnd(user_thid, NULL);
  }
# endif

	/* Delay 2.5 seconds before returning to the OS. */
	sceKernelDelayThread(2500000);

  return 0;
}
