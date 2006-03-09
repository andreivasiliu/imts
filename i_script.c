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


/* Imperian script parser. */

#define I_SCRIPT_ID "$Name$ $Id$"

#include <stdarg.h>
#include <sys/time.h>
#include <pcre.h>

#include "module.h"


int scripter_version_major = 0;
int scripter_version_minor = 6;

char *i_script_id = I_SCRIPT_ID "\r\n" HEADER_ID "\r\n" MODULE_ID "\r\n";


/* Script under compilation. */
char *pos;
char *beginning;

/* Scripter thingies. */

#define VAR_NUMBER		0
#define VAR_STRING		1
#define VAR_POINTER		2

#define NULL_POINTER		0
#define RELATIVE_LOCAL_MEMORY	1
#define ABSOLUTE_LOCAL_MEMORY	2
#define SCRIPT_MEMORY		3
#define GLOBAL_MEMORY		4
#define SCRIPT_FUNCTIONS	5



/* Structures that we'll need. */

typedef struct script_data SCRIPT;
typedef struct pointer_data POINTER;
typedef struct variable_data VARIABLE;
typedef struct symbol_data SYMBOL;
typedef struct symbol_table_data SYMBOL_TABLE;
typedef struct instr_data INSTR;
typedef struct code_data CODE;
typedef struct function_data FUNCTION;
typedef struct trigger_data TRIGGER;



struct pointer_data
{
   int offset;
   
   int memory_space;
} null_pointer = { 0, 0 };


struct variable_data
{
   char *name;
   
   int type;
   
   /* String. */
   char *string;
   
   /* Number. */
   int nr;
   
   /* Pointer. */
   POINTER pointer;
   
   /* Its place in memory. */
   POINTER address;
   
   VARIABLE *next;
};



struct symbol_data
{
   char *name;
   
   int invisible;
   
   /* Variable - where will it point to? */
   POINTER pointer;
   
   /* Constant. */
   int is_constant;
   VARIABLE constant;
};



struct symbol_table_data
{
   SYMBOL *symbol;
   
   int nr;
   int memory_size;
};



struct instr_data
{
   int instruction;
   
   /* Constant. */
   VARIABLE constant;
   
   /* Variable. */
   char *var_name;
   short defined_as_local;
   short defined_as_global;
   POINTER pointer;
   
   int no_linking_needed;
   
   /* Operator. */
   int oper;
   
   int sys_function;
   
   int links;
   INSTR **link;
};



struct code_data
{
   SYMBOL_TABLE symbol_table;
   
   INSTR *first_instruction;
};



struct function_data
{
   char *name;
   short alias;
   int args_nr;
   
   int code_offset;
   
   SCRIPT *parent;
   FUNCTION *next;
};



struct script_data
{
   char *name;
   char *description;
   
   short in_use;
   
   CODE *codes;
   int codes_nr;
   
   FUNCTION *functions;
   TRIGGER *triggers;
   
   SYMBOL_TABLE symbol_table;
   
   VARIABLE *script_memory;
   VARIABLE *script_memory_top;
   
   SCRIPT *next;
};



/* When a function is called, one such structure is created in the call-stack. */
struct trigger_data
{
   char *name;
   
   short prompt;
   short raw;
   short regex;
   short continuous;
   
   /* Each and every line, once. */
   short everything;
   /* Normal pattern. */
   char *message;
   /* Regex. */
   pcre *pattern;
   pcre_extra *extra;
   
   int code_offset;
   
   SCRIPT *parent;
   
   TRIGGER *next;
};


/* Convenience structure. */
struct prev_data
{
   int bottom, top;
   short script_in_use;
   SCRIPT *script;
   VARIABLE *function_return_value;
};


VARIABLE *variables;
SCRIPT *scripts;



/*** Instruction Handler Table ***/

#define INSTR_FUNCTION		0
#define INSTR_DOALL		1
#define INSTR_OPERATOR		2
#define INSTR_CONSTANT		3
#define INSTR_VARIABLE		4
#define INSTR_RETURN		5
#define INSTR_IF		6

typedef int instruction_handler( INSTR *, VARIABLE * );
instruction_handler instr_function;
instruction_handler instr_doall;
instruction_handler instr_operator;
instruction_handler instr_constant;
instruction_handler instr_variable;
instruction_handler instr_return;
instruction_handler instr_if;

/* Must be in the order of the defines above. */
instruction_handler *instructions[ ] =
{
   instr_function,
     instr_doall,
     instr_operator,
     instr_constant,
     instr_variable,
     instr_return,
     instr_if,
     
     NULL
};



/*** Operator Table ***/

#define P VAR_POINTER
#define S VAR_STRING

typedef int operator_handler( VARIABLE *, VARIABLE *, VARIABLE * );
operator_handler oper_not_implemented;
operator_handler oper_dereference;
operator_handler oper_address_of;
operator_handler oper_assignment;
operator_handler oper_modulus;
operator_handler oper_addition;
operator_handler oper_substraction;
operator_handler oper_equal_to;
operator_handler oper_not_equal_to;
operator_handler oper_greater_than;
operator_handler oper_less_than;
operator_handler oper_greater_than_or_equal_to;
operator_handler oper_less_than_or_equal_to;
operator_handler oper_not;
operator_handler oper_logical_and;
operator_handler oper_logical_or;

struct operator_data
{
   char *symbol;
   
   short unary;
   short rightsided;
   
   short right_to_left;
   
   short priority;
   
   short types;
   
   short assignment;
   
   operator_handler *function;
} operators[ ] =
  /* The order of the elements is listed above. I can't make a proper table. */
{
   /* Operators with.. bigger length. */
     { "+=",	0, 0,	1, 12,	P|S,	1, oper_not_implemented	},
     { "-=",	0, 0,	1, 12,	P,	1, oper_not_implemented	},
     { "*=",	0, 0,	1, 12,	0,	1, oper_not_implemented	},
     { "/=",	0, 0,	1, 12,	0,	1, oper_not_implemented	},
     { "%=",	0, 0,	1, 12,	0,	1, oper_not_implemented	},
     { "&=",	0, 0,	1, 12,	0,	1, oper_not_implemented	},
     { "^=",	0, 0,	1, 12,	0,	1, oper_not_implemented	},
     { "|=",	0, 0,	1, 12,	0,	1, oper_not_implemented	},
     { "<<=",	0, 0,	1, 12,	0,	1, oper_not_implemented	},
     { ">>=",	0, 0,	1, 12,	0,	1, oper_not_implemented	},
   /* Unary Operators. */
     { "++",	1, 0,	1, 1,	0,	1, oper_not_implemented	},
     { "--",	1, 0,	1, 1,	0,	1, oper_not_implemented	},
     { "++",	1, 1,	1, 1,	0,	1, oper_not_implemented	},
     { "--",	1, 1,	1, 1,	0,	1, oper_not_implemented	},
//   { "+",	1, 0,	1, 1,	0,	0, oper_not_implemented	},
     { "-",	1, 0,	1, 1,	0,	0, oper_not_implemented	},
     { "!",	1, 0,	1, 1,	S,	0, oper_not		},
     { "not",	1, 0,	1, 1,	S,	0, oper_not		},
     { "~",	1, 0,	1, 1,	0,	0, oper_not_implemented	},
     { "*",	1, 0,	1, 1,	P,	0, oper_dereference	},
     { "&",	1, 0,	1, 1,	P|S,	0, oper_address_of	},
   /* Multiplication, division, modulus. */
     { "*",	0, 0,	0, 2,	0,	0, oper_not_implemented	},
     { "/",	0, 0,	0, 2,	0,	0, oper_not_implemented	},
     { "%",	0, 0,	0, 2,	0,	0, oper_modulus		},
   /* Addition, substraction. */
     { "+",	0, 0,	0, 3,	S,	0, oper_addition	},
     { "-",	0, 0,	0, 3,	0,	0, oper_substraction	},
   /* Bitwise shift left, bitwise shift right. */
     { "<<",	0, 0,	0, 4,	0,	1, oper_not_implemented	},
     { ">>",	0, 0,	0, 4,	0,	1, oper_not_implemented	},
   /* Relational less than, less than or equal to, etc. */
     { "<=",	0, 0,	0, 5,	0,	0, oper_less_than_or_equal_to	 },
     { "<",	0, 0,	0, 5,	0,	0, oper_less_than	},
     { ">=",	0, 0,	0, 5,	0,	0, oper_greater_than_or_equal_to },
     { ">",	0, 0,	0, 5,	0,	0, oper_greater_than	},
   /* Equal to, not equal to. */
     { "==",	0, 0,	0, 6,	P|S,	0, oper_equal_to	},
     { "!=",	0, 0,	0, 6,	P|S,	0, oper_not_equal_to	},
   /* Logical AND. */
     { "&&",	0, 0,	0, 10,	P|S,	0, oper_logical_and	},
     { "and",	0, 0,	0, 10,	P|S,	0, oper_logical_and	},
   /* Logical OR. */
     { "||",	0, 0,	0, 11,	P|S,	0, oper_logical_or	},
     { "or",	0, 0,	0, 11,	P|S,	0, oper_logical_or	},
   /* Bitwise AND. */
     { "&",	0, 0,	0, 7,	0,	0, oper_not_implemented	},
   /* Bitwise exclusive OR. */
     { "^",	0, 0,	0, 8,	0,	0, oper_not_implemented	},
   /* Bitwise inclusive OR. */
     { "|",	0, 0,	0, 9,	0,	0, oper_not_implemented	},
   /* Assignment. */
     { "=",	0, 0,	1, 12,	P|S,	2, oper_assignment	},
   
     { NULL }
};

#define UNARY_PRIORITY	 1
#define FULL_PRIORITY	13

#undef P
#undef N
#undef S

/*** System Functions Table. ***/

typedef int system_function( INSTR *, VARIABLE * );
system_function sysfunc_not_implemented;
system_function sysfunc_echo;
system_function sysfunc_send;
system_function sysfunc_necho;
system_function sysfunc_nsend;
system_function sysfunc_arg;
system_function sysfunc_args;
system_function sysfunc_show_prompt;
system_function sysfunc_debug;
system_function sysfunc_load_script;
system_function sysfunc_unload_script;
system_function sysfunc_call;
system_function sysfunc_nr;
system_function sysfunc_str;

struct sys_function_data
{
   char *name;
   
   int args;
   
   system_function *function;
} sys_functions[ ] =
{
     { "echo",		1, sysfunc_echo			},
     { "send",		1, sysfunc_send			},
     { "necho",		1, sysfunc_necho		},
     { "nsend",		1, sysfunc_nsend		},
     { "arg",		1, sysfunc_arg			},
     { "args",		2, sysfunc_args			},
     { "show_prompt",	0, sysfunc_show_prompt		},
     { "debug",		0, sysfunc_debug		},
     { "load_script",	1, sysfunc_load_script		},
     { "unload_scriot",	1, sysfunc_unload_script	},
     { "call",		2, sysfunc_call			},
     { "nr",		1, sysfunc_nr			},
     { "str",		1, sysfunc_str			},
   
     { NULL }
};

int last_system_function;


/* Alias and Regex callbacks. */

char *current_line;
int regex_callbacks;
int regex_ovector[30];
int alias_args_start[16];
int alias_args_end[16];

int target_offset;

#define SB_FULL_SIZE 40960
#define SB_HALF_SIZE SB_FULL_SIZE / 2

char searchback_buffer[SB_FULL_SIZE];
int sb_size;

int returning_successful;

SCRIPT *current_script;
CODE *current_code;

SYMBOL_TABLE symbol_table;

INSTR *compile_command( int priority );
int run_code( SCRIPT *script, int code_offset );

VARIABLE *local_memory;
VARIABLE *global_memory;

VARIABLE *current_local_bottom;
VARIABLE *current_local_top;

VARIABLE *top_of_allocated_memory_for_locals;

VARIABLE *function_return_value;



/* Here we register our functions. */

void scripter_module_init_data( );
void scripter_process_server_line( LINE *l );
void scripter_process_server_prompt( LINE *l );
int  scripter_process_client_aliases( char *cmd );


ENTRANCE( scripter_module_register )
{
   self->name = strdup( "IScript" );
   self->version_major = scripter_version_major;
   self->version_minor = scripter_version_minor;
   self->id = i_script_id;
   
   self->init_data = scripter_module_init_data;
   self->process_server_line = scripter_process_server_line;
   self->process_server_prompt = scripter_process_server_prompt;
   self->process_client_command = NULL;
   self->process_client_aliases = scripter_process_client_aliases;
   self->build_custom_prompt = NULL;
   self->main_loop = NULL;
   self->update_descriptors = NULL;
   
   GET_FUNCTIONS( self );
#if defined( FOR_WINDOWS )
   gettimeofday = self->get_func( "gettimeofday" );
#endif
}



