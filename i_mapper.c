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


/* Imperian internal mapper. */

#define I_MAPPER_ID "$Name$ $Id$"

#include "module.h"
#include "i_mapper.h"


int mapper_version_major = 1;
int mapper_version_minor = 3;


char *i_mapper_id = I_MAPPER_ID "\r\n" I_MAPPER_H_ID "\r\n" HEADER_ID "\r\n" MODULE_ID "\r\n";

char *room_color = "\33[33m";
//char *room_color = "\33[35m";
int room_color_len = 5;

char map_file[256] = "IMap";

/* A few things we'll need. */
char *dir_name[] = 
{ "none", "north", "northeast", "east", "southeast", "south",
     "southwest", "west", "northwest", "up", "down", "in", "out", NULL };

char *dir_small_name[] = 
{ "-", "n", "ne", "e", "se", "s", "sw", "w", "nw", "u", "d", "in", "out", NULL };

int reverse_exit[] =
{ 0, EX_S, EX_SW, EX_W, EX_NW, EX_N, EX_NE, EX_E, EX_SE,
     EX_D, EX_U, EX_OUT, EX_IN, 0 };


ROOM_TYPE *room_types;
ROOM_TYPE *null_room_type;

/*
 ROOM_TYPE room_types[256] = 
{  { "Unknown",			C_r,	1, 1, 0, 0 },
   { "Undefined",		C_R,	1, 1, 0, 0 },
   { "Road",			C_D,	1, 1, 0, 0 },
   { "Path",			C_D,	1, 1, 0, 0 },
   { "Natural underground",	C_y,	1, 1, 0, 0 },
   { "River",			C_B,	3, 3, 1, 0 },
   { "Ocean",			C_B,	3, 3, 1, 0 },
   { "Grasslands",		C_G,	1, 1, 0, 0 },
   { "Forest",			C_g,	1, 1, 0, 0 },
   { "Beach",			C_Y,	1, 1, 0, 0 },
   { "Garden",			C_G,	1, 1, 0, 0 },
   { "Urban",			C_D,	1, 1, 0, 0 },
   { "Hills",			C_y,	1, 1, 0, 0 },
   { "Mountains",		C_y,	1, 1, 0, 0 },
   { "Desert",			C_y,	1, 1, 0, 0 },
   { "Jungle",			C_g,	1, 1, 0, 0 },
   { "Valley",			C_y,	1, 1, 0, 0 },
   { "Freshwater",		C_B,	3, 3, 1, 0 },
   { "Constructed underground",	C_y,	1, 1, 0, 0 },
   { "Swamp",			C_y,	1, 1, 0, 0 },
   { "Underworld",		C_R,	1, 1, 0, 0 },
   { "Sewer",			C_D,	1, 1, 0, 0 },
   { "Tundra",			C_W,	1, 1, 0, 0 },
   { "Sylayan city",		C_D,	1, 1, 0, 0 },
   { "Crags",			C_D,	1, 1, 0, 0 },
   { "Deep Ocean",		C_B,	1, 1, 0, 1 },
   { "Dark Forest",		C_D,	1, 1, 0, 0 },
   { "Polar",			C_W,	1, 1, 0, 0 },
   { "Warrens",			C_y,	1, 1, 0, 0 },
   { "Dwarven city",		C_D,	1, 1, 0, 0 },
   { "Underground Lake",	C_B,	3, 3, 1, 0 },
   { "Tainted Water",		C_R,	5, 5, 1, 0 },
   { "Farmland",		C_Y,	1, 1, 0, 0 },
   { "Village",			C_c,	1, 1, 0, 0 },
   { "Academia",		C_c,	1, 1, 0, 0 },
   { "the Stream of Consciousness",	C_m,	1, 1, 0, 0 },
   
   { NULL, NULL, 0, 0, 0 }
};*/


COLOR_DATA color_names[] =
{
     { "normal",	 C_0,		"\33[0m", 4 },
     { "red",		 C_r,		"\33[31m", 5 },
     { "green",		 C_g, 		"\33[32m", 5 },
     { "brown",		 C_y, 		"\33[33m", 5 },
     { "blue",		 C_b, 		"\33[34m", 5 },
     { "magenta",	 "\33[0;35m", 	"\33[35m", 5 },
     { "cyan",		 "\33[0;36m", 	"\33[36m", 5 },
     { "white",		 "\33[0;37m", 	"\33[37m", 5 },
     { "dark",		 C_D, 		C_D, 7 },
     { "bright-red",	 C_R, 		C_R, 7 },
     { "bright-green",	 C_G, 		C_G, 7 },
     { "bright-yellow",	 C_Y, 		C_Y, 7 },
     { "bright-blue",	 C_B, 		C_B, 7 },
     { "bright-magenta", "\33[1;35m", 	"\33[1;35m", 7 },
     { "bright-cyan",	 C_C, 		C_C, 7 },
     { "bright-white",	 C_W, 		C_W, 7 },
   
     { NULL, NULL, 0 }
};



/* Misc. */
int parsing_room;

int mode;
#define NONE		0
#define FOLLOWING	1
#define CREATING	2
#define GET_UNLOST	3

int get_unlost_exits;
int get_unlost_detected_exits[13];
int auto_walk;
int pear_defence;
int sense_message;
int scout_list;
int evstatus_list;
int destroying_world;
int door_closed;
int door_locked;
int locate_arena;
int capture_special_exit;
char cse_command[5120];
char cse_message[5120];

/* settings.mapper.txt options. */
int disable_swimming;
int disable_wholist;
int disable_alertness;
int disable_locating;
int disable_areaname;


char *dash_command;

int set_length_to;
int switch_exit_stops_mapping;
int update_area_from_survey;
int use_direction_instead;
int unidirectional_exit;

ROOM_DATA *world;
ROOM_DATA *world_last;
ROOM_DATA *current_room;
ROOM_DATA *hash_world[MAX_HASH];

ROOM_DATA *link_next_to;

AREA_DATA *areas;
AREA_DATA *areas_last;
AREA_DATA *current_area;

EXIT_DATA *global_special_exits;

ROOM_DATA *map[MAP_X+2][MAP_Y+2];
char extra_dir_map[MAP_X+2][MAP_Y+2];

/* New! */
MAP_ELEMENT map_new[MAP_X][MAP_Y];

int last_vnum;

/* Path finder chains. */
ROOM_DATA *pf_current_openlist;
ROOM_DATA *pf_new_openlist;


/* Command queue. -1 is look, positive number is direction. */
int queue[256];
int q_top;

int auto_bump;
ROOM_DATA *bump_room;
int bump_exits;


/* Here we register our functions. */

void i_mapper_module_init_data( );
void i_mapper_module_unload( );
void i_mapper_process_server_line_prefix( char *colorless_line, char *colorful_line, char *raw_line );
void i_mapper_process_server_line_suffix( char *colorless_line, char *colorful_line, char *raw_line );
void i_mapper_process_server_prompt_informative( char *line, char *rawline );
void i_mapper_process_server_prompt_action( char *rawline );
int  i_mapper_process_client_command( char *cmd );
int  i_mapper_process_client_aliases( char *cmd );


ENTRANCE( i_mapper_module_register )
{
   self->name = strdup( "IMapper" );
   self->version_major = mapper_version_major;
   self->version_minor = mapper_version_minor;
   self->id = i_mapper_id;
   
   self->init_data = i_mapper_module_init_data;
   self->unload = i_mapper_module_unload;
   self->process_server_line_prefix = i_mapper_process_server_line_prefix;
   self->process_server_line_suffix = i_mapper_process_server_line_suffix;
   self->process_server_prompt_informative = i_mapper_process_server_prompt_informative;
   self->process_server_prompt_action = i_mapper_process_server_prompt_action;
   self->process_client_command = NULL;
   self->process_client_aliases = i_mapper_process_client_aliases;
   self->build_custom_prompt = NULL;
   self->main_loop = NULL;
   self->update_descriptors = NULL;
   
   GET_FUNCTIONS( self );
}



int get_free_vnum( )
{
   last_vnum++;
   return last_vnum;
}



void link_to_area( ROOM_DATA *room, AREA_DATA *area )
{
   DEBUG( "link_to_area" );
   
   /* Link to an area. */
   if ( area )
     {
	room->next_in_area = area->rooms;
	area->rooms = room;
	
	room->area = area;
     }
   else if ( current_area )
     link_to_area( room, current_area );
   else if ( areas )
     {
	debugf( "No current area set, while trying to link a room." );
	link_to_area( room, areas );
     }
   else
     clientfr( "Can't link this room anywhere." );
}



void unlink_from_area( ROOM_DATA *room )
{
   ROOM_DATA *r;
   
   DEBUG( "unlink_from_area" );
   
   /* Unlink from area. */
   if ( room->area->rooms == room )
     room->area->rooms = room->next_in_area;
   else
     for ( r = room->area->rooms; r; r = r->next_in_area )
       {
	  if ( r->next_in_area == room )
	    {
	       r->next_in_area = room->next_in_area;
	       break;
	    }
       }
}



ROOM_DATA *create_room( int vnum_create )
{
   ROOM_DATA *new_room;
   
   DEBUG( "create_room" );
   
   /* calloc will also clear everything to 0, for us. */
   new_room = calloc( sizeof( ROOM_DATA ), 1 );
   
   /* Init. */
   new_room->name = NULL;
   new_room->special_exits = NULL;
   new_room->room_type = null_room_type;
   if ( vnum_create < 0 )
     new_room->vnum = get_free_vnum( );
   else
     new_room->vnum = vnum_create;
   
   /* Link to main chain. */
   if ( !world )
     {
	world = new_room;
	world_last = new_room;
     }
   else
     world_last->next_in_world = new_room;
   
   new_room->next_in_world = NULL;
   world_last = new_room;
   
   /* Link to hashed table. */
   if ( !hash_world[new_room->vnum % MAX_HASH] )
     {
	hash_world[new_room->vnum % MAX_HASH] = new_room;
	new_room->next_in_hash = NULL;
     }
   else
     {
	new_room->next_in_hash = hash_world[new_room->vnum % MAX_HASH];
	hash_world[new_room->vnum % MAX_HASH] = new_room;
     }
   
   /* Link to current area. */
   link_to_area( new_room, current_area );
   
   return new_room;
}



AREA_DATA *create_area( )
{
   AREA_DATA *new_area;
   
   DEBUG( "create_area" );
   
   new_area = calloc( sizeof( AREA_DATA ), 1 );
   
   /* Init. */
   new_area->name = NULL;
   new_area->rooms = NULL;
   
   /* Link to main chain. */
   if ( !areas )
     {
	areas = new_area;
	areas_last = new_area;
     }
   else
     areas_last->next = new_area;
   
   new_area->next = NULL;
   areas_last = new_area;
   
   return new_area;
}



EXIT_DATA *create_exit( ROOM_DATA *room )
{
   EXIT_DATA *new_exit;
   
   new_exit = calloc( sizeof ( EXIT_DATA ), 1 );
   
   new_exit->vnum = -1;
   
   /* If room is null, create in the global list. */
   if ( room )
     {
	new_exit->next = room->special_exits;
	room->special_exits = new_exit;
	new_exit->owner = room;
     }
   else
     {
	new_exit->next = global_special_exits;
	global_special_exits = new_exit;
	new_exit->owner = NULL;
     }
   
   return new_exit;
}



void free_exit( EXIT_DATA *spexit )
{
   if ( spexit->command )
     free( spexit->command );
   if ( spexit->message )
     free( spexit->message );
   free( spexit );
}



void check_pointed_by( ROOM_DATA *room )
{
   ROOM_DATA *r;
   EXIT_DATA *e;
   
   DEBUG( "check_pointed_by" );
   
   if ( !room || destroying_world )
     return;
   
   for ( r = world; r; r = r->next_in_world )
     {
	for ( e = r->special_exits; e; e = e->next )
	  {
	     if ( e->to == room )
	       {
		  room->pointed_by = 1;
		  return;
	       }
	  }
     }
   
   room->pointed_by = 0;
}



void free_room( ROOM_DATA *room )
{
   EXIT_DATA *e, *e_next;
   ROOM_DATA *r;
   
   if ( room->name )
     free( room->name );
   
   for ( e = room->special_exits; e; e = e_next )
     {
	e_next = e->next;
	
	if ( e->to )
	  {
	     r = e->to;
	     e->to = NULL;
	     check_pointed_by( r );
	  }
	
	free_exit( e );
     }
   
   free( room );
}


void destroy_room( ROOM_DATA *room )
{
   ROOM_DATA *r;
   
   unlink_from_area( room );
   
   /* Unlink from world. */
   if ( world == room )
     {
	world = room->next_in_world;
	if ( room == world_last )
	  world_last = world;
     }
   else
     for ( r = world; r; r = r->next_in_world )
       {
	  if ( r->next_in_world == room )
	    {
	       r->next_in_world = room->next_in_world;
	       if ( room == world_last )
		 world_last = r;
	       break;
	    }
       }
   
   /* Unlink from hash table. */
   if ( room == hash_world[room->vnum % MAX_HASH] )
     hash_world[room->vnum % MAX_HASH] = room->next_in_hash;
   else
     {
	for ( r = hash_world[room->vnum % MAX_HASH]; r; r = r->next_in_hash )
	  {
	     if ( r->next_in_hash == room )
	       {
		  r->next_in_hash = room->next_in_hash;
		  break;
	       }
	  }
     }
   
   /* Free it up. */
   free_room( room );
}


void destroy_area( AREA_DATA *area )
{
   AREA_DATA *a;
   
   /* Unlink from areas. */
   if ( area == areas )
     {
	areas = area->next;
	if ( area == areas_last )
	  areas_last = areas; /* Always null, anyways. */
     }
   else
     for ( a = areas; a; a = a->next )
       {
	  if ( a->next == area )
	    {
	       a->next = area->next;
	       if ( area == areas_last )
		 areas_last = a;
	       break;
	    }
       }
   
   /* Free it up. */
   free( area );
}



void destroy_exit( EXIT_DATA *spexit )
{
   ROOM_DATA *room;
   EXIT_DATA *e;
   
   if ( ( room = spexit->to ) )
     {
	spexit->to = NULL;
	check_pointed_by( room );
     }
   
   /* Unlink from room, or global. */
   if ( ( room = spexit->owner ) )
     {
	if ( room->special_exits == spexit )
	  room->special_exits = spexit->next;
	else
	  for ( e = room->special_exits; e; e = e->next )
	    if ( e->next == spexit )
	      {
		 e->next = spexit->next;
		 break;
	      }
     }
   else
     {
	if ( global_special_exits == spexit )
	  global_special_exits = spexit->next;
	else
	  for ( e = global_special_exits; e; e = e->next )
	    if ( e->next == spexit )
	      {
		 e->next = spexit->next;
		 break;
	      }
     }
   
   /* Free it up. */
   free_exit( spexit );
}


/* Destroy everything. */
void destroy_map( )
{
   ROOM_DATA *room, *next_room;
   AREA_DATA *area, *next_area;
   int i;
   
   DEBUG( "destroy_map" );
   
   destroying_world = 1;
   
   /* Rooms. */
   for ( room = world; room; room = next_room )
     {
	next_room = room->next_in_world;
	
	/* Free it up. */
	free_room( room );
     }
   
   /* Areas. */
   for ( area = areas; area; area = next_area )
     {
	next_area = area->next;
	
	/* Free it up. */
	if ( area->name )
	  free( area->name );
	free( area );
     }
   
   /* Hash table. */
   for ( i = 0; i < MAX_HASH; i++ )
     hash_world[i] = NULL;
   
   /* Global special exits. */
   while ( global_special_exits )
     destroy_exit( global_special_exits );
   
   destroying_world = 0;
   
   world = world_last = NULL;
   areas = areas_last = NULL;
   current_room = NULL;
   current_area = NULL;
   
   mode = NONE;
}


/* Free -everything- up, and prepare to be destroyed. */
void i_mapper_module_unload( )
{
   del_timer( "queue_reset_timer" );
   
   destroy_map( );
}


/* This is what the hash table is for. */
ROOM_DATA *get_room( int vnum )
{
   ROOM_DATA *room;
   
   for ( room = hash_world[vnum % MAX_HASH]; room; room = room->next_in_hash )
     if ( room->vnum == vnum )
       return room;
   
   return NULL;
}



void queue_reset( TIMER *self )
{
   if ( !q_top )
     return;
   
   q_top = 0;
   
   clientff( "\r\n" C_R "[" C_D "Mapper's command queue cleared." C_R "]"
	     C_0 "\r\n" );
   show_prompt( );
}


void add_queue( int value )
{
   int i;
   
   for ( i = q_top; i > 0; i-- )
     queue[i] = queue[i-1];
   q_top++;
   queue[0] = value;
   
   add_timer( "queue_reset_timer", 10, queue_reset, 0, 0, 0 );
}


void add_queue_top( int value )
{
   queue[q_top] = value;
   q_top++;
   
   add_timer( "queue_reset_timer", 10, queue_reset, 0, 0, 0 );
}


int must_swim( ROOM_DATA *src, ROOM_DATA *dest )
{
   if ( !src || !dest )
     return 0;
   
   if ( disable_swimming )
     return 0;
   
   if ( ( src->underwater || src->room_type->underwater ) &&
	pear_defence )
     return 0;
   
   if ( src->room_type->must_swim || dest->room_type->must_swim )
     return 1;
   
   return 0;
}


int room_cmp( char *room, char *smallr )
{
   char buf[256];
   
   if ( !strcmp( room, smallr ) )
     return 0;
   
   strcpy( buf, smallr );
   strcat( buf, " (path)" );
   
   if ( !strcmp( room, buf ) )
     return 0;
   
   strcpy( buf, smallr );
   strcat( buf, " (road)" );
   
   if ( !strcmp( room, buf ) )
     return 0;
   
   return 1;
}


void set_reverse( ROOM_DATA *source, int dir, ROOM_DATA *destination )
{
   if ( ( !source && destination->more_reverse_exits[dir] ) ||
	( source && destination->reverse_exits[dir] &&
	  destination->reverse_exits[dir] != source ) )
     {
	ROOM_DATA *room;
	
	destination->reverse_exits[dir] = NULL;
	destination->more_reverse_exits[dir] = 0;
	
	for ( room = world; room; room = room->next_in_world )
	  {
	     if ( room->exits[dir] != destination )
	       continue;
	     
	     if ( destination->reverse_exits[dir] )
	       destination->more_reverse_exits[dir]++;
	     else
	       destination->reverse_exits[dir] = room;
	  }
     }
   else
     destination->reverse_exits[dir] = source;
}


