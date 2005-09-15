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


/* Imperian specific code. */

#define IMPERIAN_ID "$Name$ $Id$"

#define TELCMDS
#define TELOPTS

#include "module.h"
#include "imperian.h"


int version_major = 1;
int version_minor = 9;

char *imperian_id = IMPERIAN_ID "\r\n" IMPERIAN_H_ID "\r\n" HEADER_ID "\r\n" MODULE_ID "\r\n";

/* The Great Tables. */
struct trigger_table_type *triggers;
struct defences_table_type *defences;
struct cure_table_type *cures;
struct affliction_table_type *afflictions;

/* Balances. */
int balance_herbs;
int balance_healing;
int balance_salve;
int balance_smoke;
int balance_cureelix;
int balance_toadstool;
int balance_left_arm;
int balance_right_arm;
int balance_tree;
int balance_kai_heal;
int balance_transmute;

int balance_purge;
int balance_focus;

int cure_with_tree;
int cure_with_focus;
int cure_with_purge;

/* Command checks. */
int waiting_for_herbs_command;
int waiting_for_smoke_command;
int waiting_for_salve_command;
int waiting_for_healing_command;
int waiting_for_toadstool_command;
int waiting_for_tree_command;
int waiting_for_purge_command;
int waiting_for_focus_command;

/* Spam prevention. */
int failed_herbs_commands;
int failed_smoke_commands;
int failed_salve_commands;
int failed_tree_commands;
int failed_healing_commands;
int failed_toadstool_commands;
int failed_focus_commands;
int failed_purge_commands;

/* Writhe. */
int writhing;
int writhe[WRITHE_LAST];

/*
 * Default level is 2, safe and dangerous trigger actions will
 * be executed, afflictions' suggestions won't.
 */
int trigger_level = 2;

/* Body parts. */
int partdamage[6];

/* Misc. */
int tripped;
int lines_until_prompt = 1;
int prompt_lines = 0;
int last_cure;
int last_cure_balance;
int ignore_next;
int changed;
int show_limbs;
int prompt_newline;
int show_on_term;
int stats_biggestname;
int keep_standing;
int sitting;
int applied_legs;
int applied_arms;
int double_actions;
int mark_tried;
int sleeping;
int line_since_prompt;
int parsing_diagnose;
int instakill_warning;
int parsing_fullsense;
int fullsense_nr;
int ignore_illusion;
char fullsense_persons[256][256];
int need_balance_for_defences;
int use_mana_above;
int clot_mana_usage = 5;
int bleeding;
int keep_pipes;
int pipes_lit;
int pipes_going_dark;
int normal_prompt;
int show_afflictions;
int mind_locked;
int kai_boost;
int arrows;
int arrows_timer;
char last_pipe[64];
int aeon;
int was_aeon;
int aeon_check;
int slow_check;
int tree_recovery = 30;
char lust[256];
int outr_once = 0;
int outr_toadstool = 0;
int curse_fear = 0;
int wildcard_location;
char selfishness_buffer[4096];
char taekate_stance;
int arti_pipes;
int reset_time;
int allow_diagdef;
int yoth;
int can_kipup;


/* One little powerful thing, that deserves a comment. ;) */
char prompt[4096];
char custom_prompt[4096];

/* Inventory parser. */
int inventory;
char inventory_buf[32767];
int rat_value;
int spider_value;
int serpent_value;
int caiman_value;
int locust_value;

/* Kick/punch counter, for each limb. */
//int hit_counter[7][12];
//int last_hit, last_hit_location;

/* Some prompt and player data. */
int p_hp, p_mana, p_end, p_will, p_kai;
int p_balance, p_equilibrium, p_prone, p_flying;
int p_blind, p_deaf, p_stunned;
int p_heal, p_toad_heal, p_toad_mana, p_heal_mana, p_heal_kai, p_heal_transmute;
char p_color_hp[16], p_color_mana[16], p_color_end[16];
char p_color_will[16], p_color_kai[16];

/* What's our max, in the tables? */
int data_nr;
int triggersnr;
int defencesnr;
int curesnr;
int afflictionsnr;
int data_errors;

char stats[30][256];


/* Some values that will be often used. */
int CURE_LAUREL;
int CURE_LOVAGE;
int CURE_CONCENTRATE;
int CURE_IMMUNITY;
int AFF_NONE = 0;
int AFF_ALL = 1;
int AFF_STUPIDITY;
int AFF_ANOREXIA;
int AFF_SLICKNESS;
int AFF_ASTHMA;
int AFF_PARALYSIS;
int AFF_HYPOCHONDRIA;
int AFF_MADNESS;
int AFF_DEMENTIA;
int AFF_STUPIDITY;
int AFF_CONFUSION;
int AFF_HYPERSOMNIA;
int AFF_PARANOIA;
int AFF_HALLUCINATIONS;
int AFF_IMPATIENCE;
int AFF_ADDICTION;
int AFF_RECKLESSNESS;
int AFF_MASOCHISM;
int AFF_SOMETHING;

/* Here we register our functions. */

void imperian_module_init_data( );
void imperian_process_server_line( LINE *line );
void imperian_process_server_prompt_informative( char *line, char *rawline );
void imperian_process_server_prompt_action( char *rawline );
void imperian_process_server_prompt( LINE *line );
int  imperian_process_client_command( char *cmd );
int  imperian_process_client_aliases( char *cmd );
char *imperian_build_custom_prompt( );
void imperian_mxp_enabled( );

ENTRANCE( imperian_module_register )
{
   self->name = strdup( "Imperian" );
   self->version_major = version_major;
   self->version_minor = version_minor;
   self->id = imperian_id;
   
   self->init_data = imperian_module_init_data;
   self->process_server_line = imperian_process_server_line;
   self->process_server_prompt = imperian_process_server_prompt;
   self->process_server_prompt_informative = imperian_process_server_prompt_informative;
   self->process_server_prompt_action = imperian_process_server_prompt_action;
   self->process_client_command = imperian_process_client_command;
   self->process_client_aliases = imperian_process_client_aliases;
   self->build_custom_prompt = imperian_build_custom_prompt;
   self->main_loop = NULL;
   self->update_descriptors = NULL;
   self->mxp_enabled = imperian_mxp_enabled;
   
   GET_FUNCTIONS( self );
}



/* If double_actions is on, then send that twice in some special cases. */
void send_to_server_d( char *str )
{
   send_to_server( str );
   if ( double_actions && ( afflictions[AFF_STUPIDITY].afflicted ||
			    partdamage[PART_HEAD] >= 2 ) )
     send_to_server( str );
}



/* Return the number of an affliction that begins with 'name' */
int get_affliction( char *name )
{
   int i;
   
   DEBUG( "get_affliction" );
   
   for ( i = 0; afflictions[i].name; i++ )
     {
	if ( !strcmp( afflictions[i].name, name ) )
	  return i;
     }
   
   return -1;
}


/* A fuction which ignores unprintable characters. */
void i_strip_unprint( char *string, char *dest )
{
   char *a, *b;
   
   DEBUG( "i_strip_unprint" );
   
   for ( a = string, b = dest; *a; a++ )
     if ( isprint( *a ) )
       {
	  *b = *a;
	  b++;
       }
   
   *b = 0;
}



void reset_outr( TIMER *self )
{
   if ( outr_once )
     {
	outr_once = 0;
   
	clientff( "\r\n" C_R "[" C_W "Option 'outrift once' set back to 0." C_R "]"
		  C_0 "\r\n" );
	show_prompt( );
     }
}




void load_options( char *section )
{
   FILE *fl;
   char buf[1025];
   int options_nr = 0;
   char current_section[1024];
   char name[1024], value_str[1024];
   int value;
   char *p;
   
   DEBUG( "load_options" );
   
   current_section[0] = 0;
   
   fl = fopen( "options", "r" );
   
   if ( !fl )
     {
	if ( strcmp( section, "init" ) )
	  {
	     sprintf( buf, C_R "[Can't open options file: " C_B "%s" C_R "]\r\n" C_0,
		      strerror( errno ) );
	     clientf( buf );
	  }
	
	return;
     }
   
   if ( strcmp( section, "init" ) )
     {
	sprintf( buf, C_R "[Loading from section " C_G "%s" C_R ".]\r\n" C_0,
		 section );
	clientf( buf );
     }
   
   while( 1 )
     {
	fgets( buf, 1024, fl );
	
	if ( feof( fl ) )
	  break;
	
	/* Increment line number. */
	options_nr++;
	
	/* Skip if empty/comment line. */
	if ( buf[0] == '#' || buf[0] == ' ' || buf[0] == '\n' || !buf[0] )
	  continue;
	
	/* Section? */
	if ( buf[0] == '[' )
	  {
	     if ( buf[1] != '\"' )
	       continue;
	     
	     get_string( buf+1, current_section, 1024 );
//	     debugf( "Section: [%s]", current_section );
	     continue;
	  }
	
	/* Not current section? Continue then. */
	if ( current_section[0] && strcmp( section, current_section ) )
	  continue;
	
	/* Now, load em up. */
	
	if ( buf[0] == 'D' && buf[1] == ' ' )
	  {
	     int defence = -1, i;
	     
	     /* Example: D "quince" on, or D "all" off */
	     p = buf + 2;
	     p = get_string( p, name, 1024 );
	     p = get_string( p, value_str, 1024 );
	     
	     value = !strncmp( value_str, "on", 2 ) ? 1 : 0;
	     
	     if ( !strcmp( name, "all" ) )
	       {
		  for ( i = 0; defences[i].name; i++ )
		    defences[i].keepon = value;
	       }
	     else
	       {
		  for ( i = 0; defences[i].name; i++ )
		    if ( !strcmp( defences[i].name, name ) )
		      {
			 defence = i;
			 break;
		      }
		  
		  if ( defence == -1 )
		    {
		       debugf( "load_options: no such defence, \"%s\", found.", name );
		       continue;
		    }
		  
		  defences[defence].keepon = value;
	       }
	  }
	else if ( buf[0] == 'P' && buf[1] == ' ' )
	  {
	     /* Example: P "<1/233 (you are dead)> " or P 0 */
	     if ( buf[2] == '0' )
	       {
		  custom_prompt[0] = 0;
	       }
	     else
	       {
		  p = buf + 2;
		  p = get_string( p, custom_prompt, 4096 );
	       }
	  }
	else
	  {
	     /* Example: health 150 */
	     p = buf;
	     p = get_string( p, name, 1024 );
	     p = get_string( p, value_str, 1024 );
	     value = atoi( value_str );
	     
	     if ( !strcmp( name, "level" ) )
	       trigger_level = value < 0 || value > 3 ? 2 : value;
	     else if ( !strcmp( name, "health" ) )
	       p_heal = value >= 0 ? value : 0;
	     else if ( !strcmp( name, "mana" ) )
	       p_heal_mana = value >= 0 ? value : 0;
	     else if ( !strcmp( name, "toadstool_heal" ) )
	       p_toad_heal = value >= 0 ? value : 0;
	     else if ( !strcmp( name, "toadstool_mana" ) )
	       p_toad_mana = value >= 0 ? value : 0;
	     else if ( !strcmp( name, "kai_heal" ) )
	       p_heal_kai = value >= 0 ? value : 0;
	     else if ( !strcmp( name, "transmute" ) )
	       p_heal_transmute = value >= 0 ? value : 0;
	     else if ( !strcmp( name, "keep_standing" ) )
	       keep_standing = value ? 1 : 0;
	     else if ( !strcmp( name, "double_actions" ) )
	       double_actions = value ? 1 : 0;
	     else if ( !strcmp( name, "show_info_on_term" ) )
	       show_on_term = value ? 1 : 0;
	     else if ( !strcmp( name, "keep_pipes_lit" ) )
	       keep_pipes = value ? 1 : 0;
	     else if ( !strcmp( name, "use_mana_above" ) )
	       use_mana_above = value >= 0 ? value : 0;
	     else if ( !strcmp( name, "clot_mana_usage" ) )
	       clot_mana_usage = value >= 0 ? value : 0;
	     else if ( !strcmp( name, "cure_with_tree" ) )
	       cure_with_tree = value ? 1 : 0;
	     else if ( !strcmp( name, "cure_with_focus" ) )
	       cure_with_focus = value ? 1 : 0;
	     else if ( !strcmp( name, "cure_with_purge" ) )
	       cure_with_purge = value ? 1 : 0;
	     else if ( !strcmp( name, "show_afflictions" ) )
	       show_afflictions = value ? 1 : 0;
	     else if ( !strcmp( name, "tree_recovery" ) )
	       tree_recovery = value >= 1 ? value : 1;
	     else if ( !strcmp( name, "arti_pipes" ) )
	       arti_pipes = value ? 1 : 0;
	     else if ( !strcmp( name, "reset_time" ) )
	       reset_time = value >= 1 ? value : 1;
	     else if ( !strcmp( name, "can_kipup" ) )
	       can_kipup = value ? 1 : 0;
	     else if ( !strcmp( name, "outr_once" ) )
	       {
		  int j;
		  outr_once = value ? 1 : 0;
		  for ( j = 0; cures[j].name; j++ )
		    cures[j].outr = 0;
		  outr_toadstool = 0;
		  if ( outr_once )
		    add_timer( "outr_limit", 1800, reset_outr, 0, 0, 0 );
		  else
		    del_timer( "outr_limit" );
	       }
	  }
	
//	debugf( "Loading: N:[%s] V:[%d]", name, value );
     }
   
   fclose( fl );
}



/* Fills one table entry, could cause problems when user misspells. */
/* Remake. */
void triggers_table_update( char *str )
{
   static char buf[256];
   char *p;
   int special = 0;
   int i;
   
   DEBUG( "triggers_table_update" );
   
   triggersnr++;
   i = triggersnr - 1;
   
   triggers = realloc( triggers, (triggersnr+1) * sizeof( struct trigger_table_type ) );
   
   /* Clear the trigger after this one. */
   memset( &triggers[i+1], 0, sizeof( struct trigger_table_type ) );
   
   p = str;
   
   /* Reset all values. */
   triggers[i].action = NULL;
   triggers[i].defence = NULL;
   triggers[i].special = 0;
   triggers[i].affliction = 0;
   triggers[i].gives = 0;
   triggers[i].missing_chars = 0;
   triggers[i].level = 0;
   triggers[i].silent = 0;
   triggers[i].nocolors = 0;
   
   /* Trigger message. */
   p = get_string( p, buf, 256 );
   if ( buf[0] )
     triggers[i].message = strdup( buf );
   else
     {
	debugf( "data (line %d): trigger: expected trigger pattern.", data_nr );
	data_errors++;
	triggers[i].message = strdup( "(null string)" );
     }
   
   /* Trigger action. (affliction/special) */
   p = get_string( p, buf, 256 );
   if ( buf[0] == '\0' )
     {
	debugf( "data (line %d): trigger: expected affliction, special or none.", data_nr );
	data_errors++;
	triggers[i].affliction = 0, triggers[i].special = SPEC_NORMAL;
     }
   else if ( !strncmp( buf, "spec", 4 ) )
     special = 1;
   else if ( !strncmp( buf, "none", 4 ) )
     special = 2;
   else
     {
	triggers[i].affliction = get_affliction( buf );
	if ( triggers[i].affliction < 0 )
	  {
	     debugf( "data (line %d): trigger: unknown affliction '%s'.", data_nr, buf );
	     data_errors++;
	     triggers[i].affliction = AFF_NONE;
	  }
     }
   
   /* Special = 0 (affliction, expect give/cure) */
   if ( special == 0 )
     {
	triggers[i].special = SPEC_NORMAL;
	triggers[i].level = 3;
	
	p = get_string( p, buf, 256 );
	if ( !strcmp( buf, "give" ) )
	  triggers[i].gives = 1;
	else if ( !strcmp( buf, "cure" ) )
	  triggers[i].gives = 0;
	else
	  {
	     debugf( "data (line %d): trigger for %s: expected give or cure, got '%s'.", data_nr, afflictions[triggers[i].affliction].name, buf );
	     data_errors++;
	     triggers[i].gives = 0;
	  }
     }
   else
     {
	/* Let's make sure it doesn't do something else. */
	triggers[i].special = SPEC_NOTHING;
	triggers[i].level = 2;
	
	if ( special != 2 )
	  {
	     p = get_string( p, buf, 256 );
	     if ( buf[0] == '\0' )
	       {
		  debugf( "data (line %d): special trigger: expected type of special.", data_nr );
		  data_errors++;
		  triggers[i].special = SPEC_NOTHING;
	       }
	     
	     /* Body part damage triggers. */
	     else if ( !strcmp( buf, "part" ) )
	       {
		  triggers[i].special = SPEC_PARTDAMAGE;
		  
		  /* Which part? */
		  p = get_string( p, buf, 256 );
		  if ( buf[0] != '\0' )
		    {
		       if ( !strcmp( buf, "head" ) )
			 triggers[i].p_part = PART_HEAD;
		       else if ( !strcmp( buf, "body" ) || !strcmp( buf, "torso" ) )
			 triggers[i].p_part = PART_TORSO;
		       else if ( !strcmp( buf, "leftarm" ) )
			 triggers[i].p_part = PART_LEFTARM;
		       else if ( !strcmp( buf, "rightarm" ) )
			 triggers[i].p_part = PART_RIGHTARM;
		       else if ( !strcmp( buf, "leftleg" ) )
			 triggers[i].p_part = PART_LEFTLEG;
		       else if ( !strcmp( buf, "rightleg" ) )
			 triggers[i].p_part = PART_RIGHTLEG;
		       else if ( !strcmp( buf, "arm" ) )
			 triggers[i].p_part = PART_ARM;
		       else if ( !strcmp( buf, "leg" ) )
			 triggers[i].p_part = PART_LEG;
		       else if ( !strcmp( buf, "any" ) )
			 triggers[i].p_part = PART_ANY;
		       else if ( !strcmp( buf, "onearm" ) )
			 triggers[i].p_part = PART_ONEARM;
		       else if ( !strcmp( buf, "oneleg" ) )
			 triggers[i].p_part = PART_ONELEG;
		       else
			 {
			    debugf( "data (line %d): special trigger: unknown body part '%s'.", data_nr, buf );
			    data_errors++;
			    triggers[i].p_part = PART_NONE;
			 }
		    }
		  /* And how is it affected? */
		  p = get_string( p, buf, 256 );
		  triggers[i].p_level = PART_NONE;
		  if ( buf[0] != '\0' )
		    {
		       if ( !strcmp( buf, "healed" ) )
			 triggers[i].p_level = PART_HEALED;
		       else if ( !strcmp( buf, "crippled" ) )
			 triggers[i].p_level = PART_CRIPPLED;
		       else if ( !strcmp( buf, "mangled" ) )
			 triggers[i].p_level = PART_MANGLED;
		       else if ( !strcmp( buf, "mangled2" ) )
			 triggers[i].p_level = PART_MANGLED2;
		       else
			 {
			    debugf( "data (line %d): special trigger: unknown damage level '%s'.", data_nr, buf );
			    data_errors++;
			 }
		    }
	       }
	     
	     /* Triggers which tell us if we've been tripped down. */
	     else if ( !strcmp( buf, "trip" ) )
	       {
		  triggers[i].special = SPEC_TRIP;
		  triggers[i].level = 1;
	       }
	     else if ( !strcmp( buf, "ignorenext" ) )
	       {
		  triggers[i].special = SPEC_IGNORENEXT;
	       }
	     else if ( !strcmp( buf, "instakill" ) )
	       {
		  triggers[i].special = SPEC_INSTAKILL;
	       }
	     else if ( !strcmp( buf, "defence" ) )
	       {
		  triggers[i].special = SPEC_DEFENCE;
		  triggers[i].level = 1;
		  
		  /* The defence this trigger will change. */
		  p = get_string( p, buf, 256 );
		  if ( buf[0] )
		    triggers[i].defence = strdup( buf );
		  else
		    {
		       debugf( "data (line %d): special trigger: expected defence name.", data_nr );
		       data_errors++;
		       triggers[i].defence = strdup( "(null string)" );
		    }
		  
		  /* Does it turn the defence on, or off? */
		  p = get_string( p, buf, 256 );
		  if ( !strcmp( buf, "give" ) )
		    triggers[i].gives = 1;
		  else if ( !strcmp( buf, "take" ) )
		    triggers[i].gives = 0;
		  else
		    {
		       debugf( "data (line %d): special trigger: expected give or take.", data_nr );
		       data_errors++;
		       triggers[i].gives = 0;
		    }
	       }
	     else if ( !strcmp( buf, "writhe" ) )
	       {
		  triggers[i].special = SPEC_WRITHE;
		  triggers[i].level = 2;
		  
		  p = get_string( p, buf, 256 );
		  if ( !strcmp( buf, "webs" ) )
		    triggers[i].writhe = WRITHE_WEBS;
		  else if ( !strcmp( buf, "ropes" ) )
		    triggers[i].writhe = WRITHE_ROPES;
		  else if ( !strcmp( buf, "impale" ) )
		    triggers[i].writhe = WRITHE_IMPALE;
		  else if ( !strcmp( buf, "arrow" ) )
		    triggers[i].writhe = WRITHE_ARROW;
		  else if ( !strcmp( buf, "transfix" ) )
		    triggers[i].writhe = WRITHE_TRANSFIX;
		  else if ( !strcmp( buf, "dart" ) )
		    triggers[i].writhe = WRITHE_DART;
		  else if ( !strcmp( buf, "strangle" ) )
		    triggers[i].writhe = WRITHE_STRANGLE;
		  else if ( !strcmp( buf, "bolt" ) )
		    triggers[i].writhe = WRITHE_BOLT;
		  else if ( !strcmp( buf, "net" ) )
		    triggers[i].writhe = WRITHE_NET;
		  else if ( !strcmp( buf, "grapple" ) )
		    triggers[i].writhe = WRITHE_GRAPPLE;
		  else
		    {
		       debugf( "data (line %d): special trigger: unknown writhe type '%s'.", data_nr, buf );
		       data_errors++;
		       triggers[i].writhe = 0;
		    }
		  
		  p = get_string( p, buf, 256 );
		  if ( !strcmp( buf, "begin" ) )
		    triggers[i].gives = 2;
		  else if ( !strcmp( buf, "cure" ) )
		    triggers[i].gives = 0;
		  else if ( !strcmp( buf, "give" ) )
		    triggers[i].gives = 1;
		  else
		    {
		       debugf( "data (line %d): special trigger: expected give, cure, or begin.", data_nr );
		       data_errors++;
		       triggers[i].gives = 1;
		    }
	       }
	     else if ( !strcmp( buf, "arrows" ) )
	       {
		  triggers[i].special = SPEC_ARROWS;
		  triggers[i].level = 1;
		  
		  p = get_string( p, buf, 256 );
		  
		  if ( !strcmp( buf, "add" ) )
		    triggers[i].gives = 1;
		  else if ( !strcmp( buf, "remove" ) )
		    triggers[i].gives = 0;
		  else if ( !strcmp( buf, "clear" ) )
		    triggers[i].gives = 2;
		  else
		    triggers[i].gives = 1;
	       }
	     else if ( !strcmp( buf, "aeon" ) )
	       {
		  triggers[i].special = SPEC_AEON;
		  triggers[i].level = 2;
		  
		  p = get_string( p, buf, 256 );
		  if ( !strcmp( buf, "cure" ) )
		    triggers[i].gives = 0;
		  else if ( !strcmp( buf, "give" ) )
		    triggers[i].gives = 1;
		  else
		    {
		       debugf( "data (line %d): special trigger: expected give or cure.", data_nr );
		       data_errors++;
		       triggers[i].gives = 1;
		    }
	       }
	     else if ( !strcmp( buf, "slow" ) )
	       {
		  triggers[i].special = SPEC_SLOW;
		  triggers[i].level = 2;
	       }
	     else if ( !strcmp( buf, "yoth" ) )
	       {
		  triggers[i].special = SPEC_YOTH;
		  triggers[i].level = 2;
		  
		  p = get_string( p, buf, 256 );
		  if ( !strcmp( buf, "cure" ) )
		    triggers[i].gives = 0;
		  else if ( !strcmp( buf, "give" ) )
		    triggers[i].gives = 1;
		  else
		    {
		       debugf( "data (line %d): special trigger: expected give or cure.", data_nr );
		       data_errors++;
		       triggers[i].gives = 1;
		    }
	       }
	  }
     }
   
   /* X marks the spot.. err.. marks extended options. */
   p = get_string( p, buf, 256 );
   if ( buf[0] != 'x' && buf[0] != 'X' )
     return;
   
   /* Go on extended, so keep looking for options. */
   while( 1 )
     {
	p = get_string( p, buf, 256 );
	if ( buf[0] == '\0' )
	  break;
	
	/* Trigger level. */
	if ( buf[0] == 'L' || buf[0] == 'l' )
	  {
	     p = get_string( p, buf, 256 );
	     if ( isdigit( buf[0] ) )
	       triggers[i].level = atoi( buf );
	     
	     if ( !triggers[i].level )
	       {
		  debugf( "data (line %d): extended options on trigger: level not specified.", data_nr );
		  data_errors++;
	       }
	  }
	/* User defined action. */
	else if ( buf[0] == 'A' || buf[0] == 'a' )
	  {
	     p = get_string( p, buf, 256 );
	     if ( buf[0] != '\0' )
	       triggers[i].action = strdup( buf );
	     else
	       {
		  debugf( "data (line %d): extended options on trigger: action not specified.", data_nr );
		  data_errors++;
	       }
	  }
	/* Has missing characters? */
	else if ( buf[0] == 'M' || buf[0] == 'm' )
	  {
	     p = get_string( p, buf, 256 );
	     if ( isdigit( buf[0] ) )
	       triggers[i].missing_chars = atoi( buf );
	     
	     if ( !triggers[i].missing_chars )
	       {
		  debugf( "data (line %d): extended options on trigger: number of missing characters not specified.", data_nr );
		  data_errors++;
	       }
	  }
	/* Should we send anything to the client? */
	else if ( buf[0] == 'S' || buf[0] == 's' )
	  {
	     triggers[i].silent = 1;
	  }
	/* Compare it to a color stripped version of the line. */
	else if ( buf[0] == 'C' || buf[0] == 'c' )
	  {
	     triggers[i].nocolors = 1;
	  }
	/* Is it on the 'F'irst line or not the first (!F) line? */
	else if ( buf[0] == 'F' || buf[0] == 'f' )
	  {
	     triggers[i].first_line = 1;
	  }
	else if ( buf[0] == '!' && ( buf[1] == 'F' || buf[1] == 'f' ) )
	  {
	     triggers[i].first_line = 2;
	  }
	else
	  {
	     debugf( "data (line %d): trigger: unknown special '%s'.", data_nr, buf );
	     data_errors++;
	  }
     }
}


