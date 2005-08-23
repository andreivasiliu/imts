/*
 * Copyright (c) 2004, 2005  Andrei Vasiliu
 * 
 * 
 * This file is part of MudBot.
 * 
 * MudBot is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * MudBot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MudBot; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/* Main header file, to be used with all source files. */

#define HEADER_ID "$Name$ $Id$"

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#if defined( FOR_WINDOWS )
# include <windows.h>
#endif


/* We need some colors, to use with clientf( ). */
#define C_0 "\33[0;37m"
#define C_D "\33[1;30m"
#define C_R "\33[1;31m"
#define C_G "\33[1;32m"
#define C_Y "\33[1;33m"
#define C_B "\33[1;34m"
#define C_C "\33[1;36m"
#define C_W "\33[1;37m"

#define C_r "\33[0;31m"
#define C_g "\33[0;32m"
#define C_y "\33[0;33m"
#define C_b "\33[0;34m"
#define C_m "\33[0;35m"
#define C_c "\33[0;36m"

/* Portability stuff. */
#if defined( FOR_WINDOWS )
# define TELOPT_ECHO	1 /* client local echo */
# define IAC	      255 /* interpret as command */
# define WILL	      251 /* I will use option */
# define GA	      249 /* you may reverse the line */
# define DONT	      254 /* you are not to use option */
# define DO	      253 /* please, you use option */
# define WONT	      252 /* I won't use option */
# define SB	      250 /* interpret as subnegotiation */
# define SE	      240 /* end sub negotiation */
# define EOR	      239 /* end of record (transparent mode) */
#else
# include <arpa/telnet.h>
#endif
#define ATCP	      200 /* Achaea Telnet Communication Protocol. */

#define TELOPT_COMPRESS 85  /* MCCP - Mud Client Compression Protocol. */
#define TELOPT_COMPRESS2 86 /* MCCPv2 */
#define TELOPT_MXP 91       /* MXP - Mud eXtension Protocol. */

#define INPUT_BUF 2048

#if defined( FOR_WINDOWS )
# define __atribute__ ; //
#endif

typedef struct module_data MODULE;
typedef struct descriptor_data DESCRIPTOR;
typedef struct timer_data TIMER;

/* Module information structure. */
struct module_data
{
   char *name;
   
   void (*register_module)( MODULE *self );
   
   /* This is set by MudBot, and used by modules. */
   void *(*get_func)( char *name );
   
#if defined( FOR_WINDOWS )
   HINSTANCE dll_hinstance;
#else
   void *dl_handle;
#endif
   char *file_name;
   
   MODULE *next;
   
   /* These will be set by the module. */
   int version_major;
   int version_minor;
   char *id;
   
   void (*init_data)( );
   void (*unload)( );
   void (*show_notice)( );
   void (*process_server_line_prefix)( char *colorless_line, char *colorful_line, char *raw_line );
   void (*process_server_line_suffix)( char *colorless_line, char *colorful_line, char *raw_line );
   void (*process_server_prompt_informative)( char *line, char *rawline );
   void (*process_server_prompt_action)( char *rawline );
   int  (*process_client_command)( char *cmd );
   int  (*process_client_aliases)( char *cmd );
   char*(*build_custom_prompt)( );
   int  (*main_loop)( int argc, char **argv );
   void (*update_descriptors)( );
   void (*update_modules)( );
   void (*update_timers)( );
   void (*debugf)( char *string );
};


/* Sockets/descriptors. */
struct descriptor_data
{
   char *name;
   char *description;
   MODULE *mod;
   
   int deleted;
   int fd;
   
   void (*callback_in)( DESCRIPTOR *self );
   void (*callback_out)( DESCRIPTOR *self );
   void (*callback_exc)( DESCRIPTOR *self );
   
   DESCRIPTOR *next;
};

/* Timers. */
struct timer_data
{
   char *name;
   int delay;
   MODULE *mod;
   
   TIMER *next;
   
   int data[3];
   void (*callback)( TIMER *timer );
};

