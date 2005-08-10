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

#include <sys/time.h>
#include <pcre.h>

#include "module.h"


int offensive_version_major = 0;
int offensive_version_minor = 4;


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

typedef struct alias_data ALIAS_DATA;
typedef struct variable_data VARIABLE_DATA;
typedef struct trigger_data TRIGGER;
typedef struct char_data CHAR_DATA;

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


struct script_data
{
   char *body;
   
   
   
};


struct alias_data
{
   char *name;
   
   char *action;
   
   ALIAS_DATA *next;
};


struct variable_data
{
   char *name;
   
   char *value;
   
   VARIABLE_DATA *next;
};


struct trigger_data
{
   pcre *pattern;
   pcre_extra *extra;
   
   char *action;
   
   TRIGGER *next;
};


ALIAS_DATA *aliases;
VARIABLE_DATA *vars;
CHAR_DATA *characters;
TRIGGER *triggers;


/* Common/special variables. */

char *alias_arg[10]; /* $1, $5, etc. */
char *alias_fullarg; /* $* */
char *target; /* $t/$tar/$target */

int parsing_whohere_list;
char wholist_targets[32][256];

#define SB_FULL_SIZE 40960
#define SB_HALF_SIZE SB_FULL_SIZE / 2

char searchback_buffer[SB_FULL_SIZE];
int sb_size;

void execute_block( char *string );
void execute_line( char *string );


/* Here we register our functions. */

void offensive_module_init_data( );
void offensive_process_server_line_prefix( char *colorless_line, char *colorful_line, char *raw_line );
void offensive_process_server_line_suffix( char *colorless_line, char *colorful_line, char *raw_line );
void offensive_process_server_prompt_informative( char *line, char *rawline );
void offensive_process_server_prompt_action( char *rawline );
int  offensive_process_client_command( char *cmd );
int  offensive_process_client_aliases( char *cmd );