void defences_table_update( char *str )
{
   static char buf[256];
   char *p;
   int i;
   
   DEBUG( "defences_table_update" );
   
   defencesnr++;
   i = defencesnr - 1;
   
   defences = realloc( defences, (defencesnr+1) * sizeof( struct defences_table_type ) );
   
   /* Clear the defence after this one. */
   memset( &defences[i+1], 0, sizeof( struct defences_table_type ) );
   
   /* Skip the initial "D " */
   p = str+2;
   
   if ( strncmp( p, "defence ", 8 ) )
     {
	debugf( "data (line %d): defence 'unkown': syntax error.", data_nr );
	data_errors++;
	return;
     }
   
   /* To be noted that .on is not set to 0. */
   defences[i].tried = 0;
   defences[i].keepon = 0;
   defences[i].needy = 0;
   defences[i].level = 1;
   defences[i].action = NULL;
   
   defences[i].needherb = 0;
   defences[i].takeherb = 0;
   defences[i].needsalve = 0;
   defences[i].takesalve = 0;
   defences[i].needsmoke = 0;
   defences[i].takesmoke = 0;
   
   /* Skip the initial "defence " */
   p += 8;
   
   /* Defence name. */
   p = get_string( p, buf, 256 );
   if ( buf[0] )
     defences[i].name = strdup( buf );
   else
     {
	debugf( "data (line %d): defence 'unkown': expected defence name.", data_nr );
	data_errors++;
	defences[i].name = strdup( "(null string)" );
     }
   
   /* We'll need the biggest name, for some formatting in build_stats. */
   if ( strlen( defences[i].name ) > stats_biggestname )
     stats_biggestname = strlen( defences[i].name );
   
   /* Do we have some extended options here? */
   p = get_string( p, buf, 256 );
   if ( buf[0] != 'x' && buf[0] != 'X' )
     return;
   
   /* Keep looking for options. */
   while( 1 )
     {
	p = get_string( p, buf, 256 );
	if ( buf[0] == '\0' )
	  break;
	
	/* User defined action. */
	if ( buf[0] == 'A' || buf[0] == 'a' )
	  {
	     p = get_string( p, buf, 256 );
	     if ( buf[0] != '\0' )
	       defences[i].action = strdup( buf );
	     else
	       {
		  debugf( "data (line %d): defence '%s': action not specified.", data_nr, defences[i].name );
		  data_errors++;
	       }
	  }
	/* Does it require balance and equilibrium? */
	else if ( !strcmp( buf, "needy" ) )
	  defences[i].needy = 1;
	else if ( !strcmp( buf, "standing" ) )
	  defences[i].standing = 1;
	else if ( !strcmp( buf, "needherb" ) )
	  defences[i].needherb = 1;
	else if ( !strcmp( buf, "takeherb" ) )
	  defences[i].takeherb = 1;
	else if ( !strcmp( buf, "needsalve" ) )
	  defences[i].needsalve = 1;
	else if ( !strcmp( buf, "takesalve" ) )
	  defences[i].takesalve = 1;
	else if ( !strcmp( buf, "needsmoke" ) )
	  defences[i].needsmoke = 1;
	else if ( !strcmp( buf, "takesmoke" ) )
	  defences[i].takesmoke = 1;
	else if ( !strcmp( buf, "takebalance" ) )
	  defences[i].takebalance = 1;
	else if ( !strcmp( buf, "notimer" ) )
	  defences[i].nodelay = 1;
	/* Defence level. */
	else if ( buf[0] == 'L' || buf[0] == 'l' )
	  {
	     p = get_string( p, buf, 256 );
	     if ( isdigit( buf[0] ) )
	       defences[i].level = atoi( buf );
	     if ( !defences[i].level )
	       {
		  debugf( "data (line %d): defence '%s': level not specified.", data_nr, defences[i].name );
		  data_errors++;
	       }
	  }
	else
	  {
	     debugf( "data (line %d): defence '%s': unknown special option '%s'.", data_nr, defences[i].name, buf );
	     data_errors++;
	  }
     }
}



/* Parse one cure line, and add it in the cures table. */
void cures_table_update( char *str )
{
   char buf[1024];
   char *p;
   int i;
   
   DEBUG( "cures_table_update" );
   
   curesnr++;
   i = curesnr - 1;
   
   cures = realloc( cures, (curesnr+1) * sizeof( struct cure_table_type ) );
   
   p = str + 2;
   
   p = get_string( p, buf, 1024 );
   if ( buf[0] )
     cures[i].name = strdup( buf );
   else
     {
	cures[i].name = strdup( "unnamed" );
	debugf( "data (line %d): cure 'unknown': syntax error in cure's name.", data_nr );
	data_errors++;
     }
   
   p = get_string( p, buf, 1024 );
   if ( buf[0] )
     cures[i].action = strdup( buf );
   else
     cures[i].action = strdup( "unknown" );
   
   p = get_string( p, buf, 1024 );
   if ( !strcmp( buf, "herb" ) )
     cures[i].balance_type = BAL_HERB;
   else if ( !strcmp( buf, "salve" ) )
     cures[i].balance_type = BAL_SALVE;
   else if ( !strcmp( buf, "smoke" ) )
     cures[i].balance_type = BAL_SMOKE;
   else if ( !strcmp( buf, "none" ) || !buf[0] )
     cures[i].balance_type = BAL_NONE;
   else
     {
	cures[i].balance_type = BAL_NONE;
	debugf( "data (line %d): cure '%s': syntax error in cure's balance type.", data_nr, cures[i].name );
	data_errors++;
     }
   
   p = get_string( p, buf, 1024 );
   if ( buf[0] )
     cures[i].fullname = strdup( buf );
   else
     cures[i].fullname = NULL;
   
   /* Clear the cure after this one. */
   memset( &cures[i+1], 0, sizeof( struct cure_table_type ) );
   
   /* Set a few of the most used cures. */
   if ( !strcmp( cures[i].name, "laurel" ) )
     CURE_LAUREL = i;
   else if ( !strcmp( cures[i].name, "lovage" ) )
     CURE_LOVAGE = i;
   else if ( !strcmp( cures[i].name, "concentrate" ) )
     CURE_CONCENTRATE = i;
   else if ( !strcmp( cures[i].name, "immunity" ) )
     CURE_IMMUNITY = i;
}


/* Part of afflictions_table_update */
int get_data_cure( char *buf )
{
   int i;
   
   if ( !strcmp( buf, "n/a" ) )
     return 0;
   
   if ( !buf[0] )
     {
	debugf( "data (line %d): affliction: cure '%s' cannot be found.", data_nr, buf );
	data_errors++;
	return 0;
     }
   
   for ( i = 0; cures[i].name; i++ )
     if ( !strcmp( buf, cures[i].name ) )
       return i;
   
   data_errors++;
   return 0;
}



void afflictions_table_update( char *str )
{
   char buf[1024];
   char *p;
   int i;
   
   DEBUG( "afflictions_table_update" );
   
   afflictionsnr++;
   i = afflictionsnr - 1;
   
   afflictions = realloc( afflictions, (afflictionsnr+1) * sizeof( struct affliction_table_type ) );
   
   p = str + 2;
   
   p = get_string( p, buf, 1024 );
   if ( buf[0] )
     afflictions[i].name = strdup( buf );
   else
     {
	afflictions[i].name = strdup( "unnamed" );
	debugf( "data (line %d): affliction 'unknown': syntax error in affliction's name.", data_nr );
	data_errors++;
     }
   
   p = get_string( p, buf, 1024 );
   afflictions[i].herbs_cure = get_data_cure( buf );
   
   p = get_string( p, buf, 1024 );
   afflictions[i].smoke_cure = get_data_cure( buf );
   
   p = get_string( p, buf, 1024 );
   afflictions[i].salve_cure = get_data_cure( buf );
   
   p = get_string( p, buf, 1024 );
   if ( buf[0] )
     {
	if ( !strcmp( buf, "yes" ) )
	  afflictions[i].focus = 1;
	else
	  {
	     afflictions[i].focus = 0;
	     if ( strcmp( buf, "no" ) )
	       {
		  debugf( "data (line %d): affliction '%s': expected yes or no for Focus.", data_nr, afflictions[i].name );
		  data_errors++;
	       }
	  }
     }
   else
     {
	afflictions[i].focus = 0;
	debugf( "data (line %d): affliction '%s': expected yes or no for Focus.", data_nr, afflictions[i].name );
	data_errors++;
     }
   
   p = get_string( p, buf, 1024 );
   if ( buf[0] )
     {
	if ( !strcmp( buf, "yes" ) )
	  afflictions[i].purge_blood = 1;
	else
	  {
	     afflictions[i].purge_blood = 0;
	     if ( strcmp( buf, "no" ) )
	       {
		  debugf( "data (line %d): affliction '%s': expected yes or no for Purge Blood.", data_nr, afflictions[i].name );
		  data_errors++;
	       }
	  }
     }
   else
     {
	afflictions[i].purge_blood = 0;
	debugf( "data (line %d): affliction '%s': expected yes or no for Purge Blood.", data_nr, afflictions[i].name );
	data_errors++;
     }
   
   p = get_string( p, buf, 1024 );
   afflictions[i].special = get_data_cure( buf );
   
   p = get_string( p, buf, 1024 );
   if ( buf[0] )
     afflictions[i].diagmsg = strdup( buf );
   else
     afflictions[i].diagmsg = NULL;
   
   /* Clear the affliction after this one. */
   memset( &afflictions[i+1], 0, sizeof( struct affliction_table_type ) );
   
   /* Set a few of the most used afflictions. */
   if ( !strcmp( afflictions[i].name, "stupidity" ) )
     AFF_STUPIDITY = i;
   else if ( !strcmp( afflictions[i].name, "anorexia" ) )
     AFF_ANOREXIA = i;
   else if ( !strcmp( afflictions[i].name, "slickness" ) )
     AFF_SLICKNESS = i;
   else if ( !strcmp( afflictions[i].name, "asthma" ) )
     AFF_ASTHMA = i;
   else if ( !strcmp( afflictions[i].name, "paralysis" ) )
     AFF_PARALYSIS = i;
   else if ( !strcmp( afflictions[i].name, "hypochondria" ) )
     AFF_HYPOCHONDRIA = i;
   else if ( !strcmp( afflictions[i].name, "madness" ) )
     AFF_MADNESS = i;
   else if ( !strcmp( afflictions[i].name, "dementia" ) )
     AFF_DEMENTIA = i;
   else if ( !strcmp( afflictions[i].name, "stupidity" ) )
     AFF_STUPIDITY = i;
   else if ( !strcmp( afflictions[i].name, "confusion" ) )
     AFF_CONFUSION = i;
   else if ( !strcmp( afflictions[i].name, "hypersomnia" ) )
     AFF_HYPERSOMNIA = i;
   else if ( !strcmp( afflictions[i].name, "paranoia" ) )
     AFF_PARANOIA = i;
   else if ( !strcmp( afflictions[i].name, "hallucinations" ) )
     AFF_HALLUCINATIONS = i;
   else if ( !strcmp( afflictions[i].name, "impatience" ) )
     AFF_IMPATIENCE = i;
   else if ( !strcmp( afflictions[i].name, "addiction" ) )
     AFF_ADDICTION = i;
   else if ( !strcmp( afflictions[i].name, "recklessness" ) )
     AFF_RECKLESSNESS = i;
   else if ( !strcmp( afflictions[i].name, "masochism" ) )
     AFF_MASOCHISM = i;
   else if ( !strcmp( afflictions[i].name, "something" ) )
     AFF_SOMETHING = i;
}



void load_data( )
{
   char buf[512], clean_buf[512];
   int i;
   FILE *fl;
   
   DEBUG( "load_data" );
   
   data_errors = 0;
   
   /* First, clean it up. */
   
   if ( triggers )
     {
	for ( i = 0; triggers[i].message; i++ )
	  {
	     free( triggers[i].message );
	     free( triggers[i].action );
	     if ( triggers[i].defence )
	       free( triggers[i].defence );
	     triggers[i].message = NULL;
	  }
	
	free( triggers );
     }
   
   if ( defences )
     {
	for ( i = 0; defences[i].name; i++ )
	  {
	     free( defences[i].name );
	     if ( defences[i].action )
	       free( defences[i].action );
	     defences[i].name = NULL;
	  }
	
	free( defences );
     }
   
   if ( cures )
     {
	for ( i = 0; cures[i].name; i++ )
	  {
	     free( cures[i].name );
	     
	     if ( cures[i].action )
	       free( cures[i].action );
	     if ( cures[i].fullname )
	       free( cures[i].fullname );
	  }
	
	free( cures );
     }
   
   if ( afflictions )
     {
	for ( i = 0; afflictions[i].name; i++ )
	  {
	     free( afflictions[i].name );
	     
	     if ( afflictions[i].diagmsg )
	       free( afflictions[i].diagmsg );
	  }
	
	free( afflictions );
     }
   
   /* Initialize the tables. */
   
   triggers = calloc( 1, sizeof( struct trigger_table_type ) );
   triggers[0].message = NULL;
   
   defences = calloc( 1, sizeof( struct defences_table_type ) );
   defences[0].name = NULL;
   
   cures = calloc( 2, sizeof( struct cure_table_type ) );
   cures[0].name = strdup( "none" );
   cures[0].action = strdup( "none" );
   cures[1].name = NULL;
   
   afflictions = calloc( 3, sizeof( struct affliction_table_type ) );
   afflictions[0].name = strdup( "none" );
   afflictions[1].name = strdup( "all" );
   afflictions[2].name = NULL;
   
   triggersnr = 0;
   defencesnr = 0;
   curesnr = 1;
   afflictionsnr = 2;
   
   stats_biggestname = 0;
   
   /* Now read it all again. */
   
   fl = fopen( "data", "r" );
   if ( !fl )
     {
	debugerr( "data" );
	return;
     }
   
   /* Line number. */
   data_nr = 0;
   
   while( 1 )
     {
	fgets( buf, 256, fl );
	
	if ( feof( fl ) )
	  break;
	
	/* Increment line number. */
	data_nr++;
	
	/* Skip if empty/comment line. */
	if ( buf[0] == '#' || buf[0] == ' ' || buf[0] == '\n' || !buf[0] )
	  continue;
	
	/* Strip weird characters. */
	i_strip_unprint( buf, clean_buf );
	
	if ( clean_buf[0] == '"' )
	  triggers_table_update( clean_buf );
	else if ( clean_buf[0] == 'D' || clean_buf[0] == 'd' )
	  defences_table_update( clean_buf );
	else if ( clean_buf[0] == 'C' || clean_buf[0] == 'c' )
	  cures_table_update( clean_buf );
	else if ( clean_buf[0] == 'A' || clean_buf[0] == 'a' )
	  afflictions_table_update( clean_buf );
	else
	  {
	     debugf( "Syntax error in data file, on line %d.", data_nr );
	     fclose( fl );
	     return;
	  }
     }
   
   fclose( fl );
   
   /* Also load our options, from the options file, too. */
   load_options( "init" );
}


void imperian_module_init_data( void )
{
   DEBUG( "imperian_init_data" );
   
   load_data( );
}


/* Damned repetitive thing. I'll make a balance[] table some day. */
void balance_reset( TIMER *self )
{
   if ( self->data[0] == 1 )
     {
	clientf( "\r\n" );
	if ( self->data[1] == 1 && failed_tree_commands > 1 )
	  {
	     clientfr( C_G "*" C_B "Unable to handle tree balance." C_G "*" C_R );
	     failed_tree_commands = 0;
	  }
	else
	  {
	     balance_tree = 0;
	     clientfr( C_D "Tree balance recovered." C_R );
	     if ( self->data[1] == 1 )
	       failed_tree_commands++;
	  }
	waiting_for_tree_command = 0;
	show_prompt( );
     }
   else if ( self->data[0] == 2 )
     {
	clientf( "\r\n" );
	if ( failed_herbs_commands > 1 )
	  {
	     clientfr( C_G "*" C_B "Unable to handle herbs balance." C_G "*" C_R );
	     failed_herbs_commands = 0;
	  }
	else
	  {
	     balance_herbs = 0;
	     failed_herbs_commands++;
	     clientfr( C_D "Herbs balance reset." C_R );
	  }
	show_prompt( );
	waiting_for_herbs_command = 0;
     }
   else if ( self->data[0] == 3 )
     {
	clientf( "\r\n" );
	if ( failed_salve_commands > 1 )
	  {
	     clientfr( C_G "*" C_B "Unable to handle salve balance." C_G "*" C_R );
	     failed_salve_commands = 0;
	  }
	else
	  {
	     balance_salve = 0;
	     failed_salve_commands++;
	     clientfr( C_D "Salve balance reset." C_R );
	  }
	show_prompt( );
	waiting_for_salve_command = 0;
     }
   else if ( self->data[0] == 4 )
     {
	clientf( "\r\n" );
	if ( failed_smoke_commands > 1 )
	  {
	     clientfr( C_G "*" C_B "Unable to handle smoke balance." C_G "*" C_R );
	     failed_smoke_commands = 0;
	  }
	else
	  {
	     balance_smoke = 0;
	     failed_smoke_commands++;
	     pipes_lit = 0;
	     clientfr( C_D "Pipe balance reset." C_R );
	  }
	show_prompt( );
	waiting_for_smoke_command = 0;
     }
   else if ( self->data[0] == 5 )
     {
	clientf( "\r\n" );
	if ( failed_healing_commands > 1 )
	  {
	     clientfr( C_G "*" C_B "Unable to handle healing balance." C_G "*" C_R );
	  }
	else
	  {
	     balance_healing = 0;
	     failed_healing_commands++;
	     clientfr( C_D "Healing balance reset." C_R );
	  }
	show_prompt( );
	waiting_for_healing_command = 0;
     }
   else if ( self->data[0] == 6 )
     {
	clientf( "\r\n" );
	if ( failed_toadstool_commands > 1 )
	  {
	     clientfr( C_G "*" C_B "Unable to handle toadstool balance." C_G "*" C_R );
	     failed_toadstool_commands = 0;
	  }
	else
	  {
	     balance_toadstool = 0;
	     outr_toadstool = 0;
	     failed_toadstool_commands++;
	     clientfr( C_D "Toadstool balance reset." C_R );
	  }
	show_prompt( );
	waiting_for_toadstool_command = 0;
     }
   else if ( self->data[0] == 7 )
     {
	clientf( "\r\n" );
	if ( failed_focus_commands > 1 )
	  {
	     clientfr( C_G "*" C_B "Unable to handle focus balance." C_G "*" C_R );
	     waiting_for_focus_command = 0;
	     failed_focus_commands = 0;
	  }
	else
	  {
	     balance_focus = 0;
	     clientfr( C_D "Focus balance recovered." C_R );
	     failed_focus_commands++;
	  }
	show_prompt( );
     }
   else if ( self->data[0] == 8 )
     {
	clientf( "\r\n" );
	if ( failed_purge_commands > 1 )
	  {
	     clientfr( C_G "*" C_B "Unable to handle purge blood balance." C_G "*" C_R );
	     waiting_for_purge_command = 0;
	     failed_purge_commands = 0;
	  }
	else
	  {
	     balance_purge = 0;
	     clientfr( C_D "Purge Blood balance recovered." C_R );
	     failed_purge_commands++;
	  }
	show_prompt( );
     }
   else
     debugf( "Unknown balance, on balance_reset." );
}



void check_cure_line( char *string )
{
   int i, len;
   
   DEBUG( "check_cure_line" );
   
   /* strlen( "You quickly eat " ) == 16 */
   len = strlen( string + 16 ) - 1;
   
   for ( i = 0; cures[i].name; i++ )
     {
	if ( !cures[i].fullname )
	  continue;
	
	if ( strncmp( string + 16, cures[i].fullname, len ) )
	  continue;
	
	last_cure = i;
	return;
     }
   
   last_cure = 0;
}



void check_outrift( char *string )
{
   int i;
   char *c;
   
   DEBUG( "check_cure_line" );
   
   if ( !outr_once )
     return;
   
   /* strlen( "You remove 1 " ) == 13 */
   
   if ( !strncmp( string + 13, "toadstool", 9 ) )
     {
	outr_toadstool = 1;
	return;
     }
   
   for ( i = 0; cures[i].name; i++ )
     {
	if ( !cures[i].fullname )
	  continue;
	
	/* I hate this. */
	c = !strncmp( cures[i].fullname, "an ", 3 ) ? cures[i].fullname + 3 : cures[i].fullname + 2;
	
	if ( strncmp( string + 13, c, strlen( c ) ) )
	  continue;
	
	cures[i].outr = 1;
	return;
     }
   
   last_cure = 0;
}



void check_arm_balance( char *string )
{
   /* Did we use our arms for something? */
   if ( !cmp( "You lash out with a quick jab at *", string ) ||
	!cmp( "You unleash a powerful hook towards *", string ) ||
	!cmp( "You launch a powerful uppercut at *", string ) ||
	!cmp( "You throw your force behind a forward palmstrike at *", string ) ||
	!cmp( "You ball up one fist and hammerfist *", string ) ||
	!cmp( "You form a spear hand and stab out towards *", string ) ||
	!cmp( "You feint to *, and he drops ^ guard.", string ) )
     {
	if ( balance_left_arm )
	  balance_right_arm = 1;
	else
	  balance_left_arm = 1;
     }
   
   /* There's a trick here. If an arm is broken, we can't know which
    * we used first. */
   if ( !strcmp( string, "You have regained left arm balance." ) )
     {
	if ( balance_left_arm )
	  balance_left_arm = 0;
	else
	  balance_right_arm = 0;
     }
   else if ( !strcmp( string, "You have regained right arm balance." ) )
     {
	if ( balance_right_arm )
	  balance_right_arm = 0;
	else
	  balance_left_arm = 0;
     }
}



/*
 * Checks a string to see if it matches a trigger string.
 * String "He*lo *orld!" matches trigger "Hello world!", if missing_chars is 2.
 * String "Hello world!" matches trigger "Hel*orld!", and will set willcard_location.
 * String "Hello world!" matches trigger "Hello ^!". (single word)
 * 
 * Returns 0 if the strings match, 1 if not.
 */

