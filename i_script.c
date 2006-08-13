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
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>


#include "module.h"


int scripter_version_major = 0;
int scripter_version_minor = 7;

char *i_script_id = I_SCRIPT_ID "\r\n" HEADER_ID "\r\n" MODULE_ID "\r\n";


/* 
 * Roadmap:
 *   0.8:
 *     - dynamic memory zones
 *     - functions to create/free/resize them
 *     - a[] and a.elem
 *     - include
 *   0.9:
 *     - timers
 *     - rnd, gag, prefix, sf_warning, mxp, utility functions
 *     - immortalize globals
 *   1.0:
 *     - ifassign/noeffect warnings, loop checks
 */

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
#define DYNAMIC_MEMORY		5
#define SCRIPT_FUNCTIONS	6

#define V_TYPE( val ) ( (val).type )
#define V_STR( val ) ( (val).u.string )
#define V_NR( val ) ( (val).u.nr )
#define V_POINTER( val ) ( (val).u.pointer )
#define V_TRUE( val ) ( ( V_TYPE( (val) ) == VAR_STRING && V_STR( (val) )[0] ) || \
                        ( V_TYPE( (val) ) == VAR_NUMBER && V_NR( (val) ) ) || \
                        ( V_TYPE( (val) ) == VAR_POINTER && V_POINTER( (val) ).memory_space != NULL_POINTER ) )

#define INIT_VALUE( val ) V_TYPE(val) = VAR_NUMBER, V_NR(val) = 0
#define FREE_VALUE( val ) if ( V_TYPE(val) == VAR_STRING && V_STR(val) ) free( V_STR(val) );

#define INIT_VARIABLE( var ) INIT_VALUE( (var).value ), (var).address.memory_space = NULL_POINTER
#define FREE_VARIABLE( var ) FREE_VALUE( (var).value )


/* Layout:
 * 
 * There may be several iScript groups.
 *  - Each group has its own memory zone, and some scripts in it.
 *    - Each script has multiple triggers, aliases, and functions.
 *      - Each trigger has a code that gets executed.
 *      - Each function also has a code that gets executed.
 *      - And each alias, the same.
 *        - Finally, each code has many instructions that compose it.
 */

/* Structures that we'll need. */

typedef struct list_data LIST;
typedef struct pointer_data POINTER;
typedef struct value_data VALUE;
typedef struct variable_data VARIABLE;
typedef struct symbol_data SYMBOL;
typedef struct symbol_table_data SYMBOL_TABLE;
typedef struct instr_data INSTR;
typedef struct code_data CODE;
typedef struct memory_data MEMORY;
typedef struct function_data FUNCTION;
typedef struct trigger_data TRIGGER;
typedef struct script_data SCRIPT;
typedef struct script_group_data SCRIPT_GROUP;



#define LINKED_LIST( type ) type *next, *prev


/* Currently, 16 bytes. A nice number. */
struct pointer_data
{
   int offset;
   
   unsigned char memory_space;
   
   void *memory_section;
} null_pointer = { 0, 0 };



struct value_data
{
   char type;
   union
     {
        // fixme.. consider long? We have enough space in the union..
        int nr;
        char *string;
        POINTER pointer;
     } u;
};



struct variable_data
{
   /* The most obvious part of a variable. */
   VALUE value;
   
   /* Its place in memory. */
   POINTER address;
};



struct symbol_data
{
   LINKED_LIST( SYMBOL );
   
   char *name;
   
   /* Line where it was declared. */
   int declared_at;
   
   /* Variable - where will it point to? */
   POINTER pointer;
   
   /* Constant. */
   int is_constant;
   VALUE constant;
   
   /* In the scope stack, it's a pointer to the symbol in the symbol table. */
   SYMBOL *symbol;
};



// FIXME: Destroy too.
struct symbol_table_data
{
   SYMBOL *symbol;
   SYMBOL *last;
   
   int memory_size;
   
   struct symbol_table_data *lower;
};



struct instr_data
{
   int instruction;
   
   /* Constant - its value. */
   VALUE constant;
   
   /* Variable - pointer to its memory space. */
   POINTER pointer;
   
   /* Operator. */
   int oper;
   
   /* Used when compiling. */
   int sys_function;
   SYMBOL *symbol;
   
   SCRIPT *parent_script;
   
   int links;
   INSTR **link;
};



struct memory_data
{
   LINKED_LIST( MEMORY );
   
   int size;
   VALUE *memory;
};



struct code_data
{
   LINKED_LIST( CODE );
   
   SYMBOL_TABLE local_symbol_table;
   
   INSTR *first_instruction;
};



struct function_data
{
   LINKED_LIST( FUNCTION );
   
   char *name;
   short alias;
   int args_nr;
   
   POINTER pointer;
   
   SCRIPT *parent;
};



struct trigger_data
{
   LINKED_LIST( TRIGGER );
   
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
   
   POINTER pointer;
   
   SCRIPT *parent;
};



struct script_data
{
   LINKED_LIST( SCRIPT );
   
   char *name;
   char *path;
   char *description;
   
   time_t modified_date;
   
   SYMBOL_TABLE *scope_symbol_table;
   SYMBOL_TABLE script_symbol_table;
   
   CODE *codes, *codes_last;
   
   FUNCTION *functions, *functions_last;
   TRIGGER *triggers, *triggers_last;
   
   SCRIPT_GROUP *parent_group;
   
   CODE *init_code;
};



// FIXME: don't forget about destroying
struct script_group_data
{
   LINKED_LIST( SCRIPT_GROUP );
   
   char *name;
   
   short changed;
   
   SCRIPT *scripts, *scripts_last;
   
   SYMBOL_TABLE group_symbol_table;
   
   VALUE *script_memory;
   VALUE *script_memory_top;
   
   MEMORY *dynamic_memory, *dm_last;
};



/* Convenience structure. */
struct prev_data
{
   int bottom, top;
   VARIABLE *function_return_value;
};





/*** Instruction Handler Table ***/

#define INSTR_FUNCTION		0
#define INSTR_DOALL		1
#define INSTR_OPERATOR		2
#define INSTR_CONSTANT		3
#define INSTR_VARIABLE		4
#define INSTR_RETURN		5
#define INSTR_IF		6
#define INSTR_WHILE		7

typedef int instruction_handler( INSTR *, VARIABLE * );
instruction_handler instr_function;
instruction_handler instr_doall;
instruction_handler instr_operator;
instruction_handler instr_constant;
instruction_handler instr_variable;
instruction_handler instr_return;
instruction_handler instr_if;
instruction_handler instr_while;

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
     instr_while,
     
     NULL
};



/*** Operator Table ***/

#define P VAR_POINTER
#define S VAR_STRING

typedef int operator_handler( VARIABLE *, VARIABLE *, VARIABLE *, INSTR * );
operator_handler oper_not_implemented;
operator_handler oper_dereference;
operator_handler oper_address_of;
operator_handler oper_assignment;
operator_handler oper_multiplication;
operator_handler oper_division;
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
     { "*",	0, 0,	0, 2,	0,	0, oper_multiplication	},
     { "/",	0, 0,	0, 2,	0,	0, oper_division	},
     { "%",	0, 0,	0, 2,	0,	0, oper_modulus		},
   /* Addition, substraction. */
     { "+",	0, 0,	0, 3,	P|S,	0, oper_addition	},
     { "-",	0, 0,	0, 3,	P,	0, oper_substraction	},
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
system_function sysfunc_nr;
system_function sysfunc_str;
system_function sysfunc_memory;
system_function sysfunc_resize;
system_function sysfunc_sizeof;
system_function sysfunc_get_mb_variable;
system_function sysfunc_replace_line;

struct sys_function_data
{
   char *name;
   
   int args;
   short full_arg_priority;
   
   system_function *function;
} sys_functions[ ] =
{
     { "echo",			1, 1, sysfunc_echo		},
     { "send",			1, 1, sysfunc_send		},
     { "necho",			1, 1, sysfunc_necho		},
     { "nsend",			1, 1, sysfunc_nsend		},
     { "arg",			1, 0, sysfunc_arg		},
     { "args",			2, 0, sysfunc_args		},
     { "show_prompt",		0, 0, sysfunc_show_prompt	},
     { "debug",			0, 0, sysfunc_debug		},
     { "nr",			1, 0, sysfunc_nr		},
     { "str",			1, 0, sysfunc_str		},
     { "memory",		1, 0, sysfunc_memory		},
     { "resize",		1, 0, sysfunc_resize		},
     { "sizeof",		1, 0, sysfunc_sizeof		},
     { "get_mb_variable",	1, 0, sysfunc_get_mb_variable	},
     { "replace_line",		1, 0, sysfunc_replace_line	},
   
     { NULL }
};

int last_system_function;


/* Alias and Regex callbacks. */

char *current_line;
int regex_callbacks;
int regex_ovector[60];
int alias_args_start[16];
int alias_args_end[16];

int target_offset;

#define SB_FULL_SIZE 40960
#define SB_HALF_SIZE SB_FULL_SIZE / 2

char searchback_buffer[SB_FULL_SIZE];
int sb_size;

int returning_successful;

/* This one has it all. All the goodness is fit in here. */
SCRIPT_GROUP *groups, *groups_last;

SCRIPT_GROUP *current_group;
SCRIPT *current_script;
CODE *current_code;

SYMBOL_TABLE global_symbol_table;

VALUE *global_memory;

VALUE *local_memory;
VALUE *current_local_bottom;
VALUE *current_local_top;
VALUE *top_of_allocated_memory_for_locals;

VARIABLE *function_return_value;


INSTR *compile_command( int priority );
int run_code( CODE *code );



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



/* Generic linked-list handlers. Had some really neat functions, but they didn't go well with high optimization. */
#define link_to_list( obj, first, last ) \
{ if ( last ) { (last)->next = obj; (obj)->prev = last; (obj)->next = NULL; last = obj; } \
		 else { first = last = obj; (obj)->next = NULL; (obj)->prev = NULL; } }

#define unlink_from_list( obj, first, last ) \
{ if ( (obj)->prev ) (obj)->prev->next = (obj)->next; else first = (obj)->next; \
  if ( (obj)->next ) (obj)->next->prev = (obj)->prev; else last = (obj)->prev; }

#define replace_link_in_list( src, dest, first, last ) \
{ (src)->prev = (dest)->prev; (src)->next = (dest)->next; \
  if ( (src)->prev ) (src)->prev->next = src; else first = src; \
  if ( (src)->next ) (src)->next->prev = src; else last = src; }


void exec_trigger( TRIGGER *trigger )
{
   run_code( (CODE*)trigger->pointer.memory_section );
}



void check_triggers( LINE *l, int prompt )
{
   SCRIPT_GROUP *group;
   SCRIPT *script;
   TRIGGER *trigger;
   char *line;
   int len;
   int rc;
   
   for ( group = groups; group; group = group->next )
     for ( script = group->scripts; script; script = script->next )
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
                 rc = pcre_exec( trigger->pattern, trigger->extra, line, len, 0, 0, regex_ovector, 60 );
                 
                 if ( rc < 0 )
                   continue;
                 
                 /* Regex Matched. Beware of recursiveness! */
                 
                 if ( rc == 0 )
                   regex_callbacks = 20;
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
   global_memory = realloc( global_memory, size * sizeof( VALUE ) );
   
   V_TYPE( *(global_memory+target_offset) ) = VAR_STRING;
   share_memory( "target", &V_STR( *(global_memory + target_offset) ), sizeof( char * ) );
   shared_memory_is_pointer_to_string( "target" );
}



