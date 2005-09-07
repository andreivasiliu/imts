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


/* MudMaster CHAT Protocol. (client-to-client communication) */

#define MMCHAT_ID "$Name$ $Id$"

#include <sys/stat.h>	/* For fstat() */


#include "module.h"


int mmchat_version_major = 0;
int mmchat_version_minor = 5;

char *mmchat_id = MMCHAT_ID "\r\n" HEADER_ID "\r\n" MODULE_ID "\r\n";


MODULE *mm_self;

char *chat_name = "Whyte";
char *chat_ip = "127.0.0.1";
int chat_port = 4050;

char *proxy_host = "stipo.ro";
int proxy_port = 8080;

/* Taken directly from the MudMaster protocol specifications file. */

/*
Mud Master Chat

Establishing a Connection
Caller: Once a connection is made the caller sends a connection string; which looks like: "CHAT:<chat name>\n<ip address><port>".   The sprintf syntax is: "CHAT:%s\n%s%-5u".  The port must be 5 characters, padded on the right side with spaces.  Once this string has been sent it waits for a response from the other side.  If a "NO" is received the call is cancelled.  If the call was accepted the string "YES:<chat name>\n" is received.  Next the MM version number is sent out.  The version number sending is not required and can happen independently of the connection.  It's just a convenient place to do it. 

Receiver:  When a socket call is detected it accepts the socket then waits for the "CHAT:" string to be send from the caller.  If the receiver wishes to deny the call, the string "NO" needs to be sent back to the caller.  To accept the call, the string "YES:<chat name>\n" is sent back.  That's all the receiver has to do.  Next the MM version number is sent out.  This is not required.

Chat Data Blocks
A chat data block looks like this:  <block ID byte><data><end of data byte>.  All data dealing with needs to follow this format with a couple exceptions.  The connection process doesn't use the data blocks and the file transfer blocks are a fixed size and don't need the <end of data> byte.
Below is a list of the <block ID> values:
 */

#define CHAT_NAME_CHANGE		1
#define CHAT_REQUEST_CONNECTIONS	2
#define CHAT_CONNECTION_LIST		3
#define CHAT_TEXT_EVERYBODY		4
#define CHAT_TEXT_PERSONAL		5
#define CHAT_TEXT_GROUP			6
#define CHAT_MESSAGE			7
#define CHAT_DO_NOT_DISTURB		8
#define CHAT_SEND_ACTION		9
#define CHAT_SEND_ALIAS			10
#define CHAT_SEND_MACRO			11
#define CHAT_SEND_VARIABLE		12
#define CHAT_SEND_EVENT			13
#define CHAT_SEND_GAG			14
#define CHAT_SEND_HIGHLIGHT		15
#define CHAT_SEND_LIST			16
#define CHAT_SEND_ARRAY			17
#define CHAT_SEND_BARITEM		18
#define CHAT_VERSION			19
#define CHAT_FILE_START			20
#define CHAT_FILE_DENY			21
#define CHAT_FILE_BLOCK_REQUEST		22
#define CHAT_FILE_BLOCK			23
#define CHAT_FILE_END			24
#define CHAT_FILE_CANCEL		25
#define CHAT_PING_REQUEST		26
#define CHAT_PING_RESPONSE		27

#define CHAT_PEEK_CONNECTIONS		28
#define CHAT_PEEK_LIST			29
#define CHAT_SNOOP			30
#define CHAT_SNOOP_DATA			31


/*
The <end of data> byte is 255:
 */

#define CHAT_END_OF_COMMAND		255