int trigger_cmp( char *trigger, char *string, int missing_chars )
{
   char *t, *s;
   int reverse = 0;
   
   DEBUG( "trigger_cmp" );
   
   wildcard_location = -1;
   
   /* We shall begin from the start line. */
   t = trigger, s = string;
   
   /* Ready.. get set.. Go! */
   while ( 1 )
     {
	/* Are we checking in reverse? */
	if ( !reverse )
	  {
	     /* The end? */
	     if ( !*t && !*s )
	       return 0;
	     
	     /* One got faster to the end? */
	     if ( ( !*t || !*s ) && *t != '*' )
	       return 1;
	  }
	else
	  {
	     /* End of reverse? */
	     if ( t >= trigger && *t == '*' )
	       return 0;
	     
	     /* No '*' found? */
	     if ( s < string )
	       return 1;
	  }
	
        if ( *t == '^' )
	  {
	     if ( !reverse )
	       {
		  while ( isalpha( *s ) )
		    s++;
		  t++;
	       }
	     else
	       {
		  while( s >= string && isalpha( *s ) )
		    s--;
		  t--;
	       }
	  }
	
	if ( *t != *s )
	  {
	     if ( *t == '*' )
	       {
		  /* Wildcard found, reversing search. */
		  reverse = 1;
		  
		  /* Set its location, some triggers will need this. */
		  wildcard_location = s - string;
		  
		  /* Move them at the end. */
		  t = t + strlen( t );
		  s = s + strlen( s );
	       }
	     else if ( missing_chars > 0 )
	       {
		  /* Allow it to continue. */
		  missing_chars--;
	       }
	     else
	       {
		  /* Chars differ, strings don't match. */
		  return 1;
	       }
	  }
	
	/* Run. Backwards if needed. */
	if ( !reverse )
	  t++, s++;
	else
	  t--, s--;
     }
}



/* Greatest thing ever invented! Diagnose! */
int parse_diagnose( char *line )
{
   static int was_aeon;
   char buf[256];
   int i, dmg = 0;
   int left_align = 60;
   int simple_diag = 0;
   
   DEBUG( "parse_diagnose" );
   
   /* If it's the case, then enter Diag mode. */
   if ( line_since_prompt == 1 && !strcmp( line, "You are:" ) )
     {
	/* Make sure we sent the command. */
	if ( !allow_diagdef )
	  {
	     ignore_illusion = 1;
	     clientff( C_W " (" C_G "i" C_W ")" C_0 );
	     return 1;
	  }
	
	parsing_diagnose = 1;
	
	/* Remove all afflictions that have their message known. */
	for ( i = 0; afflictions[i].name; i++ )
	  {
	     if ( !afflictions[i].afflicted ||
		  !afflictions[i].diagmsg )
	       continue;
	     
	     afflictions[i].afflicted = 0;
	  }
	
	/* Heal all limbs. */
	for ( i = 0; i <= PART_LAST; i++ )
	  partdamage[i] = PART_HEALED;
	
	/* Clear the writhe balances, that we know the diag message of. */
	writhe[WRITHE_WEBS] = 0;
	writhe[WRITHE_ROPES] = 0;
	writhe[WRITHE_TRANSFIX] = 0;
	writhe[WRITHE_IMPALE] = 0;
	
	was_aeon = aeon;
	aeon = 0;
	yoth = 0;
	
	/* Remove the defences Insomnia, Blindness and Deafness. */
	for ( i = 0; defences[i].name; i++ )
	  {
	     if ( !strcmp( defences[i].name, "insomnia" ) ||
		  !strcmp( defences[i].name, "blindness" ) ||
		  !strcmp( defences[i].name, "deafness" ) )
	       defences[i].on = 0;
	  }
	
	return 1;
     }
   
   if ( !parsing_diagnose )
     return 0;
   
   /* Check for afflictions. */
   for ( i = 0; afflictions[i].name; i++ )
     {
	if ( !afflictions[i].diagmsg ||
	     cmp( afflictions[i].diagmsg, line ) )
	  continue;
	
	afflictions[i].afflicted = 1;
	
	if ( simple_diag )
	  {
	     sprintf( buf, C_W " (" C_R "%s" C_W ")" C_0, afflictions[i].name );
	  }
	else
	  {
	     sprintf( buf, " %*s " C_R "%s" C_W "  (" C_G "%s" C_W ")" C_0,
		      left_align - (int) strlen( line ), C_D "-" C_0,
		      afflictions[i].name, cures[afflictions[i].herbs_cure].name );
	  }
	clientf( buf );
	
	return 1;
     }
   
   /* Check for defences. */
   if ( !strcmp( line, "an insomniac." ) )
     {
	for ( i = 0; defences[i].name; i++ )
	  {
	     if ( !strcmp( defences[i].name, "insomnia" ) )
	       defences[i].on = 1;
	  }
     }
   
   if ( !strcmp( line, "blind." ) )
     {
	for ( i = 0; defences[i].name; i++ )
	  {
	     if ( !strcmp( defences[i].name, "blindness" ) )
	       defences[i].on = 1;
	  }
     }
   
   if ( !strcmp( line, "deaf." ) )
     {
	for ( i = 0; defences[i].name; i++ )
	  {
	     if ( !strcmp( defences[i].name, "deafness" ) )
	       defences[i].on = 1;
	  }
     }
   
   if ( !strcmp( line, "shriveled throat." ) )
     yoth = 1;
   
   if ( !strcmp( line, "afflicted with the curse of the Aeon." ) )
     aeon = was_aeon ? was_aeon : 1;
   
   /* Check for writhe messages. */
   if ( !strcmp( line, "entangled in strands of webbing." ) )
     writhe[WRITHE_WEBS] = 1;
   else if ( !strcmp( line, "entangled in ropes." ) )
     writhe[WRITHE_ROPES] = 1;
   else if ( !strcmp( line, "transfixed." ) )
     writhe[WRITHE_TRANSFIX] = 1;
   else if ( !strcmp( line, "impaled." ) )
     writhe[WRITHE_IMPALE] = 1;
   
   /* Check for body parts. */
   if ( !trigger_cmp( "afflicted by a crippled *.", line, 0 ) )
     dmg = PART_CRIPPLED;
   else if ( !trigger_cmp( "has a partially damaged *.", line, 0 ) )
     dmg = PART_MANGLED;
   else if ( !trigger_cmp( "has a mangled *.", line, 0 ) )
     dmg = PART_MANGLED2;
   
   if ( dmg )
     {
	char *p = line + wildcard_location;
	
	if ( wildcard_location < 0 || wildcard_location > strlen( line ) )
	  {
	     debugf( "Diagnose: Bad wildcard position, %d in line '%s'.", wildcard_location, line );
	  }
	
	if ( !strncmp( p, "left arm", 8 ) )
	  partdamage[PART_LEFTARM] = dmg;
	else if ( !strncmp( p, "right arm", 9 ) )
	  partdamage[PART_RIGHTARM] = dmg;
	else if ( !strncmp( p, "left leg", 8 ) )
	  partdamage[PART_LEFTLEG] = dmg;
	else if ( !strncmp( p, "right leg", 9 ) )
	  partdamage[PART_RIGHTLEG] = dmg;
	else if ( !strncmp( p, "head", 4 ) )
	  partdamage[PART_HEAD] = PART_CRIPPLED;
	
	return 1;
     }
   
   if ( !strcmp( line, "has a serious concussion." ) )
     partdamage[PART_HEAD] = PART_MANGLED;
   else if ( !strcmp( line, "has mild internal trauma." ) )
     partdamage[PART_TORSO] = PART_CRIPPLED;
   else if ( !strcmp( line, "has serious internal trauma." ) )
     partdamage[PART_TORSO] = PART_MANGLED;
   else
     return 0;
   
   return 1;
}



int parse_full_sense( char *line )
{
   DEBUG( "parse_fullsense" );
   
   /* From here, a big list begins. */
   if ( line_since_prompt == 1 &&
	!strcmp( line, "You seek out all mental presences within your reach:" ) )
     {
	parsing_fullsense = 1;
	fullsense_nr = 0;
	return 1;
     }
   
   if ( !parsing_fullsense )
     return 0;
   
   if ( !strncmp( line, "You sense ", 10 ) )
     {
	strcpy( fullsense_persons[fullsense_nr], line );
	fullsense_nr++;
	return 1;
     }
   else
     {
	int len = strlen( fullsense_persons[fullsense_nr] );
	
	if ( fullsense_nr - 1 < 0 )
	  {
	     debugf( "What's this?! Fullsense illusion?" );
	     return 1;
	  }
	
	/* Add a space to the original line, if there isn't any. */
	if ( len > 0 && fullsense_persons[fullsense_nr][len-1] != ' ' )
	  {
	     fullsense_persons[fullsense_nr][len+1] = 0;
	     fullsense_persons[fullsense_nr][len] = ' ';
	  }
	
	strcat( fullsense_persons[fullsense_nr-1], line );
	return 1;
     }
   
   /* Don't ask me why this return is here. *hum* */
   return 0;
}



void check_fullsense_list( )
{
   char buf[256];
   char *pos;
   int i, count = 0;
   
   DEBUG( "check_fullsense_list" );
   
   /* Could be empty, or with one (you) person. */
   if ( fullsense_nr < 2 )
     return;
   
   /* Keep only the room names in it. */
   for ( i = 0; i < fullsense_nr; i++ )
     {
	pos = strstr( fullsense_persons[i], " at " );
	
	if ( !pos )
	  {
	     debugf( "Can't find AT in fullsense." );
	     continue;
	  }
	
	strcpy( buf, pos );
	strcpy( fullsense_persons[i], buf );
     }
   
   /* Now count all that look like the last one. */
   for ( i = 0; i < fullsense_nr - 1; i++ )
     {
	if ( !strcmp( fullsense_persons[i], fullsense_persons[fullsense_nr - 1] ) )
	  count++;
     }
   
   if ( count )
     {
	prefixf( C_R "[" C_B "You sense %d other person%s in this room."
		 C_R "]" C_0 "\r\n", count, count == 1 ? "" : "s" );
     }
   else
     prefix( C_R "[" C_B "You sense that you are alone in this room." C_R "]" C_0 "\r\n" );
}



void check_bleeding( )
{
   int can_clot, clots, i;
   
   /* Bleeding more than 10 health? Ouch, let's fix that. */
   if ( bleeding < 10 || !use_mana_above || !clot_mana_usage )
     return;
   
   /* Hopefully we don't run out of mana, by doing so. */
   if ( p_mana < use_mana_above )
     return;
   
   /* How many can we do, with our current mana level? */
   can_clot = ( p_mana - use_mana_above ) / clot_mana_usage;
   if ( can_clot <= 0 )
     return;
   
   /* And how many should we do? One clot will take one bleeding. */
   clots = can_clot > bleeding / 2 ? bleeding / 2 : can_clot;
   
   clientff( C_B "(" C_W "clot * %d" C_B ") " C_0, clots );
   
   /* Good, go ahead and send it to the server, "clots" times. */
   for ( i = 0; i < clots; i++ )
     send_to_server( "clot\r\n" );
   
   /* But also make sure we don't do this at each prompt we get. */
   bleeding = 0;
}


int have_balance( )
{
   if ( !p_equilibrium || !p_balance )
     return 0;
   
   if ( balance_left_arm || balance_right_arm )
     return 0;
   
   return 1;
}



void affliction_check( int trigger )
{
   if ( triggers[trigger].affliction == AFF_ANOREXIA && triggers[trigger].gives )
     {
	/* This is what happens when eating while having anorexia. */
	if ( !strcmp( triggers[trigger].message,
		      "The thought of eating sickens you." ) )
	  {
	     balance_herbs = 0;
	     waiting_for_herbs_command = 0;
	     del_timer( "herb_balance_reset" );
	     
	     waiting_for_toadstool_command = 0;
	     failed_toadstool_commands = 0;
	     del_timer( "toadstool_balance_reset" );
	  }
     }
   
   if ( triggers[trigger].affliction == AFF_SLICKNESS && triggers[trigger].gives )
     {
	/* Also, this is what happens when applying while having slickness. */
	if ( !strcmp( triggers[trigger].message,
		      "The salve cannot absorb into your slimy skin." ) )
	  {
	     balance_salve = 0;
	     /* Timer is reset, as the apply message is still received. */
	  }
     }
   
   if ( triggers[trigger].affliction == AFF_ASTHMA && triggers[trigger].gives )
     {
	/* Guess what. Same thing. Asthma. */
	if ( !strcmp( triggers[trigger].message,
		      "Your heavy lungs are much too constricted to smoke *" ) )
	  {
	     failed_smoke_commands = 0;
	     waiting_for_smoke_command = 0;
	     del_timer( "smoke_balance_reset" );
	     balance_smoke = 0;
	     
	     /* Reset all defences that need pipe balance. */
	     
	     /* ... */
	  }
     }
}


void parse_writhe( char *line )
{
   if ( !strcmp( line, "You begin to writhe helplessly, throwing your body off balance." ) )
     {
	int i;
	
	/* Reset only Impale, if that's the case. Otherwise, all. */
	
	if ( writhing == 2 )
	  writhe[WRITHE_IMPALE] = 0;
	else
	  for ( i = 0; i < WRITHE_LAST; i++ )
	    writhe[i] = 0;
	
	writhing = 0;
	del_timer( "reset_writhe" );
	
	return;
     }
}


void check_demonray( char *ray )
{
   if ( ray[strlen( ray )-1] == ',' )
     ray[strlen( ray )-1] = 0;
   
   if ( !strcmp( ray, "orange" ) )
     {
	afflictions[AFF_PARALYSIS].afflicted = 1;
	changed = 1;
     }
   else if ( !strcmp( ray, "indigo" ) )
     {
	afflictions[AFF_STUPIDITY].afflicted = 1;
	changed = 1;
     }
   else if ( !strcmp( ray, "violet" ) )
     {
	int i;
	for ( i = 0; defences[i].name; i++ )
	  {
	     if ( strcmp( defences[i].name, "heightened speed" ) )
	       continue;
	     
	     if ( defences[i].on )
	       defences[i].on = 0, aeon_check = 1;
	     else
	       aeon = 1;
	     break;
	  }
     }
}



void reset_mindlocking( TIMER *self )
{
   if ( mind_locked == 2 )
     mind_locked = 0;
}



void parse_runes( char *line )
{
   char name[256];
   char *rune, *aff;
   char *end;
   
   if ( cmp( "A rune * is sketched in slot ^.", line ) )
     return;
   
   end = strstr( line, " is sketched in" );
   
   if ( !end || end <= line + 7 )
     return;
   
   /* Too big? */
   if ( end - line - 7 > 64 )
     return;
   
   strncpy( name, line + 7, end - line - 7 );
   name[end - line - 7] = 0;
   
   if ( !strcmp( name, "like an open eye" ) )
     rune = "wunjo", aff = "cure blind";
   else if ( !strcmp( name, "that looks like a stick man" ) )
     rune = "inguz", aff = "paralysis";
   else if ( !strcmp( name, "shaped like a butterfly" ) )
     rune = "nairat", aff = "transfix";
   else if ( !strcmp( name, "like a closed eye" ) )
     rune = "fehu", aff = "sleep";
   else if ( !strcmp( name, "resembling a bell" ) )
     rune = "mannaz", aff = "cure deaf";
   else if ( !strcmp( name, "that looks like a nail" ) )
     rune = "sowulu", aff = "damage";
   else if ( !strcmp( name, "that looks like something out of your nightmares" ) )
     rune = "kena", aff = "fear";
   else if ( !strcmp( name, "shaped like a viper" ) )
     rune = "sleizak", aff = "cyanide";
   else if ( !strcmp( name, "resembling an apple core" ) )
     rune = "loshre", aff = "anorexia";
   else if ( !strcmp( name, "resembling a square box" ) )
     rune = "pithakhan", aff = "drain mana";
   else if ( !strcmp( name, "like a lightning bolt" ) )
     rune = "uruz", aff = "heal";
   else if ( !strcmp( name, "resembling a leech" ) )
     rune = "nauthiz", aff = "hunger";
   else if ( !strcmp( name, "" ) )
     rune = "", aff = "";
   else
     {
	clientf( C_R " (" C_B "unknown!" C_R ")" C_0 );
	return;
     }
   
   clientff( C_R " (" C_D "%s:" C_0 " %s" C_R ")" C_0, rune, aff );
   
   /* Personal runes:
    * shaped like a mighty oak - jera - +1 con
    * resembling an elk - algiz - protection
    * like a lion - berkana - health regen
    */
}


void parse_stances( char *line )
{
   if ( !cmp( "You ease yourself out of the ^ stance.", line ) )
     taekate_stance = 0;
   if ( !cmp( "You drop your legs into a sturdy Horse stance.", line ) )
     taekate_stance = 'h';
   if ( !cmp( "You draw back and balance into the Eagle stance.", line ) )
     taekate_stance = 'e';
   if ( !cmp( "You tense your muscles and look about sharply as you take the stance of the*", line ) )
     taekate_stance = 'c';
   if ( !cmp( "You draw yourself up to full height and roar aloud, adopting the Bear stance.", line ) )
     taekate_stance = 'b';
   if ( !cmp( "You take the Rat stance.", line ) )
     taekate_stance = 'r';
   if ( !cmp( "You sink back into the menacing stance of the Scorpion.", line ) )
     taekate_stance = 's';
   if ( !cmp( "You allow the form of the Dragon to fill your mind and govern your actions.", line ) )
     taekate_stance = 'd';
}