void scripter_module_init_data( )
{
   SYMBOL *add_symbol_to_table( char *name, int unique, SYMBOL_TABLE *table, int space );
   void load_scripts( );
   
   DEBUG( "scripter_init_data" );
   
   current_line = "";
   set_args( "" );
   
   // fix..?
   /* Initialize the common variables. */
   add_symbol_to_table( "target", 1, &global_symbol_table, GLOBAL_MEMORY );
   target_offset = 0;
   
   resize_global_memory( global_symbol_table.memory_size );
   
   /* Initialize memory for scripts. */
   if ( !local_memory )
     {
	local_memory = calloc( 32, sizeof( VALUE ) );
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
   
   clientff( "> %.*s" C_R "%.*s" C_0 "%.*s\r\n",
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



#if 0
void show_symbol( SYMBOL *s )
{
   clientff( " - %s: %s", s->name, s->is_constant ? "constant = " : "variable at " );
   if ( s->is_constant )
     {
        if ( V_TYPE( s->constant ) == VAR_NUMBER )
          clientff( "%d", V_NR( s->constant ) );
        else if ( V_TYPE( s->constant ) == VAR_STRING )
          clientff( "\"%s\"", V_STR( s->constant ) );
        else if ( V_TYPE( s->constant ) == VAR_POINTER )
          clientff( "pointer" );
     }
   else
     {
        clientff( "%d (in %d)", s->pointer.offset, s->pointer.memory_space );
     }
   clientff( C_0 "\r\n" );
}
#endif



#define add_object( obj ) (*objects)++, (*total_size) += sizeof( obj )
#define add_string( str ) if ( str ) (*strings)++, (*string_size) += 1 + strlen( str )
#define add_memory( mem ) (*mem_zones)++, (*memory_size) += (mem);

void recursive_add_instruction( INSTR *instr,
                                int *objects, int *total_size,
                                int *strings, int *string_size )
{
   int i;
   
   add_object( INSTR );
   if ( V_TYPE( instr->constant ) == VAR_STRING )
     add_string( V_STR( instr->constant ) );
   
   for ( i = 0; i < instr->links; i++ )
     if ( instr->link[i] )
       recursive_add_instruction( instr->link[i],
                                  objects, total_size,
                                  strings, string_size );
}

void show_statistics( SCRIPT_GROUP *groups )
{
   SCRIPT_GROUP *g;
   SCRIPT *s;
   SYMBOL *sb;
   SYMBOL_TABLE *tb;
   CODE *c;
   FUNCTION *f;
   TRIGGER *t;
   
   int data[6];
   int *objects = &data[0];
   int *strings = &data[1];
   int *mem_zones = &data[2];
   int *total_size = &data[3];
   int *string_size = &data[4];
   int *memory_size = &data[5];
   memset( data, 0, 6*sizeof( int ) );
   
   add_memory( top_of_allocated_memory_for_locals - local_memory );
   add_memory( global_symbol_table.memory_size );
   
   for ( g = groups; g; g = g->next )
     {
        add_object( SCRIPT_GROUP );
        add_string( g->name );
        
        for ( sb = g->group_symbol_table.symbol; sb; sb = sb->next )
          {
             add_object( SYMBOL );
             add_string( sb->name );
             if ( V_TYPE( sb->constant ) == VAR_STRING )
               add_string( V_STR( sb->constant ) );
          }
        
        add_memory( g->script_memory_top - g->script_memory );
        
        for ( s = g->scripts; s; s = s->next )
          {
             add_object( SCRIPT );
             add_string( s->name );
             add_string( s->path );
             add_string( s->description );
             
             for ( sb = s->script_symbol_table.symbol; sb; sb = sb->next )
               {
                  add_object( SYMBOL );
                  add_string( sb->name );
                  if ( V_TYPE( sb->constant ) == VAR_STRING )
                    add_string( V_STR( sb->constant ) );
               }
             
             if ( s->scope_symbol_table )
               for ( tb = s->scope_symbol_table; tb; tb = tb->lower )
                 for ( sb = tb->symbol; sb; sb = sb->next )
                 {
                    add_object( SYMBOL );
                    add_string( sb->name );
                    if ( V_TYPE( sb->constant ) == VAR_STRING )
                      add_string( V_STR( sb->constant ) );
                 }
             
             for ( c = s->codes; c; c = c->next )
               {
                  add_object( CODE );
                  
                  for ( sb = c->local_symbol_table.symbol; sb; sb = sb->next )
                    {
                       add_object( SYMBOL );
                       add_string( sb->name );
                       if ( V_TYPE( sb->constant ) == VAR_STRING )
                         add_string( V_STR( sb->constant ) );
                    }
               }
             
             for ( f = s->functions; f; f = f->next )
               {
                  add_object( FUNCTION );
                  add_string( f->name );
               }
             for ( t = s->triggers; t; t = t->next )
               {
                  add_object( TRIGGER );
                  add_string( t->name );
                  add_string( t->message );
               }
          }
     }
   
   clientfr( "iScript Statistics" );
   clientff( "Strings: %d, occupying %d bytes.\r\n", *strings, *string_size );
   clientff( "Objects: %d, occupying %d bytes.\r\n", *objects, *total_size );
   clientff( "M Zones: %d, occupying %d bytes, in %d variables.\r\n", *mem_zones, *memory_size * sizeof( VALUE ), *memory_size );
   clientff( "A grand total of: %.2f megabytes of your precious RAM.\r\n",
             (float) ( *string_size + *total_size + ( *memory_size * sizeof( VALUE ) ) ) / (1024*1024) );
}



void show_scripts( )
{
   SCRIPT_GROUP *group;
   SCRIPT *script;
   
   clientfr( "Loaded iScripts:" );
   for ( group = groups; group; group = group->next )
     {
        clientff( C_B "* %s:\r\n", group->name );
        for ( script = group->scripts; script; script = script->next )
          {
             clientff( C_B " - %s/" C_W "%s",
                       script->path, script->name );
             if ( script->description )
               clientff( C_D " (%s)", script->description );
             clientff( "\r\n" C_0 );
          }
     }
}



int scripter_process_client_aliases( char *line )
{
   SCRIPT_GROUP *group;
   SCRIPT *script;
   FUNCTION *function;
   char command[4096], *args;
   
   DEBUG( "scripter_process_client_aliases" );
   
   args = get_string( line, command, 4096 );
   
   if ( !strcmp( command, "grep" ) )
     {
	do_searchback( args );
	show_prompt( );
	return 1;
     }
   
   /* Look around the alias list. */
   
   for ( group = groups; group; group = group->next )
     for ( script = group->scripts; script; script = script->next )
       for ( function = script->functions; function; function = function->next )
         if ( function->alias && !strcmp( function->name, command ) )
           {
              current_line = line;
              set_args( line );
              
              run_code( (CODE*)function->pointer.memory_section );
              
              return 1;
           }
   
   if ( strcmp( command, "is" ) )
     return 0;
   
   args = get_string( args, command, 4096 );
   
   if ( !strcmp( command, "load" ) )
     {
	void load_scripts( );
	load_scripts( );
     }
   else if ( !strcmp( command, "laod" ) )
     {
        void load_scripts( );
        clientfr( "I'll assume you meant 'load'. Learn to type please." );
        load_scripts( );
     }
   else if ( !strcmp( command, "stat" ) ||
             !strcmp( command, "stats" ) ||
             !strcmp( command, "statistics" ) )
     {
        show_statistics( groups );
     }
   else if ( !strcmp( command, "scripts" ) ||
             !strcmp( command, "groups" ) )
     {
        show_scripts( );
     }
#if 0
   else if ( !strcmp( command, "show" ) )
     {
	void do_show( );         
	
	do_show( !strcmp( args, "full" ) );
     }
   else if ( !strcmp( command, "symbols" ) )
     {
        SCRIPT *script;
        SYMBOL *s;
        
        clientfr( "Global Symbols:" );
        for ( s = global_symbol_table.symbol; s; s = s->next )
          show_symbol( s );
        
        clientf( "\r\n" );
        clientfr( "Script Symbols:" );
        for ( script = scripts; script; script = script->next )
          {
             clientfr( script->name );
             for ( s = script->script_symbol_table.symbol; s; s = s->next )
               show_symbol( s );
          }
     }
   else if ( !strcmp( command, "scripts" ) )
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
	     
	     clientff( C_W "* " C_B "%s" C_0 " (" C_B "%s" C_0 ") - "
                       C_G "%d" C_D " function%s, "
                       C_G "%d" C_D " alias%s, "
                       C_G "%d" C_D " trigger%s." C_0 "\r\n",
		       script->name, script->description,
                       f_count, f_count == 1 ? "" : "s",
                       a_count, a_count == 1 ? "" : "es",
                       t_count, t_count == 1 ? "" : "s" );
	  }
     }
#endif
   else if ( !strcmp( command, "call" ) )
     {
	int run_function_with_args( FUNCTION *function, char *args );
        SCRIPT_GROUP *g;
	SCRIPT *s;
	FUNCTION *f;
	char name[256];
	
	args = get_string( args, name, 256 );
	
	f = NULL;
        for ( g = groups; g; g = g->next )
          for ( s = g->scripts; s; s = s->next )
            for ( f = s->functions; f; f = f->next )
              if ( !strcmp( name, f->name ) )
                {
                   current_line = args;
                   set_args( args );
                   
                   run_function_with_args( f, args );
                   
                   return 1;
                }
        
        clientfr( "Function not found. Perhaps you misspelled it?" );
        show_prompt( );
     }
   
   else
     clientfr( "Unknown IScript command." );
   
   show_prompt( );
   return 1;
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



// Test me. max overflow.
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



int get_line_number( )
{
   char *p = beginning;
   int line = 1, column = 1;
   
   if ( !p )
     return -1;
   
   while ( *p )
     {
        if ( *p == '\n' )
          line++, column = 1;
        else if ( *p != '\r' )
          column++;
        
        p++;
        if ( p == pos )
          break;
     }
   
   /* Just.. in case a file is THAT big. */
   if ( line == 0 )
     line = -1;
   
   return line;
}



void abort_script( int syntax, char *error, ... )
{
   char buf[1024];
   char where[1024];
   char what[1024];
   char error_buf[1024];
   
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
     sprintf( where, " in file %s/%s:%d", current_script->path, current_script->name, get_line_number( ) );
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



void copy_value( VALUE *v1, VALUE *v2 )
{
//   if ( v2->type &&
//	v2->type != VAR_NUMBER &&
//	v2->type != VAR_STRING &&
//	v2->type != VAR_POINTER )
  //   script_warning( "Assigning to an uninitialized variable!" );
   
   if ( V_TYPE( *v2 ) == VAR_STRING && V_STR( *v2 ) )
     free( V_STR( *v2 ) );
   
   *v2 = *v1;
   if ( V_TYPE( *v2 ) == VAR_STRING )
     V_STR( *v2 ) = strdup( V_STR( *v1 ) );
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
        v->address.memory_section = pointer.memory_section;
     }
}


void destroy_symbols( SYMBOL_TABLE *table )
{
   SYMBOL *symbol;
   
   while ( table->symbol )
     {
        symbol = table->symbol;
        table->symbol = symbol->next;
        
        FREE_VALUE( symbol->constant );
        if ( symbol->name )
          free( symbol->name );
        free( symbol );
     }
}



void destroy_instruction( INSTR *instr )
{
   int i;
   
   FREE_VALUE( instr->constant );
   
   for ( i = 0; i < instr->links; i++ )
     destroy_instruction( instr->link[i] );
   
   if ( instr->link )
     free( instr->link );
   
   free( instr );
}



void destroy_script( SCRIPT *script )
{
   FUNCTION *f, *f_next;
   TRIGGER *t, *t_next;
   CODE *c, *c_next;
   SYMBOL_TABLE *table;
   
   if ( script->name )
     free( script->name );
   if ( script->description )
     free( script->description );
   
   for ( c = script->codes; c; c = c_next )
     {
        c_next = c->next;
        
        if ( c->first_instruction )
          destroy_instruction( c->first_instruction );
        
        free( c );
     }
   
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
   
   while ( script->scope_symbol_table )
     {
        table = script->scope_symbol_table;
        script->scope_symbol_table = table->lower;
        
        destroy_symbols( table );
        free( table );
     }
   
   destroy_symbols( &script->script_symbol_table );
   
   free( script );
}



void destroy_group( SCRIPT_GROUP *group )
{
   // add strings, add everything
   
   destroy_symbols( &group->group_symbol_table );
   
   if ( group->script_memory )
     free( group->script_memory );
   
   free( group );
}



SYMBOL *add_symbol_to_table( char *name, int unique, SYMBOL_TABLE *table, int space )
{
   SYMBOL *symbol;
   
   /* Make sure it doesn't exist already. */
   if ( unique )
     for ( symbol = table->symbol; symbol; symbol = symbol->next )
       if ( symbol->name && !strcmp( name, symbol->name ) )
         {
            if ( unique == -1 )
              {
                 symbol->declared_at = get_line_number( beginning, pos );
                 return symbol;
              }
            
            abort_script( 0, "Symbol '%s' redefined. Previously defined at line %d.",
                          name, symbol->declared_at );
            return NULL;
         }
   
   /* First, see if we can recycle any unused entries. */
   for ( symbol = table->symbol; symbol; symbol = symbol->next )
     if ( !symbol->name )
       break;
   
   /* And if not, create a brand new one. */
   if ( !symbol )
     {
        symbol = calloc( 1, sizeof( SYMBOL ) );
        link_to_list( symbol, table->symbol, table->last );
     }
   
   symbol->name = strdup( name );
   // file, too. headers, for example
   symbol->declared_at = get_line_number( beginning, pos );
   if ( space )
     {
        symbol->pointer.memory_space = space;
        symbol->pointer.offset = table->memory_size;
        table->memory_size++;
     }
   
   return symbol;
}



SYMBOL *create_constant( char *name, VALUE *constant )
{
   SYMBOL *scope_symbol;
   
   scope_symbol = add_symbol_to_table( name, 1, current_script->scope_symbol_table, 0 );
   if ( !scope_symbol )
     return NULL;
   
   scope_symbol->is_constant = 1;
   scope_symbol->constant = *constant;
   
   return scope_symbol;
}



SYMBOL *create_local_variable( char *name )
{
   SYMBOL *scope_symbol;
   SYMBOL *local_symbol;
   
   scope_symbol = add_symbol_to_table( name, 1, current_script->scope_symbol_table, 0 );
   if ( !scope_symbol )
     return NULL;
   
   if ( current_script->scope_symbol_table->lower )
     local_symbol = add_symbol_to_table( name, 0, &current_code->local_symbol_table, RELATIVE_LOCAL_MEMORY );
   else
     {
        char bigger_name[4096];
        
        /* Why? Prevents it from conflicting with other scripts. */
        snprintf( bigger_name, 4096, "%s/%s:%s", current_script->path, current_script->name, name );
        local_symbol = add_symbol_to_table( bigger_name, 1, &current_script->script_symbol_table, SCRIPT_MEMORY );
     }
   if( !local_symbol )
     return NULL;
   
   scope_symbol->symbol = local_symbol;
   
   return local_symbol;
}



SYMBOL *create_global_variable( char *name )
{
   SYMBOL *scope_symbol;
   SYMBOL *global_symbol;
   
   scope_symbol = add_symbol_to_table( name, 1, current_script->scope_symbol_table, 0 );
   if ( !scope_symbol )
     return NULL;
   
   global_symbol = add_symbol_to_table( name, -1, &global_symbol_table, GLOBAL_MEMORY );
   if( !global_symbol )
     return NULL;
   
   scope_symbol->symbol = global_symbol;
   
   return global_symbol;
}



VALUE *get_constant( char *name )
{
   SYMBOL_TABLE *table;
   SYMBOL *symbol;
   
   for ( table = current_script->scope_symbol_table; table; table = table->lower )
     for ( symbol = table->symbol; symbol; symbol = symbol->next )
       if ( symbol->is_constant && symbol->name && !strcmp( name, symbol->name ) )
         return &symbol->constant;
   
   return NULL;
}



// check all places for get_constant, too
SYMBOL *get_symbol( char *name )
{
   SYMBOL_TABLE *table;
   SYMBOL *symbol;
   
   for ( table = current_script->scope_symbol_table; table; table = table->lower )
     for ( symbol = table->symbol; symbol; symbol = symbol->next )
       if ( symbol->name && !strcmp( name, symbol->name ) )
         return symbol->symbol;
   
   /* Doesn't exist? Create a script-wide variable on the bottom, then. */
   
   // check validity first.
   
   return add_symbol_to_table( name, -1, &current_script->script_symbol_table, SCRIPT_MEMORY );
}



void enter_scope( SCRIPT *script )
{
   SYMBOL_TABLE *table;
   
   table = calloc( 1, sizeof( SYMBOL_TABLE ) );
   
   if ( !script->scope_symbol_table )
     {
        script->scope_symbol_table = table;
        return;
     }
   
   table->lower = script->scope_symbol_table;
   script->scope_symbol_table = table;
}



void leave_scope( SCRIPT *script )
{
   SYMBOL_TABLE *table;
   
   table = script->scope_symbol_table;
   script->scope_symbol_table = table->lower;
   
   destroy_symbols( table );
   free( table );
}



INSTR *new_instruction( int instruction )
{
   INSTR *instr;
   
   instr = calloc( 1, sizeof( INSTR ) );
   
   instr->instruction = instruction;
   instr->parent_script = current_script;
   
   return instr;
}



void link_instruction( INSTR *child, INSTR *parent )
{
   parent->link = realloc( parent->link, sizeof( INSTR * ) * (parent->links+1) );
   parent->link[parent->links] = child;
   parent->links++;
}



int force_constant( VALUE *val )
{
   INSTR *instr;
   
   instr = compile_command( FULL_PRIORITY );
   if ( !pos )
     return 1;
   
   /* This will bring all instructions into one constant. */
     {
        void optimize_constants( INSTR *instr );
        optimize_constants( instr );
     }
   
   if ( instr->instruction != INSTR_CONSTANT )
     {
        abort_script( 0, "Constant value required." );
        destroy_instruction( instr );
        return 1;
     }
   
   copy_value( &instr->constant, val );
   destroy_instruction( instr );
   return 0;
}



void create_initializer( SYMBOL *symbol, VALUE *constant )
{
   /* Use is_constant for now.. even though it's not a constant. */
   if ( symbol->is_constant )
     {
        script_warning( "Symbol '%s' redefined!", symbol->name );
        FREE_VALUE( symbol->constant );
     }
   
   symbol->is_constant = 1;
   symbol->constant = *constant;
}



int load_declaration( SYMBOL *use_symbol, int global, int constant, INSTR *parent )
{
   INSTR *instr = NULL, *c_instr;
   SYMBOL *symbol;
   VALUE val;
   char name[4096];
   int have_value = 0;
   
   INIT_VALUE( val );
   
   /* General: [[local/global/constant] name] [= [value/constant]] */
   
   /* First, get its name.. if needed. */
   if ( !use_symbol )
     {
        get_identifier( name, 4096 );
        if ( !name[0] )
          {
             abort_script( 1, "Expected identifier." );
             return 1;
          }
        
        // check validity
     }
   
   /* Then see if it has a value. */
   
   skip_whitespace( );
   if ( *pos == '=' )
     {
        pos++;
        
        /* Only constant initializers allowed here. */
        if ( constant || !parent )
          {
             if ( force_constant( &val ) )
               return 1;
          }
        else
          {
             instr = compile_command( FULL_PRIORITY );
             if ( !pos )
               return 1;
          }
        
        have_value = 1;
     }
   else if ( constant || use_symbol )
     {
        abort_script( 1, "Expected '=' after '%s'.",
                      use_symbol ? use_symbol->name : name );
        return 1;
     }
   else
     instr = NULL;
   
   /* Then, create the symbol! */
   if ( constant )
     {
        if ( !create_constant( name, &val ) )
          return 1;
     }
   else
     {
        if ( use_symbol )
          symbol = use_symbol;
        else if ( global )
          symbol = create_global_variable( name );
        else
          symbol = create_local_variable( name );
        
        if ( !symbol )
          {
             if ( instr )
               destroy_instruction( instr );
             return 1;
          }
        
        /* And give it a value at startup, if we must. */
        if ( have_value )
          {
             if ( !parent )
               {
                  create_initializer( symbol, &val );
               }
             else
               {
                  INSTR *variable;
                  int i;
                  
                  for ( i = 0; operators[i].symbol; i++ )
                    if ( !strcmp( operators[i].symbol, "=" ) )
                      break;
                  
                  if ( !operators[i].symbol )
                    {
                       abort_script( 0, "Internal error... Operator table is buggy!" );
                       destroy_instruction( instr );
                       return 1;
                    }
                  
                  c_instr = new_instruction( INSTR_OPERATOR );
                  c_instr->oper = i;
                  
                  variable = new_instruction( INSTR_VARIABLE );
                  link_instruction( variable, c_instr );
                  variable->symbol = symbol;
                  variable->pointer = symbol->pointer;
                  
                  link_instruction( instr, c_instr );
                  link_instruction( c_instr, parent );
               }
          }
     }
   
   skip_whitespace( );
   if ( !use_symbol && *pos == ',' )
     {
        pos++;
        
        return load_declaration( NULL, global, constant, parent );
     }
   
   skip_whitespace( );
   if ( *pos != ';' )
     {
        abort_script( 1, "Expected ';' instead." );
        return 1;
     }
   
   return 0;
}



INSTR *compile_operator( INSTR *instr, int priority )
{
   INSTR *instr2, *i_oper, *i_func;
   char *temp_pos, buf[4096];
   int i;
   
   skip_whitespace( );
   
   /* Check for function calls. */
   if ( *pos == '(' || ( instr->sys_function &&
                         instr->instruction == INSTR_VARIABLE ) )
     {
        int args = 0;
        int expect_ending_bracket = 0;
        int first = 1;
        
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
             i_func->sys_function = 1;
             args = sys_functions[i_func->pointer.offset].args;
	     destroy_instruction( instr );
	  }
	
	/* Next, are arguments. */
        
	if ( *pos == '(' )
	  pos++, expect_ending_bracket = 1;
        
        while ( 1 )
          {
             skip_whitespace( );
             
             /* System functions have a specific argument number.
              * Normal functions don't. */
             if ( i_func->sys_function )
               {
                  if ( !args )
                    break;
                  
                  args--;
               }
             else
               if ( *pos == ')' )
                 break;
             
             /* Require commas between arguments. */
             if ( !first )
               {
		  if ( *pos != ',' )
		    {
		       abort_script( 1, "Expected ',' between function arguments, or ')'." );
		       destroy_instruction( i_func );
		       return NULL;
                    }
                  pos++;
                  
                  skip_whitespace( );
               }
             else
               first = 0;
             
             /* Last argument of a system (bracket-less) function? */
             if ( i_func->sys_function && args == 0 &&
                  !sys_functions[i_func->pointer.offset].full_arg_priority &&
                  !expect_ending_bracket )
               instr2 = compile_command( UNARY_PRIORITY );
             else
               instr2 = compile_command( FULL_PRIORITY );
             
	     if ( !pos )
	       {
		  destroy_instruction( i_func );
		  return NULL;
	       }
	     
	     link_instruction( instr2, i_func );
          }
        
        if ( expect_ending_bracket )
          {
             skip_whitespace( );
             
             if ( *pos != ')' )
               {
                  abort_script( 1, "Expected ')' instead." );
                  return NULL;
               }
             
             pos++;
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



// Limited by 'int', of course. Not good.
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
        
        /* Example 5e2 = 5 * pow(10,2) */
        if ( *pos == 'e' || *pos == 'E' )
          {
             int e = 0;
             
             pos++;
             
             while ( *pos >= '0' && *pos <= '9' )
               {
                  e *= 10;
                  e += (int) (*pos - '0');
                  
                  pos++;
               }
             
             while ( e )
               nr *= 10, e--;
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
	
	V_TYPE( instr->constant ) = VAR_STRING;
	V_STR( instr->constant ) = strdup( buf );
	
	return compile_operator( instr, priority );
     }
   
   /* Constant number. */
   if ( *pos >= '0' && *pos <= '9' )
     {
	nr = get_number( );
	
	if ( !pos )
	  return NULL;
	
	instr = new_instruction( INSTR_CONSTANT );
        V_TYPE( instr->constant ) = VAR_NUMBER;
        V_NR( instr->constant ) = nr;
	
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
        SYMBOL *symbol;
        
	get_identifier( buf, 4096 );
	
        instr = new_instruction( INSTR_VARIABLE );
        
	/* System functions don't need parantheses. For compile_operator. */
	for ( i = 0; sys_functions[i].name; i++ )
	  if ( !strcmp( sys_functions[i].name, buf ) )
	    instr->sys_function = 1 + i;
        
        if ( !instr->sys_function )
          {
             VALUE *c;
             
             c = get_constant( buf );
             if ( c )
               {
                  destroy_instruction( instr );
                  instr = new_instruction( INSTR_CONSTANT );
                  
                  copy_value( c, &instr->constant );
               }
             else
               {
                  symbol = get_symbol( buf );
                  if ( !symbol )
                    return NULL;
                  instr->symbol = symbol;
                  instr->pointer = symbol->pointer;
               }
          }
	
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
     {
        expect_end_of_block = 1, pos++;
        enter_scope( current_script );
     }
   
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
             leave_scope( current_script );
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
	     /* if ( <command> ) <block> [else <block>] */
	     
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
	       break;
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
	else if ( expect_end_of_block &&
                  ( !strcmp( buf, "local" ) ||
                    !strcmp( buf, "global" ) ||
                    !strcmp( buf, "const" ) ||
                    !strcmp( buf, "constant" ) ) )
          {
             int global = 0;
             int constant = 0;
             
             if ( !strcmp( buf, "global" ) )
               global = 1;
             else if ( !strcmp( buf, "const" ) || !strcmp( buf, "constant" ) )
               constant = 1;
             
             if ( load_declaration( NULL, global, constant, do_all ) )
               return 1;
          }
        else if ( !strcmp( buf, "while" ) )
          {
             /* while ( <command> ) <block> */
             
	     skip_whitespace( );
	     
	     if ( *pos != '(' )
	       {
		  abort_script( 1, "Expected '(' instead." );
		  return 1;
	       }
	     pos++;
	     
	     c_instr = new_instruction( INSTR_WHILE );
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
	     
	     /* This will go round an' round... */
	     compile_block( c_instr );
	     if ( !pos )
	       return 1;
             
	     if ( expect_end_of_block )
	       continue;
	     else
	       break;
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
   FUNCTION *function;
   VALUE v;
   CODE *code;
   CODE *previous_code;
   char buf[4096];
   int expect_ending, expect_argument;
   
   INIT_VALUE( v );
   
   /* A function must have:
    *   - a name
    *   - a code
    *   - arguments as local variables
    *   - the number of arguments
    */
   
   function = calloc( 1, sizeof( FUNCTION ) );
   function->parent = script;
   
   /* Add a code in the script for it. */
   code = calloc( 1, sizeof( CODE ) );
   link_to_list( code, script->codes, script->codes_last );
   previous_code = current_code;
   current_code = code;
   
   /* Make a pointer for it. */
   V_TYPE( v ) = VAR_POINTER;
   V_POINTER( v ).memory_space = SCRIPT_FUNCTIONS;
   V_POINTER( v ).offset = 0;
   V_POINTER( v ).memory_section = (void *) code;
   function->pointer = V_POINTER( v );
   
   /* Get its name. */
   get_identifier( buf, 4096 );
   
   // check validity
   
   if ( !buf[0] )
     {
        abort_script( 0, "%s name not provided.", alias ? "Alias" : "Function" );
        current_code = previous_code;
        return 1;
     }
   
   /* In case of a function, make its name a variable. */
   if ( !alias )
     {
        SYMBOL *symbol;
        VALUE v2;
        
        copy_value( &v, &v2 );
        
        /* Initialize a variable for the other scripts. */
        symbol = add_symbol_to_table( buf, -1, &current_script->script_symbol_table, SCRIPT_MEMORY );
        if ( !symbol )
          return 1;
        create_initializer( symbol, &v2 );
        
        /* And a constant for this script, for speed. */
        if ( !create_constant( buf, &v ) )
          {
             current_code = previous_code;
             return 1;
          }
     }
   
   /* Link it. Easier to get destroyed with the rest if something fails. */
   link_to_list( function, script->functions, script->functions_last );
   
   /* This scope will keep all the arguments. */
   enter_scope( script );
   
   if ( !alias )
     {
	/* Format: function <name> [ [(] [arg1 [[,] arg2]... ] [)] ] { ... } */
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
             
             if ( !create_local_variable( buf ) )
               {
                  current_code = previous_code;
                  return 1;
               }
             
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
                  current_code = previous_code;
		  return 1;
	       }
	     
	     pos++;
	     skip_whitespace( );
	  }
     }
   else
     {
	/* Format: alias <name> { ... } */
	function->name = strdup( buf );
	function->alias = 1;
     }
   
   /* Common code for 'function' and 'alias'. */
   
   if ( compile_code( code ) )
     return 1;
   
   leave_scope( script );
   
   current_code = previous_code;
   
   return 0;
}



int load_script_trigger( SCRIPT *script )
{
   TRIGGER *trigger;
   CODE *code;
   char buf[4096];
   
   /* Format: trigger ['<name>'] [options] <*|"pattern"> { ... } */
   
   trigger = calloc( 1, sizeof( TRIGGER ) );
   trigger->parent = script;
   
   /* Add a code in the script for it. */
   code = calloc( 1, sizeof( CODE ) );
   link_to_list( code, script->codes, script->codes_last );
   
   trigger->pointer.memory_space = SCRIPT_FUNCTIONS;
   trigger->pointer.offset = 0;
   trigger->pointer.memory_section = (void*) code;
   
   /* Link it. */
   link_to_list( trigger, script->triggers, script->triggers_last );
   
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



int compile_script( SCRIPT *script, char *flname )
{
   void link_all_symbols( SCRIPT *script );
   SCRIPT *previous_script;
   FILE *fl;
   char buf[4096];
   char *old_pos, *old_beginning;
   char *body = NULL;
   int bytes, size = 0;
   int aborted = 0;
   
   fl = fopen( flname, "r" );
   
   if ( !fl )
     return 1;
   
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
	return 0;
     }
   
   body[size] = 0;
   
   /* A scope that will contain all script-wide variables. */
   enter_scope( script );
   
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
        
        else if ( !strcmp( buf, "include" ) )
          {
             
             
             
          }
        
	else if ( !strcmp( buf, "global" ) ||
                  !strcmp( buf, "local" ) ||
                  !strcmp( buf, "constant" ) ||
                  !strcmp( buf, "const" ) )
	  {
             int global = 0;
             int constant = 0;
             
             if ( !strcmp( buf, "global" ) )
               global = 1;
             else if ( strcmp( buf, "local" ) )
               constant = 1;
             
             if ( load_declaration( NULL, global, constant, NULL ) )
               {
                  aborted = 1;
                  break;
               }
             
             skip_whitespace( );
	     if ( *pos != ';' )
	       {
		  abort_script( 1, "Expected ';' instead." );
                  aborted = 1;
		  break;
	       }
	     pos++;
	  }
        
        else
          {
             /* Most likely a variable being initialized. */
             if ( ( buf[0] >= 'a' && buf[0] <= 'z' ) ||
                  ( buf[0] >= 'A' && buf[0] <= 'Z' ) ||
                  buf[0] == '_' )
               {
                  SYMBOL *symbol;
                  
                  symbol = get_symbol( buf );
                  if ( !symbol )
                    {
                       aborted = 1;
                       break;
                    }
                  
                  if ( load_declaration( symbol, 0, 0, NULL ) )
                    return 1;
                  
                  skip_whitespace( );
                  if ( *pos != ';' )
                    {
                       abort_script( 1, "Expected ';' instead." );
                       aborted = 1;
                       break;
                    }
                  pos++;
               }
             else
               {
                  abort_script( 1, NULL );
                  aborted = 1;
                  break;
               }
          }
     }
   
   pos = old_pos;
   beginning = old_beginning;
   free( body );
   
   leave_scope( script );
   
   if ( aborted )
     {
	current_script = previous_script;
        return 1;
     }
   
//   link_all_symbols( script );
   
   current_script = previous_script;
   
   /* Resize global memory. */
   resize_global_memory( global_symbol_table.memory_size );
   
   return 0;
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
	if ( V_TYPE( instr->constant ) == VAR_STRING )
	  clientff( "(\"%s\")", V_STR( instr->constant ) );
	if ( V_TYPE( instr->constant ) == VAR_NUMBER )
	  clientff( "(%d)", V_NR( instr->constant ) );
	if ( V_TYPE( instr->constant ) == VAR_POINTER )
	  clientff( "(p%s, %d)",
		    V_POINTER( instr->constant ).memory_space == GLOBAL_MEMORY ? "-global" :
		    V_POINTER( instr->constant ).memory_space == RELATIVE_LOCAL_MEMORY ? "-local" : "",
		    V_POINTER( instr->constant ).offset );
	break;
      case INSTR_VARIABLE:
	clientff( C_r "variable (%s)", ( instr->symbol && instr->symbol->name ) ? instr->symbol->name : "no symbol" );
        if ( instr->sys_function )
          clientff( C_r " (sys function)" );
	if ( instr->symbol && instr->symbol->name )
	  clientff( C_r " (%s (%d))",
		    instr->symbol->pointer.memory_space == RELATIVE_LOCAL_MEMORY ? "local" :
		    instr->symbol->pointer.memory_space == GLOBAL_MEMORY ? "global" :
		    instr->symbol->pointer.memory_space == SCRIPT_MEMORY ? "script-wide" : C_B "unknown memory space" C_r,
		    instr->symbol->pointer.offset );
	else
	  clientff( C_r " (not linked)" );
	break;
      case INSTR_IF: clientff( C_r "if" ); break;
      case INSTR_RETURN: clientff( C_r "return" ); break;
      default: clientff( C_R "unknown" ); break;
     }
   
   clientff( C_0 "\r\n" );
   
   for ( i = 0; i < instr->links; i++ )
     show_instr( instr->link[i], space+1 );
}



// fixme
#if 0
void do_show( int full )
{
   SCRIPT *s;
   FUNCTION *f;
   
   for ( s = scripts; s; s = s->next )
     {
	clientff( C_Y "\r\n\r\nSCRIPT: " C_B "%s\r\n" C_0, s->name );
	
	for ( f = s->functions; f; f = f->next )
	  {
	     clientff( C_B "\r\n%s - %s.\r\n", f->alias ? "Alias" : "Function", f->name );
	     
	     if ( full )
	       show_instr( s->codes[f->code_offset].first_instruction, 0 );
	  }
     }
}
#endif



void free_memory( VALUE *memory, int size )
{
   int i;
   
   for ( i = 0; i < size; i++ )
     {
        FREE_VALUE( *(memory+i) );
     }
   
   free( memory );
}


void free_vars( VARIABLE *memory, int size )
{
   int i;
   
   for ( i = 0; i < size; i++ )
     {
        FREE_VARIABLE( *(memory+i) );
     }
   
   free( memory );
}



SCRIPT *find_script( char *group_name, char *path, char *name )
{
   SCRIPT_GROUP *group;
   SCRIPT *script;
   
   for ( group = groups; group; group = group->next )
     if ( !strcmp( group->name, group_name ) )
       break;
   
   if ( !group )
     return NULL;
   
   for ( script = group->scripts; script; script = script->next )
     if ( !strcmp( script->name, name ) && !strcmp( script->path, path ) )
       return script;
   
   return NULL;
}



void scan_for_scripts( SCRIPT_GROUP *group, char *current, DIR *dir )
{
   SCRIPT *script;
   DIR *dir2;
   struct dirent *dp;
   struct stat st;
   
   //clientff( "Scanning %s\n", current );
   
#if !defined( PATH_MAX )
# define PATH_MAX 4096
#endif
   while ( ( dp = readdir( dir ) ) )
     {
        char path[PATH_MAX];
        
        /* Ignore not only "." and "..", but also hidden UNIX files. */
        if ( dp->d_name[0] == '.' )
          continue;
        
        snprintf( path, PATH_MAX, "%s/%s", current, dp->d_name );
        //clientff( "Trying %s\n", path );
        dir2 = opendir( path );
        if ( !dir2 )
          {
             if ( stat( path, &st ) < 0)
               continue;
             
             /* A file! */
             if ( strlen( path ) < 4 ||
                  strcmp( path + strlen( path ) - 3, ".is" ) ||
                  dp->d_name[0] == '.' )
               continue;
             
             script = calloc( 1, sizeof( SCRIPT ) );
             script->name = strdup( dp->d_name );
             script->path = strdup( current +
                                    strlen( "iscripts/" ) +
                                    strlen( group->name ) );
             script->modified_date = st.st_mtime;
             script->parent_group = group;
             link_to_list( script, group->scripts, group->scripts_last );
             continue;
          }
        
        /* Another directory. */
        scan_for_scripts( group, path, dir2 );
        closedir( dir2 );
     }
}



void link_symbols_in_instruction( INSTR *instr )
{
   int i;
   
   for ( i = 0; i < instr->links; i++ )
     link_symbols_in_instruction( instr->link[i] );
   
   if ( instr->instruction == INSTR_VARIABLE )
     if ( instr->symbol->pointer.memory_space == SCRIPT_MEMORY )
       instr->pointer = instr->symbol->symbol->pointer;
}



void link_symbols_in_group( SCRIPT_GROUP *group )
{
   SCRIPT *script;
   SYMBOL *symbol;
   CODE *code;
   
   /* Reset the memory. */
   if ( group->script_memory )
     free_memory( group->script_memory, group->group_symbol_table.memory_size );
   
   destroy_symbols( &group->group_symbol_table );
   group->group_symbol_table.memory_size = 0;
   
   /* Add all symbols from all scripts. */
   for ( script = group->scripts; script; script = script->next )
     for ( symbol = script->script_symbol_table.symbol; symbol; symbol = symbol->next )
       {
          symbol->symbol = add_symbol_to_table( symbol->name, -1, &group->group_symbol_table, SCRIPT_MEMORY );
          
          if ( symbol->is_constant )
            {
               if ( symbol->symbol->is_constant )
                 script_warning( "Can not initialize variable '%s' more than once.", symbol->name );
               else
                 copy_value( &symbol->constant, &symbol->symbol->constant );
               symbol->symbol->is_constant = 1;
            }
       }
   
   /* And then make all variables point in the right direction. */
   for ( script = group->scripts; script; script = script->next )
     for ( code = script->codes; code; code = code->next )
       link_symbols_in_instruction( code->first_instruction );
   
   group->script_memory = calloc( group->group_symbol_table.memory_size, sizeof( VALUE ) );
   group->script_memory_top = group->script_memory + group->group_symbol_table.memory_size;
   
   /* Fill the memory with the initial values. */
   for ( symbol = group->group_symbol_table.symbol; symbol; symbol = symbol->next )
     {
        /* By "is_constant", I mean has_value. Maybe I'll change it later. */
        if ( symbol->is_constant && symbol->pointer.memory_space == SCRIPT_MEMORY )
          copy_value( &symbol->constant, group->script_memory + symbol->pointer.offset );
     }
}



void new_load_scripts( )
{
   static SCRIPT_GROUP *groups_under_construction, *guc_last;
   SCRIPT_GROUP *group, *prev_group, *g, *g_next;
   SCRIPT *script, *s, *s_next;
   DIR *dir, *dir2;
   struct dirent *dp;
   
   clientf( "Checking scripts...\r\n" );
   
   /* Level 1 - scan "iscripts" dir, create as many groups as dirs found. */
   //  if not rebuilding - copy symbol table here.
   /* Level 2 - recursively scan each group, create empty scripts. */
   //  if not rebuilding - check deps, mark which scripts can be copied.
   /* Level 3 - back here, start compiling. */
   //  ..and then what?
   
   // Get to work!
   
   if ( groups_under_construction )
     {
        clientfr( "Something is apparently wrong... Already rebuilding?" );
        return;
     }
   
   dir = opendir( "iscripts" );
   if ( !dir )
     {
        clientff( C_R "[Unable to open directory iscripts: %s]\r\n" C_0,
                  strerror( errno ) );
        return;
     }
   
   while ( ( dp = readdir( dir ) ) )
     {
        char path[PATH_MAX];
        
        /* Ignore not only "." and "..", but also hidden UNIX files. */
        if ( dp->d_name[0] == '.' )
          continue;
        
        snprintf( path, PATH_MAX, "iscripts/%s", dp->d_name );
        dir2 = opendir( path );
        if ( !dir2 )
          continue;
        
        group = calloc( 1, sizeof( SCRIPT_GROUP ) );
        group->name = strdup( dp->d_name );
        scan_for_scripts( group, path, dir2 );
		if ( group->scripts )
		{
          link_to_list( group, groups_under_construction, guc_last );
		}
		else
		  destroy_group( group );
        closedir( dir2 );
     }
   closedir( dir );
   
   prev_group = current_group;
   
   /* Tree ready. Start compiling new or changed scripts. */
   for ( group = groups_under_construction; group; group = group->next )
     {
        current_group = group;
        group->changed = 0;
        
        for ( script = group->scripts; script; script = script->next )
          {
             char full_name[PATH_MAX];
             
             /* See if we already have it compiled. */
             s = find_script( group->name, script->path, script->name );
             if ( s && script->modified_date == s->modified_date )
               continue;
             
             /* This group is changed. Reset it! */
             group->changed = 1;
             
             snprintf( full_name, PATH_MAX, "iscripts/%s%s/%s",
                       group->name, script->path, script->name );
             clientff( "Loading %s\r\n", full_name );
             if ( !compile_script( script, full_name ) )
               continue;
             
             /* Failed! Abort. */
             
             while ( groups_under_construction )
               {
                  group = groups_under_construction;
                  unlink_from_list( group, groups_under_construction, guc_last );
                  destroy_group( group );
               }
             
             clientfr( "Existing scripts left unchanged." );
             current_group = prev_group;
             return;
          }
        
        /* See if there's any missing (removed) scripts, too. */
        for ( g = groups; g; g = g->next )
          if ( !strcmp( g->name, group->name ) )
            {
               for ( s = g->scripts; s; s = s->next )
                 {
                    for ( script = group->scripts; script; script = script->next )
                      if ( !strcmp( script->name, s->name ) )
                        break;
                    
                    /* Missing! */
                    if ( !script )
                      {
                         clientff( "Script %s was apparently removed.\r\n", s->name );
                         group->changed = 1;
                         break;
                      }
                 }
            }
     }
   current_group = prev_group;
   
   /* From this moment on, errors may not occur. */
   
   /* Copy all old, already-compiled scripts, and linkify groups. */
   for ( group = groups_under_construction; group; group = group->next )
     {
        if ( !group->changed )
          continue;
        
        for ( script = group->scripts; script; script = s_next )
          {
             s_next = script->next;
             
             /* The opposite of above. */
             s = find_script( group->name, script->path, script->name );
             if ( !s || script->modified_date != s->modified_date )
               continue;
             
             unlink_from_list( s, s->parent_group->scripts, s->parent_group->scripts_last );
             replace_link_in_list( s, script, group->scripts, group->scripts_last );
             s->parent_group = group;
             
             destroy_script( script );
          }
        
        clientff( "Relinking symbols and resetting memory in group %s.\r\n", group->name );
        
        link_symbols_in_group( group );
     }
   
   /* First replace the current ones.. for the sake of their order in the list. */
   for ( group = groups; group; group = g_next )
     {
        g_next = group->next;
        
        for ( g = groups_under_construction; g; g = g->next )
          if ( !strcmp( g->name, group->name ) )
            break;
        
        if ( !g )
          {
             clientff( "Removing group %s.\r\n", group->name );
             unlink_from_list( group, groups, groups_last );
             destroy_group( group );
             continue;
          }
        
        if ( !g->changed )
          {
             unlink_from_list( g, groups_under_construction, guc_last );
             destroy_group( g );
             continue;
          }
        
        clientff( "Replacing group %s.\r\n", group->name );
        unlink_from_list( g, groups_under_construction, guc_last );
        replace_link_in_list( g, group, groups, groups_last );
        destroy_group( group );
     }
   
   /* And add the rest. */
   for ( group = groups_under_construction; group; group = g_next )
     {
        g_next = group->next;
        
        for ( g = groups; g; g = g->next )
          if ( !strcmp( g->name, group->name ) )
            break;
        
        if ( g )
          continue;
        
        unlink_from_list( group, groups_under_construction, guc_last );
        link_to_list( group, groups, groups_last );
     }
   
   if ( groups_under_construction )
     {
        clientff( "Impossible error! This shouldn't happen!\r\n" );
     }
   
   /* Clean up the mess. */
   while ( groups_under_construction )
     {
        group = groups_under_construction;
     }
   
   resize_global_memory( global_symbol_table.memory_size );
}



void load_scripts( )
{
   new_load_scripts( );
   return;
   
#if 0
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
#endif
}



/*** Optimizer. ***/


void optimize_constants( INSTR *instr )
{
   
   
   
}




/*** Runner. These -must- be optimized for speed and nothing else but speed! ***/




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
   local_memory = realloc( local_memory, 32 * sizeof( VALUE ) );
   current_local_bottom = local_memory + b_size;
   current_local_top = local_memory + t_size;
   top_of_allocated_memory_for_locals = local_memory + 32;
}



