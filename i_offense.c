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


/* Imperian offensive system. */

#define I_OFFENSE_ID "$Name$ $Id$"

#include <stdarg.h>
#include <sys/time.h>
#include <pcre.h>

#include "module.h"


int offensive_version_major = 0;
int offensive_version_minor = 7;

char *i_offense_id = I_OFFENSE_ID "\r\n" HEADER_ID "\r\n" MODULE_ID "\r\n";


/* Running script. */
char **pos;

/* Chrono timer. */
char *string_start;
char *string_stop;
struct timeval start_time;

int silent;

/* Body parts. */

#define GENERAL 0
#define LEFT_LEG 1
#define RIGHT_LEG 2
#define LEFT_ARM 3
#define RIGHT_ARM 4
#define HEAD 5
#define TORSO 6
#define PARTS 7


/* Sync combo. */
int hit_counter[PARTS][12];
int last_hit, last_hit_location;

/* Stances. */

int stance;

/*                     Nrm  Hrs  Egs  Cts  Brs  Rts  Scs  Dgs */
int kick_damage[]  = { 100,  90,  90,  70,  60,  85, 130, 125 };
int punch_damage[] = { 100, 110,  80,  60, 140,  70, 100, 125 };


/* Structures that we'll need. */

typedef struct char_data CHAR_DATA;
typedef struct script_data SCRIPT;
typedef struct function_data FUNCTION;
typedef struct call_data CALL;
typedef struct variable_data VARIABLE;
typedef struct trigger_data TRIGGER;
typedef struct alias_data ALIAS_DATA;

struct char_data
{
   char *name;
   
   int max_health;
   int last_health;
   
   int arm_health;
   int leg_health;
   int head_health;
   
   int limit;
   
   /* Try to auto-estimate. */
   int estimate;
   
   /* Estimating damage done to them. */
   int min_estimate;
   int max_estimate;
   
   /* Body part damage. */
   int body[PARTS];
   
   /* Sync combo info. */
   int sync_parts[PARTS];
   int parried_part;
   int maybe_parried;
   int extra_hits[PARTS];
   
   CHAR_DATA *next;
};


/* A script is a piece of memory containing functions, usually loaded
 * from a file. This is not something that can be directly executed,
 * only the blocks within the functions within, can be. */
struct script_data
{
   char *name;
   char *description;
   
   FUNCTION *functions;
   TRIGGER *triggers;
   
   SCRIPT *next;
};



struct function_data
{
   char *name;
   char *body;
   short alias;
   char **args;
   int size;
   
   FUNCTION *next;
};



/* When a function is called, one such structure is created in the call-stack. */
struct call_data
{
   CALL *below;
   
   /* Pointers to where it belongs, not to be freed. */
   char *name;
   SCRIPT *script;
   
   /* Variables that will be destroyed when function returns. */
   /* They include the arguments passed to it. */
   VARIABLE *locals;
   
   /* Current position in the function. */
   char *position;
   
   /* Where to store the value it returns, and how big the buffer is. */
   char *return_value;
   int max;
   
   /* Was there an error here? */
   int aborted;
};



struct variable_data
{
   char *name;
   
   char *value;
   
   VARIABLE *next;
};


struct alias_data
{
   char *name;
   
   char *action;
   
   ALIAS_DATA *next;
};


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
   
   char *body;
   int size;
   
   TRIGGER *next;
};


ALIAS_DATA *aliases;
VARIABLE *variables;
CHAR_DATA *characters;
TRIGGER *triggers;
SCRIPT *scripts;
CALL *call_stack;


/* Common/special variables. */

char *alias_arg[10]; /* $1, $5, etc. */
char *alias_fullarg; /* $* */
char *target; /* $t/$tar/$target */

int regex_callbacks;
int regex_ovector[30];
char *regex_line;

int parsing_whohere_list;
char wholist_targets[32][256];

#define SB_FULL_SIZE 40960
#define SB_HALF_SIZE SB_FULL_SIZE / 2

char searchback_buffer[SB_FULL_SIZE];
int sb_size;

CALL *prepare_block_as_call( char *name, char *body );
void call_function( CALL *call, char *returns, int max );
void execute_block( );
void execute_single_command( char *returns, int max, int priority );


/* Here we register our functions. */

void offensive_module_init_data( );
void offensive_process_server_line( LINE *l );
void offensive_process_server_prompt( LINE *l );
int  offensive_process_client_command( char *cmd );
int  offensive_process_client_aliases( char *cmd );


ENTRANCE( offensive_module_register )
{
   self->name = strdup( "IOffense" );
   self->version_major = offensive_version_major;
   self->version_minor = offensive_version_minor;
   self->id = i_offense_id;
   
   self->init_data = offensive_module_init_data;
   self->process_server_line = offensive_process_server_line;
   self->process_server_prompt = offensive_process_server_prompt;
   self->process_client_command = NULL;
   self->process_client_aliases = offensive_process_client_aliases;
   self->build_custom_prompt = NULL;
   self->main_loop = NULL;
   self->update_descriptors = NULL;
   
   GET_FUNCTIONS( self );
#if defined( FOR_WINDOWS )
   gettimeofday = self->get_func( "gettimeofday" );
#endif
}



void hit_character( CHAR_DATA *ch, int part )
{
   ch->body[part] += punch_damage[stance];
   
   clientff( "\r\n" C_W "Damage" C_0 ":  LL: " C_G "%d" C_0 "  LA: " C_G "%d" C_0
	     "  RA: " C_G "%d" C_0 "  RL: " C_G "%d" C_0,
	     ch->body[LEFT_LEG], ch->body[LEFT_ARM],
	     ch->body[RIGHT_ARM], ch->body[RIGHT_LEG] );
}

void check_hits_counter( char *string )
{
   static char name[256];
   char *hit_string[ ] = 
     { "none", "jbp", "snk", "hkp", "sdk", "ucp", "hfp",
	  "rhk", "mnk", "spp", "axk", "wwk" };
   char *pbuf, *pname = NULL;
   int first = 0, i;
//   int punch[ ] = { 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0 };
   
   DEBUG( "check_hits_counter" );
   
   /* Reset, it needed. */
   if ( !cmp( "You have slain *.", string ) )
     {
	int j;
	
	for ( i = 0; i < 12; i++ )
	  for ( j = 0; j < PARTS; j++ )
	    hit_counter[j][i] = 0;
	return;
     }
   
   /* What did we hit with? */
   if ( !cmp( "You lash out with a quick jab at *", string ) )
     last_hit = 1;
   else if ( !cmp( "You let fly at * with a snap kick.", string ) )
     last_hit = 2, pname = string + 15;
   else if ( !cmp( "You unleash a powerful hook towards *", string ) )
     last_hit = 3;
   else if ( !cmp( "You pump out at * with a powerful side kick.", string ) )
     last_hit = 4;
   else if ( !cmp( "You launch a powerful uppercut at *", string ) )
     last_hit = 5;
   else if ( !cmp( "You ball up one fist and hammerfist *", string ) )
     last_hit = 6, pname = string + 36;
   else if ( !cmp( "You twist your torso and send a roundhouse towards *", string ) )
     last_hit = 7;
   else if ( !cmp( "You hurl yourself towards * with a lightning-fast moon ", string ) )
     last_hit = 8, pname = string + 26;
   else if ( !cmp( "You hurl yourself towards * with a lightning-fast moon kick.", string ) )
     last_hit = 8, pname = string + 26;
   else if ( !cmp( "You form a spear hand and stab out towards *", string ) )
     last_hit = 9, pname = string + 43;
   else if ( !cmp( "You kick your leg high and scythe downwards at *", string ) )
     last_hit = 10;
   else if ( !cmp( "You spin into the air and throw a whirlwind kick towards *", string ) )
     last_hit = 11;
   else if ( !last_hit )
     return;
   
   if ( pname )
     {
	get_string( pname, name, 256 );
	for ( pname = name; *pname; pname++ )
	  if ( *pname == '.' )
	    {
	       *pname = 0;
	       break;
	    }
	return;
     };
   
   /* And where did that hit go? */
   if ( !strcmp( string, "You connect!" ) )
     last_hit_location = GENERAL, pbuf = "";
   else if ( !strcmp( string, "You connect to the head!" ) )
     last_hit_location = HEAD, pbuf = "Head: ";
   else if ( !strcmp( string, "You connect to the torso!" ) )
     last_hit_location = TORSO, pbuf = "Torso: ";
   else if ( !strcmp( string, "You connect to the left arm!" ) )
     last_hit_location = LEFT_ARM, pbuf = "Left arm: ";
   else if ( !strcmp( string, "You connect to the right arm!" ) )
     last_hit_location = RIGHT_ARM, pbuf = "Right arm: ";
   else if ( !strcmp( string, "You connect to the left leg!" ) )
     last_hit_location = LEFT_LEG, pbuf = "Left leg: ";
   else if ( !strcmp( string, "You connect to the right leg!" ) )
     last_hit_location = RIGHT_LEG, pbuf = "Right leg: ";
   else
     {
	return;
     }
   
   /* So we now know what kick/punch we used, and where it went. */
   
   if ( last_hit_location != GENERAL )
     {
	CHAR_DATA *ch;
	
	for ( ch = characters; ch; ch = ch->next )
	  if ( !strcmp( ch->name, name ) )
	    break;
	
	if ( ch )
	  {
	     hit_character( ch, last_hit_location );
	     return;
	  }
     }
   
   hit_counter[last_hit_location][last_hit]++;
   
   clientf( C_R " (" C_B );
   clientf( pbuf );
   for ( i = 1; i < 12; i++ )
     {
	if ( !hit_counter[last_hit_location][i] )
	  continue;
	
	if ( first++ )
	  clientf( " " );
	clientff( C_G "%d" C_B "%s", hit_counter[last_hit_location][i],
		  hit_string[i] );
     }
   clientf( C_R ")" C_0 );
}



void show_part_hits( )
{
   char *hit_string[ ] = 
     { "none", "jbp", "snk", "hkp", "sdk", "ucp", "hfp",
	  "rhk", "mnk", "spp", "axk", "wwk" };
   char *parts[ ] = 
     { "General:  ", "Left Leg: ", "Right Leg:", "Left Arm: ", "Right Arm:",
	  "Head:     ", "Torso:    " };
   char buf[256];
   int i, j;
   
   clientf( C_R "[Hits so far on body parts:]\r\n" C_0 );
   
   for ( i = 0; i < PARTS; i++ )
     {
	int first = 0;
	
	clientff( C_0 " %s  ", parts[i] );
	
	for ( j = 1; j < 12; j++ )
	  {
	     if ( !hit_counter[i][j] )
	       continue;
	     
	     if ( first++ )
	       clientf( " " );
	     sprintf( buf, C_G "%d" C_B "%s", hit_counter[i][j], hit_string[j] );
	     clientf( buf );
	  }
	
	if ( !first )
	  clientf( C_B "None" );
	
	clientf( C_0 "\r\n" );
     }
}



void caiman_and_creeper_targetting( char *line )
{
   const int aggressive_mobs[ ] = 
     {
	/* Caiman: */ 12878, 15212, 18360, 19914, 25486, 27740, 31539, 34575,
	/* Creepers: */ 39025, 14495, 48257, 49304, 40287, 25243, 52827, 39192, 51223, 3623, 52275, 0 };
   static int mob;
   static int aggressive_mob;
   static int last_mob;
   char name[1024];
   int nr, i;
   
   if ( mob && !cmp( "Number of objects: *", line ) )
     {
	if ( aggressive_mob )
	  mob = aggressive_mob;
	
	prefixf( "Target: %s%d%s\r\n" C_0, aggressive_mob ? C_R : C_G,
		 mob, mob == last_mob ? C_W " *" : "" );
	
	sprintf( name, "%d", mob );
	free( target );
	target = strdup( name );
	
	last_mob = mob;
	mob = 0;
	aggressive_mob = 0;
	
	return;
     }
   
   if ( line[0] != '\"' )
     return;
   
   line = get_string( line, name, 1024 );
   
   if ( ( cmp( "caiman*", name ) || cmp( "a hungry caiman", line ) || !( nr = atoi( name + 6 ) ) ) &&
	( cmp( "creeper*", name ) || cmp( "a carrion creeper", line ) || !( nr = atoi( name + 7 ) ) ) )
     return;
   
   /* In case the last one is not dead yet. */
   if ( !last_mob || mob != last_mob )
     mob = nr;
   
   for ( i = 0; aggressive_mobs[i]; i++ )
     if ( nr == aggressive_mobs[i] )
       {
	  if ( mob == nr )
	    aggressive_mob = nr;
	  prefix( C_R );
	  suffix( C_0 );
       }
}