int parse_special( LINE *l )
{
   char *line = l->line;
   char *colorless_line = l->line;
   char *colorful_line = l->raw_line;
   int i;
   
   /* What line is this? Might help us, in the future, to know. */
   line_since_prompt++;
   
   if ( !strcmp( "You force your command into the unwilling mind of your victim.", line ) ||
	!cmp( "^'s aura of weapons rebounding disappears.", line ) ||
	!strcmp( "reflections", line ) ||
	!strcmp( "weirdshield", line ) )
     line_since_prompt--;
   
   if ( !cmp( "Your pipe has gone cold and dark.", l->line ) )
     {
	pipes_lit = 0;
	pipes_going_dark++;
	
	if ( pipes_going_dark > 1 )
	  {
	     l->gag_entirely = 1;
	     l->gag_ending = 1;
	  }
     }
   
   /* We have a cure after we ate something, so it's good. */
   if ( ( last_cure || last_cure_balance ) &&
	cmp( "Your insomnia has cleared up.", line ) &&
	cmp( "Your insomnia has been cured by the mandrake.", line ) &&
	cmp( "Your mind relaxes and you feel as if you could sleep.", line ) )
     {
	last_cure = 0;
	/* Focus. */
	if ( last_cure_balance == 1 || last_cure_balance == 2 )
	  last_cure_balance = 0;
     }
   
   /* Most messages begin with the "You" or "Your". Don't bother with the rest. */
   if ( !cmp( "You*", colorless_line ) )
     {
	/* Check special (balance) triggers first. */
	if ( !cmp( "You may drink another healing elixir.", line ) )
	  {
	     balance_healing = 0;
	     return 1;
	  }
	else if ( !cmp( "You may eat another herb or plant.", line ) )
	  {
	     balance_herbs = 0;
	     return 1;
	  }
	else if ( !cmp( "You may drink another affliction-healing elixir.", line ) )
	  {
	     balance_cureelix = 0;
	     return 1;
	  }
	else if ( !cmp( "You have recovered your breath and can smoke once more.", line ) )
	  {
	     balance_smoke = 0;
	     return 1;
	  }
	else if ( !cmp( "You may apply another salve.", line ) )
	  {
	     balance_salve = 0;
	     return 1;
	  }
	else if ( !cmp( "You may eat another toadstool.", line ) )
	  {
	     balance_toadstool = 0;
	     return 1;
	  }
	else if ( !cmp( "Your mind is able to focus once again.", line ) )
	  {
	     balance_focus = 0;
	     return 1;
	  }
	else if ( !cmp( "You have regained the ability to purge your body.", line ) )
	  {
	     balance_purge = 0;
	     return 1;
	  }
	
	else if ( !cmp( "You quickly eat *", line ) &&
		  cmp( "You quickly eat a toadstool.", line ) &&
		  cmp( "You quickly eat a violet root.", line ) &&
		  cmp( "You quickly eat a quince nut.", line ) &&
		  cmp( "You quickly eat a prickly pear.", line ) )
	  {
	     failed_herbs_commands = 0;
	     waiting_for_herbs_command = 0;
	     del_timer( "herb_balance_reset" );
	     
	     check_cure_line( line );
	     return 1;
	  }
	else if ( !cmp( "You remove 1 *", line ) )
	  {
	     check_outrift( line );
	  }
	else if ( line_since_prompt == 1 &&
		  !cmp( "You take a long drag off your pipe, filling your lungs with laurel smoke.", line ) )
	  {
	     failed_smoke_commands = 0;
	     waiting_for_smoke_command = 0;
	     del_timer( "smoke_balance_reset" );
	     
	     /* This is a trick. The cure messages are -before- this one, unlike eating herbs. */
	     last_cure = CURE_LAUREL;
	  }
	else if ( line_since_prompt == 1 &&
	     !cmp( "You take a long drag off your pipe, filling your lungs with lovage smoke.", line ) )
	  {
	     failed_smoke_commands = 0;
	     waiting_for_smoke_command = 0;
	     del_timer( "smoke_balance_reset" );
	     
	     /* Same story. */
	     last_cure = CURE_LOVAGE;
	  }
	else if ( line_since_prompt == 1 &&
	     !cmp( "You take a long drag off your pipe, filling your lungs with linseed smoke.", line ) )
	  {
	     failed_smoke_commands = 0;
	     waiting_for_smoke_command = 0;
	     del_timer( "smoke_balance_reset" );
	  }
	else if ( !cmp( "You quickly rub some salve on your *", line ) )
	  {
	     failed_salve_commands = 0;
	     waiting_for_salve_command = 0;
	     del_timer( "salve_balance_reset" );
	     
	     if ( !cmp( "legs.", line + 35 ) )
	       {
		  applied_legs = 1;
		  return 1;
	       }
	     else if ( !cmp( "arms.", line + 35 ) )
	       {
		  applied_arms = 1;
		  return 1;
	       }
	  }
	else if ( !cmp( "You messily spread the salve over your body, to no effect.", line ) )
	  {
	     if ( applied_arms || applied_legs )
	       {
		  if ( applied_legs )
		    {
		       partdamage[PART_LEFTLEG] = PART_HEALED;
		       partdamage[PART_RIGHTLEG] = PART_HEALED;
		    }
		  if ( applied_arms )
		    {
		       partdamage[PART_LEFTARM] = PART_HEALED;
		       partdamage[PART_RIGHTARM] = PART_HEALED;
		    }
		  
		  applied_arms = 0;
		  applied_legs = 0;
		  balance_salve = 0;
	       }
	  }
	
	else if ( !cmp( "You feel your health and mana replenished.", line ) )
	  {
	     waiting_for_toadstool_command = 0;
	     failed_toadstool_commands = 0;
	     del_timer( "toadstool_balance_reset" );
	  }
	else if ( !cmp( "You close your eyes, curl up in a ball, and fall asleep.", line ) ||
		  !cmp( "You feel incredibly tired, and fall asleep immediately.", line ) ||
		  !cmp( "Your exhausted mind can stay awake no longer, and you fall into a deep sleep.", line ) )
	  {
	     sleeping = 1;
	  }
	else if ( !cmp( "You are asleep and can do nothing. WAKE will attempt to wake you.", line ) )
	  {
	     if ( !sleeping )
	       sleeping = 1;
	  }
	else if ( !cmp( "You open your eyes and stretch languidly, feeling deliciously well-rested.", line ) ||
		  !cmp( "You fall over from your sitting position, waking up as you crash on the ground.", line ) ||
		  !cmp( "You open your eyes and yawn mightily.", line ) ||
		  !cmp( "You are jerked awake by the pain.", line ) ||
		  !cmp( "You quickly pull yourself out of sleep.", line ) ||
		  !cmp( "You already are awake.", line ) )
	  {
	     sleeping = 0;
	  }
	else if ( !cmp( "You have regained your mental equilibrium.", line ) ||
		  !cmp( "You have recovered balance.", line ) )
	  {
	     /* This will be used to notify the defence upkeeper. */
	     /* It will be turned to zero, on the next prompt, to make sure
	      * both balance and equilibrium are there. */
	     
	     if ( need_balance_for_defences )
	       need_balance_for_defences = 2;
	  }
	else if ( !cmp( "You quickly light your pipes, surrounding yourself with a cloud of smoke.", line ) )
	  {
	     pipes_lit = 1;
	  }
	else if ( !cmp( "You carefully light your treasured pipe until it is smoking nicely.", line ) )
	  {
//	     if ( pipes_lit < 3 )
//	       pipes_lit++;
	  }
	else if ( !cmp( "You have regained ^ arm balance.", line ) )
	  {
	     if ( sitting == 2 )
	       sitting = 1;
	  }
	else if ( !cmp( "You are holding:", colorless_line ) )
	  {
	     inventory = 1;
	     return 1;
	  }
	else if ( !cmp( "You are wearing:", colorless_line ) ||
		  !cmp( "You have *", colorless_line ) )
	  {
	     inventory = 0;
	  }
	else if ( !cmp( "You touch the tree of life tattoo.", colorless_line ) ||
		  !cmp( "Your senses return in a rush.", colorless_line ) )
	  {
	     failed_tree_commands = 0;
	     waiting_for_tree_command = 0;
	     add_timer( "reset_tree_balance", tree_recovery, balance_reset, 1, 0, 0 );
	     
	     last_cure_balance = 1;
	  }
	else if ( !cmp( "Your tree of life tattoo glows faintly for a moment then fades, leaving you ", colorless_line ) )
	  {
	     failed_tree_commands = 0;
	     waiting_for_tree_command = 0;
	     add_timer( "reset_tree_balance", 5, balance_reset, 1, 0, 0 );
	  }
	else if ( !cmp( "You focus your mind intently on curing your mental maladies.", colorless_line ) )
	  {
	     failed_focus_commands = 0;
	     waiting_for_focus_command = 0;
	     del_timer( "reset_focus_balance" );
	     
	     last_cure_balance = 2;
	  }
	else if ( !cmp( "You concentrate, but your mind is too tired to focus.", colorless_line ) )
	  {
	     failed_focus_commands = 0;
	     waiting_for_focus_command = 0;
	     del_timer( "reset_focus_balance" );
	  }
	else if ( !cmp( "You concentrate on purging your body of foreign toxins.", colorless_line ) )
	  {
	     failed_purge_commands = 0;
	     waiting_for_purge_command = 0;
	     del_timer( "reset_purge_balance" );
	  }
	else if ( !cmp( "You have not regained the ability to purge your body of toxins.", colorless_line ) )
	  {
	     failed_purge_commands = 0;
	     waiting_for_purge_command = 0;
	     del_timer( "reset_purge_balance" );
	  }
	else if ( !cmp( "You find your body already clear of harmful substance.", colorless_line ) )
	  {
	     failed_purge_commands = 0;
	     waiting_for_purge_command = 0;
	     del_timer( "reset_purge_balance" );
	     balance_focus = 0;
	     
	     last_cure_balance = 3;
	  }
	else if ( !cmp( "You begin to chant an ancient mantra, preparing your body to become a channel ", colorless_line ) ||
		  !cmp( "You are no longer boosting your Kai gain.", colorless_line ) )
	  {
	     kai_boost = 0;
	  }
	else if ( !cmp( "You must have balance to perform that Kaido ability.", colorless_line ) ||
		  !cmp( "Your mind is not balanced enough to perform that Kaido ability.", colorless_line ) )
	  {
	     balance_kai_heal = 0;
	  }
	/* Clot until we have reached a certain mana level. */
	else if ( !cmp( "Your wounds cause you to bleed * health.", line ) )
	  {
	     /* strlen( "Your wounds cause you to bleed " ) = 31 */
	     bleeding = atoi( line + 31 );
	  }
	else if ( !cmp( "You do not bleed my friend.", line ) )
	  {
	     bleeding = 0;
	  }
	if ( !cmp( "Your only working arm is off-balance.", line ) )
	  {
	     balance_left_arm = 1;
	     balance_right_arm = 0;
	  }
	if ( !cmp( "You are asleep and can do nothing. WAKE will attempt to wake you.", line ) )
	  {
	     /* Reset anything we can. */
	     
	     if ( waiting_for_herbs_command )
	       {
		  waiting_for_herbs_command = 0;
		  balance_herbs = 0;
		  del_timer( "herb_balance_reset" );
	       }
	     if ( waiting_for_salve_command )
	       {
		  waiting_for_salve_command = 0;
		  balance_salve = 0;
		  del_timer( "salve_balance_reset" );
	       }
	     if ( waiting_for_smoke_command )
	       {
		  waiting_for_smoke_command = 0;
		  balance_smoke = 0;
		  del_timer( "smoke_balance_reset" );
	       }
	     
	     for ( i = 0; defences[i].name; i++ )
	       if ( defences[i].tried )
		 defences[i].tried = 0;
	  }
	else if ( !cmp( "You cannot find a lit pipe with that in it.", line ) && last_pipe[0] )
	  {
	     char buf[256];
	     
	     if ( failed_smoke_commands > 2 )
	       return 1;
	     
	     if ( cmp( last_pipe, "laurel" ) &&
		  cmp( last_pipe, "lovage" ) &&
		  cmp( last_pipe, "linseed" ) )
	       return 1;
	     
	     clientff( C_R " (" C_W "outr/put %s" C_R ")" C_0, last_pipe );
	     sprintf( buf, "outr %s\r\n", last_pipe );
	     send_to_server_d( buf );
	     sprintf( buf, "put %s in pipe\r\n", last_pipe );
	     send_to_server( buf );
	     last_pipe[0] = 0;
	     pipes_lit = 0;
	     balance_smoke = 0;
	     waiting_for_smoke_command = 0;
	     failed_smoke_commands++;
	     del_timer( "smoke_balance_reset" );
	  }
	else if ( !cmp( "You are hit by a *", line ) )
	  {
	     char buf1[256], buf2[256], buf3[256];
	     
	     if ( !cmp( "You are hit by a ^ ray.", line ) )
	       {
		  sscanf( line, "You are hit by a %s ray.", buf1 );
		  check_demonray( buf1 );
	       }
	     else if ( !cmp( "You are hit by a ^ and ^ ray.", line ) )
	       {
		  sscanf( line, "You are hit by a %s and %s ray.", buf1, buf2 );
		  check_demonray( buf1 );
		  check_demonray( buf2 );
	       }
	     else if ( !cmp( "You are hit by a ^, ^, and ^ ray.", line ) ||
		       !cmp( "You are hit by a ^, ^, and ^ ray.", line ) )
	       {
		  sscanf( line, "You are hit by a %s %s and %s ray.", buf1, buf2, buf3 );
		  check_demonray( buf1 );
		  check_demonray( buf2 );
		  check_demonray( buf3 );
	       }
	  }
	
	parse_stances( line );
	
	parse_writhe( line );
	
	check_arm_balance( line );
     }
   
   
   
   /* These don't begin with 'You'. */
   
   if ( !strcmp( line, "** Illusion **" ) )
     ignore_illusion = 1;
   
   /* A curse with no message is probably Fear or Amnesia. Compose. */
   if ( curse_fear )
     curse_fear = 0;
   if ( !cmp( "^ points an imperious finger at you.", colorless_line ) )
     curse_fear = 1;
   
   if ( !cmp( "The clamor of the Song of Awakening rouses you from your sleep.", line ) )
     sleeping = 0;
   
   if ( !cmp( "What is it you wish to eat?", line ) )
     {
	if ( outr_once )
	  {
	     outr_once = 0;
	     clientff( C_R " (" C_W "outr_once = 0" C_R ")" C_0 );
	  }
     }
   
   if ( !cmp( "The elixir heals your body.", line ) ||
	!cmp( "Your mind feels rejuvenated.", line ) )
     {
	waiting_for_healing_command = 0;
	failed_healing_commands = 0;
	del_timer( "healing_balance_reset" );
     }
   
   /* Don't check for other triggers, in case this parser found somethin. */
   if ( parse_diagnose( colorful_line ) )
     return 1;
   
   /* Check who is in the same room. */
   if ( parse_full_sense( colorful_line ) )
     return 1;
   
   /* Like diagnose, ignore defence messages when a 'def' command wasn't sent. */
   if ( !cmp("Current Defences:", colorless_line ) && !allow_diagdef )
     {
	ignore_illusion = 1;
	clientff( C_W " (" C_G "i" C_W ")" C_0 );
	return 1;
     }
   
   /* We're currently getting the inventory list. */
   if ( inventory )
     {
	int len = strlen( inventory_buf );
	/* Force a space, if there is none. */
	if ( len > 0 && inventory_buf[len-1] != ' ' )
	  {
	     inventory_buf[len] = ' ';
	     inventory_buf[len+1] = 0;
	  }
	
	strcat( inventory_buf, colorless_line );
	return 1;
     }
   
   if ( strstr( colorful_line, "slashes into your skin, exposing your lifeblood to" ) )
     {
	int illusion = 0;
	
	/* Check for weak illusions. Some have some spaces after them, so it traps them. */
	if ( cmp( "* slashes into your skin, exposing your lifeblood to ^ toxic poisons.", colorful_line ) )
	  illusion = 1;
	else
	  {
	     /* So it looks good. But it's still an illusion, if we have
	      * fenugreek or immunity on. */
	     for ( i = 0; defences[i].name; i++ )
	       {
		  if ( ( !strcmp( defences[i].name, "fenugreek" ) ||
			 !strcmp( defences[i].name, "immunity" ) ) &&
		       defences[i].on )
		    illusion = 1;
	       }
	  }
	if ( illusion )
	  {
	     ignore_illusion = 1;
	     clientff( C_W " (" C_G "i" C_W ")" C_0 );
	     return 1;
	  }
     }
   
   if ( !cmp( "--> ^: conjure illusion *", line ) )
     {
	ignore_illusion = 1;
	clientff( C_W " (" C_G "i" C_W ")" C_0 );
	return 1;
     }
   
   if ( !cmp( "Your telepathic efforts are successful, and the mind of *", line ) )
     mind_locked = 1;
   
   if ( !cmp( "You focus your mind, and begin to concentrate on forming a mind lock *", line ) ||
	!cmp( "You continue to delve deeper and deeper into the mind of *", line ) ||
	strstr( line, " has moved out of range of your telepathy," ) )
     {
	mind_locked = 2;
	add_timer( "reset_mindlocking", 5, reset_mindlocking, 0, 0, 0 );
     }
   
   if ( !cmp( "Your relax your mind, and break the lock with *", line ) ||
	!cmp( "You dissolve the mindlock, severing all telepathic connections between you and*", line ) ||
	strstr( line, "is out of range of your telepathy," ) ||
	!cmp( "Your mental lock on *", line ) ||
	strstr( line, "across the land, sacrificing your" ) ||
	!cmp( "You effortlessly release your telepathic bond with *", line ) ||
	!cmp( "Your mind lock with *", line ) ||
	!cmp( "Your mind is violently thrown out of *", line ) ||
	!cmp( "You are not locked with any mind.", line ) ||
	!cmp( "You are not mindlocked with anyone at the moment.", line ) ||
	!cmp( "Your concentration is interrupted and the mindlock fails.", line ) )
     mind_locked = 0;
   
   if ( !cmp( "^ begins to bear down on you with *", line ) && trigger_level >= 2 )
     {
	char name[256], buf[256];
	
	clientff( C_W " (" C_G "paralyse" C_W ")" C_0 );
	get_string( line, name, 256 );
	sprintf( buf, "mind paralyse %s\r\n", name );
	send_to_server_d( buf );
     }
   
   if ( !cmp( "^ quickly flings a tarot card at you, and you feel unreasonable*", line ) )
     {
	char name[256];
	
	/* Security checks, first. */
	if ( line[0] == '\"' )
	  return 1;
	
	get_string( line, name, 256 );
	
	if ( !cmp( "Grace", line ) ||
	     !cmp( "grace", line ) ||
	     !cmp( "Mentor", line ) ||
	     !cmp( "mentor", line ) ||
	     !cmp( "patron", line ) ||
	     !cmp( "Patron", line ) )
	  return 1;
	
	/* Make sure we reject this person, at our first opportunity. */
	strcpy( lust, name );
     }
   
   /* Parse totems. */
   if ( !cmp( "A rune *", line ) )
     parse_runes( line );
   
   /* We might have some problems with this one. */
   if ( strstr( colorful_line, "  " ) )
     {
	ignore_next = 1;
	return 1;
     }
   
   return 0;
}



void aeon_timer( TIMER *self )
{
   /* A five second timer, which can help us tell if it's really Aeon. */
   if ( self->data[0] == 1 )
     {
	if ( aeon_check == 1 )
	  aeon_check = 0;
     }
   
   if ( self->data[0] == 2 )
     {
	if ( slow_check == 1 )
	  slow_check = 0;
     }
}



void send_action( char *string )
{
   char buf[256], *b, *p;
   
   /* Replace all ';' with '\n'. */
   
   b = buf;
   p = string;
   
   while( *p )
     {
	if ( *p == ';' )
	  *(b++) = '\n', p++;
	else
	  *(b++) = *(p++);
     }
   
   *(b++) = '\n';
   *b = 0;
   
   send_to_server_d( buf );
}



void parse_triggers( LINE *l )
{
   char *line;
   int i;
   
   DEBUG( "parse_triggers" );
   
   for ( i = 0; triggers[i].message; i++ )
     {
	line = triggers[i].nocolors ? l->line : l->raw_line;
	
	if ( !trigger_cmp( triggers[i].message, line, triggers[i].missing_chars ) )
	  {
	     /* Illusion defence. */
	     if ( ( triggers[i].first_line == 1 && line_since_prompt != 1 ) ||
		  ( triggers[i].first_line == 2 && line_since_prompt < 2 ) )
	       {
		  ignore_illusion = 1;
		  clientff( C_W " (" C_G "i" C_W ")" C_0 );
		  return;
	       }
	     /* Hypochondria defence. */
	     if ( afflictions[AFF_HYPOCHONDRIA].afflicted &&
		  triggers[i].special == SPEC_NORMAL &&
		  triggers[i].gives && line_since_prompt == 1 )
	       {
		  ignore_illusion = 1;
		  clientff( C_W " (" C_G "i" C_W ")" C_0 );
		  return;
	       }
	     
	     /* These NOTHING triggers just have their action sent, if one is specified. */
	     if ( triggers[i].special == SPEC_NOTHING );
	     /* Affliction trigger, so treat it nicely. */
	     else if ( triggers[i].special == SPEC_NORMAL )
	       {
		  /* Does it do anything to us? */
		  if ( triggers[i].affliction )
		    {
		       if ( triggers[i].affliction == AFF_ALL )
			 {
			    int j;
			    
			    /* It's about ALL of them. */
			    /* If it's AFF_NONE or AFF_ALL, which don't have a cure, don't change it. (>1) */
			    for ( j = 2; afflictions[j].name; j++ )
			      {
				   afflictions[j].afflicted = triggers[i].gives,
				   afflictions[j].tried = 0;
			      }
			    
			    /* Usually it will display that there are no more afflictions, or... */
			    /* Spam alert! */
			    if ( !triggers[i].gives )
			      changed = 1;
			 }
		       else
			 {
			    /* I know. Some very twisted and perplexing lines. *grin* */
			    if ( afflictions[triggers[i].affliction].afflicted != triggers[i].gives )
			      changed = 1;
			    
			    /* Drop the tried flag only if it was set. */
			    if ( triggers[i].gives )
			      afflictions[triggers[i].affliction].tried = 0;
			    
			    afflictions[triggers[i].affliction].afflicted = triggers[i].gives;
			    
			    affliction_check( i );
			 }
		       
		       if ( !triggers[i].silent )
			 {
			    if ( triggers[i].gives )
			      {
				 if ( triggers[i].affliction == AFF_SOMETHING )
				   clientf( C_W " (" C_Y );
				 else
				   clientf( C_W " (" C_R );
			      }
			    else
			      clientf( C_W " (" C_B );
			    clientf( afflictions[triggers[i].affliction].name );
			    clientf( C_W ")" C_0 );
			 }
		    }
	       }
	     /* Body part damage trigger. */
	     else if ( triggers[i].special == SPEC_PARTDAMAGE )
	       {
		  int part = PART_NONE;
		  char *p;
		  
		  if ( triggers[i].p_part == PART_NONE )
		    {
		       debugf( "No part or no damage level, in bodypart damage trigger %d.", i );
		       continue;
		    }
		  
		  if ( triggers[i].p_part == PART_ANY )
		    {
		       if ( wildcard_location < 0 || wildcard_location > strlen( line ) )
			 {
			    debugf( "Bad wildcard position, %d in line '%s'.", wildcard_location, line );
			 }
		       
		       p = line + wildcard_location;
		       
		       if ( !strncmp( p, "head", 4 ) )
			 part = PART_HEAD;
		       else if ( !strncmp( p, "body", 4 ) || !strncmp( p, "torso", 5 ) )
			 part = PART_TORSO;
		       else if ( !strncmp( p, "left arm", 7 ) )
			 part = PART_LEFTARM;
		       else if ( !strncmp( p, "right arm", 8 ) )
			 part = PART_RIGHTARM;
		       else if ( !strncmp( p, "left leg", 7 ) )
			 part = PART_LEFTLEG;
		       else if ( !strncmp( p, "right leg", 8 ) )
			 part = PART_RIGHTLEG;
		       else if ( !strncmp( p, "arm", 3 ) )
			 part = PART_ARM;
		       else if ( !strncmp( p, "leg", 3 ) )
			 part = PART_LEG;
		    }
		  else
		    part = triggers[i].p_part;
		  
		  if ( part == PART_NONE )
		    {
		       debugf( "Unknown body part, at trigger %d.", i );
		       continue;
		    }
		  
		  /* If it's PART_ONEARM or PART_ONELEG, then there must be at least one. */
		  if ( part == PART_ONEARM )
		    {
		       if ( partdamage[PART_LEFTARM] == PART_HEALED &&
			    partdamage[PART_RIGHTARM] == PART_HEALED )
			 part = PART_ARM;
		    }
		  else if ( part == PART_ONELEG )
		    {
		       if ( partdamage[PART_LEFTLEG] == PART_HEALED &&
			    partdamage[PART_RIGHTLEG] == PART_HEALED )
			 part = PART_LEG;
		    }
		  
		  /* If it's PART_ARM or PART_LEG, then take any leg that's healed. */
		  if ( part == PART_ARM )
		    {
		       if ( partdamage[PART_LEFTARM] == PART_HEALED )
			 part = PART_LEFTARM;
		       else if ( partdamage[PART_RIGHTARM] == PART_HEALED )
			 part = PART_RIGHTARM;
		    }
		  else if ( part == PART_LEG )
		    {
		       if ( partdamage[PART_LEFTLEG] == PART_HEALED )
			 part = PART_LEFTLEG;
		       else if ( partdamage[PART_RIGHTLEG] == PART_HEALED )
			 part = PART_RIGHTLEG;
		    }
		  
		  if ( part == PART_LEG || part == PART_ARM )
		    continue;
		  
		  partdamage[part] = triggers[i].p_level;
		  
		  /* Show damage status before a new prompt. */
		  if ( triggers[i].p_level != PART_HEALED )
		    show_limbs = 1;
	       }
	     /* Trip trigger. */
	     else if ( triggers[i].special == SPEC_TRIP )
	       {
		  tripped = 1;
	       }
	     else if ( triggers[i].special == SPEC_IGNORENEXT )
	       {
		  ignore_next = 1;
		  ignore_illusion = 1;
	       }
	     else if ( triggers[i].special == SPEC_INSTAKILL )
	       {
		  instakill_warning = 1;
	       }
	     else if ( triggers[i].special == SPEC_DEFENCE )
	       {
		  char buf[256];
		  int def = -1, j;
		  
		  if ( !triggers[i].defence )
		    {
		       debugf( "Error at trigger %d: a defence trigger, with no defence set.", i );
		       continue;
		    }
		  
		  if ( !strcmp( triggers[i].defence, "all" ) )
		    {
		       for ( j = 0; defences[j].name; j++ )
			 {
			    defences[j].on = triggers[i].gives;
			    defences[j].tried = 0;
			 }
		       
		       if ( !triggers[i].silent )
			 {
			    if ( triggers[i].gives )
			      clientf( C_B " (" C_W "all" C_B ")" C_0 );
			    else
			      clientf( C_B " (" C_R "all" C_B ")" C_0 );
			 }
		       
		       continue;
		    }
		  else if ( !strcmp( triggers[i].defence, "any" ) )
		    {
		       if ( wildcard_location < 0 || wildcard_location > strlen( line ) )
			 {
			    debugf( "The location of this defence's name doesn't look good." );
			    continue;
			 }
		       get_string( line + wildcard_location, buf, 256 );
		    }
		  else
		    strcpy( buf, triggers[i].defence );
		  
		  for ( j = 0; defences[j].name; j++ )
		    {
		       if ( strncmp( defences[j].name, buf, strlen( buf ) ) )
			 continue;
		       
		       def = j;
		       break;
		    }
		  
		  if ( def < 0 )
		    {
		       debugf( "No such defence, %s, found.", buf );
		       if ( !triggers[i].silent )
			 clientf( C_B " (" C_W "?" C_B ")" C_0 );
		       continue;
		    }
		  
		  defences[j].on = triggers[i].gives;
		  defences[j].tried = 0;
		  sprintf( buf, "reset_defence_%s", defences[j].name );
		  del_timer( buf );
		  
		  if ( !triggers[i].silent )
		    {
		       if ( defences[j].on )
			 clientf( C_B " (" C_W );
		       else
			 clientf( C_B " (" C_R );
		       clientf( defences[j].name );
		       clientf( C_B ")" C_0 );
		    }
	       }
	     else if ( triggers[i].special == SPEC_WRITHE )
	       {
		  if ( triggers[i].gives == 2 )
		    {
		       del_timer( "reset_writhe" );
		       if ( !writhe[triggers[i].writhe] )
			 clientff( "\r\n" C_R "[" C_G "Writhing from an unregistered type!" C_R "]" C_0 );
		    }
		  else
		    {
		       writhe[triggers[i].writhe] = triggers[i].gives;
		       if ( !triggers[i].gives )
			 {
			    writhing = 0;
			    
			    /* Any kind of resets go here. */
			    arrows_timer = 0;
			 }
		       if ( triggers[i].gives && sitting == 2 )
			 sitting = 1;
		    }
	       }
	     else if ( triggers[i].special == SPEC_ARROWS )
	       {
		  if ( triggers[i].gives == 1 )
		    arrows++;
		  else if ( triggers[i].gives == 0 )
		    {
		       arrows--;
		       arrows_timer = 0;
		       del_timer( "reset_arrows" );
		    }
		  else if ( triggers[i].gives == 2 )
		    {
		       arrows = 0;
		       arrows_timer = 0;
		       del_timer( "reset_arrows" );
		    }
	       }
	     else if ( triggers[i].special == SPEC_AEON )
	       {
		  if ( triggers[i].gives )
		    {
		       if ( !aeon )
			 {
			    aeon_check = 1;
			    add_timer( "aeon_check", 5, aeon_timer, 1, 0, 0 );
			 }
		    }
		  else
		    {
		       aeon = 0;
		    }
	       }
	     else if ( triggers[i].special == SPEC_SLOW )
	       {
		  if ( !aeon && trigger_level )
		    {
		       if ( aeon_check )
			 aeon = 1;
		       else if ( slow_check )
			 {
			    trigger_level = 0;
			    
			    clientff( "\r\n" C_R "[" C_W "Trigger level set to 0!" C_R "]" C_0 );
			 }
		       else
			 {
			    slow_check = 1;
			    add_timer( "slow_check", 5, aeon_timer, 2, 0, 0 );
			 }
		    }
	       }
	     else if ( triggers[i].special == SPEC_YOTH )
	       {
		  yoth = triggers[i].gives;
		  if ( !triggers[i].silent )
		    clientff( C_W " (%syoth" C_W ")" C_0, yoth ? C_R : C_B );
	       }
	     
	     /* Need we send something? Level dependent. */
	     if ( triggers[i].level <= trigger_level )
	       {
		  if ( triggers[i].action )
		    {
		       clientf( C_G " (" C_W );
		       clientf( triggers[i].action );
		       clientf( C_G ")" C_0 );
		       send_action( triggers[i].action );
		       prompt_newline = 1;
		    }
	       }
	  }
     }
}