void parse_room( char *line, char *raw_line )
{
   ROOM_DATA *new_room;
   int created = 0;
   int similar = 0;
   int q, i;
   
   DEBUG( "parse_room" );
   
   /* Room title check. */
   
   if ( !parsing_room )
     {
	if ( !strncmp( raw_line, room_color, room_color_len ) )
	  {
	     parsing_room = 1;
	  }
	else if ( !strncmp( line, "In the trees above", 18 ) &&
		  !strncmp( raw_line + 18, room_color, room_color_len ) )
	  {
	     line += 19;
	     similar = 1;
	     parsing_room = 1;
	  }
	else if ( !strncmp( line, "Flying above", 12 ) &&
		  !strncmp( raw_line + 12, room_color, room_color_len ) )
	  {
	     line += 13;
	     similar = 1;
	     parsing_room = 1;
	  }
     }
   
   if ( !parsing_room )
     return;
   
//   debugf( "Room!" );
   
   /* Stage one, room name. */
   if ( parsing_room == 1 )
     {
	if ( line[0] == '\0' )
	  {
	     /* Room name is after a newline. (ex: swim look) */
	     /* Leave parsing_room as 1. */
	     return;
	  }
	
	/* Return if it looks too weird to be a room title. */
	if ( ( line[0] < 'A' || line[0] > 'Z' ) &&
	     ( line[0] < 'a' || line[0] > 'a' ) )
	  {
	     parsing_room = 0;
	     return;
	  }
	
	/* Capturing mode. */
	if ( mode == CREATING && capture_special_exit )
	  {
	     EXIT_DATA *spexit;
	     
	     if ( !current_room )
	       {
		  clientf( C_R " (Current Room is NULL! Capturing disabled)" C_0 );
		  capture_special_exit = 0;
		  mode = FOLLOWING;
		  return;
	       }
	     
	     if ( !cse_message[0] )
	       {
		  clientf( C_R " (No message found! Capturing disabled)" C_0 );
		  capture_special_exit = 0;
		  mode = FOLLOWING;
		  return;
	       }
	     
	     if ( capture_special_exit > 0 )
	       {
		  new_room = get_room( capture_special_exit );
		  
		  if ( !new_room )
		    {
		       clientf( C_R " (Destination room is now NULL! Capturing disabled)" C_0 );
		       capture_special_exit = 0;
		       mode = FOLLOWING;
		       return;
		    }
		  
		  /* Make sure the destination matches. */
		  if ( strcmp( line, new_room->name ) &&
		    !( similar && !room_cmp( new_room->name, line ) ) )
		    {
		       clientf( C_R " (Destination does not match! Capturing disabled)" C_0 );
		       capture_special_exit = 0;
		       mode = FOLLOWING;
		       return;
		    }
	       }
	     else
	       {
		  new_room = create_room( -1 );
		  new_room->name = strdup( line );
	       }
	     
	     clientff( C_R " (" C_W "sp:" C_G "%d" C_R ")" C_0, new_room->vnum );
	     clientff( C_R "\r\n[Special exit created.]" );
	     spexit = create_exit( current_room );
	     spexit->to = new_room;
	     
	     clientff( C_R "\r\nCommand: '" C_W "%s" C_R "'" C_0,
		       cse_command[0] ? cse_command : "null" );
	     if ( cse_command[0] )
	       spexit->command = strdup( cse_command );
	     
	     clientff( C_R "\r\nMessage: '" C_W "%s" C_R "'" C_0,
		       cse_message );
	     spexit->message = strdup( cse_message );
	     
	     current_room = new_room;
	     current_area = current_room->area;
	     capture_special_exit = 0;
	     return;
	  }
	
	
	/* Following or Mapping mode. */
	if ( mode == FOLLOWING || mode == CREATING )
	  {
	     /* Queue empty? */
	     if ( !q_top )
	       {
		  parsing_room = 0;
		  return;
	       }
	     
	     q = queue[q_top-1];
	     q_top--;
	     
	     if ( !q_top )
	       del_timer( "queue_reset_timer" );
	     
	     if ( !current_room )
	       {
		  clientf( " (Current room is null, while mapping is not!)" );
		  mode = NONE;
		  parsing_room = 0;
		  return;
	       }
	     
	     /* Not just a 'look'? */
	     if ( q > 0 )
	       {
		  if ( mode == FOLLOWING )
		    {
		       if ( current_room->exits[q] )
			 {
			    current_room = current_room->exits[q];
			    current_area = current_room->area;
			 }
		       else
			 {
			    /* We moved into a strange exit, while not creating. */
			    clientf( C_R " (" C_G "lost" C_R ")" C_0 );
			    current_room = NULL;
			    mode = GET_UNLOST;
			 }
		    }
		  
		  else if ( mode == CREATING )
		    {
		       if ( current_room->exits[q] )
			 {
			    /* Just follow around. */
			    new_room = current_room->exits[q];
			 }
		       else
			 {
			    /* Create or link an exit. */
			    if ( link_next_to )
			      {
				 new_room = link_next_to;
				 link_next_to = NULL;
				 clientf( C_R " (" C_G "linked" C_R ")" C_0 );
			      }
			    else
			      {
				 new_room = create_room( -1 );
				 clientf( C_R " (" C_G "created" C_R ")" C_0 );
				 created = 1;
			      }
			    
			    current_room->exits[q] = new_room;
			    set_reverse( current_room, q, new_room );
//			    new_room->reverse_exits[q] = current_room;
			    if ( !unidirectional_exit )
			      {
				 if ( !new_room->exits[reverse_exit[q]] )
				   new_room->exits[reverse_exit[q]] = current_room;
				 else
				   {
				      current_room->exits[q] = NULL;
//				      current_room->reverse_exits[q] = NULL;
				      clientf( C_R " (" C_G "unlinked: reverse error" C_R ")" );
				   }
			      }
			    else
			      unidirectional_exit = 0;
			 }
		       
		       /* Change the length, if asked so. */
		       if ( set_length_to )
			 {
			    if ( set_length_to == -1 )
			      set_length_to = 0;
			    
			    current_room->exit_length[q] = set_length_to;
			    if ( new_room->exits[reverse_exit[q]] == current_room )
			      new_room->exit_length[reverse_exit[q]] = set_length_to;
			    clientf( C_R " (" C_G "l set" C_R ")" C_0 );
			    
			    set_length_to = 0;
			 }
		       
		       /* Stop mapping from here on? */
		       if ( switch_exit_stops_mapping )
			 {
			    i = current_room->exit_stops_mapping[q];
			    
			    i = !i;
			    
			    current_room->exit_stops_mapping[q] = i;
			    if ( new_room->exits[reverse_exit[q]] == current_room )
			      new_room->exit_stops_mapping[reverse_exit[q]] = i;
			    
			    if ( i )
			      clientf( C_R " (" C_G "s set" C_R ")" C_0 );
			    else
			      clientf( C_R " (" C_G "s unset" C_R ")" C_0 );
			    
			    switch_exit_stops_mapping = 0;
			 }
		       
		       /* Show this somewhere else on the map, instead? */
		       if ( use_direction_instead )
			 {
			    if ( use_direction_instead == -1 )
			      use_direction_instead = 0;
			    
			    current_room->use_exit_instead[q] = use_direction_instead;
			    if ( new_room->exits[reverse_exit[q]] == current_room )
			      new_room->use_exit_instead[reverse_exit[q]] = reverse_exit[use_direction_instead];
			    if ( use_direction_instead )
			      clientf( C_R " (" C_G "u set" C_R ")" C_0 );
			    else
			      clientf( C_R " (" C_G "u unset" C_R ")" C_0 );
			    
			    use_direction_instead = 0;
			 }
		       
		       current_room = new_room;
		       current_area = new_room->area;
		    }
	       }
	     
	     if ( mode == CREATING )
	       {
		  if ( !current_room->name || strcmp( line, current_room->name ) )
		    {
		       if ( !created )
			 clientf( C_R " (" C_G "updated" C_R ")" C_0 );
		       
		       if ( current_room->name )
			 free( current_room->name );
		       current_room->name = strdup( line );
		    }
	       }
	     else if ( mode == FOLLOWING )
	       {
		  if ( strcmp( line, current_room->name ) &&
		       !( similar && !room_cmp( current_room->name, line ) ) )
		    {
		       /* Didn't enter where we expected to? */
		       clientf( C_R " (" C_G "lost" C_R ")" C_0 );
		       current_room = NULL;
		       mode = GET_UNLOST;
		    }
	       }
	  }
	
	if ( mode == GET_UNLOST )
	  {
	     ROOM_DATA *r, *found = NULL;
	     int more = 0;
	     
	     for ( r = world; r; r = r->next_in_world )
	       {
		  if ( !strcmp( line, r->name ) )
		    {
		       if ( !found )
			 found = r;
		       else
			 {
			    more = 1;
			    break;
			 }
		    }
	       }
	     
	     if ( found )
	       {
		  current_room = found;
		  current_area = found->area;
		  mode = FOLLOWING;
		  
		  get_unlost_exits = more;
	       }
	  }
	
	if ( mode == CREATING )
	  {
	     clientff( C_R " (" C_G "%d" C_R ")" C_0, current_room->vnum );
	  }
	else if ( mode == FOLLOWING )
	  {
	     if ( !disable_areaname )
	       clientff( C_R " (" C_g "%s" C_R "%s)" C_0,
			 current_room->area->name, get_unlost_exits ? "?" : "" );
	  }
	
	/* We're ready to move. */
	if ( mode == FOLLOWING && auto_walk == 1 )
	  auto_walk = 2;
	
	parsing_room = 2;
	
	return;
     }
   
   /* Stage two, look for start of exits list. */
   if ( parsing_room == 2 )
     {
	if ( current_room )
	  {
	     char *p;
	     
	     if ( ( p = strstr( line, "You see exits leading" ) ) )
	       {
		  line = p + 22;
		  
		  if ( mode == CREATING )
		    for ( i = 1; dir_name[i]; i++ )
		      current_room->detected_exits[i] = 0;
		  else if ( get_unlost_exits )
		    for ( i = 1; dir_name[i]; i++ )
		      get_unlost_detected_exits[i] = 0;
		  
		  parsing_room = 3;
		  
	       }
	     
	     if ( !strncmp( line, "You see a single exit leading ", 30 ) )
	       {
		  line = line + 30;
		  
		  if ( mode == CREATING )
		    for ( i = 1; dir_name[i]; i++ )
		      current_room->detected_exits[i] = 0;
		  else if ( get_unlost_exits )
		    for ( i = 1; dir_name[i]; i++ )
		      get_unlost_detected_exits[i] = 0;
		  
		  parsing_room = 3;
	       }
	  }
     }
   
   /* Check for exits. If it got here, then current_room is checked. */
   if ( parsing_room == 3 )
     {
	char word[128];
	char *p, *w;
	
	p = line;
	
	while( *p )
	  {
	     w = word;
	     /* Extract a word. */
	     while( *p && *p != ',' && *p != ' ' && *p != '.' )
	       *w++ = *p++;
	     *w = 0;
	     
	     /* Skip spaces and weird stuff. */
	     while( *p && ( *p == ',' || *p == ' ' ) )
	       p++;
	     
	     for ( i = 1; dir_name[i]; i++ )
	       {
		  if ( !strcmp( word, dir_name[i] ) )
		    {
		       if ( mode == CREATING )
			 current_room->detected_exits[i] = 1;
		       else if ( get_unlost_exits )
			 get_unlost_detected_exits[i] = 1;
		       break;
		    }
	       }
	     
	     if ( *p == '.' )
	       {
		  parsing_room = 0;
		  break;
	       }
	  }
     }
}



void check_area( char *name )
{
   AREA_DATA *area;
   
   if ( !strcmp( name, current_room->area->name ) )
     return;
   
   /* Area doesn't match. What shall we do? */
   
   if ( mode == FOLLOWING && auto_bump )
     {
	clientf( C_R " (" C_W "doesn't match!" C_R ")" C_0 );
	auto_bump = 0;
     }
   
   if ( mode != CREATING || !current_room )
     return;
   
   /* Check if the user wants to update an existing one. */
   
   if ( update_area_from_survey || !strcmp( current_room->area->name, "New area" ) )
     {
	update_area_from_survey = 0;
	
	for ( area = areas; area; area = area->next )
	  if ( !strcmp( area->name, name ) )
	    {
	       clientf( C_R " (" C_W "warning: duplicate!" C_R ")" C_0 );
	       break;
	    }
	
	area = current_room->area;
	if ( area->name )
	  free( area->name );
	area->name = strdup( name );
	clientf( C_R " (" C_G "updated" C_R ")" C_0 );
	
	return;
     }
     
   /* First, check if the area exists. If it does, switch it. */
   
   for ( area = areas; area; area = area->next )
     if ( !strcmp( area->name, name ) )
       {
	  unlink_from_area( current_room );
	  link_to_area( current_room, area );
	  current_room->area = area;
	  current_area = area;
	  clientf( C_R " (" C_G "switched!" C_R ")" C_0 );
	  
	  return;
       }
   
   /* If not, create one. */
   
   area = create_area( );
   area->name = strdup( name );
   unlink_from_area( current_room );
   link_to_area( current_room, area );
   current_room->area = area;
   current_area = area;
   clientf( C_R " (" C_G "new area created" C_R ")" C_0 );
}



void parse_survey( char *line )
{
   DEBUG( "parse_survey" );
   
   if ( ( mode == CREATING || auto_bump ) && current_room )
     {
	if ( ( !strncmp( line, "You discern that you are standing in ", 37 ) &&
	       ( line += 37 ) ) ||
	     ( !strncmp( line, "You stand within ", 17 ) &&
	       ( line += 17 ) ) ||
	     ( !strncmp( line, "You are standing in ", 20 ) &&
	       ( line += 20 ) ) )
	  {
	     /* Might not have a "the" in there, rare cases though. */
	     if ( !strncmp( line, "the ", 4 ) )
	       line += 4;
	     
	     check_area( line );
	  }
	else if ( !strncmp( line, "Your environment conforms to that of ", 37 ) )
	  {
	     ROOM_TYPE *type;
	     
	     line = line + 37;
	     
	     for ( type = room_types; type; type = type->next )
	       if ( !strncmp( line, type->name, strlen( type->name ) ) )
		 break;
	     
	     /* Lusternia - lowercase letters. */
	     if ( !type )
	       {
		  char buf[256];
		  
		  for ( type = room_types; type; type = type->next )
		    {
		       strcpy( buf, type->name );
		       
		       buf[0] -= ( 'A' - 'a' );
		       
		       if ( !strncmp( line, buf, strlen( buf ) ) )
			 break;
		    }
	       }
	     
	     if ( type != current_room->room_type )
	       {
		  if ( mode == CREATING )
		    {
		       current_room->room_type = type;
		       
		       if ( type )
			 clientf( C_R " (" C_G "updated" C_R ")" C_0 );
		       else
			 {
			    current_room->room_type = null_room_type;
			    clientf( C_R " (" C_D "type removed" C_R ")" C_0 );
			 }
		    }
		  else if ( auto_bump )
		    {
		       clientf( C_R " (" C_W "doesn't match!" C_R ")" C_0 );
		       auto_bump = 0;
		    }
	       }
	  }
	else if ( !strcmp( line, "You cannot glean any information about your surroundings." ) )
	  {
	     ROOM_TYPE *type;
	     
	     for ( type = room_types; type; type = type->next )
	       {
		  if ( !strcmp( "Undefined", type->name ) )
		    {
		       if ( type != current_room->room_type )
			 {
			    if ( mode == CREATING )
			      {
				 current_room->room_type = type;
				 
				 clientf( C_R " (" C_G "updated" C_R ")" C_0 );
			      }
			    else if ( auto_bump )
			      {
				 clientf( C_R " (" C_W "doesn't match!" C_R ")" C_0 );
				 auto_bump = 0;
			      }
			 }
		       
		       break;
		    }
	       }
	  }
     }
}

   


int can_dash( ROOM_DATA *room )
{
   int dir;
   
   if ( !dash_command )
     return 0;
   
   dir = room->pf_direction;
   
   /* First of all, check if we have more than one room. */
   if ( dir <= 0 || !room->exits[dir] ||
	room->exits[dir]->pf_direction != dir )
     return 0;
   
   /* Now check if we have a clear way, till the end. */
   while( room )
     {
	if ( room->pf_direction != dir && room->exits[dir] )
	  return 0;
	if ( room->exits[dir] && room->exits[dir]->room_type->must_swim )
	  return 0;
	
	room = room->exits[dir];
     }
   
   return 1;
}



void go_next( )
{
   EXIT_DATA *spexit;
   char buf[256];
   
   if ( !current_room )
     {
	clientfr( "No current_room." );
	auto_walk = 0;
     }
   else if ( !current_room->pf_parent )
     {
	clientff( C_R "[" C_G "Done." C_R "]\r\n" C_0 );
	show_prompt( );
	auto_walk = 0;
     }
   else
     {
	if ( current_room->pf_direction != -1 )
	  {
	     if ( door_closed )
	       {
		  sprintf( buf, "open %s", dir_name[current_room->pf_direction] );
		  clientfr( buf );
		  sprintf( buf, "open door %s\r\n", dir_small_name[current_room->pf_direction] );
		  send_to_server( buf );
	       }
	     else if ( door_locked )
	       {
		  clientff( C_R "\r\n[Locked room " C_W "%s" C_R " of v" C_W "%d" C_R ". Speedwalking disabled.]\r\n" C_0,
			    dir_name[current_room->pf_direction], current_room->vnum );
		  show_prompt( );
		  door_locked = 0;
		  door_closed = 0;
		  auto_walk = 0;
		  return;
//		  sprintf( buf, "unlock %s", dir_name[current_room->pf_direction] );
//		  clientfr( buf );
//		  sprintf( buf, "unlock door %s\r\n", dir_small_name[current_room->pf_direction] );
//		  send_to_server( buf );
	       }
	     else
	       {
		  if ( !gag_prompt( -1 ) )
		    clientfr( dir_name[current_room->pf_direction] );
		  
		  if ( must_swim( current_room, current_room->pf_parent ) )
		    send_to_server( "swim " );
		  
		  if ( !must_swim( current_room, current_room->pf_parent ) &&
		       dash_command && can_dash( current_room ) )
		    send_to_server( dash_command );
		  else
		    {
		       if ( mode == FOLLOWING || mode == CREATING )
			 add_queue( current_room->pf_direction );
		    }
		  
		  send_to_server( dir_small_name[current_room->pf_direction] );
		  send_to_server( "\r\n" );
	       }
	  }
	else
	  {
	     for ( spexit = current_room->special_exits; spexit; spexit = spexit->next )
	       {
		  if ( spexit->to == current_room->pf_parent &&
		       spexit->command )
		    {
		       clientfr( spexit->command );
		       send_to_server( spexit->command );
		       send_to_server( "\r\n" );
		       break;
		    }
	       }
	  }
	
	auto_walk = 1;
     }
}



void add_exit_char( char *var, int dir )
{
   const char dir_chars[] = 
     { ' ', '|', '/', '-', '\\', '|', '/', '-', '\\' };
   
   if ( ( *var == '|' && dir_chars[dir] == '-' ) ||
	( *var == '-' && dir_chars[dir] == '|' ) )
     *var = '+';
   else if ( ( *var == '/' && dir_chars[dir] == '\\' ) ||
	     ( *var == '\\' && dir_chars[dir] == '/' ) )
     *var = 'X';
   else
     *var = dir_chars[dir];
}



void fill_map( ROOM_DATA *room, AREA_DATA *area, int x, int y )
{
   static int xi[] = { 2, 0, 1, 1, 1, 0, -1, -1, -1, 2, 2, 2, 2, 2 };
   static int yi[] = { 2, -1, -1, 0, 1, 1, 1, 0, -1, 2, 2, 2, 2, 2 };
   int i;
   
   if ( room->area != area )
     return;
   
   if ( room->mapped )
     return;
   
   if ( x < 0 || x >= MAP_X+1 || y < 0 || y >= MAP_Y+1 )
     return;
   
   room->mapped = 1;
   
   if ( map[x][y] )
     return;
   
   map[x][y] = room;
   
   for ( i = 1; dir_name[i]; i++ )
     {
	if ( !room->exits[i] )
	  continue;
	
	if ( room->exit_stops_mapping[i] )
	  continue;
	
	if ( room->exit_length[i] > 0 && !room->use_exit_instead[i] )
	  {
	     int j;
	     int real_x, real_y;
	     
	     
	     for( j = 1; j <= room->exit_length[i]; j++ )
	       {
		  real_x = x + ( xi[i] * j );
		  real_y = y + ( yi[i] * j );
		  
		  if ( real_x < 0 || real_x >= MAP_X+1 ||
		       real_y < 0 || real_y >= MAP_Y+1 )
		    continue;
		  
		  add_exit_char( &extra_dir_map[real_x][real_y], i );
	       }
	  }
	
	if ( room->use_exit_instead[i] )
	  {
	     int ex = room->use_exit_instead[i];
	     fill_map( room->exits[i], area,
		       x + ( xi[ex] * ( 1 + room->exit_length[i] ) ),
		       y + ( yi[ex] * ( 1 + room->exit_length[i] ) ) );
	  }
	else if ( xi[i] != 2 )
	  fill_map( room->exits[i], area,
		    x + ( xi[i] * ( 1 + room->exit_length[i] ) ),
		    y + ( yi[i] * ( 1 + room->exit_length[i] ) ) );
     }
}




/* One little nice thing. */
int has_exit( ROOM_DATA *room, int direction )
{
   if ( room )
     {
	if ( room->exits[direction] )
	  {
	     if ( room->exit_stops_mapping[direction] )
	       return 4;
	     else if ( room->area != room->exits[direction]->area )
	       return 3;
	     else if ( room->pf_highlight && room->pf_direction == direction )
	       return 5;
	     else
	       return 1;
	  }
	else if ( room->detected_exits[direction] )
	  return 2;
     }
   
   return 0;
}


/*
 * 
 * [ ]- [ ]- [ ]
 *  | \  | / 
 * 
 * [ ]- [*]- [ ]
 *  | /  | \ 
 * 
 * [ ]  [ ]  [ ]
 * 
 */

void set_exit( short *ex, ROOM_DATA *room, int dir )
{
   /* Careful. These are |=, not != */
   
   if ( room->exits[dir] )
     {
	*ex |= EXIT_NORMAL;
	
	if ( room->exits[dir]->area != room->area )
	  *ex |= EXIT_OTHER_AREA;
     }
   else
     if ( room->detected_exits[dir] )
       *ex |= EXIT_UNLINKED;
   
   if ( room->exit_stops_mapping[dir] )
     *ex |= EXIT_STOPPING;
   
   if ( room->pf_highlight && room->pf_parent == room->exits[dir] )
     *ex |= EXIT_PATH;
}



void set_all_exits( ROOM_DATA *room, int x, int y )
{
   /* East, southeast, south. (current element) */
   set_exit( &map_new[x][y].e, room, EX_E );
   set_exit( &map_new[x][y].se, room, EX_SE );
   set_exit( &map_new[x][y].s, room, EX_S );
   
   /* Northwest. (northwest element) */
   if ( x && y )
     {
	set_exit( &map_new[x-1][y-1].se, room, EX_NW );
     }
   
   /* North, northeast. (north element) */
   if ( y )
     {
	set_exit( &map_new[x][y-1].s, room, EX_N );
	set_exit( &map_new[x][y-1].se_rev, room, EX_NE );
     }
   
   /* West, southwest. (west element) */
   if ( x )
     {
	set_exit( &map_new[x-1][y].e, room, EX_W );
	set_exit( &map_new[x-1][y].se_rev, room, EX_SW );
     }
   
   /* In, out. */
   map_new[x][y].in_out |= ( room->exits[EX_IN] || room->exits[EX_OUT] ) ? 1 : 0;
}



/* Remake of fill_map. */
void fill_map_new( ROOM_DATA *room, int x, int y )
{
   const int xi[] = { 2, 0, 1, 1, 1, 0, -1, -1, -1, 2, 2, 2, 2, 2 };
   const int yi[] = { 2, -1, -1, 0, 1, 1, 1, 0, -1, 2, 2, 2, 2, 2 };
   int i;
   
   if ( map_new[x][y].room || room->mapped )
     return;
   
   if ( x < 0 || x >= MAP_X || y < 0 || y >= MAP_Y )
     return;
   
   room->mapped = 1;
   map_new[x][y].room = room;
   map_new[x][y].color = room->room_type->color;
   
   set_all_exits( room, x, y );
   
   for ( i = 1; dir_name[i]; i++ )
     {
	if ( !room->exits[i] || room->exit_stops_mapping[i] ||
	     room->exits[i]->area != room->area )
	  continue;
	
	/* Normal exit. */
	if ( !room->use_exit_instead[i] && !room->exit_length[i] )
	  {
	     if ( xi[i] != 2 && yi[i] != 2 )
	       fill_map_new( room->exits[i], x + xi[i], y + yi[i] );
	     continue;
	  }
	
	/* Exit changed. */
	  {
	     int new_i, new_x, new_y;
	     
	     new_i = room->use_exit_instead[i] ? room->use_exit_instead[i] : i;
	     
	     if ( xi[new_i] != 2 && yi[new_i] != 2 )
	       {
		  int j;
		  
		  new_x = x + xi[new_i] * ( 1 + room->exit_length[i] );
		  new_y = y + yi[new_i] * ( 1 + room->exit_length[i] );
		  
		  fill_map_new( room->exits[i], new_x, new_y );
		  
		  for( j = 1; j <= room->exit_length[i]; j++ )
		    {
		       new_x = x + ( xi[new_i] * j );
		       new_y = y + ( yi[new_i] * j );
		       
		       if ( new_x < 0 || new_x >= MAP_X ||
			    new_y < 0 || new_y >= MAP_Y )
			 break;
		       
		       /* 
			* This exit will be displayed instead of the center
			* of a room.
			*/
		       
		       if ( i == EX_N || i == EX_S )
			 map_new[new_x][new_y].extra_s = 1;
		       
		       else if ( i == EX_E || i == EX_W )
			 map_new[new_x][new_y].extra_e = 1;
		       
		       else if ( i == EX_SE || i == EX_NW )
			 map_new[new_x][new_y].extra_se = 1;
		       
		       else if ( i == EX_NE || i == EX_SW )
			 map_new[new_x][new_y].extra_se_rev = 1;
		    }
	       }
	  }
     }
}


/* Total remake of show_map. */
/* All calls to strcat/sprintf have been replaced with byte by byte
 * processing. The result has been a dramatical increase in speed. */