void exec_trigger( TRIGGER *trigger )
{
   run_code( trigger->parent, trigger->code_offset );
}



void check_triggers( LINE *l, int prompt )
{
   SCRIPT *script;
   TRIGGER *trigger;
   char *line;
   int len;
   int rc;
   
   for ( script = scripts; script; script = script->next )
     for ( trigger = script->triggers; trigger; trigger = trigger->next )
       {
	  if ( trigger->prompt != prompt )
	    continue;
	  
	  if ( !trigger->raw )
	    {
	       line = l->line;
	       len = l->len;
	    }
	  else
	    {
	       line = l->raw_line;
	       len = l->raw_len;
	    }
	  
	  if ( trigger->everything )
	    {
	       current_line = line;
	       exec_trigger( trigger );
	    }
	  
	  else if ( trigger->regex )
	    {
	       rc = pcre_exec( trigger->pattern, trigger->extra, line, len, 0, 0, regex_ovector, 30 );
	       
	       if ( rc < 0 )
		 continue;
	       
	       /* Regex Matched. Beware of recursiveness! */
	       
	       if ( rc == 0 )
		 regex_callbacks = 10;
	       else
		 regex_callbacks = rc;
	       
	       current_line = line;
	       exec_trigger( trigger );
	       
	       regex_callbacks = 0;
	    }
	  
	  else if ( !cmp( trigger->message, line ) )
	    {
	       current_line = line;
	       exec_trigger( trigger );
	    }
       }
}


void scripter_process_server_line( LINE *l )
{
   DEBUG( "scripter_process_server_line" );
   
   /* Check for triggers. */
   check_triggers( l, 0 );
   
   /* Add the line in our search-back buffer. */
   if ( l->len )
     {
	if ( l->len + sb_size + 10 > SB_FULL_SIZE )
	  {
	     /* Copy the second half onto the first half. */
	     memmove( searchback_buffer, searchback_buffer + SB_HALF_SIZE, SB_HALF_SIZE );
	     sb_size -= SB_HALF_SIZE;
	  }
	
	memcpy( searchback_buffer + sb_size, l->line, l->len );
	memcpy( searchback_buffer + sb_size + l->len, "\n", 2 );
	sb_size += l->len + 1;
     }
}


void scripter_process_server_prompt( LINE *l )
{
   DEBUG( "scripter_process_server_prompt" );
   
   check_triggers( l, 1 );
}



void set_args( char *line )
{
   char *p = line;
   int i;
   
   i = 0;
   
   while ( *p == ' ' )
     p++;
   
   if ( *p )
     alias_args_start[i] = p - line;
   
   while ( *p && *p != '\n' && *p != '\r' )
     {
	if ( *p == ' ' )
	  {
	     alias_args_end[i] = p - line;
	     
	     while ( *p == ' ' )
	       p++;
	     
	     if ( !*p || *p == '\n' || *p == '\r' )
	       break;
	     
	     i++;
	     
	     if ( i >= 16 )
	       return;
	     
	     alias_args_start[i] = p - line;
	  }
	
	p++;
     }
   
   alias_args_end[i] = p - line;
   i++;
   
   while ( i < 16 )
     alias_args_start[i] = alias_args_end[i] = 0, i++;
}



void resize_global_memory( int size )
{
   global_memory = realloc( global_memory, size * sizeof( VARIABLE ) );
   
   (global_memory+target_offset)->type = VAR_STRING;
   share_memory( "target", &(global_memory + target_offset)->string, sizeof( char * ) );
   shared_memory_is_pointer_to_string( "target" );
}


void scripter_module_init_data( )
{
   int add_global_variable( char *name, VARIABLE *constant );
   void load_scripts( );
   
   DEBUG( "scripter_init_data" );
   
   current_line = "";
   set_args( "" );
   
   /* Initialize the common variables. */
   target_offset = add_global_variable( "target", NULL );
   
   resize_global_memory( symbol_table.memory_size );
   
   /* Initialize memory for scripts. */
   if ( !local_memory )
     {
	local_memory = calloc( 32, sizeof( VARIABLE ) );
	current_local_bottom = local_memory;
	current_local_top = local_memory;
	top_of_allocated_memory_for_locals = local_memory + 32;
     }
   
   get_timer( );
   
   load_scripts( );
   
   debugf( "All scripts compiled. (%d microseconds)", get_timer( ) );
}



void print_matched_line( char *string, int match_start, int match_end )
{
   int start, end;
   
   /* Search for a newline backwards. */
   
   start = match_start;
   while ( start > 0 && string[start] != '\n' )
     start--;
   if ( string[start] == '\n' )
     start++;
   
   /* Search for a newline forward. */
   
   end = match_end;
   while ( string[end] && string[end] != '\n' )
     end++;
   
   clientff( "> %.*s" C_R "%.*s" C_0 "%.*s\n",
	     match_start - start, string + start,
	     match_end - match_start, string + match_start,
	     end - match_end, string + match_end );
}


void do_searchback( char *pattern_string )
{
   pcre *pattern;
   pcre_extra *extra;
   const char *error;
   int error_offset;
   int offset;
   int ovector[3];
   int rc, i = 0;
   
   if ( !pattern_string[0] )
     {
	clientfr( "No pattern specified!" );
	return;
     }
   
   if ( !sb_size )
     {
	clientfr( "Nothing to search in." );
	return;
     }
   
   pattern = pcre_compile( pattern_string, PCRE_MULTILINE, &error, &error_offset, NULL );
   
   if ( error )
     {
	clientff( C_R "[Error at character %d: %s]\r\n" C_0, error_offset, error );
	return;
     }
   
   extra = pcre_study( pattern, 0, &error );
   
   if ( error )
     {
	clientff( C_R "[Pattern Study: %s]\r\n" C_0, error );
	if ( extra )
	  {
	     free( extra );
	     extra = NULL;
	  }
     }
   
   offset = 0;
   
   while( offset < sb_size )
     {
	rc = pcre_exec( pattern, extra, searchback_buffer, sb_size-1,
			offset, 0, ovector, 3 );
	
	if ( rc >= 0 )
	  {
	     if ( ovector[1] - ovector[0] > 4000 )
	       clientff( "(pattern too big to display, %d bytes matched)\r\n", ovector[1] - ovector[0] );
	     else
	       print_matched_line( searchback_buffer, ovector[0], ovector[1] );
	  }
	else
	  break;
	
	offset = ovector[1];
	
	/* Skip to the next new line. */
//	while( searchback_buffer[offset] && searchback_buffer[offset] != '\n' )
//	  {
//	     offset++;
//	  }
//	if ( searchback_buffer[offset] )
//	  offset++;
	if ( !searchback_buffer[offset] )
	  break;
	
	if ( ++i > 100 )
	  {
	     clientfr( "Only 100 matches shown." );
	     break;
	  }
     }
   
   /* Free it all up. */
   
   free( pattern );
   if ( extra )
     free( extra );
}



int scripter_process_client_aliases( char *line )
{
   SCRIPT *script;
   FUNCTION *function;
   char command[4096], *args;
   
   DEBUG( "scripter_process_client_aliases" );
   
   if ( !strcmp( line, "load" ) )
     {
	void load_scripts( );
	load_scripts( );
	show_prompt( );
	return 1;
     }
   else if ( !strcmp( line, "show" ) || !strncmp( line, "show ", 5 ) )
     {
	void do_show( );         
	char buf[256];
	
	get_string( line+4, buf, 256 );
	do_show( !strcmp( buf, "full" ) );
	show_prompt( );
	return 1;
     }
   else if ( !strcmp( line, "scripts" ) )
     {
	SCRIPT *script;
	FUNCTION *f;
	TRIGGER *t;
	int f_count, a_count, t_count;
	
	if ( !scripts )
	  clientfr( "No scripts loaded." );
	else
	  clientfr( "Loaded scripts:" );
	
	for ( script = scripts; script; script = script->next )
	  {
	     clientff( C_W "* " C_B "%s" C_0 " (" C_B "%s" C_0 ")\r\n",
		       script->name, script->description );
	     
	     f_count = a_count = t_count = 0;
	     
	     for ( f = script->functions; f; f = f->next )
	       {
		  if ( f->alias )
		    a_count++;
		  else
		    f_count++;
	       }
	     
	     for ( t = script->triggers; t; t = t->next )
	       t_count++;
	     
	     clientff( " " C_G "%d" C_D " function%s" C_0 "\r\n",
		       f_count, f_count == 1 ? "" : "s" );
	     clientff( " " C_G "%d" C_D " alias%s" C_0 "\r\n",
		       a_count, a_count == 1 ? "" : "es" );
	     clientff( " " C_G "%d" C_D " trigger%s" C_0 "\r\n",
		       t_count, t_count == 1 ? "" : "s" );
	     
	     /* Function List.
	       {
		  clientff( C_D " %s%s (" C_G "%d" C_D " bytes)" C_0 "\r\n",
			    f->name,
			    f->alias ? C_D " [" C_W "A" C_D "]" : "",
			    f->size );
	       } */
	  }
	
	show_prompt( );
	return 1;
     }
   else if ( !strncmp( line, "callfunc ", 9 ) )
     {
	int run_function_with_args( FUNCTION *function, char *args );
	SCRIPT *s;
	FUNCTION *f;
	char name[256];
	
	line = get_string( line + 9, name, 256 );
	
	f = NULL;
	for ( s = scripts; s; s = s->next )
	  for ( f = s->functions; f; f = f->next )
	    if ( !strcmp( name, f->name ) )
	      break;
	
	if ( !f )
	  {
	     clientfr( "Function not found. Perhaps you misspelled it?" );
	     show_prompt( );
	     return 1;
	  }
	
	current_line = line;
	set_args( line );
	
	run_function_with_args( f, line );
	show_prompt( );
	return 1;
     }
   else if ( !strncmp( line, "grep ", 5 ) )
     {
	do_searchback( line + 5 );
	show_prompt( );
	return 1;
     }
   
   args = get_string( line, command, 4096 );
   
   /* Look around the function list. */
   
   for ( script = scripts; script; script = script->next )
     for ( function = script->functions; function; function = function->next )
       if ( function->alias && !strcmp( function->name, command ) )
	 {
	    current_line = line;
	    set_args( line );
	    
	    run_code( function->parent, function->code_offset );
	    
	    return 1;
	 }
   
   return 0;
}



/*** iScript System ***/



void skip_whitespace( )
{
   while ( *pos && ( *pos == ' ' || *pos == '\n' || *pos == '\r' || *pos == '\t' ) )
     pos++;
   
   /* Consider comments as whitespace. */
   if ( *pos == '/' )
     {
	if ( *(pos+1) == '/' )
	  {
	     while ( *pos && *pos != '\n' )
	       pos++;
	     skip_whitespace( );
	  }
	else if ( *(pos+1) == '*' )
	  {
	     while( *pos && ( *pos != '*' || *(pos+1) != '/' ) )
	       pos++;
	     if ( *pos )
	       pos += 2;
	     skip_whitespace( );
	  }
     }
}



// Test me.
void get_identifier( char *dest, int max )
{
   skip_whitespace( );
   
   while ( --max && ( ( *pos >= 'A' && *pos <= 'Z' ) ||
		      ( *pos >= 'a' && *pos <= 'z' ) ||
		      ( *pos >= '0' && *pos <= '9' ) ||
		      *pos == '_' ) )
     *(dest++) = *(pos++);
   
   *dest = 0;
   
   skip_whitespace( );
}



void abort_script( int syntax, char *error, ... )
{
   char buf[1024];
   char where[1024];
   char what[1024];
   char error_buf[1024];
   int line = 0, column = 0;
   
   va_list args;
   error_buf[0] = 0;
   if ( error )
     {
	va_start( args, error );
	vsnprintf( error_buf, 1024, error, args );
	va_end( args );
     }
   
   /* Find the current line/column in the file. */
   if ( beginning && current_script && current_script->name )
     {
	char *p = beginning;
	
	while ( *p )
	  {
	     if ( *p == '\n' )
	       line++, column = 0;
	     else if ( *p != '\r' )
	       column++;
	     
	     p++;
	     if ( p == pos )
	       break;
	  }
	
	sprintf( where, " in file %s:%d", current_script->name, line );
     }
   else
     where[0] = 0;
   
   /* Figure out what exactly is at pos. */
   if ( pos )
     get_identifier( buf, 512 );
   
   if ( !pos )
     what[0] = 0;
   else if ( buf[0] )
     sprintf( what, " before '%s'", buf );
   else if ( !*pos )
     strcpy( what, " before the End of Buffer" );
   else
     sprintf( what, " before character '%c'", *pos );
   
   sprintf( buf, "%s%s%s%c", syntax ? "Syntax error" : "Compile error",
	    where, what, error ? ':' : '.' );
   
   clientff( C_0 "(" C_W "iScript" C_0 "): " C_R "%s\r\n" C_0, buf );
   if ( error )
     clientff( C_0 "(" C_W "iScript" C_0 "): " C_R "%s\r\n" C_0, error_buf );
   
   pos = NULL;
}