void imperian_process_server_line( LINE *l )
{
   DEBUG( "imperian_process_server_line" );
   
   /* We've been sent an empty line? */
   if ( !l->len )
     return;
   
   if ( ignore_illusion )
     {
	clientff( C_W " (" C_G "i" C_W ")" C_0 );
	return;
     }
   
   /* Ignore it if we're told so. */
   if ( ignore_next )
     return;
   
   /* Things that just don't fit in triggers. */
   if ( parse_special( l ) )
     return;
   
   /* The main big thing. This is what the module will do most of its time. */
   parse_triggers( l );
}



void add_affliction( char *name )
{
   char buf[256];
   int i, len;
   
   DEBUG( "add_affliction" );
   
   len = strlen( name );
   
   for ( i = 0; afflictions[i].name; i++ )
     {
	if ( i == AFF_NONE || i == AFF_ALL )
	  continue;
	
	if ( !strncmp( afflictions[i].name, name, len ) )
	  {
	     afflictions[i].afflicted = 1;
	     afflictions[i].tried = 0;
	     sprintf( buf, C_R "[Added affliction: %s.]\r\n" C_0,
		      afflictions[i].name );
	     clientf( buf );
	     return;
	  }
     }
   clientf( C_R "[No such affliction known.]\r\n" C_0 );
}



void cure_limbs( void )
{
   char buf[256];
   char *string = "impossible error";
   int i, nothing = 1;
   
   DEBUG( "cure_limbs" );
   
   /* Do we actually have anything broken? */
   for ( i = 0; i <= PART_LAST; i++ )
     if ( partdamage[i] )
       nothing = 0;
   
   if ( nothing )
     return;
   
   /* And can we do something about it? */
   if ( balance_salve )
     return;
   
   if ( afflictions[AFF_SLICKNESS].afflicted )
     return;
   
   /* Well, then let's go heal em. Head and legs are a priority. */
   
   if ( partdamage[PART_HEAD] > 0 )
     string = "apply restoration to head";
   
   else if ( partdamage[PART_LEFTLEG] >= 2 || partdamage[PART_RIGHTLEG] >= 2 )
     string = "apply restoration to legs";
   
   else if ( partdamage[PART_LEFTLEG] == 1 || partdamage[PART_RIGHTLEG] == 1 )
     string = "apply mending to legs";
   
   else if ( partdamage[PART_TORSO] > 0 )
     string = "apply restoration to torso";
   
   else if ( partdamage[PART_LEFTARM] >= 2 || partdamage[PART_RIGHTARM] >= 2 )
     string = "apply restoration to arms";
   
   else if ( partdamage[PART_LEFTARM] == 1 || partdamage[PART_RIGHTARM] == 1 )
     string = "apply mending to arms";
   
   sprintf( buf, C_W "(" C_G "%s" C_W ") " C_0, string );
   clientf( buf );
   sprintf( buf, "%s\r\n", string );
   send_to_server_d( buf );
   
   balance_salve = 1;
   waiting_for_salve_command = 1;
   add_timer( "salve_balance_reset", reset_time, balance_reset, 3, 0, 0 );
   prompt_newline = 1;
}



void reset_special_aff( TIMER *self )
{
   if ( afflictions[self->data[0]].tried &&
	afflictions[self->data[0]].afflicted )
     {
	clientff( "\r\n" C_R "[" C_D "Resetting affliction '" C_R "%s'" C_D "." C_R "]" C_0 "\r\n",
		  afflictions[self->data[0]].name );
	show_prompt( );
	afflictions[self->data[0]].tried = 0;
	afflictions[self->data[0]].afflicted = 1;
     }
}


void execute_affliction_action( int aff, int balance )
{
   char buf[256];
   char *cure;
   
   if ( balance == BAL_HERB )
     {
	cure = cures[afflictions[aff].herbs_cure].name;
	if ( !outr_once || ( outr_once && !cures[afflictions[aff].herbs_cure].outr ) )
	  {
	     clientff( C_G "(" C_W "outr/eat %s" C_G ") " C_0, cure );
	     
	     /* Send it thrice, in case of stupidity. */
	     sprintf( buf, "outr %s\r\n", cure );
	     send_to_server_d( buf );
	     if ( aff == AFF_STUPIDITY )
	       {
		  sprintf( buf, "outr %s\r\n", cure );
		  send_to_server( buf );
	       }
	  }
	else
	  {
	     clientff( C_G "(" C_W "eat %s" C_G ") " C_0, cure );
	  }
	
	prompt_newline = 1;
	sprintf( buf, "eat %s\r\n", cure );
	send_to_server_d( buf );
	if ( aff == AFF_STUPIDITY )
	  {
	     sprintf( buf, "eat %s\r\n", cure );
	     send_to_server( buf );
	  }
	
	balance_herbs = 1;
	waiting_for_herbs_command = aff;
	add_timer( "herb_balance_reset", reset_time, balance_reset, 2, 0, 0 );
     }
   
   else if ( balance == BAL_SMOKE )
     {
	cure = cures[afflictions[aff].smoke_cure].name;
	clientff( C_G "(" C_W "%ssmoke %s" C_G ") " C_0,
		  arti_pipes ? "" : "light/", cure );
	prompt_newline = 1;
	
	if ( !arti_pipes )
	  send_to_server_d( "light pipes\r\n" );
	
	sprintf( buf, "smoke pipe with %s\r\n", cure );
	send_to_server_d( buf );
	balance_smoke = 1;
	waiting_for_smoke_command = aff;
	strcpy( last_pipe, cure );
	add_timer( "smoke_balance_reset", reset_time, balance_reset, 4, 0, 0 );
     }
   
   else if ( balance == BAL_SALVE )
     {
	cure = cures[afflictions[aff].salve_cure].name;
	clientff( C_G "(" C_W "apply %s" C_G ") " C_0, cure );
	prompt_newline = 1;
	sprintf( buf, "apply %s\r\n", cure );
	send_to_server_d( buf );
	balance_salve = 1;
	waiting_for_salve_command = aff;
	add_timer( "salve_balance_reset", reset_time, balance_reset, 3, 0, 0 );
     }
   
   
   
}

int check_madness( int aff )
{
   if ( afflictions[AFF_MADNESS].afflicted )
     {
	if ( aff == AFF_DEMENTIA ||
	     aff == AFF_STUPIDITY ||
	     aff == AFF_CONFUSION ||
	     aff == AFF_HYPERSOMNIA ||
	     aff == AFF_PARANOIA ||
	     aff == AFF_HALLUCINATIONS ||
	     aff == AFF_IMPATIENCE ||
	     aff == AFF_ADDICTION ||
	     aff == AFF_RECKLESSNESS ||
	     aff == AFF_MASOCHISM )
	  return 1;
     }
   
   return 0;
}



int can_cure_aff( int aff, int balance )
{
   if ( balance == BAL_HERB )
     {
	if ( !afflictions[aff].herbs_cure )
	  return 1;
	
	if ( waiting_for_herbs_command )
	  return 2;
	
	/* Anorexia - No herbs. */
	if ( afflictions[AFF_ANOREXIA].afflicted )
	  return 1;
	
	/* Bardic Yoth - Can't eat with a shriveled throat. */
	if ( yoth )
	  return 1;
	
	/* Can't outrift while writhing. */
	if ( writhing )
	  {
	     if ( !outr_once || ( outr_once && !cures[afflictions[aff].herbs_cure].outr ) )
	       return 1;
	  }
	
	if ( balance_herbs )
	  return -1;
     }
   
   else if ( balance == BAL_SMOKE )
     {
	if ( !afflictions[aff].smoke_cure )
	  return 1;
	
	if ( waiting_for_smoke_command )
	  return 3;
	
	/* Asthma - No smoking. */
	if ( afflictions[AFF_ASTHMA].afflicted )
	  return 1;
	
	/* Bardic Yoth - Can't smoke with a shriveled throat. */
	if ( yoth )
	  return 1;
	
	/* Smoking, with pipes unlit and off balance. */
	if ( !arti_pipes && !pipes_lit && !have_balance( ) )
	  return 1;
	
	if ( balance_smoke )
	  return -1;
     }
	
   else if ( balance == BAL_SALVE )
     {
	if ( !afflictions[aff].salve_cure )
	  return 1;
	
	if ( waiting_for_salve_command )
	  return 4;
	
        /* Slickness - No salves. */
	if ( afflictions[AFF_SLICKNESS].afflicted )
	  return 1;
	
	if ( balance_salve )
	  return -1;
     }
   
   /* Those that don't have a balance. */
   else if ( balance == BAL_NONE )
     {
	if ( !afflictions[aff].special )
	  return 1;
	
	if ( afflictions[aff].tried )
	  return 1;
     }
   
   /* Confusion - No concentrate. */
   if ( afflictions[AFF_CONFUSION].afflicted &&
	balance == BAL_NONE &&
	afflictions[aff].special == CURE_CONCENTRATE )
     return 1;
   
   /* Anorexia or Yoth - No cyanide, which is a special cure. */
   if ( ( afflictions[AFF_ANOREXIA].afflicted || yoth ) &&
	balance == BAL_NONE &&
	afflictions[aff].special == CURE_IMMUNITY )
     return 1;
   
   /* Whispering Madness - Can't cure some mental afflictions. */
   if ( check_madness( aff ) )
     return 1;
   
   return 0;
}


void cure_afflictions( void )
{
   char buf[256];
   int aff;
   
   /* AFF_NONE == 0, AFF_ALL == 1. Skip those. */
   
   for ( aff = 2; afflictions[aff].name; aff++ )
     {
	if ( !afflictions[aff].afflicted || afflictions[aff].tried )
	  continue;
	
	if ( waiting_for_herbs_command == aff ||
	     waiting_for_salve_command == aff ||
	     waiting_for_smoke_command == aff ||
	     waiting_for_purge_command == aff ||
	     waiting_for_focus_command == aff ||
	     waiting_for_tree_command == aff )
	  continue;
     
	/* Salves. */
	else if ( !can_cure_aff( aff, BAL_SALVE ) )
	  {
	     execute_affliction_action( aff, BAL_SALVE );
	  }
	/* Pipes. */
	if ( !can_cure_aff( aff, BAL_SMOKE ) )
	  {
	     execute_affliction_action( aff, BAL_SMOKE );
	  }
	/* Herbs. */
	else if ( !can_cure_aff( aff, BAL_HERB ) )
	  {
	     execute_affliction_action( aff, BAL_HERB );
	  }
	else if ( afflictions[aff].special &&
		  !can_cure_aff( aff, BAL_NONE ) )
	  {
	     clientff( C_G "(" C_W "%s" C_G ") " C_0, cures[afflictions[aff].special].action );
	     prompt_newline = 1;
	     send_action( cures[afflictions[aff].special].action );
	     
	     /* No balance stops it, so it will spam us if we don't. */
	     afflictions[aff].tried = 1;
	     
	     /* Also, it might stop it for ever. */
	     sprintf( buf, "reset_special_%s", afflictions[aff].name );
	     add_timer( buf, 5, reset_special_aff, aff, 0, 0 );
	  }
     }
   
   /* Special cures. Tree, Purge Blood and Focus. */
   
   if ( ( cure_with_focus || cure_with_purge ) &&
	use_mana_above && use_mana_above < p_mana )
     for ( aff = 2; afflictions[aff].name; aff++ )
       {
	  if ( !afflictions[aff].afflicted || afflictions[aff].tried )
	    continue;
	  
	  if ( cure_with_focus && !balance_focus &&
	       afflictions[aff].focus &&
	       waiting_for_herbs_command != aff &&
	       waiting_for_salve_command != aff &&
	       waiting_for_smoke_command != aff &&
	       waiting_for_purge_command != aff &&
	       waiting_for_tree_command != aff )
	    {
	       clientf( C_G "(" C_W "focus" C_G ") " C_0 );
	       send_to_server_d( "focus\r\n" );
	       waiting_for_focus_command = aff;
	       prompt_newline = 1;
	       balance_focus = 1;
	       add_timer( "reset_focus_balance", reset_time, balance_reset, 7, 0, 0 );
	    }
	  
	  if ( afflictions[aff].purge_blood && !writhing &&
	       cure_with_purge && !balance_purge &&
	       waiting_for_herbs_command != aff &&
	       waiting_for_salve_command != aff &&
	       waiting_for_smoke_command != aff &&
	       waiting_for_focus_command != aff &&
	       waiting_for_tree_command != aff )
	    {
	       clientf( C_G "(" C_W "purge blood" C_G ") " C_0 );
	       send_to_server_d( "purge blood\r\n" );
	       waiting_for_purge_command = aff;
	       prompt_newline = 1;
	       balance_purge = 1;
	       add_timer( "reset_purge_balance", reset_time, balance_reset, 8, 0, 0 );
	    }
       }
   
   if ( cure_with_tree )
     for ( aff = 2; afflictions[aff].name; aff++ )
       {
	  if ( !afflictions[aff].afflicted || afflictions[aff].tried )
	    continue;
	  
	  if ( !balance_tree &&
	       waiting_for_herbs_command != aff &&
	       waiting_for_salve_command != aff &&
	       waiting_for_smoke_command != aff &&
	       waiting_for_focus_command != aff &&
	       waiting_for_purge_command != aff )
	    {
	       /* Check if paralysed, and if we have our hands free. */
	       if ( !afflictions[AFF_PARALYSIS].afflicted &&
		    ( partdamage[PART_LEFTARM] == PART_HEALED ||
		      partdamage[PART_RIGHTARM] == PART_HEALED ) &&
		    !writhing )
		 {
		    clientf( C_G "(" C_W "touch tree" C_G ") " C_0 );
		    send_to_server_d( "touch tree\r\n" );
		    waiting_for_tree_command = aff;
		    prompt_newline = 1;
		    balance_tree = 1;
		    /* Only 5 seconds. Will be replaced with 20 seconds, at
		     * the 'touch tree' message. */
		    add_timer( "reset_tree_balance", 5, balance_reset, 1, 1, 0 );
		 }
	    }
       }
}



void list_afflictions( void )
{
   char buf[256];
   int i;
   int nr = 0;
   int cure, can_cure;
   
   DEBUG( "list_afflictions" );
   
   for ( i = 0; afflictions[i].name; i++ )
     {
	if ( afflictions[i].afflicted )
	  {
	     if ( !nr )
	       clientf( C_R "[Afflictions:]\r\n" C_0 );
	     nr++;
	     
	     if ( afflictions[i].salve_cure )
	       {
		  cure = afflictions[i].salve_cure;
		  can_cure = can_cure_aff( i, BAL_SALVE );
		  if ( can_cure < 0 )
		    can_cure = 0;
	       }
	     else if ( afflictions[i].smoke_cure )
	       {
		  cure = afflictions[i].smoke_cure;
		  can_cure = can_cure_aff( i, BAL_SMOKE );
		  if ( can_cure < 0 )
		    can_cure = 0;
	       }
	     else if ( afflictions[i].herbs_cure )
	       {
		  cure = afflictions[i].herbs_cure;
		  can_cure = can_cure_aff( i, BAL_HERB );
		  if ( can_cure < 0 )
		    can_cure = 0;
	       }
	     else if ( afflictions[i].special )
	       {
		  cure = afflictions[i].special;
		  can_cure = can_cure_aff( i, BAL_NONE );
		  if ( can_cure < 0 )
		    can_cure = 0;
	       }
	     else
	       cure = 0, can_cure = 1;
	     
	     sprintf( buf, C_R "[" C_D "Afflicted by: [%s%s" C_D "]  Cure: ["
		      "%s%s" C_D "] (" C_W "`%d" C_D ")" C_R "]\r\n" C_0,
		      i == AFF_SOMETHING ? C_Y : C_R, afflictions[i].name,
		      afflictions[i].tried ? C_R : ( !can_cure ? C_B : C_R ),
		      cures[cure].name, nr );
	     clientf( buf );
	  }
     }
   if ( !nr )
     clientf( C_R "[No more known afflictions on you.]\r\n" C_0 );
}



void build_stats( int console_codes )
{
   static char defs[20][128];
   int i, d = 0, s = 0;
   int nr = 0;
   int can_cure;
   char *eol, *eof, *bof;
   
   DEBUG( "build_stats" );
   
   if ( console_codes )
     eol = "\33[K", eof = "\33[J", bof = "\33[0;0H";
   else
     eol = "", eof = "", bof = "";
   
   for ( i = 0; defences[i].name && d < 20; i++ )
     {
	if ( !defences[i].on && !defences[i].keepon )
	  continue;
	
	sprintf( defs[d++], C_R "[" C_B "%-*s %s*" C_R "]",
		 stats_biggestname, defences[i].name,
		 !defences[i].on ? C_R : ( defences[i].keepon ? C_W : C_G ) );
     }
   
   while ( d < 20 )
     defs[d++][0] = '\0';
   
   d = 0;
   
   /* Body part damage.. */
   sprintf( stats[s++], "%s" C_R "[Limb damage:]" C_0 "                         "
	    C_R "[Stats:]" C_0 "        " C_R "[Defences:]" C_0 "%s", bof, eol );
   
   sprintf( stats[s++], C_R "[" C_B "Left Arm: %s%d" C_B
	    "  Right Arm: %s%d" C_B "  Head:  %s%d" C_R "]" C_0
	    "  " C_R "[" C_B "Health: %s%4d" C_R "]" C_0 "  %s" C_0 "%s",
	    partdamage[PART_LEFTARM] ? C_R : C_G, partdamage[PART_LEFTARM],
	    partdamage[PART_RIGHTARM] ? C_R : C_G, partdamage[PART_RIGHTARM],
	    partdamage[PART_HEAD] ? C_R : C_G, partdamage[PART_HEAD],
	    p_hp < p_heal ? C_R : C_G, p_hp, defs[d++], eol );
   
   sprintf( stats[s++], C_R "[" C_B "Left Leg: %s%d" C_B
	    "  Right Leg: %s%d" C_B "  Torso: %s%d" C_R "]" C_0
	    "  " C_R "[" C_B "Mana:   %s%4d" C_R "]" C_0 "  %s" C_0 "%s",
	    partdamage[PART_LEFTLEG] ? C_R : C_G, partdamage[PART_LEFTLEG],
	    partdamage[PART_RIGHTLEG] ? C_R : C_G, partdamage[PART_RIGHTLEG],
	    partdamage[PART_TORSO] ? C_R : C_G, partdamage[PART_TORSO],
	    p_mana < p_heal_mana ? C_R : C_G, p_mana, defs[d++], eol );
   sprintf( stats[s++], "                                                       %s" C_0 "%s", defs[d++], eol );
   
   
   /* Balances.. */
   sprintf( stats[s++], C_R "[Balances:]                                            %s" C_0 "%s", defs[d++], eol );
   sprintf( stats[s++], C_R "[" C_B "Balance:     %s%d" C_B "  Healing: %s%d"
	    C_B "  Salves: %s%d" C_B "  Cure Elix: %s%d" C_R "]  %s" C_0 "%s",
	    p_balance ? C_G : C_R, p_balance,
	    balance_healing ? C_R : C_G, balance_healing,
	    balance_salve ? C_R : C_G, balance_salve,
	    balance_cureelix ? C_R : C_G, balance_cureelix, defs[d++], eol );
   sprintf( stats[s++], C_R "[" C_B "Equilibrium: %s%d" C_B "  Herbs:   %s%d"
	    C_B "  Smoke:  %s%d" C_B "  Toadstool: %s%d" C_R "]  %s" C_0 "%s",
	    p_equilibrium ? C_G : C_R, p_equilibrium,
	    balance_herbs ? C_R : C_G, balance_herbs,
	    balance_smoke ? C_R : C_G, balance_smoke,
	    balance_toadstool ? C_R : C_G, balance_toadstool, defs[d++], eol );
   
   sprintf( stats[s++], "                                                       %s" C_0 "%s", defs[d++], eol );
   
   /* Afflictions.. */
   for ( i = 0; afflictions[i].name && nr < 10; i++ )
     {
	if ( afflictions[i].afflicted )
	  {
	     if ( !nr )
	       sprintf( stats[s++], C_R "[Afflictions:]                                         %s" C_0 "%s", defs[d++], eol );
	     nr++;
	     
	     can_cure = !can_cure_aff( i, BAL_HERB );
	     
	     sprintf( stats[s++], C_R "[" C_G "Afflicted by: [" C_R "%s" C_G "]  Cure: ["
		      "%s%s" C_G "] (" C_W "`%d" C_G ")" C_R "]  %*s" C_0 "%s",
		      afflictions[i].name,
		      afflictions[i].tried ? C_R : ( can_cure ? C_B : C_G ),
		      cures[afflictions[i].herbs_cure].name, nr,
		      63 - (int) strlen( afflictions[i].name ) - (int) strlen( cures[afflictions[i].herbs_cure].name ) -
		      ( nr < 10 ? 1 : ( nr < 100 ? 2 : 3 ) ), defs[d++], eol );
	  }
     }
   if ( !nr )
     sprintf( stats[s++], C_R "[No known afflictions.]                                %s" C_0 "%s", defs[d++], eol );
   
   while ( d < 20 && defs[d][0] )
     sprintf( stats[s++], "                                                       %s" C_0 "%s", defs[d++], eol );
   
   sprintf( stats[s], "%s", eof );
}



void show_stats_on_term( void )
{
#if !defined( FOR_WINDOWS )
   int s;
   
   DEBUG( "show_stats_on_term" );
   
   if ( !show_on_term )
     return;
   
   build_stats( 1 );
   
   for ( s = 0; stats[s][0]; s++ )
     debugf( stats[s] );
#endif
}



void show_part_damage( void )
{
   char buf[256];
   
   DEBUG( "show_part_damage" );
   
   clientf( C_R "[Limb damage:]\r\n" C_0 );
   
   sprintf( buf, C_R "[" C_B "Left Arm: %s%d" C_B
	    "  Right Arm: %s%d" C_B "  Head:  %s%d" C_R "]\r\n" C_0,
	    partdamage[PART_LEFTARM] ? C_R : C_G, partdamage[PART_LEFTARM],
	    partdamage[PART_RIGHTARM] ? C_R : C_G, partdamage[PART_RIGHTARM],
	    partdamage[PART_HEAD] ? C_R : C_G, partdamage[PART_HEAD] );
   clientf( buf );
   
   sprintf( buf, C_R "[" C_B "Left Leg: %s%d" C_B
	    "  Right Leg: %s%d" C_B "  Torso: %s%d" C_R "]\r\n" C_0,
	    partdamage[PART_LEFTLEG] ? C_R : C_G, partdamage[PART_LEFTLEG],
	    partdamage[PART_RIGHTLEG] ? C_R : C_G, partdamage[PART_RIGHTLEG],
	    partdamage[PART_TORSO] ? C_R : C_G, partdamage[PART_TORSO] );
   clientf( buf );
}



void show_our_prompt( )
{
   char buf[256];
   int afflictionsnr = 0, damagedparts = 0;
   int i;
   
   DEBUG( "show_our_prompt" );
   
   for ( i = 0; afflictions[i].name; i++ )
     if ( afflictions[i].afflicted )
       afflictionsnr++;
   
   for ( i = 0; i <= PART_LAST; i++ )
     if ( partdamage[i] )
       damagedparts++;
   
/*   sprintf( buf, C_0 "L:" C_G "%d" C_0 " B: h:%s t:%s t:%s A:%s%d" C_0
	    " P:%s%d" C_0 "\r\n",
	    trigger_level,
	    balance_healing   ? C_R "n" C_0 : C_B "y" C_0,
	    balance_toadstool ? C_R "n" C_0 : C_B "y" C_0,
	    tripped           ? C_R "n" C_0 : C_B "y" C_0,
	    afflictionsnr ? C_R : C_B, afflictionsnr,
	    damagedparts  ? C_R : C_B, damagedparts );*/
   
   sprintf( buf, C_0 "L:" C_G "%d" C_0 " A:%s%d" C_0 " P:%s%d " C_0,
	    trigger_level,
	    afflictionsnr ? C_R : C_B, afflictionsnr,
	    damagedparts  ? C_R : C_B, damagedparts );
   clientf( buf );
}