/*
CHAT_NAME_CHANGE
When a user changes their chat name the new name needs to be broadcast to all of their connections.  The data block looks like: "<CHAT_NAME_CHANGE><new name><CHAT_END_OF_COMMAND>" – "%c%s%c".

CHAT_REQUEST_CONNECTIONS
Sender: Requesting connections from another connection asks to see all the people that person has marked as public, then try to connect to all of those yourself.  The request for connections is just "<CHAT_REQUEST_CONNECTIONS><CHAT_END_OF_COMMAND>" – "%c%c".

Receiver: Need to put all the IPs and port numbers of your public connections in a comma delimited string and send them back as a connection list.  "<CHAT_CONNECTION_LIST><ip addresses and ports><CHAT_END_OF_COMMAND>" – "%c%s%c".  The ip/port string looks like "<ip address>,<port>,<ip address>,<port>,…>".  If a connection is missing an IP address the address in the string should say "<Unknown>".  A sample string might looks something like this: "28.25.102.48,4050,100.284.27.65,4000,<Unknown>,4050"

CHAT_CONNECTION_LIST
This is a result from a connection request.  The IP addresses and port numbers need to be parsed out of the string and an attempt made to connect them.  See CHAT_REQUEST_CONNECTIONS for the format of the string.

CHAT_TEXT_EVERYBODY
Sender: <CHAT_TEXT_EVERYBODY><text to send><CHAT_END_OF_COMMAND>
Used to send some chat text to everybody.  All the text you want to be displayed needs to be generated on the sender's side, including the line feeds and the "<chat name> chats to everybody" string. "\n%s chats to everybody, '%s'\n"  The reason for sending all the text rather than having the receiving side generate the "<chatname> chats to everybody" is just to be polite  and lift some of the burden from the receiver's side.  The receiver can just print the string that comes in and isn't having to constantly build all the chat strings.  The data should only be sent to chat connections are you not ignoring.

Receiver: If the chat connection isn't being ignored, you simply print the string.  If you have any connections marked as being served you need to echo this string to those connections.  Or if this is coming from a connection being served, you need to echo to all your other connections.  This allows people who cannot connect directly to each other to connect with a 3rd person who *can* connect to both and be a server for them.

CHAT_TEXT_PERSONAL
Sender: <CHAT_TEXT_PERSONAL><text to send><CHAT_END_OF_COMMAND>
This works the same way as CHAT_TEXT_EVERYBODY as far as what you need to send.  The text should obviously be changed so the person receiving knows this was a personal chat and not broadcast to everybody.  "\n%s chats to you, '%s'\n"

Receiver: Just print the string that comes in if you aren't ignoring this connection.

CHAT_TEXT_GROUP
Sender: <CHAT_TEXT_GROUP><group><text to send><CHAT_END_OF_COMMAND>
Used when you send text to a specific group of connections.  Works basically the same as the other text commands.  The group name is a 15 character string.  If *must* be 15 characters long – pad it on the right with spaces to fill it out.  "\n%s chats to the group, '%s'\n"

Receiver: Just print the string that comes in if you aren't ignoring this connection.

CHAT_MESSAGE
Sender: <CHAT_MESSAGE><message><CHAT_END_OF_COMMAND>
This is used to send a message to another chat connection.  An example of this is when you try to send a command (action, alias, etc…) to another chat connection and they don't have you flagged as accepting commands.  In that case a chat message is sent back to the sender telling them that command are not being accepted.  To let the other side know the message is generated from the chat program it is a good idea to make the string resemble something like: "\n<CHAT> %s is not allowing commands.\n"

Receiver: Just print the message string.

CHAT_VERSION
Sender:	 <CHAT_VERSION><version string><CHAT_END_OF_COMMAND>

CHAT_FILE_START
Sender: <CHAT_FILE_START><filename,length><CHAT_END_OF_COMMAND>
This is sent to start sending a chat connection a file.  The filename should be just the filename and not a path.  Length is the size of the file in bytes.

Receiver: First should check to make sure you are allowing files from this connection.  Make sure the filename is valid and that the length was trasnmitted.  MM by default won't allow you to overwrite files; which keeps people from messing with file already in your directory.  If for any reason the data isn't valid or you don't want to accept files from this person a CHAT_FILE_DENY should be sent back to abort the transfer.  If you want to continue with the transfer you need to start it off by requesting a block of data with CHAT_FILE_BLOCK_REQUEST.

CHAT_FILE_DENY
Sender: <CHAT_FILE_DENY><message><CHAT_END_OF_COMMAND>
This is used when a CHAT_FILE_START has been received and you want to prevent the transfer from continuing.  <message> is a string telling the reason it was denied.  For example, if the file already existed you might deny it with: "File already exists."

Receiver: Print the deny message.  Deal with cleaning up any files you opened when you tried to start the transfer.

CHAT_FILE_BLOCK_REQUEST
Sender: <CHAT_FILE_BLOCK_REQUEST><CHAT_END_OF_COMMAND>
Sent to request the next block of data in a transfer.

Receiver: Need to create a file block to be sent back.  File blocks are fixed length so they don't need the CHAT_END_OF_COMMAND byte.  If the end of file is reached need to send a CHAT_FILE_END close up the files and let the user know it is done sending.

CHAT_FILE_BLOCK
Sender: <CHAT_FILE_BLOCK><block of data>
A file block is 500 bytes.  A file block is ALWAYS 500 bytes so no CHAT_END_OF_COMMAND is needed.

Receiver: The receiver needs to keep track of the number of bytes written to properly write the last block of data.  If you keep track of the bytes written you know when to expect that last block that probably doesn't have a full 500 bytes to be saved.  File transfers are receiver driven, so for each block of data you accept, you need to send another CHAT_FILE_BLOCK_REQUEST back out to get more data.

CHAT_FILE_END
Sender: <CHAT_FILE_END><CHAT_END_OF_COMMAND>

Receiver: Close up your files and be done with it.

CHAT_FILE_CANCEL
Sender: <CHAT_FILE_CANCEL><CHAT_END_OF_COMMAND>
Either side can send this command to abort a file transfer in progress.

CHAT_PING_REQUEST
Sender: <CHAT_PING_REQUEST><timing data><CHAT_END_OF_COMMAND>
The timing data is really up to the ping requester.  MM uses the number of clocks ticks elapsed to determine the length of a ping.  Your timing data is sent right back to you from the side receiving the ping so you can use anything you want to indicate the length of a ping.  The reason for sending the ping data was to keep the code simple.  If rather than sending the data you keep track of it in your program, you'd have to build some sort of object that keeps track of all the pings you have sent and their start times.  This way you don't have to keep track of the start time, it's given right back to you when the ping is returned.

Receiver: When a request is received send the timing data right back in a CHAT_PING_RESPONSE block.

CHAT_PING_RESPONSE
Sender: <CHAT_PING_RESPONSE><timing data><CHAT_END_OF_COMMAND>
The timing data is just the data that was sent to you with the CHAT_PING_REQUEST.

*/


