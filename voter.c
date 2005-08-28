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


#define VOTER_ID "$Name$ $Id$"

#include "module.h"


int voter_version_major = 0;
int voter_version_minor = 5;

char *voter_id = VOTER_ID "\r\n" HEADER_ID "\r\n" MODULE_ID "\r\n";

/* Globals. */

DESCRIPTOR *connection_desc;

#define MAX_BODY 4096*16

char url[1024];

int available_to_analyze;
char response[MAX_BODY];
int response_bytes;

char proxy_host[256];
int proxy_port;

char post_string[256];
int post_method;

int auto_vote;

typedef struct cookie_data COOKIE;

struct cookie_data
{
   char *name;
   char *content;
   
   COOKIE *next;
};

COOKIE *cookies;

/* Functions. */

int voter_process_commands( char *line );
void voter_connection_exception( DESCRIPTOR *desc );
void voter_process_connection( DESCRIPTOR *desc );



ENTRANCE( voter_module_register )
{
   self->name = strdup( "Voter" );
   self->version_major = voter_version_major;
   self->version_minor = voter_version_minor;
   self->id = voter_id;
   
   self->init_data = NULL;
   self->process_server_line_prefix = NULL;
   self->process_server_line_suffix = NULL;
   self->process_server_prompt_informative = NULL;
   self->process_server_prompt_action = NULL;
   self->process_client_command = NULL;
   self->process_client_aliases = voter_process_commands;
   self->build_custom_prompt = NULL;
   self->main_loop = NULL;
   self->update_descriptors = NULL;
   
   GET_FUNCTIONS( self );
}




void make_cookies( char *dest )
{
   COOKIE *cookie;
   char buf[1024];
   
   dest[0] = 0;
   
   for ( cookie = cookies; cookie; cookie = cookie->next )
     {
	sprintf( buf, "Cookie: %s=%s\n", cookie->name, cookie->content );
	strcat( dest, buf );
     }
}



DESCRIPTOR *voter_connect( char *url )
{
   DESCRIPTOR *desc;
   char buf[256], host[256], *p, *b;
   int sock, port = 0;
   
   clientff( C_R "[%s '%s'.]\r\n" C_0, post_method ? "Posting on" : "Trying",
	     url );
   
   /* 
    * URL: http://host:port/addr?post
    * :port and ?post are optional.
    */
   
   if ( strncmp( url, "http://", 7 ) )
     {
	if ( strstr( url, "://" ) )
	  {
	     clientfr( "Protocol not supported." );
	     return NULL;
	  }
	
	strcpy( buf, "http://" );
	strcat( buf, url );
	strcpy( url, buf );
     }
   
   /* Get the host name, from the URL. */
   p = url + 7, b = host;
   while ( *p && *p != '/' && *p != ':' )
     *(b++) = *(p++);
   *b = 0;
   
   /* Get the port, if there's one. */
   if ( *p == ':' )
     {
	b = buf;
	p++;
	while ( *p >= '0' && *p <= '9' )
	  *(b++) = *(p++);
	*b = 0;
	
	port = atoi( buf );
     }
   
   if ( *p != '/' )
     {
	if ( !*p )
	  p = "/";
	else
	  {
	     clientfr( "Unable to understand the URL." );
	     return NULL;
	  }
     }
   
   if ( !port )
     port = 80;
   
   if ( !proxy_host[0] )
     {
	sock = mb_connect( host, port );
     }
   else
     {
	clientff( C_R "[Using proxy %s:%d.]\r\n" C_0, proxy_host, proxy_port );
	sock = mb_connect( proxy_host, proxy_port );
     }
   
   if ( sock < 0 )
     {
	sprintf( buf, "Failed: %s.", get_connect_error( ) );
	clientfr( buf );
	return NULL;
     }
   
   /* Put it in MudBot's connection list. */
   desc = calloc( 1, sizeof( DESCRIPTOR ) );
   desc->name = strdup( "Voter" );
   sprintf( buf, "URL: '%s'", url );
   desc->description = strdup( buf );
   desc->mod = NULL;
   desc->fd = sock;
   desc->callback_in = voter_process_connection;
   desc->callback_exc = voter_connection_exception;
   add_descriptor( desc );
   connection_desc = desc;
   
   /* Send the HTTP request over the connection. */
   
   if ( post_method )
     {
	/* POST method. */
	
	char cookie_pot[4096];
	
//	make_cookies( cookie_pot );
	cookie_pot[0] = 0;
	
	sprintf( buf, "POST %s HTTP/1.0\n"
		 "Host: %s\n"
		 "%s"
		 "Content-Type: application/x-www-form-urlencoded\n"
		 "Content-Length: %d\n\n%s",
		 p, host, cookie_pot, (int) strlen( post_string ), post_string );
//	debugf( "[%s]", buf );
	c_write( sock, buf, strlen( buf ) );
     }
   else
     {
	/* GET method. */
	
	sprintf( buf, "GET %s HTTP/1.0\nHost: %s\n\n", p, host );
	c_write( sock, buf, strlen( buf ) );
     }
   
   /* Await response. */
   
   response[0] = 0;
   response_bytes = 0;
   available_to_analyze = 0;
   
   clientfr( "Connected. Request sent, awaiting response." );
   
   return desc;
}