void list_triggers( char *str )
{
   char buf[256];
   int i, affliction = -1, len;
   
   DEBUG( "list_triggers" );
   
   if ( str )
     {
	len = strlen( str );
	for ( i = 0; afflictions[i].name; i++ )
	  if ( !strncmp( str, afflictions[i].name, len ) )
	    {
	       affliction = i;
	       break;
	    }
     }
   
   if ( str && affliction == -1 )
     {
	sprintf( buf, C_R "[No such affliction, %s, known.]\r\n" C_0, str );
	clientf( buf );
	return;
     }
   
   if ( affliction == -1 )
     sprintf( buf, C_R "[Triggers:]\r\n" C_0 );
   else
     sprintf( buf, C_R "[Triggers for %s:]\r\n" C_0, afflictions[affliction].name );
   clientf( buf );
   
   for ( i = 0; triggers[i].message; i++ )
     {
	if ( affliction != -1 && triggers[i].affliction != affliction )
	  continue;
	
	/* This would be one huge sprintf that I hate. */
	sprintf( buf, C_R "[" C_G "Message: \"%s\" (" C_R "%s" C_G ")%s%s%s%s%s" C_R "]\r\n" C_0,
		 triggers[i].message,
		 afflictions[triggers[i].affliction].name,
		 triggers[i].affliction != AFF_NONE ? ( triggers[i].gives ? " (" C_R "give" C_G ")" :
							" (" C_B "cure" C_G ")" ) : "",
		 triggers[i].missing_chars ? " (" C_B "supports different chars" C_G ")" : "",
		 triggers[i].action ? C_W " (" C_B : "",
		 triggers[i].action ? triggers[i].action : "",
		 triggers[i].action ? C_W ")" C_G : "" );
	clientf( buf );
     }
}



void parse_inventory( )
{
   int baby_rat = 0, young_rat = 0, rat = 0, old_rat = 0, black_rat = 0;
   int spider = 0, caiman = 0, serpents = 0, locusts = 0;
   char *p = inventory_buf, *b;
   char buf[256];
   
   DEBUG( "parse_inventory" );
   
   /* Until I find another way, I'll leave it so. */
   if ( strstr( inventory_buf, "   " ) )
     return;
   
   while ( 1 )
     {
	b = buf;
	while ( *p && *p != ',' && *p != '.' )
	  *(b++) = *(p++);
	*b = 0;
	
	/* Now that we have the item name in 'buf', let's check it. */
	/* Rats. */
	if ( !strcmp( buf, "the corpse of a baby rat" ) )
	  baby_rat = 1;
	else if ( !strcmp( buf, "the corpse of a young rat" ) )
	  young_rat = 1;
	else if ( !strcmp( buf, "the corpse of a rat" ) )
	  rat = 1;
	else if ( !strcmp( buf, "the corpse of an old rat" ) )
	  old_rat = 1;
	else if ( !strcmp( buf, "the corpse of a black rat" ) )
	  black_rat = 1;
	else if ( strstr( buf, "corpses of a baby rat" ) )
	  baby_rat = atoi( buf );
	else if ( strstr( buf, "corpses of a young rat" ) )
	  young_rat = atoi( buf );
	else if ( strstr( buf, "corpses of a rat" ) )
	  rat = atoi( buf );
	else if ( strstr( buf, "corpses of an old rat" ) )
	  old_rat = atoi( buf );
	else if ( strstr( buf, "corpses of a black rat" ) )
	  black_rat = atoi( buf );
	/* Spiders. */
	else if ( !strcmp( buf, "the corpse of a large black spider" ) )
	  spider = 1;
	else if ( strstr( buf, "corpses of a large black spider" ) )
	  spider = atoi( buf );
	/* Sewer.. things. */
	else if ( strstr( buf, "corpses of a vile serpent" ) )
	  serpents += atoi( buf );
	else if ( strstr( buf, "corpses of a hungry caiman" ) )
	  caiman += atoi( buf );
	else if ( strstr( buf, "corpses of a short-horned desert locust" ) )
	  locusts += atoi( buf );
	
	/* The end? */
	if ( *p == '.' || *p == 0 )
	  break;
	
	/* Skip ", " */
	p += 2;
     }
   
   rat_value = 7 * baby_rat + 14 * young_rat + 21 * rat +
     28 * old_rat + 35 * black_rat;
   spider_value = spider * 15;
   serpent_value = serpents * 15;
   caiman_value = caiman * 30;
   locust_value = locusts * 10;
}



int is_cured_by( int aff, int cure )
{
   if ( afflictions[aff].herbs_cure == cure ||
	afflictions[aff].smoke_cure == cure ||
	afflictions[aff].salve_cure == cure )
     return 1;
   
   return 0;
}


void reset_arrows( TIMER *self )
{
   arrows_timer = 0;
   clientff( "\r\n" C_R "[" C_D "Resetting arrows balance." C_R "]"
	     C_0 "\r\n" );
   show_prompt( );
}


void disallow_diagdef( TIMER *self )
{
   allow_diagdef = 0;
}



void reset_defence( TIMER *self )
{
   clientff( "\r\n" C_R "[" C_D "Resetting defence '" C_G "%s'" C_D "." C_R "]" C_0 "\r\n",
	     defences[self->data[0]].name );
   show_prompt( );
   defences[self->data[0]].on = 0;
   defences[self->data[0]].tried = 0;
}



void keep_up_defences( )
{
   int i;
   
   for ( i = 0; defences[i].name; i++ )
     {
	if ( defences[i].keepon && !defences[i].on && !defences[i].tried )
	  {
	     if ( ( defences[i].needy && ( !have_balance( ) || need_balance_for_defences ) ) ||
		  ( defences[i].standing && p_prone ) )
	       continue;
	     
	     if ( defences[i].level > trigger_level )
	       continue;
	     
	     if ( ( defences[i].needherb && balance_herbs ) ||
		  ( defences[i].needsalve && balance_salve ) ||
		  ( defences[i].needsmoke && balance_smoke ) )
	       continue;
	     
	     if ( defences[i].action )
	       {
		  char buf[256];
		  
		  /* If it begins with 'eat', also send 'outr'. */
		  
		  if ( !strncmp( defences[i].action, "eat ", 4 ) )
		    {
		       if ( afflictions[AFF_ANOREXIA].afflicted )
			 continue;
		       
		       if ( writhing )
			 continue;
		       
		       sprintf( buf, "outr %s\r\n", defences[i].action + 4 );
		       send_to_server_d( buf );
		    }
		  else if ( !strcmp( defences[i].action, "apply fenugreek" ) )
		    {
		       if ( writhing )
			 continue;
		       
		       send_to_server_d( "outr fenugreek\r\n" );
		    }
		  else if ( !strncmp( defences[i].action, "smoke pipe with ", 16 ) )
		    {
		       if ( !pipes_lit )
			 send_to_server_d( "light pipes\r\n" );
		       
		       strcpy( last_pipe, defences[i].action + 16 );
		    }
		  
		  if ( !strcmp( defences[i].action, "def" ) ||
		       !strcmp( defences[i].action, "diag" ) )
		    {
		       allow_diagdef = 1;
		       
		       add_timer( "allow_diagdef", 3, disallow_diagdef, 0, 0, 0 );
		    }
		  
		  send_action( defences[i].action );
	       }
	     else
	       send_to_server( defences[i].name );
	     send_to_server( "\r\n" );
	     defences[i].tried = 1;
	     
 	     /* We'll just send its name, if there's no action specified. */
	     clientf( C_B "(" );
	     clientf( defences[i].action ? defences[i].action : defences[i].name );
	     clientf( ") " C_0 );
	     
	     if ( !defences[i].nodelay )
	       {
		  char buf[256];
		  
		  sprintf( buf, "reset_defence_%s", defences[i].name );
		  add_timer( buf, 3, reset_defence, i, 0, 0 );
	       }
	     
	     if ( defences[i].takeherb )
	       {
		  balance_herbs = 1;
		  add_timer( "herb_balance_reset", reset_time, balance_reset, 2, 0, 0 );
	       }
	     
	     if ( defences[i].takesalve )
	       {
		  balance_salve = 1;
		  add_timer( "salve_balance_reset", reset_time, balance_reset, 3, 0, 0 );
	       }
	     
	     if ( defences[i].takesmoke )
	       {
		  /* Safe to assume that the only thing we can smoke is linseed. */
		  /* That one has a longer salve balance. */
		  add_timer( "smoke_balance_reset", reset_time + 2, balance_reset, 4, 0, 0 );
		  balance_smoke = 1;
	       }
	     
	     if ( defences[i].takebalance )
	       need_balance_for_defences = 1;
	     
	     prompt_newline = 1;
	  }
     }
}



void imperian_process_server_prompt( LINE *line )
{
   char *custom_prompt;
   
   imperian_process_server_prompt_informative( line->line, line->raw_line );
   
   custom_prompt = imperian_build_custom_prompt( );
   if ( custom_prompt )
     {
	replace( custom_prompt );
     }
   
   imperian_process_server_prompt_action( line->line );
}



void imperian_process_server_prompt_informative( char *line, char *rawline )
{
   int i;
   
   DEBUG( "imperian_process_server_prompt_informative" );
   
   /* Illusion ended. */
   if ( ignore_next )
     ignore_next = 0;
   
   /* We have a prompt, so clear this one up. */
   if ( line_since_prompt )
     line_since_prompt = 0;
   
   /* Perhaps an illusion, ignore it. */
   if ( applied_legs || applied_arms )
     applied_legs = 0, applied_arms = 0;
   
   /* Illusion ended. */
   if ( ignore_illusion )
     ignore_illusion = 0;
   
   /* Diagnose output ended. */
   if ( parsing_diagnose )
     parsing_diagnose = 0;
   
   /* Fullsense output ended. */
   if ( parsing_fullsense )
     {
	parsing_fullsense = 0;
	check_fullsense_list( );
     }
   
   /* Show how many pipe lines were gagged. */
   if ( pipes_going_dark )
     {
	if ( pipes_going_dark > 1 )
	  prefixf( C_R "(" C_D "+%d more..." C_R ")" C_0 "\r\n", pipes_going_dark - 1 );
	pipes_going_dark = 0;
     }
   
   /* A possible instant kill? Show a very visible warning. */
   if ( instakill_warning )
     {
	prefix( C_R "[ " C_B "*" C_G "*" C_R "*" C_W
		 " Warning!! Possible instant kill! "
		 C_R "*" C_G "*" C_B "*" C_R " ]" C_0 "\r\n" );
	
	instakill_warning = 0;
     }
   
   /* Show a big nice Aeon message. */
   if ( !was_aeon && aeon )
     {
	was_aeon = 1;
	prefix( C_R "[ " C_B "*" C_G "*" C_R "*" C_W
		 " Warning! Aeon mode enabled! " C_R "*" 
		 C_G "*" C_B "*" C_R " ]" C_0 "\r\n" );
     }
   else if ( was_aeon && !aeon )
     {
	was_aeon = 0;
	prefix( C_R "[ " C_B "*" C_G "*" C_R "*" C_W
		 " Aeon mode disabled. " C_R "*" 
		 C_G "*" C_B "*" C_R " ]" C_0 "\r\n" );
     }
   
   if ( inventory_buf[0] )
     {
	parse_inventory( );
	inventory_buf[0] = 0;
	inventory = 0;
     }
   
   if ( rat_value )
     {
	prefixf( C_R "[" C_B "Rat worth: " C_G "%d" C_R "]\r\n" C_0,
		 rat_value );
	rat_value = 0;
     }
   
   if ( spider_value )
     {
	prefixf( C_R "[" C_B "Spider worth: " C_G "%d" C_R "]\r\n" C_0,
		 spider_value );
	spider_value = 0;
     }
   
   if ( serpent_value )
     {
	prefixf( C_R "[" C_B "Serpent worth: " C_G "%d" C_R "]\r\n" C_0,
		 serpent_value );
	serpent_value = 0;
     }
   
   if ( caiman_value )
     {
	prefixf( C_R "[" C_B "Caiman worth: " C_G "%d" C_R "]\r\n" C_0,
		 caiman_value );
	caiman_value = 0;
     }
   
   if ( locust_value )
     {
	prefixf( C_R "[" C_B "Locust worth: " C_G "%d" C_R "]\r\n" C_0,
		 locust_value );
	locust_value = 0;
     }
   
   /* No cures after we ate something? */
   /* Then we may have something wrong in our list. */
   if ( last_cure )
     {
	int first = 1;
	
	if ( last_cure == CURE_LAUREL )
	  aeon = 0;
	
	for ( i = 0; afflictions[i].name; i++ )
	  {
	     if ( !is_cured_by( i, last_cure ) )
	       continue;
	     
	     if ( !afflictions[i].afflicted )
	       continue;
	     
	     if ( check_madness( i ) )
	       continue;
	     
	     if ( first )
	       {
		  prefix( C_R "[Removing afflictions: " C_B );
		  first = 0;
	       }
	     else
	       prefix( C_R ", " C_B );
	     
	     prefix( afflictions[i].name );
	     afflictions[i].afflicted = 0;
	     afflictions[i].tried = 0;
	  }
	if ( !first )
	  prefix( C_R ".]\r\n" C_0 );
	
	last_cure = 0;
     }
   
   if ( last_cure_balance )
     {
	int first = 1;
	
	for ( i = 0; afflictions[i].name; i++ )
	  {
	     if ( !afflictions[i].afflicted )
	       continue;
	     
	     if ( last_cure_balance != 1 &&
		  !( last_cure_balance == 2 && afflictions[i].focus ) &&
		  !( last_cure_balance == 3 && afflictions[i].purge_blood ) )
	       continue;
	     
	     /* Madness and its subafflictions can't be cured by tree. */
	     if ( i == AFF_MADNESS )
	       continue;
	     if ( check_madness( i ) )
	       continue;
	     
	     if ( first )
	       {
		  prefix( C_R "[Removing afflictions: " C_B );
		  first = 0;
	       }
	     else
	       prefix( C_R ", " C_B );
	     prefix( afflictions[i].name );
	     afflictions[i].afflicted = 0;
	     afflictions[i].tried = 0;
	  }
	
	if ( !first )
	  clientf( C_R ".]\r\n" C_0 );
	
	last_cure_balance = 0;
     }
   
   if ( changed )
     {
	if ( show_afflictions )
	  list_afflictions( );
	changed = 0;
     }
   
   if ( show_limbs )
     {
	show_part_damage( );
	show_limbs = 0;
     }
   
   if ( prompt_lines > 0 )
     {
	lines_until_prompt--;
	if ( lines_until_prompt <= 0 )
	  {
	     show_our_prompt( );
	     lines_until_prompt = prompt_lines;
	  }
     }
   
   
   /* Now gather information, from the prompt. */
   
   normal_prompt = 0;
   
   /* Normal prompt? */
   if ( !strncmp( line, "H:", 2 ) )
     {
	char *p, *c;
	int to_blindness = 0;
	
	normal_prompt = 2;
	
	p_hp = p_mana = p_end = p_will = p_kai = 0;
	
	/* The color codes gave me a head ache. But no more.. */
	/* Repetitive, I know. */
	if ( ( p = strstr( rawline, "H:" ) ) )
	  {
	     if ( *(p+2) == '\33' )
	       {
		  c = p_color_hp;
		  p += 2;
		  while( *p != 'm' )
		    *c++ = *p++;
		  *c++ = *p++;
		  *c = 0;
		  p_hp = atoi( p );
	       }
	     else
	       {
		  p_color_hp[0] = 0;
		  p_hp = atoi( p + 2 );
	       }
	  }
	if ( ( p = strstr( rawline, "M:" ) ) )
	  {
	     if ( *(p+2) == '\33' )
	       {
		  c = p_color_mana;
		  p += 2;
		  while( *p != 'm' )
		    *c++ = *p++;
		  *c++ = *p++;
		  *c = 0;
		  p_mana = atoi( p );
	       }
	     else
	       {
		  p_color_mana[0] = 0;
		  p_mana = atoi( p + 2 );
	       }
	  }
	if ( ( p = strstr( rawline, "E:" ) ) )
	  {
	     if ( *(p+2) == '\33' )
	       {
		  c = p_color_end;
		  p += 2;
		  while( *p != 'm' )
		    *c++ = *p++;
		  *c++ = *p++;
		  *c = 0;
		  p_end = atoi( p );
	       }
	     else
	       {
		  p_color_end[0] = 0;
		  p_end = atoi( p + 2 );
	       }
	  }
	if ( ( p = strstr( rawline, "W:" ) ) )
	  {
	     if ( *(p+2) == '\33' )
	       {
		  c = p_color_will;
		  p += 2;
		  while( *p != 'm' )
		    *c++ = *p++;
		  *c++ = *p++;
		  *c = 0;
		  p_will = atoi( p );
	       }
	     else
	       {
		  p_color_will[0] = 0;
		  p_will = atoi( p + 2 );
	       }
	  }
	if ( ( p = strstr( rawline, "Kai:" ) ) )
	  {
	     if ( *(p+4) == '\33' )
	       {
		  c = p_color_kai;
		  p += 4;
		  while( *p != 'm' )
		    *c++ = *p++;
		  *c++ = *p++;
		  *c = 0;
		  p_kai = atoi( p );
	       }
	     else
	       {
		  p_color_kai[0] = 0;
		  p_kai = atoi( p + 4 );
	       }
	  }
	
//	sscanf( line, "H:%d M:%d E:%d W:%d", &p_hp, &p_mana, &p_end, &p_will );
	
	p_equilibrium = 0, p_balance = 0, p_prone = 0, p_flying = 0,
	  p_blind = 0, p_deaf = 0, p_stunned = 0;
	for ( p = line; *p != '<'; p++ );
	while( *p != '>' )
	  {
	     /* Blindness and balance are both 'b', so we have a problem. */
	     switch( *p )
	       {
		case 'e': p_equilibrium = 1; break;
		case 'b':
		  if ( !to_blindness )
		    p_balance = 1;
		  else
		    p_blind = 1;
		  break;
		case 'p': p_prone = 1; break;
		case 'f': p_flying = 1; break;
		case 'd': p_deaf = 1; break;
		case 's': p_stunned = 1; break;
		case ' ': to_blindness = 1; break;
	       }
	     p++;
	  }
	
	/* Handle the deafness/blindness defences. */
	for ( i = 0; defences[i].name; i++ )
	  {
	     if ( !strcmp( defences[i].name, "deafness" ) )
	       {
		  defences[i].on = p_deaf;
		  if ( defences[i].tried && p_deaf )
		    defences[i].tried = 0;
	       }
	     else if ( !strcmp( defences[i].name, "blindness" ) )
	       {
		  defences[i].on = p_blind;
		  if ( defences[i].tried && p_blind )
		    defences[i].tried = 0;
	       }
	     else
	       continue;
	     
	     break;
	  }
     }
   
   /* Black-out prompt? */
   if ( !strncmp( line, " ", 1 ) )
     normal_prompt = 1;
}


void reset_writhe( TIMER *self )
{
   writhing = 0;
   
   clientff( "\r\n" C_R "[" C_D "Resetting Writhe." C_R "]" C_0 "\r\n" );
   show_prompt( );
}

/* Writhing:
 *  0 - Not writhing out of anything.
 *  1 - Writhing out of something other than impale.
 *  2 - Writhing out of impale.
 */
int check_writhe( )
{
   int i;
   
   if ( writhe[WRITHE_IMPALE] && writhing != 2 )
     {
	if ( !have_balance( ) )
	  return 1;
	
	writhing = 2;
	clientff( C_W "(" C_G "writhe impale" C_W ") " C_0 );
	send_to_server_d( "writhe impale\r\n" );
	prompt_newline = 1;
	
	/* Add a timer to it. */
	add_timer( "reset_writhe", 6, reset_writhe, 0, 0, 0 );
     }
   
   if ( writhing )
     return 1;
   
   for ( i = 0; i < WRITHE_LAST; i++ )
     {
	if ( writhe[i] )
	  {
	     writhing = 1;
	     clientff( C_W "(" C_G "writhe" C_W ") " C_0 );
	     send_to_server_d( "writhe\r\n" );
	     prompt_newline = 1;
	     
	     /* Add a timer to it. */
	     add_timer( "reset_writhe", 6, reset_writhe, 0, 0, 0 );
	     return 1;
	  }
     }
   
   return 0;
}



void recover_kaiheal( TIMER *self )
{
   balance_kai_heal = 0;
}


void recover_transmute( TIMER *self )
{
   balance_transmute = 0;
}


void reset_sitting( TIMER *self )
{
   if ( sitting == 2 )
     sitting = 1;
}


void restore_selfishness( TIMER *self )
{
   int i;
   
   for ( i = 0; defences[i].name; i++ )
     if ( !strcmp( defences[i].name, "selfishness" ) )
       break;
   
   if ( defences[i].name )
     defences[i].keepon = 1;
   
   clientff( "\r\n" C_R "[" C_D "Restoring selfishness." C_R "]" C_0 "\r\n" );
   show_prompt( );
}


int check_aeon( )
{
   /*
    * 0 - All ok.
    * 1 - Just afflicted.
    * 2 - Waiting for balance if pipes not lit.
    * 3 - Waiting for lit pipes.
    * 4 - Waiting for diag/smoke laurel.
    */
   
   if ( aeon == 0 )
     return 0;
   
   /* Been afflicted? Do something, then. */
   if ( aeon == 1 )
     {
	/* Check for lit pipes. */
	if ( !arti_pipes && !pipes_lit )
	  aeon = 2;
	else
	  aeon = 3;
     }
   
   /* We need our pipes lit. */
   if ( aeon == 2 )
     {
	if ( !writhing && have_balance( ) )
	  {
	     clientff( C_W "(" C_G "light pipes" C_W ")\r\n" C_0 );
	     send_to_server( "light pipes\r\n" );
	     aeon = 3;
	  }
     }
   
   /* Do we have them ready? */
   if ( aeon == 3 )
     {
	if ( arti_pipes || pipes_lit )
	  {
	     /* If we really have Aeon, then diag will not go through. */
	     clientff( C_W "(" C_G "diag" C_W "/" C_G "smoke laurel" C_W ")\r\n" C_0 );
	     send_to_server( "diag\r\nsmoke pipe with laurel\r\n" );
	     
	     aeon = 4;
	  }
     }
   
   return 1;
}



