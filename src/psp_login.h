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

# ifndef _PSP_LOGIN_H_
# define _PSP_LOGIN_H_

#ifdef __cplusplus
extern "C" {
#endif

# define MAX_USER_LENGTH      40
# define MAX_PASSWORD_LENGTH  40
# define MAX_NICKNAME_LENGTH  40
# define MAX_REALNAME_LENGTH  40
# define MAX_HOST_LENGTH      40
# define MAX_CHANNEL_LENGTH   40
# define MAX_SCRIPT_LENGTH    40
# define MAX_PORT_LENGTH       6

  typedef struct psplogin_t {

    char  user[MAX_USER_LENGTH];
    char  password[MAX_PASSWORD_LENGTH];
    char  nickname[MAX_NICKNAME_LENGTH];
    char  realname[MAX_REALNAME_LENGTH];
    char  host[MAX_HOST_LENGTH];
    char  port[MAX_PORT_LENGTH];
    char  channel[MAX_CHANNEL_LENGTH];
    char  script[MAX_SCRIPT_LENGTH];

  } psplogin_t;

  typedef struct psplogin_list_t {

    struct psplogin_list_t *next;
    psplogin_t              login;

  } psplogin_list_t;

  extern psplogin_list_t  *psp_login_current;

#ifdef __cplusplus
}
#endif

# endif