/* Basic connection structure. */
typedef struct connection_data CONN;

struct connection_data
{
   char *name;
   int fd;
   
   int accepted;
   
   char *version;
   
   char *filename;
   int file_bytes_sent;
   int file_size;
   
   DESCRIPTOR *desc;
   CONN *next;
};


CONN *connections;


/* Here we register our functions. */

void mmchat_module_init_data( );
int  mmchat_process_client_aliases( char *cmd );


ENTRANCE( mmchat_module_register )
{
   self->name = strdup( "MMChat" );
   self->version_major = mmchat_version_major;
   self->version_minor = mmchat_version_minor;
   self->id = mmchat_id;
   
   self->init_data = mmchat_module_init_data;
   self->process_server_line_prefix = NULL;
   self->process_server_line_suffix = NULL;
   self->process_server_prompt_informative = NULL;
   self->process_server_prompt_action = NULL;
   self->process_client_command = NULL;
   self->process_client_aliases = mmchat_process_client_aliases;
   self->build_custom_prompt = NULL;
   self->main_loop = NULL;
   self->update_descriptors = NULL;
   
   GET_FUNCTIONS( self );
   
   mm_self = self;
}



/* Return a pointer to our version string. */
char *mmchat_module_version( )
{
   static char version[256];
   
   sprintf( version, C_B "MMChat v" C_G "%d" C_B "." C_G "%d" C_B,
	    mmchat_version_major, mmchat_version_minor );
   
   return version;
}