void script_warning( char *warning, ... )
{
   char buf[1024];
   
   va_list args;
   
   buf[0] = 0;
   
   if ( warning )
     {
	va_start( args, warning );
	vsnprintf( buf, 1024, warning, args );
	va_end( args );
     }
   
   clientff( C_0 "(" C_W "iScript" C_0 "): " C_R "Warning: %s\r\n" C_0, buf );
}



void clear_value( VARIABLE *v )
{
   if ( v->type == VAR_STRING && v->string )
     {
	free( v->string );
	v->string = NULL;
     }
   
   v->type = VAR_NUMBER;
   v->nr = 0;
}



void copy_value( VARIABLE *v1, VARIABLE *v2, int move )
{
   if ( v2->type == VAR_STRING && v2->string )
     free( v2->string );
   
//   if ( v2->type &&
//	v2->type != VAR_NUMBER &&
//	v2->type != VAR_STRING &&
//	v2->type != VAR_POINTER )
  //   script_warning( "Assigning to an uninitialized variable!" );
   
   v2->type = v1->type;
   
   if ( v1->type == VAR_STRING )
     {
	if ( move )
	  v2->string = v1->string;
	else
	  v2->string = strdup( v1->string );
     }
   else if ( v1->type == VAR_NUMBER )
     {
	v2->nr = v1->nr;
     }
   else if ( v1->type == VAR_POINTER )
     {
	v2->pointer = v1->pointer;
     }
}


void copy_address( POINTER pointer, VARIABLE *v )
{
   /* If relative, convert its address to absolute.
    * We don't want pointers pointing to relative locations. */
   
   if ( pointer.memory_space == RELATIVE_LOCAL_MEMORY )
     {
	v->address.memory_space = ABSOLUTE_LOCAL_MEMORY;
	v->address.offset = pointer.offset + ( current_local_bottom - local_memory );
     }
   else
     {
	v->address.memory_space = pointer.memory_space;
	v->address.offset = pointer.offset;
     }
}


void destroy_symbol_table( SYMBOL_TABLE table )
{
   int i;
   
   for ( i = 0; i < table.nr; i++ )
     {
	if ( table.symbol[i].name )
	  free( table.symbol[i].name );
	
	clear_value( &table.symbol[i].constant );
     }
   
   if ( table.symbol )
     free( table.symbol );
}



void destroy_instruction( INSTR *instr )
{
   int i;
   
   clear_value( &instr->constant );
   
   if ( instr->var_name )
     free( instr->var_name );
   
   for ( i = 0; i < instr->links; i++ )
     destroy_instruction( instr->link[i] );
   
   if ( instr->link )
     free( instr->link );
   
   free( instr );
}



void destroy_code( CODE code )
{
   destroy_symbol_table( code.symbol_table );
   
   if ( code.first_instruction )
     destroy_instruction( code.first_instruction );
}



void destroy_script( SCRIPT *script )
{
   SCRIPT *s;
   FUNCTION *f, *f_next;
   TRIGGER *t, *t_next;
   int i;
   
   /* Unlink it. */
   if ( script == scripts )
     scripts = script->next;
   else
     for ( s = scripts; s; s = s->next )
       {
	  if ( s->next == script )
	    {
	       s->next = script->next;
	       break;
	    }
       }
   
   if ( script->name )
     free( script->name );
   if ( script->description )
     free( script->description );
   
   for ( i = 0; i < script->codes_nr; i++ )
     destroy_code( script->codes[i] );
   
   if ( script->codes )
     free( script->codes );
   
   for ( f = script->functions; f; f = f_next )
     {
	f_next = f->next;
	
	if ( f->name )
	  free( f->name );
	
	free( f );
     }
   
   for ( t = script->triggers; t; t = t_next )
     {
	t_next = t->next;
	
	if ( t->name )
	  free( t->name );
	if ( t->message )
	  free( t->message );
	if ( t->pattern )
	  free( t->pattern );
	if ( t->extra )
	  free( t->extra );
	
	free( t );
     }
   
   destroy_symbol_table( script->symbol_table );
   
   if ( script->script_memory )
     free( script->script_memory );
   
   free( script );
}



int add_symbol( SYMBOL_TABLE *table, int memory_space, char *name, VARIABLE *constant )
{
   int i;
   
   /* Already here? */
   for ( i = 0; i < table->nr; i++ )
     if ( !strcmp( name, table->symbol[i].name ) )
       {
	  if ( constant )
	    {
	       if ( table->symbol[i].is_constant )
		 {
		    abort_script( 0, "Constant was already defined somewhere else." );
		    return -1;
		 }
	       
	       copy_value( constant, &(table->symbol[i].constant), 1 );
	       table->symbol[i].is_constant = 1;
	    }
	  
	  return i;
       }
   
   table->symbol = realloc( table->symbol, sizeof( SYMBOL ) * (table->nr + 1) );
   memset( &table->symbol[table->nr], 0, sizeof ( SYMBOL ) );
   table->symbol[table->nr].name = strdup( name );
   
   if ( constant )
     {
	copy_value( constant, &(table->symbol[table->nr].constant), 1 );
	table->symbol[table->nr].is_constant = 1;
     }
   else
     {
	table->symbol[table->nr].pointer.offset = table->memory_size;
	table->symbol[table->nr].pointer.memory_space = memory_space;
	
	table->memory_size++;
     }
   
   return table->nr++;
}



int add_local_variable( CODE *code, char *name, VARIABLE *constant )
{
   if ( !code )
     return -1;
   
   return add_symbol( &code->symbol_table, RELATIVE_LOCAL_MEMORY, name, constant );
}

int add_script_variable( SCRIPT *script, char *name, VARIABLE *constant )
{
   if ( !script )
     return -1;
   
   return add_symbol( &script->symbol_table, SCRIPT_MEMORY, name, constant );
}

int add_global_variable( char *name, VARIABLE *constant )
{
   return add_symbol( &symbol_table, GLOBAL_MEMORY, name, constant );
}



INSTR *new_instruction( int instruction )
{
   INSTR *instr;
   
   instr = calloc( 1, sizeof( INSTR ) );
   
   instr->instruction = instruction;
   
   return instr;
}



void link_instruction( INSTR *child, INSTR *parent )
{
   parent->link = realloc( parent->link, sizeof( INSTR * ) * (parent->links+1) );
   parent->link[parent->links] = child;
   parent->links++;
}



INSTR *compile_operator( INSTR *instr, int priority )
{
   INSTR *instr2, *i_oper, *i_func;
   char *temp_pos, buf[4096];
   int expect_ending_bracket = 0;
   int i;
   
   skip_whitespace( );
   
   /* Check for function calls. */
   if ( *pos == '(' || ( instr->sys_function && instr->instruction == INSTR_VARIABLE ) )
     {
	if ( *pos == '(' )
	  pos++, expect_ending_bracket = 1;
	
	i_func = new_instruction( INSTR_FUNCTION );
	
	if ( !instr->sys_function )
	  {
	     /* First child instruction of a function, is its address. */
	     link_instruction( instr, i_func );
	     i_func->sys_function = 0;
	  }
	else
	  {
	     i_func->pointer.offset = instr->sys_function - 1;
	     i_func->sys_function = 1 + sys_functions[i_func->pointer.offset].args;
	     destroy_instruction( instr );
	  }
	
	i = 0;
	/* Next, are arguments. */
	while ( 1 )
	  {
	     skip_whitespace( );
	     
	     /* Eh... It's a mess, but I couldn't be bothered to think. */
	     if ( i_func->sys_function )
	       {
		  if ( i_func->sys_function == 1 )
		    {
		       if ( expect_ending_bracket )
			 {
			    if ( *pos == ')' )
			      {
				 pos++;
				 break;
			      }
			    else
			      {
				 abort_script( 1, "Expected ')' instead." );
				 return NULL;
			      }
			 }
		       break;
		    }
		  i_func->sys_function--;
	       }
	     
	     if ( expect_ending_bracket && *pos == ')' )
	       {
		  if ( i_func->sys_function )
		    {
		       /* If it got here, then sys_function was not '1'. */
		       abort_script( 1, "Not enough arguments passed to a system function." );
		       destroy_instruction( i_func );
		       return NULL;
		    }
		  
		  pos++;
		  break;
	       }
	     
	     if ( i )
	       {
		  if ( *pos != ',' )
		    {
		       abort_script( 1, "Expected ',' between function arguments, or ')'." );
		       destroy_instruction( i_func );
		       return NULL;
		    }
		  else
		    pos++;
	       }
	     
	     instr2 = compile_command( FULL_PRIORITY );
	     if ( !pos )
	       {
		  destroy_instruction( i_func );
		  return NULL;
	       }
	     
	     link_instruction( instr2, i_func );
	     i++;
	  }
	
	return compile_operator( i_func, priority );
     }
   
   temp_pos = pos;
   get_identifier( buf, 4096 );
   pos = temp_pos;
   
   /* Check for right-sided unary operators. */
   for ( i = 0; operators[i].symbol; i++ )
     {
	if ( !operators[i].unary || !operators[i].rightsided )
	  continue;
	
	if ( buf[0] )
	  {
	     if ( strcmp( buf, operators[i].symbol ) )
	       continue;
	  }
	else
	  if ( strncmp( pos, operators[i].symbol, strlen( operators[i].symbol ) ) )
	    continue;
	
	if ( operators[i].function == oper_not_implemented )
	  script_warning( "Operator '%s' not yet implemented.", operators[i].symbol );
	
	pos += strlen( operators[i].symbol );
	
	instr2 = new_instruction( INSTR_OPERATOR );
	instr2->oper = i;
	
	link_instruction( instr2, instr );
	
	return compile_operator( instr, priority );
     }
   
   /* Check for binary operators. */
   for ( i = 0; operators[i].symbol; i++ )
     {
	if ( operators[i].unary || operators[i].rightsided )
	  continue;
	
	if ( buf[0] )
	  {
	     if ( strcmp( buf, operators[i].symbol ) )
	       continue;
	  }
	else
	  if ( strncmp( pos, operators[i].symbol, strlen( operators[i].symbol ) ) )
	    continue;
	
	/* Priority doesn't allow us to go with this operator? */
	if ( priority < operators[i].priority )
	  return instr;
	
	if ( operators[i].function == oper_not_implemented )
	  script_warning( "Operator '%s' not yet implemented.", operators[i].symbol );
	
	pos += strlen( operators[i].symbol );
	
	/* Right-to-left associativity ( a = b = c, is ( a = ( b = c ) ) )
	 * will not let operators of the same type. */
	instr2 = compile_command( operators[i].priority - !operators[i].right_to_left );
	
	if ( !pos )
	  {
	     destroy_instruction( instr );
	     return NULL;
	  }
	
	i_oper = new_instruction( INSTR_OPERATOR );
	i_oper->oper = i;
	
	link_instruction( instr, i_oper );
	link_instruction( instr2, i_oper );
	
	return compile_operator( i_oper, priority );
     }
   
   return instr;
}



// This could be changed to return a pointer to an malloc'ed string. */
void get_text( char *dest, int max )
{
   skip_whitespace( );
   
   if ( *pos != '\"' )
     {
	abort_script( 1, "Expected a string." );
	return;
     }
   
   pos++;
   
   while ( --max && *pos && *pos != '\"' )
     {
	if ( *pos == '\\' )
	  {
	     pos++;
	     switch( *pos )
	       {
		case 'n':
		  *(dest++) = '\n'; break;
		case 'r':
		  *(dest++) = '\r'; break;
		case '\\':
		  *(dest++) = '\\'; break;
		case 'e':
		  *(dest++) = '\33'; break;
		case '\"':
		  *(dest++) = '\"'; break;
		default:
		  *(dest++) = *pos;
	       }
	     pos++;
	  }
	else
	  *(dest++) = *(pos++);
     }
   
   *dest = 0;
   
   if ( *pos != '\"' )
     {
	abort_script( 1, "Expected end of string." );
	return;
     }
   
   pos++;
   skip_whitespace( );
}