void show_map_new( ROOM_DATA *room )
{
   ROOM_DATA *r;
   char map_buf[4096], buf[64], *p, *s, *s2;
   char vnum_buf[64];
   int x, y, len, len2, loop;
   
   DEBUG( "show_map_new" );
   
   if ( !room )
     return;
   
   get_timer( );
//   debugf( "--map new--" );
   
   for ( r = room->area->rooms; r; r = r->next_in_area )
     r->mapped = 0;
   
   /* Clear it up. */
   for ( x = 0; x < MAP_X; x++ )
     for ( y = 0; y < MAP_Y; y++ )
       memset( &map_new[x][y], 0, sizeof( MAP_ELEMENT ) );
   
   /* Pathfinder - Go around and set which rooms to highlight. */
   for ( r = room->area->rooms; r; r = r->next_in_area )
     r->pf_highlight = 0;
   
   if ( room->pf_parent )
     {
	for ( r = room, loop = 0; r->pf_parent && loop < 100; r = r->pf_parent, loop++ )
	  r->pf_highlight = 1;
     }
   
//   debugf( "1: %d", get_timer( ) );
   
   /* From the current *room, place all other rooms on the map. */
   fill_map_new( room, MAP_X / 2, MAP_Y / 2 );
   
//   debugf( "2: %d", get_timer( ) );
   
   
   /* Build it up. */
   
   map_buf[0] = 0;
   p = map_buf;
   
   /* Upper banner. */
   
   sprintf( vnum_buf, "v%d", room->vnum );
   
   s = "/--" C_C;
   while( *s )
     *(p++) = *(s++);
   s = room->area->name, len = 0;
   while( *s )
     *(p++) = *(s++), len++;
   s = C_0;
   while( *s )
     *(p++) = *(s++);
   len2 = MAP_X * 4 - 7 - strlen( vnum_buf );
   for ( x = len; x < len2; x++ )
     *(p++) = '-';
   s = C_C;
   while( *s )
     *(p++) = *(s++);
   s = vnum_buf;
   while( *s )
     *(p++) = *(s++);
   s = C_0 "--\\\r\n";
   while( *s )
     *(p++) = *(s++);
   *p = 0;
   
   /* The map, row by row. */
   for ( y = 0; y < MAP_Y; y++ )
     {
	/* (1) [o]- */
	/* (2)  | \ */
	
	/* (1) */
	
	if ( y )
	  {
	     *(p++) = ' ';
	     
	     for ( x = 0; x < MAP_X; x++ )
	       {
		  if ( x )
		    {
		       if ( map_new[x][y].room )
			 {
			    if ( !map_new[x][y].room->underwater )
			      s = map_new[x][y].color;
			    else
			      s = C_C;
			    
			    while( *s )
			      *(p++) = *(s++);
			    *(p++) = '[';
			    
			    if ( map_new[x][y].room == current_room )
			      s = C_B "*";
			    else if ( map_new[x][y].in_out )
			      s = C_r "o";
			    else
			      s = " ";
			    while( *s )
			      *(p++) = *(s++);
			    
			    if ( !map_new[x][y].room->underwater )
			      s = map_new[x][y].color;
			    else
			      s = C_C;
			    
			    while( *s )
			      *(p++) = *(s++);
			    s = "]" C_0;
			    while( *s )
			      *(p++) = *(s++);
			 }
		       else
			 {
			    *(p++) = ' ';
			    
			    if ( map_new[x][y].extra_e && map_new[x][y].extra_s )
			      *(p++) = '+';
			    else if ( map_new[x][y].extra_se && map_new[x][y].extra_se_rev )
			      *(p++) = 'X';
			    else if ( map_new[x][y].extra_e )
			      *(p++) = '-';
			    else if ( map_new[x][y].extra_s )
			      *(p++) = '|';
			    else if ( map_new[x][y].extra_se )
			      *(p++) = '\\';
			    else if ( map_new[x][y].extra_se_rev )
			      *(p++) = '/';
			    else
			      *(p++) = ' ';
			    
			    *(p++) = ' ';
			 }
		    }
		  
		  /* Exit color. */
		  if ( map_new[x][y].e & EXIT_UNLINKED )
		    s = C_D, s2 = C_0;
		  else if ( map_new[x][y].e & EXIT_OTHER_AREA )
		    s = C_W, s2 = C_0;
		  else if ( map_new[x][y].e & EXIT_STOPPING )
		    s = C_R, s2 = C_0;
		  else if ( map_new[x][y].e & EXIT_PATH )
		    s = C_B, s2 = C_0;
		  else
		    s = "", s2 = "";
		  while( *s )
		    *(p++) = *(s++);
		  
		  *(p++) = map_new[x][y].e ? '-' : ' ';
		  
		  while( *s2 )
		    *(p++) = *(s2++);
	       }
	     
	     *(p++) = '\r';
	     *(p++) = '\n';
	  }
	
	/* (2) */
	
	for ( x = 0; x < MAP_X; x++ )
	  {
	     *(p++) = ' ';
	     if ( x )
	       {
		  /* Exit color. */
		  if ( map_new[x][y].s & EXIT_UNLINKED )
		    s = C_D, s2 = C_0;
		  else if ( map_new[x][y].s & EXIT_OTHER_AREA )
		    s = C_W, s2 = C_0;
		  else if ( map_new[x][y].s & EXIT_STOPPING )
		    s = C_R, s2 = C_0;
		  else if ( map_new[x][y].s & EXIT_PATH )
		    s = C_B, s2 = C_0;
		  else
		    s = "", s2 = "";
		  while( *s )
		    *(p++) = *(s++);
		  
		  *(p++) = map_new[x][y].s ? '|' : ' ';
		  
		  while( *s2 )
		    *(p++) = *(s2++);

		  *(p++) = ' ';
	       }
	     
	     /* Exit color. */
	     if ( ( map_new[x][y].se | map_new[x][y].se_rev ) & EXIT_UNLINKED )
	       s = C_D, s2 = C_0;
	     else if ( ( map_new[x][y].se | map_new[x][y].se_rev ) & EXIT_OTHER_AREA )
	       s = C_W, s2 = C_0;
	     else if ( ( map_new[x][y].se | map_new[x][y].se_rev ) & EXIT_STOPPING )
	       s = C_R, s2 = C_0;
	     else if ( ( map_new[x][y].se | map_new[x][y].se_rev ) & EXIT_PATH )
	       s = C_B, s2 = C_0;
	     else
	       s = "", s2 = "";
	     while( *s )
	       *(p++) = *(s++);
	     
	     *(p++) = ( map_new[x][y].se ?
			( map_new[x][y].se_rev ? 'X' : '\\' ) :
			map_new[x][y].se_rev ? '/' : ' ' );
	     
	     while( *s2 )
	       *(p++) = *(s2++);
	  }
	
	*(p++) = '\r';
	*(p++) = '\n';
     }
   
   /* Lower banner. */
   s = "\\--";
   while( *s )
     *(p++) = *(s++);
   sprintf( buf, "Time: %d usec", get_timer( ) );
   s = buf, len = 0;
   while( *s )
     *(p++) = *(s++), len++;
   for ( x = len; x < MAP_X * 4 - 5; x++ )
     *(p++) = '-';
   s = "/\r\n";
   while( *s )
     *(p++) = *(s++);
   *p = 0;
   
//   debugf( "3: %d", get_timer( ) );
   
   /* Show it away. */
   clientf( map_buf );
}


/* One big ugly thing. */
/* Somebody find a cure to this stupidity! It's way too ugly! */
void show_map( ROOM_DATA *room )
{
   ROOM_DATA *troom;
   char big_buffer[4096];
   char dir_map[MAP_X*2+4][MAP_Y*2+4];
   char *dir_color_map[MAP_X*2+4][MAP_Y*2+4];
   char buf[128], room_buf[128];
   int x, y;
   char *dir_colors[] = { "", "", C_D, C_W, C_R, C_B };
   int d1, d2;
   int loop;
   
   DEBUG( "show_map" );
   
   get_timer( );
   get_timer( );
//   debugf( "--map--" );
   
   big_buffer[0] = 0;
   
//   debugf( "1: %d", get_timer( ) );
   
   for ( troom = room->area->rooms; troom; troom = troom->next_in_area )
     troom->mapped = 0;
   
   for ( x = 0; x < MAP_X+1; x++ )
     for ( y = 0; y < MAP_Y+1; y++ )
       map[x][y] = NULL, extra_dir_map[x][y] = ' ';
   
   for ( x = 0; x < MAP_X*2+2; x++ )
     for ( y = 0; y < MAP_Y*2+2; y++ )
       dir_map[x][y] = ' ', dir_color_map[x][y] = "";
   
   /* Pathfinder - Go around and set which rooms to highlight. */
   for ( troom = room->area->rooms; troom; troom = troom->next_in_area )
     troom->pf_highlight = 0;
   
   if ( room->pf_parent )
     {
	for ( troom = room, loop = 0; troom && loop < 100; troom = troom->pf_parent, loop++ )
	  troom->pf_highlight = 1;
     }
   
//   debugf( "2: %d", get_timer( ) );
   
   /* Build rooms. */
   fill_map( room, room->area, (MAP_X+1) / 2, (MAP_Y+1) / 2 );
   
//   debugf( "3: %d", get_timer( ) );
   
   /* Build directions. */
   for ( y = 0; y < MAP_Y+1-1; y++ )
     {
	for ( x = 0; x < MAP_X+1-1; x++ )
	  {
	     /* [ ]-  (01) */
	     /*  | X  (11) */
	     
	     /* d1 and d2 should be: 1-normal, 2-only detected, 3-another area, 4-stopped mapping, 5-pathfinder. */
	     dir_map[x*2][y*2] = ' ';
	     
	     /* South. */
	     d1 = map[x][y] ? has_exit( map[x][y], EX_S ) : 0;
	     d2 = map[x][y+1] ? has_exit( map[x][y+1], EX_N ) : 0;
	     
	     if ( d1 || d2 )
	       dir_map[x*2][y*2+1] = '|';
	     
	     dir_color_map[x*2][y*2+1] = dir_colors[d1>d2 ? d1 : d2];
	     
	     /* East. */
	     d1 = map[x][y] ? has_exit( map[x][y], EX_E ) : 0;
	     d2 = map[x+1][y] ? has_exit( map[x+1][y], EX_W ) : 0;
	     
	     if ( d1 || d2 )
	       dir_map[x*2+1][y*2] = '-';
	     
	     dir_color_map[x*2+1][y*2] = dir_colors[d1>d2 ? d1 : d2];
	     
	     /* Southeast. (\) */
	     d1 = map[x][y] ? has_exit( map[x][y], EX_SE ) : 0;
	     d2 = map[x+1][y+1] ? has_exit( map[x+1][y+1], EX_NW ) : 0;
	     
	     if ( d1 || d2 )
	       dir_map[x*2+1][y*2+1] = '\\';
	     
	     dir_color_map[x*2+1][y*2+1] = dir_colors[d1>d2 ? d1 : d2];
	     
	     /* Southeast. (/) */
	     d1 = map[x+1][y] ? has_exit( map[x+1][y], EX_SW ) : 0;
	     d2 = map[x][y+1] ? has_exit( map[x][y+1], EX_NE ) : 0;
	     
	     if ( d1 || d2 )
	       {
		  if ( dir_map[x*2+1][y*2+1] == ' ' )
		    dir_map[x*2+1][y*2+1] = '/';
		  else
		    dir_map[x*2+1][y*2+1] = 'X';
	       }
	     
	     if ( dir_color_map[x*2+1][y*2+1] != C_D &&
		  dir_colors[d1>d2 ? d1 : d2][0] != 0 )
	       dir_color_map[x*2+1][y*2+1] = dir_colors[d1>d2 ? d1 : d2];
	  }
     }
   
//   debugf( "4: %d", get_timer( ) );
   
   /* And now combine them. */
   strcat( big_buffer, "/--" "\33[1;36m" );
   strcat( big_buffer, room->area->name );
   strcat( big_buffer, C_0 );
   for ( x = 2+strlen( room->area->name ); x < (MAP_X-2+1)*4+1; x++ )
     strcat( big_buffer, "-" );
   strcat( big_buffer, "\\\r\n" );
   
//   debugf( "5: %d", get_timer( ) );
   
   for ( y = 0; y < MAP_Y-1+1; y++ )
     {
	if ( y > 0 )
	  {
	     /* [o]- */
	     
	     /* *[*o*]*- */
	     
	     strcat( big_buffer, " " );
	     for ( x = 0; x < MAP_X-1+1; x++ )
	       {
		  if ( x == 0 )
		    sprintf( buf, "%s%c%s", dir_color_map[x*2+1][y*2], dir_map[x*2+1][y*2],
			     dir_color_map[x*2+1][y*2][0] != 0 ? C_0 : "" );
		  else
		    {
		       char *color;
		       char *center;
		       char str[2];
		       
		       if ( !map[x][y] )
			 color = "";
		       else if ( map[x][y]->underwater )
			 color = "\33[1;36m";
		       else
			 color = map[x][y]->room_type->color;
		       
		       if ( !map[x][y] )
			 {
			    /* Ugh! */
			    sprintf( str, "%c", extra_dir_map[x][y] );
			    center = str;
			 }
		       else if ( map[x][y] == current_room )
			 center = C_B "*";
		       else if ( map[x][y]->exits[EX_IN] || map[x][y]->exits[EX_OUT] )
			 center = C_r "o";
		       else
			 center = " ";
		       
		       sprintf( room_buf, "%s%s%s%s%s",
				color, map[x][y] ? "[" : " ",
				center,
				color, map[x][y] ? "]" C_0 : " " );
		       
		       sprintf( buf, "%s%s%c%s", room_buf,
				dir_color_map[x*2+1][y*2], 
				dir_map[x*2+1][y*2],
				dir_color_map[x*2+1][y*2][0] != 0 ? C_0 : "" );
		    }
		  strcat( big_buffer, buf );
	       }
	     strcat( big_buffer, "\r\n" );
	  }
	strcat( big_buffer, " " );
	
	for ( x = 0; x < MAP_X-1+1; x++ )
	  {
	     /*  | X */
	     
	     if ( x == 0 )
	       sprintf( buf, "%s%c%s", dir_color_map[x*2+1][y*2+1], dir_map[x*2+1][y*2+1],
			dir_color_map[x*2+1][y*2+1][0] != 0 ? C_0 : "" );
	     else
	       sprintf( buf, " %s%c%s %s%c%s", dir_color_map[x*2][y*2+1], dir_map[x*2][y*2+1],
			dir_color_map[x*2][y*2+1][0] != 0 ? C_0 : "", dir_color_map[x*2+1][y*2+1],
			dir_map[x*2+1][y*2+1], dir_color_map[x*2+1][y*2+1][0] != 0 ? C_0 : "" );
	     strcat( big_buffer, buf );
	  }
	strcat( big_buffer, "\r\n" );
     }
   
//   debugf( "6: %d", get_timer( ) );
   
   strcat( big_buffer, "\\--" );
   sprintf( buf, "Time: %d usec", get_timer( ) );
   strcat( big_buffer, buf );
   for ( x = 2 + strlen( buf ); x < (MAP_X-2+1)*4+1; x++ )
     strcat( big_buffer, "-" );
   strcat( big_buffer, "/\r\n" );
   
   /* Now let's send this big monster we've just created... */
   clientf( big_buffer );
}



char *get_color( char *name )
{
   int i;
   
   for ( i = 0; color_names[i].name; i++ )
     if ( !strcmp( name, color_names[i].name ) )
       return color_names[i].code;
   
   return NULL;
}


ROOM_TYPE *add_room_type( char *name, char *color, int c_in, int c_out, int m_swim, int underw )
{
   ROOM_TYPE *type;
   
   for ( type = room_types; type; type = type->next )
     if ( !strcmp( name, type->name ) )
       break;
   
   /* New? Create one more. */
   if ( !type )
     {
	ROOM_TYPE *t;
	
	type = calloc( 1, sizeof( ROOM_TYPE ) );
	type->name = strdup( name );
	type->next = NULL;
	
	if ( !room_types )
	  room_types = type;
	else
	  {
	     for ( t = room_types; t->next; t = t->next );
	     t->next = type;
	  }
     }
   
   if ( !type->color || strcmp( type->color, color ) )
     type->color = color;
   
   type->cost_in = c_in;
   type->cost_out = c_out;
   type->must_swim = m_swim;
   type->underwater = underw;
   
   return type;
}



int save_settings( char *file )
{
   ROOM_DATA *r;
   FILE *fl;
   int i;
   
   fl = fopen( file, "w" );
   
   if ( !fl )
     return 1;
   
   fprintf( fl, "# File generated by IMapper.\r\n" );
   fprintf( fl, "# Manual changes that are not loaded will be lost on the next rewrite.\r\n\r\n" );
   
   for ( i = 0; color_names[i].name; i++ )
     {
	if ( !strcmp( color_names[i].title_code, room_color ) )
	  {
	     fprintf( fl, "Title-Color %s\r\n", color_names[i].name );
	     break;
	  }
     }
   
   fprintf( fl, "Disable-Swimming %s\r\n", disable_swimming ? "yes" : "no" );
   fprintf( fl, "Disable-WhoList %s\r\n", disable_wholist ? "yes" : "no" );
   fprintf( fl, "Disable-Alertness %s\r\n", disable_alertness ? "yes" : "no" );
   fprintf( fl, "Disable-Locating %s\r\n", disable_locating ? "yes" : "no" );
   fprintf( fl, "Disable-AreaName %s\r\n", disable_areaname ? "yes" : "no" );
   
   /* Save all landmarks. */
   
   fprintf( fl, "\r\n# Landmarks.\r\n\r\n" );
   for ( r = world; r; r = r->next_in_world )
     {
	if ( r->landmark )
	  fprintf( fl, "# %s (%s)\r\nLand-Mark %d\r\n",
		   r->name, r->area->name, r->vnum );
     }
   
   fclose( fl );
   return 0;
}


int load_settings( char *file )
{
   FILE *fl;
   char line[1024];
   char option[256];
   char value[1024];
   char *p;
   int i;
   
   fl = fopen( file, "r" );
   
   if ( !fl )
     return 1;
   
   while( 1 )
     {
	fgets( line, 1024, fl );
	
	if ( feof( fl ) )
	  break;
	
	/* Strip newline. */
	  {
	     p = line;
	     while ( *p != '\n' && *p != '\r' && *p )
	       p++;
	     
	     *p = 0;
	  }
	
	if ( line[0] == '#' || line[0] == 0 || line[0] == ' ' )
	  continue;
	
	p = get_string( line, option, 256 );
	
	get_string( p, value, 1024 );
	
	if ( !strcmp( option, "Title-Color" ) ||
	     !strcmp( option, "Title-Colour" ) )
	  {
	     for ( i = 0; color_names[i].name; i++ )
	       {
		  if ( !strcmp( value, color_names[i].name ) )
		    {
		       room_color = color_names[i].title_code;
		       room_color_len = color_names[i].length;
		       break;
		    }
	       }
	  }
	
	else if ( !strcmp( option, "Disable-Swimming" ) )
	  {
	     if ( !strcmp( value, "yes" ) )
	       disable_swimming = 1;
	     else if ( !strcmp( value, "no" ) )
	       disable_swimming = 0;
	     else
	       debugf( "Parse error in file '%s', expected 'yes' or 'no', got '%s' instead.", file, value );
	  }
	
	else if ( !strcmp( option, "Disable-WhoList" ) )
	  {
	     if ( !strcmp( value, "yes" ) )
	       disable_wholist = 1;
	     else if ( !strcmp( value, "no" ) )
	       disable_wholist = 0;
	     else
	       debugf( "Parse error in file '%s', expected 'yes' or 'no', got '%s' instead.", file, value );
	  }
	
	else if ( !strcmp( option, "Disable-Alertness" ) )
	  {
	     if ( !strcmp( value, "yes" ) )
	       disable_alertness = 1;
	     else if ( !strcmp( value, "no" ) )
	       disable_alertness = 0;
	     else
	       debugf( "Parse error in file '%s', expected 'yes' or 'no', got '%s' instead.", file, value );
	  }
	
	else if ( !strcmp( option, "Disable-Locating" ) )
	  {
	     if ( !strcmp( value, "yes" ) )
	       disable_locating = 1;
	     else if ( !strcmp( value, "no" ) )
	       disable_locating = 0;
	     else
	       debugf( "Parse error in file '%s', expected 'yes' or 'no', got '%s' instead.", file, value );
	  }
	
	else if ( !strcmp( option, "Disable-AreaName" ) )
	  {
	     if ( !strcmp( value, "yes" ) )
	       disable_areaname = 1;
	     else if ( !strcmp( value, "no" ) )
	       disable_areaname = 0;
	     else
	       debugf( "Parse error in file '%s', expected 'yes' or 'no', got '%s' instead.", file, value );
	  }
	
	else if ( !strcmp( option, "Land-Mark" ) )
	  {
	     ROOM_DATA *room;
	     int vnum = atoi( value );
	     
	     if ( !vnum )
	       debugf( "Parse error in file '%s', expected a landmark vnum, got '%s' instead.", file, value );
	     else
	       {
		  room = get_room( vnum );
		  if ( !room )
		    debugf( "Warning! Unable to landmark room %d, it doesn't exist!", vnum );
		  else
		    room->landmark = 1;
	       }
	  }
	
	else
	  debugf( "Parse error in file '%s', unknown option '%s'.", file, option );
     }
   
   fclose( fl );
   return 0;
}



void save_map( char *file )
{
   AREA_DATA *area;
   ROOM_DATA *room;
   EXIT_DATA *spexit;
   ROOM_TYPE *type;
   FILE *fl;
   char buf[256];
   int i, j, count = 0;
   
   DEBUG( "save_map" );
   
   fl = fopen( file, "w" );
   
   if ( !fl )
     {
	sprintf( buf, "Can't open '%s' to save map: %s.", file,
		 strerror( errno ) );
	clientfr( buf );
	return;
     }
   
   fprintf( fl, "MAPFILE\n\n" );
   
   fprintf( fl, "\n\nROOM-TYPES\n\n"
	    "# \"Name\", Color Name, Cost In, Cost Out, Swim, Underwater\n" );
   
   for ( type = room_types; type; type = type->next )
     {
	/* Find the color's name. */
	
	for ( j = 0; color_names[j].name; j++ )
	  if ( !strcmp( color_names[j].code, type->color ) )
	    break;
	
	fprintf( fl, "T: \"%s\" %s %d %d %s %s\n", type->name,
		 color_names[j].name ? color_names[j].name : "normal",
		 type->cost_in, type->cost_out,
		 type->must_swim ? "yes" : "no",
		 type->underwater ? "yes" : "no" );
     }
   
   fprintf( fl, "\n\nMESSAGES\n\n" );
   
   for ( spexit = global_special_exits; spexit; spexit = spexit->next )
     {
	fprintf( fl, "GSE: %d \"%s\" \"%s\"\n", spexit->vnum,
		 spexit->command ? spexit->command : "", spexit->message );
     }
   
   fprintf( fl, "\n\n" );
   
   for ( area = areas; area; area = area->next, count++ )
     {
	fprintf( fl, "AREA\n" );
	fprintf( fl, "Name: %s\n", area->name );
	if ( area->disabled )
	  fprintf( fl, "Disabled\n" );
	fprintf( fl, "\n" );
	
	for ( room = area->rooms; room; room = room->next_in_area )
	  {
	     fprintf( fl, "ROOM v%d\n", room->vnum );
	     fprintf( fl, "Name: %s\n", room->name );
	     if ( room->room_type != null_room_type )
	       fprintf( fl, "Type: %s\n", room->room_type->name );
	     if ( room->underwater )
	       fprintf( fl, "Underwater\n" );
	     for ( i = 1; dir_name[i]; i++ )
	       {
		  if ( room->exits[i] )
		    {
		       fprintf( fl, "E: %s %d\n", dir_name[i],
				room->exits[i]->vnum );
		       if ( room->exit_length[i] )
			 fprintf( fl, "EL: %s %d\n", dir_name[i],
				  room->exit_length[i] );
		       if ( room->exit_stops_mapping[i] )
			 fprintf( fl, "ES: %s %d\n", dir_name[i],
				  room->exit_stops_mapping[i] );
		       if ( room->use_exit_instead[i] &&
			    room->use_exit_instead[i] != i )
			 fprintf( fl, "UE: %s %s\n", dir_name[i],
				  dir_name[room->use_exit_instead[i]] );
		    }
		  else if ( room->detected_exits[i] )
		    {
		       if ( room->locked_exits[i] )
			 fprintf( fl, "DEL: %s\n", dir_name[i] );
		       else
			 fprintf( fl, "DE: %s\n", dir_name[i] );
		    }
	       }
	     
	     for ( spexit = room->special_exits; spexit; spexit = spexit->next )
	       {
		  fprintf( fl, "SPE: %d %d \"%s\" \"%s\"\n",
			   spexit->vnum, spexit->alias,
			   spexit->command ? spexit->command : "",
			   spexit->message ? spexit->message : "" );
	       }
	     
	     fprintf( fl, "\n" );
	  }
	fprintf( fl, "\n\n" );
     }
   
   fprintf( fl, "EOF\n" );
   
   sprintf( buf, "%d areas saved.", count );
   clientfr( buf );
   
   fclose( fl );
}



