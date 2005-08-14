Table of Contents
=================

1. Introduction
2. Installing
3. MudBot
  3.1. Concepts
  3.2. Commands
4. IMapper
  4.1. Concepts
    4.1.1. Description
    4.1.2. Virtual Numbers
    4.1.3. Current Room
    4.1.4. Map Generator
    4.1.5. Pathfinder
    4.1.6. Speedwalker
    4.1.7. Automapper
  4.2. Commands
    4.1.1. General
    4.1.2. Map commands
    4.1.3. Room commands
    4.1.4. Area commands
    4.1.5. Exit commands
  4.3. Automapping



1. Introduction
===============

The Imperian Modularized Trigger System, IMTS, is composed of the core engine, MudBot, and various modules. The core starts listening on port 123, loads all the modules, then waits for a connection. Once a user connected, it can connect further to a MUD server, and monitor/modify everything that passes through. Every server line and every user command are sent to the modules for processing, before being sent further.


2. Installing
=============

- Open modules.txt, and comment/uncomment any modules you wish or don't wish to load.
- Run the MudBot executable (mb-core.exe)
- Connect with your MUD client at host 'localhost' port '123'
- Once connected, send 'connect imperian.com 23'
- Log in



3. MudBot
=========

3.1. Concepts
-------------

The core, named MudBot, begins listening for connections on port 23. Once a MUD client connected, it will ask the client where to connect to, and once the client sends the 'connect' command, MudBot will connect to the MUD server. This allows MudBot to be the "man-in-the-middle", a proxy that intercepts, examines and may modify the connection data that passes through.

To support MCCP, the Mud Client Compression Protocol, MudBot hides the compression request sequence that comes from the server, and handles the negotiation with the server itself. The server will then send compressed data, MudBot will decompress it, proceed to examine/modify it, then send it uncompressed to the client. Compressed data -must- be uncompressed by MudBot, otherwise it won't understand anything from it, nor would it be able to send data to the client.

To support ATCP, the Achaea Telnet Communication Protocol, MudBot will handle the negotiation itself, logging in as either MudBot or JavaClient. Once logged in, the server will send various extra information, such as character's name and title, current and maximum health/mana/end/will values, the contents of a scroll when editing it, etc.

MudBot is able to load and unload modules on the fly, at the user's request. The 'modules.txt' file contains modules that will be loaded each time MudBot starts. Right after being loaded, MudBot will request information on what functions the core should call from the module. If this fails, the module will be unloaded.

A module, once loaded, will be able to add text before or after a line/prompt is displayed, to gag a line or prompt, to replace the prompt, to prevent client commands from being sent to the MUD server, and to send its own commands.



3.2. Commands
-------------

All commands need to be sent through the MUD client, once it connected to MudBot, and MudBot successfully connected to a MUD server. All commands begin with the ` character (not ').

`help - Will send a brief description of each command in MudBot to the client.

`reboot - Available only on non-Windows systems, this will restart the application while keeping all connections alive. Useful mainly for developers, to load the changes without closing MudBot, and also in case MudBot crashes and the connection was successfully kept alive after the crash.

`disconnect - Close the connection to the MUD server. Once disconnected, MudBot will not understand any commands except "connect <hostname> <port>".

`mccp start - Requires zlib.dll present and MCCP support enabled. This will attempt to begin data compression with the MUD server. If successful, data sent by the MUD server will be compressed, and MudBot will decompress it. Saves bandwidth traffic, at the expense of more CPU usage.

`mccp stop - Send the 'IAC DONT COMPRESS2' sequence to the MUD server. If compression was started, the server will cleanly close it, and data will be sent normally.

`atcp - Display some information gathered through the Achaea Telnet Communication Protocol, if it successfully logged in.

`mods - List all loaded or built-in modules and their versions.

`load - Attempt to load a module. Syntax: `load file_name. (Example: `load ./my_module.dll)

`unload - Unload a module. This is impossible if the module is built in. This will also allow the file to be moved/replaced on Windows systems. Syntax: `unload module_name. (Example: `unload MyModule)

`reload - Unload and load module. This will completely reset the module. Useful on non-Windows systems, to quickly load the changes after replacing the file. On Windows, the module must first be unloaded before it can be replaced, making this command useless.

`timers - List the names of all registered timers that have not yet fired, and how many seconds are left until they fire.

`descriptors - List all registered TCP/IP connections, and what flags they have set. In: The module will be notified each time data is received. Out: The module will be notified when the connection was successful if it was made non-blocking, then each time data can be sent. Exc: Notifies the module each time an error occurs (Exception Raised), and the connection must be forced to close.

`status - Show some MCCP/ATCP/etc information.