void end_code_call( struct prev_data *prev )
{
   current_local_bottom = local_memory + prev->bottom;
   current_local_top = local_memory + prev->top;
   
   function_return_value = prev->function_return_value;
}



int prepare_code_call( int size, VARIABLE *returns, struct prev_data *prev )
{
   prev->function_return_value = function_return_value;
   function_return_value = returns;
   
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
	
	local_memory = realloc( local_memory, ( l_size + 32 ) * sizeof( VALUE ) );
	current_local_bottom = local_memory + b_size;
	current_local_top = local_memory + t_size;
	memset( local_memory + l_size, 0, 32 * sizeof( VALUE ) );
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
   VARIABLE returned_value, function_value;
   VALUE *v, *locals = NULL;
   struct prev_data prev;
   char buf[2048];
   int i;
   
   struct timeval call_start, call_prepared, call_end;
   int sec1, usec1, sec2, usec2, sec3, usec3;
   
   gettimeofday( &call_start, NULL );
   
   INIT_VARIABLE( returned_value );
   INIT_VARIABLE( function_value );
   
   code = (CODE*)function->pointer.memory_section;
   
   if ( function->args_nr )
     {
	locals = calloc( function->args_nr, sizeof( VALUE ) );
	
	/* Fill function arguments. */
	for ( i = 0; i < function->args_nr; i++ )
	  {
	     args = get_string( args, buf, 2048 );
	     if ( !buf[0] )
	       break;
	     
	     v = locals + i;
             V_TYPE( *v ) = VAR_STRING;
             V_STR( *v ) = strdup( buf );
	  }
     }
   
   /* Prepare memory. */
   if ( !prepare_code_call( code->local_symbol_table.memory_size, &function_value, &prev ) )
     {
	/* Copy locals. */
	// Fix meeee! I'm a huge performance problem!
	if ( locals )
	  memcpy( current_local_bottom, locals, function->args_nr * sizeof( VALUE ) );
	
	/* Prepared. Run! */
	
	gettimeofday( &call_prepared, NULL );
	
	i = instructions[code->first_instruction->instruction]( code->first_instruction, &returned_value );
	i = i && !returning_successful;
	returning_successful = 0;
	
	FREE_VARIABLE( returned_value );
	FREE_VARIABLE( function_value );
	
	if ( i )
	  clientf( "Failed!\r\n" );
	else
	  {
	     clientf( "Returned value: " );
	     if ( V_TYPE( function_value.value ) == VAR_STRING )
	       clientff( "%s\r\n", V_STR( function_value.value ) );
	     if ( V_TYPE( function_value.value ) == VAR_NUMBER )
	       clientff( "%d\r\n", V_NR( function_value.value ) );
	     if ( V_TYPE( function_value.value ) == VAR_POINTER )
	       clientff( "@%d-%d\r\n", V_POINTER( function_value.value ).memory_space,
			 V_POINTER( function_value.value ).offset );
	     
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



int run_code( CODE *code )
{
   VARIABLE returned_value, function_value;
   struct prev_data prev;
   int i;
   
   INIT_VARIABLE( returned_value );
   INIT_VARIABLE( function_value );
   
   /* Prepare memory. */
   if ( !prepare_code_call( code->local_symbol_table.memory_size, &function_value, &prev ) ) // &prev_bottom, &prev_top ) )
     {
	/* Prepared. Run! */
	
	i = instructions[code->first_instruction->instruction]( code->first_instruction, &returned_value );
	i = i && !returning_successful;
	returning_successful = 0;
	
	FREE_VARIABLE( returned_value );
	FREE_VARIABLE( function_value );
	
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
   VARIABLE pointer, *v, *locals = NULL, *top;
   struct prev_data prev;
   int i;
   
   INIT_VARIABLE( pointer );
   
//   clientff( "instr::%sfunction\r\n", instr->sys_function ? "(sys)" : "" );
   
   if ( instr->sys_function )
     return sys_functions[instr->pointer.offset].function( instr, returned_value );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &pointer ) )
     {
	FREE_VARIABLE( pointer );
	return 1;
     }
   
   if ( V_TYPE( pointer.value ) != VAR_POINTER ||
	V_POINTER( pointer.value ).memory_space != SCRIPT_FUNCTIONS ||
	V_POINTER( pointer.value ).offset < 0 )
     {
	clientf( C_R "*** Calling function from invalid pointer! ***\r\n" C_0 );
	FREE_VARIABLE( pointer );
	return 1;
     }
   
   code = (CODE*)V_POINTER( pointer.value ).memory_section;
   
   FREE_VARIABLE( pointer );
   INIT_VARIABLE( pointer );
   
   if ( code->local_symbol_table.memory_size )
     {
	locals = calloc( code->local_symbol_table.memory_size, sizeof( VARIABLE ) );
	top = locals + code->local_symbol_table.memory_size;
        v = locals;
	
	// Be careful when returning, it may have strings in it!
        
	/* Fill function's arguments. */
	for ( i = 1; i < instr->links; i++, v++ )
	  {
	     if ( v >= top )
	       {
		  clientf( C_R "*** Number of arguments passed exceeds function's local memory size! ***\r\n" C_0 );
		  free_vars( locals, code->local_symbol_table.memory_size );
		  return 1;
	       }
	     
	     if ( instructions[instr->link[i]->instruction]( instr->link[i], v ) )
	       {
		  free_vars( locals, code->local_symbol_table.memory_size );
		  return 1;
	       }
	  }
     }
   
   /* Prepare memory. */
   if ( !prepare_code_call( code->local_symbol_table.memory_size, returned_value, &prev ) )
     {
	/* Copy locals. */
        for ( i = 0; i < instr->links - 1; i++ )
          {
             FREE_VALUE( current_local_bottom[i] );
             current_local_bottom[i] = locals[i].value;
          }
        
	/* Prepared. Run! */
	
	i = instructions[code->first_instruction->instruction]( code->first_instruction, &pointer );
	i = i && !returning_successful;
	returning_successful = 0;
	
	FREE_VARIABLE( pointer );
	
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
   
   INIT_VARIABLE( devnull );
   
//   clientf( "instr::doall\r\n" );
   
   nr = instr->links;
   
   while ( i < nr )
     if ( instructions[instr->link[i]->instruction]( instr->link[i++], &devnull ) )
       {
          FREE_VARIABLE( devnull );
	  return 1;
       }
   
   FREE_VARIABLE( devnull );
   return 0;
}


int instr_operator( INSTR *instr, VARIABLE *returned_value )
{
   VARIABLE lvalue, rvalue;
   short i;
   
   INIT_VARIABLE( lvalue );
   
//   clientf( "instr::operator\r\n" );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &lvalue ) )
     {
	FREE_VARIABLE( lvalue );
	return 1;
     }
   
   /* Check for compatibility on lvalue. Irrelevant on some assignments. */
   if ( V_TYPE( lvalue.value ) &&
	!( operators[instr->oper].types & V_TYPE( lvalue.value ) ) &&
	!operators[instr->oper].assignment == 2 )
     {
	clientff( C_R "*** Invalid lvalue (%s) passed to operator '%s'! ***\r\n" C_0,
		  V_TYPE( lvalue.value ) == VAR_STRING ? "string" :
		  V_TYPE( lvalue.value ) == VAR_POINTER ? "pointer" :
		  V_TYPE( lvalue.value ) == VAR_NUMBER ? "number" : "unknown",
		  operators[instr->oper].symbol );
	FREE_VARIABLE( lvalue );
	return 1;
     }
   
   /* Unary operators. */
   if ( operators[instr->oper].unary )
     {
	i = operators[instr->oper].function( &lvalue, NULL, returned_value, instr );
	FREE_VARIABLE( lvalue );
	return i;
     }
   
   INIT_VARIABLE( rvalue );
   
   /* Binary operators. */
   if ( instructions[instr->link[1]->instruction]( instr->link[1], &rvalue ) )
     {
	FREE_VARIABLE( lvalue );
	FREE_VARIABLE( rvalue );
	return 1;
     }
   
   /* Check for compatibility on rvalue. */
   if ( V_TYPE( rvalue.value ) && !( operators[instr->oper].types & V_TYPE( rvalue.value ) ) )
     {
	clientff( C_R "*** Invalid rvalue (%s) passed to operator '%s'! ***\r\n" C_0,
		  V_TYPE( rvalue.value ) == VAR_STRING ? "string" :
		  V_TYPE( rvalue.value ) == VAR_POINTER ? "pointer" :
		  V_TYPE( rvalue.value ) == VAR_NUMBER ? "number" : "unknown",
		  operators[instr->oper].symbol );
	FREE_VARIABLE( lvalue );
	FREE_VARIABLE( rvalue );
	return 1;
     }
   
   i = operators[instr->oper].function( &lvalue, &rvalue, returned_value, instr );
   FREE_VARIABLE( lvalue );
   FREE_VARIABLE( rvalue );
   return i;
}


int instr_constant( INSTR *instr, VARIABLE *returned_value )
{
//   clientf( "instr::constant\r\n" );
   
   returned_value->value = instr->constant;
   if ( V_TYPE( returned_value->value ) == VAR_STRING )
     V_STR( returned_value->value ) = strdup( V_STR( returned_value->value ) );
   
   returned_value->address.memory_space = NULL_POINTER;
   
   return 0;
}


VALUE *dereference_pointer( POINTER pointer, SCRIPT_GROUP *group )
{
   VALUE *v;
   
   if ( pointer.memory_space == SCRIPT_MEMORY )
     {
	v = group->script_memory + pointer.offset;
	if ( v >= group->script_memory && v < group->script_memory_top )
	  return v;
	
	clientff( C_R "*** Segmentation Fault! ***\r\n" C_0 );
	clientff( C_R "*** Accessing script memory out of bounds ***\r\n" C_0 );
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
   else if ( pointer.memory_space == DYNAMIC_MEMORY )
     {
        if ( !pointer.memory_section )
          {
             clientff( C_R "*** Internal error: Pointer to dynamic memory has no section ***\r\n" C_0 );
             return NULL;
          }
        
        v = ((MEMORY*)pointer.memory_section)->memory + pointer.offset;
        if ( pointer.offset >= 0 && pointer.offset < ((MEMORY*)pointer.memory_section)->size )
          return v;
        
	clientff( C_R "*** Segmentation Fault! ***\r\n" C_0 );
	clientff( C_R "*** Accessing dynamic memory out of bounds ***\r\n" C_0 );
	return NULL;
     }
   else if ( pointer.memory_space == GLOBAL_MEMORY )
     {
	v = global_memory + pointer.offset;
	if ( v >= global_memory && v < global_memory + global_symbol_table.memory_size )
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
   VALUE *v;
   
//   clientf( "instr::variable\r\n" );
   
   /* Copy its value. */
   
   v = dereference_pointer( instr->pointer, instr->parent_script->parent_group );
   
   if ( !v )
     {
        if ( instr->symbol && instr->symbol->name )
          clientff( C_R "*** Attempt to access variable '%s' failed ***\r\n" C_0, instr->symbol->name );
        return 1;
     }
   
   copy_value( v, &returned_value->value );
   
   /* And also its address. */
   
   copy_address( instr->pointer, returned_value );
   
   return 0;
}


int instr_return( INSTR *instr, VARIABLE *returned_value )
{
//   clientf( "instr::return\r\n" );
   
   if ( instr->links )
     if ( instructions[instr->link[0]->instruction]( instr->link[0], function_return_value ) )
       return 1;
   
   returning_successful = 1;
   return 1;
}


int instr_if( INSTR *instr, VARIABLE *returned_value )
{
   VARIABLE var;
   short i;
   
   INIT_VARIABLE( var );
   
//   clientf( "instr::if\r\n" );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &var ) )
     {
	FREE_VARIABLE( var );
	return 1;
     }
   
   /* If true... */
   if ( V_TRUE( var.value ) )
     {
        FREE_VARIABLE( var );
        INIT_VARIABLE( var );
        i = instructions[instr->link[1]->instruction]( instr->link[1], &var );
     }
   /* Else, false. */
   else if ( instr->links == 3 )
     {
        FREE_VARIABLE( var );
        INIT_VARIABLE( var );
        i = instructions[instr->link[2]->instruction]( instr->link[2], &var );
     }
   else
     i = 0;
   
   FREE_VARIABLE( var );
   
   return i;
}



int instr_while( INSTR *instr, VARIABLE *returned_value )
{
   VARIABLE var;
   
   INIT_VARIABLE( var );
   
//   clientf( "instr::while\r\n" );
   
   /* Since it's a while.. put it in a while! */
   while ( 1 )
     {
        if ( instructions[instr->link[0]->instruction]( instr->link[0], &var ) )
          {
             FREE_VARIABLE( var );
             return 1;
          }
        
        /* If true... */
        if ( V_TRUE( var.value ) )
          {
             FREE_VARIABLE( var );
             INIT_VARIABLE( var );
             if ( instructions[instr->link[1]->instruction]( instr->link[1], &var ) )
               {
                  FREE_VARIABLE( var );
                  return 1;
               }
          }
        /* Else, goodbye. */
        else
          break;
     }
   
   FREE_VARIABLE( var );
   
   return 0;
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
   
   INIT_VARIABLE( value );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &value ) )
     return 1;
   
   if ( V_TYPE( value.value ) != VAR_STRING )
     {
	clientff( C_R "*** sysfunc_echo: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   clientf( V_STR( value.value ) );
   clientf( "\r\n" );
   FREE_VARIABLE( value );
   return 0;
}


SYSFUNC( sysfunc_send )
{
   VARIABLE value;
   char *buf, *p;
   
   INIT_VARIABLE( value );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &value ) )
     return 1;
   
   if ( V_TYPE( value.value ) != VAR_STRING )
     {
	clientff( C_R "*** sysfunc_send: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   /* Translate all semicolons into new lines. */
   buf = strdup( V_STR( value.value ) );
   for ( p = buf; *p; p++ )
     if ( *p == ';' )
       *p = '\n';
   
   send_to_server( buf );
   send_to_server( "\r\n" );
   free( buf );
   FREE_VARIABLE( value );
   return 0;
}


SYSFUNC( sysfunc_necho )
{
   VARIABLE value;
   
   INIT_VARIABLE( value );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &value ) )
     return 1;
   
   if ( V_TYPE( value.value ) != VAR_STRING )
     {
	clientff( C_R "*** sysfunc_necho: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   clientf( V_STR( value.value ) );
   FREE_VARIABLE( value );
   return 0;
}


SYSFUNC( sysfunc_nsend )
{
   VARIABLE value;
   char *buf, *p;
   
   INIT_VARIABLE( value );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &value ) )
     return 1;
   
   if ( V_TYPE( value.value ) != VAR_STRING )
     {
	clientff( C_R "*** sysfunc_nsend: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   /* Translate all semicolons into new lines. */
   buf = strdup( V_STR( value.value ) );
   for ( p = buf; *p; p++ )
     if ( *p == ';' )
       *p = '\n';
   
   send_to_server( buf );
   free( buf );
   FREE_VARIABLE( value );
   return 0;
}


SYSFUNC( sysfunc_arg )
{
   VARIABLE value;
   char *p;
   int nr, size;
   
   INIT_VARIABLE( value );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &value ) )
     return 1;
   
   if ( V_TYPE( value.value ) == VAR_NUMBER )
     {
	nr = V_NR( value.value );
     }
   else if ( V_TYPE( value.value ) == VAR_STRING )
     {
	nr = V_STR( value.value )[0] - '0';
     }
   else
     {
	clientff( C_R "*** sysfunc_arg: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   FREE_VARIABLE( value );
   
   V_TYPE( returns->value ) = VAR_STRING;
   
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
   
   V_STR( returns->value ) = malloc( size + 1 );
   memcpy( V_STR( returns->value ), p, size );
   V_STR( returns->value )[size] = 0;
   
   return 0;
}


SYSFUNC( sysfunc_args )
{
   VARIABLE value;
   char *p;
   int nr1, nr2, start_offset, end_offset, size;
   
   INIT_VARIABLE( value );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &value ) )
     return 1;
   
   nr1 = V_NR( value.value );
   FREE_VARIABLE( value );
   
   if ( V_TYPE( value.value ) != VAR_NUMBER )
     {
	clientff( C_R "*** sysfunc_args: first argument is invalid ***\r\n" C_0 );
	return 1;
     }
   
   if ( instructions[instr->link[1]->instruction]( instr->link[1], &value ) )
     return 1;
   
   nr2 = V_NR( value.value );
   FREE_VARIABLE( value );
   
   if ( V_TYPE( value.value ) != VAR_NUMBER )
     {
	clientff( C_R "*** sysfunc_args: second argument is invalid ***\r\n" C_0 );
	return 1;
     }
   
   V_TYPE( returns->value ) = VAR_STRING;
   
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
   
   V_STR( returns->value ) = malloc( size + 1 );
   memcpy( V_STR( returns->value ), p, size );
   V_STR( returns->value )[size] = 0;
   
   return 0;
}


SYSFUNC( sysfunc_show_prompt )
{
   show_prompt( );
   
   return 0;
}



inline void debug_show_value( VALUE *v, char *name, int i, char *c )
{
   const char mem_space[ ] = { 'n', 'r', 'a', 's', 'g', 'd', 'f' };
   
   clientff( "%s%2d (%s):" C_0 " " C_D, c, i, name );
   if ( V_TYPE( *v ) == VAR_NUMBER )
     clientff( "(n)" C_0 " %d\r\n", V_NR( *v ) );
   else if ( V_TYPE( *v ) == VAR_STRING )
     clientff( "(s)" C_0 " \"%s\" (%d)\r\n", V_STR( *v ), strlen( V_STR( *v ) ) );
   else if ( V_TYPE( *v ) == VAR_POINTER )
     clientff( "(p)" C_0 " @%c%d\r\n",
	       mem_space[V_POINTER( *v ).memory_space],
	       V_POINTER( *v ).offset );
   else
     clientff( "(uninitialized)\r\n" C_0 );
}

SYSFUNC( sysfunc_debug )
{
   SCRIPT_GROUP *group;
   SYMBOL *s;
   VALUE *v;
   int i;
   
   clientff( C_Y "\r\nGlobal Memory (%d variables)\r\n" C_0,
	     global_symbol_table.memory_size );
   
   for ( s = global_symbol_table.symbol; s; s = s->next )
     {
        if ( s->pointer.offset < 0 || s->pointer.offset >= global_symbol_table.memory_size )
          {
             clientff( C_R "%s out of bounds.\r\n" C_0, s->name );
             continue;
          }
        
        v = global_memory + s->pointer.offset;
	if ( V_TYPE( *v ) != VAR_NUMBER || V_NR( *v ) != 0 )
          debug_show_value( v, s->name, s->pointer.offset, C_Y );
     }
   
   group = instr->parent_script->parent_group;
   
   clientff( C_Y "\r\nScript Group Memory (%d variables)\r\n" C_0,
	     group->script_memory_top - group->script_memory );
   
   for ( s = group->group_symbol_table.symbol; s; s = s->next )
     {
        if ( s->pointer.offset < 0 || s->pointer.offset >= group->group_symbol_table.memory_size )
          {
             clientff( C_R "%s out of bounds.\r\n" C_0, s->name );
             continue;
          }
        
        v = group->script_memory + s->pointer.offset;
	if ( V_TYPE( *v ) != VAR_NUMBER || V_NR( *v ) != 0 )
          debug_show_value( v, s->name, s->pointer.offset, C_Y );
     }
   
   clientff( C_Y "\r\nLocal Memory (%d variables, %d total)\r\n" C_0,
	     current_local_top - current_local_bottom,
	     current_local_top - local_memory );
   
   for ( i = 0; i < 64; i++ )
     {
	v = local_memory + i;
	if ( v >= current_local_top )
	  break;
	
	if ( v >= current_local_bottom || V_TYPE( *v ) != VAR_NUMBER || V_NR( *v ) != 0 )
	  debug_show_value( v, "unknown", i, v >= current_local_bottom ? C_Y : C_D );
     }
   
   return 0;
}


SYSFUNC( sysfunc_nr )
{
   VARIABLE nr;
   
   INIT_VARIABLE( nr );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &nr ) )
     return 1;
   
   if ( V_TYPE( nr.value ) == VAR_NUMBER )
     {
	V_NR( returns->value ) = V_NR( nr.value );
     }
   else if ( V_TYPE( nr.value ) == VAR_STRING )
     {
	char *p;
	int i = 0, neg = 0;
	
	p = V_STR( nr.value );
	
	while ( *p == ' ' )
	  p++;
	
	if ( *p == '-' )
	  neg = 1;
	
	while ( *p >= '0' && *p <= '9' )
	  i *= 10, i += *p - '0', p++;
	
	if ( neg )
	  i = -i;
	
	V_NR( returns->value ) = i;
     }
   else
     V_NR( returns->value ) = 0;
   
   FREE_VARIABLE( nr );
   
   V_TYPE( returns->value ) = VAR_NUMBER;
   
   return 0;
}


SYSFUNC( sysfunc_str )
{
   VARIABLE str;
   
   INIT_VARIABLE( str );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &str ) )
     return 1;
   
   if ( V_TYPE( str.value ) == VAR_STRING )
     {
	V_STR( returns->value ) = strdup( V_STR( str.value ) );
     }
   else if ( V_TYPE( str.value ) == VAR_NUMBER )
     {
	// Optimize me!
	
	char buf[256];
	
	sprintf( buf, "%d", V_NR( str.value ) );
	
	V_STR( returns->value ) = strdup( buf );
     }
   else
     V_STR( returns->value ) = strdup( "(null)" );
   
   FREE_VARIABLE( str );
   
   V_TYPE( returns->value ) = VAR_STRING;
   
   return 0;
}