void voter_close_connection( )
{
   if ( connection_desc )
     {
	c_close( connection_desc->fd );
	remove_descriptor( connection_desc );
	connection_desc = NULL;
     }
}


char *get_line( char *src, char *dest )
{
   char *s = src, *d = dest;
   
   while ( *s && *s != '\n' && *s != '\r' )
     {
	*(d++) = *(s++);
     }
   
   *d = 0;
   
   if ( *s == '\r' )
     {
	s++;
	if ( *s == '\n' )
	  s++;
     }
   
   else if ( *s == '\n' )
     {
	s++;
	if ( *s == '\r' )
	  s++;
     }
   
   return s;
}


/* This would be soooo much easier with RegEx :( */
void get_info_from_content( char *string )
{
   char *p, *s, *s2 = NULL;
   int up = 0, down = 0;
   int in_tag = 0;
   int place = -1, in = -1, out = -1;
   
   /* Find the beginning of our section. */
   
   p = strstr( string, "<img src=\"/images/imperian_icon.gif\">" );
   
   if ( !p )
     return;
   
   s = string;
   
   while ( s < p )
     {
	s2 = s;
	s = strstr( s2, "<tr" );
	if ( !s )
	  return;
	s++;
     }
   
   s = s2;
   
   s2 = strstr( s, "</tr>" );
   *s2 = 0;
   
   /* We now have our section, in which to scan for things. */
   
   up = strstr( s, "<img src=\"http://www.topmudsites.com/images/up.gif\">" ) ? 1 : 0;
   down = strstr( s, "<img src=\"http://www.topmudsites.com/images/down.gif\">" ) ? 1 : 0;
   
   /* Look for numbers. We need numbers, that are out of html tags. */
   
   p = s;
   
   while ( *p )
     {
	if ( in_tag )
	  {
	     if ( *p == '>' )
	       in_tag = 0;
	     p++;
	     continue;
	  }
	
	if ( *p == '<' )
	  {
	     in_tag = 1;
	     p++;
	     continue;
	  }
	
	/* Not quite within a html tag, but ignore it anyway. */
	if ( *p < '0' || *p > '9' )
	  {
	     in_tag = 1;
	     p++;
	     continue;
	  }
	
	if ( place < 0 )
	  {
	     place = atoi( p );
	     in_tag = 1;
	  }
	
	else if ( in < 0 )
	  {
	     in = atoi( p );
	     in_tag = 1;
	  }
	else if ( out < 0 )
	  {
	     out = atoi( p );
	     in_tag = 1;
	  }
	
	p++;
     }
   
   if ( place >= 0 && in >= 0 )
     clientff( C_R "[Imperian: " C_W "%d" C_R "%s place%s. Votes: "
	       C_W "%d" C_R ".]\r\n" C_0, place,
	       place % 10 == 1 ? "st" : place % 10 == 2 ? "nd" : place % 10 == 3 ? "rd" : "th",
	       up ? " (and rising)" : ( down ? " (and falling)" : "" ), in );
   else
     clientfr( "Weird..." );
}


void add_cookie( char *string )
{
   COOKIE *cookie;
   char name[256];
   char content[1024];
   char *p, *d;
   
   /* name=content;otherdata */
   
   /* Get the name. */
   p = string;
   d = name;
   
   while ( *p != '=' && *p )
     *(d++) = *(p++);
   *d = 0;
   
   if ( !name[0] )
     return;
   
   /* Look it up around our cookies. */
   for ( cookie = cookies; cookie; cookie = cookie->next )
     if ( !strcmp( cookie->name, name ) )
       break;
   
   /* If it doesn't exist, create one. */
   if ( !cookie )
     {
	cookie = calloc( 1, sizeof( COOKIE ) );
	cookie->name = strdup( name );
	
	cookie->next = cookies;
	cookies = cookie;
     }
   
   /* Get the content. */
   d = content;
   if ( *p == '=' )
     p++;
   while ( *p != ';' && *p )
     *(d++) = *(p++);
   *d = 0;
   
   if ( cookie->content )
     free( cookie->content );
   cookie->content = strdup( content );
}