`echo - Process a line as if it came from the MUD server, passing it through all the loaded modules. Syntax: `echo text. (Example: `echo Your body stiffens rapidly with paralysis.)

`edit - (Windows only) Open the Editor window, mainly useful for writing scrolls before opening the MUD's editor.

`license - Show the GNU General Public License notice.

`quit - Close all connections, and exit MudBot for good.



4. IMapper
==========

4.1. Concepts
-------------

4.1.1. Description
------------------

The IMapper (i_mapper.so/i_mapper.dll) is a text-based map generator, pathfinder and speedwalker, among other things that make locating/walking much easier.


4.1.2. Current Room
-------------------

The mapper always needs to know where the user is. When the user sends a 'look' or movement command, that command is added in the mapper's command queue. If the command queue is not empty, and a line of a corresponding colour is received, the mapper will treat it as a Room Title. If the mapper already knows where the user is, it will attempt to match the current room with the new Room Title, and if that fails, it sends "(lost)" to the client and enters the "get unlost" mode. In this mode, the client will attempt to match the Room Title with all rooms in its database. If more rooms of that name are found, it will use the room's exits to narrow down the choices. If it successfully found itself, the area name will be shown to the user.


4.1.3 Virtual Numbers
---------------------

A virtual number, commonly known among MUDs as a 'vnum', is a unique number assigned to a room, to allow the user a way to specify a room. The numbers have nothing to do with the way the room database is stored, they can even be random numbers. In IMapper's case, it numbers the rooms ascendingly, in the order they were mapped.


4.1.4. Map Generator
--------------------

The map generator will "draw" a map of the current area (or however much fits) from the current room. The mapper does not store absolute positions of the rooms (this room is at pos. 242x123), but instead stores relative positions (this room is east of this room), therefore a map must be generated each time the user requests one.

Room colours represent the environment type (Urban, Forest, etc) from Survey.
Bright white exits lead to a different area.
Dark exits are unlinked, and need to be mapped.
Red exits have something behind them, but the user chose to hide it because it may not fit on a generated map. ("exit stop")
Blue exits show the path to the destination set via "map path".
Longer exits mean nothing. They're longer just so rooms won't overlap on a generated map. ("exit length <nr>")


4.1.5. Pathfinder
-----------------

Most pathfinders will try to find the shortest route from a source to a destination. Contrary to popular belief, the IMapper will instead build paths from the destination, in reverse, to every other room in the world. This is a much slower algorithm, but once the directions are built, the destination does not need to be set again, as the source is always irrelevant to this pathfinder.

Multiple vnums can be used with the path finder. This will make the speedwalker go to the nearest vnum from the current room.


4.1.6. Speedwalker
------------------

Once the pathfinder is called, each room in the world will contain the direction that leads the user a bit closer to the destination, if such a direction exists. The speedwalker, once enabled, will attempt to go in this direction each time the Current Room changes. It does not wait. This makes you receive a lot of 'Don't be so hasty' messages, but it speedwalks at the highest possible speed that your skills and Internet connection can provide. All 'Don't be so hasty' messages are gagged if and only if speedwalking is enabled.

Because the Current Room is always irrelevant to the pathfinder, it never needs to be re-called on the same destination, even if the user moves (or is moved) manually to a completely different place. Simply start speedwalking again, from wherever you are.

If the user has the ability 'dash' or 'sprint', the speedwalker can make use of them. The commands are "go dash" and "go sprint" instead of "go".

The "stop" command, which stops the speedwalker, will override the MUD's "stop" command only if speedwalking is enabled. Otherwise, the command will be sent to the MUD server.


4.1.7. Automapper
-----------------

The mapper can be in three modes: Disabled, Following, or Mapping. In the default mode, Following, the mapper will always try to find itself on the map. In Mapping mode, any look or movement commands will change/update the map, and all mapping commands are enabled. To create a room, mapping must be enabled, and the user needs to simply walk in a direction. To update an existing room's name, such as a shop, the user must simply walk in it while mapping is on. To link a room to another existing room, the destination room's vnum must be specified, then the user has to walk into that room. Therefore, if the user cannot move, the user cannot map.


4.2. Commands
-------------

4.2.1. General
--------------

With very few exceptions, all mapper commands begin with one of the four base commands: "map", "area", "room" or "exit". Each base command has the "help" subcommand, which lists all other available subcommands ("map help", "area help", etc).

The base command can be abbreviated using its first letter in uppercase, or its first letter in lowercase followed by a space. The subcommand can be abbreviated using its first few letters in lowercase. As an example, valid ways to use the "map path 103" command can be:
 - "map path 103"
 - "m path 103"
 - "m pat 103"
 - "m p 103"
 - "Mpat 103"
 - "Mp 103"

As an exception, the "exit link 103" command can be abbreviated to "E103" (and NOT "E 103" or "El 103").