void mmchat_module_init_data( )
{
   DEBUG( "mmchat_init_data" );
   
}



void mm_send_packet( CONN *conn, char type, char *packet )
{
   char buf[4096];
   
   sprintf( buf, "%c%s%c", type, packet, (char) CHAT_END_OF_COMMAND );
   
   c_write( conn->desc->fd, buf, strlen( buf ) );
}


void mm_send_version( CONN *conn )
{
   char buf[256];
   
   sprintf( buf, "MudBot Chat - v%d.%d",
	    mmchat_version_major, mmchat_version_minor );
   
   mm_send_packet( conn, (char) CHAT_VERSION, buf );
}



void mm_send_file_block( CONN *conn )
{
   FILE *fl;
   char block[512];
   int length;
   
   if ( !conn->filename )
     {
	mm_send_packet( conn, (char) CHAT_MESSAGE,
			"\nSend you a block? From what file?\n" );
	mm_send_packet( conn, (char) CHAT_FILE_CANCEL, "" );
	return;
     }
   
   fl = fopen( conn->filename, "r" );
   
   if ( !fl )
     {
	mm_send_packet( conn, (char) CHAT_MESSAGE,
			"\nNo longer have access to that file.\n" );
	mm_send_packet( conn, (char) CHAT_FILE_CANCEL, "" );
	clientff( "No longer have access to that file. "
		  "Cancelling the transfer to " C_W "%s" C_0 ".\r\n",
		  conn->name );
	
	free( conn->filename );
	conn->filename = NULL;
	
	return;
     }
   
   if ( fseek( fl, conn->file_bytes_sent, SEEK_SET ) )
     {
	mm_send_packet( conn, (char) CHAT_MESSAGE,
			"\nInternal seek error, unable to send the file.\n" );
	mm_send_packet( conn, (char) CHAT_FILE_CANCEL, "" );
	clientff( "Internal error, file transfer to " C_W " %s" C_0
		  "cancelled.\r\n", conn->name );
	fclose( fl );
	
	free( conn->filename );
	conn->filename = NULL;
	
	return;
     }
   
   length = fread( block+1, 1, 500, fl );

   fclose( fl );
   
   /* End of the file? */
   if ( !length )
     {
	if ( conn->file_bytes_sent != conn->file_size )
	  {
	     mm_send_packet( conn, (char) CHAT_MESSAGE,
			     "\nUnexpected end of file, transfer cancelled.\n" );
	     mm_send_packet( conn, (char) CHAT_FILE_CANCEL, "" );
	     clientff( "Unexpected end of file, file transfer cancelled.\r\n" );
	     
	     free( conn->filename );
	     conn->filename = NULL;
	     
	     return;
	  }
	
	mm_send_packet( conn, (char) CHAT_FILE_END, "" );
	
	free( conn->filename );
	conn->filename = NULL;
	
	clientff( "\r\n" C_W "%s" C_0 ": File transfer completed.\r\n",
		  conn->name );
	show_prompt( );
	return;
     }
   
   if ( !conn->accepted )
     {
	conn->accepted = 1;
	clientff( "\r\n" C_W "%s" C_0 ": File transfer started.\r\n",
		  conn->name );
	show_prompt( );
     }
   
   conn->file_bytes_sent += length;
   
   /* If it's not a full block, fill the rest with 0's. */
   
   if ( length != 500 )
     {
	int i;
	
	for ( i = length + 1; i < 501; i++ )
	  block[i] = 0;
     }
   
   block[0] = CHAT_FILE_BLOCK;
   block[501] = CHAT_END_OF_COMMAND;
   block[502] = 0;
   
   c_write( conn->desc->fd, block, 502 );
}