int check_map( )
{
   DEBUG( "check_map" );
   
   
   return 0;
}



void remake_vnums( )
{
   AREA_DATA *area;
   ROOM_DATA *room;
   char buf[128];
   int i = 1;
   
   DEBUG( "remake_vnums" );
   
   for ( area = areas; area; area = area->next )
     {
	for ( room = area->rooms; room; room = room->next_in_area )
	  {
	     room->vnum = i++;
	  }
     }
   
   sprintf( buf, "Room vnums remade. Last is %d.", i-1 );
   clientfr( buf );
}



void convert_vnum_exits( )
{
   ROOM_DATA *room;
   EXIT_DATA *spexit;
   int i;
   
   DEBUG( "convert_vnum_exits" );
   
   for ( room = world; room; room = room->next_in_world )
     {
	/* Normal exits. */
	for ( i = 1; dir_name[i]; i++ )
	  {
	     if ( room->vnum_exits[i] )
	       {
		  room->exits[i] = get_room( room->vnum_exits[i] );
		  if ( !room->exits[i] )
		    {
		       debugf( "Can't link room %d (%s) to %d.",
			       room->vnum, room->name, room->vnum_exits[i] );
		       continue;
		    }
		  set_reverse( room, i, room->exits[i] );
//		  room->exits[i]->reverse_exits[i] = room;
		  room->vnum_exits[i] = 0;
	       }
	  }
	
	/* Special exits. */
	for ( spexit = room->special_exits; spexit; spexit = spexit->next )
	  {
	     if ( spexit->vnum < 0 )
	       spexit->to = NULL;
	     else
	       {
		  if ( !( spexit->to = get_room( spexit->vnum ) ) )
		       {
			  debugf( "Can't link room %d (%s) to %d. (special exit)",
				  room->vnum, room->name, spexit->vnum );
		       }
		  else
		    spexit->to->pointed_by = 1;
	       }
	  }
     }
   
   for ( spexit = global_special_exits; spexit; spexit = spexit->next )
     {
	if ( spexit->vnum < 0 )
	  spexit->to = NULL;
	else
	  {
	     if ( !( spexit->to = get_room( spexit->vnum ) ) )
	       {
		  debugf( "Can't link global special exit '%s' to %d.",
			  spexit->command, spexit->vnum );
	       }
	  }
     }
}



int load_map( char *file )
{
   AREA_DATA *area = NULL;
   ROOM_DATA *room = NULL;
   ROOM_TYPE *type;
   FILE *fl;
   char buf[256];
   char line[256];
   char *eof;
   int section = 0;
   int nr = 0, i;
   
   DEBUG( "load_map" );
   
   fl = fopen( file, "r" );
   
   if ( !fl )
     {
	sprintf( buf, "Can't open '%s' to load map: %s.", file,
		 strerror( errno ) );
	clientfr( buf );
	return 1;
     }
   
   while( 1 )
     {
	eof = fgets( line, 256, fl );
	if ( feof( fl ) )
	  break;
	
	if ( !strncmp( line, "EOF", 3 ) )
	  break;
	
	nr++;
	
	if ( !line[0] )
	  continue;
	
	if ( !strncmp( line, "AREA", 4 ) )
	  {
	     area = create_area( );
	     current_area = area;
	     section = 1;
	  }
	
	else if ( !strncmp( line, "ROOM v", 6 ) )
	  {
	     int vnum;
	     
	     if ( !area )
	       {
		  debugf( "Room out of area. Line %d.", nr );
		  return 1;
	       }
	     
	     vnum = atoi( line + 6 );
	     room = create_room( vnum );
	     if ( vnum > last_vnum )
	       last_vnum = vnum;
	     section = 2;
	  }
	
	else if ( !strncmp( line, "ROOM-TYPES", 10 ) )
	  {
	     section = 3;
	  }
	
	else if ( !strncmp( line, "MESSAGES", 8 ) )
	  {
	     section = 4;
	  }
	
	/* Strip newline. */
	  {
	     char *p = line;
	     while ( *p != '\n' && *p != '\r' && *p )
	       p++;
	     
	     *p = 0;
	  }
	
	if ( !strncmp( line, "Name: ", 6 ) )
	  {
	     if ( section == 1 )
	       {
		  if ( area->name )
		    {
		       debugf( "Double name, on area. Line %d.", nr );
		       return 1;
		    }
		  
		  area->name = strdup( line + 6 );
	       }
	     else if ( section == 2 )
	       {
		  if ( room->name )
		    {
		       debugf( "Double name, on room. Line %d.", nr );
		       return 1;
		    }
		  
		  room->name = strdup( line + 6 );
	       }
	  }
	
	else if ( !strncmp( line, "Type: ", 6 ) )
	  {
	     /* Type: Road */
	     for ( type = room_types; type; type = type->next )
	       if ( !strncmp( line+6, type->name, strlen( type->name ) ) )
		 {
		    room->room_type = type;
		    break;
		 }
	  }
	
	else if ( !strncmp( line, "E: ", 3 ) )
	  {
	     /* E: northeast 14 */
	     char buf[256];
	     int dir;
	     
	     if ( section != 2 )
	       {
		  debugf( "Misplaced exit line. Line %d.", nr );
		  return 1;
	       }
	     
	     sscanf( line, "E: %s %d", buf, &dir );
	     
	     if ( !dir )
	       {
		  debugf( "Syntax error at exit, line %d.", nr );
		  return 1;
	       }
	     
	     for ( i = 1; dir_name[i]; i++ )
	       {
		  if ( !strcmp( buf, dir_name[i] ) )
		    {
		       room->vnum_exits[i] = dir;
		       break;
		    }
	       }
	  }
	
	else if ( !strncmp( line, "DE: ", 4 ) )
	  {
	     /* DE: northeast */
	     char buf[256];
	     
	     sscanf( line, "DE: %s", buf );
	     
	     for ( i = 1; dir_name[i]; i++ )
	       {
		  if ( !strcmp( buf, dir_name[i] ) )
		    {
		       room->detected_exits[i] = 1;
		       break;
		    }
	       }
	  }
	
	else if ( !strncmp( line, "DEL: ", 4 ) )
	  {
	     /* DEL: northeast */
	     char buf[256];
	     
	     sscanf( line, "DEL: %s", buf );
	     
	     for ( i = 1; dir_name[i]; i++ )
	       {
		  if ( !strcmp( buf, dir_name[i] ) )
		    {
		       room->detected_exits[i] = 1;
		       room->locked_exits[i] = 1;
		       break;
		    }
	       }
	  }
	
	else if ( !strncmp( line, "EL: ", 4 ) )
	  {
	     /* EL: northeast 1 */
	     char buf[256];
	     int l;
	     
	     sscanf( line, "EL: %s %d", buf, &l );
	     
	     if ( l < 1 )
	       {
		  debugf( "Syntax error at exit length, line %d.", nr );
		  return 1;
	       }
	     
	     for ( i = 1; dir_name[i]; i++ )
	       {
		  if ( !strcmp( buf, dir_name[i] ) )
		    {
		       room->exit_length[i] = l;
		       break;
		    }
	       }
	  }
	
	else if ( !strncmp( line, "ES: ", 4 ) )
	  {
	     /* ES: northeast 1 */
	     char buf[256];
	     int s;
	     
	     sscanf( line, "ES: %s %d", buf, &s );
	     
	     for ( i = 1; dir_name[i]; i++ )
	       {
		  if ( !strcmp( buf, dir_name[i] ) )
		    {
		       room->exit_stops_mapping[i] = s;
		       break;
		    }
	       }
	  }
	else if ( !strncmp( line, "UE: ", 4 ) )
	  {
	     /* UE in east */
	     char buf1[256], buf2[256];
	     int j;
	     
	     sscanf( line, "UE: %s %s", buf1, buf2 );
	     
	     for ( i = 1; dir_name[i]; i++ )
	       {
		  if ( !strcmp( dir_name[i], buf1 ) )
		    {
		       for ( j = 1; dir_name[j]; j++ )
			 {
			    if ( !strcmp( dir_name[j], buf2 ) )
			      {
				 room->use_exit_instead[i] = j;
				 break;
			      }
			 }
		       break;
		    }
	       }
	  }
	
	else if ( !strncmp( line, "SPE: ", 5 ) )
	  {
	     EXIT_DATA *spexit;
	     
	     char vnum[1024];
	     char alias[1024];
	     char cmd[1024];
	     char msg[1024];
	     char *p = line + 5;
	     
	     /* SPE: -1 0 "pull lever" "You pull a lever, and fall below the ground." */
	     
	     p = get_string( p, vnum, 1024 );
	     p = get_string( p, alias, 1024 );
	     p = get_string( p, cmd, 1024 );
	     p = get_string( p, msg, 1024 );
	     
	     spexit = create_exit( room );
	     
	     if ( vnum[0] == '-' )
	       spexit->vnum = -1;
	     else
	       spexit->vnum = atoi( vnum );
	     
	     if ( alias[0] )
	       spexit->alias = atoi( alias );
	     else
	       spexit->alias = 0;
	     
	     if ( cmd[0] )
	       spexit->command = strdup( cmd );
	     else
	       spexit->command = NULL;
	     
	     if ( msg[0] )
	       spexit->message = strdup( msg );
	     else
	       spexit->message = NULL;
	  }
	
	else if ( !strncmp( line, "Disabled", 8 ) )
	  {
	     if ( section != 1 )
	       {
		  debugf( "Wrong section of a Disabled statement." );
		  return 1;
	       }
	     
	     area->disabled = 1;
	  }
	
	else if ( !strncmp( line, "Underwater", 10 ) )
	  {
	     if ( section != 2 )
	       {
		  debugf( "Wrong section of an Underwater statement." );
		  return 1;
	       }
	     
	     room->underwater = 1;
	  }
	
	/* Deprecated. Here only for backwards compatibility. */
	else if ( !strncmp( line, "Marked", 6 ) )
	  {
	     if ( section != 2 )
	       {
		  debugf( "Wrong section of a Marked statement." );
		  return 1;
	       }
	     
	     room->landmark = 1;
	  }
	
	else if ( !strncmp( line, "T: ", 3 ) )
	  {
	     char name[512];
	     char color_name[512];
	     char *color;
	     char buf[512];
	     char *p = line + 3;
	     int cost_in, cost_out;
	     int must_swim;
	     int underwater;
	     
	     if ( section != 3 )
	       {
		  debugf( "Misplaced room type. Line %d.", nr );
		  return 1;
	       }
	     
	     p = get_string( p, name, 512 );
	     p = get_string( p, color_name, 512 );
	     p = get_string( p, buf, 512 );
	     cost_in = atoi( buf );
	     p = get_string( p, buf, 512 );
	     cost_out = atoi( buf );
	     p = get_string( p, buf, 512 );
	     must_swim = buf[0] == 'y' ? 1 : 0;
	     p = get_string( p, buf, 512 );
	     underwater = buf[0] == 'y' ? 1 : 0;
	     
	     if ( !name[0] || !color_name[0] || !cost_in || !cost_out )
	       {
		  debugf( "Buggy Room-Type, line %d.", nr );
		  return 1;
	       }
	     
	     color = get_color( color_name );
	     
	     if ( !color )
	       {
		  debugf( "Unknown color name, line %d.", nr );
		  return 1;
	       }
	     
	     add_room_type( name, color, cost_in, cost_out, must_swim, underwater );
	  }
	
	else if ( !strncmp( line, "GSE: ", 5 ) )
	  {
	     EXIT_DATA *spexit;
	     
	     char vnum[1024];
	     char cmd[1024];
	     char msg[1024];
	     char *p = line + 5;
	     
	     if ( section != 4 )
	       {
		  debugf( "Misplaced global special exit. Line %d.", nr );
		  return 1;
	       }
	     
	     /* GSE: -1 "brazier" "You feel a moment of disorientation as you are summoned." */
	     
	     p = get_string( p, vnum, 1024 );
	     p = get_string( p, cmd, 1024 );
	     p = get_string( p, msg, 1024 );
	     
	     spexit = create_exit( NULL );
	     
	     if ( vnum[0] == '-' )
	       spexit->vnum = -1;
	     else
	       spexit->vnum = atoi( vnum );
	     
	     if ( cmd[0] )
	       spexit->command = strdup( cmd );
	     else
	       spexit->command = NULL;
	     
	     if ( msg[0] )
	       spexit->message = strdup( msg );
	     else
	       spexit->message = NULL;
	  }
     }
   
   fclose( fl );
   
   return 0;
}


void init_openlist( ROOM_DATA *room )
{
   ROOM_DATA *r;
   
   if ( !room )
     {
	pf_current_openlist = NULL;
	return;
     }
   
   /* Make sure it's not already there. */
   for ( r = pf_current_openlist; r; r = r->next_in_pfco )
     if ( r == room )
       return;
   
   /* Add it. */
   if ( pf_current_openlist )
     {
	room->next_in_pfco = pf_current_openlist;
	pf_current_openlist = room;
     }
   else
     {
	pf_current_openlist = room;
	room->next_in_pfco = NULL;
     }
}


int get_cost( ROOM_DATA *src, ROOM_DATA *dest )
{
   return src->room_type->cost_out + dest->room_type->cost_in;
}




void add_openlist( ROOM_DATA *src, ROOM_DATA *dest, int dir )
{
   ROOM_DATA *troom;
   
//   DEBUG( "add_openlist" );
   
   /* To add, or not to add... now that is the question. */
   if ( dest->pf_cost == 0 ||
	src->pf_cost + get_cost( dest, src ) < dest->pf_cost )
     {
	int found = 0;
	
	/* Link to the new openlist. */
	/* But first make sure it's not there already. */
	for ( troom = pf_new_openlist; troom; troom = troom->next_in_pfno )
	  if ( troom == dest )
	    {
	       found = 1;
	       break;
	    }
	
	if ( !found )
	  {
	     dest->next_in_pfno = pf_new_openlist;
	     pf_new_openlist = dest;
	  }
	
	dest->pf_cost = src->pf_cost + get_cost( dest, src );
	dest->pf_parent = src;
	dest->pf_direction = dir;
     }
}




void path_finder( )
{
   ROOM_DATA *room, *r;
   EXIT_DATA *spexit;
   int i;
   
   DEBUG( "path_finder" );
   
   if ( !world )
     return;
   
   /* Okay, get ready by clearing it up. */
   for ( room = world; room; room = room->next_in_world )
     {
	room->pf_parent = NULL;
	room->pf_cost = 0;
	room->pf_direction = 0;
     }
   for ( room = pf_current_openlist; room; room = room->next_in_pfco )
     room->pf_cost = 1;
   
   pf_new_openlist = NULL;
   
   /* Were we sent NULL? Then it was just for the above, to clear it up. */
   
   if ( !pf_current_openlist )
     return;
   
   while ( pf_current_openlist )
     {
	/*** This was check_openlist( ) ***/
	
	room = pf_current_openlist;
	
	/* Walk around and create a new openlist. */
	while( room )
	  {
	     for ( i = 1; dir_name[i]; i++ )
	       {
		  /* Normal exits. */
		  if ( room->exits[i] && !room->exits[i]->area->disabled &&
		       room->exits[i]->exits[reverse_exit[i]] == room )
		    add_openlist( room, room->exits[i], reverse_exit[i] );
		  /* Unidirectional exits leading here. */
		  else if ( room->reverse_exits[i] && !room->exits[i] &&
			    room->reverse_exits[i]->exits[i] == room )
		    {
		       if ( !room->more_reverse_exits[i] )
			 add_openlist( room, room->reverse_exits[i], i );
		       else
			 {
			    /* More rooms lead here. Search for them. */
			    for ( r = world; r; r = r->next_in_world )
			      if ( r->exits[i] == room )
				add_openlist( room, r, i );
			 }
		    }
	       }
	     
	     /*	for ( i = 1; dir_name[i]; i++ )
	      {
	      if ( room->reverse_exits[i] && !room->exits[i] &&
	      room->reverse_exits[i]->exits[i] == room )
	      {
	      add_openlist( room, room->reverse_exits[i], i );
	      }
	      }*/
	     
	     /* Special exits, version 2. (ugly remix) */
	     if ( room->pointed_by )
	       {
		  for ( r = world; r; r = r->next_in_world )
		    {
		       int found = 0;
		       
		       for ( spexit = r->special_exits; spexit; spexit = spexit->next )
			 {
			    if ( spexit->to == room )
			      {
				 found = 1;
				 break;
			      }
			 }
		       
		       if ( found )
			 add_openlist( room, r, -1 );
		    }
	       }
	     
	     room = room->next_in_pfco;
	  }
	
	/* Replace the current openlist with the new openlist. */
	pf_current_openlist = pf_new_openlist;
	for ( room = pf_new_openlist; room; room = room->next_in_pfno )
	  room->next_in_pfco = room->next_in_pfno;
	pf_new_openlist = NULL;
     }
}


void show_path( ROOM_DATA *current )
{
   ROOM_DATA *room;
   EXIT_DATA *spexit;
   char buf[4096];
   int nr = 0;
   int wrap = 7; /* strlen( "[Path: " ) */
   
   DEBUG( "show_path" );
   
   if ( !current->pf_parent )
     {
	clientfr( "Can't find a path from here." );
	return;
     }
   
   sprintf( buf, C_R "[Path: " C_G );
   for ( room = current; room && room->pf_parent && nr < 100; room = room->pf_parent )
     {
	if ( wrap > 70 )
	  {
	     strcat( buf, C_R ",\r\n" C_G );
	     wrap = 0;
	  }
	else if ( nr++ )
	  {
	     strcat( buf, C_R ", " C_G );
	     wrap += 2;
	  }
	
	/* We also have special exits. */
	if ( room->pf_direction != -1 )
	  {
	     if ( must_swim( room, room->pf_parent ) )
	       strcat( buf, C_B );
	     
	     strcat( buf, dir_small_name[room->pf_direction] );
	     wrap += strlen( dir_small_name[room->pf_direction] );
	  }
	else
	  {
	     /* Find the special exit, leading to the parent. */
	     for ( spexit = room->special_exits; spexit; spexit = spexit->next )
	       {
		  if ( spexit->to == room->pf_parent )
		    {
		       if ( spexit->command )
			 {
			    strcat( buf, spexit->command );
			    wrap += strlen( spexit->command );
			 }
		       else
			 {
			    strcat( buf, "?" );
			    wrap++;
			 }
		       
		       break;
		    }
	       }
	  }
     }
   strcat( buf, C_R ".]\r\n" C_0 );
   clientf( buf );
}



void i_mapper_module_init_data( )
{
   DEBUG( "i_mapper_init_data" );
   
   null_room_type = add_room_type( "Unknown", get_color( "red" ), 1, 1, 0, 0 );
   add_room_type( "Undefined", get_color( "bright-red" ), 1, 1, 0, 0 );
   
   destroy_map( );
   get_timer( );
   if ( load_map( map_file ) )
     {
	destroy_map( );
	mode = NONE;
	return;
     }
   else
     {
	convert_vnum_exits( );
	check_map( );
	debugf( "Map loaded. (%d microseconds)", get_timer( ) );
     }
   
   load_settings( "settings.mapper.txt" );
   
   mode = GET_UNLOST;
}



/* A case insensitive version of strstr. */

#define LOW_CASE( a ) ( (a) >= 'A' && (a) <= 'Z' ? (a) - 'A' + 'a' : (a) )

int case_strstr( char *haystack, char *needle )
{
   char *h = haystack, *n = needle;
   
   while ( *h )
     {
	if ( LOW_CASE( *h ) == LOW_CASE( *(needle) ) )
	  {
	     n = needle;
	     
	     while( 1 )
	       {
		  if ( !(*n) )
		    return 1;
		  
		  if ( LOW_CASE( *h ) != LOW_CASE( *n ) )
		    break;
		  h++, n++;
	       }
	  }
	
	h++;
     }
   
   return 0;
}