void imperian_process_server_prompt_action( char *line )
{
   DEBUG( "imperian_process_server_prompt_action" );
   
   /* Not a standard "H:..." prompt? */
   if ( !normal_prompt )
     return;
   
   show_stats_on_term( );
   
   /* Is it ALL disabled? */
   if ( !trigger_level )
     return;
   
   /* A few checks on what we should do. */
   if ( keep_standing )
     {
	if ( !sitting && p_prone )
	  sitting = 1;
	if ( sitting == 2 && !p_prone )
	  sitting = 0;
     }
   
   /* Does this mean we can send our next defences? */
   if ( need_balance_for_defences == 2 && have_balance( ) )
     {
	need_balance_for_defences = 0;
     }
   
   /* We can't do anything like that... */
   if ( p_stunned )
     return;
   
   /* Off yer bed, lazy worker! */
   if ( keep_standing && sleeping == 1 )
     {
	/* We won't set prompt_newline on, because we return. */
	clientf( C_W "(" C_G "wake" C_W ")" C_0 "\r\n" );
	send_to_server_d( "wake\r\n" );
	sleeping = 2;
	sitting = 1;
	return;
     }
   
   if ( sleeping )
     return;
   
   if ( check_aeon( ) )
     return;
   
   if ( trigger_level >= 2 )
     check_writhe( );
   
   if ( curse_fear )
     {
	curse_fear = 0;
	
	if ( trigger_level )
	  {
	     clientf( C_B "(compose) " C_0 );
	     send_to_server_d( "compose\r\n" );
	     prompt_newline = 1;
	  }
     }
   
   if ( p_hp < p_heal_transmute && p_mana > use_mana_above &&
	!balance_transmute && have_balance( ) && !p_prone )
     {
	if ( p_hp < 100 )
	  {
	     clientf( C_B "(transmute 100) " C_0 );
	     send_to_server_d( "transmute 100\r\n" );
	  }
	else
	  {
	     
	     clientf( C_B "(transmute 50) " C_0 );
	     send_to_server_d( "transmute 50\r\n" );
	  }
	
	prompt_newline = 1;
	add_timer( "recover_transmute", 2, recover_transmute, 0, 0, 0 );
	balance_transmute = 1;
     }
   
   /* Keep pipes lit. */
   if ( keep_pipes && !pipes_lit && have_balance( ) )
     {
	clientf( C_W "(" C_G "light pipes" C_W ") " C_0 );
	send_to_server_d( "light pipes\r\n" );
	pipes_lit = 3;
	prompt_newline = 1;
     }
   
   /* Auto heal. */
   if ( !balance_healing && !afflictions[AFF_ANOREXIA].afflicted )
     {
	if ( p_hp < p_heal )
	  {
	     clientf( C_B "(drink health) " C_0 );
	     send_to_server_d( "drink health\r\n" );
	     waiting_for_healing_command = 1;
	     add_timer( "healing_balance_reset", reset_time, balance_reset, 5, 0, 0 );
	     balance_healing = 1;
	     prompt_newline = 1;
	  }
	else if ( p_mana < p_heal_mana )
	  {
	     clientf( C_B "(drink mana) " C_0 );
	     send_to_server_d( "drink mana\r\n" );
	     waiting_for_healing_command = 1;
	     add_timer( "healing_balance_reset", reset_time, balance_reset, 5, 0, 0 );
	     balance_healing = 1;
	     prompt_newline = 1;
	  }
     }
   
   /* Auto clot. */
   check_bleeding( );
   
   /* Auto heal, with toadstool. */
   if ( trigger_level >= 3 &&
	( p_hp < p_toad_heal || p_mana < p_toad_mana ) &&
	!balance_toadstool && !afflictions[AFF_ANOREXIA].afflicted )
     {
	if ( !outr_once || ( outr_once && !outr_toadstool ) )
	  send_to_server_d( "outr toadstool\r\n" );
	
	clientf( C_B "(eat toadstool) " C_0 );
	send_to_server_d( "eat toadstool\r\n" );
	waiting_for_toadstool_command = 1;
	add_timer( "toadstool_balance_reset", reset_time, balance_reset, 6, 0, 0 );
	balance_toadstool = 1;
	prompt_newline = 1;
     }
   
	/* Stand up and fight! */
   if ( !writhing && 
	partdamage[PART_LEFTLEG] == PART_HEALED &&
	partdamage[PART_RIGHTLEG] == PART_HEALED )
     {
	int unable_to_stand = 0;
	
	if ( keep_standing && sitting == 1 )
	  {
	     if ( can_kipup &&
		  ( partdamage[PART_LEFTARM] == PART_HEALED ||
		    partdamage[PART_RIGHTARM] == PART_HEALED ) )
	       {
		  clientf( C_W "(" C_G "kipup" C_W ") " C_0 );
		  send_to_server_d( "kipup\r\n" );
	       }
	     else if ( have_balance( ) )
	       {
		  clientf( C_W "(" C_G "stand" C_W ") " C_0 );
		  send_to_server_d( "stand\r\n" );
	       }
	     else
	       unable_to_stand = 1;
	     
	     if ( !unable_to_stand )
	       {
		  sitting = 2;
		  prompt_newline = 1;
		  add_timer( "reset_sitting", 2, reset_sitting, 0, 0, 0 );
		  /* Reset it, so we won't stand when we take keep_standing off. */
		  if ( tripped )
		    tripped = 0;
	       }
	  }
	else if ( !keep_standing && tripped )
	  {
	     if ( can_kipup &&
		  ( partdamage[PART_LEFTARM] == PART_HEALED ||
		    partdamage[PART_RIGHTARM] == PART_HEALED ) )
	       {
		  clientf( C_W "(" C_G "kipup" C_W ") " C_0 );
		  send_to_server_d( "kipup\r\n" );
	       }
	     else if ( have_balance( ) )
	       {
		  clientf( C_W "(" C_G "stand" C_W ") " C_0 );
		  send_to_server_d( "stand\r\n" );
	       }
	     else
	       unable_to_stand = 1;
	     
	     if ( !unable_to_stand )
	       {
		  tripped = 0;
		  prompt_newline = 1;
	       }
	  }
     }
   
   if ( arrows && have_balance( ) && !writhing && !arrows_timer )
     {
	arrows_timer = 1;
	add_timer( "reset_arrows", 2, reset_arrows, 0, 0, 0 );
	clientf( C_G "(" C_R "pull arrow from body" C_G ") " C_0 );
	prompt_newline = 1;
	send_to_server( "pull arrow from body\r\n" );
     }
   
   if ( p_hp < p_heal_kai && p_kai > 20 && !balance_kai_heal &&
	have_balance( ) )
     {
	clientf( C_B "(kai heal) " C_0 );
	send_to_server_d( "kai heal\r\n" );
	prompt_newline = 1;
	add_timer( "recover_kai_heal", 3, recover_kaiheal, 0, 0, 0 );
	balance_kai_heal = 1;
     }
   
   keep_up_defences( );
   
   if ( trigger_level == 3 )
     cure_afflictions( );
   if ( trigger_level >= 2 )
     cure_limbs( );
   
   /* Keep Kai boosting up. */
   if ( !kai_boost && p_kai > 10 && have_balance( ) )
     {
	clientf( C_W "(" C_G "kai boost" C_W ") " C_0 );
	send_to_server_d( "kai boost\r\n" );
	prompt_newline = 1;
	kai_boost = 1;
     }
   
   if ( lust[0] && have_balance( ) )
     {
	char buf[256];
	
	clientff( C_R "(" C_W "reject %s" C_R ") " C_0, lust );
	sprintf( buf, "reject %s\r\n", lust );
	send_to_server_d( buf );
	prompt_newline = 1;
	lust[0] = 0;
     }
   
   if ( selfishness_buffer[0] && have_balance( ) )
     {
	int i;
	
	for ( i = 0; defences[i].name; i++ )
	  if ( !strcmp( defences[i].name, "selfishness" ) )
	    break;
	
	if ( defences[i].name && !defences[i].on )
	  {
	     send_to_server( selfishness_buffer );
	     clientf( C_R "(" C_W "..." C_R ") " C_0 );
	     prompt_newline = 1;
	     
	     selfishness_buffer[0] = 0;
	  }
     }
   
   /* Perhaps it's needed to show a newline. */
   if ( prompt_newline )
     {
//	clientf( "\r\n" );
	prompt_newline = 0;
     }
}


int imperian_process_client_command( char *cmd )
{
   static char buf[256];
   int nr, nr2 = 0, i;
   
   DEBUG( "imperian_process_client_command" );
   
   /* Do we have a number here? */
   if ( isdigit( *(cmd+1) ) )
     {
	nr = atoi( cmd+1 );
	
	if ( nr < 1 )
	  {
	     return 1;
	  }
	
	for ( i = 0; afflictions[i].name; i++ )
	  {
	     if ( afflictions[i].afflicted )
	       nr2++;
	     
	     if ( nr == nr2 )
	       {
//		  clientf( C_R "[" );
		  if ( afflictions[i].salve_cure )
		    execute_affliction_action( i, BAL_SALVE );
		  else if ( afflictions[i].smoke_cure )
		    execute_affliction_action( i, BAL_SMOKE );
		  else if ( afflictions[i].herbs_cure )
		    execute_affliction_action( i, BAL_HERB );
		  else
		    execute_affliction_action( i, BAL_NONE );
		  
		  clientf( "\r\n" C_0 );
		  show_stats_on_term( );
		  return 0;
	       }
	  }
	
	clientf( C_R "[No such thing that you could do.]\r\n" C_0 );
     }
   else if ( *(cmd+1) == 'l' )
     {
	list_afflictions( );
     }
   else if ( *(cmd+1) == 'c' )
     {
	if ( *(cmd+2) == 't' )
	  {
	     for ( i = 0; afflictions[i].name; i++ )
	       afflictions[i].tried = 0;
	     
	     clientf( C_R "[No more afflictions marked as 'tried'.]\r\n" C_0 );
	  }
	else if ( isdigit( *(cmd+2) ) )
	  {
	     /* Clear an affliction. */
	     nr = atoi( cmd+2 );
	     for ( i = 0; afflictions[i].name; i++ )
	       {
		  if ( afflictions[i].afflicted )
		    nr2++;
		  if ( nr == nr2 )
		    {
		       afflictions[i].afflicted = 0;
		       sprintf( buf, C_R "[Removing affliction: %s.]\r\n" C_0,
				afflictions[i].name );
		       clientf( buf );
		       return 0;
		    }
	       }
	     
	     clientf( C_R "[No such affliction found.]\r\n" C_0 );
	  }
	else
	  {
	     /* Clear all afflictions. */
	     for ( i = 0; afflictions[i].name; i++ )
	       {
		  afflictions[i].afflicted = 0;
	       }
	     clientf( C_R "[All afflictions removed.]\r\n" C_0 );
	  }
     }
   else if ( *(cmd+1) == 'r' )
     {
	/* Read the trigger table again. */
	
	load_data( );
	
	clientff( C_R "[%d cures, %d afflictions, %d defences and %d triggers loaded from %d lines.]\r\n" C_0,
		 curesnr - 1, afflictionsnr - 1, defencesnr - 1, triggersnr - 1, data_nr );
	if ( data_errors )
	  clientff( C_R "[%d errors have been encountered. Check the console.]\r\n" C_0,
		    data_errors );
     }
   else if ( *(cmd+1) == 't' )
     {
	if ( *(cmd+2) == '\0' )
	  {
	     /* List em all.. */
	     list_triggers( NULL );
	  }
	else if ( *(cmd+2) == 't' )
	  {
	     /* List them who have that name. */
	     list_triggers( cmd+3 );
	  }
	else if ( *(cmd+2) == 'd' )
	  {
	     if ( *(cmd+3) == '\0' )
	       {
		  clientf( C_R "[Defences:]\r\n" C_0 );
		  for ( i = 0; defences[i].name; i++ )
		    {
		       sprintf( buf, C_R "[" C_G "Name: '" C_B "%s" C_G "'%s%s%s%s%s" C_R "]\r\n" C_0,
				defences[i].name,
				defences[i].keepon ? C_G " (" C_B "keepon" C_G ")" : "",
				defences[i].needy ? C_G " (" C_R "needy" C_G ")" : "",
				defences[i].action ? C_G " (" C_W : "",
				defences[i].action ? defences[i].action : "",
				defences[i].action ? C_G ")" : "" );
		       clientf( buf );
		    }
	       }
	     else if ( *(cmd+3) == 'm' || *(cmd+3) == 't' )
	       {
		  clientf( C_R "[Defence triggers:]\r\n" C_0 );
		  for ( i = 0; triggers[i].message; i++ )
		    {
		       if ( triggers[i].special != SPEC_DEFENCE )
			 continue;
		       
		       sprintf( buf, C_R "[" C_G "Message: \"%s\" (%s%s" C_G ")%s" C_R "]\r\n" C_0,
				triggers[i].message,
				!strcmp( triggers[i].defence, "any" ) ? C_W : C_R,
				triggers[i].defence,
				triggers[i].gives ? " (" C_B "give" C_G ")" : " (" C_R "take" C_G ")" );
		       clientf( buf );
		    }
	       }
	     else
	       clientf( C_R "[It's either `td or `tdm/`tdt]\r\n" C_0 );
	  }
	else
	  clientf( C_R "[It may be `t (triggers) `td (defences) or `tdm (defence message, or triggers)]\r\n" C_0 );
     }
   else if ( *(cmd+1) == 'a' )
     {
	if ( *(cmd+2) == '\0' )
	  clientf( C_R "[Add what?]\r\n" );
	else
	  {
	     add_affliction( cmd+2 );
	  }
     }
   else if ( *(cmd+1) == 'd' )
     {
	if ( *(cmd+2) == 'e' && *(cmd+3) == 'f' )
	  {
	     int first = 1;
	     
	     clientf( C_R "[Defences:]\r\n" C_0 );
	     for ( i = 0; defences[i].name; i++ )
	       {
		  if ( !defences[i].on )
		    continue;
		  
		  sprintf( buf, "%s%s\r\n" C_0,
			   defences[i].keepon ? C_W : C_G, defences[i].name );
		  clientf( buf );
	       }
	     
	     for ( i = 0; defences[i].name; i++ )
	       {
		  if ( defences[i].on || !defences[i].keepon )
		    continue;
		  
		  if ( first )
		    {
		       clientf( C_R "[Trying to get on:]\r\n" C_0 );
		       first = 0;
		    }
		  
		  sprintf( buf, C_G "%s\r\n" C_0, defences[i].name );
		  clientf( buf );
	       }
	  }
	else if ( *(cmd+2) == 't' )
	  {
	     if ( *(cmd+3) == 'm' )
	       {
		  if ( !isdigit( *(cmd+4) ) )
		    clientf( C_R "[I need a number. (`dtm50)]\r\n" C_0 );
		  else
		    {
		       p_toad_mana = atoi( cmd+4 );
		       sprintf( buf, C_R "[Will now eat toadstool if mana is lower than %d.]\r\n" C_0,
				p_toad_mana );
		       clientf( buf );
		    }
	       }
	     else if ( !isdigit( *(cmd+3) ) )
	       clientf( C_R "[I need a number. (`dt50)]\r\n" C_0 );
	     else
	       {
		  p_toad_heal = atoi( cmd+3 );
		  sprintf( buf, C_R "[Will now eat toadstool if lower than %d.]\r\n" C_0,
			   p_toad_heal );
		  clientf( buf );
	       }
	  }
	else if ( *(cmd+2) == 'm' )
	  {
	     if ( !isdigit( *(cmd+3) ) )
	       clientf( C_R "[I need a number. (`dm50)]\r\n" C_0 );
	     else
	       {
		  p_heal_mana = atoi( cmd+3 );
		  sprintf( buf, C_R "[Will now drink mana if lower than %d.]\r\n" C_0, p_heal_mana );
		  clientf( buf );
	       }
	  }
	else if ( *(cmd+2) == 'k' )
	  {
	     if ( !isdigit( *(cmd+3) ) )
	       clientf( C_R "[I need a number. (`dk50)]\r\n" C_0 );
	     else
	       {
		  p_heal_kai = atoi( cmd+3 );
		  sprintf( buf, C_R "[Will now Kai Heal if lower than %d.]\r\n" C_0, p_heal_kai );
		  clientf( buf );
	       }
	  }
	else if ( *(cmd+2) == 'x' )
	  {
	     if ( !isdigit( *(cmd+3) ) )
	       clientf( C_R "[I need a number. (`dx50)]\r\n" C_0 );
	     else
	       {
		  p_heal_transmute = atoi( cmd+3 );
		  sprintf( buf, C_R "[Will now transmute 50 mana if lower than %d.]\r\n" C_0, p_heal_transmute );
		  clientf( buf );
	       }
	  }
	else if ( !isdigit( *(cmd+2) ) )
	  clientf( C_R "[I need a number. (`d50)]\r\n" C_0 );
	else
	  {
	     p_heal = atoi( cmd+2 );
	     sprintf( buf, C_R "[Autodrink set on %d.]\r\n" C_0, p_heal );
	     clientf( buf );
	  }
     }
   else if ( *(cmd+1) == 'o' )
     {
	clientf( C_R "[Option            - State]\r\n" C_0 );
	
	/* Trigger level. */
	clientff( C_G " Trigger level       " C_B "%5d\r\n" C_0, trigger_level );
	
	/* Auto drink health vial. */
	clientff( C_G " Drink health below  " C_B "%5d\r\n" C_0, p_heal );
	
	/* Auto drink mana vial. */
	clientff( C_G " Drink mana below    " C_B "%5d\r\n" C_0, p_heal_mana );
	
	/* Auto Kai Heal. */
	clientff( C_G " Kai Heal below      " C_B "%5d\r\n" C_0, p_heal_kai );
	
	/* Auto Transmute. */
	clientff( C_G " Transmute below     " C_B "%5d\r\n" C_0, p_heal_transmute );
	
	/* Same for toadstool. */
	clientff( C_G " Toadstool for hp    " C_B "%5d\r\n" C_0, p_toad_heal );
	
	/* Toadstool, but for mana */
	clientff( C_G " Toadstool for mana  " C_B "%5d\r\n" C_0, p_toad_mana );
	
	/* Auto clotting. */
	clientff( C_G " Use mana above      " C_B "%5d\r\n" C_0, use_mana_above );
	
	/* Clot mana value. */
	clientff( C_G " One clot takes      " C_B "%5d\r\n" C_0, clot_mana_usage );
	
	/* How many until our prompt? */
	clientff( C_G " Lines for prompt    " C_B "%5d\r\n" C_0, prompt_lines );
	
	/* Do we fill the terminal with our nice informations? */
	clientff( C_G " Show info on term   " C_B "%5d\r\n" C_0, show_on_term );
	
	/* Can we sit, or not? */
	clientff( C_G " Keep standing       " C_B "%5d\r\n" C_0, keep_standing );
	
	/* Should we send the command two times? */
	clientff( C_G " Double actions      " C_B "%5d\r\n" C_0, double_actions );
	
	/* Shoud we send the afflictions' action, if already tried it? */
	clientff( C_G " Don't send tried    " C_B "%5d\r\n" C_0, mark_tried );
	
	/* Should we keep all our pipes lit? */
	clientff( C_G " Keep pipes lit      " C_B "%5d\r\n" C_0, keep_pipes );
	
	/* Should we use these to cure with? */
	clientff( C_G " Cure with tree      " C_B "%5d\r\n" C_0, cure_with_tree );
	clientff( C_G " Cure with focus     " C_B "%5d\r\n" C_0, cure_with_focus );
	clientff( C_G " Cure with purge     " C_B "%5d\r\n" C_0, cure_with_purge );
	
	/* Can we use the ability kipup to stand? */
	clientff( C_G " Can kipup           " C_B "%5d\r\n" C_0, can_kipup );
	
	/* Should we send an affliction list each time? */
	clientff( C_G " Show afflictions    " C_B "%5d\r\n" C_0, show_afflictions );
	
	/* Tree recovery time. */
	clientff( C_G " Tree recovery time  " C_B "%5d\r\n" C_0, tree_recovery );
     }
   else if ( *(cmd+1) == 's' )
     {
	if ( *(cmd+2) == 't' && *(cmd+3) == 'a' && *(cmd+4) == 't' )
	  {
	     build_stats( 0 );
	     
	     for ( i = 0; stats[i][0]; i++ )
	       clientff( "%s\r\n", stats[i] );
	  }
	else if ( *(cmd+2) == 'h' )
	  {
	     clientf( C_R "[Followings options can be set:]\r\n"
		      " `st  - Show info on the terminal the bot runs in.\r\n"
		      " `sd  - If afflicted with stupidity, double the cure commands.\r\n"
		      " `sm  - Mark afflictions as 'tried'. Good for illusions, bad otherwise.\r\n"
		      " `ss  - Keep standing, never allow to be sitting.\r\n"
		      " `sp  - Keep all pipes lit.\r\n"
		      " `sk  - Count kicks and punches, on each limb.\r\n"
		      " `sa  - Show the list of afflictions each time.\r\n"
		      " `so  - Outrift a herb only once. Useful in the Arena.\r\n"
		      " `sat - Use the tree tattoo to cure afflictions.\r\n"
		      " `saf - Use focus to cure afflictions.\r\n"
		      " `sap - Use purge blood to cure afflictions.\r\n"
		      " `sak - Use kipup to stand, when possible.\r\n" C_0 );
	  }
	else if ( *(cmd+2) == 'a' )
	  {
	     if ( *(cmd+3) == 't' )
	       {
		  cure_with_tree = cure_with_tree ? 0 : 1;
		  if ( cure_with_tree )
		    clientf( C_R "[The tree tattoo will now be used to cure afflictions.]\r\n" C_0 );
		  else
		    clientf( C_R "[The tree tattoo will no longer be used to cure afflictions.]\r\n" C_0 );
	       }
	     else if ( *(cmd+3) == 'f' )
	       {
		  cure_with_focus = cure_with_focus ? 0 : 1;
		  if ( cure_with_focus )
		    clientf( C_R "[Focusing will now be used to cure afflictions.]\r\n" C_0 );
		  else
		    clientf( C_R "[Focusing will no longer be used to cure afflictions.]\r\n" C_0 );
	       }
	     else if ( *(cmd+3) == 'p' )
	       {
		  cure_with_purge = cure_with_purge ? 0 : 1;
		  if ( cure_with_purge )
		    clientf( C_R "[Purge blood will now be used to cure afflictions.]\r\n" C_0 );
		  else
		    clientf( C_R "[Purge blood will no longer be used to cure afflictions.]\r\n" C_0 );
	       }
	     else if ( *(cmd+3) == 'k' )
	       {
		  can_kipup = can_kipup ? 0 : 1;
		  if ( can_kipup )
		    clientf( C_R "[Kipup will now be used instead of stand, when possible.]\r\n" C_0 );
		  else
		    clientf( C_R "[Kipup will no longer be used.]\r\n" C_0 );
	       }
	     else
	       {
		  show_afflictions = show_afflictions ? 0 : 1;
		  
		  if ( show_afflictions )
		    clientf( C_R "[Each time you get an affliction, the affliction list will be shown.]\r\n" C_0 );
		  else
		    clientf( C_R "[The affliction list will not be shown at each affliction you get.]\r\n" C_0 );
	       }
	  }
	else if ( *(cmd+2) == 'c' )
	  {
	     if ( *(cmd+3) == 'm' )
	       {
		  if ( !isdigit( *(cmd+4) ) )
		    clientf( C_R "[I need a number, which is how much mana a Clot takes. (`scm5)]\r\n" C_0 );
		  else
		    {
		       clot_mana_usage = atoi( cmd+4 );
		       sprintf( buf, C_R "[Clot mana usage set on %d.]\r\n" C_0, clot_mana_usage );
		       clientf( buf );
		    }
	       }
	     else
	       {
		  if ( !isdigit( *(cmd+3) ) )
		    clientf( C_R "[I need a number. (`sc50)]\r\n" C_0 );
		  else
		    {
		       use_mana_above = atoi( cmd+3 );
		       if ( use_mana_above )
			 sprintf( buf, C_R "[Will use mana only if above %d.]\r\n" C_0, use_mana_above );
		       else
			 sprintf( buf, C_R "[Will never use mana.]\r\n" C_0 );
		       clientf( buf );
		    }
	       }
	  }
	else if ( *(cmd+2) == 't' )
	  {
	     show_on_term = show_on_term ? 0 : 1;
	     if ( show_on_term )
	       clientf( C_R "[Information can now be displayed in the terminal.]\r\n" C_0 );
	     else
	       {
		  clientf( C_R "[Information will not be sent to the terminal.]\r\n" C_0 );
		  debugf( "\33[H\33[J" "No longer displaying information on the terminal." );
	       }
	  }
	else if ( *(cmd+2) == 'p' )
	  {
	     keep_pipes = keep_pipes ? 0 : 1;
	     if ( keep_pipes )
	       clientf( C_R "[Will now try to keep all pipes lit.]\r\n" C_0 );
	     else
	       clientf( C_R "[It was hard to remember all those pipes anyways.]\r\n" C_0 );
	  }
	else if ( *(cmd+2) == 'd' )
	  {
	     double_actions = double_actions ? 0 : 1;
	     if ( double_actions )
	       clientf( C_R "[If afflicted with stupidity, you will now eat/apply/etc two times.]\r\n" C_0 );
	     else
	       clientf( C_R "[Will now send the affliction actions only once.]\r\n" C_0 );
	  }
	else if ( *(cmd+2) == 'm' )
	  {
	     mark_tried = mark_tried ? 0 : 1;
	     if ( mark_tried )
	       clientf( C_R "[Afflictions will now be marked as 'tried', when their afflictions is sent.]\r\n" C_0 );
	     else
	       clientf( C_R "[Afflictions won't be marked as 'tried'. `ct is now useless.]\r\n" C_0 );
	  }
	else if ( *(cmd+2) == 's' )
	  {
	     keep_standing = keep_standing ? 0 : 1;
	     if ( keep_standing )
	       clientf( C_R "[Sitting is no longer an option for you..]\r\n" C_0 );
	     else
	       clientf( C_R "[You may now sit freely, once again.]\r\n" C_0 );
	  }
	else if ( *(cmd+2) == 'o' )
	  {
	     int i;
	     
	     for ( i = 0; cures[i].name; i++ )
	       cures[i].outr = 0;
	     outr_toadstool = 0;
	     
	     outr_once = outr_once ? 0 : 1;
	     if ( outr_once )
	       {
		  add_timer( "outr_limit", 1800, reset_outr, 0, 0, 0 );
		  clientf( C_R "[Only one outrift per herb will be done.]\r\n" C_0 );
	       }
	     else
	       {
		  del_timer( "outr_limit" );
		  clientf( C_R "[Will outrift at each herb used.]\r\n" C_0 );
	       }
	  }
	else if ( !isdigit( *(cmd+2) ) )
	  clientf( C_R "[I need a number. (`s0)]\r\n" C_0 );
	else
	  {
	     trigger_level = atoi( cmd+2 );
	     if ( trigger_level < 0 || trigger_level > 3 )
	       {
		  clientf( C_R "[There are only 4 levels. From level 0 to level 3.]\r\n" C_0 );
		  trigger_level = 2;
	       }
	     
	     sprintf( buf, C_R "[Trigger level set on %d.]\r\n" C_0, trigger_level );
	     clientf( buf );
	     if ( trigger_level == 0 )
	       clientf( C_R "[No trigger can cause anything to be sent.]\r\n" C_0 );
	     else if ( trigger_level == 1 )
	       clientf( C_R "[Safe triggers' action will be sent.]\r\n" C_0 );
	     else if ( trigger_level == 2 )
	       clientf( C_R "[Safe and Dangerous triggers' action will be sent.]\r\n" C_0 );
	     else if ( trigger_level == 3 )
	       clientf( C_R "[All triggers, including affliction triggers, will have their"
			" suggestion sent.]\r\n" C_0 );
	  }
     }
   else if ( *(cmd+1) == 'b' )
     {
	/* So we may eat/drink/etc again. */
	switch( *(cmd+2) )
	  {
	   case 'h':
	     clientf( C_R "[Herb balance reset.]\r\n" C_0 );
	     balance_herbs = 0; break;
	   case 'd':
	     clientf( C_R "[Healing elixirs balance reset.]\r\n" C_0 );
	     balance_healing = 0; break;
	   case 's':
	     clientf( C_R "[Salve balance reset.]\r\n" C_0 );
	     balance_salve = 0; break;
	   case 'p':
	     clientf( C_R "[Smoke balance reset.]\r\n" C_0 );
	     balance_smoke = 0; break;
	   case 'c':
	     clientf( C_R "[Curing elixirs balance reset.]\r\n" C_0 );
	     balance_cureelix = 0; break;
	   case 't':
	     clientf( C_R "[Toadstool balance reset.]\r\n" C_0 );
	     balance_toadstool = 0; break;
	   case 'a':
	     clientf( C_R "[Arm balance reset.]\r\n" C_0 );
	   default:
	     clientf( C_R "[All balances are now 0.]\r\n" C_0 );
	     balance_herbs = 0;
	     balance_healing = 0;
	     balance_salve = 0;
	     balance_smoke = 0;
	     balance_cureelix = 0;
	     balance_toadstool = 0;
	     balance_tree = 0;
	     balance_purge = 0;
	     balance_focus = 0;
	     balance_left_arm = 0;
	     balance_right_arm = 0;
	     for ( i = 0; defences[i].name; i++ )
	       defences[i].tried = 0;
	     for ( i = 0; afflictions[i].name; i++ )
	       afflictions[i].tried = 0;
	     break;
	     if ( need_balance_for_defences )
	       need_balance_for_defences = 2;
	     
	     waiting_for_healing_command = 0;
	     waiting_for_toadstool_command = 0;
	     waiting_for_smoke_command = 0;
	     waiting_for_salve_command = 0;
	     waiting_for_focus_command = 0;
	     waiting_for_purge_command = 0;
	     waiting_for_tree_command = 0;
	  }
     }
   else if ( *(cmd+1) == 'm' )
     {
	if ( *(cmd+2) == 0 )
	  {
	     show_part_damage( );
	  }
	else if ( *(cmd+2) == 'c' )
	  {
	     /* Perhaps it's looping too much. */
	     partdamage[PART_HEAD] = 0;
	     partdamage[PART_TORSO] = 0;
	     partdamage[PART_LEFTARM] = 0;
	     partdamage[PART_RIGHTARM] = 0;
	     partdamage[PART_LEFTLEG] = 0;
	     partdamage[PART_RIGHTLEG] = 0;
	     clientf( C_R "[All body part damage reset to 0.]\r\n" C_0 );
	  }
	else
	  {
	     if ( *(cmd+2) == 'h' )
	       partdamage[PART_HEAD] >= 2 ? partdamage[PART_HEAD] = 0 : partdamage[PART_HEAD]++;
	     else if ( *(cmd+2) == 't' )
	       partdamage[PART_TORSO] >= 2 ? partdamage[PART_TORSO] = 0 : partdamage[PART_TORSO]++;
	     else if ( *(cmd+2) == 'l' && *(cmd+3) == 'a' )
	       partdamage[PART_LEFTARM] >= 3 ? partdamage[PART_LEFTARM] = 0 : partdamage[PART_LEFTARM]++;
	     else if ( *(cmd+2) == 'r' && *(cmd+3) == 'a' )
	       partdamage[PART_RIGHTARM] >= 3 ? partdamage[PART_RIGHTARM] = 0 : partdamage[PART_RIGHTARM]++;
	     else if ( *(cmd+2) == 'l' && *(cmd+3) == 'l' )
	       partdamage[PART_LEFTLEG] >= 3 ? partdamage[PART_LEFTLEG] = 0 : partdamage[PART_LEFTLEG]++;
	     else if ( *(cmd+2) == 'r' && *(cmd+3) == 'l' )
	       partdamage[PART_RIGHTLEG] >= 3 ? partdamage[PART_RIGHTLEG] = 0 : partdamage[PART_RIGHTLEG]++;
	     else
	       {
		  clientf( C_R "[Possible options: `mh `mt `mla `mra `mll `mrl]\r\n" C_0 );
		  return 0;
	       }
	     
	     show_part_damage( );
	  }
     }
   else if ( *(cmd+1) == 'p' )
     {
	if ( !isdigit( *(cmd+2) ) )
	  clientf( C_R "[I need a number (`p3). 0 will stop the prompt to be displayed.]\r\n" C_0 );
	else
	  {
	     prompt_lines = atoi( cmd+2 );
	     if ( prompt_lines > 0 )
	       {
		  sprintf( buf, C_R "[Every %d normal prompt(s), ours will be displayed too.]\r\n" C_0,
			   prompt_lines );
		  lines_until_prompt = prompt_lines;
	       }
	     else
	       {
		  sprintf( buf, C_R "[Our prompt will never be displayed.]\r\n" C_0 );
		  clientf( buf );
		  lines_until_prompt = 1;
	       }
	  }
     }
   else if ( *(cmd+1) == 'k' )
     {
	if ( *(cmd+2) == '\0' )
	  clientf( C_R "[No defence specified. It can be `kcloak, `kall, `knone, etc.]\r\n" C_0 );
	else
	  {
	     int len = strlen( cmd + 2 );
	     int all = -1, found = 0;
	     
	     if ( !strcmp( cmd+2, "none" ) )
	       all = 0;
	     else if ( !strcmp( cmd+2, "all" ) )
	       all = 1;
	     
	     for ( i = 0; defences[i].name; i++ )
	       {
		  if ( all == -1 && strncmp( cmd+2, defences[i].name, len ) )
		    continue;
		  
		  if ( all == -1 )
		    {
		       defences[i].keepon = defences[i].keepon ? 0 : 1;
		       if ( defences[i].keepon )
			 sprintf( buf, C_R "[Will now try to keep %s on.]\r\n" C_0, defences[i].name );
		       else
			 sprintf( buf, C_R "[No longer keeping %s on.]\r\n" C_0, defences[i].name );
		       clientf( buf );
		       found = 1;
		       break;
		    }
		  else
		    {
		       /* Keep on only those that are on. */
		       if ( all == 0 || defences[i].on )
			 defences[i].keepon = all;
		    }
	       }
	     
	     if ( all != -1 )
	       {
		  if ( all == 1 )
		    clientf( C_R "[All set defences will now be kept on.]\r\n" C_0 );
		  else
		    clientf( C_R "[No longer keeping any defences on.]\r\n" C_0 );
	       }
	     else if ( !found )
	       clientf( C_R "[No such defence known. Try `td.]\r\n" C_0 );
	  }
     }
   else if ( *(cmd+1) == 'L' )
     {
	if ( !*(cmd+2) )
	  clientf( C_R "[Load from what section? (ex: `Lsection)]\r\n" C_0 );
	else
	  load_options( cmd+2 );
     }
   else if ( *(cmd+1) == 'P' )
     {
	if ( !*(cmd+2) )
	  {
	     sprintf( buf, C_R "[" C_G "%s" C_R "]\r\n" C_0,
		      custom_prompt[0] ? custom_prompt : "none" );
	     clientf( buf );
	  }
	else if ( *(cmd+2) == '0' )
	  {
	     custom_prompt[0] = 0;
	     clientf( C_R "[Custom prompt turned off.]\r\n" C_0 );
	  }
	else if ( *(cmd+2) != '\"' )
	  {
	     clientf( C_R "[Use quotation marks please.]\r\n" C_0 );
	  }
	else
	  {
	     get_string( cmd+2, custom_prompt, 4096 );
	     sprintf( buf, C_R "[Custom prompt now changed to \"" C_G "%s"
		      C_R "\".]\r\n" C_0, custom_prompt );
	     clientf( buf );
	  }
     }
   else if ( *(cmd+1) == 'h' )
     {
	clientfr( "Module: Imperian. Commands:" );
	clientf( " `l     - Lists all suspected afflictions.\r\n"
		 " `#     - Does the suggested command for affliction #. (`1)\r\n"
		 " `aS    - Add affliction S to the 'suspicious' list. (`aanorexia)\r\n"
		 " `c     - Clears all afflictions.\r\n"
		 " `c#    - Clears the affliction with number #. (`c2)\r\n"
		 " `ct    - Make all afflictions lose the 'tried' flag.\r\n"
		 " `r     - Reads the data table again.\r\n"
		 " `t     - Lists all the triggers.\r\n"
		 " `ttS   - Show all triggers on affliction S. (`ttanorexia)\r\n"
		 " `tdm   - Lists all defence triggers. Also same as `tdt.\r\n"
		 " `td    - Lists all the defences.\r\n"
		 " `d#    - Sets the health value on which to 'drink healh'. (`d100)\r\n"
		 " `dm#   - Sets the manavalue on which to 'drink mana'. (`dm100)\r\n"
		 " `dt#   - Same, but to 'eat toadstool'. (`dt40)\r\n"
		 " `dk#   - Sets the health value on which to 'kai heal'. (`dk30)\r\n"
		 " `o     - Shows the current state of some options.\r\n"
		 " `s#    - Sets trigger level. 0-None, 1-Safe, 2-Dangerous, 3-ALL. (`s2)\r\n"
		 " `sh    - Shows a list of things you can set.\r\n"
		 " `sc#   - Use mana only if above #. 0 is off. (`sc100)\r\n"
		 " `scm#  - How much mana does a single Clot use? (`scm7)\r\n"
		 " `b     - Resets all balance states to 0.\r\n"
		 " `bC    - Resets C, where C can be h, d, s, p, c, t. (`bd)\r\n"
		 " `m     - Shows body part damage.\r\n"
		 " `mc    - Resets all part damage to 0.\r\n"
		 " `mXY   - Adds one to damage at X=l/r/h/t Y=a/l. (`mrl means Right Leg)\r\n"
		 " `p#    - Number of normal prompts until we display ours. (`p5)\r\n"
		 " `kS    - Try to keep defence S on, or stop doing that. Can also be 'all' or 'none'.\r\n"
		 " `stat  - Shows the most important informations, all in a nice format. ;)\r\n"
		 " `LS    - Load options, from section S. (`Lcombat)\r\n"
		 " `P\"S\"  - Set custom prompt to S. `P0 will remove it.\r\n"
		 " `h     - This thing.\r\n"
		 " `help  - The MudBot's help.\r\n" );
	clientfr( "Replace # with a number. Affliction numbers can be found in `l" );
     }
   else
     {
	return 1;
     }
   
   show_stats_on_term( );
   return 0;
}