int get_number( )
{
   int nr = 0;
   
   if ( *pos < '0' || *pos > '9' )
     {
	abort_script( 1, "Expected numeric value." );
	return 0;
     }
   
   /* Decimal. */
   
   if ( *pos != '0' )
     {
	while ( *pos >= '0' && *pos <= '9' )
	  {
	     nr *= 10;
	     nr += (int) (*pos - '0');
	     
	     pos++;
	  }
	
	return nr;
     }
   
   pos++;
   
   /* Hexadecimal. */
   
   if ( *pos == 'x' || *pos == 'X' )
     {
	pos++;
	
	if ( !( ( *pos >= '0' && *pos <= '9' ) ||
		( *pos >= 'a' && *pos <= 'f' ) ||
		( *pos >= 'A' && *pos <= 'F' ) ) )
	  {
	     abort_script( 1, "Invalid hexadecimal constant." );
	     return 0;
	  }
	
	while ( ( *pos >= '0' && *pos <= '9' ) ||
		( *pos >= 'a' && *pos <= 'f' ) ||
		( *pos >= 'A' && *pos <= 'F' ) )
	  {
	     nr *= 16;
	     
	     if ( *pos >= '0' && *pos <= '9' )
	       nr += (int) (*pos - '0');
	     if ( *pos >= 'a' && *pos <= 'f' )
	       nr += (int) (*pos - 'a') + 10;
	     if ( *pos >= 'A' && *pos <= 'F' )
	       nr += (int) (*pos - 'A') + 10;
	     
	     pos++;
	  }
	
	return nr;
     }
   
   /* Octal. */
   
   while ( *pos >= '0' && *pos <= '8' )
     {
	nr *= 8;
	nr += (int) (*pos - '0');
	
	pos++;
	
     }
   
   return nr;
}



INSTR *compile_command( int priority )
{
   INSTR *instr, *i_oper;
   char *temp_pos, buf[4096];
   int nr, i;
   
   skip_whitespace( );
   
   /* Constant string. */
   if ( *pos == '\"' )
     {
	get_text( buf, 4096 );
	
	if ( !pos )
	  return NULL;
	
	instr = new_instruction( INSTR_CONSTANT );
	
	instr->constant.type = VAR_STRING;
	instr->constant.string = strdup( buf );
	
	return compile_operator( instr, priority );
     }
   
   /* Constant number. */
   if ( *pos >= '0' && *pos <= '9' )
     {
	nr = get_number( );
	
	if ( !pos )
	  return NULL;
	
	instr = new_instruction( INSTR_CONSTANT );
	instr->constant.type = VAR_NUMBER;
	instr->constant.nr = nr;
	
	return compile_operator( instr, priority );
     }
   
   if ( *pos == '(' )
     {
	pos++;
	
	instr = compile_command( FULL_PRIORITY );
	
	if ( !pos )
	  return NULL;
	
	skip_whitespace( );
	
	if ( *pos != ')' )
	  {
	     abort_script( 1, "Expected ')' instead." );
	     return NULL;
	  }
	
	pos++;
	
	return compile_operator( instr, priority );
     }
   
   /* Variable, function, system function, etc. */
   if ( ( *pos >= 'a' && *pos <= 'z' ) ||
	( *pos >= 'A' && *pos <= 'Z' ) ||
	*pos == '_' )
     {
	get_identifier( buf, 4096 );
	
	instr = new_instruction( INSTR_VARIABLE );
	instr->var_name = strdup( buf );
	
	/* Specifically declared as local? */
	for ( i = 0; i < current_code->symbol_table.nr; i++ )
	  if ( !strcmp( current_code->symbol_table.symbol[i].name, buf ) &&
	       !current_code->symbol_table.symbol[i].invisible )
	    {
	       instr->defined_as_local = 1;
	       break;
	    }
	
	/* Or global? */
	for ( i = 0; i < symbol_table.nr; i++ )
	  if ( !strcmp( symbol_table.symbol[i].name, buf ) &&
	       !symbol_table.symbol[i].invisible )
	    {
	       instr->defined_as_global = 1;
	       break;
	    }
	
	/* System functions don't need parantheses. For compile_operator. */
	for ( i = 0; sys_functions[i].name; i++ )
	  if ( !strcmp( sys_functions[i].name, buf ) )
	    instr->sys_function = 1 + i;
	
	return compile_operator( instr, priority );
     }
   
   temp_pos = pos;
   get_identifier( buf, 4096 );
   pos = temp_pos;
   
   /* Check for unary operators. Like !var. */
   for ( i = 0; operators[i].symbol; i++ )
     {
	if ( !operators[i].unary || operators[i].rightsided )
	  continue;
	
	if ( buf[0] )
	  {
	     if ( strcmp( buf, operators[i].symbol ) )
	       continue;
	  }
	else
	  if ( strncmp( pos, operators[i].symbol, strlen( operators[i].symbol ) ) )
	    continue;
	
	if ( operators[i].function == oper_not_implemented )
	  script_warning( "Operator '%s' not yet implemented.", operators[i].symbol );
	
	pos += strlen( operators[i].symbol );
	
	i_oper = new_instruction( INSTR_OPERATOR );
	i_oper->oper = i;
	
	instr = compile_command( UNARY_PRIORITY );
	
	if ( !pos )
	  return NULL;
	
	link_instruction( instr, i_oper );
	
	return compile_operator( i_oper, priority );
     }
   
   abort_script( 1, NULL );
   return NULL;
}



int compile_block( INSTR *parent )
{
   INSTR *do_all, *c_instr, *instr;
   char *temp_pos, buf[4096];
   int expect_end_of_block = 0;
   
   if ( !pos )
     return 1;
   
   skip_whitespace( );
   
   if ( *pos == '{' )
     expect_end_of_block = 1, pos++;
   
   do_all = new_instruction( INSTR_DOALL );
   link_instruction( do_all, parent );
   
   while ( 1 )
     {
	skip_whitespace( );
	
	if ( !*pos )
	  {
	     abort_script( 1, NULL );
	     return 1;
	  }
	
	/* Done here? */
	if ( expect_end_of_block && *pos == '}' )
	  {
	     pos++;
	     return 0;
	  }
	
	/* Block in a block? */
	if ( *pos == '{' )
	  {
	     compile_block( do_all );
	     
	     if ( !pos )
	       return 1;
	     
	     continue;
	  }
	
	/* Block controls or special keywords? */
	
	temp_pos = pos;
	get_identifier( buf, 4096 );
	
	if ( !strcmp( buf, "if" ) )
	  {
	     /* if <command> <block> [else <block>] */
	     
	     skip_whitespace( );
	     
	     if ( *pos != '(' )
	       {
		  abort_script( 1, "Expected '(' instead." );
		  return 1;
	       }
	     pos++;
	     
	     c_instr = new_instruction( INSTR_IF );
	     link_instruction( c_instr, do_all );
	     
	     instr = compile_command( FULL_PRIORITY );
	     
	     if ( !pos )
	       return 1;
	     
	     link_instruction( instr, c_instr );
	     
	     skip_whitespace( );
	     
	     if ( *pos != ')' )
	       {
		  abort_script( 1, "Expected ')' instead." );
		  return 1;
	       }
	     pos++;
	     
	     /* Execute of true. */
	     compile_block( c_instr );
	     if ( !pos )
	       return 1;
	     
	     temp_pos = pos;
	     get_identifier( buf, 4096 );
	     
	     /* Execute of false. */
	     if ( !strcmp( buf, "else" ) )
	       {
		  compile_block( c_instr );
		  if ( !pos )
		    return 1;
	       }
	     else
	       pos = temp_pos;
	     
	     if ( expect_end_of_block )
	       continue;
	     else
	       return 1;
	  }
	else if ( !strcmp( buf, "return" ) )
	  {
	     c_instr = new_instruction( INSTR_RETURN );
	     link_instruction( c_instr, do_all );
	     
	     skip_whitespace( );
	     
	     if ( *pos != ';' )
	       {
		  instr = compile_command( FULL_PRIORITY );
		  if ( !pos )
		    return 1;
		  link_instruction( instr, c_instr );
	       }
	  }
	else if ( !strcmp( buf, "local" ) || !strcmp( buf, "global" ) )
	  {
	     int global;
	     
	     global = !strcmp( buf, "global" );
	     
	     while ( *pos != ';' )
	       {
		  skip_whitespace( );
		  
		  get_identifier( buf, 4096 );
		  
		  if ( !buf[0] )
		    {
		       abort_script( 1, "Expected identifier after 'local' keyword." );
		       return 1;
		    }
		  
		  if ( global && add_global_variable( buf, NULL ) < 0 )
		    return 1;
		  else if ( !global && add_local_variable( current_code, buf, NULL ) < 0 )
		    return 1;
		  
		  skip_whitespace( );
		  
		  if ( *pos == '=' )
		    {
		       int i;
		       
		       pos++;
		       
		       for ( i = 0; operators[i].symbol; i++ )
			 if ( !strcmp( operators[i].symbol, "=" ) )
			   break;
		       
		       if ( !operators[i].symbol )
			 {
			    abort_script( 0, "Internal error... Operator table is buggy!" );
			    return 1;
			 }
		       
		       c_instr = new_instruction( INSTR_OPERATOR );
		       link_instruction( c_instr, do_all );
		       c_instr->oper = i;
		       
		       instr = new_instruction( INSTR_VARIABLE );
		       link_instruction( instr, c_instr );
		       instr->var_name = strdup( buf );
		       if ( global )
			 instr->defined_as_global = 1;
		       else
			 instr->defined_as_local = 1;
		       
		       instr = compile_command( FULL_PRIORITY );
		       if ( !pos )
			 return 1;
		       link_instruction( instr, c_instr );
		       
		       skip_whitespace( );
		    }
		  
		  if ( *pos == ',' )
		    {
		       pos++;
		       continue;
		    }
		  
		  break;
	       }
	  }
	
	// While's, For's, etc.
	
	/* We have a single, normal command here. */
	else
	  {
	     pos = temp_pos;
	     
	     instr = compile_command( FULL_PRIORITY );
	     
	     if ( !pos )
	       return 1;
	     
	     link_instruction( instr, do_all );
	  }
	
	skip_whitespace( );
	
	if ( *pos != ';' )
	  {
	     abort_script( 0, "Expected ';' instead." );
	     return 1;
	  }
	
	pos++;
	
	if ( expect_end_of_block )
	  continue;
	else
	  break;
     }
   
   return 0;
}


int compile_code( CODE *code )
{
   INSTR *instr;
   CODE *previous_code;
   
   previous_code = current_code;
   current_code = code;
   
   instr = new_instruction( INSTR_DOALL );
   code->first_instruction = instr;
   
   if ( compile_block( instr ) )
     {
	current_code = previous_code;
	return 1;
     }
   
   current_code = previous_code;
   
   if ( !pos )
     return 1;
   
   return 0;
}



int load_script_function( SCRIPT *script, int alias )
{
   FUNCTION *function, *f;
   VARIABLE v;
   CODE *code;
   char buf[4096];
   int expect_ending, expect_argument;
   
   /* A function must have:
    *   - a name
    *   - a code
    *   - arguments as local variables
    *   - the number of arguments
    */
   
   function = calloc( 1, sizeof( FUNCTION ) );
   function->parent = script;
   
   /* Add a code in the script for it. */
   script->codes = realloc( script->codes, sizeof( CODE ) * ( script->codes_nr + 1 ) );
   memset( &script->codes[script->codes_nr], 0, sizeof( CODE ) );
   function->code_offset = script->codes_nr;
   code = &script->codes[script->codes_nr];
   
   /* Make a pointer for it. */
   v.type = VAR_POINTER;
   v.pointer.memory_space = SCRIPT_FUNCTIONS;
   v.pointer.offset = script->codes_nr++;
   
   /* Link it. Easier to get destroyed with the rest if something fails. */
   if ( script->functions )
     {
	for ( f = script->functions; f->next; f = f->next );
	f->next = function;
     }
   else
     script->functions = function;
   
   if ( !alias )
     {
	/* Format: function <name> [ [(] [arg1 [[,] arg2]... ] [)] ] { ... } */
	get_identifier( buf, 4096 );
	
	if ( !buf[0] )
	  {
	     abort_script( 0, "Function name not provided." );
	     return 1;
	  }
	
	function->name = strdup( buf );
	
	if ( *pos == '(' )
	  {
	     expect_ending = 1;
	     pos++;
	     skip_whitespace( );
	  }
	else
	  expect_ending = 0;
	
	/* Find all arguments. */
	while ( 1 )
	  {
	     if ( function->args_nr && *pos == ',' )
	       {
		  expect_argument = 1;
		  pos++;
	       }
	     else
	       expect_argument = 0;
	     
	     get_identifier( buf, 4096 );
	     
	     if ( !buf[0] )
	       break;
	     
	     if ( expect_argument )
	       expect_argument = 0;
	     add_local_variable( code, buf, NULL );
	     function->args_nr++;
	  }
	
	if ( expect_argument )
	  {
	     abort_script( 0, "Expected a function argument." );
	     return 1;
	  }
	
	if ( expect_ending )
	  {
	     if ( *pos != ')' )
	       {
		  abort_script( 0, "Mismatched '(', expected ')' on argument list." );
		  return 1;
	       }
	     
	     pos++;
	     skip_whitespace( );
	  }
     }
   else
     {
	/* Format: alias <name> { ... } */
	get_identifier( buf, 4096 );
	
	if ( !buf[0] )
	  {
	     abort_script( 0, "Alias name not provided." );
	     return 1;
	  }
	
	function->name = strdup( buf );
	function->alias = 1;
     }
   
   /* Common code for 'function' and 'alias'. */
   
   if ( add_script_variable( script, function->name, &v ) < 0 )
     return 1;
   
   if ( compile_code( code ) )
     return 1;
   
   return 0;
}