void mm_process_packet( CONN *conn, char *packet )
{
   char type;
   
   type = packet[0];
   
   packet++;
   
   if ( !type )
     {
	clientfr( "Empty packet?!" );
	return;
     }
   
   if ( type == (char) CHAT_TEXT_PERSONAL )
     {
	/* Just display it. */
	
	clientff( C_W "%s" C_0, packet );
	show_prompt( );
	return;
     }
   else if ( type == (char) CHAT_TEXT_EVERYBODY )
     {
	/* Same. */
	
	clientff( C_R "%s" C_0, packet );
	show_prompt( );
     }
   else if ( type == (char) CHAT_MESSAGE )
     {
	/* Say from who it's from, and display it. */
	clientff( C_R "\r\nChat message from %s:%s" C_W "%s\r\n" C_0,
		  conn->name, packet[0] == '\n' ? "" : "\r\n", packet );
	show_prompt( );
     }
   else if ( type == (char) CHAT_NAME_CHANGE )
     {
	clientff( C_R "\r\n[" C_W "%s" C_R " will now be known as " C_W "%s"
		  C_R ".]\r\n", conn->name, packet );
	if ( conn->name )
	  free( conn->name );
	conn->name = strdup( packet );
     }
   else if ( type == (char) CHAT_VERSION )
     {
	/* Put it in the connection's version pointer. */
	
	if ( conn->version )
	  free( conn->version );
	conn->version = strdup( packet );
     }
   else if ( type == (char) CHAT_PING_REQUEST )
     {
	/* Return a ping reply, containg the same message. */
	
	mm_send_packet( conn, (char) CHAT_PING_RESPONSE, packet );
	
	clientff( C_R "\r\n<CHAT> Ping from " C_W "%s" C_R ". Reply sent.\r\n" C_0,
		  conn->name );
	show_prompt( );
     }
   else if ( type == (char) CHAT_SNOOP_DATA )
     {
	char cfg[3], cbg[3];
	int fg, bg;
	
	if ( strlen( packet ) < 4 )
	  return;
	
	cfg[0] = packet[0];
	cfg[1] = packet[1];
	cfg[2] = 0;
	cbg[0] = packet[2];
	cbg[1] = packet[3];
	cbg[2] = 0;
	
	fg = atoi( cfg );
	bg = atoi( cbg );
	clientff( "%s", packet+4 );
     }
   else if ( type == (char) CHAT_FILE_BLOCK_REQUEST )
     {
	mm_send_file_block( conn );
     }
   else if ( type == (char) CHAT_FILE_DENY )
     {
	if ( conn->filename )
	  free( conn->filename );
	conn->filename = NULL;
	clientff( C_W "\r\nFile denied by " C_W "%s" C_0 ": %s\r\n" C_0,
		  conn->name, packet );
	show_prompt( );
     }
   else
     {
	/* Debugging. */
	
	clientff( C_R "\r\n[Packet type: %d.]" C_r "\r\n{%s}\r\n" C_0,
		  (int) type, packet );
	show_prompt( );
     }
   
}



void mm_remove_connection( CONN *conn )
{
   CONN *c;
   
   /* Unlink it from the connections chain. */
   
   if ( conn == connections )
     connections = connections->next;
   else
     {
	for ( c = connections; c; c = c->next )
	  if ( c->next == conn )
	    {
	       c->next = conn->next;
	       break;
	    }
     }
   
   /* Free it up. */
   
   remove_descriptor( conn->desc );
   
   if ( conn->name )
     free( conn->name );
   if ( conn->version )
     free( conn->version );
   free( conn );
}