void exec_trigger( TRIGGER *trigger )
{
   CALL *call;
   
   call = prepare_block_as_call( trigger->name ? trigger->name : "trigger", trigger->body );
   call_function( call, NULL, 0 );
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
	       regex_line = line;
	       
	       exec_trigger( trigger );
	       
	       regex_callbacks = 0;
	    }
	  
	  else if ( !cmp( trigger->message, line ) )
	    {
	       exec_trigger( trigger );
	    }
       }
}


void offensive_process_server_line( LINE *l )
{
   void offensive_process_server_line_suffix( char *colorless_line, char *colorful_line, char *raw_line );
   
   DEBUG( "offensive_process_server_line" );
   
   if ( !cmp( "^ rubs some salve on ^ ^.", l->line ) &&
	cmp( "^ rubs some salve on ^ skin.", l->line ) &&
	cmp( "^ rubs some salve on ^ body.", l->line ) )
     {
	prefix( C_B );
	suffix( C_0 );
     }
   
   if ( !cmp( "^ touches a tree of life tattoo.", l->line ) )
     {
	prefix( C_Y );
	suffix( C_0 );
     }
   
   if ( !cmp( "^ stands up and stretches ^ arms out wide.", l->line ) )
     {
	prefix( C_W );
	suffix( C_0 );
     }
   
   if ( !cmp( "^ slowly pulls back *", l->line ) )
     {
	prefix( C_W );
	suffix( C_0 );
     }
   
   caiman_and_creeper_targetting( l->line );
   
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
   
   offensive_process_server_line_suffix( l->line, l->raw_line, l->raw_line );
}


void check_timer( char *line )
{
   if ( string_stop && strstr( line, string_stop ) )
     {
	struct timeval stop_time;
	int sec, usec;
	
	gettimeofday( &stop_time, NULL );
	
	sec = stop_time.tv_sec - start_time.tv_sec;
	usec = stop_time.tv_usec - start_time.tv_usec;
	
	if ( usec < 0 )
	  sec -= 1, usec += 1000000;
	
	clientff( C_W " (" C_D "%d.%06d" C_W ")" C_0, sec, usec );
     }
   
   if ( string_start && strstr( line, string_start ) )
     {
	gettimeofday( &start_time, NULL );
	clientf( C_W " (" C_D "set" C_W ")" C_0 );
     }
}



void break_estimate( int health, int dmg )
{
   float needed;
   int damage;
   
   clientff( C_R "[Break estimate for " C_G "%d" C_R ".]\r\n" C_0, health );
   
   /* 60% of the total max health is needed, to break a limb. */
   needed = (float) (6*health)/10;
   
   clientff( " Damage needed: %.2f.\r\n", needed );
   
   for ( damage = dmg - 2; damage < dmg + 3; damage++ )
     if ( damage )
       clientff( " (%d):  %.2f punches.\r\n", damage, (float) needed / damage );
}


void parse_character( char *line )
{
   if ( !cmp( "You glance over ^ and see that ^ health is at *", line ) )
     {
	CHAR_DATA *ch;
	char name[256];
	int max_health;
	
	get_string( line + 16, name, 256 );
	
	for ( ch = characters; ch; ch = ch->next )
	  if ( !strcmp( name, ch->name ) )
	    break;
	
	if ( !ch )
	  return;
	
	sscanf( line, "You glance over %*s and see that %*s health is at %*d of a possible %d.",
		&max_health );
	
	if ( max_health != ch->max_health )
	  {
	     ch->max_health = max_health;
	     clientf( C_R " (" C_B "set" C_R ")" C_0 );
	  }
     }
   
   else if ( !cmp( "^'s condition stands at * mana.", line ) )
     {
	CHAR_DATA *ch;
	char name[256];
	int max_health, last_health;
	
	get_string( line, name, 256 );
	*(strchr( name, '\'' )) = 0;
	
	for ( ch = characters; ch; ch = ch->next )
	  if ( !strcmp( name, ch->name ) )
	    break;
	
	if ( !ch )
	  return;
	
	sscanf( line, "%*s condition stands at %d/%d health", &last_health, &max_health );
	
	if ( ch->max_health != max_health )
	  {
	     ch->max_health = max_health;
	     clientf( C_R " (" C_B "set" C_R ")" C_0 );
	  }
	
	if ( ch->last_health && ch->last_health != last_health )
	  clientff( C_R " (" C_G "%d" C_g "h" C_R ")" C_0, last_health - ch->last_health );
	
	if ( ch->last_health && last_health &&
	     ch->last_health - last_health > 10 &&
	     ch->last_health - last_health < 30 )
	  {
	     clientf( "\r\n" );
	     break_estimate( ch->max_health, ch->last_health - last_health );
	  }
	
	ch->last_health = last_health;
     }
   
   else if ( !cmp( "^ possesses * mana.", line ) )
     {
	CHAR_DATA *ch;
	char name[256];
	int health;
	
	get_string( line, name, 256 );
	
	for ( ch = characters; ch; ch = ch->next )
	  if ( !strcmp( name, ch->name ) )
	    break;
	
	if ( !ch )
	  return;
	
	sscanf( line, "%*s possesses %d health and %*d mana.",
		&health );
	
	if ( ch->max_health && health )
	  {
	     clientff( C_R " (" C_G "%d" C_g "h" C_R ")" C_0, health - ch->max_health );
	     
	     if ( ch->max_health - health > 10 && ch->max_health - health < 35 )
	       {
		  clientf( "\r\n" );
		  break_estimate( ch->max_health, ch->max_health - health );
	       }
	  }
     }
}



void parse_whohere_list( char *line )
{
   static int i;
   
   /* 
    * You see the following people here:
    * Someone, Nobody, Whyte
    * (1)      (2)     (3)
    */
   
   if ( !strcmp( line, "You see the following people here:" ) )
     {
	parsing_whohere_list = 1;
	wholist_targets[0][0] = 0;
	i = 0;
	return;
     }
   
   if ( !parsing_whohere_list )
     return;
   
   if ( !line[0] )
     return;
   
   /* This function will be called at each and every line.
    * So keep the variables here. */
     {
	char name[256], buf[2048], buf2[256];
	char *p;
	int position[32];
	int j, k, l, len, c_len;
	
	p = line;
	
	l = i;
	
	while ( *p )
	  {
	     if ( *p == ',' || *p == ' ' )
	       {
		  p++;
		  continue;
	       }
	     
	     if ( i > 30 )
	       {
		  parsing_whohere_list = 0;
		  break;
	       }
	     
	     if ( !p )
	       {
		  debugf( "A small HUGE error!" );
		  parsing_whohere_list = 0;
		  return;
	       }
	     
	     position[i] = p - line;
	     
	     p = get_string( p, name, 256 );
	     
	     /* Remove the trailing ',' if there is one. */
	     if ( strchr( name, ',' ) )
	       *(strchr( name, ',' ) ) = 0;
	     
	     if ( !name[0] )
	       continue;
	     
	     /* Add it to the list. */
	     strcpy( wholist_targets[i], name );
	     
	     wholist_targets[++i][0] = 0;
	  }
	
	if ( !i )
	  return;
	
	/* Do our best to show it up. */
	
	strcpy( buf, "\r\n " );
	len = 0;
	c_len = strlen( C_D C_C C_D );
	
	for ( k = l; wholist_targets[k][0]; k++ )
	  {
	     /* Fill up with spaces. */
	     for ( j = len; j < position[k]; j++, len++ )
	       strcat( buf, " " );
	     
	     /* Add the number. */
	     sprintf( buf2, C_D "(" C_C "%d" C_D ")", k+1 );
	     strcat( buf, buf2 );
	     len += strlen( buf2 ) - c_len;
	  }
	
	strcat( buf, C_0 );
	
	clientf( buf );
     }
}



void check_restoration( TIMER *self )
{
   CHAR_DATA *ch;
   char name[256];
   
   get_string( self->name + 12, name, 256 );
   
   for ( ch = characters; ch; ch = ch->next )
     if ( !strcmp( ch->name, name ) )
       break;
   
   if ( !ch )
     return;
   
   if ( !ch->limit )
     return;
   
   /* Legs. */
   if ( self->data[0] == 1 )
     {
	if ( ch->body[LEFT_LEG] > ch->limit )
	  ch->body[LEFT_LEG] = 0;
	else if ( ch->body[RIGHT_LEG] > ch->limit )
	  ch->body[RIGHT_LEG] = 0;
	else
	  return;
     }

   /* Arms. */
   if ( self->data[0] == 2 )
     {
	if ( ch->body[LEFT_ARM] > ch->limit )
	  ch->body[LEFT_ARM] = 0;
	else if ( ch->body[RIGHT_ARM] > ch->limit )
	  ch->body[RIGHT_ARM] = 0;
	else
	  return;
     }
   
   clientf( C_R "\r\n[" C_B "Opponent's limb healed." C_R "]\r\n" C_0 );
   show_prompt( );
}


void check_salve( char *line )
{
   CHAR_DATA *ch;
   char name[256];
   char place[256];
   
   /* Name */
   line = get_string( line, name, 256 );
   
   for ( ch = characters; ch; ch = ch->next )
     if ( !strcmp( name, ch->name ) )
       break;
   
   if ( !ch )
     return;
   
   /* rubs some salve on his/her (using 'place' as a temporary buffer) */
   line = get_string( line, place, 256 );
   line = get_string( line, place, 256 );
   line = get_string( line, place, 256 );
   line = get_string( line, place, 256 );
   line = get_string( line, place, 256 );
   
   /* arms/legs/torso/head. */
   line = get_string( line, place, 256 );
   
   if ( !strcmp( place, "legs." ) )
     {
	if ( ch->estimate )
	  {
	     int damage;
	     
	     damage = ch->body[LEFT_LEG] > ch->body[RIGHT_LEG] ? ch->body[LEFT_LEG] : ch->body[RIGHT_LEG];
	     
	     if ( ch->min_estimate < damage - punch_damage[stance] )
	       ch->min_estimate = damage - punch_damage[stance];
	     
	     if ( !ch->max_estimate || ch->max_estimate > damage )
		  ch->max_estimate = damage;
	     
	     ch->estimate = 0;
	     clientff( C_W "\r\nMin estimate: " C_G "%d" C_W "  Max estimate: " C_G "%d" C_W "." C_0,
		       ch->min_estimate, ch->max_estimate );
	  }
	
	  {
	     static int count;
	     char buf[256];
	     
	     count++;
	     if ( count > 256 )
	       count = 0;
	     
	     sprintf( buf, "restoration %s %d", name, count );
	     
	     add_timer( buf, 3, check_restoration, 1, 0, 0 );
	  }
     }
}


void parse_stance( char *line )
{
   char *messages[] =
     {    "You ease yourself out of the ^ stance.",
	  "You drop your legs into a sturdy Horse stance.",
	  "You draw back and balance into the Eagle stance.",
	  "You tense your muscles and look about sharply as you take the stance of the*",
	  "You draw yourself up to full height and roar aloud, adopting the Bear stance.",
	  "You take the Rat stance.",
	  "You sink back into the menacing stance of the Scorpion.",
	  "You allow the form of the Dragon to fill your mind and govern your actions.",
	  NULL
     };
   int i;
   
   for ( i = 0; messages[i]; i++ )
     {
	if ( !cmp( messages[i], line ) )
	  {
	     stance = i;
	     return;
	  }
     }
}