int load_script_trigger( SCRIPT *script )
{
   TRIGGER *trigger, *t;
   CODE *code;
   char buf[4096];
   
   /* Format: trigger ['<name>'] [options] <*|"pattern"> { ... } */
   
   trigger = calloc( 1, sizeof( TRIGGER ) );
   trigger->parent = script;
   
   /* Add a code in the script for it. */
   script->codes = realloc( script->codes, sizeof( CODE ) * ( script->codes_nr + 1 ) );
   memset( &script->codes[script->codes_nr], 0, sizeof( CODE ) );
   trigger->code_offset = script->codes_nr;
   code = &script->codes[script->codes_nr++];
   
   /* Link it. */
   if ( script->triggers )
     {
	for ( t = script->triggers; t->next; t = t->next );
	t->next = trigger;
     }
   else
     script->triggers = trigger;
   
   /* Name, if there's one. */
   if ( *pos == '\'' )
     {
	pos++;
	
	get_identifier( buf, 4096 );
	
	if ( *pos != '\'' )
	  {
	     abort_script( 1, "Invalid trigger name." );
	     return 1;
	  }
	
	pos++;
	
	if ( buf[0] )
	  trigger->name = strdup( buf );
     }
   
   /* Options, if there are any. */
   while ( 1 )
     {
	get_identifier( buf, 4096 );
	
	if ( !buf[0] )
	  break;
	
	if ( !strcmp( buf, "prompt" ) )
	  trigger->prompt = 1;
	else if ( !strcmp( buf, "raw" ) )
	  trigger->raw = 1;
	else if ( !strcmp( buf, "regex" ) )
	  trigger->regex = 1;
	else if ( !strcmp( buf, "continuous" ) )
	  trigger->continuous = 1;
	else
	  {
	     if ( trigger->name )
	       abort_script( 1, "Unknown option in trigger '%s'.", trigger->name );
	     else
	       abort_script( 1, "Unknown option in a trigger definition." );
	     return 1;
	  }
     }
   
   /* Pattern. */
   
   if ( *pos == '*' )
     {
	trigger->everything = 1;
	pos++;
	skip_whitespace( );
     }
   else
     {
	get_text( buf, 4096 );
	
	if ( !pos )
	  return 1;
	
	if ( !buf[0] )
	  {
	     abort_script( 0, "Empty pattern." );
	     return 1;
	  }
	
	if ( trigger->regex )
	  {
	     const char *error;
	     int e_offset;
	     
	     trigger->pattern = pcre_compile( buf, 0, &error, &e_offset, NULL );
	     
	     if ( error )
	       {
		  trigger->pattern = NULL;
		  abort_script( 0, "Pattern error, offset %d: %s.", e_offset, error );
		  return 1;
	       }
	     
	     trigger->extra = pcre_study( trigger->pattern, 0, &error );
	     
	     if ( error )
	       script_warning( "Pattern study error: %s.", error );
	  }
	else
	  trigger->message = strdup( buf );
     }
   
   if ( compile_code( code ) )
     return 1;
   
   return 0;
}



SCRIPT *load_script( const char *flname )
{
   int link_symbols( CODE *code, INSTR *instr );
   SCRIPT *script, *s, *previous_script;
   FILE *fl;
   char buf[4096];
   char *old_pos, *old_beginning;
   char *body = NULL;
   int bytes, size, i;
   int aborted = 0;
   
   fl = fopen( flname, "r" );
   
   if ( !fl )
     return NULL;
   
   /* Load the code that is to be compiled. */
   
   while ( 1 )
     {
	bytes = fread( buf, 1, 4096, fl );
	
	if ( !bytes )
	  break;
	
	if ( !body )
	  {
	     size = bytes;
	     body = malloc( size + 1 );
	     memcpy( body, buf, bytes );
	  }
	else
	  {
	     body = realloc( body, size + 1 + 4096 );
	     memcpy( body + size, buf, bytes );
	     size += bytes;
	  }
     }
   
   fclose( fl );
   if ( !body )
     {
	script_warning( "Empty script file." );
	return NULL;
     }
   
   body[size] = 0;
   
   script = calloc( 1, sizeof( SCRIPT ) );
   
   script->name = strdup( flname );
   
   /* Seek all functions/triggers within it. */
   
   previous_script = current_script;
   current_script = script;
   
   old_pos = pos;
   pos = body;
   old_beginning = beginning;
   beginning = body;
   
   while ( 1 )
     {
	skip_whitespace( );
	
	if ( !*pos )
	  break;
	
	get_identifier( buf, 4096 );
	if ( !buf[0] )
	  {
	     abort_script( 1, NULL );
	     aborted = 1;
	     break;
	  }
	
	if ( !strcmp( buf, "function" ) )
	  {
	     if ( load_script_function( script, 0 ) )
	       {
		  aborted = 1;
		  break;
	       }
	  }
	
	else if ( !strcmp( buf, "alias" ) )
	  {
	     if ( load_script_function( script, 1 ) )
	       {
		  aborted = 1;
		  break;
	       }
	  }
	
	else if ( !strcmp( buf, "trigger" ) )
	  {
	     if ( load_script_trigger( script ) )
	       {
		  aborted = 1;
		  break;
	       }
	  }
	
	else if ( !strcmp( buf, "script" ) )
	  {
	     get_text( buf, 4096 );
	     
	     if ( !pos )
	       {
		  aborted = 1;
		  break;
	       }
	     
	     if ( *pos != ';' )
	       {
		  abort_script( 1, "Expected ';' instead." );
		  break;
	       }
	     pos++;
	     
	     if ( script->description )
	       free( script->description );
	     
	     script->description = strdup( buf );
	  }
	else
	  {
	     abort_script( 1, NULL );
	     aborted = 1;
	     break;
	  }
     }
   
   pos = old_pos;
   beginning = old_beginning;
   free( body );
   
   if ( aborted )
     {
	destroy_script( script );
	current_script = previous_script;
	return NULL;
     }
   
   if ( !script->description )
     script->description = strdup( "No Description" );
   
   for ( i = 0; i < script->codes_nr; i++ )
     if ( link_symbols( &script->codes[i], script->codes[i].first_instruction ) )
       {
	  destroy_script( script );
	  current_script = previous_script;
	  return NULL;
       }
   
   current_script = previous_script;
   
   /* Initialize its memory. */
   script->script_memory = calloc( script->symbol_table.memory_size, sizeof( VARIABLE ) );
   script->script_memory_top = script->script_memory + script->symbol_table.memory_size;
   
   /* Resize global memory. */
   resize_global_memory( symbol_table.memory_size );
   
   
   /* Everything went okay, go ahead and replace/link it! */
   
   /* Unload the script if it's already loaded, first. */
   for ( s = scripts; s; s = s->next )
     if ( !strcmp( s->name, script->name ) )
       {
	  if ( s->in_use )
	    {
	       destroy_script( script );
	       return NULL;
	    }
	  
	  destroy_script( s );
	  break;
       }
   
   /* Link the script. */
   script->next = scripts;
   scripts = script;
   
   return script;
}



void show_instr( INSTR *instr, int space )
{
   int i;
   
   clientff( C_m " %*s->", space, "" );
   
   switch ( instr->instruction )
     {
      case INSTR_FUNCTION: clientff( C_r "function %s", instr->sys_function ? sys_functions[instr->pointer.offset].name : "" ); break;
      case INSTR_DOALL: clientff( C_r "do all" ); break;
      case INSTR_OPERATOR: clientff( C_r "operator (%s)", operators[instr->oper].symbol ); break;
      case INSTR_CONSTANT:
	clientff( C_r "const " );
	if ( instr->constant.type == VAR_STRING )
	  clientff( "(\"%s\")", instr->constant.string );
	if ( instr->constant.type == VAR_NUMBER )
	  clientff( "(%d)", instr->constant.nr );
	if ( instr->constant.type == VAR_POINTER )
	  clientff( "(p%s, %d)",
		    instr->constant.pointer.memory_space == GLOBAL_MEMORY ? "-global" :
		    instr->constant.pointer.memory_space == RELATIVE_LOCAL_MEMORY ? "-local" : "",
		    instr->constant.pointer.offset );
	break;
      case INSTR_VARIABLE:
	clientff( C_r "variable (%s)", instr->var_name );
	if ( instr->pointer.memory_space )
	  clientff( C_r " (linked - %s (%d))",
		    instr->pointer.memory_space == RELATIVE_LOCAL_MEMORY ? "defined as local" :
		    instr->pointer.memory_space == GLOBAL_MEMORY ? "defined as global" :
		    instr->pointer.memory_space == SCRIPT_MEMORY ? "defined as script-wide" : C_B "Huh?!" C_r,
		    instr->pointer.offset );
	else if ( instr->sys_function )
	  clientff( C_r " (sys function)" );
	else
	  clientff( C_r " (not linked)" );
	break;
      case INSTR_IF: clientff( C_r "if" ); break;
      case INSTR_RETURN: clientff( C_r "return" ); break;
      default: clientff( C_R "unknown" ); break;
     }
   
   clientff( C_0 "\n" );
   
   for ( i = 0; i < instr->links; i++ )
     show_instr( instr->link[i], space+1 );
}



void do_show( int full )
{
   SCRIPT *s;
   FUNCTION *f;
   int i;
   
   for ( i = 0; i < symbol_table.nr; i++ )
     clientff( C_Y "GSym: %s\n" C_0, symbol_table.symbol[i].name );
   
   for ( s = scripts; s; s = s->next )
     {
	clientff( C_Y "\n\nSCRIPT: " C_B "%s\n" C_0, s->name );
	
	for ( i = 0; i < s->symbol_table.nr; i++ )
	  clientff( C_Y "SSym: %s\n" C_0, s->symbol_table.symbol[i].name );
	
	for ( f = s->functions; f; f = f->next )
	  {
	     clientff( C_B "\n%s - %s.\n", f->alias ? "Alias" : "Function", f->name );
	     
	     for ( i = 0; i < s->codes[f->code_offset].symbol_table.nr; i++ )
	       clientff( C_Y "LSym: %s\n" C_0, s->codes[f->code_offset].symbol_table.symbol[i].name );
	     
	     if ( full )
	       show_instr( s->codes[f->code_offset].first_instruction, 0 );
	  }
     }
}



void load_scripts( )
{
   FUNCTION *f;
   SCRIPT *s;
   
   s = load_script( "init.is" );
   
   if ( !s )
     return;
   
   /* Find the 'main' function and run it. */
   for ( f = s->functions; f; f = f->next )
     {
	if ( !strcmp( f->name, "init" ) )
	  {
	     current_line = "";
	     set_args( "" );
	     run_code( s, f->code_offset );
	     break;
	  }
     }
}



/*** Linker. ***/

void abort_linker( INSTR *instr, char *error, ... )
{
   char error_buf[1024];
   
   va_list args;
   error_buf[0] = 0;
   if ( error )
     {
	va_start( args, error );
	vsnprintf( error_buf, 1024, error, args );
	va_end( args );
     }
   
   clientff( C_0 "(" C_W "iScript" C_0 "): " C_R "Linker error: %s\r\n" C_0, error_buf );
}