void locate_room( char *name, int area )
{
   ROOM_DATA *room, *found = NULL;
   char buf[256];
   char buf2[256];
   int more = 0;
   
   DEBUG( "locate_room" );
   
   strcpy( buf, name );
   
   for ( room = world; room; room = room->next_in_world )
     {
	if ( !strcmp( buf, room->name ) )
	  {
	     if ( !found )
	       found = room;
	     else
	       {
		  more = 1;
		  break;
	       }
	  }
     }
   
   /* Search with "(path)" added after it. */
   if ( !found )
     {
	strcpy( buf2, buf );
	strcat( buf, " (road)" );
	strcat( buf2, " (path)" );
	
	for ( room = world; room; room = room->next_in_world )
	  {
	     if ( !strcmp( buf, room->name ) ||
		  !strcmp( buf2, room->name ) )
	       {
		  if ( !found )
		    found = room;
		  else
		    {
		       more = 1;
		       break;
		    }
	       }
	  }
     }
   
   if ( found )
     {
	/* Attempt to find out in which side of the arena it is. */
	if ( locate_arena && !more && found->area )
	  {
	     if ( case_strstr( found->area->name, "north" ) )
	       {
		  if ( case_strstr( found->area->name, "east" ) )
		    clientf( C_D " (" C_R "NE" C_D ")" C_0 );
		  else if ( case_strstr( found->area->name, "west" ) )
		    clientf( C_D " (" C_R "NW" C_D ")" C_0 );
		  else
		    clientf( C_D " (" C_R "N" C_D ")" C_0 );
	       }
	     else if ( case_strstr( found->area->name, "south" ) )
	       {
		  if ( case_strstr( found->area->name, "east" ) )
		    clientf( C_D " (" C_R "SE" C_D ")" C_0 );
		  else if ( case_strstr( found->area->name, "west" ) )
		    clientf( C_D " (" C_R "SW" C_D ")" C_0 );
		  else
		    clientf( C_D " (" C_R "S" C_D ")" C_0 );
	       }
	     else if ( case_strstr( found->area->name, "central" ) ||
		       case_strstr( found->area->name, "palm orchard" ) )
	       clientf( C_D " (" C_R "C" C_D ")" C_0 );
	     else if ( case_strstr( found->area->name, "east" ) )
	       clientf( C_D " (" C_R "E" C_D ")" C_0 );
	     else if ( case_strstr( found->area->name, "west" ) )
	       clientf( C_D " (" C_R "W" C_D ")" C_0 );
	     
	     locate_arena = 0;
	  }
	
	/* Show the vnum, after the line. */
	clientff( " " C_D "(" C_G "%d" C_D "%s)" C_0,
		 found->vnum, more ? C_D "," C_G "..." C_D : "" );
	
	/* A farsight-like thing. */
	if ( area && found->area )
	  {
	     clientff( "\r\nFrom your knowledge, that room is in '%s'",
		       found->area->name );
	  }
     }
}



void parse_msense( char *line )
{
   char *end;
   char buf[256];
   
   DEBUG( "parse_msense" );
   
   if ( strncmp( line, "An image of ", 12 ) )
     return;
   
   line += 12;
   
   if ( !( end = strstr( line, " appears in your mind." ) ) )
     return;
   
   if ( end < line )
     return;
   
   strncpy( buf, line, end - line );
   buf[end-line] = '.';
   buf[end-line+1] = 0;
   
   /* We now have the room name in 'buf'. */
   locate_room( buf, 1 );
}



void parse_window( char *line )
{
   DEBUG( "parse_window" );
   
   if ( strncmp( line, "You see that ", 12 ) )
     return;
   
   line = strstr( line, " is at " );
   
   if ( !line )
     return;
   
   line += 7;
   
   locate_room( line, 1 );
}



void parse_scent( char *line )
{
   if ( !strncmp( line, "You detect traces of scent from ", 32 ) )
     {
	locate_room( line + 32, 1 );
     }
}



void parse_scry( char *line )
{
   char buf[256];
   
   DEBUG( "parse_scry" );
   
   if ( !sense_message )
     {
	if ( !strncmp( line, "You create a pool of water in the air in front of you, and look through it,", 75 ) )
	  sense_message = 1;
     }
   
   if ( sense_message == 1 )
     {
	/* Next line: "sensing Whyte at Antioch Runners tent." */
	/* Skip the first three words. */
	line = get_string( line, buf, 256 );
	line = get_string( line, buf, 256 );
	line = get_string( line, buf, 256 );
	
	locate_room( line, 1 );
     }
}



void parse_ka( char *line )
{
   static char buf[1024];
   static int periods;
   int i;
   
   DEBUG( "parse_ka" );
   
   /* A shimmering image of *name* appears before you. She/he is at *room*.*/
   
   if ( !sense_message )
     {
	if ( !strncmp( line, "A shimmering image of ", 22 ) )
	  {
	     buf[0] = 0;
	     sense_message = 2;
	     periods = 0;
	  }
     }
   
   if ( sense_message == 2 )
     {
	for ( i = 0; line[i]; i++ )
	  if ( line[i] == '.' )
	    periods++;
	
	strcat( buf, line );
	
	if ( periods == 2 )
	  {
	     sense_message = 0;
	     
	     if ( ( line = strstr( buf, " is at " ) ) )
	       locate_room( line + 7, 1 );
	     else if ( ( line = strstr( buf, " is located at " ) ) )
	       locate_room( line + 15, 1 );
	  }
     }
}



void parse_seek( char *line )
{
   static char buf[1024];
   static int periods;
   char *end;
   int i;
   
   DEBUG( "parse_seek" );
   
   /* A shimmering image of *name* appears before you. She/he is at *room*.*/
   
   if ( !sense_message )
     {
	if ( !strncmp( line, "You bid your guardian to seek out the lifeforce of ", 51 ) )
	  {
	     buf[0] = 0;
	     sense_message = 3;
	     periods = 0;
	  }
     }
   
   if ( sense_message == 3 )
     {
	for ( i = 0; line[i]; i++ )
	  if ( line[i] == '.' )
	    periods++;
	
	strcat( buf, line );
	
	if ( periods == 3 )
	  {
	     sense_message = 0;
	     
	     if ( ( line = strstr( buf, " at " ) ) )
	       {
		  line += 4;
		  
		  end = strchr( line, ',' );
		  if ( !end )
		    return;
		  
		  strncpy( buf, line, end - line );
		  buf[end-line] = '.';
		  buf[end-line+1] = 0;
		  
		  locate_room( buf, 1 );
	       }
	  }
     }
}



void parse_shrinesight( char *line )
{
   char *p, *end;
   char buf[256];
   
   DEBUG( "parse_shrinesight" );
   
   if ( !strstr( line, " shrine at '" ) )
     return;
   
   p = strstr( line, "at '" );
   if ( !p )
     return;
   p += 4;
   
   end = strstr( p, "'" );
   if ( !end )
     return;
   
   if ( p >= end )
     return;
   
   strncpy( buf, p, end - p );
   buf[end-p] = 0;
   
   /* We now have the room name in 'buf'. */
   locate_room( buf, 0 );
}



void parse_fullsense( char *line )
{
   char *p;
   char buf[256];
   
   DEBUG( "parse_fullsense" );
   
   if ( strncmp( line, "You sense ", 10 ) )
     return;
   
   p = strstr( line, " at " );
   if ( !p )
     return;
   p += 4;
   
   strcpy( buf, p );
   
   /* We now have the room name in 'buf'. */
   locate_room( buf, 0 );
}



void parse_scout( char *line )
{
   char buf[512], *p;
   
   DEBUG( "parse_scout" );
   
   if ( !strcmp( line, "You sense the following people in the area:" ) ||
	!strcmp( line, "You sense the following creatures in the area:" ) )
     {
	scout_list = 1;
	return;
     }
   
   if ( !scout_list )
     return;
   
   p = strstr( line, " at " );
   if ( !p )
     return;
   
   strcpy( buf, p + 4 );
   strcat( buf, "." );
   
   locate_room( buf, 0 );
}



void parse_pursue( char *line )
{
   char buf[512], buf2[512];
   char *p;
   static int pursue_message;
   
   DEBUG( "parse_pursue" );
   
   if ( !cmp( "You sense that ^ has entered *", line ) )
     {
	pursue_message = 1;
	buf[0] = 0;
     }
   
   if ( !pursue_message )
     return;
   
   /* Add a space to the last line, in case there wasn't one. */
   if ( buf[0] && buf[strlen(buf)-1] != ' ' )
     strcat( buf, " " );
   
   strcat( buf, line );
   
   if ( strstr( buf, "." ) )
     {
	pursue_message = 0;
	
	p = strstr( buf, " has entered " );
	
	if ( !p )
	  return;
	
	strcpy( buf2, p + 13 );
	
	p = buf2 + strlen( buf2 );
	
	while ( p > buf2 )
	  {
	     if ( *p == ',' )
	       {
		  *(p++) = '.';
		  *p = 0;
		  break;
	       }
	     p--;
	  }
	
	locate_room( buf2, 1 );
     }
}



void parse_alertness( char *line )
{
   static char buf[512];
   static int alertness_message;
   
   DEBUG( "parse_alertness" );
   
   if ( disable_alertness )
     return;
   
   if ( !cmp( "Your enhanced senses inform you that *", line ) )
     {
	buf[0] = 0;
	alertness_message = 1;
     }
   
   if ( !alertness_message )
     return;
   
   gag_line( 1 );
   
   /* In case something goes wrong. */
   if ( alertness_message++ > 3 )
     {
	alertness_message = 0;
	return;
     }
   
   if ( buf[0] && buf[strlen(buf)-1] != ' ' )
     strcat( buf, " " );
   
   strcat( buf, line );
   
   if ( strstr( buf, "nearby." ) )
     {
	/* We now have the full message in 'buf'. Parse it. */
	char player[256];
	char room_name[256];
	char *p, *p2;
	int found = 0;
	int i;
	
	alertness_message = 0;
	
	/* Your enhanced senses inform you that <player> has entered <room> nearby. */
	
	p = strstr( buf, " has entered" );
	if ( !p )
	  return;
	*p = 0;
	
	strcpy( player, buf + 37 );
	
	p2 = strstr( p + 1, " nearby" );
	if ( !p2 )
	  return;
	*p2 = 0;
	
	strcpy( room_name, p + 13 );
	strcat( room_name, "." );
	
	alertness_message = 0;
	
	clientff( C_R "[" C_W "%s" C_R " - ", player );
   
	/* Find where that room is. */
	
	if ( current_room )
	  {	
	     if ( !room_cmp( current_room->name, room_name ) )
	       {
		  clientf( C_W "here" );
		  found = 1;
	       }
	     
	     for ( i = 1; dir_name[i]; i++ )
	       {
		  if ( !current_room->exits[i] )
		    continue;
		  
		  if ( !room_cmp( current_room->exits[i]->name, room_name ) )
		    {
		       if ( found )
			 clientf( C_R ", " );
		       
		       clientff( C_B "%s", dir_name[i] );
		       found = 1;
		    }
	       }
	  }
	
	if ( !found )
	  clientff( C_R "%s", room_name );
	
	clientf( C_R "]\r\n" C_0 );
     }
}



void parse_eventstatus( char *line )
{
   char buf[512];
   
   DEBUG( "parse_eventstatus" );
   
   if ( !strncmp( line, "Current event: ", 15 ) )
     {
	evstatus_list = 1;
	return;
     }
   
   if ( !evstatus_list )
     return;
   
   line = get_string( line, buf, 512 );
   
   if ( buf[0] == '-' )
     return;
   
   if ( !strcmp( buf, "Player" ) )
     return;
   
   if ( buf[0] < 'A' || buf[0] > 'Z' )
     {
	evstatus_list = 0;
	return;
     }
   
   /* Skip all numbers and other things, until we get to the room name. */
   while ( 1 )
     {
	if ( !line[0] )
	  return;
	
	if ( line[0] < 'A' || line[0] > 'Z' )
	  {
	     line = get_string( line, buf, 512 );
	     continue;
	  }
	
	break;
     }
   
   strcpy( buf, line );
   strcat( buf, "." );
   
   locate_arena = 1;
   locate_room( buf, 0 );
}



void parse_who( char *line, char *colorless )
{
   static int first_time = 1, len1, len2;
   ROOM_DATA *r, *room;
   AREA_DATA *area;
   char name[64];
   char roomname[256];
   char buf2[1024];
   char color[64];
   int more, len, moreareas;
   
   DEBUG( "parse_who" );
   
   if ( disable_wholist )
     return;
   
   if ( colorless[0] != ' ' )
     return;
   
   if ( strlen( line ) < 70 )
     return;
   
   /* Initialize these two, so we won't do them each time. */
   if ( first_time )
     {
	len1 = 9 + strlen( C_D C_G C_D );
	len2 = 9 + strlen( C_D C_R C_D );
	first_time = 0;
     }
   
   if ( !cmp( "[36m - [37m*", line + 14 ) &&
	!cmp( "[36m - [37m*", line + 59 ) )
     {
	strcpy( color, C_0 + 1 );
	
	get_string( colorless, name, 64 );
	
	strcpy( roomname, line + 70 );
     }
   else if ( !cmp( " - *", colorless + 14 ) &&
	     !cmp( " - *", colorless + 51 ) )
     {
	get_string( line, color, 64 );
	get_string( colorless, name, 64 );
	
	strcpy( roomname, colorless + 54 );
     }
   else
     return;
   
   gag_line( 1 );
   
   /* Search for it. */
   
   room = NULL;
   area = NULL;
   more = 0;
   moreareas = 0;
   len = strlen( roomname );
   
   for ( r = world; r; r = r->next_in_world )
     if ( !strncmp( roomname, r->name, len ) )
       {
	  if ( !room )
	    room = r, area = r->area;
	  else
	    {
	       more++;
	       if ( area != r->area )
		 moreareas++;
	    }
       }
   
   clientff( "\33%s%14s" C_c " - " C_0 "%-24s ", color, name, roomname );
   
   if ( !room )
     {
	clientff( "%9s", "" );
     }
   else
     {
	char buf3[256];
	int len;
	
	if ( !more )
	  {
	     sprintf( buf3, C_D "(" C_G "%d" C_D ")", room->vnum );
	     len = len1;
	  }
	else
	  {
	     sprintf( buf3, C_D "(" C_R "%d rms" C_D ")", more + 1 );
	     len = len2;
	  }
	
	clientff( "%*s", len, buf3 );
     }
   
   clientf( C_c " - " C_0 );
   
   if ( area )
     {
	if ( !moreareas )
	  {
	     strcpy( buf2, area->name );
	     buf2[24] = 0;
	     clientf( buf2 );
	  }
	else
	  clientf( C_D "(" C_R "multiple areas matched" C_D ")" C_0 );
     }
   else
     clientf( C_D "(" C_R "unknown" C_D ")" C_0 );
   
   clientf( "\r\n" );
}



void parse_underwater( char *line )
{
   DEBUG( "parse_underwater" );
   
   /* 1 - No, 2 - Trying to get up, 3 - Up. */
   
   if ( !strcmp( line, "form an air pocket around you." ) ||
	!strcmp( line, "You are already surrounded by a pocket of air." ) ||
	!strcmp( line, "You are surrounded by a pocket of air and so must move normally through water." ) )
     pear_defence = 2;
   
   if ( !strcmp( line, "The pocket of air around you dissipates into the atmosphere." ) ||
	!strcmp( line, "The bubble of air around you dissipates." ) )
     pear_defence = 0;
}



void parse_special_exits( char *line )
{
   EXIT_DATA *spexit;
   
   DEBUG( "parse_special_exits" );
   
   if ( !current_room )
     return;
   
   if ( mode != FOLLOWING && mode != CREATING )
     return;
   
   /* Since this function is checked after parse_room, this is a good place. */
   if ( mode == CREATING && capture_special_exit )
     {
	strcpy( cse_message, line );
	return;
     }
   
   for ( spexit = current_room->special_exits; spexit; spexit = spexit->next )
     {
	if ( !spexit->message )
	  continue;
	
	if ( !cmp( spexit->message, line ) )
	  {
	     if ( spexit->to )
	       {
		  current_room = spexit->to;
		  current_area = current_room->area;
		  add_queue_top( -1 );
	       }
	     else
	       {
		  mode = GET_UNLOST;
	       }
	     
	     return;
	  }
     }
   
   for ( spexit = global_special_exits; spexit; spexit = spexit->next )
     {
	if ( !spexit->message )
	  continue;
	
	if ( !cmp( spexit->message, line ) )
	  {
	     if ( spexit->to )
	       {
		  current_room = spexit->to;
		  current_area = current_room->area;
		  add_queue_top( -1 );
	       }
	     else
	       {
		  mode = GET_UNLOST;
	       }
	     
	     clientff( C_R " (" C_Y "%s" C_R ")" C_0, spexit->command ? spexit->command : "teleport" );
	     return;
	  }
     }
}



void parse_sprint( char *line )
{
   static int sprinting;
   char buf[256];
   
   DEBUG( "parse_squint" );
   
   if ( mode != FOLLOWING && mode != CREATING )
     return;
   
   if ( !sprinting && !strncmp( line, "You look off to the ", 20 ) &&
	strstr( line, " and dash speedily away." ) )
     {
	int i;
	
	get_string( line + 20, buf, 256 );
	
	sprinting = 0;
	
	for ( i = 1; dir_name[i]; i++ )
	  {
	     if ( !strcmp( buf, dir_name[i] ) )
	       {
		  sprinting = i;
		  break;
	       }
	  }
	
	return;
     }
   
   if ( !sprinting )
     return;
   
   /* End of the sprint? */
   if ( strncmp( line, "You dash through ", 17 ) )
     {
	add_queue_top( sprinting );
	sprinting = 0;
	return;
     }
   
   if ( !current_room || !current_room->exits[sprinting] ||
	room_cmp( current_room->exits[sprinting]->name, line + 17 ) )
     {
	sprinting = 0;
	clientf( " " C_D "(" C_G "lost" C_D ")" C_0 );
	return;
     }
   
   current_room = current_room->exits[sprinting];
   clientff( " " C_D "(" C_G "%d" C_D ")" C_0, current_room->vnum );
}



void parse_follow( char *line )
{
   static char msg[256];
   static int in_message;
   char *to;
   
   if ( mode != FOLLOWING || !current_room )
     return;
   
   if ( !cmp( "You follow *", line ) )
     {
	in_message = 1;
	
	strcpy( msg, line );
     }
   else if ( in_message )
     {
	if ( strlen( msg ) > 0 && msg[strlen(msg)-1] != ' ' )
	  strcat( msg, " " );
	strcat( msg, line );
     }
   else
     return;
   
   if ( in_message && !strchr( msg, '.' ) )
     return;
   
   in_message = 0;
   
   if ( !( to = strstr( msg, " to " ) ) )
     return;
   
     {
	/* We now have our message, which should look like this... */
	/* You follow <TitledName> <direction> to <roomname> */
	
	ROOM_DATA *destination;
	char room[256], dir[256];
	char *d;
	int changed_area = 0;
	int i, dir_nr = 0;
	
	/* Snatch the room name, then look backwards for the direction. */
	strcpy( room, to + 4 );
	if ( !room[0] )
	  {
	     debugf( "No room name found." );
	     return;
	  }
	
	d = to - 1;
	while ( d > msg && *d != ' ' )
	  d--;
	get_string( d, dir, 256 );
	
	/* We now have the room, and the direction. */
	
	for ( i = 1; dir_name[i]; i++ )
	  if ( !strcmp( dir, dir_name[i] ) )
	    {
	       dir_nr = i;
	    }
	
	if ( !dir_nr )
	  return;
	
	destination = current_room->exits[dir_nr];
	
	if ( !destination || room_cmp( destination->name, room ) )
	  {
	     /* Perhaps it's a special exit, so look for one. */
	     EXIT_DATA *spexit;
	     
	     for ( spexit = current_room->special_exits; spexit; spexit = spexit->next )
	       {
		  if ( !spexit->to || room_cmp( spexit->to->name, room ) )
		    continue;
		  
		  destination = spexit->to;
		  clientff( C_R " (" C_Y "sp" C_R ")" C_0 );
		  break;
	       }
	     
	     /* Or perhaps we're just simply lost... */
	     if ( !spexit )
	       {
		  clientff( C_R " (" C_G "lost" C_R ")" C_0 );
		  mode = GET_UNLOST;
		  return;
	       }
	  }
	
	if ( current_room->area != destination->area )
	  changed_area = 1;
	
	current_room = destination;
	
	clientff( C_R " (" C_g "%d" C_R ")" C_0, current_room->vnum );
	if ( changed_area )
	  {
	     current_area = current_room->area;
	     clientff( "\r\n" C_R "[Entered the %s]" C_0,
		       current_area->name );
	  }
     }
}


void parse_autobump( char *line )
{
   /* 1 - Bump all around that room. */
   /* 2 - Wait for bumps to complete. */
   /* 3 - Search a new room. */
   
   if ( !auto_bump )
     return;
   
   if ( auto_bump == 2 )
     {
	if ( !strcmp( "There is no exit in that direction.", line ) )
	  bump_exits--;
	
	if ( !bump_exits )
	  {
	     ROOM_DATA *r;
	     int i;
	     
	     auto_bump = 3;
	     
	     r = bump_room, i = 0;
	     /* Count rooms left. */
	     while ( r )
	       {
		  r = r->next_in_area;
		  i++;
	       }
	     
	     clientff( C_R "\r\n[Rooms left: %d.]\r\n" C_0, i - 1 );
	     
	     bump_room = bump_room->next_in_area;
	     
	     if ( !bump_room )
	       {
		  auto_bump = 0;
		  clientfr( "All rooms completed." );
	       }
	     
	     init_openlist( NULL );
	     init_openlist( bump_room );
	     path_finder( );
	     go_next( );
	  }
     }
   
}


void check_autobump( )
{
   if ( !auto_bump || !bump_room )
     return;
   
   /* 1 - Bump all around that room. */
   /* 2 - Wait for bumps to complete. */
   /* 3 - Search a new room. */
   
   if ( auto_bump == 1 )
     {
	bump_exits = 0;
	int i;
	
	send_to_server( "survey\r\n" );
	clientff( "\r\n" );
	
	for ( i = 1; dir_name[i]; i++ )
	  {
	     if ( bump_room->exits[i] )
	       continue;
	     
	     bump_exits++;
	     send_to_server( dir_small_name[i] );
	     send_to_server( "\r\n" );
	  }
	
	auto_bump = 2;
     }
   
   if ( auto_bump == 2 )
     return;
   
   if ( auto_bump == 3 && bump_room == current_room )
     {
	/* We're here! */
	
	auto_bump = 1;
	check_autobump( );
     }
}



void i_mapper_process_server_line_prefix( char *colorless_line, char *colorful_line, char *raw_line )
{
   char *line = colorless_line;
   
   DEBUG( "i_mapper_process_server_line_prefix" );
   
   if ( auto_walk &&
	( !cmp( line, "You cannot move that fast, slow down!" ) ||
	  !cmp( line, "Now now, don't be so hasty!" ) ) )
     {
	gag_line( 1 );
	gag_prompt( 1 );
     }
   
   if ( auto_bump && !cmp( line, "There is no exit in that direction." ) )
     {
	gag_line( 1 );
	gag_prompt( 1 );
     }
   
   /* Gag/replace the alertness message, with something easier to read. */
   parse_alertness( colorless_line );
   
   parse_who( colorful_line, colorless_line );
}