SYSFUNC( sysfunc_memory )
{
   MEMORY *memory;
   VARIABLE var;
   SCRIPT_GROUP *g;
   
   INIT_VARIABLE( var );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &var ) )
     return 1;
   
   if ( V_TYPE( var.value ) != VAR_NUMBER )
     {
	clientff( C_R "*** sysfunc_memory: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   if ( V_NR( var.value ) < 1 || V_NR( var.value ) > 4096 )
     {
        clientff( C_R "*** sysfunc_memory: %d is too %s ***\r\n" C_0,
                  V_NR( var.value ), V_NR( var.value ) < 0 ? "low" : "high" );
        return 1;
     }
   
   memory = malloc( sizeof( MEMORY ) );
   memory->size = V_NR( var.value );
   memory->memory = calloc( V_NR( var.value ), sizeof( VALUE ) );
   g = instr->parent_script->parent_group;
   link_to_list( memory, g->dynamic_memory, g->dm_last );
   
   V_TYPE( returns->value ) = VAR_POINTER;
   V_POINTER( returns->value ).memory_space = DYNAMIC_MEMORY;
   V_POINTER( returns->value ).offset = 0;
   V_POINTER( returns->value ).memory_section = (void *)memory;
   
   return 0;
}



SYSFUNC( sysfunc_sizeof )
{
   VARIABLE var;
   
   INIT_VARIABLE( var );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &var ) )
     return 1;
   
   if ( V_TYPE( var.value ) != VAR_POINTER &&
        V_POINTER( var.value ).memory_space != DYNAMIC_MEMORY )
     {
	clientff( C_R "*** sysfunc_memory: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   V_TYPE( returns->value ) = VAR_NUMBER;
   V_NR( returns->value ) = ((MEMORY *) V_POINTER( var.value ).memory_section)->size;
   
   return 0;
}



SYSFUNC( sysfunc_resize )
{
   VARIABLE p, size;
   MEMORY *memory;
   int last;
   
   INIT_VARIABLE( p );
   INIT_VARIABLE( size );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &p ) )
     return 1;
   
   if ( V_TYPE( p.value ) != VAR_POINTER &&
        V_POINTER( p.value ).memory_space != DYNAMIC_MEMORY )
     {
	clientff( C_R "*** sysfunc_memory: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   if ( instructions[instr->link[1]->instruction]( instr->link[1], &size ) )
     return 1;
   
   if ( V_TYPE( size.value ) != VAR_NUMBER )
     {
	clientff( C_R "*** sysfunc_memory: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   if ( V_NR( size.value ) < 1 || V_NR( size.value ) > 4096 )
     {
        clientff( C_R "*** sysfunc_memory: %d is too %s ***\r\n" C_0,
                  V_NR( size.value ), V_NR( size.value ) < 0 ? "low" : "high" );
        return 1;
     }
   
   memory = (MEMORY *) V_POINTER( p.value ).memory_section;
   
   last = memory->size;
   memory->size = V_NR( size.value );
   memory->memory = realloc( memory->memory, V_NR( size.value ) * sizeof( VALUE ) );
   if ( last < memory->size )
     {
        int i;
        
        for ( i = last; i < memory->size; i++ )
          {
             INIT_VALUE( *(memory->memory + i) );
          }
     }
   
   V_TYPE( returns->value ) = VAR_POINTER;
   V_POINTER( returns->value ).memory_space = DYNAMIC_MEMORY;
   V_POINTER( returns->value ).offset = 0;
   V_POINTER( returns->value ).memory_section = (void *)memory;
   
   return 0;
}



SYSFUNC( sysfunc_get_mb_variable )
{
   VARIABLE var;
   
   INIT_VARIABLE( var );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &var ) )
     return 1;
   
   if ( V_TYPE( var.value ) != VAR_STRING )
     {
	clientff( C_R "*** sysfunc_get_mb_variable: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   V_NR( returns->value ) = 0;
   
   if ( !strcmp( V_STR( var.value ), "a_hp" ) )
     {
        static int *a_hp;
        if ( !a_hp )
          a_hp = get_variable( "a_hp" );
        if ( a_hp )
          V_NR( returns->value ) = *a_hp / 11;
     }
   else if ( !strcmp( V_STR( var.value ), "a_hp" ) )
     {
        static int *a_hp;
        if ( !a_hp )
          a_hp = get_variable( "a_hp" );
        if ( a_hp )
          V_NR( returns->value ) = *a_hp / 11;
     }
   else if ( !strcmp( V_STR( var.value ), "a_max_hp" ) )
     {
        static int *a_max_hp;
        if ( !a_max_hp )
          a_max_hp = get_variable( "a_max_hp" );
        if ( a_max_hp )
          V_NR( returns->value ) = *a_max_hp / 11;
     }
   else if ( !strcmp( V_STR( var.value ), "a_mana" ) )
     {
        static int *a_mana;
        if ( !a_mana )
          a_mana = get_variable( "a_mana" );
        if ( a_mana )
          V_NR( returns->value ) = *a_mana / 11;
     }
   else if ( !strcmp( V_STR( var.value ), "a_max_mana" ) )
     {
        static int *a_max_mana;
        if ( !a_max_mana )
          a_max_mana = get_variable( "a_max_mana" );
        if ( a_max_mana )
          V_NR( returns->value ) = *a_max_mana / 11;
     }
   else if ( !strcmp( V_STR( var.value ), "a_exp" ) )
     {
        static int *a_exp;
        if ( !a_exp )
          a_exp = get_variable( "a_exp" );
        if ( a_exp )
          V_NR( returns->value ) = *a_exp;
     }
   else
     {
        int *a;
        a = get_variable( V_STR( var.value ) );
        if ( a )
          V_NR( returns->value ) = *a;
     }
   
   FREE_VARIABLE( var );
   V_TYPE( returns->value ) = VAR_NUMBER;
   
   return 0;
}



SYSFUNC( sysfunc_replace_line )
{
   VARIABLE var;
   
   INIT_VARIABLE( var );
   
   if ( instructions[instr->link[0]->instruction]( instr->link[0], &var ) )
     return 1;
   
   if ( V_TYPE( var.value ) != VAR_STRING )
     {
	clientff( C_R "*** sysfunc_get_mb_variable: invalid argument ***\r\n" C_0 );
	return 1;
     }
   
   replace( V_STR( var.value ) );
   
   return 0;
}




/** Operators. **/

#define OPER( oper ) int (oper)( VARIABLE *lvalue, VARIABLE *rvalue, VARIABLE *returns, INSTR *instr )

OPER( oper_not_implemented )
{
   clientff( C_R "*** Operator not implemented! ***\r\n" C_0 );
   
//   debugf( "Executing an operator that is not yet implemented." );
   
   return 1;
}


OPER( oper_dereference )
{
   VALUE *v;
   
   if ( V_TYPE( lvalue->value ) != VAR_POINTER )
     {
	clientf( C_R "*** Invalid operand passed to operator '*' ***\r\n" C_0 );
        clientff( "Passed type %d, r is %s.\n", V_TYPE( lvalue->value ), V_STR( lvalue->value ) );
	return 1;
     }
   
   v = dereference_pointer( V_POINTER( lvalue->value ), instr->parent_script->parent_group );
   
   if ( !v )
     return 1;
   
   copy_value( v, &returns->value );
   copy_address( V_POINTER( lvalue->value ), returns );
   
   return 0;
}


OPER( oper_address_of )
{
   if ( lvalue->address.memory_space == NULL_POINTER )
     {
	clientf( C_R "*** Invalid operand passed to operator '&' ***\r\n" C_0 );
	return 1;
     }
   
   V_TYPE( returns->value ) = VAR_POINTER;
   if ( lvalue->address.memory_space == RELATIVE_LOCAL_MEMORY )
     {
	V_POINTER( returns->value ).memory_space = ABSOLUTE_LOCAL_MEMORY;
	V_POINTER( returns->value ).offset = lvalue->address.offset + ( current_local_bottom - local_memory );
     }
   else
     {
	V_POINTER( returns->value ).memory_space = lvalue->address.memory_space;
	V_POINTER( returns->value ).offset = lvalue->address.offset;
     }
   
   return 0;
}


OPER( oper_assignment )
{
   VALUE *v;
   
   v = dereference_pointer( lvalue->address, instr->parent_script->parent_group );
   
   if ( !v )
     return 1;
   
   copy_value( &rvalue->value, v );
   
   copy_value( &rvalue->value, &returns->value );
   copy_address( V_POINTER( lvalue->value ), returns );
   
   return 0;
}


OPER( oper_multiplication )
{
   V_TYPE( returns->value ) = VAR_NUMBER;
   
   V_NR( returns->value ) = V_NR( lvalue->value ) * V_NR( rvalue->value );
   
   return 0;
}


OPER( oper_division )
{
   V_TYPE( returns->value ) = VAR_NUMBER;
   
   V_NR( returns->value ) = V_NR( lvalue->value ) / V_NR( rvalue->value );
   
   return 0;
}


OPER( oper_modulus )
{
   V_TYPE( returns->value ) = VAR_NUMBER;
   
   V_NR( returns->value ) = V_NR( lvalue->value ) % V_NR( rvalue->value );
   
   return 0;
}


OPER( oper_addition )
{
   if ( V_TYPE( lvalue->value ) != V_TYPE( rvalue->value ) )
     {
	/* Consider uninitialized variables as empty strings. */
	if ( V_TYPE( lvalue->value ) == VAR_STRING && V_TYPE( rvalue->value ) == VAR_NUMBER &&
	     V_NR( rvalue->value ) == 0 )
	  {
	     V_TYPE( returns->value ) = VAR_STRING;
	     V_STR( returns->value ) = strdup( V_STR( lvalue->value ) );
	     return 0;
	  }
	else if ( V_TYPE( rvalue->value ) == VAR_STRING && V_TYPE( lvalue->value ) == VAR_NUMBER &&
		  V_NR( lvalue->value ) == 0 )
	  {
	     V_TYPE( returns->value ) = VAR_STRING;
	     V_STR( returns->value ) = strdup( V_STR( rvalue->value ) );
	     return 0;
	  }
        else if ( V_TYPE( lvalue->value ) == VAR_POINTER && V_TYPE( rvalue->value ) == VAR_NUMBER )
          {
             V_TYPE( returns->value ) = VAR_POINTER;
             V_POINTER( returns->value ) = V_POINTER( lvalue->value );
             V_POINTER( returns->value ).offset += V_NR( rvalue->value );
             return 0;
          }
        else if ( V_TYPE( rvalue->value ) == VAR_POINTER && V_TYPE( lvalue->value ) == VAR_NUMBER )
          {
             V_TYPE( returns->value ) = VAR_POINTER;
             V_POINTER( returns->value ) = V_POINTER( rvalue->value );
             V_POINTER( returns->value ).offset += V_NR( lvalue->value );
             return 0;
          }
	
	clientf( C_R "*** Invalid operands passed to operator '+' ***\r\n" C_0 );
	return 1;
     }
   
   if ( V_TYPE( lvalue->value ) == VAR_NUMBER )
     {
	V_TYPE( returns->value ) = VAR_NUMBER;
	V_NR( returns->value ) = V_NR( lvalue->value ) + V_NR( rvalue->value );
	
	return 0;
     }
   
   if ( V_TYPE( lvalue->value ) == VAR_STRING )
     {
	V_TYPE( returns->value ) = VAR_STRING;
	
	// Not nice. Too many external functions called.
	V_STR( returns->value ) = calloc( 1, strlen( V_STR( lvalue->value ) ) + strlen( V_STR( rvalue->value ) ) + 1 );
	strcpy( V_STR( returns->value ), V_STR( lvalue->value ) );
	strcat( V_STR( returns->value ), V_STR( rvalue->value ) );
	
	return 0;
     }
   
   if ( V_TYPE( lvalue->value ) == VAR_POINTER )
     {
        clientf( C_R "*** Invalid operands passed to operator '+' ***\r\n" C_0 );
        return 1;
     }
   
   clientf( C_R "*** Internal error: unknown variable type ***\r\n" C_0 );
   return 1;
}


OPER( oper_substraction )
{
   if ( V_TYPE( lvalue->value ) == VAR_NUMBER && V_TYPE( rvalue->value ) == VAR_NUMBER )
     {
        V_TYPE( returns->value ) = VAR_NUMBER;
        
        V_NR( returns->value ) = V_NR( lvalue->value ) - V_NR( rvalue->value );
     }
   
   if ( V_TYPE( lvalue->value ) == VAR_POINTER && V_TYPE( rvalue->value ) == VAR_POINTER )
     {
        clientf( C_R "*** Invalid operands passed to operator '-' ***\r\n" C_0 );
        return 1;
     }
   
   if ( V_TYPE( lvalue->value ) == VAR_POINTER )
     {
        V_TYPE( returns->value ) = VAR_POINTER;
        V_POINTER( returns->value ) = V_POINTER( lvalue->value );
        V_POINTER( returns->value ).offset += V_NR( rvalue->value );
     }
   
   else if ( V_TYPE( rvalue->value ) == VAR_POINTER )
     {
        V_TYPE( returns->value ) = VAR_POINTER;
        V_POINTER( returns->value ) = V_POINTER( rvalue->value );
        V_POINTER( returns->value ).offset += V_NR( lvalue->value );
     }
   
   return 0;
}


OPER( oper_equal_to )
{
   V_TYPE( returns->value ) = VAR_NUMBER;
   
   if ( V_TYPE( lvalue->value ) != V_TYPE( rvalue->value ) )
     {
	V_NR( returns->value ) = 0;
     }
   else if ( V_TYPE( lvalue->value ) == VAR_NUMBER )
     {
	V_NR( returns->value ) = ( V_NR( lvalue->value ) == V_NR( rvalue->value ) );
     }
   else if ( V_TYPE( lvalue->value ) == VAR_POINTER )
     {
	// watch out for relative vs absolute
	V_NR( returns->value ) = ( ( V_POINTER( lvalue->value ).memory_space == V_POINTER( rvalue->value ).memory_space ) &&
		       ( V_POINTER( lvalue->value ).offset == V_POINTER( rvalue->value ).offset ) );
     }
   else if ( V_TYPE( lvalue->value ) == VAR_STRING )
     {
	V_NR( returns->value ) = !strcmp( V_STR( lvalue->value ), V_STR( rvalue->value ) );
     }
   
   return 0;
}


OPER( oper_not_equal_to )
{
   oper_equal_to( lvalue, rvalue, returns, instr );
   
   V_NR( returns->value ) = !V_NR( returns->value );
   
   return 0;
}


OPER( oper_less_than )
{
   if ( V_TYPE( lvalue->value ) != V_TYPE( rvalue->value ) )
     {
	clientff( C_R "*** Invalid operands passed to operator '<' ***\r\n" C_0 );
	return 1;
     }
   
   V_TYPE( returns->value ) = VAR_NUMBER;
   
   if ( V_TYPE( lvalue->value ) == VAR_POINTER )
     {
	if ( V_POINTER( lvalue->value ).memory_space != V_POINTER( rvalue->value ).memory_space )
	  V_NR( returns->value ) = 0;
	else
	  V_NR( returns->value ) = V_POINTER( lvalue->value ).offset < V_POINTER( rvalue->value ).offset;
	return 0;
     }
   
   /* VAR_NUMBER. */
   V_NR( returns->value ) = V_NR( lvalue->value ) < V_NR( rvalue->value );
   return 0;
}


OPER( oper_greater_than )
{
   if ( V_TYPE( lvalue->value ) != V_TYPE( rvalue->value ) )
     {
	clientff( C_R "*** Invalid operands passed to operator '>' ***\r\n" C_0 );
	return 1;
     }
   
   V_TYPE( returns->value ) = VAR_NUMBER;
   
   if ( V_TYPE( lvalue->value ) == VAR_POINTER )
     {
	if ( V_POINTER( lvalue->value ).memory_space != V_POINTER( rvalue->value ).memory_space )
	  V_NR( returns->value ) = 0;
	else
	  V_NR( returns->value ) = V_POINTER( lvalue->value ).offset > V_POINTER( rvalue->value ).offset;
	return 0;
     }
   
   /* VAR_NUMBER. */
   V_NR( returns->value ) = V_NR( lvalue->value ) > V_NR( rvalue->value );
   return 0;
}


OPER( oper_less_than_or_equal_to )
{
   if ( V_TYPE( lvalue->value ) != V_TYPE( rvalue->value ) )
     {
	clientff( C_R "*** Invalid operands passed to operator '<=' ***\r\n" C_0 );
	return 1;
     }
   
   V_TYPE( returns->value ) = VAR_NUMBER;
   
   if ( V_TYPE( lvalue->value ) == VAR_POINTER )
     {
	if ( V_POINTER( lvalue->value ).memory_space != V_POINTER( rvalue->value ).memory_space )
	  V_NR( returns->value ) = 0;
	else
	  V_NR( returns->value ) = V_POINTER( lvalue->value ).offset <= V_POINTER( rvalue->value ).offset;
	return 0;
     }
   
   /* VAR_NUMBER. */
   V_NR( returns->value ) = V_NR( lvalue->value ) <= V_NR( rvalue->value );
   return 0;
}


OPER( oper_greater_than_or_equal_to )
{
   if ( V_TYPE( lvalue->value ) != V_TYPE( rvalue->value ) )
     {
	clientff( C_R "*** Invalid operands passed to operator '>=' ***\r\n" C_0 );
	return 1;
     }
   
   V_TYPE( returns->value ) = VAR_NUMBER;
   
   if ( V_TYPE( lvalue->value ) == VAR_POINTER )
     {
	if ( V_POINTER( lvalue->value ).memory_space != V_POINTER( rvalue->value ).memory_space )
	  V_NR( returns->value ) = 0;
	else
	  V_NR( returns->value ) = V_POINTER( lvalue->value ).offset >= V_POINTER( rvalue->value ).offset;
	return 0;
     }
   
   /* VAR_NUMBER. */
   V_NR( returns->value ) = V_NR( lvalue->value ) >= V_NR( rvalue->value );
   return 0;
}


OPER( oper_not )
{
   V_TYPE( returns->value ) = V_TYPE( lvalue->value );
   
   if ( V_TYPE( lvalue->value ) == VAR_NUMBER )
     V_NR( returns->value ) = !V_NR( lvalue->value );
   else if ( V_TYPE( lvalue->value ) == VAR_STRING )
     V_STR( returns->value ) = strdup( V_STR( lvalue->value )[0] ? "" : "true" );
   
   return 0;
}


OPER( oper_logical_and )
{
   V_TYPE( returns->value ) = VAR_NUMBER;
   V_NR( returns->value ) = V_TRUE( lvalue->value ) && V_TRUE( rvalue->value );
   
   return 0;
}


OPER( oper_logical_or )
{
   V_TYPE( returns->value ) = VAR_NUMBER;
   V_NR( returns->value ) = V_TRUE( lvalue->value ) || V_TRUE( rvalue->value );
   
   return 0;
}