int link_symbols( CODE *code, INSTR *instr )
{
   int i;
   
   if ( instr->instruction == INSTR_VARIABLE )
     {
	if ( instr->defined_as_local )
	  {
	     i = add_local_variable( code, instr->var_name, NULL );
	     
	     if ( i < 0 )
	       return 1;
	     
	     if ( code->symbol_table.symbol[i].is_constant )
	       {
		  copy_value( &code->symbol_table.symbol[i].constant, &instr->constant, 0 );
		  instr->instruction = INSTR_CONSTANT;
	       }
	     else
	       instr->pointer = code->symbol_table.symbol[i].pointer;
	  }
	else if ( instr->defined_as_global )
	  {
	     i = add_global_variable( instr->var_name, NULL );
	     
	     if ( i < 0 )
	       return 1;
	     
	     if ( symbol_table.symbol[i].is_constant )
	       {
		  copy_value( &symbol_table.symbol[i].constant, &instr->constant, 0 );
		  instr->instruction = INSTR_CONSTANT;
	       }
	     else
	       instr->pointer = symbol_table.symbol[i].pointer;
	  }
	else
	  {
	     i = add_script_variable( current_script, instr->var_name, NULL );
	     
	     if ( i < 0 )
	       return 1;
	     
	     if ( current_script->symbol_table.symbol[i].is_constant )
	       {
		  copy_value( &current_script->symbol_table.symbol[i].constant, &instr->constant, 0 );
		  instr->instruction = INSTR_CONSTANT;
	       }
	     else
	       instr->pointer = current_script->symbol_table.symbol[i].pointer;
	  }
     }
   
   for ( i = 0; i < instr->links; i++ )
     if ( link_symbols( code, instr->link[i] ) )
       return 1;
   
   return 0;
}



/*** Optimizer. ***/




/*** Runner. These -must- be optimized for speed! ***/


#define CLEAR_VALUE( var ) (var).type = VAR_NUMBER, (var).nr = 0, (var).address.memory_space = NULL_POINTER;
#define FREE_VALUE( var ) if ( (var).type == VAR_STRING && (var).string ) free( (var).string );


void shrink_local_memory( )
{
   int b_size, t_size;
   int i;
   
   if ( top_of_allocated_memory_for_locals - local_memory <= 32 )
     return;
   
   for ( i = 32; i < top_of_allocated_memory_for_locals - local_memory; i++ )
     FREE_VALUE( *(local_memory + i) );
   
   b_size = current_local_bottom - local_memory;
   t_size = current_local_top - local_memory;
   local_memory = realloc( local_memory, 32 * sizeof( VARIABLE ) );
   current_local_bottom = local_memory + b_size;
   current_local_top = local_memory + t_size;
   top_of_allocated_memory_for_locals = local_memory + 32;
}



void end_code_call( struct prev_data *prev )
{
   current_local_bottom = local_memory + prev->bottom;
   current_local_top = local_memory + prev->top;
   
   current_script->in_use = prev->script_in_use;
   current_script = prev->script;
   
   function_return_value = prev->function_return_value;
}



int prepare_code_call( int size, SCRIPT *script, VARIABLE *returns, struct prev_data *prev )
{
   prev->function_return_value = function_return_value;
   function_return_value = returns;
   
   prev->script = current_script;
   current_script = script;
   
   prev->script_in_use = script->in_use;
   script->in_use = 1;
   
   /* Local memory. */
   prev->bottom = current_local_bottom - local_memory;
   prev->top = current_local_top - local_memory;
   current_local_bottom = current_local_top;
   current_local_top = current_local_bottom + size;
   
   /* Resize if needed. */
   // Murh... I dun like this. Any other way?
   while ( current_local_top > top_of_allocated_memory_for_locals )
     {
	int b_size, t_size, l_size;
	b_size = current_local_bottom - local_memory;
	t_size = current_local_top - local_memory;
	l_size = ( top_of_allocated_memory_for_locals - local_memory );
	
	local_memory = realloc( local_memory, ( l_size + 32 ) * sizeof( VARIABLE ) );
	current_local_bottom = local_memory + b_size;
	current_local_top = local_memory + t_size;
	memset( local_memory + l_size, 0, 32 * sizeof( VARIABLE ) );
	top_of_allocated_memory_for_locals = local_memory + l_size + 32;
	
	if ( l_size >= 8192 )
	  {
	     clientff( C_R "*** Stack overflow! Can't extend memory to %d elements ***\r\n" C_0,
		       l_size );
	     end_code_call( prev );
	     return 1;
	  }
     }
   
   return 0;
}



int run_function_with_args( FUNCTION *function, char *args )
{
   CODE *code;
   VARIABLE *v, returned_value, function_value;
   VARIABLE *locals = NULL;
   struct prev_data prev;
   char buf[2048];
   int i;
   
   struct timeval call_start, call_prepared, call_end;
   int sec1, usec1, sec2, usec2, sec3, usec3;
   
   gettimeofday( &call_start, NULL );
   
   CLEAR_VALUE( returned_value );
   CLEAR_VALUE( function_value );
   
   code = &function->parent->codes[function->code_offset];
   
   if ( function->args_nr )
     {
	locals = calloc( function->args_nr, sizeof( VARIABLE ) );
	
	/* Fill function arguments. */
	for ( i = 0; i < function->args_nr; i++ )
	  {
	     args = get_string( args, buf, 2048 );
	     if ( !buf[0] )
	       break;
	     
	     v = locals + i;
	     v->type = VAR_STRING;
	     v->string = strdup( buf );
	  }
     }
   
   /* Prepare memory. */
   if ( !prepare_code_call( code->symbol_table.memory_size, function->parent, &function_value, &prev ) )
     {
	/* Copy locals. */
	// Fix meeee! I'm a huge performance problem!
	if ( locals )
	  memcpy( current_local_bottom, locals, function->args_nr * sizeof( VARIABLE ) );
	
	/* Prepared. Run! */
	
	gettimeofday( &call_prepared, NULL );
	
	i = instructions[code->first_instruction->instruction]( code->first_instruction, &returned_value );
	i = i && !returning_successful;
	returning_successful = 0;
	
	FREE_VALUE( returned_value );
	FREE_VALUE( function_value );
	
	if ( i )
	  clientf( "Failed!\r\n" );
	else
	  {
	     clientf( "Returned value: " );
	     if ( function_value.type == VAR_STRING )
	       clientff( "%s\r\n", function_value.string );
	     if ( function_value.type == VAR_NUMBER )
	       clientff( "%d\r\n", function_value.nr );
	     if ( function_value.type == VAR_POINTER )
	       clientff( "@%d-%d\r\n", function_value.pointer.memory_space,
			 function_value.pointer.offset );
	     
	  }
	
	gettimeofday( &call_end, NULL );
   
	sec1 = call_end.tv_sec - call_prepared.tv_sec;
	usec1 = call_end.tv_usec - call_prepared.tv_usec;
	if ( usec1 < 0 )
	  sec1 -= 1, usec1 += 1000000;
	
	sec2 = call_prepared.tv_sec - call_start.tv_sec;
	usec2 = call_prepared.tv_usec - call_start.tv_usec;
	if ( usec2 < 0 )
	  sec2 -= 1, usec2 += 1000000;
	
	sec3 = sec1 + sec2;
	usec3 = usec1 + usec2;
	if ( usec3 >= 1000000 )
	  sec3 += 1, usec3 -= 1000000;
	
	clientff( C_r "Execution time: " C_W "%d.%06d" C_r " seconds (preparing: " C_W "%d.%06d" C_r ", running: " C_W "%d.%06d" C_r ").\r\n" C_0,
		  sec3, usec3, sec2, usec2, sec1, usec1 );
	
	/* Release local memory. Shrink it too, if needed. */
	end_code_call( &prev );
	shrink_local_memory( );
     }
   else
     i = 1;
   
   if ( locals )
     free( locals );
   
   return i;
}



int run_code( SCRIPT *script, int code_offset )
{
   VARIABLE returned_value, function_value;
   CODE *code;
   struct prev_data prev;
   int i;
   
   CLEAR_VALUE( returned_value );
   CLEAR_VALUE( function_value );
   code = &script->codes[code_offset];
   
   /* Prepare memory. */
   if ( !prepare_code_call( code->symbol_table.memory_size, script, &function_value, &prev ) ) // &prev_bottom, &prev_top ) )
     {
	/* Prepared. Run! */
	
	i = instructions[code->first_instruction->instruction]( code->first_instruction, &returned_value );
	i = i && !returning_successful;
	returning_successful = 0;
	
	FREE_VALUE( returned_value );
	FREE_VALUE( function_value );
	
	/* Release local memory. Shrink it too, if needed. */
	end_code_call( &prev );
	shrink_local_memory( );
     }
   else
     i = 1;
   
   return i;
}



int instr_function( INSTR *instr, VARIABLE *returned_value )
{
   CODE *code;
   VARIABLE *v, pointer, *locals = NULL, *top;
   struct prev_data prev;
   int i;
   
   CLEAR_VALUE( pointer );
   
//   clientff( "instr::%sfunction\r\n", instr->sys_function ? "(sys)" : "" );
   
   if ( instr->sys_function )
     return sys_functions[instr->pointer.offset].function( instr, returned_value );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &pointer ) )
     {
	FREE_VALUE( pointer );
	return 1;
     }
   
   if ( pointer.type != VAR_POINTER ||
	pointer.pointer.memory_space != SCRIPT_FUNCTIONS ||
	pointer.pointer.offset < 0 ||
	pointer.pointer.offset >= current_script->codes_nr )
     {
	clientf( C_R "*** Calling function from invalid pointer! ***\r\n" C_0 );
	FREE_VALUE( pointer );
	return 1;
     }
   
   code = &current_script->codes[pointer.pointer.offset];
   
   FREE_VALUE( pointer );
   CLEAR_VALUE( pointer );
   
   if ( code->symbol_table.memory_size )
     {
	locals = calloc( code->symbol_table.memory_size, sizeof( VARIABLE ) );
	top = locals + code->symbol_table.memory_size;
	
	// Be careful when returning, it may have strings in it!
	
	/* Fill function's arguments. */
	for ( i = 1; i < instr->links; i++ )
	  {
	     v = locals + i - 1;
	     
	     if ( v >= top )
	       {
		  clientf( C_R "*** Number of arguments passed exceeds function's local memory size! ***\r\n" C_0 );
		  free( locals );
		  return 1;
	       }
	     
	     FREE_VALUE( *v );
	     CLEAR_VALUE( *v );
	     
	     if ( instructions[instr->link[i]->instruction]( instr->link[i], v ) )
	       {
		  free( locals );
		  return 1;
	       }
	  }
     }
   
   /* Prepare memory. */
   if ( !prepare_code_call( code->symbol_table.memory_size, current_script, returned_value, &prev ) )
     {
	/* Copy locals. */
	// Fix meeee! I'm a huge performance problem!
	if ( locals )
	  memcpy( current_local_bottom, locals, ( top - locals ) * sizeof( VARIABLE ) );
	
	/* Prepared. Run! */
	
	i = instructions[code->first_instruction->instruction]( code->first_instruction, &pointer );
	i = i && !returning_successful;
	returning_successful = 0;
	
	FREE_VALUE( pointer );
	
	/* Release local memory. */
	end_code_call( &prev );
	
     }
   else
     i = 1;
   
   if ( locals )
     free( locals );
   
   if ( returning_successful )
     {
	returning_successful = 0;
	return 0;
     }
   
   return i;
}


int instr_doall( INSTR *instr, VARIABLE *returned_value )
{
   VARIABLE devnull;
   int i = 0, nr;
   
   CLEAR_VALUE( devnull );
   
//   clientf( "instr::doall\r\n" );
   
   nr = instr->links;
   
   while ( i < nr )
     if ( instructions[instr->link[i]->instruction]( instr->link[i++], &devnull ) )
       {
	  FREE_VALUE( devnull );
	  return 1;
       }
   
   FREE_VALUE( devnull );
   return 0;
}