void i_mapper_process_server_line_suffix( char *colorless_line, char *colorful_line, char *raw_line )
{
   const char *block_messages[] =
     {    "You cannot move that fast, slow down!",
	  "You cannot move until you have regained equilibrium.",
	  "You are regaining balance and are unable to move.",
	  "You'll have to swim to make it through the water in that direction.",
	  "rolling around in the dirt a bit.",
	  "You are asleep and can do nothing. WAKE will attempt to wake you.",
	  "There is a door in the way.",
	  "You must first raise yourself and stand up.",
	  "You are surrounded by a pocket of air and so must move normally through water.",
	  "You cannot fly indoors.",
	  "You slip and fall on the ice as you try to leave.",
	  "A rite of piety prevents you from leaving.",
	  "A wall blocks your way.",
	  "The hands of the grave grasp at your ankles and throw you off balance.",
	  "You are unable to see anything as you are blind.",
	  "The forest seems to close up before you and you are prevented from moving that ",
	  "There is no exit in that direction.",
	  "The ground seems to tilt and you fall with a thump.",
	  "Your legs are tangled in a mass of rope and you cannot move.",
	  "You must stand up to do that.",
	  "Your legs are crippled, how will you move?",
	  "You cannot walk through an enormous stone gate.",
	  
	  /* Lusternia. */
	  "Now now, don't be so hasty!",
	  "You are recovering equilibrium and cannot move yet.",
	  "There's water ahead of you. You'll have to swim in that direction to make it ",
	  
	  /* Aetolia. */
	  "You must first raise yourself from the floor and stand up.",
	  
	  NULL
     };
   char *line = colorless_line;
   int i;
   
   DEBUG( "i_mapper_process_server_line" );
   
   if ( !colorful_line[0] )
     return;
   
   /* Are we sprinting, now? */
   parse_sprint( colorless_line );
   
   /* Is this a room? Parse it. */
   parse_room( colorless_line, raw_line );
   
   /* Is this a special exit message? */
   parse_special_exits( colorless_line );
   
   /* Is this a follow message, if we're following someone? */
   parse_follow( colorless_line );
   
   /* Is this a sense/seek command? */
   if ( !disable_locating )
     {
	parse_msense( colorless_line );
	parse_window( colorless_line );
	parse_scent( colorless_line );
	parse_scry( colorless_line );
	parse_ka( colorless_line );
	parse_seek( colorless_line );
	parse_scout( colorless_line );
	parse_pursue( colorless_line );
	parse_eventstatus( colorless_line );
	
	/* Is this a fullsense command? */
	parse_fullsense( colorless_line );
	parse_shrinesight( colorless_line );
     }
   
   /* Can we get the area name and room type from here? */
   parse_survey( line );
   
   for ( i = 0; block_messages[i]; i++ )  
     if ( !strcmp( line, block_messages[i] ) )
       {
	  /* Remove last command from the queue. */
	  if ( q_top )
	    q_top--;
	  
	  if ( !q_top )
	    del_timer( "queue_reset_timer" );
	  
	  break;
       }
   
   if ( mode == FOLLOWING || mode == CREATING )
     parse_underwater( line );
   
   parse_autobump( line );
   
   if ( auto_walk )
     {
	if ( !strcmp( line, "You'll have to swim to make it through the water in that direction." ) )
	  {
	     if ( disable_swimming )
	       {
		  clientf( "\r\n" C_R "[Hint: Swimming is disabled. Use 'map config swim' to turn it back on.]" C_0 );
	       }
	  }
	     
	if ( !strcmp( line, "You cannot move that fast, slow down!" ) ||
	     !strcmp( line, "Now now, don't be so hasty!" ) )
	  {
	     auto_walk = 2;
	  }
	
	if ( !strcmp( line, "There is a door in the way." ) )
	  {
	     door_closed = 1;
	     door_locked = 0;
	     auto_walk = 2;
	  }
	
	if ( !strcmp( line, "The door is locked." ) )
	  {
	     door_closed = 0;
	     door_locked = 1;
	     auto_walk = 2;
	  }
	
	if ( !strncmp( line, "You open the door to the ", 25 ) )
	  {
	     door_closed = 0;
	     door_locked = 0;
	     auto_walk = 2;
	  }
	
	if ( !strncmp( line, "You unlock the door to the ", 27 ) )
	  {
	     door_closed = 1;
	     door_locked = 0;
	     auto_walk = 2;
	  }
	
	if ( !strcmp( line, "You are not carrying a key for this door." ) )
	  {
	     door_closed = 0;
	     door_locked = 0;
//	     auto_walk = 0;
//	     clientfr( "Auto-walk disabled." );
	  }
	
	if ( !strncmp( line, "There is no door to the ", 24 ) )
	  {
	     door_closed = 0;
	     door_locked = 0;
	     auto_walk = 2;
	  }
     }
}




void i_mapper_process_server_prompt_informative( char *line, char *rawline )
{
   DEBUG( "i_mapper_process_server_prompt_informative" );
   
   if ( parsing_room )
     parsing_room = 0;
   
   if ( sense_message )
     sense_message = 0;
   
   if ( scout_list )
     scout_list = 0;
   
   if ( evstatus_list )
     evstatus_list = 0;
   
   if ( get_unlost_exits )
     {
	ROOM_DATA *r, *found = NULL;
	int count = 0, i;
	
	get_unlost_exits = 0;
	
	if ( !current_room )
	  return;
	
	for ( r = world; r; r = r->next_in_world )
	  {
	     if ( !strcmp( r->name, current_room->name ) )
	       {
		  int good = 1;
		  
		  for ( i = 1; dir_name[i]; i++ )
		    {
		       if ( ( r->exits[i] && !get_unlost_detected_exits[i] ) ||
			    ( !r->exits[i] && get_unlost_detected_exits[i] ) )
			 {
			    good = 0;
			    break;
			 }
		    }
		  
		  if ( good )
		    {
		       if ( !found )
			 found = r;
		       
		       count++;
		    }
	       }
	  }
	
	if ( !found )
	  {
	     clientfr( "No perfect matches found." );
	     mode = GET_UNLOST;
	  }
	else
	  {
	     char buf[256];
	     
	     current_room = found;
	     current_area = current_room->area;
	     mode = FOLLOWING;
	     
	     if ( !count )
	       {
		  clientfr( "Impossible error." );
		  return;
	       }
	     
	     sprintf( buf, "Match probability: %d%%", 100 / count );
	     clientfr( buf );
	  }
     }
}



void i_mapper_process_server_prompt_action( char *line )
{
   int prompt_newline = 0;
   
   DEBUG( "i_mapper_process_server_prompt_action" );
   
   if ( current_room && !pear_defence &&
	( current_room->underwater ||
	  current_room->room_type->underwater ) )
     {
	pear_defence = 1;
	send_to_server( "outr pear\r\neat pear\r\n" );
	clientf( C_W "(" C_G "outr/eat pear" C_W ") " C_0 );
	prompt_newline = 1;
     }
   
   if ( mode == FOLLOWING && auto_walk == 2 )
     {
	go_next( );
	/* Tricky. ;) */
	prompt_newline = 0;
     }
   
   check_autobump( );
   
   if ( prompt_newline )
     clientf( "\r\n" );
}



/* Not needed, for now. Everything is in process_client_aliases..
int i_mapper_process_client_command( char *cmd )
{
   static char buf[256];
   
   DEBUG( "process_client_command" );
   
   return 0;
} */



/* Map commands. */

void do_map( char *arg )
{
   ROOM_DATA *room;
   
   if ( arg[0] && isdigit( arg[0] ) )
     {
	if ( !( room = get_room( atoi( arg ) ) ) )
	  {
	     clientfr( "No room with that vnum found." );
	     return;
	  }
     }
   else
     {
	if ( current_room )
	  room = current_room;
	else
	  {
	     clientfr( "No current room set." );
	     return;
	  }
     }
   
   show_map_new( room );
}



void do_map_old( char *arg )
{
   ROOM_DATA *room;
   
   if ( arg[0] && isdigit( arg[0] ) )
     {
	if ( !( room = get_room( atoi( arg ) ) ) )
	  {
	     clientfr( "No room with that vnum found." );
	     return;
	  }
     }
   else
     {
	if ( current_room )
	  room = current_room;
	else
	  {
	     clientfr( "No current room set." );
	     return;
	  }
     }
   
   show_map( room );
}



void do_map_help( char *arg )
{
   clientfr( "Module: IMapper. Commands:" );
   clientf( " map help     - This help.\r\n"
	    " area help    - Area commands help.\r\n"
	    " room help    - Room commands help.\r\n"
	    " exit help    - Room exit commands help.\r\n"
	    " map load     - Load map.\r\n"
	    " map save     - Save map.\r\n"
	    " map none     - Disable following or mapping.\r\n"
	    " map follow   - Enable following.\r\n"
	    " map create   - Enable mapping.\r\n"
	    " map remake   - Remake vnums.\r\n"
	    " map path #   - Build directions to vnum #.\r\n"
	    " map status   - Show general information about the map.\r\n"
	    " map color    - Change the color of the room title.\r\n"
	    " map file     - Set the file for map load and map save.\r\n"
	    " map teleport - Manage global special exits.\r\n"
	    " map queue    - Show the command queue.\r\n"
	    " map queue cl - Clear the command queue.\r\n"
	    " map config   - Configure the mapper.\r\n"
	    " map          - Generate a map, from the current room.\r\n"
	    " landmarks    - Show all the landmarks, in the world.\r\n"
	    " go/stop      - Begins, or stops speedwalking.\r\n" );
}



void do_map_remake( char *arg )
{
   if ( mode != CREATING )
     {
	clientfr( "Turn mapping on, first." );
	return;
     }
   
   remake_vnums( );
}



void do_map_create( char *arg )
{
   if ( world == NULL )
     {
	clientfr( "Mapping on. Please 'look', to update this room." );
	create_area( );
	current_area = areas;
	
	create_room( -1 );
	current_room = world;
	
	current_area->name = strdup( "New area" );
	
	q_top = 0;
     }
   else if ( !current_room )
     {
	clientfr( "No current room set, from which to begin mapping." );
	return;
     }
   else
     clientfr( "Mapping on." );
   
   mode = CREATING;
}



void do_map_follow( char *arg )
{
   if ( !current_room )
     {
	mode = GET_UNLOST;
	clientfr( "Will try to find ourselves on the map." );
     }
   else
     {
	mode = FOLLOWING;
	clientfr( "Following on." );
     }
}



void do_map_none( char *arg )
{
   if ( mode != NONE )
     {
	mode = NONE;
	clientfr( "Mapping and following off." );
     }
   else
     clientfr( "Mapping or following were not enabled, anyway." );
}



void do_map_save( char *arg )
{
   if ( areas )
     save_map( map_file );
   else
     clientfr( "No areas to save." );
}



void do_map_load( char *arg )
{
   char buf[256];
   
   destroy_map( );
   get_timer( );
   if ( load_map( map_file ) )
     {
	destroy_map( );
	clientfr( "Couldn't load map." );
     }
   else
     {
	sprintf( buf, "Map loaded. (%d microseconds)", get_timer( ) );
	clientfr( buf );
	convert_vnum_exits( );
	sprintf( buf, "Vnum exits converted. (%d microseconds)", get_timer( ) );
	clientfr( buf );
	check_map( );
	load_settings( "settings.mapper.txt" );
	
	mode = GET_UNLOST;
     }
}



void do_map_path( char *arg )
{
   ROOM_DATA *room;
   char buf[256];
   
   if ( !isdigit( *(arg) ) && *(arg) != 'n' )
     {
	init_openlist( NULL );
	path_finder( );
	clientfr( "Map directions cleared." );
	
	return;
     }
   
   init_openlist( NULL );
   
   while ( arg[0] )
     {
	int neari = 0;
	
	arg = get_string( arg, buf, 256 );
	
	if ( !strcmp( buf, "near" ) )
	  {
	     arg = get_string( arg, buf, 256 );
	     neari = 1;
	  }
	
	room = get_room( atoi( buf ) );
	
	if ( !room )
	  {
	     clientff( C_R "[No room with the vnum '%s' was found.]\r\n" C_0, buf );
	     init_openlist( NULL );
	     return;
	  }
	else
	  {
	     if ( !neari )
	       init_openlist( room );
	     else
	       {
		  for ( neari = 1; dir_name[neari]; neari++ )
		    if ( room->exits[neari] )
		      init_openlist( room->exits[neari] );
	       }
	  }
     }
   
   get_timer( );
   path_finder( );
   sprintf( buf, "Path calculated in: %d microseconds.", get_timer( ) );
   clientfr( buf );
   
   if ( current_room )
     show_path( current_room );
   else
     clientfr( "Can't show the path from here though, as no current room is set." );
}



void do_map_status( char *arg )
{
   AREA_DATA *a;
   ROOM_DATA *r;
   char buf[256];
   int count = 0, count2 = 0;
   int i;
   
   DEBUG( "do_map_status" );
   
   clientfr( "General information:" );
   
   for ( r = world; r; r = r->next_in_world )
     count++, count2 += sizeof( ROOM_DATA );
   
   sprintf( buf, "Rooms: " C_G "%d" C_0 ", using " C_G "%d" C_0 " bytes of memory.\r\n", count, count2 );
   clientf( buf );
   
   count = 0, count2 = 0;
   
   for ( a = areas; a; a = a->next )
     {
	int notype = 0, unlinked = 0;
	count++;
	
	for ( r = a->rooms; r; r = r->next_in_area )
	  {
	     if ( !notype && r->room_type == null_room_type )
	       notype = 1;
	     if ( !unlinked )
	       for ( i = 1; dir_name[i]; i++ )
		 if ( !r->exits[i] && r->detected_exits[i] &&
		      !r->locked_exits[i] )
		   unlinked = 1;
	  }
	
	if ( !( notype || unlinked ) )
	  count2++;
     }
   
   sprintf( buf, "Areas: " C_G "%d" C_0 ", of which fully mapped: " C_G "%d" C_0 ".\r\n", count, count2 );
   clientf( buf );
   
   sprintf( buf, "Room structure size: " C_G "%d" C_0 " bytes.\r\n", sizeof( ROOM_DATA ) );
   clientf( buf );
   
   sprintf( buf, "Hash table size: " C_G "%d" C_0 " chains.\r\n", MAX_HASH );
   clientf( buf );
   
   sprintf( buf, "Map size, x: " C_G "%d" C_0 " y: " C_G "%d" C_0 ".\r\n", MAP_X, MAP_Y );
   clientf( buf );
   
   for ( i = 0; color_names[i].name; i++ )
     {
	if ( !strcmp( color_names[i].title_code, room_color ) )
	  break;
     }
   
   sprintf( buf, "Room title color: %s%s" C_0 ", code length " C_G "%d" C_0 ".\r\n", room_color,
	    color_names[i].name ? color_names[i].name : "unknown", room_color_len );
   clientf( buf );
}



void do_map_color( char *arg )
{
   char buf[256];
   int i;
   
   arg = get_string( arg, buf, 256 );
   
   for ( i = 0; color_names[i].name; i++ )
     {
	if ( !strcmp( color_names[i].name, buf ) )
	  {
	     room_color = color_names[i].title_code;
	     room_color_len = color_names[i].length;
	     
	     sprintf( buf, "Room title color changed to: " C_0 "%s%s" C_R ".",
		      room_color, color_names[i].name );
	     clientfr( buf );
	     clientfr( "Use 'map config save' to make it permanent." );
	     
	     return;
	  }
     }
   
   clientfr( "Possible room-title colors:" );
   
   for ( i = 0; color_names[i].name; i++ )
     {
	sprintf( buf, "%s - " C_0 "%s%s" C_0 ".\r\n",
		 !strcmp( color_names[i].title_code, room_color ) ? C_R : "",
		 color_names[i].title_code, color_names[i].name );
	clientf( buf );
     }
   
   clientfr( "Use 'map color <name>' to change it." );
}



void do_map_bump( char *arg )
{
   if ( !strcmp( arg, "skip" ) )
     {
	if ( bump_room && bump_room->next_in_area )
	  {
	     bump_room = bump_room->next_in_area;
	     init_openlist( NULL );
	     init_openlist( bump_room );
	     path_finder( );
	     clientfr( "Skipped one room." );
	  }
	else
	  clientfr( "Skipping a room is not possible." );
	
	return;
     }
   
   if ( !strcmp( arg, "continue" ) )
     {
	if ( !bump_room )
	  clientfr( "Unable to continue bumping." );
	else
	  {
	     clientfr( "Bumping continues." );
	     
	     auto_bump = 3;
	     
	     init_openlist( NULL );
	     init_openlist( bump_room );
	     path_finder( );
	     go_next( );
	  }
	
	return;
     }
   
   if ( auto_bump )
     {
	auto_bump = 0;
	clientfr( "Bumping ended." );
	return;
     }
   
   if ( !current_room )
     {
	clientfr( "No current room set." );
	return;
     }
   
   
   clientfr( "Bumping begins." );
   
   auto_bump = 3;
   
   bump_room = current_room->area->rooms;
   init_openlist( NULL );
   init_openlist( bump_room );
   path_finder( );
   go_next( );
}



void do_map_queue( char *arg )
{
   char buf[256];
   int i;
   
   if ( arg[0] == 'c' )
     {
	q_top = 0;
	clientfr( "Queue cleared." );
	del_timer( "queue_reset_timer" );
	return;
     }
   
   if ( q_top )
     {
	clientfr( "Command queue:" );
	
	for ( i = 0; i < q_top; i++ )
	  {
	     sprintf( buf, C_R " %d: %s.\r\n" C_0, i,
		      queue[i] < 0 ? "look" : dir_name[queue[i]] );
	     clientf( buf );
	  }
     }
   else
     clientfr( "Queue empty." );
}



void do_map_config( char *arg )
{
   char option[256];
   
   arg = get_string( arg, option, 256 );
   
   if ( !strcmp( option, "save" ) )
     {
	if ( save_settings( "settings.mapper.txt" ) )
	  clientfr( "Unable to open the file for writing." );
	else
	  clientfr( "All settings saved." );
	return;
     }
   else if ( !strcmp( option, "load" ) )
     {
	if ( load_settings( "settings.mapper.txt" ) )
	  clientfr( "Unable to open the file for reading." );
	else
	  clientfr( "All settings reloaded." );
	return;
     }
   else if ( !strcmp( option, "swim" ) )
     {
	disable_swimming = disable_swimming ? 0 : 1;
	if ( disable_swimming )
	  clientfr( "Swimming disabled - will now walk over water." );
	else
	  clientfr( "Swimming enabled." );
     }
   else if ( !strcmp( option, "wholist" ) )
     {
	disable_wholist = disable_wholist ? 0 : 1;
	if ( disable_wholist )
	  clientfr( "Parsing disabled." );
	else
	  clientfr( "Parsing enabled." );
     }
   else if ( !strcmp( option, "alertness" ) )
     {
	disable_alertness = disable_alertness ? 0 : 1;
	if ( disable_alertness )
	  clientfr( "Parsing disabled." );
	else
	  clientfr( "Parsing enabled." );
     }
   else if ( !strcmp( option, "locate" ) )
     {
	disable_locating = disable_locating ? 0 : 1;
	if ( disable_locating )
	  clientfr( "Locating disabled." );
	else
	  clientfr( "Vnums will now be displayed on locating abilities." );
     }
   else if ( !strcmp( option, "showarea" ) )
     {
	disable_areaname = disable_areaname ? 0 : 1;
	if ( disable_areaname )
	  clientfr( "The area name will no longer be shown." );
	else
	  clientfr( "The area name will be shown after room titles." );
     }
   else
     {
	clientfr( "Commands:" );
	clientf( " map config save      - Save all settings.\r\n"
		 " map config load      - Reload the previously saved settings.\r\n"
		 " map config swim      - Toggle swimming.\r\n"
		 " map config wholist   - Parse and replace the 'who' list.\r\n"
		 " map config alertness - Parse and replace alertness messages.\r\n"
		 " map config locate    - Append vnums to various locating abilities.\r\n"
		 " map config showarea  - Show the current area after a room title.\r\n" );
	
	return;
     }
  
   clientfr( "Use 'map config save' to make it permanent." );
}




/* Area commands. */

void do_area_help( char *arg )
{
   clientfr( "Module: IMapper. Area commands:" );
   clientf( " area create - Create a new area, and switch to it.\r\n"
	    " area list   - List all areas.\r\n"
	    " area switch - Switch the area of the current room.\r\n"
	    " area update - Update current area's name.\r\n"
	    " area info   - Show information about current area.\r\n"
	    " area off    - Disable pathfinding in the current area.\r\n"
	    " area orig   - Check the originality of the area.\r\n" );
}



void do_area_orig( char *arg )
{
   ROOM_DATA *room, *r;
   int rooms = 0, original_rooms = 0;
   
   if ( !current_area )
     {
	clientfr( "No current area." );
	return;
     }
   
   for ( room = current_area->rooms; room; room = room->next_in_area )
     {
	/* Add one to the room count. */
	rooms++;
	
	/* And check if there is any other room with this same name. */
	original_rooms++;
	for ( r = current_area->rooms; r; r = r->next_in_area )
	  if ( !strcmp( r->name, room->name ) && r != room )
	    {
	       original_rooms--;
	       break;
	    }
     }
   
   if ( !rooms )
     {
	clientfr( "The area is empty..." );
	return;
     }
   
   clientff( C_R "[Rooms: " C_G "%d" C_R "  Original rooms: " C_G "%d" C_R
	     "  Originality: " C_G "%d%%" C_R "]\r\n" C_0,
	     rooms, original_rooms,
	     original_rooms ? original_rooms * 100 / rooms : 0 );
}


void do_map_orig( char *arg )
{
   ROOM_DATA *room, *r;
   int rooms = 0, original_rooms = 0;
   
   for ( room = world; room; room = room->next_in_world )
     {
	/* Add one to the room count. */
	rooms++;
	
	/* And check if there is any other room with this same name. */
	original_rooms++;
	for ( r = world; r; r = r->next_in_world )
	  if ( !strcmp( r->name, room->name ) && r != room )
	    {
	       original_rooms--;
	       break;
	    }
     }
   
   if ( !rooms )
     {
	clientfr( "The area is empty..." );
	return;
     }
   
   clientff( C_R "[Rooms: " C_G "%d" C_R "  Original rooms: " C_G "%d" C_R
	     "  Originality: " C_G "%d%%" C_R "]\r\n" C_0,
	     rooms, original_rooms,
	     original_rooms ? original_rooms * 100 / rooms : 0 );
}



void do_map_file( char *arg )
{
   if ( !arg[0] )
     strcpy( map_file, "IMap" );
   else
     strcpy( map_file, arg );
   
   clientff( C_R "[File for map load/save set to '%s'.]\r\n" C_0, map_file );
}



