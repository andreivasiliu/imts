/* Prototype ILua Module */

#define I_LUA_ID "$Name$ $Id$"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <sys/stat.h>

#include "module.h"


int i_lua_version_major = 0;
int i_lua_version_minor = 3;

char *i_lua_id = I_LUA_ID "\r\n" HEADER_ID "\r\n" MODULE_ID "\r\n";


/* A LuaMod must have
 * a name
 * a state
 * an active flag
 * a main file name
 * cpath/path
 */

typedef struct ilua_mod ILUA_MOD;

struct ilua_mod
{
   char *name;
   
   lua_State *L;
   
   short disabled;
   
   ILUA_MOD *next;
};


ILUA_MOD *ilua_modules;

void ilua_open_mbapi( lua_State *L );
void ilua_open_mbcolours( lua_State *L );
void read_ilua_config( char *file_name, char *mod_name );

/* Here we register our functions. */

void i_lua_init_data( );
void i_lua_unload( );
void i_lua_process_server_line( LINE *line );
void i_lua_process_server_prompt( LINE *line );
int i_lua_process_client_aliases( char *cmd );
int i_lua_process_client_command( char *cmd );


ENTRANCE( i_lua_module_register )
{
   self->name = strdup( "ILua" );
   self->version_major = i_lua_version_major;
   self->version_minor = i_lua_version_minor;
   self->id = i_lua_id;
   
   self->init_data = i_lua_init_data;
   self->unload = i_lua_unload;
   self->process_server_line = i_lua_process_server_line;
   self->process_server_prompt = i_lua_process_server_prompt;
   self->process_client_command = i_lua_process_client_command;
   self->process_client_aliases = i_lua_process_client_aliases;
   
   GET_FUNCTIONS( self );
}



void i_lua_init_data( )
{
   read_ilua_config( "config.ilua.txt", NULL );
}



static int ilua_errorhandler( lua_State *L )
{
   lua_Debug ar;
   const char *errmsg;
   char where[256];
   int level;
   
   errmsg = lua_tostring( L, -1 );
   if ( !errmsg )
     errmsg = "Untrapped error from Lua";
   
   level = 1;
   lua_getstack( L, level, &ar );
   lua_getinfo( L, "nSl", &ar );
   
   clientff( C_R "\r\n[" C_W "ILua" C_R "]: " C_R "%s" C_R "]\r\n" C_0, errmsg );
   
   clientff( C_R "Traceback:\r\n" C_0 );
   level = 1;
   while ( lua_getstack( L, level++, &ar ) )
     {
        lua_getinfo( L, "nSl", &ar );
        
        if ( ar.name )
          snprintf( where, 256, "in function '%s'", ar.name);
        else if ( ar.what[0] == 'm' )
          snprintf( where, 256, "in main chunk" );
        else if ( ar.what[0] == 't' )
          snprintf( where, 256, "in a tail call (I forgot the function)" );
        else if ( ar.what[0] == 'C' )
          snprintf( where, 256, "in a C function" );
        else
          snprintf( where, 256, "in a Lua function" );
        
        clientff( C_W " %s" C_0 ":" C_G "%d" C_0 ": %s\r\n", ar.short_src, ar.currentline, where );
     }
   
   return 1;
}



void close_ilua_module( ILUA_MOD *module )
{
   ILUA_MOD *m;
   
   /* Unlink it. */
   if ( module == ilua_modules )
     ilua_modules = module->next;
   else
     {
        m = ilua_modules;
        while ( m && m->next != module )
          m = m->next;
        
        if ( m )
          m->next = module->next;
     }
   
   if ( module->L )
     lua_close( module->L );
   if ( module->name )
     free( module->name );
   
   free( module );
}



