/* Blank Module, that does absolutely nothing. */

#define BLANK_ID "$Name$ $Id$"

/* #include <...> */

#include "module.h"


int blank_version_major = 0;
int blank_version_minor = 0;

char *blank_id = BLANK_ID "\r\n" HEADER_ID "\r\n" MODULE_ID "\r\n";



/* Here we register our functions. */

void blank_init_data( );


ENTRANCE( blank_module_register )
{
   self->name = strdup( "BlankMod" );
   self->version_major = blank_version_major;
   self->version_minor = blank_version_minor;
   self->id = blank_id;
   
   self->init_data = blank_init_data;
   self->process_server_line_prefix = NULL;
   self->process_server_line_suffix = NULL;
   self->process_server_prompt_informative = NULL;
   self->process_server_prompt_action = NULL;
   self->process_client_command = NULL;
   self->process_client_aliases = NULL;
   self->build_custom_prompt = NULL;
   
   self->main_loop = NULL;
   self->update_descriptors = NULL;
   self->update_modules = NULL;
   self->update_timers = NULL;
   self->debugf = NULL;
   
   /* Communication */
   get_modules = self->get_func( "get_modules" );
   get_variable = self->get_func( "get_variable" );
   DEBUG = self->get_func( "DEBUG" );
   debugf = self->get_func( "debugf" );
   debugerr = self->get_func( "debugerr" );
   logff = self->get_func( "logff" );
   clientf = self->get_func( "clientf" );
   clientfr = self->get_func( "clientfr" );
   clientff = self->get_func( "clientff" );
   send_to_server = self->get_func( "send_to_server" );
   show_prompt = self->get_func( "show_prompt" );
   gag_line = self->get_func( "gag_line" );
   gag_prompt = self->get_func( "gag_prompt" );
   /* Utility */
   get_string = self->get_func( "get_string" );
   cmp = self->get_func( "cmp" );
   /* Timers */
   get_timers = self->get_func( "get_timers" );
   get_timer = self->get_func( "get_timer" );
   add_timer = self->get_func( "add_timer" );
   del_timer = self->get_func( "del_timer" );
   /* Networking */
   get_descriptors = self->get_func( "get_descriptors" );
   mb_connect = self->get_func( "mb_connect" );
   get_connect_error = self->get_func( "get_connect_error" );
   add_descriptor = self->get_func( "add_descriptor" );
   remove_descriptor = self->get_func( "remove_descriptor" );
   c_read = self->get_func( "c_read" );
   c_write = self->get_func( "c_write" );
   c_close = self->get_func( "c_close" );
}



void blank_init_data( )
{
   
   
}

/* Called before unloading. *
void blank_unload( )
{
   
   
}
*/

/* Not yet called anywhere, but reserved for copyright notices. *
void blank_show_notice( )
{
   
   
}
*/

/* Called before every normal line.
 * Args: colorless_line = String with all color codes stripped.
 *       colorful_line = String with all non-printable characters stripped.
 *       raw_line = String containing the data as it came from the server.
 *
void blank_process_server_line_prefix( char *colorless_line, char *colorful_line, char *raw_line )
{
   
   
}
*/

/* Called after every normal line.
 * Args: Check above.
 *
void blank_process_server_line_suffix( char *colorless_line, char *colorful_line, char *raw_line )
{
   
   
}
*/

/* Called before every prompt.
 * Args: line = String with all colors stripped.
 *       rawline = String with everything in it.
 *
void blank_process_server_prompt_informative( char *line, char *rawline )
{
   
   
}
*/

/* Called after every prompt.
 * Args: rawline = String with everything in it.
 *
void blank_process_server_prompt_action( char *rawline )
{
   
   
}
*/

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

/* Called before a prompt, but after process_prompt_informative.
 * Returns: NULL = Use the normal prompt.
 *          string = Use this instead of the normal prompt.
 *
char *blank_build_custom_prompt( )
{
   
   
}
*/