void mm_read_from_connection( DESCRIPTOR *self )
{
   CONN *conn;
   char buf[4096];
   int bytes;
   static char data_block[4096];
   static char *d;
   char *b;
   
   /* Find the connection structure. */
   for ( conn = connections; conn; conn = conn->next )
     {
	if ( conn->fd == self->fd )
	  break;
     }
   if ( !conn )
     {
	clientfr( "Got a callback on an unregistered connection. Closing." );
	c_close( self->fd );
	mm_remove_connection( conn );
	return;
     }
   
   bytes = c_read( self->fd, buf, 4096 );
   
   if ( bytes == 0 )
     {
	c_close( self->fd );
	mm_remove_connection( conn );
	clientff( C_R "\r\n[Connection closed by " C_W "%s" C_R ".]\r\n" C_0,
		  conn->name );
	show_prompt( );
	return;
     }
   if ( bytes < 0 )
     {
	clientff( C_R "\r\n[Error on reading, connection to "
		  C_R "%s" C_W " closed.]\r\n" C_0, conn->name );
	clientfr( "Error on read." );
	c_close( self->fd );
	mm_remove_connection( conn );
	return;
     }
   
   buf[bytes] = '\0';
   
   /* Connection refused? */
   if ( !strncmp( buf, "NO", 2 ) )
     {
	c_close( self->fd );
	mm_remove_connection( conn );
	clientff( C_R "\r\n[Connection rejected.]\r\n" C_0 );
	show_prompt( );
	return;
     }
   
   /* Connection accepted? */
   if ( !strncmp( buf, "YES:", 4 ) )
     {
	char name[512];
	char *n = name, *b = buf + 4;
	
	while ( *b && *b != '\n' )
	  *(n++) = *(b++);
	*n = 0;
	
	if ( conn->name )
	  free( conn->name );
	conn->name = strdup( name );
	conn->accepted = 1;
	
	/* Send our version string. */
	mm_send_version( conn );
	
	clientff( C_R "\r\n[Connection accepted from " C_W "%s" C_R ".]" C_0 "\r\n",
		  name );
	show_prompt( );
	
	return;
     }
   
   /* Proxy stuff. */
   if ( !strncmp( buf, "HTTP/", 5 ) )
     {
	char *b;
	
	/* Ex: HTTP/1.0 404 Not Found\n.... */
	
	/* Connection established. */
	if ( !strncmp( buf + 9, "200 ", 4 ) )
	  return;
	
	/* Find the end of the line, and end the string there. */
	for ( b = buf + 9; *b; b++ )
	  if ( *b == '\n' )
	    {
	       *b = 0;
	       break;
	    }
	
	clientff( C_R "[Proxy error: " C_r "%s" C_R ".]" C_0 "\r\n",
		  buf + 9 );
	return;
     }
   
   /* A normal chat data packet. */
   
   /* Make sure the pointers are looking good. */
   if ( !d )
     d = data_block;
   b = buf;
   
   /* Byte by byte, put each packet into data_block, and process when done. */
   while ( *b )
     {
	*d = *(b++);
	
	if ( *d == (char) CHAT_END_OF_COMMAND )
	  {
	     *(d) = 0;
	     mm_process_packet( conn, data_block );
	     d = data_block;
	     continue;
	  }
	
	d++;
     }
}



int mm_connect( char *args )
{
   DESCRIPTOR *desc;
   CONN *conn;
   char hostname[512];
   char port_string[512];
   char buf[256];
   int port, sock;
   
   args = get_string( args, hostname, 512 );
   get_string( args, port_string, 512 );
   
   port = atoi( port_string );
   /* Default is 4050, so use that if none is supplied. */
   if ( !port )
     port = 4050;
   
   clientfr( "Connecting..." );
   sock = mb_connect( hostname, port );
//   sock = mb_connect( proxy_host, proxy_port );
   
   if ( sock < 0 )
     {
	sprintf( buf, "Failed: %s.", get_connect_error( ) );
	clientfr( buf );
	return 0;
     }
   
   clientfr( "Done." );
   
   /* Connected! Build a connection structure, now. */
   conn = calloc( 1, sizeof( CONN ) );
   
   conn->fd = sock;
   conn->name = strdup( "Unknown" );
   conn->accepted = 0;
   
   conn->next = connections;
   connections = conn;
   
   /* Proxy stuff. */
//   sprintf( buf, "CONNECT %s:%d HTTP/1.0\r\n\r\n",
//	    hostname, port );
//   c_write( sock, buf, strlen( buf ) );
   
   /* Send the connection request, over the socket. */
   sprintf( buf, "CHAT: %s\n%s%-5u", chat_name, chat_ip, chat_port );
   c_write( sock, buf, strlen( buf ) );
   
   /* Put it in MudBot's connection list. */
   desc = calloc( 1, sizeof( DESCRIPTOR ) );
   desc->name = strdup( "Chat" );
   desc->description = strdup( "MudMaster Chat connection." );
   desc->mod = NULL;
   desc->fd = conn->fd;
   desc->callback_in = mm_read_from_connection;
   add_descriptor( desc );
   
   conn->desc = desc;
   
   return 1;
}