void analyze_content( )
{
   char *s = response;
   char line[MAX_BODY];
   
   int moved = 0;
   
//   clientfr( "Analyzing header..." );
   
   while ( *s )
     {
	s = get_line( s, line );
	
	if ( !strncmp( line, "HTTP/", 5 ) )
	  {
	     /* HTTP/1.0 302 Moved Temporarily */
	     char *p;
	     
	     char code[256];
	     int code_nr;
	     
	     p = get_string( line, code, 256 ); /* HTTP/1.0 */
	     p = get_string( p, code, 256 ); /* 302 */
	     code_nr = atoi( code );
	     
	     if ( code_nr == 404 )
	       {
		  clientfr( "The page does not exist on the site." );
		  url[0] = 0;
		  return;
	       }
	     else if ( code_nr == 302 )
	       {
//		  clientfr( "The page is found somewhere else." );
		  moved = 1;
	       }
	     else
	       {
//		  clientff( C_R "[Code %d: %s]\r\n" C_0, code_nr, p );
	       }
	  }
	else if ( !strncmp( line, "Location: ", 10 ) )
	  {
	     if ( moved )
	       {
		  strcpy( url, line + 10 );
		  
//		  clientff( C_R "[URL set to: '%s'. Try 'vote' again.]\r\n" C_0,
//			    url );
		  
		  if ( auto_vote )
		    auto_vote = 2;
	       }
	  }
	else if ( !strncmp( line, "Set-Cookie: ", 12 ) )
	  {
	     add_cookie( line + 12 );
	  }
	else if ( !line[0] )
	  {
//	     clientfr( "End of header." );
	     break;
	  }
     }
   
//   clientfr( "Analyzing contents..." );
   
   if ( ( strstr( s, "<form name=\"click\" action=\"http://www.topmudsites.com/cgi-bin/topmuds/rankem.cgi\" method=\"POST\">" ) ) )
     {
	int ses;
	
	s = strstr( s, "name=\"ses\" value=" );
	if ( !s )
	  return;
	
	sscanf( s, "name=\"ses\" value=\"%d\"", &ses );
	
	s = strstr( s, "name=\"id\" value=\"avasyu\"" );
	if ( !s )
	  return;
	
//	clientff( C_R "[Got the session number! Ready to 'post'.]\r\n" C_0 );
	
	strcpy( url, "http://www.topmudsites.com/cgi-bin/topmuds/rankem.cgi" );
	sprintf( post_string, "ses=%d&id=avasyu", ses );
	
	if ( auto_vote )
	  auto_vote = 3;
     }
   
   if ( strstr( s, "<a href=\"http://www.topmudsites.com/cgi-bin/topmuds/out.cgi"
		  "?id=avasyu&url=http%3a%2f%2fwww.imperian.com\" class=\"blue\" target=\"_blank\">"
		  "Imperian</a></font>" ) )
     {
	get_info_from_content( s );
     }
   
   if ( auto_vote == 2 )
     {
	auto_vote = 1;
	voter_connect( url );
     }
   else if ( auto_vote == 3 )
     {
	auto_vote = 1;
	
	post_method = 1;
	if ( url[0] && post_string[0] )
	  voter_connect( url );
	post_method = 0;
     }
   else
     clientfr( "All done." );
   show_prompt( );
}



void voter_process_connection( DESCRIPTOR *desc )
{
   char buf[MAX_BODY];
   int bytes;
   
   bytes = c_read( desc->fd, buf, 4096 );
   
   if ( bytes == 0 )
     {
	clientf( "\r\n" );
	clientfr( "Connection closed." );
	available_to_analyze = 1;
	response[response_bytes] = 0;
	voter_close_connection( );
	
	if ( auto_vote )
	  {
	     analyze_content( );
	  }
	else
	  show_prompt( );
     }
   else if ( bytes < 0 )
     {
	clientf( "\r\n" );
	clientfr( "Connection error." );
	show_prompt( );
	voter_close_connection( );
     }
   else
     {
//	clientff( C_R "[Received %d bytes.]\r\n", bytes );
	
	if ( response_bytes + bytes > MAX_BODY )
	  {
	     clientf( "\r\n" );
	     clientfr( "The response is too big for me. Closing connection." );
	     show_prompt( );
	     
	     voter_close_connection( );
	  }
	else
	  {
	     memcpy( &response[response_bytes], buf, bytes );
	     response_bytes += bytes;
	  }
     }
}


void voter_connection_exception( DESCRIPTOR *desc )
{
   clientf( "\r\n" );
   clientfr( "Connection error (exception raised on socket)." );
   show_prompt( );
   
   voter_close_connection( );
}