int instr_operator( INSTR *instr, VARIABLE *returned_value )
{
   VARIABLE lvalue, rvalue;
   short i;
   
   CLEAR_VALUE( lvalue );
   
//   clientf( "instr::operator\r\n" );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &lvalue ) )
     {
	FREE_VALUE( lvalue );
	return 1;
     }
   
   /* Check for compatibility on lvalue. Irrelevant on some assignments. */
   if ( lvalue.type &&
	!( operators[instr->oper].types & lvalue.type ) &&
	!operators[instr->oper].assignment == 2 )
     {
	clientff( C_R "*** Invalid lvalue (%s) passed to operator '%s'! ***\r\n" C_0,
		  lvalue.type == VAR_STRING ? "string" :
		  lvalue.type == VAR_POINTER ? "pointer" :
		  lvalue.type == VAR_NUMBER ? "number" : "unknown",
		  operators[instr->oper].symbol );
	FREE_VALUE( lvalue );
	return 1;
     }
   
   /* Unary operators. */
   if ( operators[instr->oper].unary )
     {
	i = operators[instr->oper].function( &lvalue, NULL, returned_value );
	FREE_VALUE( lvalue );
	return i;
     }
   
   CLEAR_VALUE( rvalue );
   
   /* Binary operators. */
   if ( instructions[instr->link[1]->instruction]( instr->link[1], &rvalue ) )
     {
	FREE_VALUE( lvalue );
	FREE_VALUE( rvalue );
	return 1;
     }
   
   /* Check for compatibility on rvalue. */
   if ( rvalue.type && !( operators[instr->oper].types & rvalue.type ) )
     {
	clientff( C_R "*** Invalid rvalue (%s) passed to operator '%s'! ***\r\n" C_0,
		  rvalue.type == VAR_STRING ? "string" :
		  rvalue.type == VAR_POINTER ? "pointer" :
		  rvalue.type == VAR_NUMBER ? "number" : "unknown",
		  operators[instr->oper].symbol );
	FREE_VALUE( lvalue );
	FREE_VALUE( rvalue );
	return 1;
     }
   
   i = operators[instr->oper].function( &lvalue, &rvalue, returned_value );
   FREE_VALUE( lvalue );
   FREE_VALUE( rvalue );
   return i;
}


int instr_constant( INSTR *instr, VARIABLE *returned_value )
{
//   clientf( "instr::constant\r\n" );
   
   copy_value( &instr->constant, returned_value, 0 );
   
   returned_value->address.memory_space = NULL_POINTER;
   
   return 0;
}


VARIABLE *dereference_pointer( POINTER pointer )
{
   VARIABLE *v;
   
   if ( pointer.memory_space == SCRIPT_MEMORY )
     {
	v = current_script->script_memory + pointer.offset;
	if ( v >= current_script->script_memory && v < current_script->script_memory_top )
	  return v;
	
	clientff( C_R "*** Segmentation Fault! ***\r\n" C_0 );
	clientff( C_R "*** Accessing script-wide memory out of bounds ***\r\n" C_0 );
	return NULL;
     }
   else if ( pointer.memory_space == RELATIVE_LOCAL_MEMORY )
     {
	v = current_local_bottom + pointer.offset;
	if ( v >= local_memory && v < current_local_top )
	  return v;
	
	clientff( C_R "*** Segmentation Fault! ***\r\n" C_0 );
	clientff( C_R "*** Accessing relative local memory out of bounds ***\r\n" C_0 );
	return NULL;
     }
   else if ( pointer.memory_space == GLOBAL_MEMORY )
     {
	v = global_memory + pointer.offset;
	if ( v >= global_memory && v < global_memory + symbol_table.memory_size )
	  return v;
	
	clientff( C_R "*** Segmentation Fault! ***\r\n" C_0 );
	clientff( C_R "*** Accessing global memory out of bounds ***\r\n" C_0 );
	return NULL;
     }
   else if ( pointer.memory_space == ABSOLUTE_LOCAL_MEMORY )
     {
	v = local_memory + pointer.offset;
	if ( v >= local_memory && v < current_local_top )
	  return v;
	
	clientff( C_R "*** Segmentation Fault! ***\r\n" C_0 );
	clientff( C_R "*** Accessing absolute local memory out of bounds ***\r\n" C_0 );
	return NULL;
     }
   else if ( pointer.memory_space == NULL_POINTER )
     {
	clientff( C_R "*** Segmentation Fault! ***\r\n" C_0 );
	clientff( C_R "*** Dereferencing a null pointer ***\r\n" C_0 );
	return NULL;
     }
   
   clientff( C_R "*** Internal error: unknown pointer type (%d) ***\r\n" C_0, pointer.memory_space );
   return NULL;
}


int instr_variable( INSTR *instr, VARIABLE *returned_value )
{
   VARIABLE *v;
   
//   clientf( "instr::variable\r\n" );
   
   /* Copy its value. */
   
   v = dereference_pointer( instr->pointer );
   
   if ( !v )
     return 1;
   
   copy_value( v, returned_value, 0 );
   
   /* And also its address. */
   
   copy_address( instr->pointer, returned_value );
   
   return 0;
}


int instr_return( INSTR *instr, VARIABLE *returned_value )
{
//   clientf( "instr::return\r\n" );
   
   if ( instr->links )
     instructions[instr->link[0]->instruction]( instr->link[0], function_return_value );
   
   returning_successful = 1;
   return 1;
}


int instr_if( INSTR *instr, VARIABLE *returned_value )
{
   VARIABLE var;
   short i;
   
   CLEAR_VALUE( var );
   
//   clientf( "instr::if\r\n" );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &var ) )
     {
	FREE_VALUE( var );
	return 1;
     }
   
   /* If true... */
   if ( ( var.type == VAR_STRING && var.string[0] ) ||
	( var.type == VAR_NUMBER && var.nr ) ||
	( var.type == VAR_POINTER && var.pointer.memory_space != NULL_POINTER ) )
     {
	i = instructions[instr->link[1]->instruction]( instr->link[1], &var );
     }
   /* Else, false. */
   else if ( instr->links == 3 )
     {
	i = instructions[instr->link[2]->instruction]( instr->link[2], &var );
     }
   else
     i = 0;
   
   FREE_VALUE( var );
   
   return i;
}



/** System functions. **/

#define SYSFUNC( sysfunc ) int (sysfunc)( INSTR *instr, VARIABLE *returns )

SYSFUNC( sysfunc_not_implemented )
{
   clientff( C_R "*** System function not implemented! ***\r\n" C_0 );
   
   debugf( "Running a function that is not yet implemented." );
   
   return 1;
}


SYSFUNC( sysfunc_echo )
{
   VARIABLE value;
   
   CLEAR_VALUE( value );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &value ) )
     return 1;
   
   if ( value.type != VAR_STRING )
     {
	clientff( C_R "*** sysfunc_echo: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   clientf( value.string );
   clientf( "\r\n" );
   FREE_VALUE( value );
   return 0;
}


SYSFUNC( sysfunc_send )
{
   VARIABLE value;
   char *buf, *p;
   
   CLEAR_VALUE( value );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &value ) )
     return 1;
   
   if ( value.type != VAR_STRING )
     {
	clientff( C_R "*** sysfunc_send: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   /* Translate all semicolons into new lines. */
   buf = strdup( value.string );
   for ( p = buf; *p; p++ )
     if ( *p == ';' )
       *p = '\n';
   
   send_to_server( buf );
   send_to_server( "\r\n" );
   free( buf );
   FREE_VALUE( value );
   return 0;
}


SYSFUNC( sysfunc_necho )
{
   VARIABLE value;
   
   CLEAR_VALUE( value );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &value ) )
     return 1;
   
   if ( value.type != VAR_STRING )
     {
	clientff( C_R "*** sysfunc_necho: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   clientf( value.string );
   FREE_VALUE( value );
   return 0;
}


SYSFUNC( sysfunc_nsend )
{
   VARIABLE value;
   char *buf, *p;
   
   CLEAR_VALUE( value );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &value ) )
     return 1;
   
   if ( value.type != VAR_STRING )
     {
	clientff( C_R "*** sysfunc_nsend: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   /* Translate all semicolons into new lines. */
   buf = strdup( value.string );
   for ( p = buf; *p; p++ )
     if ( *p == ';' )
       *p = '\n';
   
   send_to_server( buf );
   free( buf );
   FREE_VALUE( value );
   return 0;
}


SYSFUNC( sysfunc_arg )
{
   VARIABLE value;
   char *p;
   int nr, size;
   
   CLEAR_VALUE( value );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &value ) )
     return 1;
   
   if ( value.type == VAR_NUMBER )
     {
	nr = value.nr;
     }
   else if ( value.type == VAR_STRING )
     {
	nr = value.string[0] - '0';
     }
   else
     {
	clientff( C_R "*** sysfunc_arg: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   FREE_VALUE( value );
   
   returns->type = VAR_STRING;
   
   if ( nr < 0 )
     p = "", size = 0;
   
   else if ( regex_callbacks )
     {
	/* RegEx buffer callbacks. */
	
	if ( nr >= regex_callbacks )
	  p = "", size = 0;
	else
	  {
	     size = regex_ovector[nr*2+1] - regex_ovector[nr*2];
	     p = current_line + regex_ovector[nr*2];
	  }
     }
   
   else
     {
	/* Alias arguments. */
	
	if ( nr >= 16 )
	  p = "", size = 0;
	else
	  {
	     size = alias_args_end[nr] - alias_args_start[nr];
	     p = current_line + alias_args_start[nr];
	  }
     }
   
   returns->string = malloc( size + 1 );
   memcpy( returns->string, p, size );
   returns->string[size] = 0;
   
   return 0;
}


SYSFUNC( sysfunc_args )
{
   VARIABLE value;
   char *p;
   int nr1, nr2, start_offset, end_offset, size;
   
   CLEAR_VALUE( value );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &value ) )
     return 1;
   
   nr1 = value.nr;
   FREE_VALUE( value );
   
   if ( value.type != VAR_NUMBER )
     {
	clientff( C_R "*** sysfunc_args: first argument is invalid ***\r\n" C_0 );
	return 1;
     }
   
   if ( instructions[instr->link[1]->instruction]( instr->link[1], &value ) )
     return 1;
   
   nr2 = value.nr;
   FREE_VALUE( value );
   
   if ( value.type != VAR_NUMBER )
     {
	clientff( C_R "*** sysfunc_args: second argument is invalid ***\r\n" C_0 );
	return 1;
     }
   
   returns->type = VAR_STRING;
   
   if ( regex_callbacks )
     {
	clientff( C_R "*** sysfunc_args: called from a regex trigger ***\r\n" C_0 );
	return 1;
     }
   
   /* Make sure nr2 is a sane value. */
   if ( nr2 < 0 )
     nr2 = 0;
   else if ( nr2 > 15 )
     nr2 = 15;
   
   if ( nr1 < 0 || nr1 > 15 || ( nr1 && !alias_args_start[nr1] ) )
     p = "", size = 0;
   
   else
     {
	start_offset = alias_args_start[nr1];
	end_offset = nr2 ? alias_args_end[nr2] : 0;
	
	if ( end_offset <= start_offset )
	  for ( p = current_line + start_offset, end_offset = start_offset; *p; p++ )
	    end_offset++;
	
	p = current_line + start_offset;
	size = end_offset - start_offset;
     }
   
   returns->string = malloc( size + 1 );
   memcpy( returns->string, p, size );
   returns->string[size] = 0;
   
   return 0;
}


SYSFUNC( sysfunc_show_prompt )
{
   show_prompt( );
   
   return 0;
}


SYSFUNC( sysfunc_load_script )
{
   VARIABLE name;
   
   CLEAR_VALUE( name );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &name ) )
     return 1;
   
   if ( name.type != VAR_STRING )
     {
	clientff( C_R "*** sysfunc_load_script: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   returns->type = VAR_NUMBER;
   returns->nr = !( load_script( name.string ) == NULL );
   FREE_VALUE( name );
   
   return 0;
}



SYSFUNC( sysfunc_unload_script )
{
   VARIABLE name;
   SCRIPT *s;
   
   CLEAR_VALUE( name );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &name ) )
     return 1;
   
   if ( name.type != VAR_STRING )
     {
	clientff( C_R "*** sysfunc_unload_script: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   returns->type = VAR_NUMBER;
   
   for ( s = scripts; s; s = s->next )
     {
	if ( !strcmp( s->name, name.string ) && !s->in_use )
	  {
	     destroy_script( s );
	     returns->nr = 0;
	     FREE_VALUE( name );
	     return 0;
	  }
     }
   
   returns->nr = 1;
   FREE_VALUE( name );
   return 0;
}



SYSFUNC( sysfunc_call )
{
   VARIABLE func_name, script_name;
   SCRIPT *s;
   FUNCTION *f;
   int found = 0;
   
   CLEAR_VALUE( func_name );
   CLEAR_VALUE( script_name );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &func_name ) )
     return 1;
   
   if ( func_name.type != VAR_STRING )
     {
	clientff( C_R "*** sysfunc_call: invalid function argument ***\r\n" C_0 );
	return 1;
     }
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &script_name ) )
     return 1;
   
   if ( script_name.type != VAR_STRING )
     {
	clientff( C_R "*** sysfunc_call: invalid script argument ***\r\n" C_0 );
	FREE_VALUE( func_name );
	return 1;
     }
   
   for ( s = scripts; s; s = s->next )
     {
	if ( !strcmp( s->name, script_name.string ) )
	  {
	     for ( f = s->functions; f; f = f->next )
	       {
		  if ( !strcmp( f->name, func_name.string ) )
		    {
		       run_code( f->parent, f->code_offset );
		       
		       found = 1;
		       break;
		    }
	       }
	     break;
	  }
     }
   
   returns->type = VAR_NUMBER;
   returns->nr = !found;
   
   return 0;
}