void parse_autolink( char *line )
{
   if ( !cmp( "(Taekyon): ^ says, \"Link.\"", line ) )
     {
	char name[256];
	char buf[256];
	
	get_string( line + 11, name, 256 );
	
	/* He will get no help, from the Taekyon. */
	if ( !strcmp( name, "Inyvon" ) )
	  return;
	
	clientff( C_R " (" C_W "link %s" C_R ")" C_0, name );
	sprintf( buf, "link %s\r\n", name );
	send_to_server( buf );
     }
   
   if ( !cmp( "You begin directing your mental energies in support of Inyvon.", line ) )
     {
	clientff( C_R " (" C_W "unlink" C_R ")" C_0 );
	send_to_server( "unlink\r\n" );
     }
}


void offensive_process_server_line_suffix( char *colorless_line, char *colorful_line, char *raw_line )
{
   char *line = colorless_line;
   
   DEBUG( "offensive_process_server_line" );
   
   check_hits_counter( line );
   
   check_timer( line );
   
   parse_character( line );
   
   parse_stance( line );
   
   parse_whohere_list( line );
   
   parse_autolink( line );
   
   if ( !cmp( "^ rubs some salve on ^ ^.", line ) &&
	cmp( "^ rubs some salve on ^ skin.", line ) )
     check_salve( line );
   
   /* An tiny little anti-sab trigger. Just for fun. */
   if ( !cmp( "^ snaps ^ fingers in front of you.", line ) &&
	cmp( colorless_line, colorful_line ) )
     {
	char name[256], buf[256];
	
	clientff( C_W " (" C_G "batter" C_W ")" C_0 );
	get_string( line, name, 256 );
	sprintf( buf, "mind batter %s\r\n", name );
	send_to_server( buf );
     }
}




void offensive_process_server_prompt( LINE *l )
{
   DEBUG( "offensive_process_server_prompt" );
   
   /* We may hit with something else. */
   if ( last_hit )
     last_hit = 0;
   
   if ( parsing_whohere_list )
     parsing_whohere_list = 0;
   
   check_triggers( l, 1 );
}



void add_alias( char *name, char *action )
{
   ALIAS_DATA *alias;
   
   for ( alias = aliases; alias; alias = alias->next )
     if ( !strcmp( alias->name, name ) )
       {
	  if ( !silent )
	    clientff( "Alias already exists, delete the old one first.\r\n" );
	  return;
       }
   
   alias = calloc( 1, sizeof( ALIAS_DATA ) );
   
   alias->name = strdup( name );
   alias->action = strdup( action );
   
   alias->next = aliases;
   aliases = alias;
   
   if ( !silent )
     clientff( "Alias '%s' defined.\r\n", name );
}


void free_alias( ALIAS_DATA *alias )
{
   free( alias->name );
   free( alias->action );
   free( alias );
}


void del_alias( char *name )
{
   ALIAS_DATA *a, *alias = NULL;
   
   if ( !aliases )
     alias = NULL;
   else if ( !strcmp( name, aliases->name ) )
     {
	alias = aliases;
	aliases = aliases->next;
     }
   else
     {
	for ( a = aliases; a->next; a = a->next )
	  if ( !strcmp( a->next->name, name ) )
	    {
	       alias = a->next;
	       a = a->next->next;
	    }
     }
   
   if ( !alias )
     {
	if ( !silent )
	  clientff( "There is no alias with that name.\r\n" );
	return;
     }
   
   free_alias( alias );
   if ( !silent )
     clientff( "Alias '%s' cleared.\r\n", name );
}


void do_alias( char *args )
{
   ALIAS_DATA *alias;
   char cmd[256];
   
   args = get_string( args, cmd, 256 );
   
   if ( !strcmp( cmd, "list" ) )
     {
	int first = 1;
	
	for ( alias = aliases; alias; alias = alias->next )
	  {
	     if ( first )
	       {
		  clientff( "Defined aliases:\r\n" );
		  first = 0;
	       }
	     
	     clientff( " " C_G "%s" C_0 " - '%s'\r\n", alias->name,
		       alias->action );
	  }
	
	if ( first )
	  clientff( "There are no aliases defined.\r\n" );
     }
   else if ( !strcmp( cmd, "add" ) )
     {
	char name[256];
	
	args = get_string( args, name, 256 );
	
	if ( !name[0] || !args[0] )
	  {
	     if ( silent )
	       debugf( "Error while adding an alias." );
	     else
	       clientff( "Syntax: alias add <name> <action>\r\n" );
	     return;
	  }
	
	add_alias( name, args );
     }
   else if ( !strcmp( cmd, "del" ) || !strcmp( cmd, "rem" ) )
     {
	char name[256];
	
	args = get_string( args, name, 256 );
	
	if ( !name[0] )
	  {
	     if ( silent )
	       debugf( "Error while deleting an alias." );
	     else
	       clientff( "Syntax: alias del <name>\r\n" );
	     return;
	  }
	
	del_alias( name );
     }
   else if ( !strcmp( cmd, "clear" ) )
     {
	ALIAS_DATA *a, *a_next;
	for ( a = aliases; a; a = a_next )
	  {
	     a_next = a->next;
	     free_alias( a );
	  }
	aliases = NULL;
	if ( !silent )
	  clientff( "All aliases cleared.\r\n" );
     }
   else
     clientff( "Syntax: alias add/del/list [args]\r\n" );
}


TRIGGER *new_trigger( )
{
   TRIGGER *trigger;
   
   trigger = calloc( 1, sizeof( TRIGGER ) );
   
   trigger->next = triggers;
   triggers = trigger;
   
   return trigger;
}



void remove_trigger( TRIGGER *trigger )
{
   TRIGGER *t;
   
   /* Unlink. */
   if ( trigger == triggers )
     {
	triggers = triggers->next;
     }
   else
     {
	for ( t = triggers; t->next; t = t->next )
	  if ( trigger == t->next )
	    {
	       t->next = trigger->next;
	       break;
	    }
     }
   
   if ( trigger->pattern )
     free( trigger->pattern );
   if ( trigger->body )
     free( trigger->body );
   free( trigger );
}


void do_trigger( char *args )
{
   TRIGGER *trigger;
   char pattern_buf[1024];
   pcre *pattern;
   const char *error;
   int error_offset;
   
   args = get_string( args, pattern_buf, 1024 );
   
   if ( !pattern_buf[0] )
     {
	clientfr( "No pattern specified." );
	return;
     }
   
   pattern = pcre_compile( pattern_buf, 0, &error, &error_offset, NULL );
   
   if ( error )
     {
	clientff( C_R "[Pattern error at offset %d: %s]\r\n" C_0, error_offset, error );
	return;
     }
   
   trigger = new_trigger( );
   trigger->pattern = pattern;
   trigger->extra = pcre_study( pattern, 0, &error );
   trigger->body = strdup( args );
   
   if ( error )
     clientff( C_R "[Pattern study error: %s]\r\n" C_0, error );
   
   clientfr( "Trigger added." );
}



void skip_whitespace( )
{
   while ( **pos && ( **pos == ' ' || **pos == '\n' || **pos == '\r' || **pos == '\t' ) )
     (*pos)++;
   
   /* Consider comments as whitespace. */
   if ( **pos == '/' )
     {
	if ( *((*pos)+1) == '/' )
	  {
	     while ( **pos && **pos != '\n' )
	       (*pos)++;
	     skip_whitespace( );
	  }
	else if ( *((*pos)+1) == '*' )
	  {
	     while( **pos && ( **pos != '*' || *((*pos)+1) != '/' ) )
	       (*pos)++;
	     if ( **pos )
	       (*pos) += 2;
	     skip_whitespace( );
	  }
     }
}



void link_script( SCRIPT *script )
{
   script->next = scripts;
   scripts = script;
}



void destroy_script( SCRIPT *script )
{
   SCRIPT *s;
   FUNCTION *f, *f_next;
   TRIGGER *t, *t_next;
   int i;
   
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
   
   for ( f = script->functions; f; f = f->next )
     {
	f_next = f->next;
	
	if ( f->name )
	  free( f->name );
	if ( f->body )
	  free( f->body );
	
	if ( f->args )
	  {
	     for ( i = 0; f->args[i]; i++ )
	       free( f->args[i] );
	     free( f->args );
	  }
	
	free( f );
     }
   
   for ( t = script->triggers; t; t = t->next )
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
   
   free( script );
}



void load_scripts( )
{
   char *initial_script = "{ if ( !load ( \"scripts.is\" ) ) init( ); }";
   CALL *call;
   
   call = prepare_block_as_call( "initial_script", initial_script );
   call_function( call, NULL, 0 );
}



void offensive_module_init_data( )
{
   void do_char_load( );
   int i;
   
   DEBUG( "offensive_init_data" );
   
   /* Initialize the common variables. */
   for ( i = 0; i < 10; i++ )
     alias_arg[i] = strdup( "" );
   alias_fullarg = strdup( "" );
   target = strdup( "none" );
   
   load_scripts( );
   do_char_load( );
   
   {
//      int a = 1, b = 1, c = 3;
//      a = b == c == !0 + 3 and 3 == 2 or 2;
//      debugf( "a: %d b: %d c: %d.", a, b, c );
   }
}




void set_args( char *args )
{
   char arg[256];
   int i;
   
   free( alias_fullarg );
   alias_fullarg = strdup( args );
   
   for ( i = 0; i < 10; i++ )
     {
	free( alias_arg[i] );
	args = get_string( args, arg, 256 );
	alias_arg[i] = strdup( arg );
     }
}


void parse_command( char *line )
{
   
   
   
}


const char *get_variable_from_name( char *name )
{
   if ( !name[0] )
     return "";
   
   /* Look for common variables. */
   if ( isdigit( name[0] ) && name[0] != '0' )
     return alias_arg[(int)(name[0]-'1')];
   if ( name[0] == '*' )
     return alias_fullarg;
   if ( !strcmp( name, "t" ) ||
	!strcmp( name, "tar" ) ||
	!strcmp( name, "target" ) )
     return target;
   if ( !strcmp( name, "T" ) )
     return alias_arg[0][0] ? alias_arg[0] : target;
   
   /* Look in the variable list. */
     {
	VARIABLE *var;
	
	if ( call_stack )
	  for ( var = call_stack->locals; var; var = var->next )
	    if ( !strcmp( name, var->name ) )
	      return var->value;
	
	for ( var = variables; var; var = var->next )
	  if ( !strcmp( name, var->name ) )
	    return var->value;
     }
   
   return "";
}



void convert_variables( char *line, char *dest )
{
   char var[256], *p;
   
   while ( *line )
     {
	if ( *line != '$' )
	  {
	     *(dest++) = *(line++);
	     continue;
	  }
	
	/* Skip the initial '$'. */
	line++;
	
	/* Copy the variable name into 'var'. */
	
	p = var;
	if ( *line == '(' )
	  {
	     line++;
	     while ( *line != ')' && *line )
	       *(p++) = *(line++);
	     line++;
	  }
	else
	  {
	     while ( ( ( *line >= 'a' && *line <= 'z' ) ||
		       ( *line >= 'A' && *line <= 'Z' ) ||
		       ( *line >= '0' && *line <= '9' ) ||
		       ( *line == '*' ) ) && *line )
	       *(p++) = *(line++);
	  }
	*p = 0;
	
	/* Need something better than "not space". */
	
	*dest = 0;
	strcat( dest, get_variable_from_name( var ) );
	dest += strlen( dest );
     }
   
   *dest = 0;
}



void get_identifier( char *dest, int max )
{
   skip_whitespace( );
   
   while ( --max && ( ( **pos >= 'A' && **pos <= 'Z' ) ||
		      ( **pos >= 'a' && **pos <= 'z' ) ||
		      ( **pos >= '0' && **pos <= '9' ) ||
		      **pos == '_' ) )
     *(dest++) = *((*pos)++);
   
   *dest = 0;
   
   skip_whitespace( );
}