void do_vote( char *args )
{
   int post = 0, noauto = 0;
   
   strcpy( url, "" );
   strcpy( proxy_host, "" );
   
   while ( *args )
     {
	char arg[256];
	
	args = get_string( args, arg, 256 );
	
	if ( !strcmp( arg, "url" ) )
	  {
	     args = get_string( args, url, 1024 );
	  }
	else if ( !strcmp( arg, "proxy" ) )
	  {
	     char port[256];
	     args = get_string( args, proxy_host, 256 );
	     args = get_string( args, port, 256 );
	     proxy_port = atoi( port );
	  }
	else if ( !strcmp( arg, "post" ) )
	  {
	     if ( !post_string[0] )
	       {
		  clientfr( "Nothing to post." );
		  return;
	       }
	     if ( !url[0] )
	       {
		  clientfr( "No URL." );
		  return;
	       }
	     
	     post = 1;
	  }
	else if ( !strcmp( arg, "noauto" ) )
	  {
	     noauto = 1;
	  }
	
	else if ( !strcmp( arg, "analyze" ) )
	  {
	     if ( !available_to_analyze )
	       clientfr( "Unfortunately there is nothing to be analyzed." );
	     else
	       {
		  clientfr( "Showing contents:" );
		  
		  clientf( response );
		  clientf( "\r\n" );
		  
		  analyze_content( );
	       }
	     
	     return;
	  }
	else if ( !strcmp( arg, "disconnect" ) )
	  {
	     if ( connection_desc )
	       {
		  voter_close_connection( );
		  clientfr( "Connection closed." );
	       }
	     else
	       clientfr( "Not connected anywhere." );
	     
	     return;
	  }
	else if ( !strcmp( arg, "cookies" ) )
	  {
	     COOKIE *cookie;
	     
	     if ( !cookies )
	       {
		  clientfr( "I have no cookies for you. Sorry." );
	       }
	     else
	       {
		  clientfr( "Cookies:" );
		  for ( cookie = cookies; cookie; cookie = cookie->next )
		    {
		       clientff( " - " C_W "%s" C_0 ": %s\r\n",
				 cookie->name, cookie->content );
		    }
	       }
	     
	     return;
	  }
	else if ( !strcmp( arg, "help" ) )
	  {
	     clientfr( "Voter Help" );
	     
	     clientf( "\r\n"
		      " " C_0 "Syntax: " C_W "vote" C_R " [" C_W "options" C_R "]\r\n"
		      "\r\n"
		      "A simple 'vote' command will attempt to connect to the default\r\n"
		      "site, and follow any links/redirections for you.\r\n"
		      "For a list of all possible options, use 'vote extrahelp'.\r\n" C_0 );
	     
	     return;
	  }
	
	else if ( !strcmp( arg, "extrahelp" ) )
	  {
	     clientfr( "Voter Help" );
	     
	     clientf( "\r\n"
		      " " C_0 "Possible options:\r\n"
		      "    " C_W "url" C_R " <" C_W "url" C_R ">           - Connect to this URL instead.\r\n"
		      "    " C_W "proxy" C_R " <" C_W "host" C_R "> <" C_W "port" C_R"> - Use a proxy for all connections.\r\n"
		      "    " C_W "post" C_R "                - Post the data analyzed to the specified URL.\r\n"
		      "    " C_W "noauto" C_R "              - Do not automatically follow redirections.\r\n"
		      "    " C_W "analyze" C_R "             - Analyze the header/page received.\r\n"
		      "    " C_W "disconnect" C_R "          - Disconnect from the site, if already connected.\r\n"
		      "    " C_W "cookies" C_R "             - List all gathered cookies.\r\n"
		      "\r\n"
		      " Example:\r\n"
		      "  " C_W "vote url http://www.topmudsites.com/ proxy 10.0.0.1 8080" C_R "\r\n"
		      "  (this will only check how many votes there are, over a proxy)\r\n"
		      "\r\n"
		      "The module will only follow links (and vote) for Imperian.\r\n"
		      "The main connection will be frozen while attempting to connect\r\n"
		      "somewhere, so be careful when you specify another url/proxy.\r\n" C_0 );
	     return;
	  }
     }
   
   if ( connection_desc )
     {
	clientfr( "Wait until a disconnect!" );
	return;
     }
   
   if ( !url[0] )
     strcpy( url, "http://www.ironrealms.com/irex/tmsvote.php?id=imperian" );
   
   if ( noauto )
     auto_vote = 0;
   else
     auto_vote = 1;
   
   if ( post )
     post_method = 1;
   
   voter_connect( url );
   
   post_method = 0;
}


int voter_process_commands( char *line )
{
   DEBUG( "voter_process_commands" );
   
   if ( !strcmp( line, "vote" ) )
     {
	do_vote( "" );
	
	show_prompt( );
	return 1;
     }
   
   else if ( !strncmp( line, "vote ", 5 ) )
     {
	do_vote( line + 5 );
	
	show_prompt( );
	return 1;
     }
   
   return 0;
}