/* Snatched from do_exit_special. */
void do_map_teleport( char *arg )
{
   EXIT_DATA *spexit;
   char cmd[256];
   char buf[256];
   int i, nr = 0;
   
   DEBUG( "do_map_teleport" );
   
   if ( mode != CREATING && strcmp( arg, "list" ) )
     {
	clientfr( "Turn mapping on, first." );
	return;
     }
   
   if ( !strncmp( arg, "help", strlen( arg ) ) )
     {
	clientfr( "Syntax: map teleport <command> [exit] [args]" );
	clientfr( "Commands: list, add, create, destroy, name, message, link" );
	return;
     }
   
   arg = get_string( arg, cmd, 256 );
   
   if ( !strcmp( cmd, "list" ) )
     {
	clientfr( "Global special exits:" );
	for ( spexit = global_special_exits; spexit; spexit = spexit->next )
	  {
	     sprintf( buf, C_B "%3d" C_0 " - L: " C_G "%4d" C_0 " N: '" C_g "%s" C_0 "' M: '" C_g "%s" C_0 "'\r\n",
		      nr++, spexit->vnum, spexit->command, spexit->message );
	     clientf( buf );
	  }
	
	if ( !nr )
	  clientf( " - None.\r\n" );
     }
   
   else if ( !strcmp( cmd, "add" ) )
     {
	char name[256];
	
	arg = get_string( arg, name, 256 );
	
	if ( !arg[0] )
	  {
	     clientfr( "Syntax: map teleport add <name> <message>" );
	     return;
	  }
	
	spexit = create_exit( NULL );
	spexit->command = strdup( name );
	spexit->message = strdup( arg );
	
	clientfr( "Global special exit created." );
     }
   
   else if ( !strcmp( cmd, "create" ) )
     {
	create_exit( NULL );
	clientfr( "Global special exit created." );
     }
   
   else if ( !strcmp( cmd, "destroy" ) )
     {
	int done = 0;
	
	if ( !arg[0] || !isdigit( arg[0] ) )
	  {
	     clientfr( "What global special exit do you wish to destroy?" );
	     return;
	  }
	
	nr = atoi( arg );
	
	for ( i = 0, spexit = global_special_exits; spexit; spexit = spexit->next )
	  {
	     if ( i++ == nr )
	       {
		  destroy_exit( spexit );
		  done = 1;
		  break;
	       }
	  }
	
	if ( done )
	  sprintf( buf, "Global special exit %d destroyed.", nr );
	else
	  sprintf( buf, "Global special exit %d was not found.", nr );
	clientfr( buf );
     }
   
   else if ( !strcmp( cmd, "message" ) )
     {
	int done = 0;
	
	/* We'll store the number in cmd. */
	arg = get_string( arg, cmd, 256 );
	
	if ( !isdigit( cmd[0] ) )
	  {
	     clientfr( "Set message on what global special exit?" );
	     return;
	  }
	
	nr = atoi( cmd );
	
	for ( i = 0, spexit = global_special_exits; spexit; spexit = spexit->next )
	  {
	     if ( i++ == nr )
	       {
		  if ( spexit->message )
		    free( spexit->message );
		  
		  spexit->message = strdup( arg );
		  done = 1;
		  break;
	       }
	  }
	
	if ( done )
	  sprintf( buf, "Message on global exit %d changed to '%s'", nr, arg );
	else
	  sprintf( buf, "Can't find global special exit %d.", nr );
	
	clientfr( buf );
     }
   
   else if ( !strcmp( cmd, "name" ) )
     {
	int done = 0;
	
	/* We'll store the number in cmd. */
	arg = get_string( arg, cmd, 256 );
	
	if ( !isdigit( cmd[0] ) )
	  {
	     clientfr( "Set name on what global special exit?" );
	     return;
	  }
	
	nr = atoi( cmd );
	
	for ( i = 0, spexit = global_special_exits; spexit; spexit = spexit->next )
	  {
	     if ( i++ == nr )
	       {
		  if ( spexit->command )
		    free( spexit->command );
		  
		  if ( arg[0] )
		    spexit->command = strdup( arg );
		  done = 1;
		  break;
	       }
	  }
	
	if ( done )
	  {
	     if ( arg[0] )
	       sprintf( buf, "Name on global exit %d changed to '%s'", nr, arg );
	     else
	       sprintf( buf, "Name on global exit %d cleared.", nr );
	  }
	else
	  sprintf( buf, "Can't find global special exit %d.", nr );
	
	clientfr( buf );
     }
   
   else if ( !strcmp( cmd, "link" ) )
     {
	/* E special link 0 -1 */
	char vnum[256];
	
	/* We'll store the number in cmd. */
	arg = get_string( arg, cmd, 256 );
	
	if ( !isdigit( cmd[0] ) )
	  {
	     clientfr( "Link which global special exit?" );
	     return;
	  }
	
	nr = atoi( cmd );
	
	for ( i = 0, spexit = global_special_exits; spexit; spexit = spexit->next )
	  {
	     if ( i++ == nr )
	       {
		  get_string( arg, vnum, 256 );
		  
		  if ( vnum[0] == '-' )
		    {
		       ROOM_DATA *to;
		       
		       to = spexit->to;
		       
		       spexit->vnum = -1;
		       spexit->to = NULL;
		       
		       check_pointed_by( to );
		       
		       sprintf( buf, "Link cleared on exit %d.", nr );
		       clientfr( buf );
		       return;
		    }
		  else if ( isdigit( vnum[0] ) )
		    {
		       spexit->vnum = atoi( vnum );
		       spexit->to = get_room( spexit->vnum );
		       if ( !spexit->to )
			 {
			    clientfr( "A room whith that vnum was not found." );
			    spexit->vnum = -1;
			    return;
			 }
		       sprintf( buf, "Global special exit %d linked to '%s'.",
				nr, spexit->to->name );
		       clientfr( buf );
		       return;
		    }
		  else
		    {
		       clientfr( "Link to which vnum?" );
		       return;
		    }
	       }
	  }
	
	sprintf( buf, "Can't find global special exit %d.", nr );
	clientfr( buf );
     }
   
   else
     {
	clientfr( "Unknown command... Try 'map teleport help'." );
     }
}




void do_area_create( char *arg )
{
   AREA_DATA *new_area;
   
   if ( mode != CREATING )
     {
	clientfr( "Turn mapping on, first." );
	return;
     }
   
   new_area = create_area( );
   
   new_area->name = strdup( "New area" );
   
   current_area = new_area;
	     
   clientfr( "Area created. Next room will be in that area." );
}



void do_area_list( char *arg )
{
   AREA_DATA *area;
   ROOM_DATA *room;
   const int align = 37;
   char spcs[256];
   char buf[1024];
   int space = 0;
   int right = 1;
   int i;
   
   DEBUG( "do_area_list" );
   
   clientfr( "Areas:" );
   for ( area = areas; area; area = area->next )
     {
	int unlinked = 0, notype = 0;
	
	for ( room = area->rooms; room; room = room->next_in_area )
	  {
	     if ( !notype && room->room_type == null_room_type )
	       notype = 1;
	     if ( !unlinked )
	       for ( i = 1; dir_name[i]; i++ )
		 if ( !room->exits[i] && room->detected_exits[i] &&
		      !room->locked_exits[i] )
		   unlinked = 1;
	  }
	
	if ( !right )
	  {
	     space = align - strlen( area->name );
	     spcs[0] = '\r', spcs[1] = '\n', spcs[2] = 0;
	     right = 1;
	  }
	else
	  {
	     for ( i = 0; i < space; i++ )
	       spcs[i] = ' ';
	     spcs[i] = 0;
	     right = 0;
	  }
	clientf( spcs );
	
	sprintf( buf, " (%s%c" C_0 ") %s%s%s",
		 notype ? C_R : ( unlinked ? C_G : C_B ),
		 notype ? 'x' : ( unlinked ? 'l' : '*' ),
		 area == current_area ? C_W : ( area->disabled ? C_D : "" ),
		 area->name, area == current_area ? C_0 : "" );
	clientf( buf );
     }
   clientf( "\r\n" );
}



void do_area_switch( char *arg )
{
   AREA_DATA *area;
   char buf[256];
   
   if ( mode != CREATING )
     {
	clientfr( "Turn mapping on, first." );
	return;
     }
   
   if ( !current_room )
     {
	clientfr( "No current room, to change." );
	return;
     }
   
   /* Move in a circular motion. */
   area = current_room->area->next;
   if ( !area )
     area = areas;
   if ( current_room->area )
     unlink_from_area( current_room );
   link_to_area( current_room, area );
   current_area = current_room->area;
   
   sprintf( buf, "Area switched to '%s'.",
	    current_area->name );
   clientfr( buf );
}



void do_area_update( char *arg )
{
   if ( mode != CREATING )
     {
	clientfr( "Turn mapping on, first." );
	return;
     }
   
   if ( update_area_from_survey )
     {
	update_area_from_survey = 0;
	clientfr( "Disabled." );
     }
   else
     {
	update_area_from_survey = 1;
	clientfr( "Type survey. The current area's name will be updated." );
     }
}



void do_area_info( char *arg )
{
   ROOM_DATA *room;
   char buf[256];
   int detected_only = 0, unknown_type = 0;
   int rooms = 0, i;
   
   DEBUG( "do_area_info" );
   
   if ( !current_area )
     {
	clientfr( "No current area set." );
	return;
     }
   
   sprintf( buf, "Area: %s", current_area->name );
   clientfr( buf );
   
   for ( room = current_area->rooms; room; room = room->next_in_area )
     {
	rooms++;
	
	if ( room->room_type == null_room_type )
	  unknown_type++;
	
	for ( i = 1; dir_name[i]; i++ )
	  if ( room->detected_exits[i] && !room->exits[i] &&
	       !room->locked_exits[i] )
	    detected_only++;
     }
   
   sprintf( buf, C_R "Rooms: " C_G "%d" C_R "  Unlinked exits: " C_G
	    "%d" C_R "  Unknown type rooms: " C_G "%d" C_R ".",
	    rooms, detected_only, unknown_type );
   clientfr( buf );
}



void do_area_off( char *arg )
{
   char buf[256];
   
   if ( !current_area )
     {
	clientfr( "No current area set." );
	return;
     }
   
   if ( mode != CREATING )
     {
	clientfr( "Turn mapping on, first." );
	return;
     }
   
   current_area->disabled = current_area->disabled ? 0 : 1;
   
   if ( current_area->disabled )
     sprintf( buf, "Pathfinding in '%s' disabled.", current_area->name );
   else
     sprintf( buf, "Pathfinding in '%s' re-enabled.", current_area->name );
   
   clientfr( buf );
}




/* Room commands. */

void do_room_help( char *arg )
{
   clientfr( "Module: IMapper. Room commands:" );
   clientf( " room switch  - Switch current room to another vnum.\r\n"
	    " room create  - Create a new room and switch to it.\r\n"
	    " room look    - Show info on the current room only.\r\n"
	    " room info    - List all rooms that weren't properly mapped.\r\n"
	    " room find    - Find all rooms that contain something in their name.\r\n"
	    " room destroy - Unlink and destroy a room.\r\n"
	    " room list    - List all rooms in the current area.\r\n"
	    " room underw  - Set the room as Underwater.\r\n"
	    " room mark    - Set or clear a landmark on a vnum, or current room.\r\n"
	    " room types   - List all known room types.\r\n" );
}



void do_room_switch( char *arg )
{
   ROOM_DATA *room;
   char buf[256];
   
   if ( !isdigit( *(arg) ) )
     clientfr( "Specify a vnum to switch to." );
   else
     {
	room = get_room( atoi( arg ) );
	if ( !room )
	  clientfr( "No room with that vnum was found." );
	else
	  {
	     sprintf( buf, "Switched to '%s' (%s).", room->name,
		      room->area->name );
	     clientfr( buf );
	     current_room = room;
	     current_area = room->area;
	     if ( mode == GET_UNLOST )
	       mode = FOLLOWING;
	  }
     }
}



void do_room_create( char *arg )
{
   ROOM_DATA *room;
   
   if ( mode != CREATING )
     {
	clientfr( "Turn mapping on, first." );
	return;
     }
   
   room = create_room( -1 );
   
   room->name = strdup( "New room." );
   
   current_room = room;
   current_area = room->area;
   clientfr( "New room created." );
}



void do_room_info( char *arg )
{
   char buf[256];  
   int i;
   
   if ( !current_area )
     clientfr( "No current area." );
   else
     {
	ROOM_DATA *room;
	
	clientfr( "Unlinked or no type rooms:" );
	
	for ( room = current_area->rooms; room; room = room->next_in_area )
	  {
	     int unlinked = 0, notype = 0, locked = 0;
	     
	     if ( room->room_type == null_room_type )
	       notype = 1;
	     for ( i = 1; dir_name[i]; i++ )
	       if ( !room->exits[i] && room->detected_exits[i] )
		 {
		    if ( room->locked_exits[i] )
		      locked = 1;
		    else
		      unlinked = 1;
		 }
	     
	     if ( unlinked || notype )
	       {
		  sprintf( buf, " - %s (" C_G "%d" C_0 ")", room->name, room->vnum );
		  if ( unlinked )
		    strcat( buf, C_B " (" C_G "unlinked" C_B ")" C_0 );
		  else if ( locked )
		    strcat( buf, C_B " (" C_G "locked" C_B ")" C_0 );
		  if ( notype )
		    strcat( buf, C_B " (" C_G "no type" C_B ")" C_0 );
		  strcat( buf, "\r\n" );
		  clientf( buf );
	       }
	  }
     }
}



void do_room_find( char *arg )
{
   ROOM_DATA *room;
   char buf[256];
   
   if ( *(arg) == 0 )
     {
	clientfr( "Find what?" );
	return;
     }
   
   clientfr( "Rooms that match:" );
   for ( room = world; room; room = room->next_in_world )
     {
	if ( case_strstr( room->name, arg ) )
	  {
	     sprintf( buf, " " C_D "(" C_G "%d" C_D ")" C_0 " %s%s%s%s",
		      room->vnum, room->name,
		      room->area != current_area ? " (" : "",
		      room->area != current_area ? room->area->name : "",
		      room->area != current_area ? ")" : "" );
	     if ( mode == CREATING )
	       {
		  int i, first = 1;
		  for ( i = 1; dir_name[i]; i++ )
		    {
		       if ( room->exits[i] || room->detected_exits[i] )
			 {
			    if ( first )
			      {
				 strcat( buf, C_D " (" );
				 first = 0;
			      }
			    else
			      strcat( buf, C_D "," );
			    
			    if ( room->exits[i] )
			      strcat( buf, C_B );
			    else
			      strcat( buf, C_R );
			    
			    strcat( buf, dir_small_name[i] );
			 }
		    }
		  if ( !first )
		    strcat( buf, C_D ")" C_0 );
	       }
	     strcat( buf, "\r\n" );
	     clientf( buf );
	  }
     }
}



void do_room_look( char *arg )
{
   ROOM_DATA *room;
   char buf[256];
   int i;
   
   if ( arg[0] && isdigit( arg[0] ) )
     {
	if ( !( room = get_room( atoi( arg ) ) ) )
	  {
	     clientfr( "No room with that vnum found." );
	     return;
	  }
     }
   else
     {
	if ( current_room )
	  room = current_room;
	else
	  {
	     clientfr( "No current room set." );
	     return;
	  }
     }
   
   sprintf( buf, "Room: %s  Vnum: %d.  Area: %s", room->name ?
	    room->name : "-null-", room->vnum, room->area->name );
   clientfr( buf );
   sprintf( buf, "Type: %s.  Underwater: %s.  Pointed by: %s.", room->room_type->name,
	    room->underwater ? "Yes" : "No", room->pointed_by ? "Yes" : "No" );
   clientfr( buf );
   
   for ( i = 1; dir_name[i]; i++ )
     {
	if ( room->exits[i] )
	  {
	     char lngth[128];
	     if ( room->exit_length[i] )
	       sprintf( lngth, " (%d)", room->exit_length[i] );
	     else
	       lngth[0] = 0;
	     
	     sprintf( buf, "  %s: (%d) %s%s\r\n", dir_name[i],
		      room->exits[i]->vnum,
		      room->exits[i]->name, lngth );
	     clientf( buf );
	  }
	else if ( room->detected_exits[i] )
	  {
	     if ( room->locked_exits[i] )
	       sprintf( buf, "  %s: locked exit.\r\n", dir_name[i] );
	     else
	       sprintf( buf, "  %s: unlinked exit.\r\n", dir_name[i] );
	     clientf( buf );
	  }
     }
}



void do_room_destroy( char *arg )
{
   int i;
   
   if ( mode != CREATING )
     {
	clientfr( "Turn mapping on, first." );
	return;
     }
   
   if ( !isdigit( *(arg) ) )
     clientfr( "Specify a room's vnum to destroy." );
   else
     {
	ROOM_DATA *room;
	
	room = get_room( atoi( arg ) );
	if ( !room )
	  {
	     clientfr( "No room with such a vnum was found." );
	     return;
	  }
	
	if ( room == current_room )
	  clientfr( "Can't destroy the room you're currently in." );
	else
	  {
	     ROOM_DATA *r;
	     EXIT_DATA *e, *e_next;
	     
	     /* We don't want pointers to point in unknown locations, don't we? */
	     for ( i = 1; dir_name[i]; i++ )
	       if ( room->exits[i] )
		 set_reverse( NULL, i, room->exits[i] );
//		 room->exits[i]->reverse_exits[i] = NULL;
	     
	     for ( r = world; r; r = r->next_in_world )
	       {
		  for ( i = 1; dir_name[i]; i++ )
		    if ( r->exits[i] == room )
		      r->exits[i] = NULL;
		  for ( e = r->special_exits; e; e = e_next )
		    {
		       e_next = e->next;
		       if ( e->to == room )
			 destroy_exit( e );
		    }
	       }
	     
	     destroy_room( room );
	     clientfr( "Room unlinked and destroyed." );
	  }
     }
}
	     


void do_room_list( char *arg )
{
   ROOM_DATA *room;
   char buf[256];
   
   if ( !current_area && !arg[0] )
     {
	clientfr( "No current area set." );
	return;
     }
   
   if ( arg[0] )
     {
	AREA_DATA *area = NULL, *a;
	
	for ( a = areas; a; a = a->next )
	  if ( case_strstr( a->name, arg ) )
	    {
	       if ( !area )
		 area = a;
	       else
		 {
		    /* More than one area of that name. */
		    /* List them all, and return. */
		    
		    clientfr( "Multiple matches found:" );
		    for ( area = areas; area; area = area->next )
		      if ( case_strstr( area->name, arg ) )
			{
			   clientff( " - %s\r\n", area->name );
			}
		    
		    return;
		 }
	    }
	
	if ( !area )
	  {
	     clientfr( "No area names match that." );
	     return;
	  }
	
	room = area->rooms;
     }
   else
     room = current_area->rooms;
   
   clientff( C_R "[Rooms in " C_B "%s" C_R "]\r\n" C_0,
	     room->area->name );
   while( room )
     {
	sprintf( buf, " " C_D "(" C_G "%d" C_D ")" C_0 " %s\r\n", room->vnum,
		 room->name ? room->name : "no name" );
	clientf( buf );
	room = room->next_in_area;
     }
}



void do_room_underw( char *arg )
{
   if ( !current_room )
     {
	clientfr( "No current room set." );
	return;
     }
   
   if ( mode != CREATING )
     {
	clientfr( "Turn mapping on, first." );
	return;
     }
   
   current_room->underwater = current_room->underwater ? 0 : 1;
   
   if ( current_room->underwater )
     clientfr( "Current room set as an underwater place." );
   else
     clientfr( "Current room set as normal, with a nice sky (or roof) above." );
}



void do_room_mark( char *arg )
{
   ROOM_DATA *room;
   char buf[256];
   
   if ( arg[0] && isdigit( arg[0] ) )
     {
	if ( !( room = get_room( atoi( arg ) ) ) )
	  {
	     clientfr( "No room with that vnum found." );
	     return;
	  }
     }
   else
     {
	if ( current_room )
	  room = current_room;
	else
	  {
	     clientfr( "No current room set." );
	     return;
	  }
     }
   
   room->landmark = room->landmark ? 0 : 1;
   
   sprintf( buf, "Landmark on '%s' (%d) ", room->name, room->vnum );
   
   if ( room->landmark )
     strcat( buf, "set." );
   else
     strcat( buf, "cleared." );
   
   clientfr( buf );
   
   clientfr( "Using 'map config save' will make it permanent." );
}



void do_room_types( char *arg )
{
   ROOM_TYPE *type;
   
   clientfr( "Room types known:" );
   
   clientff( "   %-25s In Out Swim Underwater\r\n", "Name" );
   for ( type = room_types; type; type = type->next )
     clientff( " - %s%-25s" C_0 " " C_G "%2d" C_0 " " C_G "%3d" C_0
	       "  %s  %s\r\n",
	       type->color, type->name,
	       type->cost_in, type->cost_out,
	       type->must_swim ? C_B "yes" C_0 : C_R " no" C_0,
	       type->underwater ? C_B "yes" C_0 : C_R " no" C_0 );
}




/* Exit commands. */

void do_exit_help( char *arg )
{
   clientfr( "Module: IMapper. Room exit commands:" );
   clientf( " exit link    - Link room # to this one.\r\n"
	    " exit stop    - Stop mapping from this exit.\r\n"
	    " exit length  - Increase the exit length by #.\r\n"
	    " exit map     - Map the next exit elsewhere.\r\n"
	    " exit lock    - Set all unlinked exits in room as locked.\r\n"
	    " exit unilink - Link, but do not create a reverse link.\r\n"
	    " exit destroy - Destroy an exit, and its reverse.\r\n"
	    " exit special - Create/list/modify special exits.\r\n" );
}



void do_exit_length( char *arg )
{
   if ( mode != CREATING )
     {
	clientfr( "Turn mapping on, first." );
	return;
     }
   
   if ( !isdigit( *(arg) ) )
     clientfr( "Specify a length. 0 is normal." );
   else
     {
	set_length_to = atoi( arg );
	
	if ( set_length_to == 0 )
	  set_length_to = -1;
	
	clientfr( "Move in the direction you wish to have this length." );
     }
}



void do_exit_stop( char *arg )
{
   if ( mode != CREATING )
     {
	clientfr( "Turn mapping on, first." );
	return;
     }
   
   clientfr( "Move in the direction you wish to stop (or start again) mapping from." );
   switch_exit_stops_mapping = 1;
}



void do_exit_map( char *arg )
{
   char buf[256];
   int i;
   
   if ( mode != CREATING )
     {
	clientfr( "Turn mapping on, first." );
	return;
     }
   
   use_direction_instead = 0;
   
   for ( i = 1; dir_name[i]; i++ )
     {
	if ( !strcmp( arg, dir_small_name[i] ) )
	  use_direction_instead = i;
     }
   
   if ( !use_direction_instead )
     {
	use_direction_instead = -1;
	clientfr( "Will use default exit, as map position." );
     }
   else
     {
	sprintf( buf, "The room will be mapped '%s' from here, instead.",
		 dir_name[use_direction_instead] );
	clientfr( buf );
     }
}



void do_exit_link( char *arg )
{
   if ( mode != CREATING )
     {
	clientfr( "Turn mapping on, first." );
	return;
     }
   
   if ( !isdigit( *(arg) ) )
     {
	clientfr( "Specify a vnum to link to." );
	return;
     }
   
   link_next_to = get_room( atoi( arg ) );
   
   unidirectional_exit = 0;
   
   if ( !link_next_to )
     clientfr( "Disabled." );
   else
     clientfr( "Move in the direction the room is." );
}