void abort_script( int syntax, char *error, ... )
{
   char buf[1024];
   char what[1024];
   char func[1024];
   char error_buf[1024];
   
   va_list args;
   error_buf[0] = 0;
   if ( error )
     {
	va_start( args, error );
	vsnprintf( error_buf, 1024, error, args );
	va_end( args );
     }
   
   /* Figure out what exactly is at *pos. */
   if ( *pos )
     get_identifier( buf, 512 );
   
   if ( !*pos )
     what[0] = 0;
   else if ( buf[0] )
     sprintf( what, " before '%s'", buf );
   else if ( !**pos )
     strcpy( what, " before the End of Buffer" );
   else
     sprintf( what, " before character '%c'", **pos );
   
   if ( call_stack )
     sprintf( func, " in function %s,", call_stack->name );
   else
     func[0] = 0;
   
   sprintf( buf, "%s%s%s%c", syntax ? "Syntax error" : "Script aborted",
	    func, what, error ? ':' : '.' );
   
   clientff( C_0 "(" C_W "iScript" C_0 "): " C_R "%s\r\n" C_0, buf );
   if ( error )
     clientff( C_0 "(" C_W "iScript" C_0 "): " C_R "%s\r\n" C_0, error_buf );
   
   *pos = NULL;
   
   if ( call_stack )
     call_stack->aborted = 1;
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



void get_text( char *dest, int max )
{
   skip_whitespace( );
   
   if ( **pos != '\"' )
     {
	abort_script( 1, "Expected a string." );
	return;
     }
   
   (*pos)++;
   
   while ( --max && **pos && **pos != '\"' )
     {
	if ( **pos == '\\' )
	  {
	     (*pos)++;
	     switch( **pos )
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
		  *(dest++) = **pos;
	       }
	     (*pos)++;
	  }
	else
	  *(dest++) = *((*pos)++);
     }
   
   *dest = 0;
   
   if ( **pos != '\"' )
     {
	abort_script( 1, "Expected end of string." );
	return;
     }
   
   (*pos)++;
   skip_whitespace( );
}



void set_variable( char *name, char *value )
{
   VARIABLE *var = NULL;
   
   /* Locals. */
   if ( call_stack )
     for ( var = call_stack->locals; var; var = var->next )
       if ( !strcmp( name, var->name ) )
	 break;
   
   /* System variables. */
   if ( !var )
     {
	if ( !strcmp( name, "t" ) ||
	     !strcmp( name, "tar" ) ||
	     !strcmp( name, "target" ) )
	  {
	     free( target );
	     target = strdup( value );
	     return;
	  }
     }
   
   /* Globals. */
   if ( !var )
     for ( var = variables; var; var = var->next )
       if ( !strcmp( name, var->name ) )
	 break;
   
   /* Create it as global, if it doesn't exist. */
   if ( !var )
     {
	var = calloc( 1, sizeof( VARIABLE ) );
	var->next = variables;
	variables = var;
	var->name = strdup( name );
     }
   
   if ( var->value )
     free( var->value );
   
   var->value = strdup( value );
}



void set_local_variable( CALL *call, char *name, char *value )
{
   VARIABLE *var;
   
   if ( !call )
     {
	script_warning( "No call stack on set_local_variable?!" );
	return;
     }
   
   /* See if it already exists. */
   for ( var = call->locals; var; var = var->next )
     if ( !strcmp( name, var->name ) )
       break;
   
   if ( !var )
     {
	var = calloc( 1, sizeof( VARIABLE ) );
	var->next = call->locals;
	call->locals = var;
	var->name = strdup( name );
     }
   
   if ( var->value )
     free( var->value );
   
   var->value = strdup( value );
}



void copy_variable( char *name, char *dest, int max )
{
   char *src;
   
   /* Look for common variables. */
   if ( isdigit( name[0] ) && name[0] != '0' )
     src = alias_arg[(int)(name[0]-'1')];
   else if ( name[0] == '*' )
     src = alias_fullarg;
   else if ( !strcmp( name, "t" ) ||
	!strcmp( name, "tar" ) ||
	!strcmp( name, "target" ) )
     src = target;
   else if ( !strcmp( name, "T" ) )
     src = alias_arg[0][0] ? alias_arg[0] : target;
   
   /* Look in the variable list. */
   else
     {
	VARIABLE *var;
	
	src = NULL;
	
	if ( call_stack )
	  for ( var = call_stack->locals; var; var = var->next )
	    if ( !strcmp( name, var->name ) )
	      src = var->value;
	
	if ( !src )
	  for ( var = variables; var; var = var->next )
	    if ( !strcmp( name, var->name ) )
	      src = var->value;
     }
   
   if ( !src )
     src = "";
   
   while ( --max && *src )
     *(dest++) = *(src++);
   *dest = 0;
}



void kill_local_variables( CALL *call )
{
   if ( call->locals )
     {
	VARIABLE *v, *v_next;
	
	for ( v = call->locals; v; v = v_next )
	  {
	     v_next = v->next;
	     if ( v->name )
	       free( v->name );
	     if ( v->value )
	       free( v->value );
	     free( v );
	  }
     }
}



void push_call_in_stack( char *name, char *body )
{
   CALL *call;
   
   call = calloc( 1, sizeof( CALL ) );
   
   call->name = name;
   call->position = body;
   // Add parent script.
   
   call->below = call_stack;
   call_stack = call;
   
   pos = &call->position;
}



void pop_call_from_stack( )
{
   CALL *call = call_stack;
   
   /* Kill all local variables. */
   kill_local_variables( call );
   
   call_stack = call->below;
   
   if ( call_stack )
     {
	pos = &call_stack->position;
	
	if ( call->aborted )
	  {
	     *pos = NULL;
	     call_stack->aborted = 1;
	  }
     }
   
   free( call );
}



CALL *prepare_block_as_call( char *name, char *body )
{
   CALL *call;
   
   call = calloc( 1, sizeof( CALL ) );
   
   call->name = name;
   call->position = body;
   // Add parent script.
   
   return call;
}



CALL *prepare_call( FUNCTION *function )
{
   CALL *call;
   
   // Use the above?
   
   call = calloc( 1, sizeof( CALL ) );
   
   call->name = function->name;
   call->position = function->body;
   // Add parent script.
   
   return call;
}



void call_function( CALL *call, char *returns, int max )
{
   call->below = call_stack;
   call_stack = call;
   
   if ( returns )
     *returns = 0;
   call->return_value = returns;
   call->max = max;
   pos = &call->position;
   
   /* Burrrn! */
   execute_block( );
   
   /* Kill all local variables. */
   kill_local_variables( call );
   
   call_stack = call->below;
   
   if ( call_stack )
     {
	pos = &call_stack->position;
	
	if ( call->aborted )
	  {
	     *pos = NULL;
	     call_stack->aborted = 1;
	  }
     }
   
   free( call );
}



/* Operator priority. */

#define PRIORITY_INVERT	6 /* <-- Also for Argument. */
#define PRIORITY_PLUS	5
#define PRIORITY_EQUAL	4
#define PRIORITY_ASSIGN	3
#define PRIORITY_OR	2
#define PRIORITY_AND	1
#define PRIORITY_NORMAL	0

#define PRIORITY_ARGUMENT PRIORITY_INVERT


void do_echo( )
{
   char buf[4096];
   char buf2[4096];
   
   execute_single_command( buf, 4096, PRIORITY_NORMAL );
   if ( *pos )
     {
	convert_variables( buf, buf2 );
	strcat( buf2, "\r\n" );
	clientf( buf2 );
     }
}



void do_send( )
{
   char buf[4096];
   char buf2[4096];
   char *p;
   
   execute_single_command( buf, 4096, PRIORITY_NORMAL );
   if ( *pos )
     {
	/* Fixme. Overflow! */
	convert_variables( buf, buf2 );
	for ( p = buf2; *p; p++ )
	  if ( *p == ';' )
	    *p = '\n';
	
	strcat( buf2, "\n" );
	send_to_server( buf2 );
     }
}



void do_arg( char *returns, int max )
{
   char buf[4096];
   char *src;
   int nr;
   
   execute_single_command( buf, 4096, PRIORITY_ARGUMENT );
   
   if ( !returns )
     return;
   
   if ( buf[0] >= '0' && buf[0] <= '9' && !buf[1] )
     nr = (int)(buf[0]-'0');
   else
     {
	*returns = 0;
	return;
     }
   
   /* RegEx buffer callbacks. */
   
   if ( regex_callbacks )
     {
	if ( nr < regex_callbacks )
	  {
	     int size = regex_ovector[nr*2+1] - regex_ovector[nr*2];
	     char *p = regex_line + regex_ovector[nr*2];
	     
	     while ( --max && size-- && *p )
	       *(returns++) = *(p++);
	     *returns = 0;
	  }
	else
	  *returns = 0;
	
	return;
     }
   
   /* Alias arguments. */
   
   if ( nr != 0 )
     src = alias_arg[nr-1];
   else
     src = "";
   
   while ( --max && *src )
     *(returns++) = *(src++);
   *returns = 0;
}



void do_showprompt( char *returns, int max )
{
   if ( returns )
     *returns = 0;
   
   show_prompt( );
}



void do_not( char *returns, int max )
{
   char buf[4096];
   char *src;
   
   execute_single_command( buf, 4096, PRIORITY_INVERT );
   
   if ( buf[0] )
     src = "";
   else
     src = "true";
   
   if ( !returns )
     return;
   
   while ( --max && *src )
     *(returns++) = *(src++);
   *returns = 0;
}



void do_load( char *returns, int max )
{
   SCRIPT *load_script( const char *flname );
   SCRIPT *script;
   char buf[4096];
   char *src;
   
   execute_single_command( buf, 4096, PRIORITY_ARGUMENT );
   
   if ( buf[0] )
     for ( script = scripts; script; script = script->next )
       if ( !strcmp( script->name, buf ) )
	 {
	    destroy_script( script );
	    break;
	 }
   
   src = "";
   
   if ( !buf[0] )
     src = "No File Specified";
   else if ( !( script = load_script( buf ) ) )
     src = "Loading Failed";
   else
     link_script( script );
   
   if ( !returns )
     return;
   
   while ( --max && *src )
     *(returns++) = *(src++);
   *returns = 0;
}



/* After parsing an identifier, see what to do with it. */
void handle_results( char *results, char *dest, int max, int priority )
{
   char buf[4096], *b = buf;
   int b_max = 4096;
   // Check for !=

   skip_whitespace( );
   
   if ( **pos == '+' && priority <= PRIORITY_PLUS )
     {
	char right[4096];
	
	(*pos)++;
	
	if ( results )
	  while ( --b_max && *results )
	    *(b++) = *(results++);
	
	/* Concatenate this with the next. */
	execute_single_command( right, 4096, PRIORITY_PLUS );
	if ( !*pos )
	  return;
	
	results = right;
	if ( b_max )
	  while ( --b_max && *results )
	    *(b++) = *(results++);
	*b = 0;
	
	handle_results( buf, dest, max, priority );
	return;
     }
   
   // ToDo: Gonna have to find a way of... automatic grouping?
   
   /* ("asd" == "asd") == "asd" */
   /* Exception: ("" == "") == "true" */
   if ( **pos == '=' && *((*pos)+1) == '=' && priority <= PRIORITY_EQUAL )
     {
	char right[4096];
	
	*pos += 2;
	
	execute_single_command( right, 4096, PRIORITY_EQUAL );
	if ( !*pos )
	  return;
	
	if ( results )
	  {
	     if ( strcmp( results, right ) )
	       results = "";
	     else if ( !*results )
	       results = "true";
	     /* else, leave the results as they were. */
	  }
	else
	  {
	     if ( right[0] )
	       results = "";
	     else
	       results = "true";
	  }
	
	while ( --b_max && *results )
	  *(b++) = *(results++);
	*b = 0;
	
	handle_results( buf, dest, max, priority );
	return;
     }
   
   /* Compare, and update the position if true, at the same time.
    * Yeah, tricky, but convenient. */
   
   /* ( "asd" && "qwe" ) == "true" */
   else if ( priority <= PRIORITY_AND &&
	     ( ( **pos == '&' && *((*pos)+1) == '&' && *((*pos)+=2) ) ||
	       ( **pos == '&' && *((*pos)+=1) ) ||
	       ( **pos == 'a' && *((*pos)+1) == 'n' && *((*pos)+2) == 'd' && *((*pos)+=3) ) ) )
     {
	char right[4096];
	
	execute_single_command( right, 4096, PRIORITY_AND );
	if ( !*pos )
	  return;
	
	
	if ( results && results[0] && right[0] )
	  results = "true";
	else if ( !results && !right[0] )
	  results = "true";
	else
	  results = "";
	
	while ( --b_max && *results )
	  *(b++) = *(results++);
	*b = 0;
	
	handle_results( buf, dest, max, priority );
	return;
     }
   
   /* "asd" || "" == "true" */
   else if ( priority <= PRIORITY_OR &&
	     ( ( **pos == '|' && *((*pos)+1) == '|' && *((*pos)+=2) ) ||
	       ( **pos == '|' && *((*pos)+=1) ) ||
	       ( **pos == 'o' && *((*pos)+1) == 'r' && *((*pos)+=2) ) ) )
     {
	char right[4096];
	
	execute_single_command( right, 4096, PRIORITY_OR );
	if ( !*pos )
	  return;
	
	if ( ( results && results[0] ) || right[0] )
	  results = "true";
	else
	  results = "";
	
	while ( --b_max && *results )
	  *(b++) = *(results++);
	*b = 0;
	
	handle_results( buf, dest, max, priority );
	return;
     }
   
   /* Nothing? Then we're finished here. Copy the results. */
   
   if ( !dest || !results )
     return;
   
   while ( --max && *results )
     *(dest++) = *(results++);
   *dest = 0;
}