ENTRANCE( offensive_module_register )
{
   self->name = strdup( "IOffense" );
   self->version_major = offensive_version_major;
   self->version_minor = offensive_version_minor;
   
   self->init_data = offensive_module_init_data;
   self->process_server_line_prefix = offensive_process_server_line_prefix;
   self->process_server_line_suffix = offensive_process_server_line_suffix;
   self->process_server_prompt_informative = offensive_process_server_prompt_informative;
   self->process_server_prompt_action = offensive_process_server_prompt_action;
   self->process_client_command = NULL;
   self->process_client_aliases = offensive_process_client_aliases;
   self->build_custom_prompt = NULL;
   self->main_loop = NULL;
   self->update_descriptors = NULL;
   
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
#if defined( FOR_WINDOWS )
   gettimeofday = self->get_func( "gettimeofday" );
#endif
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



void caiman_targetting( char *line )
{
   const int aggressive_caimans[ ] = 
     { 12878, 15212, 18360, 19914, 25486, 27740, 31539, 34575, 0 };
   static int caiman;
   static int aggressive_caiman;
   static int last_caiman;
   static int color;
   char name[1024];
   int nr, i;
   
   /* Stop the red colour from bleeding. */
   if ( color )
     {
	clientf( C_0 );
	color = 0;
     }
   
   if ( caiman && !cmp( "Number of objects: *", line ) )
     {
	if ( aggressive_caiman )
	  caiman = aggressive_caiman;
	
	clientff( "Target: %s%d%s\r\n" C_0, aggressive_caiman ? C_R : C_G,
		  caiman, caiman == last_caiman ? C_W " *" : "" );
	
	sprintf( name, "%d", caiman );
	free( target );
	target = strdup( name );
	
	last_caiman = caiman;
	caiman = 0;
	aggressive_caiman = 0;
	
	return;
     }
   
   if ( line[0] != '\"' )
     return;
   
   line = get_string( line, name, 1024 );
   
   if ( cmp( "caiman*", name ) || cmp( "a hungry caiman", line ) )
     return;
   
   nr = atoi( name + 6 );
   if ( !nr )
     return;
   
   /* In case the last one is not dead yet. */
   if ( !last_caiman || caiman != last_caiman )
     caiman = nr;
   
   for ( i = 0; aggressive_caimans[i]; i++ )
     if ( nr == aggressive_caimans[i] )
       {
	  if ( caiman == nr )
	    aggressive_caiman = nr;
	  clientf( C_R );
	  color = 1;
       }
}



int check_pattern( TRIGGER *trigger, char *string )
{
   int ovector[30];
   int rc;
   
   rc = pcre_exec( trigger->pattern, trigger->extra, string, strlen( string ), 0, 0, ovector, 30 );
   
   if ( rc < 0 )
     return 0;
   
   return 1;
}



void offensive_process_server_line_prefix( char *colorless_line, char *colorful_line, char *raw_line )
{
   TRIGGER *trigger;
   int len;
   
   DEBUG( "offensive_process_server_line_prefix" );
   
   if ( !cmp( "^ rubs some salve on ^ ^.", colorless_line ) &&
	cmp( "^ rubs some salve on ^ skin.", colorless_line ) &&
	cmp( "^ rubs some salve on ^ body.", colorless_line ) )
     clientf( C_B );
   
   if ( !cmp( "^ touches a tree of life tattoo.", colorless_line ) )
     clientf( C_Y );
   
   if ( !cmp( "^ stands up and stretches ^ arms out wide.", colorless_line ) )
     clientf( C_W );
   
   if ( !cmp( "^ slowly pulls back *", colorless_line ) )
     clientf( C_W );
   
   caiman_targetting( colorless_line );
   
   /* Check for triggers. */
   for ( trigger = triggers; trigger; trigger = trigger->next )
     {
	if ( check_pattern( trigger, colorless_line ) )
	  execute_block( trigger->action );
     }
   
   /* Add the line in our search-back buffer. */
   len = strlen( colorless_line );
   
   if ( len )
     {
	if ( len + sb_size + 10 > SB_FULL_SIZE )
	  {
	     /* Copy the second half onto the first half. */
	     memmove( searchback_buffer, searchback_buffer + SB_HALF_SIZE, SB_HALF_SIZE );
	     sb_size -= SB_HALF_SIZE;
	  }
	
	memcpy( searchback_buffer + sb_size, colorless_line, len );
	memcpy( searchback_buffer + sb_size + len, "\n", 2 );
	sb_size += len + 1;
     }
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
	
	clientff( C_W " (" C_D "%d.%d" C_W ")" C_0, sec, usec );
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




void offensive_process_server_prompt_informative( char *line, char *rawline )
{
   DEBUG( "offensive_process_server_prompt_informative" );
   
   /* We may hit with something else. */
   if ( last_hit )
     last_hit = 0;
   
   if ( parsing_whohere_list )
     parsing_whohere_list = 0;
}



void offensive_process_server_prompt_action( char *line )
{
   DEBUG( "offensive_process_server_prompt_action" );
   
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
   if ( trigger->action )
     free( trigger->action );
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
   trigger->action = strdup( args );
   
   if ( error )
     clientff( C_R "[Pattern study error: %s]\r\n" C_0, error );
   
   clientfr( "Trigger added." );
}



void load_scripts( )
{
   FILE *fl;
   char buf[2048], *p;
   const char *flname = "scripts";
   
   fl = fopen( flname, "r" );
   
   if ( !fl )
     {
	clientff( C_R "[Can't open %s: %s]\r\n" C_0,
		  flname, strerror( errno ) );
	return;
     }
   
   silent = 1;
   
   while( 1 )
     {
	fgets( buf, 2048, fl );
   
	if ( feof( fl ) )
	  break;
	
	if ( buf[0] == '#' || buf[0] == ' ' || buf[0] == '\n' || !buf[0] )
	  continue;
	
	/* Remove the newline from it. */
	p = buf;
	while( *p && *p != '\n' )
	  p++;
	*p = 0;
	
	if ( !strncmp( buf, "alias ", 6 ) )
	  do_alias( buf + 6 );
     }
   
   silent = 0;
   
   fclose( fl );
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
	VARIABLE_DATA *var;
	for ( var = vars; var; var = var->next )
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



void script_echo( char *args )
{
   char echo[4096];
   
   convert_variables( args, echo );
   clientff( C_R "%s\r\n" C_0, echo );
}



void script_echo_prompt( char *args )
{
   show_prompt( );
}



void execute_line( char *string )
{
   char cmd[256], *args;
   
   while ( *string == ' ' && *string )
     string++;
   
   //clientff( C_R "exec: [%s]\r\n" C_0, string );
   
   args = get_string( string, cmd, 256 );
   
   /* Commands begin with a period. */
   if ( cmd[0] == '.' )
     {
	if ( !strcmp( cmd, ".echo" ) )
	  script_echo( args );
	else if ( !strcmp( cmd, ".prompt" ) )
	  script_echo_prompt( args );
     }
   /* A simple block, without any command. */
   else if ( cmd[0] == '{' )
     {
	char *block, *end;
	int blocks = 1;
	
	end = string;
	
	while ( blocks )
	  {
	     end++;
	     
	     if ( *end == '{' )
	       blocks++;
	     else if ( *end == '}' )
	       blocks--;
	     else if ( !*end )
	       {
		  clientfr( "Syntax error: Matching '}' not found in a script." );
		  return;
	       }
	  }
	
	block = calloc( 1, end - string );
	memcpy( block, string + 1, end - string - 1 );
	block[end - string - 1] = 0;
	
	execute_block( block );
	
	free( block );
     }
   else
     {
	char line[4096];
	
	/* Convert the variables, and send it away. */
	
	convert_variables( string, line );
	
	strcat( line, "\r\n" );
	
	send_to_server( line );
     }
}



/* Execute a block of commands.
 * This does not need a temporary buffer, as it leaves *string unchaged. */

void execute_block( char *string )
{
   int cmd_end;
   int finished = 0;
   char *cmd;
   int end_as_block = 0;
   
   //clientff( C_B "block: [%s]\r\n" C_0, string );
   
   cmd_end = 0;
   
   while ( 1 )
     {
	if ( string[cmd_end] == ';' || string[cmd_end] == 0 ||
	     string[cmd_end] == '{' )
	  {
	     if ( string[cmd_end] == 0 )
	       finished = 1;
	     
	     if ( string[cmd_end] == '{' )
	       {
		  /* Skip until the matching '}'. */
		  int blocks = 1;
		  
		  while ( blocks )
		    {
		       cmd_end++;
		       
		       if ( string[cmd_end] == '{' )
			 blocks++;
		       else if ( string[cmd_end] == '}' )
			 blocks--;
		       else if ( !string[cmd_end] )
			 {
			    clientfr( "Syntax error: Matching '}' not found in a script." );
			    return;
			 }
		    }
		  
		  end_as_block = 1;
	       }
	     
	     /*
	      * Make a copy of the piece, and execute it.
	      * Unless it's empty.
	      */
	     
	     if ( cmd_end + 1 + end_as_block > 1 )
	       {
		  cmd = calloc( 1, cmd_end + 1 + end_as_block );
		  memcpy( cmd, string, cmd_end );
		  if ( !end_as_block )
		    cmd[cmd_end] = 0;
		  else
		    {
		       cmd[cmd_end] = '}';
		       cmd[cmd_end+1] = 0;
		       end_as_block = 0;
		    }
		  
		  execute_line( cmd );
		  
		  free( cmd );
	       }
	     
	     string = string + cmd_end + 1;
	     cmd_end = -1;
	  }
	
	if ( finished )
	  break;
	
	cmd_end++;
     }
   
   //clientff( C_B "block end\r\n" C_0 );
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
   ALIAS_DATA *alias;
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
   
   /* Look around the alias list. */
   
   args = get_string( line, command, 4096 );
   
   for ( alias = aliases; alias; alias = alias->next )
     {
	char *block;
	
	if ( strcmp( command, alias->name ) )
	  continue;
	
	set_args( args );
	
	block = strdup( alias->action );
	
	execute_block( block );
	
	return 1;
     }
   
   return 0;
}

