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

# ifndef _PSP_IRC_H_
# define _PSP_IRC_H_

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct irc_chan_elem {
    struct irc_chan_elem *next;
    char                 *name;
    char                 *topic;
    int                   number_user;

  } irc_chan_elem;

  typedef struct irc_chan_list {
    int             number_chan;
    irc_chan_elem** chan_array;

  } irc_chan_list;

  extern irc_chan_list* psp_irc_get_chanlist();
  extern void psp_irc_free_chanlist(irc_chan_list* chan_list);

  extern void psp_irc_thread(SceSize argc, void *argv);
  extern void psp_irc_main_loop();

  extern int psp_irc_chanlist_is_running();
  extern void psp_irc_chanlist_begin();
  extern irc_chan_elem* psp_irc_add_chanlist(const char *new_line);
  extern void psp_irc_chanlist_end();
  extern void psp_irc_chanlist_refresh();
  extern void psp_irc_set_chanlist_match(char *match_string);
  extern void psp_irc_set_chanlist_min_user(int min_user);

#ifdef __cplusplus
}
#endif

# endif