/* Something between parantheses will be returned as a whole. */
void group_results( char *dest, int max )
{
   char buf[4096], *b;
   
   (*pos)++;
   
   execute_single_command( buf, 4096, PRIORITY_NORMAL );
   if ( !*pos )
     return;
   
   skip_whitespace( );
   if ( **pos != ')' )
     {
	abort_script( 1, "Expected ')'." );
	return;
     }
   
   (*pos)++;
   
   if ( !dest )
     return;
   
   b = buf;
   
   while ( --max && *b )
     *(dest++) = *(b++);
   *dest = 0;
}



/* Parse a single command. */
void execute_single_command( char *returns, int max, int priority )
{
   VARIABLE *variable;
   FUNCTION *function;
   SCRIPT *script;
   char ident[4096];
   char buf[4096];
   int b_max = 4096, i;
   
   buf[0] = 0;
   
   skip_whitespace( );
   
   /* Special cases. */
   if ( **pos == '>' )
     {
	(*pos)++;
	do_send( buf, b_max );
	if ( !*pos )
	  return;
	
	handle_results( buf, returns, max, priority );
	return;
     }
   
   if ( **pos == '!' )
     {
	(*pos)++;
	do_not( buf, b_max );
	if ( !*pos )
	  return;
	
	handle_results( buf, returns, max, priority );
	return;
     }
   
   /* Constant string. */
   if ( **pos == '\"' )
     {
	get_text( buf, b_max );
	
	if ( !*pos )
	  return;
	
	handle_results( buf, returns, max, priority );
	return;
     }
   
   /* Group. */
   if ( **pos == '(' )
     {
	group_results( buf, b_max );
	if ( !*pos )
	  return;
	
	handle_results( buf, returns, max, priority );
	return;
     }
   
   get_identifier( ident, 4096 );
   /* Skip white space has been called by it, don't call it again. */
   
   if ( returns && max )
     returns[0] = 0;
   
   if ( !ident[0] )
     return;
   
   function = NULL;
   
   /* Maybe it's a function. */
   for ( script = scripts; script; script = script->next )
     {
	for ( function = script->functions; function; function = function->next )
	  if ( !strcmp( ident, function->name ) )
	    break;
	
	if ( function )
	  break;
     }
   
   if ( function )
     {
	CALL *call;
	char var_buf[4096];
	int expect_ending;
	
	if ( **pos == '(' )
	  {
	     (*pos)++;
	     expect_ending = 1;
	  }
	else
	  expect_ending = 0;
	
	call = prepare_call( function );
	for ( i = 0; function->args[i]; i++ )
	  {
	     execute_single_command( var_buf, 4096, expect_ending ? PRIORITY_NORMAL : PRIORITY_ARGUMENT );
	     
	     if ( !*pos )
	       {
		  kill_local_variables( call );
		  return;
	       }
	     
	     set_local_variable( call, function->args[i], var_buf );
	     
	     /* A single comma is allowed between arguments, but not required. */
	     if ( function->args[i+1] )
	       {
		  skip_whitespace( );
		  if ( **pos == ',' )
		    (*pos)++;
	       }
	  }
	
	if ( expect_ending )
	  {
	     skip_whitespace( );
	     if ( **pos != ')' )
	       {
		  abort_script( 1, "Expected ')' on end of argument list." );
		  kill_local_variables( call );
		  *pos = NULL;
		  return;
	       }
	     (*pos)++;
	  }
	
	call_function( call, buf, b_max );
	if ( !*pos )
	  return;
	
	handle_results( buf, returns, max, priority );
	return;
     }
   
   /* Perhaps it's a built-in function. */
   if ( !cmp( "return", ident ) )
     {
	if ( call_stack )
	  execute_single_command( call_stack->return_value, call_stack->max, PRIORITY_NORMAL );
	else
	  {
	     script_warning( "Returning with no call stack." );
	     execute_single_command( NULL, 0, PRIORITY_NORMAL );
	  }
	
	if ( !*pos )
	  return;
	
	*pos = NULL;
	return;
     }
   if ( !cmp( "not", ident ) )
     {
	do_not( buf, b_max );
	if ( *pos )
	  handle_results( buf, returns, max, priority );
	return;
     }
   if ( !cmp( "echo", ident ) )
     {
	do_echo( buf, b_max );
	if ( *pos )
	  handle_results( buf, returns, max, priority );
	return;
     }
   if ( !cmp( "send", ident ) )
     {
	do_send( buf, b_max );
	if ( *pos )
	  handle_results( buf, returns, max, priority );
	return;
     }
   if ( !cmp( "arg", ident ) )
     {
	do_arg( buf, b_max );
	if ( *pos )
	  handle_results( buf, returns, max, priority );
	return;
     }
   if ( !cmp( "show_prompt", ident ) )
     {
	do_showprompt( buf, b_max );
	if ( *pos )
	  handle_results( buf, returns, max, priority );
	return;
     }
   if ( !cmp( "load", ident ) )
     {
	do_load( buf, b_max );
	if ( *pos )
	  handle_results( buf, returns, max, priority );
	return;
     }
   
   /* Assignment. */
   if ( **pos == '=' && *((*pos)+1) != '=' )
     {
	char var_buf[4096];
	
	(*pos)++;
	
	// Normal, Assignment, or highest from priority and assignment?
	execute_single_command( var_buf, 4096, PRIORITY_ASSIGN );
	if ( !*pos )
	  return;
	
	set_variable( ident, var_buf );
	
	handle_results( var_buf, returns, max, priority );
	return;
     }
   
   /* Maybe it's a variable. Locals, then globals. */
   for ( variable = call_stack->locals; variable; variable = variable->next )
     if ( !strcmp( ident, variable->name ) )
       break;
   if ( !variable )
     for ( variable = variables; variable; variable = variable->next )
       if ( !strcmp( ident, variable->name ) )
	 break;
   
   if ( variable )
     {
	handle_results( variable->value, returns, max, priority );
     }
   else
     {
//	script_warning( "Unknown identifier '%s'.", ident );
	handle_results( "", returns, max, priority );
     }
   
   return;
}



/* Skip an expression. Or something. Between parantheses, anyway. */
void skip_group( )
{
   int brackets = 1, within_string = 0, within_comment = 0;
   
   if ( !*pos )
     return;
   
   if ( **pos != '(' )
     {
	abort_script( 1, "Expected '(' instead." );
	return;
     }
   
   (*pos)++;
   
   /* Find the matching ')'. */
   while ( brackets )
     {
	switch ( **pos )
	  {
	   case '\"':
	     if ( !within_comment )
	       within_string = !within_string; break;
	   case '(':
	     if ( !within_string && !within_comment ) brackets++; break;
	   case ')':
	     if ( !within_string && !within_comment ) brackets--; break;
	   case '/':
	     if ( !within_string && !within_comment )
	       {
		  if ( *((*pos)+1) == '/' )
		    within_comment = 1, *pos += 1;
		  else if ( *((*pos)+1) == '*' )
		    within_comment = 2, *pos += 1;
	       }
	     break;
	   case '\n':
	     if ( within_comment == 1 )
	       within_comment = 0;
	     break;
	   case '*':
	     if ( *((*pos)+1) == '/' && within_comment == 2 )
	       within_comment = 0, *pos += 1;
	     break;
	   case 0:
	     abort_script( 0, "Mismatched '(', unexpected End of Buffer." );
	     return;
	  }
	
	(*pos)++;
     }
}



/* Skip a block of commands. */
void skip_block( )
{
   int expect_end_of_block = 0;
   char *temp_pos;
   char buf[4096];
   
   if ( !*pos || !**pos )
     return;
   
   skip_whitespace( );
   
   if ( **pos == '{' )
     expect_end_of_block = 1, (*pos)++;
   
   while ( 1 )
     {
	skip_whitespace( );
	
	if ( !**pos )
	  {
	     abort_script( 1, NULL );
	     break;
	  }
	
	/* Done here? */
	if ( expect_end_of_block && **pos == '}' )
	  {
	     (*pos)++;
	     break;
	  }
	
	/* Block in a block? */
	if ( **pos == '{' )
	  {
	     skip_block( );
	     if ( !*pos )
	       break;
	     continue;
	  }
	
	/* Block controls? If's, While's, For's.. */
	if ( **pos == 'i' )
	  {
	     temp_pos = *pos;
	     get_identifier( buf, 4096 );
	     
	     if ( !strcmp( buf, "if" ) )
	       {
		  /* if <command> <block> else <block> */
		  skip_group( );
		  if ( !*pos )
		    break;
		  
		  skip_block( );
		  if ( !*pos )
		    break;
		  
		  temp_pos = *pos;
		  get_identifier( buf, 4096 );
		  if ( !strcmp( buf, "else" ) )
		    {
		       skip_block( );
		       if ( !*pos )
			 break;
		    }
		  else
		    *pos = temp_pos;
		  
		  if ( expect_end_of_block )
		    continue;
		  else
		    break;
	       }
	     
	     *pos = temp_pos;
	  }
	
	/* We have a command here. */
	while ( **pos && **pos != ';' )
	  (*pos)++;
	if ( **pos != ';' )
	  {
	     abort_script( 0, "Expected ';' instead." );
	     break;
	  }
	(*pos)++;
	
	if ( expect_end_of_block )
	  continue;
	else
	  break;
     }
}


/* Execute a block of commands. */
void execute_block( )
{
   int expect_end_of_block = 0;
   char *temp_pos;
   char buf[4096];
   
   if ( !*pos || !**pos )
     return;
   
   if ( **pos == '{' )
     expect_end_of_block = 1, (*pos)++;
   
   while ( 1 )
     {
	skip_whitespace( );
	
	if ( !**pos )
	  {
	     abort_script( 1, NULL );
	     return;
	  }
	
	/* Done here? */
	if ( expect_end_of_block && **pos == '}' )
	  {
	     (*pos)++;
	     return;
	  }
	
	/* Block in a block? */
	if ( **pos == '{' )
	  {
	     execute_block( );
	     if ( !*pos )
	       return;
	     continue;
	  }
	
	/* Block controls? If's, While's, For's.. */
	if ( **pos == 'i' )
	  {
	     temp_pos = *pos;
	     get_identifier( buf, 4096 );
	     
	     if ( !strcmp( buf, "if" ) )
	       {
		  int truth;
		  
		  /* if <command> <block> else <block> */
		  execute_single_command( buf, 4096, PRIORITY_NORMAL );
		  if ( !*pos )
		    return;
		  
		  if ( buf[0] )
		    truth = 1;
		  else
		    truth = 0;
		  
		  if ( truth )
		    execute_block( );
		  else
		    skip_block( );
		  if ( !*pos )
		    return;
		  
		  temp_pos = *pos;
		  get_identifier( buf, 4096 );
		  if ( !strcmp( buf, "else" ) )
		    {
		       if ( !truth )
			 execute_block( );
		       else
			 skip_block( );
		       if ( !*pos )
			 return;
		    }
		  else
		    *pos = temp_pos;
		  
		  if ( expect_end_of_block )
		    continue;
		  else
		    return;
	       }
	     
	     *pos = temp_pos;
	  }
	
	/* We have a command here. */
	execute_single_command( NULL, 0, PRIORITY_NORMAL );
	if ( !*pos )
	  return;
	skip_whitespace( );
	
	if ( **pos != ';' )
	  {
	     abort_script( 0, "Expected ';' instead." );
	     return;
	  }
	
	(*pos)++;
	
	if ( expect_end_of_block )
	  continue;
	else
	  return;
     }
}



