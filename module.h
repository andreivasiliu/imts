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


/* Module header file, to be used with all module source files. */

#if defined( FOR_WINDOWS )
# define BUILDING_DLL
#endif


#include "header.h"

#if defined( BUILTIN_MODULE )
# define ENTRANCE( name ) void (name)( MODULE *self )
#else
# if defined( FOR_WINDOWS )
#  define ENTRANCE( name ) BOOL WINAPI __declspec( dllexport ) LibMain( HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved ) { return TRUE; } \
   void __declspec( dllexport ) module_register( MODULE *self )
# else
#  define ENTRANCE( name ) void module_register( MODULE *self )
# endif
#endif


/*** Global functions. ***/

/* Communication */
MODULE* (*get_modules)( );
void *	(*get_variable)( char *name );
void	(*DEBUG)( char *name );
#if !defined( FOR_WINDOWS )
void	(*debugf)( char *string, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );
#else
void	(*debugf)( char *string, ... );
#endif
void	(*debugerr)( char *string );
#if !defined( FOR_WINDOWS )
void	(*logff)( char *type, char *string, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
#else
void	(*logff)( char *type, char *string, ... );
#endif
void	(*clientf)( char *string );
void	(*clientfr)( char *string );
#if !defined( FOR_WINDOWS )
void	(*clientff)( char *string, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );
#else
void	(*clientff)( char *string, ... );
#endif
void	(*send_to_server)( char *string );
void	(*show_prompt)( );
int	(*gag_line)( int gag );
int	(*gag_prompt)( int gag );

/* Utility */
#if defined( FOR_WINDOWS )
int	(*gettimeofday)( struct timeval *tv, void * );
#endif
char *	(*get_string)( char *argument, char *arg_first, int max );
int	(*cmp)( char *trigger, char *string );

/* Timers */
TIMER *	(*get_timers)( );
int	(*get_timer)( );
void	(*add_timer)( char *name, int delay, void (*cb)( TIMER *self ), int d0, int d1, int d2 );
void	(*del_timer)( char *name );

/* Networking */
DESCRIPTOR *(*get_descriptors)( );
int	(*mb_connect)( char *hostname, int port );
char *	(*get_connect_error)( );
void	(*add_descriptor)( DESCRIPTOR *desc );
void	(*remove_descriptor)( DESCRIPTOR *desc );
int	(*c_read)( int fd, void *buf, size_t count );
int	(*c_write)( int fd, const void *buf, size_t count );
int	(*c_close)( int fd );