void mm_chat( char *args )
{
   CONN *conn;
   char name[256];
   char buf[512];
   
   args = get_string( args, name, 256 );
   
   if ( !name[0] )
     {
	clientfr( "Chat to who? Check c/list." );
	return;
     }
   
   if ( !args[0] )
     {
	clientfr( "Chat what?" );
	return;
     }
   
   for ( conn = connections; conn; conn = conn->next )
     if ( !strcmp( conn->name, name ) )
       break;
   
   if ( !conn )
     {
	clientfr( "No such connection known..." );
	return;
     }
   
   sprintf( buf, "\n%s chats to you, '%s'\n", chat_name, args );
   
   mm_send_packet( conn, (char) CHAT_TEXT_PERSONAL, buf );
   
   clientff( C_W "You chat to %s, '%s'" C_0 "\r\n", conn->name, args );
}


void mm_disconnect( char *args )
{
   CONN *conn;
   
   /* Look up which connection we're talking about. */
   for ( conn = connections; conn; conn = conn->next )
     {
	if ( conn->name && !strcmp( conn->name, args ) )
	  break;
     }
   
   if ( !conn )
     {
	clientfr( "No such connection known." );
	return;
     }
   
   /* Disconnect it... */
   
   c_close( conn->fd );
   mm_remove_connection( conn );
   
   clientfr( "Connection closed, unlinked and destroyed. Happy?" );
}



void mm_list( )
{
   CONN *conn;
   
   if ( !connections )
     {
	clientfr( "There are no established connections." );
	return;
     }
   
   clientfr( "Name            Version" );
   for ( conn = connections; conn; conn = conn->next )
     {
	clientff( " %-15s %s\r\n", conn->name,
		  conn->version ? conn->version : "-unknown-" );
     }
}



void mm_snoop( char *args )
{
   CONN *conn;
   
   for ( conn = connections; conn; conn = conn->next )
     {
	if ( !strcmp( conn->name, args ) )
	  break;
     }
   
   if ( !conn )
     {
	clientff( "Who do you want to snoop?\r\n" );
	return;
     }
   
   mm_send_packet( conn, (char) CHAT_SNOOP, "" );
   
   clientff( C_R "[Snoop request send to " C_W "%s" C_R ".]\r\n", conn->name );
}


void mm_forge( char *args )
{
   CONN *dest;
   char name[256];
   char ctype[256];
   int type;
   
   args = get_string( args, name, 256 );
   args = get_string( args, ctype, 256 );
   
   for ( dest = connections; dest; dest = dest->next )
     {
	if ( !strcmp( dest->name, name ) )
	  break;
     }
   
   if ( !dest )
     {
	clientff( "Send a packet to who?\r\n" );
	return;
     }
   
   type = atoi( ctype );
   
   if ( !type )
     {
	clientff( "What type of packet to send?\r\n" );
	return;
     }
   
   /* Send it away... */
   
   mm_send_packet( dest, (char) type, args );
   
   clientfr( "Packet forged, and sent away." );
}