void do_exit_unilink( char *arg )
{
   if ( mode != CREATING )
     {
	clientfr( "Turn mapping on, first." );
	return;
     }
   
   if ( !isdigit( *(arg) ) )
     {
	clientfr( "Specify a vnum to link to." );
	return;
     }
   
   link_next_to = get_room( atoi( arg ) );
   
   unidirectional_exit = 1;
   
   if ( !link_next_to )
     {
	unidirectional_exit = 0;
	clientfr( "Disabled." );
     }
   else
     clientfr( "Move in the direction the room is." );
}



void do_exit_lock( char *arg )
{
   int i, set;
   
   if ( mode != CREATING )
     {
	clientfr( "Turn mapping on, first." );
	return;
     }
   
   if ( !current_room )
     {
	clientfr( "No current room set." );
	return;
     }
   
   if ( arg[0] == 'c' )
     set = 0;
   else
     set = 1;
   
   for ( i = 1; dir_name[i]; i++ )
     {
	if ( current_room->detected_exits[i] &&
	     !current_room->exits[i] )
	  {
	     current_room->locked_exits[i] = set;
	  }
     }
   
   if ( set )
     clientfr( "All unlinked exits in this room have been marked as locked." );
   else
     clientfr( "All locked exits now unlocked." );
}



void do_exit_destroy( char *arg )
{
   char buf[256];
   int dir = 0;
   int i;
   
   if ( mode != CREATING )
     {
	clientfr( "Turn mapping on, first." );
	return;
     }
   
   if ( !current_room )
     {
	clientfr( "No current room set." );
	return;
     }
   
   for ( i = 1; dir_name[i]; i++ )
     {
	if ( !strcmp( arg, dir_name[i] ) )
	  dir = i;
     }
   
   if ( !dir )
     {
	clientfr( "Which link do you wish to destroy?" );
	return;
     }
   
   if ( !current_room->exits[dir] )
     {
	clientfr( "No link in that direction." );
	return;
     }
   
   if ( current_room->exits[dir]->exits[reverse_exit[dir]] != current_room )
     {
	clientfr( "Reverse link was not destroyed." );
     }
   else
     {
	set_reverse( NULL, reverse_exit[dir], current_room );
//	current_room->reverse_exits[reverse_exit[dir]] = NULL;
	current_room->exits[dir]->exits[reverse_exit[dir]] = NULL;
     }
   
   i = current_room->exits[dir]->vnum;
   
   set_reverse( NULL, dir, current_room->exits[dir] );
//   current_room->exits[dir]->reverse_exits[dir] = NULL;
   current_room->exits[dir] = NULL;
   
   sprintf( buf, "Link to vnum %d destroyed.", i );
   clientfr( buf );
}



void do_exit_special( char *arg )
{
   EXIT_DATA *spexit;
   char cmd[256];
   char buf[256];
   int i, nr = 0;
   
   DEBUG( "do_exit_special" );
   
   if ( mode != CREATING && strcmp( arg, "list" ) )
     {
	clientfr( "Turn mapping on, first." );
	return;
     }
   
   if ( !strncmp( arg, "help", strlen( arg ) ) )
     {
	clientfr( "Syntax: exit special <command> [exit] [args]" );
	clientfr( "Commands: list, capture, destroy" );
	clientfr( "Advanced: create, command, message, link, alias" );
	return;
     }
   
   if ( !current_room )
     {
	clientfr( "No current room set." );
	return;
     }
   
   arg = get_string( arg, cmd, 256 );
   
   if ( !strcmp( cmd, "list" ) )
     {
	clientfr( "Special exits:" );
	for ( spexit = current_room->special_exits; spexit; spexit = spexit->next )
	  {
	     sprintf( buf, C_B "%3d" C_0 " - L: " C_G "%d" C_0 " A: '" C_g
		      "%s" C_0 "' C: '" C_g "%s" C_0 "' M: '" C_g "%s" C_0 "'\r\n",
		      nr++, spexit->vnum, spexit->alias ? dir_name[spexit->alias] : "none",
		      spexit->command, spexit->message );
	     clientf( buf );
	  }
	
	if ( !nr )
	  clientf( " - None.\r\n" );
     }
   
   else if ( !strcmp( cmd, "create" ) )
     {
	create_exit( current_room );
	clientfr( "Special exit created." );
     }
   
   else if ( !strcmp( cmd, "destroy" ) )
     {
	int done = 0;
	
	if ( !arg[0] || !isdigit( arg[0] ) )
	  {
	     clientfr( "What special exit do you wish to destroy?" );
	     return;
	  }
	
	nr = atoi( arg );
	
	for ( i = 0, spexit = current_room->special_exits; spexit; spexit = spexit->next )
	  {
	     if ( i++ == nr )
	       {
		  destroy_exit( spexit );
		  done = 1;
		  break;
	       }
	  }
	
	if ( done )
	  sprintf( buf, "Special exit %d destroyed.", nr );
	else
	  sprintf( buf, "Special exit %d was not found.", nr );
	clientfr( buf );
     }
   
   else if ( !strcmp( cmd, "capture" ) )
     {
	arg = get_string( arg, cmd, 256 );
	
	if ( !cmd[0] )
	  {
	     clientfr( "Syntax: Esp capture create" );
	     clientfr( "        Esp capture link <vnum>" );
	     return;
	  }
	
	if ( !strcmp( cmd, "create" ) )
	  {
	     capture_special_exit = -1;
	     
	     clientfr( "Capturing. A room will be created on the other end." );
	     clientfr( "Use 'stop' to disable capturing." );
	  }
	else if ( !strcmp( cmd, "link" ) )
	  {
	     ROOM_DATA *room;
	     int vnum;
	     
	     get_string( arg, cmd, 256 );
	     vnum = atoi( cmd );
	     
	     if ( vnum < 1 )
	       {
		  clientfr( "Specify a valid vnum to link the exit to." );
		  return;
	       }
	     if ( !( room = get_room( vnum ) ) )
	       {
		  clientfr( "No room with that vnum exists!" );
		  return;
	       }
	     
	     capture_special_exit = vnum;
	     
	     clientff( C_R "[Capturing. The exit will be linked to '%s']\r\n" C_0, room->name );
	     clientfr( "Use 'stop' to disable capturing." );
	  }
	
	cse_command[0] = 0;
	cse_message[0] = 0;
	
	return;
     }
   
   else if ( !strcmp( cmd, "message" ) )
     {
	int done = 0;
	
	/* We'll store the number in cmd. */
	arg = get_string( arg, cmd, 256 );
	
	if ( !isdigit( cmd[0] ) )
	  {
	     clientfr( "Set message on what special exit?" );
	     return;
	  }
	
	nr = atoi( cmd );
	
	for ( i = 0, spexit = current_room->special_exits; spexit; spexit = spexit->next )
	  {
	     if ( i++ == nr )
	       {
		  if ( spexit->message )
		    free( spexit->message );
		  
		  spexit->message = strdup( arg );
		  done = 1;
		  break;
	       }
	  }
	
	if ( done )
	  sprintf( buf, "Message on exit %d changed to '%s'", nr, arg );
	else
	  sprintf( buf, "Can't find special exit %d.", nr );
	
	clientfr( buf );
     }
   
   else if ( !strcmp( cmd, "command" ) )
     {
	int done = 0;
	
	/* We'll store the number in cmd. */
	arg = get_string( arg, cmd, 256 );
	
	if ( !isdigit( cmd[0] ) )
	  {
	     clientfr( "Set command on what special exit?" );
	     return;
	  }
	
	nr = atoi( cmd );
	
	for ( i = 0, spexit = current_room->special_exits; spexit; spexit = spexit->next )
	  {
	     if ( i++ == nr )
	       {
		  if ( spexit->command )
		    free( spexit->command );
		  
		  spexit->command = strdup( arg );
		  done = 1;
		  break;
	       }
	  }
	
	if ( done )
	  sprintf( buf, "Command on exit %d changed to '%s'", nr, arg );
	else
	  sprintf( buf, "Can't find special exit %d.", nr );
	
	clientfr( buf );
     }
   
   else if ( !strcmp( cmd, "link" ) )
     {
	/* E special link 0 -1 */
	char vnum[256];
	
	/* We'll store the number in cmd. */
	arg = get_string( arg, cmd, 256 );
	
	if ( !isdigit( cmd[0] ) )
	  {
	     clientfr( "Link which special exit?" );
	     return;
	  }
	
	nr = atoi( cmd );
	
	for ( i = 0, spexit = current_room->special_exits; spexit; spexit = spexit->next )
	  {
	     if ( i++ == nr )
	       {
		  get_string( arg, vnum, 256 );
		  
		  if ( vnum[0] == '-' )
		    {
		       ROOM_DATA *to;
		       
		       to = spexit->to;
		       
		       spexit->vnum = -1;
		       spexit->to = NULL;
		       
		       check_pointed_by( to );
		       
		       sprintf( buf, "Link cleared on exit %d.", nr );
		       clientfr( buf );
		       return;
		    }
		  else if ( isdigit( vnum[0] ) )
		    {
		       spexit->vnum = atoi( vnum );
		       spexit->to = get_room( spexit->vnum );
		       if ( !spexit->to )
			 {
			    clientfr( "A room whith that vnum was not found." );
			    spexit->vnum = -1;
			    return;
			 }
		       spexit->to->pointed_by = 1;
		       sprintf( buf, "Special exit %d linked to '%s'.",
				nr, spexit->to->name );
		       clientfr( buf );
		       return;
		    }
		  else
		    {
		       clientfr( "Link to which vnum?" );
		       return;
		    }
	       }
	  }
	
	sprintf( buf, "Can't find special exit %d.", nr );
	clientfr( buf );
     }
   
   else if ( !strcmp( cmd, "alias" ) )
     {
	/* E special alias 0 down */
	
	if ( !arg[0] || !isdigit( arg[0] ) )
	  {
	     clientfr( "What special exit do you wish to destroy?" );
	     return;
	  }
	
	nr = atoi( arg );
	
	for ( i = 0, spexit = current_room->special_exits; spexit; spexit = spexit->next )
	  {
	     if ( i++ == nr )
	       {
		  arg = get_string( arg, cmd, 256 );
		  
		  if ( !strcmp( arg, "none" ) )
		    {
		       sprintf( buf, "Alias on exit %d cleared.", nr );
		       clientfr( buf );
		       return;
		    }
		  
		  for ( i = 1; dir_name[i]; i++ )
		    {
		       if ( !strcmp( arg, dir_name[i] ) )
			 {
			    sprintf( buf, "Going %s will now trigger exit %d.",
				     dir_name[i], nr );
			    clientfr( buf );
			    spexit->alias = i;
			    return;
			 }
		    }
		  
		  clientfr( "Use an exit, such as 'north', 'up', 'none', etc." );
		  return;
	       }
	  }
	
	sprintf( buf, "Special exit %d was not found.", nr );
	clientfr( buf );
     }
   
   else
     {
	clientfr( "Unknown command... Try 'exit special help'." );
     }
}




/* Normal commands. */

void do_landmarks( char *arg )
{
   AREA_DATA *area;
   ROOM_DATA *room;
   char buf[256];
   int first;
   
   clientfr( "Landmarks throughout the world:" );
   
   for ( area = areas; area; area = area->next )
     {
	first = 1;
	
	for ( room = area->rooms; room; room = room->next_in_area )
	  {
	     if ( !room->landmark )
	       continue;
	     
	     /* And now... */
	     
	     /* Area name, if first room in the area's list. */
	     if ( first )
	       {
		  sprintf( buf, "\r\n" C_B "%s" C_0 "\r\n", area->name );
		  clientf( buf );
		  first = 0;
	       }
	     
	     sprintf( buf, C_D " (" C_G "%d" C_D ") " C_0 "%s\r\n",
		      room->vnum, room->name );
	     clientf( buf );
	  }
     }
   
   clientf( "\r\n" );
}



void do_go( char *arg )
{
   if ( q_top )
     {
	clientfr( "Command queue isn't empty, clear it first." );
	return;
     }
   
   if ( !strcmp( arg, "dash" ) )
     dash_command = "dash ";
   else if ( !strcmp( arg, "sprint" ) )
     dash_command = "sprint ";
   else
     dash_command = NULL;
   
   go_next( );
}



void do_mhelp( char *arg )
{
   if ( !strcmp( arg, "index" ) )
     {
	clientf( C_C "MAPPER HELP" C_0 "\r\n\r\n" );
	
	clientf( "- Index\r\n"
		 "- GettingStarted\r\n"
		 "- Commands\r\n" );
     }
   
   else if ( !strcmp( arg, "gettingstarted" ) )
     {
	clientf( C_C "MAPPER HELP" C_0 " - Getting started.\r\n\r\n"
		 "Greetings.  If you're reading this, then you obviously have a new\r\n"
		 "mapper to play with.\r\n"
		 "\r\n"
		 "First thing you should do, is to check if it's working.  Do so with a\r\n"
		 "simple 'look' command, in a common room.  If the area name is not shown\r\n"
		 "after the room title, then you must set the room title color.  Do so\r\n"
		 "with 'map color'.  When the color matches the one set on Imperian, and\r\n"
		 "the room you're currently in is mapped, all should work.\r\n"
		 "\r\n"
		 "If the area name was shown, that means it now knows where it is, so\r\n"
		 "the 'map' command is available.\r\n"
		 "\r\n"
		 "You will then most likely want to make a path to somewhere, and walk\r\n"
		 "to it.  Do so by choosing a room to go to, and find its vnum (room's\r\n"
		 "virtual number).  As an example, if you'd want to go to the Shuk, in\r\n"
		 "Antioch, find its vnum with 'room find shuk'.  In this case, the vnum\r\n"
		 "is 605, so a path to it would be made by 'map path 605'.  Once the\r\n"
		 "path is built, use 'go' to start auto-walking, and 'stop' in the\r\n"
		 "middle of an auto-walking to stop it.\r\n"
		 "\r\n"
		 "These are the basics of showing a map, and going somewhere.\r\n" );
     }
   
   else if ( !strcmp( arg, "commands" ) )
     {
	clientf( C_C "MAPPER HELP" C_0 " - Commands.\r\n\r\n"
		 "Basics\r\n"
		 "------\r\n"
		 "\r\n"
		 "The mapper has four base commands, each having some subcommands.  The\r\n"
		 "base commands are 'map', 'area', 'room' and 'exit'.  Each base\r\n"
		 "command has the subcommand 'help', which lists all other subcommands.\r\n"
		 "An example would be 'map help'.\r\n"
		 "\r\n"
		 "Abbreviating\r\n"
		 "------------\r\n"
		 "\r\n"
		 "As an example, use 'area list'.  Base command is 'area', subcommand is\r\n"
		 "'list'.  The subcommand may be abbreviated to any number of letters,\r\n"
		 "so 'area lis', 'area li' and 'area l' all work.  The base command may\r\n"
		 "be abbreviated only to the first letter, 'a l'.  Also, as typing a\r\n"
		 "double space ('m p 605') is sometimes hard, the base command letter\r\n"
		 "may be used as uppercase, with no space after it.  As an example,\r\n"
		 "'Mp 605' instead of 'map path 605' or 'm p 605' would work.\r\n" );
     }
   
   else
     clientf( "Use 'mhelp index', for a list of mhelp files.\r\n" );
}




#define CMD_NONE	0
#define CMD_MAP		1
#define CMD_AREA	2
#define CMD_ROOM	3
#define CMD_EXIT	4

FUNC_DATA cmd_table[] =
{
   /* Map commands. */
     { "help",		do_map_help,	CMD_MAP },
     { "remake",	do_map_remake,	CMD_MAP },
     { "create",	do_map_create,	CMD_MAP },
     { "follow",	do_map_follow,	CMD_MAP },
     { "none",		do_map_none,	CMD_MAP },
     { "save",		do_map_save,	CMD_MAP },
     { "load",		do_map_load,	CMD_MAP },
     { "path",		do_map_path,	CMD_MAP },
     { "status",	do_map_status,	CMD_MAP },
     { "color",		do_map_color,	CMD_MAP },
     { "colour",	do_map_color,	CMD_MAP },
     { "bump",		do_map_bump,	CMD_MAP },
     { "old",		do_map_old,	CMD_MAP },
     { "file",		do_map_file,	CMD_MAP },
     { "teleport",	do_map_teleport,CMD_MAP },
     { "queue",		do_map_queue,	CMD_MAP },
     { "config",	do_map_config,	CMD_MAP },
   
   /* Area commands. */
     { "help",		do_area_help,	CMD_AREA },
     { "create",	do_area_create,	CMD_AREA },
     { "list",		do_area_list,	CMD_AREA },
     { "switch",	do_area_switch,	CMD_AREA },
     { "update",	do_area_update,	CMD_AREA },
     { "info",		do_area_info,	CMD_AREA },
     { "off",		do_area_off,	CMD_AREA },
     { "orig",		do_area_orig,	CMD_AREA },
     { "orig",		do_map_orig,	CMD_MAP },
   
   /* Room commands. */
     { "help",		do_room_help,	CMD_ROOM },
     { "switch",	do_room_switch,	CMD_ROOM },
     { "create",	do_room_create,	CMD_ROOM },
     { "info",		do_room_info,	CMD_ROOM },
     { "find",		do_room_find,	CMD_ROOM },
     { "look",		do_room_look,	CMD_ROOM },
     { "destroy",	do_room_destroy,CMD_ROOM },
     { "list",		do_room_list,	CMD_ROOM },
     { "underwater",	do_room_underw, CMD_ROOM },
     { "mark",		do_room_mark,	CMD_ROOM },
     { "types",		do_room_types,	CMD_ROOM },
   
   /* Exit commands. */
     { "help",		do_exit_help,	CMD_EXIT },
     { "length",	do_exit_length,	CMD_EXIT },
     { "stop",		do_exit_stop,	CMD_EXIT },
     { "map",		do_exit_map,	CMD_EXIT },
     { "link",		do_exit_link,	CMD_EXIT },
     { "unilink",	do_exit_unilink,CMD_EXIT },
     { "lock",		do_exit_lock,	CMD_EXIT },
     { "destroy",	do_exit_destroy,CMD_EXIT },
     { "special",	do_exit_special,CMD_EXIT },
   
   /* Normal commands. */
     { "map",		do_map,		CMD_NONE },
     { "landmarks",	do_landmarks,	CMD_NONE },
     { "mhelp",		do_mhelp,	CMD_NONE },
     { "go",		do_go,		CMD_NONE },
   
     { NULL, NULL, 0 }
};


int i_mapper_process_client_aliases( char *line )
{
   int i;
   char command[4096], *l;
   int base_cmd;
   
   DEBUG( "i_mapper_process_client_aliases" );
   
   /* Room commands queue. */
   if ( !strcmp( line, "l" ) ||
	!strcmp( line, "look" ) ||
	!strcmp( line, "ql" ) )
     {
	/* Add -1 to queue. */
	if ( mode == CREATING || mode == FOLLOWING )
	  add_queue( -1 );
	
	return 0;
     }
   
   /* Accept swimming. */
   if ( !strncmp( line, "swim ", 5 ) )
     line += 5;
   
   /* Same with Saboteur/Idrasi evading. */
   if ( !strncmp( line, "evade ", 6 ) )
     line += 6;
   
   for ( i = 1; dir_name[i]; i++ )
     {
	if ( !strcmp( line, dir_name[i] ) ||
	     !strcmp( line, dir_small_name[i] ) )
	  {
	     if ( mode == FOLLOWING && current_room && current_room->special_exits )
	       {
		  EXIT_DATA *spexit;
		  
		  for ( spexit = current_room->special_exits; spexit; spexit = spexit->next )
		    {
		       if ( spexit->alias && spexit->command && spexit->alias == i )
			 {
			    send_to_server( spexit->command );
			    send_to_server( "\r\n" );
			    return 1;
			 }
		    }
	       }
	     
	     if ( mode == CREATING || mode == FOLLOWING )
	       add_queue( i );
	     return 0;
	  }
     }
   
   if ( !strcmp( line, "stop" ) )
     {
	if ( auto_walk != 0 )
	  {
	     clientfr( "Okay, I'm frozen." );
	     auto_walk = 0;
	     return 1;
	  }
	else if ( capture_special_exit )
	  {
	     clientfr( "Capturing disabled." );
	     show_prompt( );
	     capture_special_exit = 0;
	     return 1;
	  }
     }
   
   /* Process from command table. */
   
   l = line;
   
   /* First, check for compound commands. */
   if ( !strncmp( l, "room ", 5 ) )
     base_cmd = CMD_ROOM, l += 5;
   else if ( !strncmp( l, "r ", 2 ) )
     base_cmd = CMD_ROOM, l += 2;
   else if ( !strncmp( l, "R", 1 ) )
     base_cmd = CMD_ROOM, l += 1;
   else if ( !strncmp( l, "area ", 5 ) )
     base_cmd = CMD_AREA, l += 5;
   else if ( !strncmp( l, "a ", 2 ) )
     base_cmd = CMD_AREA, l += 2;
   else if ( !strncmp( l, "A", 1 ) )
     base_cmd = CMD_AREA, l += 1;
   else if ( !strncmp( l, "map ", 4 ) )
     base_cmd = CMD_MAP, l += 4;
   else if ( !strncmp( l, "m ", 2 ) )
     base_cmd = CMD_MAP, l += 2;
   else if ( !strncmp( l, "M", 1 ) )
     base_cmd = CMD_MAP, l += 1;
   else if ( !strncmp( l, "exit ", 5 ) )
     base_cmd = CMD_EXIT, l += 5;
   else if ( !strncmp( l, "ex ", 3 ) )
     base_cmd = CMD_EXIT, l += 3;
   else if ( !strncmp( l, "E", 1 ) )
     base_cmd = CMD_EXIT, l += 1;
   else
     base_cmd = CMD_NONE;
   
   l = get_string( l, command, 4096 );
   
   if ( !command[0] )
     return 0;
   
   for ( i = 0; cmd_table[i].name; i++ )
     {
	/* If a normal command, compare it full.
	 * If after a base command, it's okay to abbreviate it.. */
	if ( base_cmd == cmd_table[i].base_cmd &&
	     ( ( base_cmd != CMD_NONE && !strncmp( command, cmd_table[i].name, strlen( command ) ) ) ||
	       ( base_cmd == CMD_NONE && !strcmp( command, cmd_table[i].name ) ) ) )
	  {
	     (cmd_table[i].func)( l );
	     show_prompt( );
	     return 1;
	  }
     }
   
   /* A little shortcut to 'exit link'. */
   if ( base_cmd == CMD_EXIT && isdigit( command[0] ) )
     {
	do_exit_link( command );
	show_prompt( );
	return 1;
     }
   if ( base_cmd == CMD_MAP && isdigit( command[0] ) )
     {
	do_map( command );
	show_prompt( );
	return 1;
     }
   
   if ( capture_special_exit )
     {
	strcpy( cse_command, line );
	clientff( C_R "[Command changed to '%s'.]\r\n" C_0, cse_command );
     }
   
   return 0;
}