inline void debug_show_value( VARIABLE *v, int i, char *c )
{
   const char mem_space[ ] = { 'n', 'r', 'a', 's', 'g', 'f' };
   
   clientff( "%s%2d:" C_0 " " C_D, c, i );
   if ( v->type == VAR_NUMBER )
     clientff( "(n)" C_0 " %d\r\n", v->nr );
   else if ( v->type == VAR_STRING )
     clientff( "(s)" C_0 " \"%s\" (%d)\r\n", v->string, strlen( v->string ) );
   else if ( v->type == VAR_POINTER )
     clientff( "(p)" C_0 " @%c%d\r\n",
	       mem_space[v->pointer.memory_space],
	       v->pointer.offset );
   else
     clientff( "(uninitialized)\r\n" C_0 );
}

SYSFUNC( sysfunc_debug )
{
   VARIABLE *v;
   int i;
   
   clientff( C_Y "\r\nGlobal Memory (%d variables)\r\n" C_0,
	     symbol_table.memory_size );
   
   for ( i = 0; i < 64; i++ )
     {
	v = global_memory + i;
	if ( v >= global_memory + symbol_table.memory_size )
	  break;
	debug_show_value( v, i, C_Y );
     }
   
   clientff( C_Y "\r\nScript Memory (%d variables)\r\n" C_0,
	     current_script->script_memory_top - current_script->script_memory );
   
   for ( i = 0; i < 64; i++ )
     {
	v = current_script->script_memory + i;
	if ( v >= current_script->script_memory_top )
	  break;
	
	if ( v->type != VAR_NUMBER || v->nr != 0 )
	  debug_show_value( v, i, C_Y );
     }
   
   clientff( C_Y "\r\nLocal Memory (%d variables, %d total)\r\n" C_0,
	     current_local_top - current_local_bottom,
	     current_local_top - local_memory );
   
   for ( i = 0; i < 64; i++ )
     {
	v = local_memory + i;
	if ( v >= current_local_top )
	  break;
	
	if ( v >= current_local_bottom || v->type != VAR_NUMBER || v->nr != 0 )
	  debug_show_value( v, i, v >= current_local_bottom ? C_Y : C_D );
     }
   
   return 0;
}


SYSFUNC( sysfunc_nr )
{
   VARIABLE nr;
   
   CLEAR_VALUE( nr );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &nr ) )
     return 1;
   
   if ( nr.type == VAR_NUMBER )
     {
	returns->nr = nr.nr;
     }
   else if ( nr.type == VAR_STRING )
     {
	char *p;
	int i = 0, neg = 0;
	
	p = nr.string;
	
	while ( *p == ' ' )
	  p++;
	
	if ( *p == '-' )
	  neg = 1;
	
	while ( *p >= '0' && *p <= '9' )
	  i *= 10, i += *p - '0', p++;
	
	if ( neg )
	  i = -i;
	
	returns->nr = i;
     }
   else
     returns->nr = 0;
   
   FREE_VALUE( nr );
   
   returns->type = VAR_NUMBER;
   
   return 0;
}


SYSFUNC( sysfunc_str )
{
   VARIABLE str;
   
   CLEAR_VALUE( str );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &str ) )
     return 1;
   
   if ( str.type == VAR_STRING )
     {
	returns->string = strdup( str.string );
     }
   else if ( str.type == VAR_NUMBER )
     {
	// Optimize me!
	
	char buf[256];
	
	sprintf( buf, "%d", str.nr );
	
	returns->string = strdup( buf );
     }
   else
     returns->string = strdup( "(null)" );
   
   FREE_VALUE( str );
   
   returns->type = VAR_STRING;
   
   return 0;
}



/** Operators. **/

#define OPER( oper ) int (oper)( VARIABLE *lvalue, VARIABLE *rvalue, VARIABLE *returns )

OPER( oper_not_implemented )
{
   clientff( C_R "*** Operator not implemented! ***\r\n" C_0 );
   
//   debugf( "Executing an operator that is not yet implemented." );
   
   return 1;
}


OPER( oper_dereference )
{
   VARIABLE *v;
   
   if ( lvalue->type != VAR_POINTER )
     {
	clientf( C_R "*** Invalid operand passed to operator '*' ***\r\n" C_0 );
	return 1;
     }
   
   v = dereference_pointer( lvalue->pointer );
   
   if ( !v )
     return 1;
   
   copy_value( v, returns, 0 );
   copy_address( lvalue->pointer, returns );
   
   return 0;
}


OPER( oper_address_of )
{
   if ( lvalue->address.memory_space == NULL_POINTER )
     {
	clientf( C_R "*** Invalid operand passed to operator '&' ***\r\n" C_0 );
	return 1;
     }
   
   returns->type = VAR_POINTER;
   if ( lvalue->address.memory_space == RELATIVE_LOCAL_MEMORY )
     {
	returns->pointer.memory_space = ABSOLUTE_LOCAL_MEMORY;
	returns->pointer.offset = lvalue->address.offset + ( current_local_bottom - local_memory );
     }
   else
     {
	returns->pointer.memory_space = lvalue->address.memory_space;
	returns->pointer.offset = lvalue->address.offset;
     }
   
   return 0;
}


OPER( oper_assignment )
{
   VARIABLE *v;
   
   v = dereference_pointer( lvalue->address );
   
   if ( !v )
     return 1;
   
   copy_value( rvalue, v, 0 );
   
   copy_value( rvalue, returns, 0 );
   copy_address( lvalue->pointer, returns );
   
   return 0;
}


OPER( oper_modulus )
{
   returns->type = VAR_NUMBER;
   
   returns->nr = lvalue->nr % rvalue->nr;
   
   return 0;
}


OPER( oper_addition )
{
   if ( lvalue->type != rvalue->type )
     {
	/* Consider uninitialized variables as empty strings. */
	if ( lvalue->type == VAR_STRING && rvalue->type == VAR_NUMBER &&
	     rvalue->nr == 0 )
	  {
	     returns->type = VAR_STRING;
	     returns->string = strdup( lvalue->string );
	     return 0;
	  }
	else if ( rvalue->type == VAR_STRING && lvalue->type == VAR_NUMBER &&
		  lvalue->nr == 0 )
	  {
	     returns->type = VAR_STRING;
	     returns->string = strdup( rvalue->string );
	     return 0;
	  }
	
	clientf( C_R "*** Invalid operands passed to operator '+' ***\r\n" C_0 );
	return 1;
     }
   
   if ( lvalue->type == VAR_NUMBER )
     {
	returns->type = VAR_NUMBER;
	returns->nr = lvalue->nr + rvalue->nr;
	
	return 0;
     }
   
   if ( lvalue->type == VAR_STRING )
     {
	returns->type = VAR_STRING;
	
	// Not nice. Too many external functions called.
	returns->string = calloc( 1, strlen( lvalue->string ) + strlen( rvalue->string ) + 1 );
	strcpy( returns->string, lvalue->string );
	strcat( returns->string, rvalue->string );
	
	return 0;
     }
   
   clientf( C_R "*** Internal error: unknown variable type ***\r\n" C_0 );
   return 1;
}


OPER( oper_substraction )
{
   returns->type = VAR_NUMBER;
   
   returns->nr = lvalue->nr - rvalue->nr;
   
   return 0;
}


OPER( oper_equal_to )
{
   returns->type = VAR_NUMBER;
   
   if ( lvalue->type != rvalue->type )
     {
	returns->nr = 0;
     }
   else if ( lvalue->type == VAR_NUMBER )
     {
	returns->nr = ( lvalue->nr == rvalue->nr );
     }
   else if ( lvalue->type == VAR_POINTER )
     {
	// watch out for relative vs absolute
	returns->nr = ( ( lvalue->pointer.memory_space == rvalue->pointer.memory_space ) &&
		       ( lvalue->pointer.offset == rvalue->pointer.offset ) );
     }
   else if ( lvalue->type == VAR_STRING )
     {
	returns->nr = !strcmp( lvalue->string, rvalue->string );
     }
   
   return 0;
}


OPER( oper_not_equal_to )
{
   oper_equal_to( lvalue, rvalue, returns );
   
   returns->nr = !returns->nr;
   
   return 0;
}


OPER( oper_less_than )
{
   if ( lvalue->type != rvalue->type )
     {
	clientff( C_R "*** Invalid operands passed to operator '<' ***\r\n" C_0 );
	return 1;
     }
   
   returns->type = VAR_NUMBER;
   
   if ( lvalue->type == VAR_POINTER )
     {
	if ( lvalue->pointer.memory_space != rvalue->pointer.memory_space )
	  returns->nr = 0;
	else
	  returns->nr = lvalue->pointer.offset < rvalue->pointer.offset;
	return 0;
     }
   
   /* VAR_NUMBER. */
   returns->nr = lvalue->nr < rvalue->nr;
   return 0;
}


OPER( oper_greater_than )
{
   if ( lvalue->type != rvalue->type )
     {
	clientff( C_R "*** Invalid operands passed to operator '>' ***\r\n" C_0 );
	return 1;
     }
   
   returns->type = VAR_NUMBER;
   
   if ( lvalue->type == VAR_POINTER )
     {
	if ( lvalue->pointer.memory_space != rvalue->pointer.memory_space )
	  returns->nr = 0;
	else
	  returns->nr = lvalue->pointer.offset > rvalue->pointer.offset;
	return 0;
     }
   
   /* VAR_NUMBER. */
   returns->nr = lvalue->nr > rvalue->nr;
   return 0;
}


OPER( oper_less_than_or_equal_to )
{
   if ( lvalue->type != rvalue->type )
     {
	clientff( C_R "*** Invalid operands passed to operator '<=' ***\r\n" C_0 );
	return 1;
     }
   
   returns->type = VAR_NUMBER;
   
   if ( lvalue->type == VAR_POINTER )
     {
	if ( lvalue->pointer.memory_space != rvalue->pointer.memory_space )
	  returns->nr = 0;
	else
	  returns->nr = lvalue->pointer.offset <= rvalue->pointer.offset;
	return 0;
     }
   
   /* VAR_NUMBER. */
   returns->nr = lvalue->nr <= rvalue->nr;
   return 0;
}


OPER( oper_greater_than_or_equal_to )
{
   if ( lvalue->type != rvalue->type )
     {
	clientff( C_R "*** Invalid operands passed to operator '>=' ***\r\n" C_0 );
	return 1;
     }
   
   returns->type = VAR_NUMBER;
   
   if ( lvalue->type == VAR_POINTER )
     {
	if ( lvalue->pointer.memory_space != rvalue->pointer.memory_space )
	  returns->nr = 0;
	else
	  returns->nr = lvalue->pointer.offset >= rvalue->pointer.offset;
	return 0;
     }
   
   /* VAR_NUMBER. */
   returns->nr = lvalue->nr >= rvalue->nr;
   return 0;
}


OPER( oper_not )
{
   returns->type = lvalue->type;
   
   if ( lvalue->type == VAR_NUMBER )
     returns->nr = !lvalue->nr;
   else if ( lvalue->type == VAR_STRING )
     returns->string = strdup( lvalue->string[0] ? "" : "true" );
   
   return 0;
}


#define TRUTH_VALUE( variable, truth ) \
   if ( (variable)->type == VAR_NUMBER ) \
     truth = (variable)->nr != 0; \
   else if ( (variable)->type == VAR_STRING ) \
     truth = (variable)->string[0] != 0; \
   else if ( (variable)->type == VAR_POINTER ) \
     truth = (variable)->pointer.memory_space != NULL_POINTER; \
   else \
     truth = 0;


OPER( oper_logical_and )
{
   short ltruth, rtruth;
   
   TRUTH_VALUE( lvalue, ltruth );
   TRUTH_VALUE( rvalue, rtruth );
   
   returns->type = VAR_NUMBER;
   returns->nr = ltruth && rtruth;
   
   return 0;
}


OPER( oper_logical_or )
{
   short ltruth, rtruth;
   
   TRUTH_VALUE( lvalue, ltruth );
   TRUTH_VALUE( rvalue, rtruth );
   
   returns->type = VAR_NUMBER;
   returns->nr = ltruth || rtruth;
   
   return 0;
}