void read_ilua_config( char *file_name, char *mod_name )
{
   FILE *fl;
   ILUA_MOD *mod;
   lua_State *L = NULL;
   struct stat stat_buf;
   const char *errmsg;
   char line[4096], cmd[256], arg[4096], *p;
   int ignore = 0, all_okay = 1;
   
   if ( stat( file_name, &stat_buf ) )
     {
        fl = fopen( file_name, "w" );
        if ( !fl )
          {
             debugerr( file_name );
             return;
          }
        
        fprintf( fl, "# ILua configuration file.\n\n" );
        
        fprintf( fl, "# At a later time, ILua will probably be modularized. But for now, just one\n"
                 "# will suffice.\n\n" );
        
        fprintf( fl, "# Files to load for the 'main' module\n"
                 "# There can be as many as you want. Syntax: load \"filename\"\n"
                 "# The path is relative to mb-core. \"../LuaScripts\" would work too.\n\n" );
        
        fprintf( fl, "#load \"example.lua\"\n\n" );
        
        fclose( fl );
     }
   
   fl = fopen( file_name, "r" );
   
   if ( !fl )
     return;
   
   get_timer( );
   
   while( 1 )
     {
        fgets( line, 4096, fl );
        
        if ( feof( fl ) )
          break;
        
        p = get_string( line, cmd, 256 );
        get_string( p, arg, 4096 );
        
        if ( !strcmp( cmd, "module" ) )
          {
             ignore = 0;
             if ( mod_name && strcmp( arg, mod_name ) )
               {
                  ignore = 1;
                  continue;
               }
             
             /* Create a shiny, brand new module. */
             if ( !ilua_modules )
               {
                  mod = calloc( 1, sizeof( ILUA_MOD ) );
                  ilua_modules = mod;
               }
             else
               {
                  mod = ilua_modules;
                  while ( mod->next )
                    mod = mod->next;
                  mod->next = calloc( 1, sizeof( ILUA_MOD ) );
                  mod = mod->next;
               }
             
             /* Initialize it. */
             L = luaL_newstate( );
             mod->L = L;
             mod->name = strdup( arg );
             
             luaL_openlibs( L );
             ilua_open_mbapi( L );
             ilua_open_mbcolours( L );
          }
        
        if ( ignore )
          continue;
        
        if ( !strcmp( cmd, "load" ) )
          {
             if ( !L )
               {
                  debugf( "%s: Trying to 'load' outside a module!", file_name );
                  return;
               }
             
             lua_pushcfunction( L, ilua_errorhandler );
             luaL_loadfile( L, arg );
             if ( lua_pcall( L, 0, 0, -2 ) )
               {
                  errmsg = lua_tostring( L, -1 );
                  if ( errmsg )
                    debugf( "%s: %s", arg, errmsg );
                  lua_pop( L, 1 );
                  
                  /* Close this module. Entirely. */
                  close_ilua_module( mod );
                  ignore = 1;
                  all_okay = 0;
               }
             lua_pop( L, 1 );
          }
     }
   
   if ( all_okay && !mod_name )
     debugf( "ILua scripts loaded. (%d microseconds)", get_timer( ) );
   
   fclose( fl );
}



void set_line_in_table( lua_State *L, LINE *line )
{
   lua_getglobal( L, "mb" );
   if ( !lua_istable( L, -1 ) )
     {
        lua_pop( L, 1 );
        clientfr( "** What the hell did you do with the global 'mb' table? **" );
        ilua_open_mbapi( L );
        lua_getglobal( L, "mb" );
     }
   
   /* Put some info from our LINE structure in here. */
   lua_pushstring( L, line->line );
   lua_setfield( L, -2, "line" );
   lua_pushlstring( L, line->raw_line, line->raw_len );
   lua_setfield( L, -2, "raw_line" );
   lua_pushstring( L, line->ending );
   lua_setfield( L, -2, "ending" );
   lua_pushinteger( L, line->len );
   lua_setfield( L, -2, "len" );
   lua_pushinteger( L, line->raw_len );
   lua_setfield( L, -2, "raw_len" );
   
   /* Reset these, else we gag everything. */
   lua_pushnil( L );
   lua_setfield( L, -2, "gag_line" );
   lua_pushnil( L );
   lua_setfield( L, -2, "gag_ending" );
   
   lua_pop( L, 1 );
}