void mm_sendfile( char *args )
{
   CONN *dest;
   FILE *fl;
   struct stat flstat;
   char name[256], *filename, *p;
   char buf[512];
   
   args = get_string( args, name, 256 );
   
   for ( dest = connections; dest; dest = dest->next )
     {
	if ( !strcmp( dest->name, name ) )
	  break;
     }
   
   if ( !dest )
     {
	clientff( "Send a file to who?\r\n" );
	return;
     }
   
   if ( !args[0] )
     {
	clientff( "Send what file?\r\n" );
	return;
     }
   
   /* Make sure we have it. */
   fl = fopen( args, "r" );
   if ( !fl )
     {
	clientff( "Can't find that file.\r\n" );
	return;
     }
   
   /* Get the file's length. */
   
   if ( fstat( fileno( fl ), &flstat ) )
     {
	clientff( "Unable to stat the file.\r\n" );
	fclose( fl );
	return;
     }
   
   fclose( fl );
   
   /* Make filename point only to the file name. */
   p = args;
   filename = args;
   while( *p )
     {
	if ( *p == '/' || *p == '\\' || *p == ':' )
	  filename = p+1;
	p++;
     }
   
   if ( !filename[0] )
     {
	clientff( "Invalid filename.\r\n" );
	return;
     }
   
   if ( dest->filename )
     {
	clientff( "Already sending a file to that connection.\r\n" );
	return;
     }
   
   dest->filename = strdup( args );
   dest->file_bytes_sent = 0;
   dest->file_size = flstat.st_size;
   dest->accepted = 0;
   
   clientff( "Offering to send " C_W "%s" C_0 " (" C_G "%d" C_0 " bytes) "
	     "to " C_W "%s" C_0 ".\r\n", filename, dest->file_size, dest->name );
   sprintf( buf, "%s,%d", filename, dest->file_size );
   mm_send_packet( dest, (char) CHAT_FILE_START, buf );
}



void mm_filestatus( )
{
   CONN *conn;
   int first = 1;
   
   for ( conn = connections; conn; conn = conn->next )
     {
	if ( first )
	  {
	     clientfr( "File transfers in progress:" );
	     first = 0;
	  }
	
	if ( conn->filename )
	  {
	     clientff( C_W "%s" C_0 " - " C_W "'" C_0 "%s" C_W "'" C_0 " - "
		       C_G "%d" C_0 " of " C_G "%d" C_0 ".\r\n", conn->name, conn->filename,
		       conn->file_bytes_sent, conn->file_size );
	  }
     }
   
   if ( first )
     clientfr( "There are no transfers in progress, currently." );
}



int mmchat_process_client_aliases( char *line )
{
   char command[4096];
   char *args;
   
   DEBUG( "mmchat_process_client_aliases" );
   
   /* Module commands look like this:
    *  chat <person>/command [<text>/[args]]
    *  c <person>/command [<text>/[args]]
    *  c/<command> [args]
    */
   
   if ( !strncmp( line, "chat ", 5 ) )
     line += 6;
   else if ( !strncmp( line, "c/", 2 ) )
     line += 2;
   else if ( !strncmp( line, "c ", 2 ) )
     line += 2;
   else
     return 0;
   
   args = get_string( line, command, 4096 );
   
   if ( !strcmp( command, "version" ) || !strcmp( command, "ver" ) )
     {
	char buf[256];
	
	sprintf( buf, "MudMaster Chat module, version %d.%d.",
		 mmchat_version_major, mmchat_version_minor );
	clientfr( buf );
     }
   else if ( !strcmp( command, "call" ) )
     {
	mm_connect( args );
     }
   else if ( !strcmp( command, "unchat" ) || !strcmp( command, "disconnect" ) )
     {
	mm_disconnect( args );
     }
   else if ( !strcmp( command, "chat" ) || !strcmp( command, "tell" ) )
     {
	mm_chat( args );
     }
   else if ( !strcmp( command, "list" ) || !strcmp( command, "who" ) )
     {
	mm_list( );
     }
   else if ( !strcmp( command, "snoop" ) )
     {
	mm_snoop( args );
     }
   else if ( !strcmp( command, "forge" ) )
     {
	mm_forge( args );
     }
   else if ( !strcmp( command, "send" ) || !strcmp( command, "sendfile" ) )
     {
	mm_sendfile( args );
     }
   else if ( !strcmp( command, "filestatus" ) )
     {
	mm_filestatus( );
     }
   else
     {
	clientfr( "Unknown chat command. Try c/help for some help." );
     }
   
   show_prompt( );
   return 1;
}