int imperian_process_client_aliases( char *line )
{
   DEBUG( "imperian_process_client_aliases" );
   
   if ( !strncmp( line, "conn_p", 6 ) )
     {
	clientf( C_R "[CONNECT 80.96.33.2:8080 HTTP/1.0]\r\n" C_0 );
	send_to_server( "CONNECT 80.96.33.2:8080 HTTP/1.0\r\n\r\n" );
	return 1;
     }
   
   if ( !strncmp( line, "conn_i", 6 ) )
     {
	clientf( C_R "[CONNECT imperian.com:23 HTTP/1.0]\r\n" C_0 );
	send_to_server( "CONNECT imperian.com:23 HTTP/1.0\r\n\r\n" );
	return 1;
     }
   
   /* Make sure we sent diag/def, so we don't parse illusions. */
   if ( !strcmp( line, "diag" ) || !strcmp( line, "diagnose" ) ||
	!strcmp( line, "def" ) || !strcmp( line, "defences" ) )
     {
	allow_diagdef = 1;
	
	add_timer( "allow_diagdef", 3, disallow_diagdef, 0, 0, 0 );
     }
   
   if ( trigger_level && ( !strncmp( line, "give ", 5 ) ||
			   !strncmp( line, "drop ", 5 ) ||
			   !strncmp( line, "offer ", 6 ) ) )
     {
	int i;
	
	for ( i = 0; defences[i].name; i++ )
	  if ( !strcmp( defences[i].name, "selfishness" ) )
	    break;
	
	if ( defences[i].name && defences[i].on )
	  {
	     /* Store the command somewhere, and send generosity first. */
	     if ( !selfishness_buffer[0] )
	       send_to_server( "generosity\r\n" );
	     
	     strcat( selfishness_buffer, line );
	     strcat( selfishness_buffer, "\r\n" );
	     
	     if ( defences[i].keepon )
	       {
		  add_timer( "restore_selfishness", 10, restore_selfishness, 0, 0, 0 );
		  
		  defences[i].keepon = 0;
	       }
	     
	     return 1;
	  }
     }
   
   if ( trigger_level && keep_standing &&
	( !strcmp( line, "sit" ) || !strncmp( line, "sit ", 4 ) ) )
     {
	clientfr( "Attempting to sit would be useless." );
	show_prompt( );
	
	return 1;
     }
   
   /* Strip newline. * I believe it's already stripped.
   while( line[ strlen( line ) - 1 ] == '\n' ||
	  line[ strlen( line ) - 1 ] == '\r' )
     line[ strlen( line ) - 1 ] = '\0';*/
   
   if ( aeon )
     {
	char buf[4096];
	
	get_string( line, buf, 4096 );
	
	if ( !strcmp( buf, "stop" ) )
	  {
	     aeon = 0;
	     trigger_level = 0;
	     clientff( C_R "[" C_W "Aeon stopped, and set on trigger level 0!" C_R "]\r\n" C_0 );
	     show_prompt( );
	     return 1;
	  }
	else if ( !strcmp( buf, "1" ) )
	  {
	     clientff( C_R "[" C_W "smoke pipe with laurel" C_R "]\r\n" C_0 );
	     send_to_server( "smoke pipe with laurel\r\n" );
	  }
	else if ( !strcmp( buf, "2" ) )
	  {
	     clientff( C_R "[" C_W "light pipes" C_R "]\r\n" C_0 );
	     send_to_server( "light pipes\r\n" );
	  }
	else
	  {
	     if ( buf[0] )
	       clientff( C_R "[" C_W "Command '" C_G "%s" C_W "' disabled! Only '" C_R "STOP" C_W "' will work now!" C_R "]\r\n" C_0, buf );
	     else
	       clientff( C_R "[ " C_W "Only '" C_R "STOP" C_W "' will work now!" C_R "]\r\n" C_0 );
	     
	     show_prompt( );
	     return 1;
	  }
     }
   
   return 0;
}



void imperian_mxp_enabled( )
{
   mxp_tag( TAG_TEMP_SECURE );
   mxp( "<!element Prompt FLAG='Prompt'>" );
}



/* Not yet a monster, but I sure hope it will be. :) */
char *imperian_build_custom_prompt( )
{
   static int last_health;
   char *c, *p, *b;
   char buf[4096], mxp1[64], mxp2[64];
   char iac_ga[] = { IAC, GA, 0 };
   char iac_eor[] = { EOR, 0 };
   int truth_value = 0, skip = 0, end;
   int *a_max_hp, *a_max_mana, *a_exp;
   int use_mxp = 0;
   
   if ( !custom_prompt[0] )
     return NULL;
   
   if ( ( use_mxp = mxp_tag( TAG_NOTHING ) ) )
     {
	sprintf( mxp1, C_0 "\33[%dz<Prompt>", TAG_LOCK_SECURE );
	sprintf( mxp2, "</Prompt>\33[%dz", use_mxp );
     }
   else
     mxp1[0] = 0, mxp2[0] = 0;
   
   /* Check for a special blackout prompt. */
   if ( normal_prompt == 1 )
     {
	int *a_hp, *a_mana;
	
	a_hp = get_variable( "a_hp" );
	a_mana = get_variable( "a_mana" );
	a_max_hp = get_variable( "a_max_hp" );
	a_max_mana = get_variable( "a_max_mana" );
	
	if ( !a_hp || !a_mana || !a_max_hp || !a_max_mana ||
	     ( !*a_hp && !*a_mana && !*a_max_hp && !*a_max_mana ) )
	  return NULL;
	
	p_hp = *a_hp / 11;
	p_mana = *a_mana / 11;
	
	sprintf( prompt, "%s" C_D "<" C_r "%d" C_D "/" C_r "%d" C_D "h " C_r "%d" C_D "/" C_r "%d" C_D "m> " C_0 "%s",
		 mxp1, *a_hp / 11, *a_max_hp / 11, *a_mana / 11, *a_max_mana / 11, mxp2 );
	return prompt;
     }
   
   if ( normal_prompt != 2 )
     return NULL;
   
   /* Here we go... */
   
   /* This is what we build from. */
   c = custom_prompt;
   /* And this is where we store the output. */
   p = prompt;
   
   b = mxp1;
   while ( *b )
     *(p++) = *(b++ );
   
   while( *c != '\0' )
     {
	if ( *c == '}' )
	  {
	     skip = 0;
	     c++;
	     continue;
	  }
	
	if ( skip )
	  {
	     c++;
	     continue;
	  }
	
	/* Just copy, if no special code. */
	if ( *c != '%' && *c != '^' && *c != '?' &&
	     *c != '!' && *c != '{' )
	  {
	     *p++ = *c++;
	     continue;
	  }
	
	/* In case we have something like "%\0" */
	end = 0;
	
	if ( *c == '%' )
	  {
	     c++;
	     
	     switch( *c )
	       {
		case 'h':
		  sprintf( buf, "%d", p_hp ); break;
		case 'm':
		  sprintf( buf, "%d", p_mana ); break;
		case 'e':
		  sprintf( buf, "%d", p_end ); break;
		case 'w':
		  sprintf( buf, "%d", p_will ); break;
		case 'k':
		  sprintf( buf, "%d", p_kai ); break;
		case 'H':
		  a_max_hp = get_variable( "a_max_hp" );
		  if ( a_max_hp )
		    sprintf( buf, "%d", *a_max_hp / 11 );
		  else
		    sprintf( buf, "0" );
		  break;
		case 'M':
		  a_max_mana = get_variable( "a_max_mana" );
		  if ( a_max_mana )
		    sprintf( buf, "%d", *a_max_mana / 11 );
		  else
		    sprintf( buf, "0" );
		  break;
		case 'x':
		  a_exp = get_variable( "a_exp" );
		  if ( a_exp )
		    sprintf( buf, "%d", *a_exp );
		  else
		    sprintf( buf, "0" );
		  break;
		case 'D': /* Damage. */
		  sprintf( buf, "%d", p_hp - last_health );
		  break;
		case 't': /* Taekate Stance. */
		  sprintf( buf, "%c", taekate_stance ? taekate_stance : 'n' );
		  break;
		case '%':
		  buf[0] = '%', buf[1] = 0; break;
		case 0:
		  end = 1; break;
		default:
		  sprintf( buf, "-?-" );
	       }
	     
	     if ( end )
	       break;
	     
	     b = buf;
	     while( *b )
	       *p++ = *b++;
	  }
	else if ( *c == '^' )
	  {
	     c++;
	     
	     switch( *c )
	       {
		case 'r':
		  sprintf( buf, "\33[0;31m" ); break;
		case 'g':
		  sprintf( buf, "\33[0;32m" ); break;
		case 'y':
		  sprintf( buf, "\33[0;33m" ); break;
		case 'b':
		  sprintf( buf, "\33[0;34m" ); break;
		case 'm':
		  sprintf( buf, "\33[0;35m" ); break;
		case 'c':
		  sprintf( buf, "\33[0;36m" ); break;
		case 'w':
		  sprintf( buf, "\33[0;37m" ); break;
		case 'R':
		  sprintf( buf, "\33[1;31m" ); break;
		case 'G':
		  sprintf( buf, "\33[1;32m" ); break;
		case 'Y':
		  sprintf( buf, "\33[1;33m" ); break;
		case 'B':
		  sprintf( buf, "\33[1;34m" ); break;
		case 'M':
		  sprintf( buf, "\33[1;35m" ); break;
		case 'C':
		  sprintf( buf, "\33[1;36m" ); break;
		case 'W':
		  sprintf( buf, "\33[1;37m" ); break;
		case 'x':
		  sprintf( buf, "\33[0;37m" ); break;
		case 'X':
		  sprintf( buf, iac_ga ); break;
		case 'E':
		  sprintf( buf, iac_eor ); break;
		  
		case '1':
		  if ( afflictions[AFF_RECKLESSNESS].afflicted )
		    sprintf( buf, C_R );
		  else
		    sprintf( buf, p_color_hp );
		  break;
		case '2':
		  sprintf( buf, p_color_mana ); break;
		case '3':
		  sprintf( buf, p_color_end ); break;
		case '4':
		  sprintf( buf, p_color_will ); break;
		case '5':
		  sprintf( buf, p_color_kai ); break;
		  
		case '^':
		  sprintf( buf, "^" ); break;
		case 0:
		  end = 1; break;
		default:
		  sprintf( buf, "-?-" );
	       }
	     
	     if ( end )
	       break;
	     
	     b = buf;
	     while( *b )
	       *p++ = *b++;
	  }
	else if ( *c == '?' )
	  {
	     c++;
//int p_balance, p_equilibrium, p_prone, p_flying;
//int p_blind, p_deaf, p_stunned;
	     
	     switch( *c )
	       {
		case 'b':
		  truth_value = p_balance; break;
		case 'e':
		  truth_value = p_equilibrium; break;
		case 'p':
		  truth_value = p_prone; break;
		case 'f':
		  truth_value = p_flying; break;
		case 'B':
		  truth_value = p_blind; break;
		case 'd':
		  truth_value = p_deaf; break;
		case 's':
		  truth_value = p_stunned; break;
		case 'L': /* Locked */
		  truth_value = mind_locked == 1; break;
		case 'M': /* Locking */
		  truth_value = mind_locked == 2; break;
		case 'a':
		  truth_value = ( p_prone || p_flying || p_blind || p_deaf || p_stunned || mind_locked || taekate_stance );
		  break;
		  
		case 'H': // h s p c t
		  truth_value = balance_herbs; break;
		case 'S':
		  truth_value = balance_salve; break;
		case 'P':
		  truth_value = balance_smoke; break;
		case 'C':
		  truth_value = balance_cureelix; break;
		case 'T':
		  truth_value = balance_tree; break;
		case 'F':
		  truth_value = balance_focus; break;
		case 'U':
		  truth_value = balance_purge; break;
		case 'W':
		  truth_value = writhing; break;
		case 'A':
		  truth_value = ( balance_herbs || balance_salve || balance_smoke ||
				  balance_cureelix || balance_tree || balance_focus ||
				  balance_purge || writhing ); break;
		  
		case 'k':
		  truth_value = p_kai ? 1 : 0; break;
		case 'l':
		  truth_value = !balance_left_arm; break;
		case 'r':
		  truth_value = !balance_right_arm; break;
		case 'D':
		  truth_value = ( last_health > p_hp ) ? 1 : 0; break;
		case 't':
		  truth_value = taekate_stance ? 1 : 0; break;
		  
		case '1':
		  truth_value = 1; break;
		case '0':
		default:
		  truth_value = 0; break;
	       }
	  }
	else if ( *c == '!' )
	  {
	     truth_value = truth_value ? 0 : 1;
	  }
	else if ( *c == '{' )
	  {
	     if ( !truth_value )
	       skip = 1;
	  }
	
	c++;
     }
   
   b = mxp2;
   while ( *b )
     *(p++) = *(b++ );
   
   *(p) = 0;
   
   last_health = p_hp;
   
   return prompt;
}