int ilua_callback( lua_State *L, char *func, char *arg )
{
   int ret_value = 0;
   
   lua_getglobal( L, "mb" );
   if ( lua_istable( L, -1 ) )
     {
        lua_pushcfunction( L, ilua_errorhandler );
        lua_getfield( L, -2, func );
        if ( lua_isfunction( L, -1 ) )
          {
             if ( arg )
               lua_pushstring( L, arg );
             
             if ( lua_pcall( L, arg ? 1 : 0, 1, -3 ) )
               {
                  clientff( C_R "Error in the '%s' callback.\r\n" C_0, func );
               }
             else
               {
                  if ( lua_isboolean( L, -1 ) && lua_toboolean( L, -1 ) )
                    ret_value = 1;
               }
             lua_pop( L, 1 );
          }
        else
          lua_pop( L, 1 ); /* The callback */
        
        lua_pop( L, 1 ); /* The error handler */
     }
   
   lua_pop( L, 1 ); /* 'mb' table */
   
   return ret_value;
}



void set_gags( lua_State *L, LINE *line )
{
   lua_getglobal( L, "mb" );
   if ( !lua_istable( L, -1 ) )
     {
        lua_pop( L, 1 );
        return;
     }
   
   lua_getfield( L, -1, "gag_line" );
   line->gag_entirely = lua_toboolean( L, -1 );
   lua_getfield( L, -2, "gag_ending" );
   line->gag_ending = lua_toboolean( L, -1 );
   
   lua_pop( L, 3 );
}



void i_lua_unload( )
{
   ILUA_MOD *m;
   
   for ( m = ilua_modules; m; m = m->next )
     {
        ilua_callback( m->L, "unload", NULL );
        lua_close( m->L );
     }
}



void i_lua_process_server_line( LINE *line )
{
   ILUA_MOD *m;
   
   for ( m = ilua_modules; m; m = m->next )
     {
        set_line_in_table( m->L, line );
        ilua_callback( m->L, "server_line", NULL );
        set_gags( m->L, line );
     }
}



void i_lua_process_server_prompt( LINE *line )
{
   ILUA_MOD *m;
   
   for ( m = ilua_modules; m; m = m->next )
     {
        set_line_in_table( m->L, line );
        ilua_callback( m->L, "server_prompt", NULL ); 
        set_gags( m->L, line );
    }
}



int i_lua_process_client_aliases( char *cmd )
{
   int i = 0;
   ILUA_MOD *m;
   
   for ( m = ilua_modules; m; m = m->next )
     {
        i = ilua_callback( m->L, "client_aliases", cmd );
     }
   
   return i;
}