int load_script_function( SCRIPT *script, int alias )
{
   FUNCTION *function, *f;
   char buf[4096];
   int expect_ending, expect_argument, brackets, within_string;
   int i, size;
   
   function = calloc( 1, sizeof( FUNCTION ) );
   
   /* Link it. */
   if ( !script->functions )
     script->functions = function;
   else
     {
	for ( f = script->functions; f->next; f = f->next );
	f->next = function;
     }
   
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
	
	if ( **pos == '(' )
	  {
	     expect_ending = 1;
	     (*pos)++;
	     skip_whitespace( );
	  }
	else
	  expect_ending = 0;
	
	function->args = calloc( 1, sizeof( char * ) );
	i = 0;
	
	/* Find all arguments. */
	while ( 1 )
	  {
	     if ( i && **pos == ',' )
	       {
		  expect_argument = 1;
		  (*pos)++;
	       }
	     else
	       expect_argument = 0;
	     
	     get_identifier( buf, 4096 );
	     
	     if ( !buf[0] )
	       break;
	     
	     if ( expect_argument )
	       expect_argument = 0;
	     function->args = realloc( function->args, sizeof( char * ) * (i + 1) );
	     function->args[i] = strdup( buf );
	     i++;
	  }
	
	function->args[i] = NULL;
	
	if ( expect_argument )
	  {
	     abort_script( 0, "Expected a function argument." );
	     return 1;
	  }
	
	if ( expect_ending )
	  {
	     if ( **pos != ')' )
	       {
		  abort_script( 0, "Mismatched '(', expected ')' on argument list." );
		  return 1;
	       }
	     (*pos)++;
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
   
   if ( **pos != '{' )
     {
	abort_script( 1, "No body provided to function '%s', expected '{'.", function->name );
	return 1;
     }
   
   /* Find the matching '}'. */
   size = 1, brackets = 1, within_string = 0;
   while ( brackets )
     {
	switch ( *((*pos)+size) )
	  {
	   case '\"':
	     within_string = !within_string; break;
	   case '{':
	     if ( !within_string ) brackets++; break;
	   case '}':
	     if ( !within_string ) brackets--; break;
	   case 0:
	     abort_script( 0, "Mismatched '{', unexpected End of Buffer." );
	     return 1;
	  }
	size++;
     }
   
   function->body = malloc( size + 1 );
   memcpy( function->body, *pos, size );
   function->body[size] = 0;
   function->size = size;
   *pos += size;
   
   return 0;
}



int load_script_trigger( SCRIPT *script )
{
   TRIGGER *trigger, *t;
   char buf[4096];
   int brackets, within_string, size;
   
   /* Format: trigger ['<name>'] [options] <*|"pattern"> { ... } */
   
   trigger = calloc( 1, sizeof( TRIGGER ) );
   
   /* Link it. */
   if ( !script->triggers )
     script->triggers = trigger;
   else
     {
	for ( t = script->triggers; t->next; t = t->next );
	t->next = trigger;
     }
   
   /* Name, if there's one. */
   if ( **pos == '\'' )
     {
	(*pos)++;
	
	get_identifier( buf, 4096 );
	
	if ( **pos != '\'' )
	  {
	     abort_script( 1, "Invalid trigger name." );
	     return 1;
	  }
	
	(*pos)++;
	
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
   
   if ( **pos == '*' )
     {
	trigger->everything = 1;
	(*pos)++;
	skip_whitespace( );
     }
   else
     {
	get_text( buf, 4096 );
	
	if ( !*pos )
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
   
   if ( **pos != '{' )
     {
	if ( trigger->name )
	  abort_script( 1, "No body provided to trigger '%s', expected '{'.", trigger->name );
	else
	  abort_script( 1, "No body provided to a trigger, expected '{'." );
	return 1;
     }
   
   /* Find the matching '}'. */
   size = 1, brackets = 1, within_string = 0;
   while ( brackets )
     {
	switch ( *((*pos)+size) )
	  {
	   case '\"':
	     within_string = !within_string; break;
	   case '{':
	     if ( !within_string ) brackets++; break;
	   case '}':
	     if ( !within_string ) brackets--; break;
	   case 0:
	     abort_script( 0, "Mismatched '{', unexpected End of Buffer." );
	     return 1;
	  }
	size++;
     }
   
   trigger->body = malloc( size + 1 );
   memcpy( trigger->body, *pos, size );
   trigger->body[size] = 0;
   trigger->size = size;
   *pos += size;
   
   return 0;
}



SCRIPT *load_script( const char *flname )
{
   SCRIPT *script;
   FILE *fl;
   char buf[4096], *body = NULL;
   int bytes, size = 0;
   int aborted = 0;
   
   fl = fopen( flname, "r" );
   
   if ( !fl )
     return NULL;
   
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
	     body = realloc( body, size + 1 + 4096);
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
   
   push_call_in_stack( "load_script", body );
   
   /* Seek all functions/triggers within it. */
   
   while ( 1 )
     {
	/* Expect... '#', function, alias, or trigger. */
	
	skip_whitespace( );
	
	if ( !**pos )
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
	     
	     if ( !*pos )
	       {
		  aborted = 1;
		  break;
	       }
	     
	     if ( **pos != ';' )
	       {
		  abort_script( 1, "Expected ';' instead." );
		  break;
	       }
	     (*pos)++;
	     
	     if ( script->description )
	       free( script->description );
	     script->description = strdup( buf );
	  }
     }
   
   pop_call_from_stack( );
   free( body );
   
   if ( aborted )
     {
	destroy_script( script );
	return NULL;
     }
   
   if ( !script->description )
     script->description = strdup( "No Description" );
   
   return script;
}



void do_char_list( )
{
   CHAR_DATA *ch;
   
   if ( !characters )
     {
	clientfr( "There are no characters registered." );
	return;
     }
   
   clientfr( "Characters         Health  Legs/Arms/Head  Limit" );
   
   for ( ch = characters; ch; ch = ch->next )
     {
	clientff( " %-20s " C_g "%4d" C_G "  %4d %4d %4d   %4d" C_0 "\r\n",
		  ch->name, ch->max_health,
		  ch->leg_health, ch->arm_health, ch->head_health,
		  ch->limit );
     }
}

void do_char_show( CHAR_DATA *ch )
{
   clientfr( ch->name );
   
   clientff( "Max Health: " C_G "%d" C_0 ".\r\n"
	     "Min/Max damage estimate: " C_G "%d" C_0 "/" C_G "%d" C_0 ".\r\n"
	     "Limit: " C_G "%d" C_0 ". Head/arm/leg health: "
	     C_G "%d" C_0 "/" C_G "%d" C_0 "/" C_G "%d" C_0 ".\r\n"
	     "Damage:\r\n"
	     "%sLeft Arm: " C_G "%3d" C_0 "  %sRight Arm: " C_G "%3d" C_0 "\r\n"
	     "%sLeft Leg: " C_G "%3d" C_0 "  %sRight Leg: " C_G "%3d" C_0 "\r\n",
	     ch->max_health, ch->min_estimate, ch->max_estimate,
	     ch->limit, ch->head_health, ch->arm_health, ch->leg_health,
	     ch->sync_parts[LEFT_ARM] ? C_W : C_0, ch->body[LEFT_ARM],
	     ch->sync_parts[RIGHT_ARM] ? C_W : C_0, ch->body[RIGHT_ARM],
	     ch->sync_parts[LEFT_LEG] ? C_W : C_0, ch->body[LEFT_LEG],
	     ch->sync_parts[RIGHT_LEG] ? C_W : C_0, ch->body[RIGHT_LEG] );
}



int check_limit( CHAR_DATA *ch, int part, int no_limit )
{
   int damage = ch->body[part] + ( (ch->extra_hits[part]+1) * punch_damage[stance] );
   
   if ( no_limit )
     return 1;
   
   if ( ch->limit && ( damage <= ch->limit ) )
     return 1;
   
   if ( !ch->limit )
     {
	if ( ( part == LEFT_LEG || part == RIGHT_LEG ) &&
	     ( damage <= ch->leg_health ) )
	  return 1;
	
	if ( ( part == LEFT_ARM || part == RIGHT_ARM ) &&
	     ( damage <= ch->arm_health ) )
	  return 1;
	
	if ( part == HEAD && damage <= ch->head_health )
	  return 1;
     }
   
   return 0;
}



int add_combo_element( CHAR_DATA *ch, int alternative, int rev, int no_limit )
{
   int j;
   
   /* First, those we are sure are not parried. */
   if ( !alternative || ( alternative && !rev ) )
     {
	for ( j = 0; j < PARTS; j++ )
	  if ( ch->sync_parts[j] && j != ch->maybe_parried &&
	       check_limit( ch, j, no_limit ) )
	    break;
     }
   else /* Check in reverse. */
     {
	for ( j = PARTS-1; j >= 0; j-- )
	  if ( ch->sync_parts[j] && j != ch->maybe_parried &&
	       check_limit( ch, j, no_limit ) )
	    break;
     }
   
   if ( j != PARTS )
     {
	ch->extra_hits[j]++;
	return j;
     }
   
   /* Now, change the parry location, by feinting at the torso. */
   if ( ch->maybe_parried != TORSO && ch->maybe_parried != GENERAL )
     {
	clientff( "P: %d.\r\n", ch->maybe_parried );
	ch->maybe_parried = TORSO;
	return TORSO;
     }
   
   /* Or just don't do anything at all. */
   return GENERAL;
}


void do_sync_combo( char *args )
{
   CHAR_DATA *ch;
   char combo[256], buf[256];
   int part[4];
   int i = 0;
   int sweep = 0, jumpkick = 0, feint = 0, alternative = 0, no_limit = 0;
   int punches;
   
   for ( ch = characters; ch; ch = ch->next )
     if ( !strcmp( ch->name, target ) )
       break;
   
   if ( !ch )
     {
	clientfr( "The target is not known in the database." );
	return;
     }
   
   /* Parse all arguments. */
   while ( *args )
     {
	args = get_string( args, buf, 256 );
	
	if ( !strcmp( buf, "swk" ) || !strcmp( buf, "s" ) )
	  sweep = 1;
	else if ( !strcmp( buf, "jpk" ) || !strcmp( buf, "j" ) )
	  jumpkick = 1;
	else if ( !strcmp( buf, "feint" ) || !strcmp( buf, "f" ) )
	  feint = 1;
	else if ( !strcmp( buf, "alt" ) )
	  alternative = 1;
	else if ( !strcmp( buf, "break" ) )
	  no_limit = 1;
	else if ( !strcmp( buf, "all" ) )
	  {
	     for ( i = 0; i < PARTS; i++ )
	       ch->sync_parts[i] = 0;
	     ch->sync_parts[LEFT_LEG] = 1;
	     ch->sync_parts[RIGHT_LEG] = 1;
	     ch->sync_parts[LEFT_ARM] = 1;
	     ch->sync_parts[RIGHT_ARM] = 1;
	  }
	else if ( !strcmp( buf, "legs" ) )
	  {
	     for ( i = 0; i < PARTS; i++ )
	       ch->sync_parts[i] = 0;
	     ch->sync_parts[LEFT_LEG] = 1;
	     ch->sync_parts[RIGHT_LEG] = 1;
	  }
	else if ( !strcmp( buf, "arms" ) )
	  {
	     for ( i = 0; i < PARTS; i++ )
	       ch->sync_parts[i] = 0;
	     ch->sync_parts[LEFT_ARM] = 1;
	     ch->sync_parts[RIGHT_ARM] = 1;
	  }
	else
	  clientfr( "Warning, unknown argument." );
     }
   
   /* Reset a few things. */
   combo[0] = 0;
   ch->maybe_parried = ch->parried_part;
   for ( i = 0; i < PARTS; i++ )
     ch->extra_hits[i] = 0;
   
   /* Choose the limbs to hit. */
   for ( i = 0; i < 4; i++ )
     part[i] = add_combo_element( ch, alternative, i % 2, no_limit );
   
   /* Optional attacks. */
   punches = 4;
   if ( jumpkick )
     {
	sprintf( buf, "jpk %s\r\n", target );
	strcat( combo, buf );
	punches = 2;
     }
   if ( sweep )
     {
	sprintf( buf, "swk %s\r\n", target );
	strcat( combo, buf );
	punches = 2;
     }
   if ( feint )
     {
	sprintf( buf, "feint %s torso\r\n", target );
	strcat( combo, buf );
	punches--;
     }
   
   /* Translate the parts into commands. */
   for ( i = 0; i < punches; i++ )
     {
	switch( part[i] )
	  {
	   case GENERAL: buf[0] = 0; break;
	   case LEFT_LEG: sprintf( buf, "hfp %s left\r\n", target ); break;
	   case RIGHT_LEG: sprintf( buf, "hfp %s right\r\n", target ); break;
	   case LEFT_ARM: sprintf( buf, "spp %s left\r\n", target ); break;
	   case RIGHT_ARM: sprintf( buf, "spp %s right\r\n", target ); break;
	   case HEAD: sprintf( buf, "ucp %s\r\n", target ); break;
	   case TORSO: sprintf( buf, "feint %s torso\r\n", target ); break;
	   default: clientff( "Eh?\r\n" ); break;
	  }
	
	strcat( combo, buf );
     }
   
   /* Fire it away! */
   send_to_server( combo );
   
   /* If there was anything in it, anyway... */
   if ( !combo[0] )
     {
	clientfr( "Nothing to do!" );
	show_prompt( );
     }
}



void do_sync( char *args )
{
   char cmd[256];
   
   args = get_string( args, cmd, 256 );
   
   if ( !strcmp( cmd, "show" ) )
     {
	show_part_hits( );
	show_prompt( );
     }
   
   else if ( !strcmp( cmd, "set" ) )
     {
	CHAR_DATA *ch;
	char buf[256];
	
	for ( ch = characters; ch; ch = ch->next )
	  if ( !strcmp( ch->name, target ) )
	    break;
	
	if ( !ch )
	  {
	     clientfr( "The target is not known in the database." );
	     show_prompt( );
	     return;
	  }
	
	args = get_string( args, buf, 256 );
	
	if ( !strcmp( buf, "left" ) )
	  {
	     if ( !strcmp( args, "leg" ) )
	       ch->sync_parts[LEFT_LEG] = ch->sync_parts[LEFT_LEG] ? 0 : 1;
	     else if ( !strcmp( args, "arm" ) )
	       ch->sync_parts[LEFT_ARM] = ch->sync_parts[LEFT_ARM] ? 0 : 1;
	  }
	else if ( !strcmp( buf, "right" ) )
	  {
	     if ( !strcmp( args, "leg" ) )
	       ch->sync_parts[RIGHT_LEG] = ch->sync_parts[RIGHT_LEG] ? 0 : 1;
	     else if ( !strcmp( args, "arm" ) )
	       ch->sync_parts[RIGHT_ARM] = ch->sync_parts[RIGHT_ARM] ? 0 : 1;
	  }
	else if ( !strcmp( buf, "all" ) )
	  {
	     ch->sync_parts[LEFT_LEG] = 1;
	     ch->sync_parts[RIGHT_LEG] = 1;
	     ch->sync_parts[LEFT_ARM] = 1;
	     ch->sync_parts[RIGHT_ARM] = 1;
	  }
	
	do_char_show( ch );
	show_prompt( );
     }
   
   else if ( !strcmp( cmd, "reset" ) )
     {
	int i, j;
	
	for ( i = 0; i < PARTS; i++ )
	  {
	     for ( j = 0; j < 12; j++ )
	       hit_counter[i][j] = 0;
	  }
	
	clientfr( "All body parts reset." );
     }
   
   else if ( !strcmp( cmd, "combo" ) )
     {
	do_sync_combo( cmd + 5 );
     }
   
}



void parse_line( char *line )
{
   char raw_line[4096];
   
   /* Skip the initial white spaces from the line. */
   while ( *line == ' ' )
     line++;
   
   /* Command, or just something to send to the server? */
   if ( line[0] == '.' )
     parse_command( line );
   else
     {
	/* So what we have, is a line, which may contain variables. */
	convert_variables( line, raw_line );
//	clientff( "Result: [%s]\r\n", raw_line );
	strcat( raw_line, "\r\n" );
	
	/* Send the freshly built line to the server. */
	send_to_server( raw_line );
     }
}



void do_time( char *args )
{
   char command[256];
   
   args = get_string( args, command, 256 );
   
   if ( !strcmp( command, "start" ) )
     {
	if ( string_start )
	  free( string_start );
	
	if ( args[0] )
	  string_start = strdup( args );
	else
	  string_start = NULL;
	
	clientff( C_R "[Start message on: '%s']\r\n" C_0,
		  string_start ? string_start : "none" );
     }
   else if ( !strcmp( command, "stop" ) )
     {
	if ( string_stop )
	  free( string_stop );
	
	if ( args[0] )
	  string_stop = strdup( args );
	else
	  string_stop = NULL;
	
	clientff( C_R "[Stop message on: '%s']\r\n" C_0,
		  string_stop ? string_stop : "none" );
     }
   else if ( !strcmp( command, "reset" ) || !strcmp( command, "clear" ) )
     {
	if ( string_start )
	  free( string_start );
	
	if ( string_stop )
	  free( string_stop );
	
	string_start = string_stop = NULL;
	
	clientfr( "Start and stop strings cleared." );
     }
   else
     {
	clientfr( "Syntax: tm <start/stop/clear> [string]" );
     }
}



CHAR_DATA *add_character( char *name )
{
   CHAR_DATA *ch;
   
   ch = calloc( 1, sizeof( CHAR_DATA ) );
   
   ch->name = strdup( name );
   
   /* Link it to the main chain. */
   ch->next = characters;
   characters = ch;
   
   return ch;
}



void remove_character( CHAR_DATA *ch )
{
   CHAR_DATA *chr;
   
   /* Unlink it from the main chain. */
   if ( ch == characters )
     characters = characters->next;
   else
     {
	for ( chr = characters; chr->next; chr = chr->next )
	  {
	     if ( chr->next == ch )
	       {
		  chr->next = ch->next;
		  break;
	       }
	  }
     }
   
   /* Free it up. */
   free( ch->name );
   free( ch );
}



void do_char_save( )
{
   FILE *fl;
   CHAR_DATA *ch;
   int i;
   
   fl = fopen( "characters", "w" );
   
   if ( !fl )
     {
	clientfr( "Can not open file, to write." );
	return;
     }
   
   fprintf( fl, "CHARACTERS\n\n\n" );
   
   for ( ch = characters; ch; ch = ch->next )
     {
	fprintf( fl, "Ch: \"%s\"\n", ch->name );
	
	fprintf( fl, "Health: %d\n", ch->max_health );
	fprintf( fl, "Min-Estimate: %d\n", ch->min_estimate );
	fprintf( fl, "Max-Estimate: %d\n", ch->max_estimate );
	fprintf( fl, "Arm-Health: %d\n", ch->arm_health );
	fprintf( fl, "Leg-Health: %d\n", ch->leg_health );
	fprintf( fl, "Head-Health: %d\n", ch->head_health );
	fprintf( fl, "Limit: %d\n", ch->limit );
	fprintf( fl, "Sync-Parts: " );
	for ( i = 0; i < PARTS; i++ )
	  if ( ch->sync_parts[i] )
	    fprintf( fl, "%d ", i );
	fprintf( fl, "end\n" );
	
	fprintf( fl, "End\n\n" );
     }
   
   fclose( fl );
}



void do_char_load( )
{
   FILE *fl;
   CHAR_DATA *ch = NULL;
   char line[2048], buf[256], cmd[64], *args, *p;
   const char *flname = "characters";
   
   fl = fopen( flname, "r" );
   
   if ( !fl )
     {
	clientff( C_R "[Can't open %s: %s]\r\n" C_0,
		  flname, strerror( errno ) );
	return;
     }
   
   /* Clear all existing characters, first. */
   while( characters )
     remove_character( characters );
   
   while( 1 )
     {
	fgets( line, 2048, fl );
	
	if ( feof( fl ) )
	  break;
	
	if ( line[0] == '#' || line[0] == ' ' || line[0] == '\n' || !line[0] )
	  continue;
	
	/* Remove the newline from it. */
	p = line;
	while( *p && *p != '\n' )
	  p++;
	*p = 0;
	
	args = get_string( line, cmd, 64 );
	
	if ( !strcmp( cmd, "Ch:" ) )
	  {
	     get_string( args, buf, 256 );
	     
	     for ( ch = characters; ch; ch = ch->next )
	       if ( !strcmp( ch->name, buf ) )
		 break;
	     
	     /* If it doesn't exist, create one. */
	     ch = add_character( buf );
	     
	     continue;
	  }
	
	if ( !ch )
	  continue;
	
	if ( !strcmp( cmd, "Health:" ) )
	  {
	     get_string( args, buf, 256 );
	     ch->max_health = atoi( buf );
	  }
	
	else if ( !strcmp( cmd, "Limit:" ) )
	  {
	     get_string( args, buf, 256 );
	     ch->limit = atoi( buf );
	  }
	
	else if ( !strcmp( cmd, "Min-Estimate:" ) )
	  {
	     get_string( args, buf, 256 );
	     ch->min_estimate = atoi( buf );
	  }
	
	else if ( !strcmp( cmd, "Max-Estimate:" ) )
	  {
	     get_string( args, buf, 256 );
	     ch->max_estimate = atoi( buf );
	  }
	
	else if ( !strcmp( cmd, "Head-Health:" ) )
	  {
	     get_string( args, buf, 256 );
	     ch->head_health = atoi( buf );
	  }
	
	else if ( !strcmp( cmd, "Arm-Health:" ) )
	  {
	     get_string( args, buf, 256 );
	     ch->arm_health = atoi( buf );
	  }
	
	else if ( !strcmp( cmd, "Leg-Health:" ) )
	  {
	     get_string( args, buf, 256 );
	     ch->leg_health = atoi( buf );
	  }
	
	else if ( !strcmp( cmd, "Sync-Parts:" ) )
	  {
	     char *p = args;
	     int part;
	     
	     while ( *p )
	       {
		  p = get_string( p, buf, 256 );
		  
		  if ( !strcmp( buf, "end" ) )
		    break;
		  
		  part = atoi( buf );
		  ch->sync_parts[part] = 1;
	       }
	  }
     }
   
   fclose( fl );
}



void do_character( char *args )
{
   CHAR_DATA *ch, *tch;
   char cmd[256];
   char name[256];
   
   args = get_string( args, cmd, 256 );
   
   /* Set tch from the $target variable. */
   for ( tch = characters; tch; tch = tch->next )
     if ( !strcmp( target, tch->name ) )
       break;
   
   if ( !strcmp( cmd, "add" ) )
     {
	args = get_string( args, name, 256 );
	
	if ( !name[0] )
	  {
	     if ( !target[0] || !strcmp( target, "none" ) )
	       {
		  clientfr( "What character to register?" );
		  return;
	       }
	     
	     strcpy( name, target );
	  }
	
	/* Make sure it doesn't exist, first. */
	for ( ch = characters; ch; ch = ch->next )
	  if ( !strcmp( name, ch->name ) )
	    {
	       clientfr( "A character by that name is already registered." );
	       return;
	    }
	
	ch = add_character( name );
	
	/* Default. */
	ch->sync_parts[LEFT_LEG] = 1;
	ch->sync_parts[RIGHT_LEG] = 1;
	ch->sync_parts[LEFT_ARM] = 1;
	ch->sync_parts[RIGHT_ARM] = 1;
	
	clientfr( "Character added." );
     }
   else if ( !strcmp( cmd, "del" ) || !strcmp( cmd, "rem" ) )
     {
	args = get_string( args, name, 256 );
	
	/* Make sure it -does- exist. */
	for ( ch = characters; ch; ch = ch->next )
	  if ( !strcmp( name, ch->name ) )
	    break;
	
	if ( !ch )
	  {
	     clientfr( "No character of that name is registered." );
	     return;
	  }
	
	remove_character( ch );
	clientfr( "Character deleted." );
     }
   else if ( !strcmp( cmd, "list" ) )
     {
	do_char_list( );
     }
   else if ( !strcmp( cmd, "estimate" ) )
     {
	if ( !tch )
	  {
	     clientfr( "Target character is not in the database." );
	     return;
	  }
	
	tch->estimate = !tch->estimate;
	if ( tch->estimate )
	  clientfr( "Estimate enabled." );
	else
	  clientfr( "Estimate disabled." );
     }
   else if ( !strcmp( cmd, "health" ) )
     {
	if ( !tch )
	  {
	     clientfr( "Target character is not in the database." );
	     return;
	  }
	
	tch->max_health = atoi( args );
	
	clientff( C_R "[Max health on " C_W "%s" C_R " set to " C_G "%d" C_R ".]\r\n" C_0,
		  tch->name, tch->max_health );
     }
   
   else if ( !strcmp( cmd, "limit" ) )
     {
	if ( !tch )
	  {
	     clientfr( "Target character is not in the database." );
	     return;
	  }
	
	tch->limit = atoi( args );
	
	clientff( C_R "[Hit limit on %s set to " C_G "%d" C_R ".]\r\n" C_0,
		  tch->name, tch->limit );
     }
   
   else if ( !strcmp( cmd, "break" ) )
     {
	char buf[256];
	
	if ( !tch )
	  {
	     clientfr( "Target character is not in the database." );
	     return;
	  }
	
	args = get_string( args, buf, 256 );
	
	if ( !strcmp( buf, "head" ) )
	  {
	     get_string( args, buf, 256 );
	     tch->head_health = atoi( buf );
	     clientff( C_R "[Head health on " C_W "%s" C_R " set to " C_G "%d" C_R ".]\r\n" C_0,
		       tch->name, tch->head_health );
	  }
	if ( !strcmp( buf, "arm" ) || !strcmp( buf, "arms" ) )
	  {
	     get_string( args, buf, 256 );
	     tch->arm_health = atoi( buf );
	     clientff( C_R "[Arm health on " C_W "%s" C_R " set to " C_G "%d" C_R ".]\r\n" C_0,
		       tch->name, tch->arm_health );
	  }
	if ( !strcmp( buf, "leg" ) || !strcmp( buf, "legs" ) )
	  {
	     get_string( args, buf, 256 );
	     tch->leg_health = atoi( buf );
	     clientff( C_R "[Leg health on " C_W "%s" C_R " set to " C_G "%d" C_R ".]\r\n" C_0,
		       tch->name, tch->leg_health );
	  }
	else
	  {
	     tch->head_health = atoi( buf );
	     tch->arm_health = tch->head_health;
	     tch->leg_health = tch->head_health;
	     clientff( C_R "[Head, arm and leg health on " C_W "%s" C_R " set to " C_G "%d" C_R ".]\r\n" C_0,
		       tch->name, tch->head_health );
	  }
     }
   
   else if ( !strcmp( cmd, "show" ) )
     {
	if ( !tch )
	  {
	     clientfr( "Target character is not in the database." );
	     return;
	  }
	
	do_char_show( tch );
     }
   else if ( !strcmp( cmd, "reset" ) )
     {
	int i;
	
	if ( !tch )
	  {
	     clientfr( "Target character is not in the database." );
	     return;
	  }
	
	for ( i = 0; i < PARTS; i++ )
	  tch->body[i] = 0;
	
	clientfr( "Damage on all body parts reset." );
     }
   
   else if ( !strcmp( cmd, "save" ) )
     {
	do_char_save( );
     }
   else if ( !strcmp( cmd, "load" ) )
     {
	do_char_load( );
     }
}



void do_decrypt( char *string )
{
   int change1, change2, change;
   char *p, chgstr[256];
   char *r, result[256];
   
   p = string;
   
   p = get_string( p, chgstr, 256 );
   change1 = atoi( chgstr );
   
   p = get_string( p, chgstr, 256 );
   change2 = atoi( chgstr );
   
   for ( change = change1; change <= change2; change++ )
     {
	p = string;
	r = result;
	p = get_string( p, chgstr, 256 );
	p = get_string( p, chgstr, 256 );
	
	while ( *p && *p != '\n' )
	  {
	     if ( ( *p < 'A' || *p > 'Z' ) &&
		  ( *p < 'a' || *p > 'z' ) )
	       *(r++) = *(p++);
	     else
	       {
		  *r = *(p++) + change;
		  
		  if ( ( *r < 'A' || *r > 'Z' ) &&
		       ( *r < 'a' || *r > 'z' ) )
		    *r = '?';
		  r++;
	       }
	  }
	
	*r = 0;
	
	clientff( C_R "[Res: '" C_W "%s" C_R "']\r\n" C_0, result );
     }
}


void print_matched_line( char *string, int match_start, int match_end )
{
   int start, end;
   
   /* Search for a newline backwards. */
   
   start = match_start;
   while ( start >= 0 && string[start] != '\n' )
     start--;
   if ( string[start] == '\n' )
     start++;
   
   /* Search for a newline foreward. */
   
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
	if( extra )
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


void set_target( char *name )
{
   CHAR_DATA *ch;
   
   free( target );
   if ( name[0] )
     target = strdup( name );
   else
     target = strdup( "none" );
   
   if ( target[0] >= 'a' && target[0] <= 'z' )
     target[0] += 'A' - 'a';
   
   for ( ch = characters; ch; ch = ch->next )
     if ( !strcmp( ch->name, target ) )
       break;
   
   if ( ch )
     do_char_show( ch );
   else
     clientff( C_R "Target: " C_G "%s" C_R "." C_0 "\r\n", target );
   
   show_prompt( );
}



int offensive_process_client_aliases( char *line )
{
   SCRIPT *script;
   FUNCTION *function;
   char command[4096], *args;
   
   DEBUG( "offensive_process_client_aliases" );
   
   if ( !strncmp( line, "alias ", 6 ) )
     {
	do_alias( line + 6 );
	show_prompt( );
	return 1;
     }
   else if ( !strcmp( line, "alias" ) )
     {
	clientff( "Syntax: alias add/del/list/load [args]\r\n" );
	show_prompt( );
	return 1;
     }
   else if ( !strncmp( line, "trigger ", 8 ) )
     {
	do_trigger( line + 8 );
	show_prompt( );
	return 1;
     }
   else if ( !strcmp( line, "load" ) )
     {
	load_scripts( );
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
   else if ( !strncmp( line, "call ", 5 ) )
     {
	CALL *call;
	SCRIPT *script;
	FUNCTION *f;
	char name[256];
	char buf[256];
	struct timeval call_start, call_end;
	int sec, usec;
	int first_arg = 1;
	int i;
	
	line = get_string( line + 5, name, 256 );
	if ( !name )
	  {
	     clientfr( "Call which function/alias?" );
	     show_prompt( );
	     return 1;
	  }
	
	f = NULL;
	for ( script = scripts; script; script = script->next )
	  for ( f = script->functions; f; f = f->next )
	    if ( !strcmp( name, f->name ) )
	      break;
	
	if ( !f )
	  {
	     clientfr( "Function not found. Perhaps you misspelled it?" );
	     show_prompt( );
	     return 1;
	  }
	
	call = prepare_call( f );
	if ( f->args )
	  for ( i = 0; f->args[i]; i++ )
	    {
	       line = get_string( line, buf, 256 );
	       if ( first_arg )
		 {
		    clientf( C_R "Args:" C_0 );
		    first_arg = 0;
		 }
	       clientff( " " C_W "%s" C_0 "=" C_g "\"%s\"" C_0, f->args[i], buf );
	       
	       set_local_variable( call, f->args[i], buf );
	    }
	if ( !first_arg )
	  clientf( ".\r\n" );
	clientff( C_R "Calling " C_W "%s" C_0 "...\r\n" C_0, f->name );
	
	gettimeofday( &call_start, NULL );
	
	call_function( call, buf, 256 );
	
	gettimeofday( &call_end, NULL );
	
	sec = call_end.tv_sec - call_start.tv_sec;
	usec = call_end.tv_usec - call_start.tv_usec;
	
	if ( usec < 0 )
	  sec -= 1, usec += 1000000;
	
	clientff( C_R "Execution time: " C_W "%d.%06d" C_R " seconds.\r\n" C_0, sec, usec );
	clientff( C_R "Return value: " C_g "\"%s\"" C_R ".\r\n" C_0, buf );
	
	show_prompt( );
	return 1;
     }
   else if ( !strncmp( line, "t ", 2 ) )
     {
	char tg[256];
	
	get_string( line + 2, tg, 256 );
	set_target( tg );
	
	return 1;
     }
   else if ( !strncmp( line, "sync ", 5 ) )
     {
	do_sync( line + 5 );
	return 1;
     }
   else if ( !strcmp( line, "syn" ) || !strncmp( line, "syn ", 4 ) )
     {
	do_sync_combo( line + 3 );
	return 1;
     }
   else if ( !strcmp( line, "hunt" ) )
     {
	char buf[256];
	if ( stance )
	  sprintf( buf, "ucp %s\nucp %s\nucp %s\nucp %s\n",
		   target, target, target, target );
	else
	  sprintf( buf, "brs\nucp %s\nucp %s\n",
		   target, target );
	send_to_server( buf );
	return 1;
     }
   else if ( !strncmp( line, "tm ", 3 ) )
     {
	do_time( line + 3 );
	show_prompt( );
	return 1;
     }
   else if ( !strncmp( line, "ch ", 3 ) )
     {
	do_character( line + 3 );
	show_prompt( );
	return 1;
     }
   else if ( !strncmp( line, "decrypt ", 8 ) )
     {
	do_decrypt( line + 8 );
	show_prompt( );
	return 1;
     }
   else if ( !strncmp( line, "grep ", 5 ) )
     {
	do_searchback( line + 5 );
	show_prompt( );
	return 1;
     }
   else if ( line[0] == 't' && line[1] > '0' && line[1] <= '9' )
     {
	int tg;
	
	tg = atoi( line+1 );
	
	if ( tg < 1 || tg > 31 )
	  return 0;
	
	if ( wholist_targets[tg-1][0] )
	  {
	     set_target( wholist_targets[tg-1] );
	  }
	else
	  {
	     clientfr( "No such target found." );
	     show_prompt( );
	  }
	return 1;
     }
   
   if ( !strncmp( line, "htell ", 6 ) )
     {
	char buf[4096];
	char buf2[4096];
	
	line = get_string( line + 6, buf, 4096 );
	if ( !line[0] )
	  {
	     clientf( C_R "[Htell what?]\r\n" C_0 );
	     show_prompt( );
	     return 1;
	  }
	
	/* First letter must be uppercase. */
	if ( line[0] >= 'a' && line[0] <= 'z' )
	  line[0] = line[0] - 'a' + 'A';
	
	sprintf( buf2, "mind hallucinate %s Whyte's voice reverberates silently within your mind, \"%s\"\r\n",
		 buf, line );
	clientff( "[%s]\r\n", buf2 );
	send_to_server( buf2 );
	return 1;
     }
   
   args = get_string( line, command, 4096 );
   
   /* Look around the function list. */
   
   for ( script = scripts; script; script = script->next )
     for ( function = script->functions; function; function = function->next )
       if ( function->alias && !strcmp( function->name, command ) )
	 {
	    CALL *call;
	    set_args( args );
	    
	    call = prepare_call( function );
	    call_function( call, NULL, 0 );
	    
	    return 1;
	 }
   
   return 0;
}

