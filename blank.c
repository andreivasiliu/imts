/* Blank Module, that does absolutely nothing. */

#define BLANK_ID "$Name$ $Id$"

/* #include <...> */

#include "module.h"


int blank_version_major = 0;
int blank_version_minor = 0;

char *blank_id = BLANK_ID "\r\n" HEADER_ID "\r\n" MODULE_ID "\r\n";



/* Here we register our functions. */

void blank_init_data( );
void blank_process_server_line( LINE *line );
void blank_process_server_prompt( LINE *line );


ENTRANCE( blank_module_register )
{
   self->name = strdup( "BlankMod" );
   self->version_major = blank_version_major;
   self->version_minor = blank_version_minor;
   self->id = blank_id;
   
   self->init_data = blank_init_data;
   self->process_server_line = blank_process_server_line;
   self->process_server_prompt = blank_process_server_prompt;
   self->process_client_command = NULL;
   self->process_client_aliases = NULL;
   
   self->main_loop = NULL;
   self->update_descriptors = NULL;
   self->update_modules = NULL;
   self->update_timers = NULL;
   self->debugf = NULL;
   
   GET_FUNCTIONS( self );
}



void blank_init_data( )
{
   
   
}

/* Called before unloading. *
void blank_unload( )
{
   
   
}
*/

/* Called at every normal line.
 * See the file header.h for information about the LINE structure.
 */
void blank_process_server_line( LINE *line )
{
   
   
}


/* Called at every prompt.
 * See the file header.h for information about the LINE structure.
 */
void blank_process_server_prompt( LINE *line )
{
   
   
}


/* Called for every client command that begins with `.
 * Args: cmd = The command, including the initial symbol.
 * Returns: 1 = Command is known.
 *          0 = Command is unknown, pass it to the other modules.
 *
int blank_process_client_command( char *cmd )
{
   
   
}
*/

/* Called for every client command.
 * Args: cmd = The command string.
 * Returns: 1 = Don't send it further to the server.
 *          0 = Check it with other modules, and send it to the server.
 *
int blank_process_client_aliases( char *cmd )
{
   
   
}
*/