int i_lua_process_client_command( char *cmd )
{
   char buf[4096], *p;
   ILUA_MOD *mod;
   
   /* Skip `il/ilua, and get the command */
   p = get_string( cmd, buf, 4096 );
   if ( strcmp( buf, "`il" ) && strcmp( buf, "`ilua" ) )
     return 1;
   
   p = get_string( p, buf, 4096 );
   
   if ( !strcmp( buf, "mods" ) )
     {
        clientfr( "Lua Modules:");
        for ( mod = ilua_modules; mod; mod = mod->next )
          clientff( " - %s\r\n", mod->name );
     }
   else if ( !strcmp( buf, "help" ) )
     {
        clientfr( "ILua commands:" );
        clientf( " mods   - Lists loaded ILua modules.\r\n"
                 " help   - I'll give you three guesses.\r\n"
                 " load   - Load all files of the given module, after unloading.\r\n"
                 " reload - An alias of the above.\r\n"
                 " unload - Unload the given module.\r\n" );
        clientfr( "All commands must be prefixed by `il or `ilua." );
     }
   else if ( !strcmp( buf, "unload" ) )
     {
        get_string( p, buf, 4096 );
        
        if ( !buf[0] )
          {
             clientfr( "Which module should I unload?" );
             return 0;
          }
        
        for ( mod = ilua_modules; mod; mod = mod->next )
          if ( !strcmp( mod->name, buf ) )
            break;
        
        if ( !mod )
          clientfr( "Careful with spelling and capitalization. That module doesn't exist." );
        else
          {
             clientff( C_R "[Unloading '%s'.]\r\n" C_0, mod->name );
             close_ilua_module( mod );
          }
     }
   else if ( !strcmp( buf, "load" ) || !strcmp( buf, "reload" ) )
     {
        get_string( p, buf, 4096 );
        
        if ( !buf[0] )
          {
             clientfr( "Which module should I load?" );
             return 0;
          }
        
        for ( mod = ilua_modules; mod; mod = mod->next )
          if ( !strcmp( mod->name, buf ) )
            break;
        
        if ( mod )
          {
             clientff( C_R "[Unloading current module.]\r\n" C_0 );
             ilua_callback( mod->L, "unload", NULL );
             close_ilua_module( mod );
          }
        
        clientff( C_R "[Loading '%s'.]\r\n" C_0, buf );
        read_ilua_config( "config.ilua.txt", buf );
     }
   else
     {
        clientfr( "Unknown ILua command. Use `il help for a useful list." );
     }
   
   return 0;
}



static int ilua_echo( lua_State *L )
{
   const char *str;
   
   /* This thingie also leaves an empty string on no arguments. Neat. */
   lua_concat( L, lua_gettop( L ) );
   
   str = lua_tostring( L, -1 );
   if ( str )
     clientf( (char*) str );
   lua_pop( L, 1 );
   
   return 0;
}



static int ilua_send( lua_State *L )
{
   const char *str;
   
   lua_pushstring( L, "\r\n" );
   lua_concat( L, lua_gettop( L ) );
   
   str = lua_tostring( L, -1 );
   if ( str )
     send_to_server( (char*) str );
   lua_pop( L, 1 );
   
   return 0;
}



static int ilua_debug( lua_State *L )
{
   const char *str;
   
   lua_concat( L, lua_gettop( L ) );
   
   str = lua_tostring( L, -1 );
   if ( str )
     debugf( "%s", str );
   lua_pop( L, 1 );
   
   return 0;
}



static int ilua_prefix( lua_State *L )
{
   const char *str;
   
   lua_concat( L, lua_gettop( L ) );
   
   str = lua_tostring( L, -1 );
   if ( str )
     prefix( (char*) str );
   lua_pop( L, 1 );
   
   return 0;
}



static int ilua_suffix( lua_State *L )
{
   const char *str;
   
   lua_concat( L, lua_gettop( L ) );
   
   str = lua_tostring( L, -1 );
   if ( str )
     suffix( (char*) str );
   lua_pop( L, 1 );
   
   return 0;
}



static int ilua_insert( lua_State *L )
{
   const char *str;
   int pos;
   
   pos = luaL_checkint( L, 1 );
   
   lua_concat( L, lua_gettop( L ) - 1 );
   
   str = lua_tostring( L, -1 );
   if ( str )
     insert( pos, (char*) str );
   
   lua_pop( L, 2 );
   
   return 0;
}



static int ilua_replace( lua_State *L )
{
   const char *str;
   
   lua_concat( L, lua_gettop( L ) );
   
   str = lua_tostring( L, -1 );
   if ( str )
     replace( (char*) str );
   lua_pop( L, 1 );
   
   return 0;
}



static int ilua_show_prompt( lua_State *L )
{
   show_prompt( );
   
   return 0;
}



static int ilua_cmp( lua_State *L )
{
   const char *pattern, *text;
   
   pattern = luaL_checkstring( L, 1 );
   text = luaL_checkstring( L, 2 );
   
   lua_pop( L, 2 );
   
   lua_pushboolean( L, !cmp( (char*) pattern, (char*) text ) );
   
   return 1;
}




/*
 * mb =
 * {
 *    version = "M.m"
 *    
 *    -- Callbacks
 *    unload = nil,
 *    server_line = nil,
 *    server_prompt = nil,
 *    client_aliases = nil,
 *    
 *    -- Communication
 *    echo
 *    send
 *    debug
 *    prefix
 *    suffix
 *    insert
 *    replace
 *    show_prompt
 *    
 *    -- Utility
 *    cmp
 *    
 *    -- MXP / not implemented
 *    
 *    -- Timers / not implemented
 *    
 *    -- Networking / not implemented
 *    
 *    -- LINE structure
 *    line
 *    raw_line
 *    ending
 *    len
 *    raw_len
 *    
 *    gag_line
 *    gag_ending
 * }
 * 
 */


void ilua_open_mbapi( lua_State *L )
{
   lua_createtable( L, 0, 10 );
   
   lua_pushfstring( L, "%d.%d", i_lua_version_major, i_lua_version_minor );
   lua_setfield( L, 1, "version" );
   
   /* C Functions. */
   lua_pushcfunction( L, ilua_echo );
   lua_setfield( L, 1, "echo" );
   lua_pushcfunction( L, ilua_send );
   lua_setfield( L, 1, "send" );
   lua_pushcfunction( L, ilua_debug );
   lua_setfield( L, 1, "debug" );
   lua_pushcfunction( L, ilua_prefix );
   lua_setfield( L, 1, "prefix" );
   lua_pushcfunction( L, ilua_suffix );
   lua_setfield( L, 1, "suffix" );
   lua_pushcfunction( L, ilua_insert );
   lua_setfield( L, 1, "insert" );
   lua_pushcfunction( L, ilua_replace );
   lua_setfield( L, 1, "replace" );
   lua_pushcfunction( L, ilua_show_prompt );
   lua_setfield( L, 1, "show_prompt" );
   lua_pushcfunction( L, ilua_cmp );
   lua_setfield( L, 1, "cmp" );
   
   /* Register the "mb" table as a global. */
   lua_setglobal( L, "mb" );
}



void ilua_open_mbcolours( lua_State *L )
{
   lua_createtable( L, 0, 16 );
   
   lua_pushstring( L, C_d );
   lua_setfield( L, 1, "d" );
   lua_pushstring( L, C_r );
   lua_setfield( L, 1, "r" );
   lua_pushstring( L, C_g );
   lua_setfield( L, 1, "g" );
   lua_pushstring( L, C_y );
   lua_setfield( L, 1, "y" );
   lua_pushstring( L, C_b );
   lua_setfield( L, 1, "b" );
   lua_pushstring( L, C_m );
   lua_setfield( L, 1, "m" );
   lua_pushstring( L, C_c );
   lua_setfield( L, 1, "c" );
   lua_pushstring( L, C_0 );
   lua_setfield( L, 1, "x" );
   lua_pushstring( L, C_D );
   lua_setfield( L, 1, "D" );
   lua_pushstring( L, C_R );
   lua_setfield( L, 1, "R" );
   lua_pushstring( L, C_G );
   lua_setfield( L, 1, "G" );
   lua_pushstring( L, C_Y );
   lua_setfield( L, 1, "Y" );
   lua_pushstring( L, C_B );
   lua_setfield( L, 1, "B" );
   lua_pushstring( L, C_M );
   lua_setfield( L, 1, "M" );
   lua_pushstring( L, C_C );
   lua_setfield( L, 1, "C" );
   lua_pushstring( L, C_W );
   lua_setfield( L, 1, "W" );
   
   /* Register the "C" colour table as a global. */
   lua_setglobal( L, "C" );
}


